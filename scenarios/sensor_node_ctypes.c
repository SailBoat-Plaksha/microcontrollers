#include <stdlib.h>
#include <string.h>

#include "sensor_node.h"
#include "mailbox_transport.h"
#include "pico/i2c_slave.h"

typedef struct {
    sensor_node_t node;
    sensor_mailbox_t mailbox;
    uint32_t now_ms;
} sensor_fixture_t;

static uint32_t fixture_clock(void *context) {
    return ((sensor_fixture_t *)context)->now_ms;
}

void *scenario_sensor_create(uint32_t boot_id) {
    sensor_fixture_t *fixture = calloc(1, sizeof *fixture);
    if (!fixture) return NULL;
    sensor_config_t config = {fixture_clock, fixture, boot_id, 10, 0, 1};
    sensor_node_init(&fixture->node, &config);
    sensor_mailbox_init(&fixture->mailbox);
    return fixture;
}

void scenario_sensor_destroy(void *opaque) { free(opaque); }

void scenario_sensor_set_time(void *opaque, uint32_t now_ms) {
    sensor_fixture_t *fixture = opaque;
    fixture->now_ms = now_ms;
    sensor_mailbox_service(&fixture->mailbox, &fixture->node);
    sensor_node_tick(&fixture->node);
    sensor_mailbox_service(&fixture->mailbox, &fixture->node);
}

int scenario_sensor_write(void *opaque, const uint8_t frame[64]) {
    sensor_fixture_t *fixture = opaque;
    i2c_inst_t bus = {.input = frame, .input_len = SENSOR_FRAME_BYTES};
    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++)
        sensor_mailbox_callback(&bus, I2C_SLAVE_RECEIVE);
    sensor_mailbox_callback(&bus, I2C_SLAVE_FINISH);
    sensor_mailbox_service(&fixture->mailbox, &fixture->node);
    return fixture->mailbox.rx_ready ? 0 : 1;
}

int scenario_sensor_read(void *opaque, uint8_t frame[64]) {
    sensor_fixture_t *fixture = opaque;
    i2c_inst_t bus = {0};
    sensor_mailbox_service(&fixture->mailbox, &fixture->node);
    sensor_mailbox_callback(&bus, I2C_SLAVE_REQUEST);
    for (size_t i = 1; i < SENSOR_FRAME_BYTES; i++)
        sensor_mailbox_callback(&bus, I2C_SLAVE_REQUEST);
    sensor_mailbox_callback(&bus, I2C_SLAVE_FINISH);
    if (bus.output_len != SENSOR_FRAME_BYTES) return 0;
    memcpy(frame, bus.output, SENSOR_FRAME_BYTES);
    sensor_mailbox_service(&fixture->mailbox, &fixture->node);
    return 1;
}

int scenario_sensor_request_local(void *opaque, uint32_t request_id,
                                  uint16_t timeout_ms, uint16_t max_age_ms) {
    sensor_fixture_t *fixture = opaque;
    return sensor_node_request_local(&fixture->node, request_id,
                                     timeout_ms, max_age_ms);
}
