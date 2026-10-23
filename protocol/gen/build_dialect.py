#!/usr/bin/env python3
"""Build the supported standalone dialect and official pymavlink bindings."""

from __future__ import annotations

import argparse
import hashlib
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROTOCOL = ROOT.parent
UPSTREAM = ROOT / "upstream"
STANDARD_MESSAGES = ("HEARTBEAT", "PING", "SCALED_IMU")


def verify_upstream() -> None:
    for line in (UPSTREAM / "SHA256SUMS").read_text().splitlines():
        digest, filename = line.split(maxsplit=1)
        path = UPSTREAM / Path(filename.strip()).name
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise SystemExit(f"upstream checksum mismatch: {path}")


def clean_generated_c() -> None:
    for path in (ROOT / "C").rglob("*.h"):
        lines = path.read_text(encoding="utf-8").splitlines()
        cleaned = []
        for line in lines:
            while " \t" in line:
                line = line.replace(" \t", "\t")
            cleaned.append(line.rstrip())
        path.write_text("\n".join(cleaned).rstrip() + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--python", required=True,
                        help="Python interpreter with pymavlink installed")
    args = parser.parse_args()
    verify_upstream()

    custom = ET.parse(PROTOCOL / "arsem.xml").getroot()
    upstream_roots = [ET.parse(UPSTREAM / name).getroot() for name in ("minimal.xml", "common.xml")]
    official_messages = {}
    official_enums = {}
    for root in upstream_roots:
        for message in root.findall("./messages/message"):
            if message.attrib.get("name") in STANDARD_MESSAGES:
                official_messages[message.attrib["name"]] = message
        for enum in root.findall("./enums/enum"):
            official_enums[enum.attrib["name"]] = enum
    if set(official_messages) != set(STANDARD_MESSAGES):
        missing = sorted(set(STANDARD_MESSAGES) - set(official_messages))
        raise SystemExit(f"missing standard messages: {missing}")

    refs = {field.attrib["enum"] for name in STANDARD_MESSAGES
            for field in official_messages[name].findall("field") if "enum" in field.attrib}
    standalone = ET.Element("mavlink")
    for tag in ("version", "dialect"):
        source = custom.find(tag)
        if source is not None:
            standalone.append(ET.fromstring(ET.tostring(source)))
    enums = ET.SubElement(standalone, "enums")
    for enum in custom.findall("./enums/enum"):
        enums.append(ET.fromstring(ET.tostring(enum)))
    for name in sorted(refs):
        enum = official_enums.get(name)
        if enum is None:
            raise SystemExit(f"missing enum required by standard messages: {name}")
        enums.append(ET.fromstring(ET.tostring(enum)))
    messages = ET.SubElement(standalone, "messages")
    for message in custom.findall("./messages/message"):
        messages.append(ET.fromstring(ET.tostring(message)))
    for name in STANDARD_MESSAGES:
        messages.append(ET.fromstring(ET.tostring(official_messages[name])))
    xml_path = ROOT / "arsem_standalone.xml"
    ET.indent(standalone, space="  ")
    ET.ElementTree(standalone).write(xml_path, encoding="utf-8", xml_declaration=True)

    import subprocess
    subprocess.run([args.python, "-c", "from pymavlink.generator import mavgen; print('pymavlink ready')"], check=True)
    # pymavlink 2.4.49's bundled XSD predates the official <superseded>
    # annotation used by PING. Keep the vendored source verbatim and omit
    # only this non-wire annotation in the temporary generator input.
    generated_input = ET.parse(xml_path).getroot()
    for node in generated_input.findall("./messages/message/superseded"):
        parent = next(message for message in generated_input.findall("./messages/message") if node in list(message))
        parent.remove(node)
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_xml = Path(temp_dir) / "arsem_standalone.xml"
        ET.indent(generated_input, space="  ")
        ET.ElementTree(generated_input).write(temp_xml, encoding="utf-8", xml_declaration=True)
        for output, language in ((ROOT / "C", "C"), (ROOT / "arsem.py", "Python3")):
            command = [args.python, "-c", (
                "import sys; from pymavlink.generator import mavgen; "
                "opts=mavgen.Opts(sys.argv[1], wire_protocol='2.0', language=sys.argv[2]); "
                "raise SystemExit(0 if mavgen.mavgen(opts, [sys.argv[3]]) else 1)"),
                str(output), language, str(temp_xml)]
            subprocess.run(command, check=True)
    clean_generated_c()


if __name__ == "__main__":
    main()
