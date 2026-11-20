"""HIL_2027 UART command recipes for the VCU bench runner.

Recipes come from a verified fixture profile, never ADC-count-to-voltage guesses.
DAC commands use the HIL firmware revision pinned by the fixture profile.
"""
import json
import math
import re
import time


POINTS = {f"T{test:02d}.P{point}"
          for test, count in ((3, 3), (4, 3), (5, 3), (8, 3), (9, 3), (10, 2), (12, 3))
          for point in range(1, count + 1)}
REQUEST = re.compile(rb"HIL (T\d{2}) P(\d+)/(\d+) E([^ ]+) ")
HIL_BASELINE_COMMIT = "98b1298f13a70b03fd5c96eac65bf2cf516fd732"


def _recipe(recipe):
    if not isinstance(recipe, dict):
        raise ValueError("HIL recipe must be an object")
    commands = recipe.get("commands")
    if not isinstance(commands, list) or not 1 <= len(commands) <= 10:
        raise ValueError("Each HIL recipe needs 1-10 acknowledged commands")
    for item in commands:
        if not isinstance(item, dict):
            raise ValueError("HIL command must be an object")
        for key in ("command", "expect"):
            value = item.get(key)
            if (not isinstance(value, str) or not value.strip()
                    or len(value) > 90 or not value.isascii()
                    or any(ord(c) < 32 or ord(c) == 127 for c in value)):
                raise ValueError(f"Invalid single-line HIL {key}")
        if item["command"].split()[0] in ("reset", "vcuTest"):
            raise ValueError("Reset and nested test commands are not stimulus recipes")
    settle = recipe.get("settle_ms")
    if (type(settle) not in (int, float) or not math.isfinite(settle)
            or not 0 <= settle <= 5000):
        raise ValueError("HIL settle_ms must be finite and between 0 and 5000")


def load_profile(path):
    with open(path, encoding="utf-8") as source:
        profile = json.load(source)
    if (not isinstance(profile, dict)
            or profile.get("board") != "STM32 HIL_2027"
            or profile.get("verified") is not True
            or not isinstance(profile.get("fixture_revision"), str)
            or not profile["fixture_revision"].strip()):
        raise ValueError("Requires a verified STM32 HIL_2027 fixture profile and revision")
    points = profile.get("points")
    if not re.fullmatch(r"[0-9a-f]{40}", str(profile.get("hil_firmware_commit", ""))):
        raise ValueError("Fixture profile must pin the full HIL firmware commit")
    if not isinstance(points, dict) or set(points) != POINTS:
        raise ValueError("Fixture profile must define all 20 sensor points exactly")
    for recipe in [profile.get("idle"), *points.values()]:
        _recipe(recipe)
    for recipe in points.values():
        expected = recipe.get("expected")
        if type(expected) not in (int, float) or not math.isfinite(expected):
            raise ValueError("Each sensor point needs a finite expected VCU reading")
    return profile


class HilFixture:
    """Use the HIL's real FreeRTOS CLI over debug UART4 at 115200 baud.

    A response must contain the configured acknowledgment as a complete line
    and end at the HIL prompt before the VCU is allowed to sample the point.
    No automatic retries: a lost acknowledgment leaves output state uncertain.
    """
    PROMPT = b"HIL > "

    def __init__(self, port, profile):
        self.port = port
        self.profile = profile
        self.active = False

    def _response(self):
        deadline = time.monotonic() + 3
        response = bytearray()
        while time.monotonic() < deadline:
            response.extend(self.port.read(256))
            if len(response) > 16384:
                raise RuntimeError("HIL response exceeded limit")
            if response.endswith(self.PROMPT):
                return bytes(response[:-len(self.PROMPT)]).decode("ascii", errors="strict")
        raise TimeoutError("HIL command acknowledgment/prompt timed out")

    def _send(self, command):
        data = (command + "\n").encode("ascii")
        if self.port.write(data) != len(data):
            raise IOError("Incomplete HIL UART write")

    def _apply(self, recipe, best_effort=False):
        errors = []
        for item in recipe["commands"]:
            try:
                # Synchronize first so a stale success line cannot acknowledge
                # this command. A missed prompt aborts rather than retrying.
                self.port.reset_input_buffer()
                self._send("")
                self._response()
                self._send(item["command"])
                response = self._response()
                lines = [line.strip() for line in response.splitlines()]
                if (item["expect"] not in lines
                        or any(re.search(r"\b(failed|error)\b", line, re.I) for line in lines)):
                    raise RuntimeError(f"HIL rejected {item['command']!r}: {response!r}")
                print(f"HIL acknowledged: {item['command']}", flush=True)
            except Exception as error:
                if not best_effort:
                    raise
                errors.append(str(error))
        time.sleep(recipe["settle_ms"] / 1000)
        if errors:
            raise RuntimeError("HIL idle restoration failed: " + "; ".join(errors))

    def start(self):
        # The local remote-tracking branch may be stale. Verify the flashed
        # board against the selected upstream revision before changing outputs.
        self.port.reset_input_buffer()
        self._send("")
        self._response()
        self._send("version")
        response = self._response()
        expected = "Commit Hash: " + self.profile["hil_firmware_commit"]
        if expected not in [line.strip() for line in response.splitlines()]:
            raise RuntimeError("HIL firmware does not match the pinned fixture revision")
        self.active = True  # Restoration must run even if initial idle fails.
        self._apply(self.profile["idle"])

    def reply(self, prompt, manual_reader):
        request = REQUEST.search(prompt)
        if request:
            key = f"{request[1].decode('ascii')}.P{int(request[2])}"
            try:
                recipe = self.profile["points"][key]
                # Firmware stores float32, while JSON uses Python doubles.
                # Permit only float32 representation error, not test tolerance.
                if not math.isclose(float(request[4]), recipe["expected"],
                                    rel_tol=1e-7, abs_tol=1e-7):
                    raise ValueError("VCU expectation differs from fixture profile")
                self._apply(recipe)
                return "ready"
            except Exception as error:
                print(f"HIL STOP: {error}", flush=True)
                return "abort"
        if b"Reply ready/skip/abort + Enter." in prompt:
            # Old firmware lacks point IDs: never guess based on text/order.
            print("HIL STOP: VCU firmware lacks HIL point identifiers", flush=True)
            return "abort"
        self._apply(self.profile["idle"])
        return manual_reader()

    def finish(self):
        if self.active:
            try:
                self._apply(self.profile["idle"], best_effort=True)
            finally:
                self.active = False
