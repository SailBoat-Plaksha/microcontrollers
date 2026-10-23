#ifndef MOTOR_TEST_HARDWARE_PWM_H
#define MOTOR_TEST_HARDWARE_PWM_H
#include <stdint.h>
typedef unsigned int uint;
void pwmWrite(uint pin, uint16_t level);
#endif
