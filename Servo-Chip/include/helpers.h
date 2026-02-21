/*
 * helpers.h -- Header for helpers.c
 * -----------------------------------------------
 * Author: Esraaj Sarkar Gupta
 * 
 */

#ifndef EXT_LED
#define EXT_LED 22
#endif

#define QUEUE_MAX 100
#define MAX_ERRORS 100

#ifndef ERR_LED
#define ERR_LED 17
#endif

#include <stdlib.h>
#include <stdint.h>

#include "servo.h"

/* ---- Pin Helpers ---- */

void pinnit(int pin);
void poutit(int pin);
void motor_init(mState *motor);

/* ---- Indicative Helpers */

int led(int high_low);

/* ==== Data Structures ==== */

/* ---- Queues and Queue Functions ---- */
typedef struct {
    int arr[QUEUE_MAX];

    int front;
    int rear;

    int count;
} Queue;

void Qinit(Queue* q);

int isEmpty(Queue* q);
int isFull(Queue* q);

int enqeue_error(Queue* q, int input);
int enqeue(Queue* q, int input);

int dequeue(Queue* q);

int peek(Queue* q);

/* ---- Controller Status Structure ---- */

typedef struct {
    uint8_t status;

    Queue ErrorCodes;
    uint64_t lost_errors; // Number of errors dropped due to ErrorCode Queue overflows.

} Controller_Status;

/* ---- PWM Helpers ---- */

#include "hardware/pwm.h"

void pwmSetup(uint pin, uint16_t wrap_value, float clkdiv);
void pwmWrite(uint pin, uint16_t level);
void pwmWritePercent(uint pin, float duty);
