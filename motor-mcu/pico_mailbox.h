#ifndef MOTOR_MCU_PICO_MAILBOX_H
#define MOTOR_MCU_PICO_MAILBOX_H

#include "servo.h"

void motor_mcu_start(mState *sail, mState *rudder);
void motor_mcu_service(void);

#endif
