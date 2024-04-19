import pkg_resources


def default_dbc_path():
    return "/home/debian/firmware/common/Data/2024CAR.dbc"


def default_dtc_path():
    return "/home/debian/firmware/common/Data/DTC.csv"


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
