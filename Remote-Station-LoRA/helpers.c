/*
 * helpers.c : Board Helpers
 * -------------------------
 * Common GPIO and JSON helpers used by the remote station.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "helpers.h"
#include "uart.h"

/* ==== GPIO Helpers ==== */

void pinnit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void poutit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void led(int high_low) {
    gpio_put(EXT_LED, high_low ? 1 : 0);
}

void err_led(int high_low) {
    gpio_put(ERR_LED, high_low ? 1 : 0);
}

/* ==== JSON Helpers ==== */

void json_send_object(uart_inst_t *uart_id, const cJSON *object) {
    (void)uart_id;
    char *payload = cJSON_PrintUnformatted(object);
    if (!payload) {
        json_send_error(uart_id, "json allocation failed");
        return;
    }

    printf("%s\r\n", payload);
    fflush(stdout);
    cJSON_free(payload);
}

void json_send_error(uart_inst_t *uart_id, const char *message) {
    cJSON *object = cJSON_CreateObject();
    if (!object) {
        return;
    }

    cJSON_AddStringToObject(object, "event", "error");
    cJSON_AddStringToObject(object, "message", message ? message : "unknown error");
    json_send_object(uart_id, object);
    cJSON_Delete(object);
}

void json_send_status(uart_inst_t *uart_id, const char *status, const char *message) {
    cJSON *object = cJSON_CreateObject();
    if (!object) {
        return;
    }

    cJSON_AddStringToObject(object, "event", status ? status : "status");
    if (message) {
        cJSON_AddStringToObject(object, "message", message);
    }
    json_send_object(uart_id, object);
    cJSON_Delete(object);
}