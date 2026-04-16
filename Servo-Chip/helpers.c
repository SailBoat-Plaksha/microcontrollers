/*
 * helpers.c -- Helper functions for Servo Driver
 * -----------------------------------------------
 * Author: Esraaj Sarkar Gupta
 * 
 */

#include <stdint.h>

#include "pico/stdlib.h"
#include "include/helpers.h"
#include "include/servo.h"
#include "include/uart.h"

/* ==== Pin Helpers ==== */

void pinnit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void poutit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void motor_init(mState* motor) {
    pinnit(motor->calibration_pin);

    pinnit(motor->encoder_pin1);
    pinnit(motor->encoder_pin2);

    poutit(motor->forward_pin);
    poutit(motor->backward_pin);

    gpio_set_function(motor->pwm_pin, GPIO_FUNC_PWM);
}

/* ==== Indicative Helpers ==== */

/**
 * Indicative functions work with GPIO pins directly to produce
 * physical changes. These functions assume that the GPIO pins
 * being used have already been initialised.
 * 
 * All initialisations must be carried out in setup() in /main.c
 */

int led(int high_low) {
    if (high_low > 1 || high_low < 0) {return 1;}
    gpio_put(EXT_LED, high_low);
    return 0;
}

int err_led(int high_low) {
    if (high_low > 1 || high_low < 0) {return 1;}

    gpio_put(ERR_LED, high_low);
    return 0;
}

int debug_led(int high_low) {
    if (high_low > 1 || high_low < 0) {return 1;}

    gpio_put(DEBUG_LED, high_low);
}

/* ---- Data Structures ---- */

// Structure defined in /include/helpers.h

void Qinit(Queue* q) {
    q->front = 0;
    q->rear = 0;

    q->count = 0;
}

int isEmpty(Queue *q) {
    return q->count == 0;
}

int isFull(Queue *q) {
    return q->count == QUEUE_MAX;
}

int enqueue_error(Queue* q, int input) {
    /**
     * @brief Insert an error code into the queue.
     * 
     * This funtion has special functionality (outlined below) for enqueueing error codes
     * in particular.
     *
     * @return 0 on success.
     * @return -1 if the queue is full.
     * @return -2 if an invalid (negative integer) input is given.
     * @return -3 if an invalid (non three-digit integer) input is given.
     * 
     * 
     * @convention: The queues in this program are intended to hold error codes. To
     * distinguish function error returns (negative) to legitimate errors recognised 
     * by the program, we assert that all legitimate error codes must be positive three
     * digit integers.
     */

    if (isFull(q)) {
        return -1;
    }

    if (input < 0) {
        return -2;
    } else if (input < 100 || input > 999) {
        return -3;
    }

    q->rear = (q->rear + 1) % QUEUE_MAX;
    q->arr[q->rear] = input;

    q->count++;

    return 0;
}

int enqueue(Queue* q, int input) {
    /**
     * @brief Insert an element into the queue.
     *
     * @return 0 on success.
     * @return -1 if the queue is full. New inputs are still recorded, oldest entry is dropped.
     */

    if (isFull(q)) {
        q->front = ++(q->front)% QUEUE_MAX;

        q->rear = ++(q->rear) % QUEUE_MAX;
        q->arr[q->rear] = input;
        return -1;
    }

    q->count++;

    q->rear = ++(q->rear) % QUEUE_MAX;
    q->arr[q->rear] = input;

    return 0;
}

int dequeue(Queue* q) {
    /**
     * @brief Remove an element from the queue.
     * 
     * @return <element> on success. This value is guaranteed to be positive if enqueued with enqueue_error().
     * @return -1 if the queue is empty.
     */
    if (isEmpty(q)) {
        return -1;
    }

    int value = q->arr[q->front];
    q->front = (q->front + 1) % QUEUE_MAX;
    q->count--;
    return value;
}

int peek(Queue* q){
    if (isEmpty(q) || q->front == NULL) {
        return -1;
    }

    return q->arr[q->front];
}

/* ---- Controller Status Structure ---- */

// Functions
void Error(Controller_Status* status, int code) {
    err_led(1);
    int response = enqueue(&(status->ErrorCodes), code);

    if (response = -1) {
        // EMG UART -> Error array is full. Error still recorded.
        // UART channels are yet to be structured.       
    }

    status->lost_errors++;
}


/* ==== PWM Helpers ==== */

void pwmSetup(uint pin, uint16_t wrap_value, float clkdiv) {
    /*
     * `pin` -- PWM_A/B pin number
     * `wrap_value` -- Recommended wrap value is 255 for 8 bit resolution
     * `clkdiv` -- Recommended 4.0 -> 12 KHz PWM frequency (quiet and motor safe)
     */
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);

    pwm_set_wrap(slice, wrap_value);
    pwm_set_clkdiv(slice, clkdiv);
    pwm_set_enabled(slice, true);
} // -- GPT5

void pwmWrite(uint pin, uint16_t level) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, channel, level);
} // -- GPT5

void pwmWritePercent(uint pin, float duty) {
    /*
     * `pin` -- PWM_A/B pin number
     * `duty` -- Belongs to the interval [0, 1] (Unitary)
     */
    if (duty < 0) duty = 0;
    if (duty > 1) duty = 1;
    uint slice = pwm_gpio_to_slice_num(pin);
    uint wrap  = pwm_hw->slice[slice].top;
    pwmWrite(pin, (uint16_t)(wrap * duty));
} // -- GPT5
