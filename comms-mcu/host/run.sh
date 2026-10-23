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
