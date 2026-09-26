"""Render repeatable, slide-ready host-test evidence images."""

import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = Path(__file__).resolve().parent
FONT = next(path for path in (
    "/usr/share/fonts/google-noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
) if Path(path).exists())


def font(size):
    return ImageFont.truetype(FONT, size)


def canvas(title, subtitle):
    image = Image.new("RGB", (1600, 900), "#0d1728")
    draw = ImageDraw.Draw(image)
    draw.text((76, 60), title, font=font(45), fill="#f4f7fb")
    draw.text((78, 125), subtitle, font=font(23), fill="#91a5bd")
    draw.line((78, 173, 1522, 173), fill="#2d4261", width=2)
    return image, draw


def card(draw, box, label, detail, accent="#53d6a3"):
    draw.rounded_rectangle(box, radius=22, fill="#172740", outline="#2d4261", width=2)
    x, y, _, _ = box
    draw.text((x + 30, y + 30), label, font=font(28), fill=accent)
    for index, line in enumerate(detail):
        draw.text((x + 30, y + 86 + 37 * index), line, font=font(22), fill="#e5edf7")


def render_tests(output):
    image, draw = canvas("MAVLink packet link tests", "Executed on host computer  |  no radio hardware")
    passed = output.count(" ... ok")
    card(draw, (78, 210, 710, 470), "UNIT TESTS", [f"{passed} passed  /  0 failed", "Python + host-compiled C"])
    card(draw, (740, 210, 1522, 470), "SERIAL LINK", ["100 / 100 fragmented frames", "reassembled in order"])
    card(draw, (78, 500, 710, 760), "LORA ENVELOPE", ["100 / 100 intact frames delivered", "100 / 100 damaged frames rejected"])
    card(draw, (740, 500, 1522, 760), "INTEROPERABILITY", ["C and Python: identical frame", "CRC + duplicate checks passed"])
    draw.text((80, 830), "Source: python -m unittest discover -s tests -v", font=font(20), fill="#91a5bd")
    image.save(OUT / "01-host-test-results.png")


def render_path():
    image, draw = canvas("One MAVLink frame across every link", "Binary serial on each side  |  ASCII only inside the LoRa modem")
    card(draw, (78, 265, 495, 555), "PI / SERVO", ["MAVLink 2", "raw 26-byte frame", "CRC checked"])
    card(draw, (590, 265, 1008, 555), "RYLR998", ["Base64 envelope", "36 ASCII characters", "one AT+SEND packet"], "#ffc875")
    card(draw, (1100, 265, 1522, 555), "RECEIVER", ["same 26 bytes", "CRC checked again", "command validated"])
    draw.line((500, 405, 575, 405), fill="#53d6a3", width=7)
    draw.polygon(((575, 405), (554, 392), (554, 418)), fill="#53d6a3")
    draw.line((1010, 405, 1085, 405), fill="#53d6a3", width=7)
    draw.polygon(((1085, 405), (1064, 392), (1064, 418)), fill="#53d6a3")
    draw.text((105, 672), "Verified: serial framing, radio encoding, frame identity, corruption rejection", font=font(22), fill="#e5edf7")
    draw.text((105, 718), "Next field check: actual LoRa range, loss, and motor behavior", font=font(22), fill="#91a5bd")
    image.save(OUT / "02-link-path.png")


def render_trace():
    sys.path.insert(0, str(ROOT))
    from protocol.codec import encode, radio_command
    fields = dict(command_id=42, position_cdeg=9000, timeout_ms=1000,
                  max_rate_cdeg_s=0, target_system=1, target_component=154,
                  actuator=1, control_source=2)
    frame = encode("ACTUATOR_SETPOINT", fields)
    radio = radio_command(frame).decode().strip()
    image, draw = canvas("Packet trace: sail setpoint", "A concrete frame shared by Python and the C host test")
    rows = [
        ("MESSAGE", "ACTUATOR_SETPOINT  |  ID 200001  |  command 42"),
        ("TARGET", "system 1  /  actuator component 154  /  sail"),
        ("VALUE", "90.00 deg  /  timeout 1000 ms  /  autonomous"),
        ("BINARY MAVLINK", frame.hex()),
        ("LORA ASCII", radio),
    ]
    y = 225
    for label, value in rows:
        draw.text((84, y), label, font=font(23), fill="#53d6a3")
        draw.text((84, y + 43), value, font=font(21), fill="#e5edf7")
        draw.line((84, y + 105, 1515, y + 105), fill="#2d4261", width=1)
        y += 120
    draw.text((84, 845), "Host-generated example; no claim of RF transmission", font=font(19), fill="#91a5bd")
    image.save(OUT / "03-packet-trace.png")


def render_console(output):
    image, draw = canvas("Unit test run", "Captured output from the current branch")
    draw.rounded_rectangle((78, 203, 1522, 824), radius=18, fill="#07101d", outline="#2d4261", width=2)
    for index, line in enumerate(output.splitlines()):
        if index >= 23:
            break
        color = "#53d6a3" if line.endswith(" ok") or line == "OK" else "#dbe7f4"
        draw.text((106, 225 + index * 25), line, font=font(18), fill=color)
    image.save(OUT / "04-test-console.png")


def main():
    run = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"],
                         cwd=ROOT, capture_output=True, text=True)
    output = run.stderr + run.stdout
    (OUT / "host-test-results.txt").write_text(output)
    if run.returncode:
        raise SystemExit(output)
    render_tests(output)
    render_path()
    render_trace()
    render_console(output)


if __name__ == "__main__":
    main()
