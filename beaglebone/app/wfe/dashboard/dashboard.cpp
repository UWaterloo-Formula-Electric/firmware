#include "dashboard.h"
#include "csv.h"
#include<thread>
#include <math.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <QApplication>


WfeDashboard::WfeDashboard(int argc, char **argv, char* canInterface, const char* fname) {
    this->canInterface = canInterface;
    data = std::make_shared<QueueData>();
    // Start thread in background to collect data
    createDtcArray(fname);
    QApplication app(argc, argv);

    wu = std::make_shared<WfeDashboardUI>(app, data);

    std::thread qThread(WfeDashboard::run, data, canInterface, dtc_list);

    app.exec();

    qThread.join();
    std::cout << "data update thread finished" <<std::endl;
}

WfeDashboard::~WfeDashboard() {}

double WfeDashboard::getSpeedKphFromRPM(double newSpeedRPM, double currentSpeedKph) {
    double radius = 9;              // Inches (wheel radius)
    double inToKm = 0.001524;     // Inches to km conversion
    double sprocketRatio = 52.0/15;  // Motor sprocket to wheel sprocket ratio
    double newSpeedKph = (newSpeedRPM * inToKm * sprocketRatio * 2 * M_PI * radius);
    return 0.5 * (currentSpeedKph + newSpeedKph);
}

void WfeDashboard::createDtcArray(const char* fname)
{
    io::CSVReader<7, io::trim_chars<>, io::double_quote_escape<',', '\"'>> in(fname);
    in.read_header(io::ignore_extra_column, "DTC CODE" , "NAME", "ORIGIN", "SEVERITY", "SUBSCRIBERS", "DATA", "MESSAGE");

    int dtc_code, severity;
    std::string name, origin, subscribers, data, message;

    while (in.read_row(dtc_code, name, origin, severity, subscribers, data, message)) {
        this->dtc_list.push_back(message);
    }
}

double WfeDashboard::roundToOneDecimal(double number) {
    return static_cast<float>(static_cast<int>(number * 10.)) / 10.;
}

signed long WfeDashboard::getSigned(unsigned long x, unsigned long max_int, unsigned long twos_complement) {
    return (x > (max_int) ? (int)x - twos_complement : x);
}

int WfeDashboard::run (
    std::shared_ptr<QueueData> data,
    const char* canInterface,
    std::vector<std::string> dtc_list
    ) {
    const std::vector<int> EM_ENABLE_FAIL_CODES = {16, 17};
    const std::vector<std::string> EM_ENABLE_FAIL_REASONS =
        {"bpsState false", "low brake pressure", "throttle non-zero",
            "brake not pressed", "not hv enabled", "motors failed to start"};
    // https://github.com/craigpeacock/CAN-Examples/blob/master/canreceive.c
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return 1;
    }

    strcpy(ifr.ifr_name, canInterface);
    ioctl(s, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return 1;
    }

    double speed = 0;

    while (true) {
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes < 0) {
            perror("Read");
            std::cout << "Error in read" << std::endl;
            return 1;
        }

        // BMU_stateBusHV
        if (frame.can_id == 2282754561) {
            // Voltage: VoltageCellMin
            // Start bit: 32, length: 16, unsigned
            // Factor: 0.0001, offset: 0
            double voltage = 0.0001 * (frame.data[4] + (frame.data[5] << 8));
            data->voltage = roundToOneDecimal(voltage);
        }
        // SOC BMU_batteryStatusHV
        else if (frame.can_id == 2281967617) {
            // Battery: StateBatteryChargeHv
            // Start bit: 0, length: 10, unsigned
            // Factor: 0.1, offset: 0
            unsigned int raw_battery = frame.data[0] + ((frame.data[1] & 1) << 8) + ((frame.data[1] & 2) << 8);
            
            // Temp: TempCellMax
            // Start bit: 32, length: 10, signed
            // Factor: 0.25, offset: 90
            signed int raw_temp = WfeDashboard::getSigned((unsigned int)(frame.data[4] + ((frame.data[5] & 1) << 8) + ((frame.data[5] & 2) << 8)), 511, 1024);
            double temp = (0.25 * raw_temp) + 90.0;

            data->battery = roundToOneDecimal(raw_battery * 0.1);
            data->temperature = roundToOneDecimal(temp);
        }
        // SpeedFeedbackRight, SpeedFeedbackLeft
        else if (frame.can_id == 2240216946 || frame.can_id == 2241265521) {
            // SpeedMotorRight, SpeeedMotorLeft
            // Start bit: 0, length: 16, unsigned
            // Factor: 1, offset: -32768
            double currentSpeed = (double)(frame.data[0] + (frame.data[1] << 8)) - 32768;
            speed = getSpeedKphFromRPM(currentSpeed, speed);
            data->speed = roundToOneDecimal(speed);
        }
        // Read DTC messages
        // PDU_DTC, DCU_DTC, VCU_F7_DTC, BMU_DTC
        else if (frame.can_id == 2147548939 || frame.can_id == 2147548938 ||
            frame.can_id == 2147548937 || frame.can_id == 2147548936 ||
            frame.can_id == 2147548931 || frame.can_id == 2147548935 ||
            frame.can_id == 2147548930 || frame.can_id == 2147548929 ||
            frame.can_id == 2147548932)
        {

            int dtc_code = (int)frame.data[0];
            int dtc_severity = (int)frame.data[1];
            long dtc_data = WfeDashboard::getSigned((unsigned int)(frame.data[2]) + (frame.data[3] << 8) + (frame.data[4] << 16) + (frame.data[5] << 24), 2147483647, 4294967296);

            std::string message;
            if (dtc_code > dtc_list.size() || dtc_code <= 0) {
                std::cout << "Out of range DTC code: " << dtc_code << std::endl;
                std::cout << "Max DTC code: " << dtc_list.size() << std::endl;
                message = "Out of range DTC code: " + std::to_string(dtc_code) + "\nMax DTC code: " + std::to_string(dtc_list.size());
            }
            else {
                message = dtc_list[dtc_code-1];
            }

            if(std::find(EM_ENABLE_FAIL_CODES.begin(), EM_ENABLE_FAIL_CODES.end(), dtc_code) != EM_ENABLE_FAIL_CODES.end()) {
                std::size_t endIndex = message.find(" (Reasons");
                message = message.substr(0, endIndex);
                message = std::regex_replace(message, std::regex("#data"), EM_ENABLE_FAIL_REASONS[dtc_data]);
            } else {
                message = std::regex_replace(message, std::regex("#data"), std::to_string(dtc_data));
            }
            std::pair<int, std::string> newMsg = std::make_pair(dtc_severity, message);
            data->dtcMessagePayload.push_back(newMsg);
            std::cout << dtc_code << " - " << dtc_severity << " - " << dtc_data << " - " << dtc_list.at(dtc_code-1) << std::endl;
        }

        sleep(0.01);
    }

    if (close(s) < 0) {
        perror("Close");
        return 1;
    }

    return 0;
}



