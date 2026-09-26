#ifndef ARSEM_ACTUATOR_COMMAND_H
#define ARSEM_ACTUATOR_COMMAND_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t last_id[2];
    bool seen[2];
} actuator_guard_t;

/* COMMAND_RESULT: 0 accepted, 2 authority denied, 3 invalid, 4 stale/duplicate. */
uint8_t actuator_check(actuator_guard_t *guard, const uint8_t *payload, uint8_t payload_length,
                       bool rc_active, uint8_t system_id, uint8_t component_id);
#endif
