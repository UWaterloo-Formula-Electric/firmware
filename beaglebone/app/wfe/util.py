import pkg_resources

def default_dbc_path():
    return pkg_resources.resource_filename(__name__, "../../../../common/Data/2018CAR.dbc")
