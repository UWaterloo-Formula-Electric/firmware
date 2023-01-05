#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <string>
#include <vector>
#include "dashboardUI.h"

class WfeDashboard {

    private:
        std::shared_ptr<WfeDashboardUI> wu;
        std::shared_ptr<QueueData> data;


    public:
        // thread related variables & functions
        std::vector<std::string> dtc_list;
        char* canInterface;

        static int run (
            std::shared_ptr<QueueData> data,
            const char* canInterface,
            std::vector<std::string> dtc_list
        );

        static double getSpeedKphFromRPM(double newSpeedRPM, double currentSpeedKph);
        void createDtcArray(const char* fname);
        static double roundToOneDecimal(double number);
        static signed long getSigned(unsigned long x, unsigned long max_int, unsigned long twos_complement);

        WfeDashboard(int argc, char **argv, char* canInterface, const char* fname);
        ~WfeDashboard();


};

#endif
