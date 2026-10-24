#include "sensor_node.h"
#include "mailbox_transport.h"
#include "pico/i2c_slave.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t fake_ms;
static uint32_t clock_fn(void *context) { (void)context; return fake_ms; }

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void print_frame(const uint8_t frame[SENSOR_FRAME_BYTES]) {
    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++) printf("%02x", frame[i]);
    putchar('\n');
}

static bool parse_time(const char *text, uint32_t *result) {
    if (!*text) return false;
    for (const char *p = text; *p; p++) if (!isdigit((unsigned char)*p)) return false;
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (errno || !end || *end || value > UINT32_MAX) return false;
    *result = (uint32_t)value;
    return true;
}

static bool parse_frame(const char *text, uint8_t frame[SENSOR_FRAME_BYTES]) {
    if (strlen(text) != SENSOR_FRAME_BYTES * 2) return false;
    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++) {
        int high = hexval((unsigned char)text[2 * i]);
        int low = hexval((unsigned char)text[2 * i + 1]);
        if (high < 0 || low < 0) return false;
        frame[i] = (uint8_t)((high << 4) | low);
    }
    return true;
}

static void receive_transaction(sensor_mailbox_t *mailbox, const uint8_t frame[64]) {
    i2c_inst_t bus = {.input = frame, .input_len = SENSOR_FRAME_BYTES};
    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++)
        sensor_mailbox_callback(&bus, I2C_SLAVE_RECEIVE);
    sensor_mailbox_callback(&bus, I2C_SLAVE_FINISH);
    (void)mailbox;
}

static bool read_transaction(sensor_mailbox_t *mailbox, uint8_t frame[64]) {
    i2c_inst_t bus = {0};
    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++)
        sensor_mailbox_callback(&bus, I2C_SLAVE_REQUEST);
    sensor_mailbox_callback(&bus, I2C_SLAVE_FINISH);
    if (bus.output_len != SENSOR_FRAME_BYTES) return false;
    memcpy(frame, bus.output, SENSOR_FRAME_BYTES);
    (void)mailbox;
    return true;
}

int main(void) {
    sensor_node_t node;
    sensor_mailbox_t mailbox;
    sensor_config_t config = {clock_fn, NULL, 1, 10, 0, 1};
    sensor_node_init(&node, &config);
    sensor_mailbox_init(&mailbox);

    char line[512];
    puts("READY sensor-mcu raw64 harness; commands: I <exactly 128 hex>, T <absolute_ms>, O");
    while (fgets(line, sizeof line, stdin)) {
        size_t length = strlen(line);
        if (length && line[length - 1] != '\n' && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            puts("ERR line too long");
            continue;
        }
        while (length && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';

        if (line[0] == 'T' && line[1] == ' ') {
            uint32_t value;
            if (!parse_time(line + 2, &value)) { puts("ERR time"); continue; }
            fake_ms = value;
            sensor_node_tick(&node);
            sensor_mailbox_service(&mailbox, &node);
            puts("OK");
        } else if (line[0] == 'I' && line[1] == ' ') {
            uint8_t frame[SENSOR_FRAME_BYTES];
            if (!parse_frame(line + 2, frame)) { puts("ERR hex"); continue; }
            receive_transaction(&mailbox, frame);
            sensor_mailbox_service(&mailbox, &node);
            puts("OK");
        } else if (strcmp(line, "O") == 0) {
            uint8_t frame[SENSOR_FRAME_BYTES];
            sensor_node_tick(&node);
            sensor_mailbox_service(&mailbox, &node);
            if (!mailbox.tx_available) { puts("EMPTY"); continue; }
            if (!read_transaction(&mailbox, frame)) { puts("ERR callback read"); continue; }
            print_frame(frame);
            /* Mirror the main-loop refill after STOP; this does not alter the
             * bytes just returned by the completed read transaction. */
            sensor_mailbox_service(&mailbox, &node);
        } else {
            puts("ERR command");
        }
    }
    return 0;
}
