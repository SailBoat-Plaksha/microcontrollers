/* Host event/FIFO shim around the production pico_mailbox.c implementation. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "../pico_mailbox.c"

#define FIFO_CAPACITY 16
#define RX_CAPACITY 96

i2c_inst_t fake_i2c0 = {0};
volatile uint32_t mode_pulse_us;
static i2c_slave_handler_t registered_handler;
static uint8_t tx_fifo[FIFO_CAPACITY];
static size_t tx_count;
static uint8_t rx_fifo[RX_CAPACITY];
static size_t rx_count;
static size_t rx_position;

void gpio_init(uint pin) { (void)pin; }
void gpio_set_dir(uint pin, bool out) { (void)pin; (void)out; }
void gpio_put(uint pin, bool value) { (void)pin; (void)value; }
void gpio_add_raw_irq_handler_masked(uint32_t mask, void (*handler)(void)) { (void)mask; (void)handler; }
void gpio_set_input_hysteresis_enabled(uint pin, bool enabled) { (void)pin; (void)enabled; }
void gpio_set_irq_enabled(uint pin, uint32_t events, bool enabled) { (void)pin; (void)events; (void)enabled; }
uint32_t gpio_get_irq_event_mask(uint pin) { (void)pin; return 0; }
void gpio_acknowledge_irq(uint pin, uint32_t events) { (void)pin; (void)events; }
bool gpio_get(uint pin) { (void)pin; return false; }
void gpio_set_function(uint pin, uint function) { (void)pin; (void)function; }
void gpio_pull_up(uint pin) { (void)pin; }
void pwmWrite(uint pin, uint16_t level) { (void)pin; (void)level; }
uint32_t save_and_disable_interrupts(void) { return 0; }
void restore_interrupts(uint32_t state) { (void)state; }
absolute_time_t get_absolute_time(void) { return 0; }
uint32_t to_ms_since_boot(absolute_time_t time) { return (uint32_t)time; }
uint32_t get_rand_32(void) { return 0x12345678u; }
int i2c_init(i2c_inst_t *i2c, uint baudrate) { (void)i2c; (void)baudrate; return 0; }
void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler) {
  (void)i2c;
  assert(address == MOTOR_MAILBOX_ADDR);
  registered_handler = handler;
}
size_t i2c_get_read_available(i2c_inst_t *i2c) {
  (void)i2c;
  return rx_count - rx_position;
}
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c) {
  (void)i2c;
  assert(rx_position < rx_count);
  return rx_fifo[rx_position++];
}
size_t i2c_get_write_available(i2c_inst_t *i2c) {
  (void)i2c;
  return FIFO_CAPACITY - tx_count;
}
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t byte) {
  (void)i2c;
  assert(tx_count < FIFO_CAPACITY);
  tx_fifo[tx_count++] = byte;
}

static uint8_t master_read_next(void) {
  /* The DW_apb_i2c RD_REQ model fires only while the master seeks a byte and
     the TX FIFO is empty. The callback's write is then copied into the FIFO. */
  if (!tx_count) registered_handler(i2c0, I2C_SLAVE_REQUEST);
  assert(tx_count > 0); /* also proves the path never leaves the master waiting */
  uint8_t result = tx_fifo[0];
  memmove(tx_fifo, tx_fifo + 1, --tx_count);
  return result;
}

static void finish(void) { registered_handler(i2c0, I2C_SLAVE_FINISH); }
static uint8_t expected_byte(unsigned int index) { return (uint8_t)(7u + index * 3u); }

static void set_expected_frame(void) {
  for (unsigned int i = 0; i < MOTOR_MAILBOX_BYTES; ++i) tx_frame[i] = expected_byte(i);
  tx_loaded = true;
  tx_position = 0;
}

static void assert_frame(unsigned int length) {
  for (unsigned int i = 0; i < length; ++i) assert(master_read_next() == expected_byte(i));
}

static void prepare(void) {
  static mState sail, rudder;
  sail.encoder_pin1 = 2; sail.encoder_pin2 = 3;
  sail.forward_pin = 4; sail.backward_pin = 5; sail.pwm_pin = 6;
  rudder.encoder_pin1 = 7; rudder.encoder_pin2 = 8;
  rudder.forward_pin = 10; rudder.backward_pin = 11; rudder.pwm_pin = 12;
  motor_mcu_start(&sail, &rudder);
  assert(registered_handler != NULL);
  tx_count = rx_count = rx_position = 0;
  set_expected_frame();
}

