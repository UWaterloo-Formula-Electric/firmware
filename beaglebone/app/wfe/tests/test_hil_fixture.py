"""Host protocol tests; these do not validate physical HIL output voltages."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from hil_fixture import HIL_BASELINE_COMMIT, HilFixture, POINTS, load_profile


def recipe():
    # Synthetic command only: the fake UART below supplies its acknowledgment.
    return {"commands": [{"command": "testOutput 0", "expect": "Output set"}], "settle_ms": 0}


def profile():
    result = {"board": "STM32 HIL_2027", "verified": True,
              "fixture_revision": "fake UART", "hil_firmware_commit": HIL_BASELINE_COMMIT,
              "idle": recipe(), "points": {}}
    for point in POINTS:
        result["points"][point] = {**recipe(), "expected": 0}
    return result


class FakePort:
    def __init__(self):
        self.sent = []
        self.pending = b""
        self.ack = b"Output set\nHIL > "
        self.commit = HIL_BASELINE_COMMIT

    def reset_input_buffer(self):
        self.pending = b""

    def write(self, data):
        self.sent.append(data)
        self.pending = b"HIL > " if data == b"\n" else self.ack
        if data == b"version\n":
            self.pending = f"Commit Hash: {self.commit}\r\nHIL > ".encode("ascii")
        return len(data)

    def read(self, count):
        # Deliberately split the prompt across reads.
        result, self.pending = self.pending[:3], self.pending[3:]
        return result


class FixtureTests(unittest.TestCase):
    def load(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "profile.json"
            path.write_text(json.dumps(data))
            return load_profile(path)

    def test_complete_profile(self):
        self.assertEqual(len(self.load(profile())["points"]), 20)

    def test_incomplete_and_malformed_profiles(self):
        for mutate in (
            lambda p: p.update(verified=False),
            lambda p: p.update(fixture_revision=""),
            lambda p: p.update(hil_firmware_commit="main"),
            lambda p: p["points"].pop("T03.P1"),
            lambda p: p["idle"].update(commands=[]),
            lambda p: p["idle"].update(settle_ms=float("nan")),
            lambda p: p["idle"]["commands"][0].update(command="reset"),
            lambda p: p["idle"]["commands"][0].update(command="one\ntwo"),
            lambda p: p["points"]["T03.P1"].update(expected=float("inf")),
        ):
            data = profile()
            mutate(data)
            with self.subTest(data=data), self.assertRaises(ValueError):
                self.load(data)

    def test_ready_only_after_acknowledged_output(self):
        port = FakePort()
        fixture = HilFixture(port, profile())
        with patch("hil_fixture.time.sleep") as sleep:
            self.assertEqual(fixture.reply(b"HIL T03 P1/3 E0 tol2. Reply ready/skip/abort + Enter.",
                                           lambda: self.fail("manual prompt")), "ready")
            sleep.assert_called_once_with(0)
        self.assertEqual(port.sent, [b"\n", b"testOutput 0\n"])

    def test_ack_must_be_complete_line_and_success(self):
        for ack in (b"Not Output set\nHIL > ", b"Output set\nWrite failed\nHIL > "):
            port = FakePort()
            port.ack = ack
            fixture = HilFixture(port, profile())
            self.assertEqual(fixture.reply(b"HIL T03 P1/3 E0 tol2. ", lambda: "pass"), "abort")

    def test_unknown_point_and_expectation_mismatch_do_not_write(self):
        for prompt in (b"HIL T99 P1/3 E0 tol2. ", b"HIL T03 P1/3 E50 tol2. "):
            port = FakePort()
            self.assertEqual(HilFixture(port, profile()).reply(prompt, lambda: "pass"), "abort")
            self.assertEqual(port.sent, [])

    def test_fractional_saved_expectation_allows_float32_rounding_only(self):
        data = profile()
        data["points"]["T03.P1"]["expected"] = 33.33333
        port = FakePort()
        fixture = HilFixture(port, data)
        self.assertEqual(fixture.reply(b"HIL T03 P1/3 E33.3333282 tol2. ", lambda: "pass"), "ready")
        port.sent.clear()
        self.assertEqual(fixture.reply(b"HIL T03 P1/3 E33.334 tol2. ", lambda: "pass"), "abort")
        self.assertEqual(port.sent, [])

    def test_old_firmware_does_not_guess(self):
        port = FakePort()
        self.assertEqual(HilFixture(port, profile()).reply(
            b"Point 1/3 Reply ready/skip/abort + Enter.", lambda: "ready"), "abort")
        self.assertEqual(port.sent, [])

    def test_timeout_does_not_retry(self):
        port = FakePort()
        port.ack = b""
        fixture = HilFixture(port, profile())
        # Sync response succeeds, then the command response expires.
        with patch("hil_fixture.time.monotonic", side_effect=[0, 0, 0, 0, 4]):
            self.assertEqual(fixture.reply(b"HIL T03 P1/3 E0 tol2. ", lambda: "pass"), "abort")
        self.assertEqual(port.sent, [b"\n", b"testOutput 0\n"])

    def test_idle_before_manual_and_at_finish(self):
        port = FakePort()
        fixture = HilFixture(port, profile())
        fixture.start()
        self.assertEqual(fixture.reply(b"Perform subcases; reply read/pass/fail/skip/abort, then Enter.",
                                       lambda: "skip"), "skip")
        fixture.finish()
        self.assertFalse(fixture.active)
        self.assertEqual(port.sent.count(b"testOutput 0\n"), 3)

    def test_wrong_firmware_rejected_before_output_changes(self):
        port = FakePort()
        port.commit = "0" * 40
        fixture = HilFixture(port, profile())
        with self.assertRaisesRegex(RuntimeError, "pinned fixture revision"):
            fixture.start()
        fixture.finish()
        self.assertEqual(port.sent, [b"\n", b"version\n"])
        self.assertFalse(fixture.active)

    def test_idle_attempts_remaining_commands_after_rejection(self):
        port = FakePort()
        data = profile()
        data["idle"]["commands"].append(copy.deepcopy(data["idle"]["commands"][0]))
        fixture = HilFixture(port, data)
        fixture.active = True
        port.ack = b"Write failed\nHIL > "
        with self.assertRaisesRegex(RuntimeError, "idle restoration failed"):
            fixture.finish()
        self.assertFalse(fixture.active)
        self.assertEqual(port.sent.count(b"testOutput 0\n"), 2)

    def test_can_pipeline_forwards_hil_ack_and_failure(self):
        from uart_over_can import UartOverCanInterface
        for success in (True, False):
            port = FakePort()
            if not success:
                port.ack = b"Write failed\nHIL > "
            fixture = HilFixture(port, profile())
            console = UartOverCanInterface.__new__(UartOverCanInterface)
            sent = []
            frames = iter([bytearray(b"\n"), bytearray(
                b"HIL T03 P1/3 E0 tol2. Reply ready/skip/abort + Enter.\r\n"),
                bytearray(b"OVERALL INCOMPLETE/FAIL\r\n"), bytearray(b"vcu_F7 >")])
            console.can_bus = types.SimpleNamespace(flush_tx_buffer=lambda: None)
            console.send = lambda kind, data: sent.append(data)
            console.fast_single_response = lambda filters: next(frames)
            list(console.fast_full_response("vcuTest run", "vcu_F7 >", [],
                 prompt_reader=lambda prompt: fixture.reply(prompt, lambda: "pass")))
            self.assertEqual(sent, ["vcuTest run\n", "ready\n" if success else "abort\n"])


if __name__ == "__main__":
    unittest.main()
