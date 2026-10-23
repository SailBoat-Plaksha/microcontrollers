#ifndef BOAT_COMMS_CONFIG_H
#define BOAT_COMMS_CONFIG_H

/* Set these to the verified board wiring before flashing. */
#define COMMS_I2C i2c0
#define COMMS_I2C_SDA_PIN 0u
#define COMMS_I2C_SCL_PIN 1u
#define COMMS_I2C_ADDRESS ARSEM_I2C_ADDR_COMMS_SENSOR
#define COMMS_LORA_UART uart1
#define COMMS_LORA_TX_PIN 4u
#define COMMS_LORA_RX_PIN 5u
#define COMMS_LORA_BAUD 115200u

/* Matching RYLR998 network configuration. Set the boat and ground addresses
 * to different values on the same network and use the same band/parameters. */
#define COMMS_RADIO_ADDRESS 1u
#define COMMS_GROUND_RADIO_ADDRESS 2u
#define COMMS_RADIO_NETWORK_ID 18u
#define COMMS_RADIO_BAND_HZ 915000000u
#define COMMS_RADIO_PARAMETER "10,7,1,12"
#define COMMS_RADIO_SEND_GAP_MS 0u

#endif
