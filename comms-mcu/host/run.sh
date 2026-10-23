#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
OUT=${TMPDIR:-/tmp}/boat-comms-relay-host-test
cc -std=c11 -Wall -Wextra -Werror \
  -I"$ROOT/comms-mcu/include" \
  -I"$ROOT/protocol" \
  -I"$ROOT/protocol/gen/C" \
  -I"$ROOT/protocol/gen/C/arsem_standalone" \
  "$ROOT/comms-mcu/relay.c" "$ROOT/comms-mcu/modem.c" \
  "$ROOT/comms-mcu/host/test_relay.c" \
  -o "$OUT"
"$OUT"
MAILBOX_OUT=${TMPDIR:-/tmp}/boat-comms-mailbox-host-test
cc -std=c11 -Wall -Wextra -Werror \
  -I"$ROOT/comms-mcu/host/fakes" \
  -I"$ROOT/comms-mcu/include" \
  -I"$ROOT/protocol" \
  -I"$ROOT/protocol/gen/C" \
  -I"$ROOT/protocol/gen/C/arsem_standalone" \
  "$ROOT/comms-mcu/mailbox_transport.c" "$ROOT/comms-mcu/relay.c" \
  "$ROOT/comms-mcu/modem.c" "$ROOT/comms-mcu/host/test_mailbox_transport.c" \
  -o "$MAILBOX_OUT"
"$MAILBOX_OUT"
RUNTIME_OUT=${TMPDIR:-/tmp}/boat-comms-main-runtime-host-test
cc -std=c11 -Wall -Wextra -Werror \
  -I"$ROOT/comms-mcu/host/fakes" \
  -I"$ROOT/comms-mcu/include" \
  -I"$ROOT/comms-mcu" \
  -I"$ROOT/protocol" \
  -I"$ROOT/protocol/gen/C" \
  -I"$ROOT/protocol/gen/C/arsem_standalone" \
  "$ROOT/comms-mcu/host/test_main_runtime.c" \
  "$ROOT/comms-mcu/mailbox_transport.c" "$ROOT/comms-mcu/relay.c" \
  "$ROOT/comms-mcu/modem.c" \
  -o "$RUNTIME_OUT"
"$RUNTIME_OUT"
