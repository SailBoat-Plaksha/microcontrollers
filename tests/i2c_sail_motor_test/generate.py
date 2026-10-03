"""Regenerate the checked-in test-only C and Python bindings from one XML."""
from pathlib import Path
from pymavlink.generator import mavgen

ROOT = Path(__file__).resolve().parent
for language, output in (("C", ROOT / "src/mavlink"),
                         ("Python3", ROOT / "arsem_test.py")):
    options = mavgen.Opts(str(output), wire_protocol="2.0", language=language)
    if not mavgen.mavgen(options, [str(ROOT / "arsem_test.xml")]):
        raise SystemExit("MAVLink generation failed")
