#ifndef FAULTMONITOR_H

#define FAULTMONITOR_H

#include "bsp.h"

typedef enum faultMonitorBits_e {
    BOTS_FAILED_BIT = 0,
    EBOX_FAILED_BIT,
    BSPD_FAILED_BIT,  
    HVD_FAILED_BIT,
    AMS_FAILED_BIT,
    IMD_FAILED_BIT, 
    CBRB_FAILED_BIT,   
    TSMS_FAILED_BIT,
    HW_CHECK_FAILED_BIT,
    FSM_STATE_BIT,
}faultMonitorBits_e;

bool getBOTS_Status();
bool getEbox_Il_Status();
bool getBSPD_Status();
bool getHVD_Status();
bool getAMS_Status();
bool getIMD_Status();
bool getCBRB_Status();
bool getTSMS_Status();
bool getHwCheck_Status();

#endif /* end of include guard: FAULTMONITOR_H */
