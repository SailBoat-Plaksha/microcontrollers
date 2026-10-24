import importlib.util
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

from boat import ids
from protocol import codec

ROOT = Path(__file__).resolve().parents[1]


def field_values(message_id):
    definition = codec.MESSAGES[message_id]
    values = {}
    for field_type, name in definition["fields"]:
        if name == "mavlink_version":
            continue
        values[name] = 1
    return values


class ProtocolContract(unittest.TestCase):
    def test_crc_values_and_frame_sizes(self):
        expected = {**dict(zip(range(200000, 200013),
                               (139, 49, 131, 143, 158, 201, 198, 121, 188, 90, 56, 71, 32))),
                    0: 50, 4: 237, 26: 170}
        self.assertEqual(set(codec.MESSAGES), set(expected))
        for message_id, crc_extra in expected.items():
            self.assertEqual(codec.MESSAGES[message_id]["crc_extra"], crc_extra,
                             codec.MESSAGES[message_id]["name"])
            frame = codec.encode(codec.MESSAGES[message_id]["name"], field_values(message_id))
            self.assertLessEqual(len(frame), 64)
            self.assertEqual(codec.decode(frame)["id"], message_id)

    def test_cross_decode_all_messages_with_official_python(self):
        spec = importlib.util.spec_from_file_location("arsem_generated", ROOT / "protocol/gen/arsem.py")
        generated = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generated)
        for message_id, definition in codec.MESSAGES.items():
            values = field_values(message_id)
            if definition["name"] == "HEARTBEAT":
                values.update(type=2, autopilot=8, base_mode=3, custom_mode=42,
                              system_status=4, mavlink_version=3)
            mav = generated.MAVLink(None, srcSystem=7, srcComponent=9)
            mav.seq = 33
            method = getattr(mav, definition["name"].lower() + "_encode")
            generated_frame = method(**values).pack(mav)
            self.assertEqual(codec.decode(generated_frame)["fields"],
                             {**values, **({"mavlink_version": 3} if message_id == 0 else {})})
            python_frame = codec.encode(definition["name"], values, system=7, component=9, sequence=33)
            parser = generated.MAVLink(None)
            decoded = None
            for byte in python_frame:
                decoded = parser.parse_char(bytes((byte,))) or decoded
            self.assertIsNotNone(decoded, definition["name"])
            self.assertEqual(decoded.get_type(), definition["name"])

    def test_mailbox_constants_match_python_ids(self):
        header = (ROOT / "protocol/mailbox.h").read_text()
        mapping = {
            "SYSTEM_BOAT": "ARSEM_SYSTEM_BOAT",
            "COMPONENT_PI": "ARSEM_COMPONENT_PI",
            "COMPONENT_SENSOR": "ARSEM_COMPONENT_SENSOR",
            "I2C_ADDR_SENSOR": "ARSEM_I2C_ADDR_SENSOR",
            "COMPONENT_MOTOR": "ARSEM_COMPONENT_MOTOR",
            "COMPONENT_COMMS_SENSOR": "ARSEM_COMPONENT_COMMS_SENSOR",
            "SYSTEM_GROUND": "ARSEM_SYSTEM_GROUND",
            "COMPONENT_GROUND": "ARSEM_COMPONENT_GROUND",
            "I2C_ADDR_MOTOR": "ARSEM_I2C_ADDR_MOTOR",
            "I2C_ADDR_COMMS": "ARSEM_I2C_ADDR_COMMS_SENSOR",
            "MAILBOX_FRAME": "ARSEM_MAILBOX_FRAME_BYTES",
            "BUS_HZ": "ARSEM_I2C_BUS_HZ",
        }
        for python_name, c_name in mapping.items():
            value = int(re.search(rf"#define\s+{c_name}\s+(0x[0-9a-fA-F]+|\d+)", header).group(1), 0)
            self.assertEqual(getattr(ids, python_name), value, python_name)

    def test_generated_c_and_codec_exchange_request_and_ping(self):
        source = r'''#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mavlink.h"
static int nibble(char c) { if (c >= '0' && c <= '9') return c-'0'; if (c >= 'a' && c <= 'f') return c-'a'+10; if (c >= 'A' && c <= 'F') return c-'A'+10; return -1; }
int main(int argc, char **argv) {
    mavlink_message_t msg; mavlink_status_t status; unsigned char input[MAVLINK_MAX_PACKET_LEN]; size_t n=0;
    for (int i=1;i<argc;i++) { if (strlen(argv[i])%2) return 2; for (size_t j=0;j<strlen(argv[i]);j+=2) { int a=nibble(argv[i][j]),b=nibble(argv[i][j+1]); if(a<0||b<0)return 3; input[n++]=(unsigned char)((a<<4)|b); } }
    int request=0,ping=0; for(size_t i=0;i<n;i++) if(mavlink_parse_char(MAVLINK_COMM_0,input[i],&msg,&status)) { if(msg.msgid==MAVLINK_MSG_ID_ACTUATOR_REQUEST) request++; if(msg.msgid==MAVLINK_MSG_ID_PING) ping++; }
    if(request!=1||ping!=1)return 4;
    mavlink_msg_actuator_request_pack(7,9,&msg,11,12,-500,1000,2000,1,154,1,2,3);
    unsigned char output[MAVLINK_MAX_PACKET_LEN]; unsigned int length=mavlink_msg_to_send_buffer(output,&msg);
    for(unsigned int i=0;i<length;i++) { printf("%02x",output[i]); } puts("");
    mavlink_msg_ping_pack(7,9,&msg,123456,42,0,0); length=mavlink_msg_to_send_buffer(output,&msg);
    for(unsigned int i=0;i<length;i++) { printf("%02x",output[i]); } puts(""); return 0;
}'''
        request = codec.encode("ACTUATOR_REQUEST", dict(request_id=11, ground_session=12,
            delta_cdeg=-500, timeout_ms=1000, max_rate_cdeg_s=2000, target_system=1,
            target_component=154, actuator=1, opcode=2, control_source=3), system=7, component=9)
        ping = codec.encode("PING", dict(time_usec=123456, seq=42, target_system=0,
                                         target_component=0), system=7, component=9)
        with tempfile.TemporaryDirectory() as temp:
            c_file = Path(temp) / "probe.c"
            binary = Path(temp) / "probe"
            c_file.write_text(source)
            subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-I", str(ROOT / "protocol/gen/C"), "-I", str(ROOT / "protocol/gen/C/arsem_standalone"),
                str(c_file), "-o", str(binary)], check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary), request.hex(), ping.hex()],
                                    check=True, capture_output=True, text=True)
        c_request, c_ping = (bytes.fromhex(line) for line in result.stdout.splitlines())
        request_fields = codec.decode(c_request)["fields"]
        ping_fields = codec.decode(c_ping)["fields"]
        self.assertEqual(request_fields, dict(request_id=11, ground_session=12,
            delta_cdeg=-500, timeout_ms=1000, max_rate_cdeg_s=2000,
            target_system=1, target_component=154, actuator=1, opcode=2, control_source=3))
        self.assertEqual(ping_fields, dict(time_usec=123456, seq=42,
            target_system=0, target_component=0))
        self.assertEqual(codec.decode(request)["id"], 200008)
        self.assertEqual(codec.decode(ping)["id"], 4)


if __name__ == "__main__":
    unittest.main(verbosity=2)
