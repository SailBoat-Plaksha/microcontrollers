# Session handoff — 2026-10-24

Use [runbook](runbook.md) for reproducible install, software checks, and the separately labeled Pi, radio, and firmware steps. The [implementation status](boat-midsem-parallel-plan.md), [protocol contract](contract.md), [branch review](branch-review.md), and [hardware checklist](hardware-checklist.md) hold the current technical details.

Current implementation uses `ACTUATOR_REQUEST` (200008). The motor Pico applies STEP once against fresh valid feedback. Mailboxes are 64 bytes; supported messages fit, and standard PING uses CRC_EXTRA 237. Do not use the older `ACTUATOR_SETPOINT` production description or claim every frame is 32 bytes.

The current bench uses laptop USB-UART → LoRa → receiving LoRa → Pi UART → Pi router → shared I2C → motor Pico. Radio gateway Picos are optional. Use `i2c+mavlink+midsem` and [direct radio setup](direct-radio.md); `i2c+mavlink` remains unchanged.

Current software checks pass 135 Python tests, all 11 existing scenarios, the five direct-radio check groups, and six C host checks. Earlier GUI browser QA and Pico/Pico 2 builds are recorded in [verification results](verification.md), with their tested revisions and limits. The new modem-health button has unit coverage but has not been browser-tested. No firmware was flashed and no physical exchange or movement was tested. The existing RC/standby pin conflict and measured encoder references still require hardware confirmation.
