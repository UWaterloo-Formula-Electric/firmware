from typing import Dict, List
from datetime import datetime


class DTC:
    def __init__(self, code: int, severity: int, data: int, time: datetime):
        self.code = code
        self.data = data
        self.severity = severity
        self.was_read = False
        self.time_logged = time

    def read(self):
        self.was_read = True
        return self.data


class DTCLogger:
    def __init__(self, can_db):
        self.dtc_log: Dict[int, List[DTC]] = {}  # {code : List of DTC objects}
        self.db = can_db

    def log_dtc(self, msg):
        decoded_dtc = self.db.decode_message(
            msg.arbitration_id,
            msg.data,
            allow_truncated=True,
            decode_choices=False,
            decode_containers=False
        )

        code = decoded_dtc['DTC_CODE']
        severity = decoded_dtc['DTC_Severity']
        data = decoded_dtc['DTC_Data']

        received_dtc = DTC(code=code, severity=severity,
                           data=data, time=datetime.now())

        if code not in self.dtc_log:
            self.dtc_log[code] = []

        self.dtc_log[code].append(received_dtc)

    def get_dtc_structs(self, code: int) -> bool:
        assert self.has_dtc(code), f"DTC Code {code} not found in DTC Log"

        # Given a DTC Code, return a list of unread DTC objects themselves
        dtcs = []
        for dtc in reversed(self.dtc_log[code]):
            if not dtc.was_read:
                dtcs.append(dtc)
                dtc.read()
        return dtcs

    def get_dtc_data(self, code: int) -> List[int]:
        assert self.has_dtc(code), f"DTC Code {code} not found in DTC Log"

        # Given a DTC Code, return a list of DTC data received with that code
        dtcs_data = []
        for dtc in reversed(self.dtc_log[code]):
            if not dtc.was_read:
                dtcs_data.append(dtc.read())
        return dtcs_data

    def has_dtc(self, code: int) -> bool:
        # Check if a DTC code was logged
        return code in self.dtc_log

    def list_dtcs(self) -> List[int]:
        # Return a list of all DTC codes that were logged
        # Note: does not provide any info on how many times they were logged
        return list(self.dtc_log.keys())

    def reset_logger(self) -> None:
        # Clear all DTCs from the log
        self.dtc_log = {}
