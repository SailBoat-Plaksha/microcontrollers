# ARSEM MAVLink dialect

`arsem.xml` is the authoritative wire contract for the boat-specific MAVLink 2 messages. It includes the standard `common.xml` dialect, so standard messages such as `HEARTBEAT`, `SYS_STATUS`, `GPS_RAW_INT`, `ATTITUDE`, `BATTERY_STATUS`, `MISSION_CURRENT`, and `STATUSTEXT` should be used instead of being redefined here.

The custom messages use private IDs `200000` through `200099`. This range is locally reserved, not assigned by the MAVLink project. Before publishing the dialect or interoperating with unrelated MAVLink networks, obtain an official allocation and migrate the IDs.

## Versioning

The XML `<version>` remains `3`, matching the MAVLink protocol version advertised by generated implementations. The application schema version is the semantic version carried by `NODE_INFO`:

- `schema_major`: increment for an incompatible field or behavior change.
- `schema_minor`: increment for a backward-compatible addition.
- `schema_patch`: increment for a clarification or compatible fix.

Version `1.0.0` is the first contract. A component sends `NODE_INFO` at startup and after reconnecting. Interfaces should decode known messages using this XML and display unknown message IDs, source IDs, sequence numbers, payload bytes, and checksum status rather than silently discarding them.

## Transport rules

- Carry raw MAVLink 2 frames over UART, USB serial, and RS-485.
- Carry Base64-encoded complete MAVLink 2 frames only inside the RYLR998 ASCII `AT+SEND` payload.
- Never Base64-encode traffic on binary-safe transports.
- Keep each MAVLink frame within a single RYLR998 transmission. With a 240-character ASCII payload limit, cap unsigned MAVLink payloads at 168 bytes and signed payloads at 155 bytes.
- Treat RS-485 as half duplex. The Raspberry Pi is the bus master and sends `BUS_GRANT`; an MCU transmits only within its grant, except for a separately specified emergency-arbitration mechanism.

## Initial node identities

The host tests use system ID `1`, Pi component ID `1`, and actuator component ID
`154`. Ground-station system IDs must differ from the onboard system ID.

## Current implementation

`codec.py` reads this XML for the eight custom messages and computes MAVLink
payload order and CRC_EXTRA. `mavlink_min.c` is a compact counterpart for the
Pico code. Both deliberately accept unsigned MAVLink 2 frames from these eight
messages and standard `HEARTBEAT`; other message IDs and signed frames are
discarded by the minimal parser. The Pi router forwards recognized frames
without rewriting them. The radio adapters do only Base64 conversion and
RYLR998 envelope handling. Neither codec implements arbitrary message
fragmentation, MAVLink signing, or a full common.xml dialect.

The servo receives `ACTUATOR_SETPOINT` on raw USB serial and sends
`ACTUATOR_STATUS` on the same link. It rejects bad CRC, wrong target, invalid
angle, RC authority conflicts, and duplicate command IDs. Hardware motion,
radio range, packet loss, and timing still require board-level testing.
