#!/usr/bin/env sh
set -eu
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
out=${TMPDIR:-/tmp}/boat-motor-adapter-harness
cc -std=c11 -Wall -Wextra -Werror \
  -I"$repo_root/protocol/gen/C" \
  -I"$repo_root/protocol/gen/C/arsem_standalone" \
  -I"$repo_root/motor-mcu" \
  "$repo_root/motor-mcu/adapter.c" \
  "$repo_root/motor-mcu/host_harness.c" \
  -o "$out"
"$out"
