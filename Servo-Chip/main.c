#include "include/uart.h"
#include "include/helpers.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"

#define EXT_LED 22

/* ==== UART Config ==== */
#define UART_ID   uart0
#define BAUD      115200
#define TX_PIN    0
#define RX_PIN    1
#define EXTLED    22

/* ==== Motor Driver Config ==== */
#define STANDBY_PIN xx

/*Observe serial communication on USB*/
static inline void usb_put_byte(uint8_t b) {
    putchar_raw((int)b);  // raw, no line discipline
}

int setup(void) {
    stdio_init_all();
    poutit(EXT_LED);

    gpio_put(EXT_LED, 1);
    return 0;
}

int UART_setup(void) {
    uart_driver_init(UART_ID, BAUD, TX_PIN, RX_PIN);
    sleep_ms(500);
    uart_send_str(UART_ID, "[UART] Boot: USB CDC + UART1 link is up\r\n");

}

int motorSetup(void);

int main(void) {
    setup();
    UART_setup();

    led(0); // Setup complete

    while(true) {
        uart_send_str(UART_ID, "TEXT");
        led(1);
        sleep_ms(100);
        led(0);
        sleep_ms(900);
    }

    return 0;
}