#ifndef HOST_FAKE_ROSC_H
#define HOST_FAKE_ROSC_H
#include <stdint.h>
typedef struct rosc_hw { uint32_t randombit; } rosc_hw_t;
extern rosc_hw_t *rosc_hw;
#endif
