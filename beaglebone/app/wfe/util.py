import pkg_resources

def default_dbc_path():
    return pkg_resources.resource_filename(__name__, "/home/wfe/data/2018CAR.dbc")

def default_dtc_path():
    return pkg_resources.resource_filename(__name__, "/home/wfe/data/DTC.csv")
