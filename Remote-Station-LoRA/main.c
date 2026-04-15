/*
 * main.c : Remote Station Controller
 * ----------------------------------
 * Entry point for the Remote-Station-LoRA controller.
 * Bridges USB JSON commands to the LoRa module and reports module events.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "cjson.h"
#include "helpers.h"
#include "lora.h"
#include "uart.h"

/* ==== Board State ==== */

typedef struct {
    lora_state_t lora;
} board_state_t;

static board_state_t board;

static bool usb_service_line(char *line, size_t *line_len, size_t line_cap);
static bool lora_uart_service_line(char *line, size_t *line_len, size_t line_cap);
static void handle_master_line(const char *line);
static void handle_lora_line(const char *line);
static void query_lora_config(void);
static void send_boot_banner(void);

/* ==== Internal Helpers ==== */

static bool append_char(char *buffer, size_t *index, size_t capacity, char value) {
    if (*index + 1u >= capacity) {
        *index = 0;
        return false;
    }

    buffer[*index] = value;
    (*index)++;
    buffer[*index] = '\0';
    return true;
}

static bool usb_service_line(char *line, size_t *line_len, size_t line_cap) {
    bool handled = false;
    int c;

    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        uint8_t byte = (uint8_t)c;
        handled = true;

        if (byte == '\r' || byte == '\n') {
            if (*line_len > 0) {
                line[*line_len] = '\0';
                handle_master_line(line);
                *line_len = 0;
                line[0] = '\0';
            }
            continue;
        }

        if (*line_len + 1u >= line_cap) {
            *line_len = 0;
            line[0] = '\0';
            json_send_error(MASTER_UART_ID, "line buffer overflow");
            continue;
        }

        append_char(line, line_len, line_cap, (char)byte);
    }

    if (*line_len > 0 && line[0] == '{' && line[*line_len - 1u] == '}') {
        line[*line_len] = '\0';
        handle_master_line(line);
        *line_len = 0;
        line[0] = '\0';
        handled = true;
    }

    return handled;
}

static bool lora_uart_service_line(char *line, size_t *line_len, size_t line_cap) {
    uint8_t byte;
    bool handled = false;

    while (uart_read_byte(LORA_UART_ID, &byte)) {
        handled = true;

        if (byte == '\r' || byte == '\n') {
            if (*line_len > 0) {
                line[*line_len] = '\0';
                handle_lora_line(line);
                *line_len = 0;
                line[0] = '\0';
            }
            continue;
        }

        if (*line_len + 1u >= line_cap) {
            *line_len = 0;
            line[0] = '\0';
            json_send_error(MASTER_UART_ID, "line buffer overflow");
            continue;
        }

        append_char(line, line_len, line_cap, (char)byte);
    }

    return handled;
}

static void send_status_snapshot(const char *event_name) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        json_send_error(MASTER_UART_ID, "status allocation failed");
        return;
    }

    cJSON_AddStringToObject(root, "event", event_name ? event_name : "status");
    cJSON_AddStringToObject(root, "master_link", "usb_cdc");
    json_send_object(MASTER_UART_ID, root);
    cJSON_Delete(root);
}

static void query_lora_config(void) {
    if (!lora_send_command(&board.lora, "AT+ADDRESS?")) {
        json_send_error(MASTER_UART_ID, "address query failed");
    }

    if (!lora_send_command(&board.lora, "AT+NETWORKID?")) {
        json_send_error(MASTER_UART_ID, "network query failed");
    }

    if (!lora_send_command(&board.lora, "AT+BAND?")) {
        json_send_error(MASTER_UART_ID, "band query failed");
    }

    if (!lora_send_command(&board.lora, "AT+PARAMETER?")) {
        json_send_error(MASTER_UART_ID, "parameter query failed");
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        json_send_error(MASTER_UART_ID, "config query allocation failed");
        return;
    }

    cJSON_AddStringToObject(root, "event", "config_query");
    cJSON_AddStringToObject(root, "message", "lora query sent");
    json_send_object(MASTER_UART_ID, root);
    cJSON_Delete(root);
}

