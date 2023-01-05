#include "dashboard.h"

int main(int argc, char **argv)
{
    // Run this to make the Makefile:
    // qmake -makefile -o Makefile dashboard.pro

    WfeDashboard wfe_dash = WfeDashboard(argc, argv, "can1", "/home/debian/firmware/common/Data/DTC.csv");

    return 0;
}
