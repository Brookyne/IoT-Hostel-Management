#ifndef FAN_TASK_H
#define FAN_TASK_H

#include <Arduino.h>
#include "main_constants.h"

// Function declarations
void setupFanPWM();
void updateFanSpeed(bool state);
void fanTask(void *parameter);

// External variables
extern bool fanState;
extern bool lastKnownFanState;

#endif /* FAN_TASK_H */ 