static void handle_master_line(const char *line) {
    cJSON *root = cJSON_Parse(line);
    if (!root) {
        json_send_error(MASTER_UART_ID, "invalid json from master");
        return;
    }

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cJSON_IsString(cmd_item) || !cmd_item->valuestring) {
        cJSON_Delete(root);
        json_send_error(MASTER_UART_ID, "missing cmd field");
        return;
    }

    const char *cmd = cmd_item->valuestring;

    if (strcmp(cmd, "send") == 0) {
        cJSON *message_item = cJSON_GetObjectItem(root, "message");
        cJSON *addr_item = cJSON_GetObjectItem(root, "addr");

        if (!cJSON_IsString(message_item) || !message_item->valuestring) {
            json_send_error(MASTER_UART_ID, "missing message field");
            cJSON_Delete(root);
            return;
        }

        int destination = 0;
        if (cJSON_IsNumber(addr_item)) {
            destination = addr_item->valueint;
        }

        if (!lora_send_message(&board.lora, (uint16_t)destination, message_item->valuestring)) {
            json_send_error(MASTER_UART_ID, "lora send failed");
        } else {
            json_send_status(MASTER_UART_ID, "ok", "message queued to lora");
        }
    } else if (strcmp(cmd, "raw") == 0) {
        cJSON *command_item = cJSON_GetObjectItem(root, "command");

        if (!cJSON_IsString(command_item) || !command_item->valuestring) {
            json_send_error(MASTER_UART_ID, "missing command field");
            cJSON_Delete(root);
            return;
        }

        if (!lora_send_command(&board.lora, command_item->valuestring)) {
            json_send_error(MASTER_UART_ID, "lora command failed");
        } else {
            json_send_status(MASTER_UART_ID, "ok", "raw command sent");
        }
    } else if (strcmp(cmd, "config") == 0) {
        bool config_ok = true;

        cJSON *address_item = cJSON_GetObjectItem(root, "address");
        cJSON *network_item = cJSON_GetObjectItem(root, "network_id");
        cJSON *band_item = cJSON_GetObjectItem(root, "band");
        cJSON *parameter_item = cJSON_GetObjectItem(root, "parameter");
        bool has_config_fields = cJSON_IsNumber(address_item) || cJSON_IsNumber(network_item) || cJSON_IsNumber(band_item) || (cJSON_IsString(parameter_item) && parameter_item->valuestring);

        if (!has_config_fields) {
            query_lora_config();
            cJSON_Delete(root);
            return;
        }

        if (cJSON_IsNumber(address_item)) {
            config_ok = lora_set_address(&board.lora, (uint16_t)address_item->valueint) && config_ok;
        }

        if (cJSON_IsNumber(network_item)) {
            config_ok = lora_set_network_id(&board.lora, (uint8_t)network_item->valueint) && config_ok;
        }

        if (cJSON_IsNumber(band_item)) {
            config_ok = lora_set_band(&board.lora, (uint32_t)band_item->valuedouble) && config_ok;
        }

        if (cJSON_IsString(parameter_item) && parameter_item->valuestring) {
            config_ok = lora_set_parameter(&board.lora, parameter_item->valuestring) && config_ok;
        }

        if (config_ok) {
            json_send_status(MASTER_UART_ID, "ok", "configuration applied");
        } else {
            json_send_error(MASTER_UART_ID, "configuration failed");
        }
    } else if (strcmp(cmd, "status") == 0) {
        send_status_snapshot("status");
    } else if (strcmp(cmd, "ping") == 0) {
        if (!lora_send_command(&board.lora, "AT")) {
            json_send_error(MASTER_UART_ID, "ping failed");
        } else {
            json_send_status(MASTER_UART_ID, "ok", "AT sent");
        }
    } else if (strcmp(cmd, "reset") == 0) {
        if (!lora_send_command(&board.lora, "AT+RESET")) {
            json_send_error(MASTER_UART_ID, "reset failed");
        } else {
            json_send_status(MASTER_UART_ID, "ok", "reset sent");
        }
    } else {
        json_send_error(MASTER_UART_ID, "unknown command");
    }

    cJSON_Delete(root);
}