static void test_exact_contract_read_advances(void) {
  prepare();
  assert_frame(MOTOR_MAILBOX_BYTES);
  finish();
  assert(!tx_loaded);
  motor_mcu_service();
  assert(tx_loaded);
  /* A service-loaded frame replaces the prepared frame after one completion. */
  set_expected_frame();
  assert_frame(MOTOR_MAILBOX_BYTES);
  finish();
  assert(!tx_loaded);
  motor_mcu_service();
  assert(!rx_pending_valid);
}

static void test_every_short_length_retains_for_retry(void) {
  for (unsigned int length = 1; length < MOTOR_MAILBOX_BYTES; ++length) {
    prepare();
    assert_frame(length);
    finish();
    assert(tx_loaded);
    assert(tx_position == 0);
    assert_frame(MOTOR_MAILBOX_BYTES);
    finish();
    assert(!tx_loaded);
  }
}

static void test_write_only_and_repeated_start(void) {
  prepare();
  for (unsigned int i = 0; i < MOTOR_MAILBOX_BYTES; ++i) rx_fifo[i] = (uint8_t)i;
  rx_count = MOTOR_MAILBOX_BYTES;
  registered_handler(i2c0, I2C_SLAVE_RECEIVE);
  assert(rx_length == MOTOR_MAILBOX_BYTES && !rx_pending_valid);
  finish();
  assert(rx_pending_valid);
  assert(tx_loaded && tx_position == 0); /* write-only does not consume reply */
  motor_mcu_service(); /* hands the pending write to the real adapter */
  assert(!rx_pending_valid);
  assert(tx_loaded);

  /* Repeated START: finish the write phase, then read the same prepared reply. */
  rx_position = rx_count = 0;
  for (unsigned int i = 0; i < MOTOR_MAILBOX_BYTES; ++i) rx_fifo[i] = (uint8_t)(255u - i);
  rx_count = MOTOR_MAILBOX_BYTES;
  registered_handler(i2c0, I2C_SLAVE_RECEIVE);
  finish();
  assert(tx_loaded);
  assert_frame(MOTOR_MAILBOX_BYTES);
  finish();
  assert(!tx_loaded);
  motor_mcu_service();
  assert(!rx_pending_valid);
}

static void test_rx_overflow_is_bounded_and_reported(void) {
  prepare();
  uint32_t errors_before = adapter.rx_errors;
  for (unsigned int i = 0; i < MOTOR_MAILBOX_BYTES + 1u; ++i) rx_fifo[i] = (uint8_t)i;
  rx_count = MOTOR_MAILBOX_BYTES + 1u;
  registered_handler(i2c0, I2C_SLAVE_RECEIVE);
  assert(rx_length == MOTOR_MAILBOX_BYTES && rx_overflow);
  finish();
  assert(!rx_pending_valid && rx_error_pending);
  motor_mcu_service();
  assert(!rx_error_pending && adapter.rx_errors == errors_before + 1u);
  assert(tx_loaded);
}

static void test_overread_is_zero_padded(void) {
  prepare();
  assert_frame(MOTOR_MAILBOX_BYTES);
  assert(master_read_next() == 0);
  finish();
  assert(!tx_loaded);
}

static void test_read_while_reply_unavailable_does_not_stretch(void) {
  prepare();
  tx_loaded = false;
  assert(master_read_next() == 0);
  /* Even if service prepares a reply mid-transfer, this read stays padded. */
  motor_mcu_service();
  assert(tx_loaded);
  for (unsigned int i = 1; i < MOTOR_MAILBOX_BYTES; ++i) assert(master_read_next() == 0);
  finish();
  assert(tx_loaded && tx_position == 0);
  set_expected_frame();
  assert_frame(MOTOR_MAILBOX_BYTES);
  finish();
  assert(!tx_loaded);
}

static void test_read_while_unavailable_after_previous_frame(void) {
  prepare();
  tx_loaded = false;
  assert(master_read_next() == 0);
  finish();
  assert(!tx_loaded && tx_position == 0);
  motor_mcu_service();
  assert(tx_loaded);
  set_expected_frame();
  assert_frame(MOTOR_MAILBOX_BYTES);
  finish();
}

int main(void) {
  test_exact_contract_read_advances();
  test_every_short_length_retains_for_retry();
  test_write_only_and_repeated_start();
  test_rx_overflow_is_bounded_and_reported();
  test_overread_is_zero_padded();
  test_read_while_reply_unavailable_does_not_stretch();
  test_read_while_unavailable_after_previous_frame();
  puts("pico mailbox callback/service shim: ok");
  return 0;
}
