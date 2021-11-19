#ifndef MAINTASKENTRY_H

#define MAINTASKENTRY_H

#include "bsp.h"

typedef enum DBW_Task_Notifications_t {
    EM_TOGGLE_BUTTON_EVENT = 0,
    HV_TOGGLE_BUTTON_EVENT = 1,
    HV_ENABLED_NOTIFICATION,
    HV_DISABLED_NOTIFICATION,
    EM_ENABLED_NOTIFICATION,
    EM_DISABLED_NOTIFICATION,
} DBW_Task_Notifications_t;

extern bool waitingForHVChange;
extern bool waitingForEMChange;

#endif /* end of include guard: MAINTASKENTRY_H */
