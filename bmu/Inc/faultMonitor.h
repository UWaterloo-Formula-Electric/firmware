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

// CAN Logging
#define BOTS_FAILED      (1 << BOTS_FAILED_BIT)
#define EBOX_FAILED      (1 << EBOX_FAILED_BIT)
#define BSPD_FAILED      (1 << BSPD_FAILED_BIT)
#define HVD_FAILED       (1 << HVD_FAILED_BIT)
#define AMS_FAILED       (1 << AMS_FAILED_BIT)
#define IMD_FAILED       (1 << IMD_FAILED_BIT)
#define CBRB_FAILED      (1 << CBRB_FAILED_BIT)
#define TSMS_FAILED      (1 << TSMS_FAILED_BIT)
#define HW_CHECK_FAILED  (1 << HW_CHECK_FAILED_BIT)
#define FSM_STATE_FAILED (1 << FSM_STATE_BIT)

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
