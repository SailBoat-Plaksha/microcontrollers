# Session handoff — 2026-10-24

Use [runbook](runbook.md) for reproducible install, software checks, and the separately labeled Pi, radio, and firmware steps. The [implementation status](boat-midsem-parallel-plan.md), [protocol contract](contract.md), [branch review](branch-review.md), and [hardware checklist](hardware-checklist.md) hold the current technical details.

Current implementation uses `ACTUATOR_REQUEST` (200008). The motor Pico applies STEP once against fresh valid feedback. Mailboxes are 64 bytes; supported messages fit, and standard PING uses CRC_EXTRA 237. Do not use the older `ACTUATOR_SETPOINT` production description or claim every frame is 32 bytes.

Host tests and the motor adapter harness are available through the runbook. Scenario integration remains partly pending. Pi, ground station, and motor review tasks, GUI browser QA, Pico firmware compilation, and physical checks are not complete in this handoff. No test count or completion claim here replaces a fresh run.
