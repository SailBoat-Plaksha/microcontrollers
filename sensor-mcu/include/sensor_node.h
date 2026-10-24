#ifndef SENSOR_NODE_H
#define SENSOR_NODE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mavlink.h"

#define SENSOR_FRAME_BYTES 64u
#define SENSOR_QUEUE_CAPACITY 8u
#define SENSOR_CACHE_CAPACITY 8u
typedef uint32_t (*sensor_clock_fn)(void *context);
typedef struct {
    sensor_clock_fn clock_ms;
    void *clock_context;
    uint32_t boot_id;
    uint8_t i2c_address;
    uint8_t sda_pin, scl_pin;
} sensor_config_t;
typedef struct {
    uint8_t frames[SENSOR_QUEUE_CAPACITY][SENSOR_FRAME_BYTES];
    bool telemetry[SENSOR_QUEUE_CAPACITY];
    uint8_t head, count;
    uint32_t dependency_drops, result_drops, telemetry_coalesced;
    uint32_t malformed, rejected, timeouts;
} sensor_node_t;

void sensor_node_init(sensor_node_t *node, const sensor_config_t *config);
void sensor_node_tick(sensor_node_t *node);
bool sensor_node_receive(sensor_node_t *node, const uint8_t frame[SENSOR_FRAME_BYTES]);
/* Local Pi-facing API; session zero is reserved for this board-only route. */
bool sensor_node_request_local(sensor_node_t *node, uint32_t request_id,
                               uint16_t timeout_ms, uint16_t max_age_ms);
bool sensor_node_take_tx(sensor_node_t *node, uint8_t frame[SENSOR_FRAME_BYTES]);
uint32_t sensor_node_now(const sensor_node_t *node);
#endif
