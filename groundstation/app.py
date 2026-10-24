"""NiceGUI ground station. Run with ``python -m groundstation.app``."""
from __future__ import annotations

import concurrent.futures
import json
import queue
import time

from boat.client import BoatClient
from boat.radio import LoRaModemTransport
from .state import STATE, TransportLog
from .transports import SerialTransport, SimulationTransport, SSHTransport

_WORKERS = concurrent.futures.ThreadPoolExecutor(max_workers=4, thread_name_prefix="boat-io")
_ACTION_LOCK = __import__("threading").Lock()
TRANSPORT_FACTORIES = {
    "serial": lambda port, baud: SerialTransport(port, int(baud)),
    "direct_radio": lambda port, baud, **settings: LoRaModemTransport(
        port, int(baud), **settings),
    "ssh": lambda alias, _baud: SSHTransport(alias),
    "simulation": lambda _port, _baud: SimulationTransport(),
}


def set_transport_factory(kind, factory):
    """Replace a transport constructor with any BoatClient-compatible Transport.

    The simulation constructor is the integration seam for an onboard-router
    simulator; replacement objects still carry raw frames through BoatClient.
    """
    if kind not in TRANSPORT_FACTORIES:
        raise ValueError("transport kind must be serial, direct_radio, ssh, or simulation")
    if not callable(factory):
        raise TypeError("factory must be callable")
    TRANSPORT_FACTORIES[kind] = factory


def _call_background(fn, *args):
    def run():
        try:
            fn(*args)
        except Exception as exc:
            STATE.enqueue(exc)
            with STATE.lock:
                STATE.detail = f"{type(exc).__name__}: {exc}"
                if isinstance(exc, ValueError):
                    STATE.busy = False
                    STATE.uncertain = False
                if STATE.connected:
                    if not isinstance(exc, ValueError):
                        STATE.uncertain = True
    return _WORKERS.submit(run)


def _connect(mode, port, baud, ssh_host, radio_settings=None):
    with _ACTION_LOCK:
        with STATE.lock:
            old_client = STATE.client
            active = STATE.connected and old_client is not None and bool(
                getattr(old_client.transport, "is_connected", True))
        if active:
            raise RuntimeError("Disconnect the active transport before choosing another.")
        if old_client is not None:
            with STATE.lock:
                old_unsubscribe = STATE.unsubscribe
                STATE.unsubscribe = None
            if old_unsubscribe is not None:
                old_unsubscribe()
            STATE.drain()
            try:
                old_client.disconnect()
            except Exception:
                try:
                    old_client.transport.close()
                except Exception:
                    pass
            with STATE.lock:
                STATE.connected = False
                STATE.client = None
        key, first = (("direct_radio", port) if mode.startswith("LoRa · direct USB UART") else
                      ("serial", port) if mode.startswith("LoRa") else
                      ("ssh", ssh_host) if mode.startswith("SSH") else
                      ("simulation", port))
        if key == "direct_radio":
            transport = TRANSPORT_FACTORIES[key](first, baud, **(radio_settings or {}))
        else:
            transport = TRANSPORT_FACTORIES[key](first, baud)
        client = BoatClient(transport)
        unsubscribe = client.subscribe(lambda event, owner=client: STATE.enqueue(event, owner))
        with STATE.lock:
            STATE.transport, STATE.client, STATE.unsubscribe = transport, client, unsubscribe
            STATE.mode, STATE.detail = transport.name, "Opening transport…"
            STATE.uncertain, STATE.busy, STATE.feedback_valid = True, False, False
            STATE.motor_report = STATE.motor_health = None
            STATE.nodes.clear()
            STATE.pings.clear()
        client.connect()
        # Initial poll is read-only. It never replays a movement request.
        client.ping_all()


def _disconnect():
    with _ACTION_LOCK:
        with STATE.lock:
            client, unsubscribe = STATE.client, STATE.unsubscribe
        if client is not None:
            try:
                client.disconnect()
                if unsubscribe is not None:
                    unsubscribe()
                STATE.drain()
            except Exception as exc:
                if unsubscribe is not None:
                    unsubscribe()
                try:
                    client.transport.close()
                finally:
                    with STATE.lock:
                        STATE.client = None
                        STATE.connected = False
                        STATE.uncertain = True
                        STATE.detail = f"Disconnect reported {type(exc).__name__}: {exc}"
                    STATE.enqueue(exc)


def _client_action(method, *args):
    with _ACTION_LOCK:
        with STATE.lock:
            client = STATE.client
            connected = STATE.connected
        if not connected or client is None:
            raise RuntimeError("Connect a transport first.")
        if not getattr(client.transport, "is_connected", True):
            raise ConnectionError("Transport reader is closed; reconnect and inspect fresh status.")
        if method in {"set_absolute_angle", "step"} and not STATE.can_move():
            raise RuntimeError("movement disabled until fresh, valid motor feedback is available and the motor is idle")
        if method in {"set_absolute_angle", "step", "stop"}:
            with STATE.lock:
                STATE.busy = True
                STATE.uncertain = True
        try:
            result = getattr(client, method)(*args)
        except ValueError:
            with STATE.lock:
                STATE.busy = False
                STATE.uncertain = False
            raise
        return result


