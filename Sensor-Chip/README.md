# Sensor-Chip

Native Raspberry Pi Pico C SDK firmware for the Adafruit Ultimate GPS
PA1616D breakout. It reads the module's NMEA UART output and forwards complete
sentences to the Pico's USB serial connection.

## Wiring

| PA1616D | Raspberry Pi Pico |
| --- | --- |
| VIN | 3V3 (or a supported supply for the breakout) |
| GND | GND |
| TX | GP5 (UART1 RX) |
| RX | GP4 (UART1 TX) |

The GPS TX connection is needed to receive NMEA data. Connect Pico TX to GPS RX
only when sending configuration commands to the module. UART defaults are
9600 baud, 8 data bits, no parity, and 1 stop bit. Pins and baud rate are
configurable with `GPS_UART_TX_PIN`, `GPS_UART_RX_PIN`, and `GPS_UART_BAUD`.

Build this directory with the Raspberry Pi Pico C/C++ SDK as a standalone
CMake project. Open the Pico's USB serial port to view NMEA sentences.

The current I2C/MAVLink branch has no sensor-board message or I2C transaction
contract yet; this initial firmware provides the GPS UART connection and USB
diagnostic stream without defining one.