static void handle_lora_line(const char *line) {
    cJSON *raw_root = cJSON_CreateObject();
    if (raw_root) {
        cJSON_AddStringToObject(raw_root, "event", "lora_line");
        cJSON_AddStringToObject(raw_root, "line", line);
        json_send_object(MASTER_UART_ID, raw_root);
        cJSON_Delete(raw_root);
    }

    lora_rx_frame_t frame;
    if (lora_parse_rcv_line(line, &frame)) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            json_send_error(MASTER_UART_ID, "receive allocation failed");
            return;
        }

        cJSON_AddStringToObject(root, "event", "received");
        cJSON_AddNumberToObject(root, "address", frame.address);
        cJSON_AddNumberToObject(root, "length", frame.length);
        cJSON_AddNumberToObject(root, "rssi", frame.rssi);
        cJSON_AddNumberToObject(root, "snr", frame.snr);
        cJSON_AddStringToObject(root, "payload", frame.payload);
        json_send_object(MASTER_UART_ID, root);
        cJSON_Delete(root);
        return;
    }

    if (strncmp(line, "+ADDRESS=", 9) == 0) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            json_send_error(MASTER_UART_ID, "address response allocation failed");
            return;
        }

        cJSON_AddStringToObject(root, "event", "config");
        cJSON_AddStringToObject(root, "field", "address");
        cJSON_AddNumberToObject(root, "value", atoi(line + 9));
        json_send_object(MASTER_UART_ID, root);
        cJSON_Delete(root);
        return;
    }

    if (strncmp(line, "+NETWORKID=", 11) == 0) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            json_send_error(MASTER_UART_ID, "network response allocation failed");
            return;
        }

        cJSON_AddStringToObject(root, "event", "config");
        cJSON_AddStringToObject(root, "field", "network_id");
        cJSON_AddNumberToObject(root, "value", atoi(line + 11));
        json_send_object(MASTER_UART_ID, root);
        cJSON_Delete(root);
        return;
    }

    if (strncmp(line, "+BAND=", 6) == 0) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            json_send_error(MASTER_UART_ID, "band response allocation failed");
            return;
        }

        cJSON_AddStringToObject(root, "event", "config");
        cJSON_AddStringToObject(root, "field", "band");
        cJSON_AddNumberToObject(root, "value", strtoul(line + 6, NULL, 10));
        json_send_object(MASTER_UART_ID, root);
        cJSON_Delete(root);
        return;
    }

    if (strncmp(line, "+PARAMETER=", 11) == 0) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            json_send_error(MASTER_UART_ID, "parameter response allocation failed");
            return;
        }

        cJSON_AddStringToObject(root, "event", "config");
        cJSON_AddStringToObject(root, "field", "parameter");
        cJSON_AddStringToObject(root, "value", line + 11);
        json_send_object(MASTER_UART_ID, root);
        cJSON_Delete(root);
        return;
    }

    if (strncmp(line, "+OK", 3) == 0) {
        json_send_status(MASTER_UART_ID, "ok", line);
        return;
    }

    if (strncmp(line, "+ERR", 4) == 0) {
        json_send_error(MASTER_UART_ID, line);
        return;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    cJSON_AddStringToObject(root, "event", "lora");
    cJSON_AddStringToObject(root, "line", line);
    json_send_object(MASTER_UART_ID, root);
    cJSON_Delete(root);
}

static void send_boot_banner(void) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    cJSON_AddStringToObject(root, "event", "boot");
    cJSON_AddStringToObject(root, "module", "REYAX RYLR998");
    cJSON_AddStringToObject(root, "role", "remote-station");
    cJSON_AddStringToObject(root, "master_uart", "usb_cdc");
    cJSON_AddStringToObject(root, "lora_uart", "uart0");
    json_send_object(MASTER_UART_ID, root);
    cJSON_Delete(root);
}

/* ==== Entry Point ==== */

int main(void) {
    stdio_init_all();

    poutit(EXT_LED);
    poutit(ERR_LED);
    led(0);
    err_led(0);

    lora_init(&board.lora, LORA_UART_ID, LORA_UART_TX_PIN, LORA_UART_RX_PIN, LORA_UART_BAUD);

    sleep_ms(200);
    send_boot_banner();

    char master_line[256] = {0};
    char lora_line[256] = {0};
    size_t master_len = 0;
    size_t lora_len = 0;

    while (true) {
        bool did_work = false;
        did_work |= usb_service_line(master_line, &master_len, sizeof(master_line));
        did_work |= lora_uart_service_line(lora_line, &lora_len, sizeof(lora_line));

        if (!did_work) {
            sleep_ms(1);
        }
    }

    return 0;
}