def build_ui(ui):
    ui.page_title("Boat ground station")
    ui.colors(primary="#245b66", secondary="#53616a", accent="#9a5b22",
              positive="#376a4b", negative="#9a3d35", warning="#9a5b22")
    with ui.column().classes("w-full min-w-0 max-w-6xl mx-auto p-4 gap-3").style("overflow-wrap: anywhere"):
        ui.label("Boat ground station").classes("text-2xl font-semibold")
        mode_badge = ui.label("DISCONNECTED · choose a transport").classes(
            "rounded px-3 py-2 bg-amber-100 text-amber-950 font-semibold")
        detail_label = ui.label("No movement is sent on connect. Reconnect requires fresh status.").classes("text-sm")

        with ui.card().classes("w-full"):
            ui.label("Connection").classes("text-lg font-medium")
            with ui.row().classes("w-full flex-wrap items-end gap-3"):
                mode = ui.select(["LoRa · direct USB UART", "LoRa · ground Pico USB (raw MAVLink)",
                                  "SSH · Pi bridge",
                                  "SIMULATION · generated values"], value="LoRa · direct USB UART",
                                 label="Transport").classes("w-64")
                port = ui.input("USB serial port", value="/dev/ttyUSB0").classes("w-56")
                baud = ui.number("Baud", value=115200, min=1200, max=3000000, precision=0).classes("w-36")
                host = ui.input("SSH config host alias", value="boat-pi").classes("w-56")
                connect_btn = ui.button("Connect", color="primary")
                disconnect_btn = ui.button("Disconnect", color="secondary")
                ping_btn = ui.button("Ping all nodes")
                disconnect_btn.disable()
                ping_btn.disable()
            ui.label("SSH uses ~/.ssh/config, strict host key checking, and python3 -m onboard bridge --config /etc/arsem/onboard.toml.").classes("text-xs text-slate-600")
            with ui.expansion("Direct radio settings", icon="settings"):
                ui.label("Configure both modems to the same network, frequency, and parameters. Blank frequency leaves the modem's current band unchanged.").classes("text-xs text-slate-600")
                with ui.row().classes("w-full flex-wrap items-end gap-3"):
                    local_address = ui.number("Local modem address", value=2, min=0, max=65535, precision=0).classes("w-44")
                    peer_address = ui.number("Expected boat modem peer", value=1, min=0, max=65535, precision=0).classes("w-48")
                    network_id = ui.number("Network ID", value=18, min=3, max=18, precision=0).classes("w-36")
                    band_hz = ui.input("Band frequency Hz (optional)", value="").classes("w-56")
                    parameter = ui.input("RF parameters (SF,BW,CR,preamble)", value="10,7,1,12").classes("w-64")

        with ui.row().classes("w-full flex-wrap items-stretch"):
            with ui.card().classes("grow basis-80 min-w-0"):
                ui.label("Actuator control").classes("text-lg font-medium")
                ui.label("Movement uses fresh measured feedback. The displayed angle is measured feedback when the peer reports it; simulation is always labeled.").classes("text-sm")
                with ui.row().classes("w-full flex-wrap items-end gap-3"):
                    actuator = ui.select({1: "Sail", 2: "Rudder"}, value=1, label="Actuator").classes("w-40")
                    absolute = ui.number("Absolute angle (degrees)", value=0, min=-327.67,
                                         max=327.67, step=0.01, precision=2).classes("w-56")
                    absolute_btn = ui.button("Set absolute angle", color="primary")
                with ui.row().classes("w-full flex-wrap items-end gap-3"):
                    step_size = ui.number("Step size (degrees)", value=5, min=0.01,
                                          max=327.67, step=0.5, precision=2).classes("w-48")
                    left_btn = ui.button("− Step")
                    right_btn = ui.button("+ Step")
                    stop_btn = ui.button("STOP", color="negative")
                    for button in (absolute_btn, left_btn, right_btn, stop_btn):
                        button.disable()
                motor_text = ui.label("Motor: no status").classes("whitespace-pre-wrap text-sm")
            with ui.card().classes("grow basis-80 min-w-0"):
                ui.label("Nodes").classes("text-lg font-medium")
                node_labels = {}
                with ui.row().classes("w-full flex-wrap items-stretch"):
                    for key, title in (((1, 1), "Pi · router / IMU"),
                                       ((1, 154), "Motor Pico"),
                                       ((1, 158), "Comms Pico (optional)")):
                        with ui.card().classes("grow basis-52 min-w-0"):
                            ui.label(title).classes("font-medium")
                            node_labels[key] = ui.label("No heartbeat/status").classes("whitespace-pre-wrap text-sm")

        with ui.card().classes("w-full"):
            ui.label("IMU · Pi acquisition owner").classes("text-lg font-medium")
            imu_text = ui.label("No IMU sample received.").classes("text-sm")
            chart = ui.echart({"animation": False, "tooltip": {"trigger": "axis"},
                "legend": {"data": ["accel x", "accel y", "accel z", "gyro x", "gyro y", "gyro z"]},
                "grid": {"left": 48, "right": 12, "top": 36, "bottom": 28},
                "xAxis": {"type": "category", "data": []},
                "yAxis": [{"type": "value", "name": "mg"}, {"type": "value", "name": "mrad/s"}],
                "series": [{"name": n, "type": "line", "showSymbol": False, "data": [],
                            "yAxisIndex": 1 if n.startswith("gyro") else 0}
                           for n in ("accel x", "accel y", "accel z", "gyro x", "gyro y", "gyro z")]})

        with ui.card().classes("w-full"):
            with ui.row().classes("w-full items-center justify-between"):
                ui.label("Packet and event log").classes("text-lg font-medium")
                export_btn = ui.button("Export JSONL")
            trace_text = ui.label("Waiting for events.").classes("w-full min-w-0 max-w-full font-mono text-xs whitespace-pre-wrap max-h-64 overflow-auto").style("overflow-wrap: anywhere")

        def refresh():
            with STATE.lock:
                active_transport = STATE.transport
            log_queue = getattr(active_transport, "stderr_lines", None)
            if log_queue is not None:
                while True:
                    try:
                        STATE.enqueue(TransportLog(active_transport.name, log_queue.get_nowait()))
                    except queue.Empty:
                        break
            STATE.drain()
            now = time.monotonic()
            with STATE.lock:
                connected = STATE.connected and bool(getattr(STATE.transport, "is_connected", True))
                if STATE.connected and not connected:
                    STATE.connected = False
                    STATE.uncertain = True
                    STATE.feedback_valid = False
                    STATE.detail = "Transport closed; reconnect and inspect status. No movement was resent."
                simulation = STATE.transport is not None and STATE.transport.name.startswith("SIMULATION")
                mode_badge.set_text(("CONNECTED · " if connected else "DISCONNECTED · ") + STATE.mode)
                mode_badge.classes(replace="rounded px-3 py-2 font-semibold " +
                    ("bg-emerald-100 text-emerald-950" if connected else "bg-amber-100 text-amber-950"))
                detail_label.set_text(STATE.detail + (" · SIMULATED VALUES" if simulation else ""))
                can_move = STATE.can_move()
                absolute_btn.set_enabled(can_move)
                left_btn.set_enabled(can_move)
                right_btn.set_enabled(can_move)
                connect_btn.set_enabled(not connected)
                disconnect_btn.set_enabled(connected)
                ping_btn.set_enabled(connected)
                stop_btn.set_enabled(connected)
                for field in (mode, port, baud, host, local_address, peer_address,
                              network_id, band_hz, parameter):
                    field.set_enabled(not connected)
                selected_actuator = int(actuator.value)
                selected_report = STATE.motor_reports.get(selected_actuator)
                selected_move = STATE.can_move_actuator(selected_actuator)
                absolute_btn.set_enabled(selected_move)
                left_btn.set_enabled(selected_move)
                right_btn.set_enabled(selected_move)
                if selected_report:
                    m = selected_report
                    age = max(0.0, now - STATE.motor_report_at.get(selected_actuator, now))
                    measured = (f"{m.measured_position_cdeg / 100:.2f}°" if STATE.feedback_valid
                                else "invalid")
                    motor_text.set_text(
                        f"{('Sail' if selected_actuator == 1 else 'Rudder')} · source {m.source[0]}/{m.source[1]} · sample age {age:.1f}s\n"
                        f"Measured: {measured} · requested: {m.requested_position_cdeg / 100:.2f}°\n"
                        f"Enabled: {bool(m.enabled)} · faults: 0x{m.fault_flags:04x} · result: {m.result}\n"
                        f"Feedback valid: {STATE.feedback_valid} · busy: {STATE.busy} · outcome uncertain: {STATE.uncertain}"
                        + ("\nSTALE FEEDBACK · movement disabled" if age > 5.0 else "")
                        + ("\nSIMULATED MEASUREMENTS" if simulation else ""))
                else:
                    motor_text.set_text(f"{('Sail' if selected_actuator == 1 else 'Rudder')}: no measured status; movement disabled." +
                                        ("\nSIMULATION MODE" if simulation else ""))
                for source, label in node_labels.items():
                    if source == (1, 158) and STATE.mode.startswith("LoRa · direct USB UART"):
                        label.set_text("Not used in direct USB-UART mode")
                        continue
                    node = STATE.nodes.get(source, {})
                    age = max(0.0, now - node.get("seen", now))
                    health = node.get("health")
                    ping = node.get("rtt")
                    h = "health unknown" if health is None else (
                        f"boot {health.boot_id} · faults 0x{health.fault_flags:04x} · "
                        f"queue drops {health.queue_drops} · feedback {health.feedback_valid}")
                    link = node.get("link")
                    errors = ("link errors unknown" if link is None else
                              f"link RX/TX errors {link['rx_errors']}/{link['tx_errors']}")
                    label.set_text((f"{source[0]}/{source[1]} · last seen {age:.1f}s · "
                        f"RTT {'unknown' if ping is None else f'{ping:.1f} ms'}\n{h}\n{errors}")
                        + ("\nSTALE" if age > 5.0 else "")
                        if node else "No heartbeat/status")
                if STATE.imu:
                    at, sample = STATE.imu[-1]
                    age = now - at
                    valid = sample.valid_fields
                    accel_ok, gyro_ok = bool(valid & 1), bool(valid & 2)
                    values = (", ".join(map(str, sample.accel_mg)) if accel_ok else "invalid",
                              ", ".join(map(str, sample.gyro_mrad_s)) if gyro_ok else "invalid")
                    imu_text.set_text(f"Source {sample.source[0]}/{sample.source[1]} (Pi) · age {age:.2f}s · "
                        f"acceleration x/y/z: {values[0]} mg · angular rate x/y/z: {values[1]} mrad/s · "
                        f"magnetometer/temperature invalid unless separately reported"
                        + (" · SIMULATED SAMPLE" if simulation else "")
                        + (" · STALE" if age > 2.0 else ""))
                    recent = list(STATE.imu)[-60:]
                    data = [[], [], [], [], [], []]
                    times = []
                    for sample_at, item in recent:
                        times.append(f"{sample_at:.1f}")
                        for index, val in enumerate(item.accel_mg):
                            data[index].append(val if item.valid_fields & 1 else None)
                        for index, val in enumerate(item.gyro_mrad_s):
                            data[index + 3].append(val if item.valid_fields & 2 else None)
                    chart.options["xAxis"]["data"] = times
                    for series, points in zip(chart.options["series"], data):
                        series["data"] = points
                    chart.update()
                else:
                    imu_text.set_text("No IMU sample received. Acceleration is mg; angular rate is mrad/s. Acquisition owner: Pi.")
                recent_trace = list(STATE.trace)[-30:]
                trace_text.set_text((f"TRACE EVENTS DROPPED: {STATE.trace_drops}\n" if STATE.trace_drops else "") +
                                    "\n".join(json.dumps(item, default=str, separators=(",", ":"))
                                               for item in reversed(recent_trace)) or "Waiting for events.")

        def connect_selected():
            settings = None
            if mode.value == "LoRa · direct USB UART":
                try:
                    settings = {"local_address": int(local_address.value),
                                "peer_address": int(peer_address.value),
                                "network_id": int(network_id.value),
                                "band_hz": int(band_hz.value) if str(band_hz.value).strip() else None,
                                "parameter": str(parameter.value).strip()}
                except (TypeError, ValueError):
                    ui.notify("Check modem addresses, network ID and frequency.", type="negative")
                    return
            _call_background(_connect, mode.value, port.value, baud.value, host.value, settings)
        connect_btn.on_click(connect_selected)
        disconnect_btn.on_click(lambda: _call_background(_disconnect))
        ping_btn.on_click(lambda: _call_background(_client_action, "ping_all"))
        absolute_btn.on_click(lambda: _call_background(_client_action, "set_absolute_angle",
                                                        int(actuator.value), round(float(absolute.value) * 100)))
        left_btn.on_click(lambda: _call_background(_client_action, "step", int(actuator.value),
                                                    -round(float(step_size.value) * 100)))
        right_btn.on_click(lambda: _call_background(_client_action, "step", int(actuator.value),
                                                     round(float(step_size.value) * 100)))
        stop_btn.on_click(lambda: _call_background(_client_action, "stop", int(actuator.value)))
        export_btn.on_click(lambda: ui.download(STATE.jsonl().encode("utf-8"), "boat-trace.jsonl"))
        ui.timer(0.25, refresh)


def main():
    import argparse
    from nicegui import ui
    parser = argparse.ArgumentParser(description="Boat ground station web app")
    parser.add_argument("--host", default="127.0.0.1", help="bind address (default: loopback only)")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()
    @ui.page("/")
    def index():
        build_ui(ui)

    ui.run(host=args.host, port=args.port, title="Boat ground station", reload=False,
           show=False)


if __name__ == "__main__":
    main()
