#ifndef ARSEM_MAILBOX_H
#define ARSEM_MAILBOX_H

/*
 * Frozen shared I2C mailbox and identity constants for the integrated demo.
 * Source of truth: docs/contract.md. Python code mirrors these values in
 * boat/ids.py; scenario tests assert the two stay identical.
 *
 * The Raspberry Pi is the sole controller of one shared 100 kHz I2C bus.
 * Every mailbox transfer is exactly ARSEM_MAILBOX_FRAME_BYTES long and
 * carries at most one whole unsigned MAVLink 2 frame, zero-padded.
 */

#define ARSEM_MAILBOX_FRAME_BYTES 64u
#define ARSEM_MAILBOX_MAX_PAYLOAD 52u /* frame bytes minus MAVLink 2 header+crc */
#define ARSEM_I2C_BUS_HZ 100000u

/* Provisional addresses; verify against real devices before wiring. */
#define ARSEM_I2C_ADDR_MOTOR 0x08
#define ARSEM_I2C_ADDR_SENSOR 0x0A
#define ARSEM_I2C_ADDR_COMMS_SENSOR 0x09

/* System and component identities. */
#define ARSEM_SYSTEM_BOAT 1
#define ARSEM_COMPONENT_PI 1
#define ARSEM_COMPONENT_SENSOR 159
#define ARSEM_COMPONENT_MOTOR 154
#define ARSEM_COMPONENT_COMMS_SENSOR 158
#define ARSEM_SYSTEM_GROUND 255
#define ARSEM_COMPONENT_GROUND 190

#endif /* ARSEM_MAILBOX_H */
