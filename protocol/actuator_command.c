#include "actuator_command.h"
#include "mavlink_min.h"

uint8_t actuator_check(actuator_guard_t *guard, const uint8_t *payload, uint8_t len,
                       bool rc_active, uint8_t system, uint8_t component) {
    if (!guard || !payload || len > 14) return 3;
    uint8_t p[14] = {0};
    for (uint8_t i = 0; i < len; ++i) p[i] = payload[i];
    uint8_t actuator = p[12], source = p[13];
    int16_t position = (int16_t)mav_u16(p + 4);
    if (actuator < 1 || actuator > 2 || position < 0 || position > 18000 ||
        (p[10] != 0 && p[10] != system) || (p[11] != 0 && p[11] != component)) return 3;
    if (rc_active || source != 2) return 2;
    uint32_t id = mav_u32(p);
    unsigned index = actuator - 1u;
    if (guard->seen[index] && id <= guard->last_id[index]) return 4;
    guard->seen[index] = true;
    guard->last_id[index] = id;
    return 0;
}
