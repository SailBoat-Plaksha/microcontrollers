#ifndef HOST_FAKE_PICO_STDLIB_H
#define HOST_FAKE_PICO_STDLIB_H
#include <stdbool.h>
#include <stdint.h>
typedef uint64_t absolute_time_t;
void stdio_init_all(void);
absolute_time_t get_absolute_time(void);
uint32_t to_ms_since_boot(absolute_time_t value);
uint32_t time_us_32(void);
void tight_loop_contents(void);
#endif
