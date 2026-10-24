#!/usr/bin/env sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
out=${TMPDIR:-/tmp}/sensor-mcu-host
mkdir -p "$out"
inc="-I$root/sensor-mcu/host/fakes -I$root/sensor-mcu/include -I$root/protocol -I$root/protocol/gen/C -I$root/protocol/gen/C/arsem_standalone"
cc -std=c11 -Wall -Wextra -Werror $inc "$root/sensor-mcu/sensor_node.c" "$root/sensor-mcu/mailbox_transport.c" "$root/sensor-mcu/host/fakes/i2c.c" "$root/sensor-mcu/host/test_sensor.c" -o "$out/test_sensor"
"$out/test_sensor"
cc -std=c11 -Wall -Wextra $inc "$root/sensor-mcu/sensor_node.c" "$root/sensor-mcu/mailbox_transport.c" "$root/sensor-mcu/host/fakes/i2c.c" "$root/sensor-mcu/host/harness.c" -o "$out/sensor-harness"
echo "Harness: $out/sensor-harness"
