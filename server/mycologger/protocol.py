"""Decode and validate MycoLogger LoRa payloads received over USB."""

from __future__ import annotations

from typing import Any


SCD41_ERRORS = {
    0: "none",
    1: "serial_command_nack",
    2: "serial_read_nack",
    3: "serial_crc",
    4: "single_shot_nack",
    5: "ready_command_nack",
    6: "ready_read_nack",
    7: "ready_crc",
    8: "measurement_timeout",
    9: "measurement_command_nack",
    10: "measurement_read_nack",
    11: "measurement_crc",
    12: "scl_held_low",
    13: "sda_held_low",
    14: "address_nack",
    15: "command_msb_nack",
    16: "command_lsb_nack",
}


def decode_radio_payload(record: dict[str, Any]) -> dict[str, Any] | None:
    """Return a validated decoded packet, or None for an unknown payload."""
    if record.get("type") != "packet":
        return None
    payload_hex = record.get("payload_hex")
    if not isinstance(payload_hex, str):
        return None
    try:
        payload = bytes.fromhex(payload_hex)
    except ValueError:
        return None

    if len(payload) < 14 or payload[:4] != b"MYCO":
        return None
    version = payload[4]
    packet_type = payload[5]
    if version != 1 or packet_type not in (1, 2, 3, 0x81):
        return None
    if packet_type == 2 and len(payload) < 26:
        return None
    if packet_type == 0x81 and len(payload) < 23:
        return None

    if packet_type == 0x81:
        return {
            "protocol": version,
            "packet_type": "config_ack",
            "node_id": int.from_bytes(payload[6:10], "big"),
            "transaction_id": int.from_bytes(payload[10:14], "big"),
            "config_revision": int.from_bytes(payload[14:18], "big"),
            "config_status": payload[18],
            "report_interval_s": int.from_bytes(payload[19:23], "big"),
        }

    if packet_type == 3:
        return {
            "protocol": version,
            "packet_type": "link_check",
            "node_id": int.from_bytes(payload[6:10], "big"),
            "tx_sequence": int.from_bytes(payload[10:14], "big"),
        }

    flags = payload[18]
    decoded: dict[str, Any] = {
        "protocol": version,
        "packet_type": "link_test" if packet_type == 1 else "sensor_reading",
        "node_id": int.from_bytes(payload[6:10], "big"),
        "tx_sequence": int.from_bytes(payload[10:14], "big"),
        "tx_uptime_s": int.from_bytes(payload[14:18], "big"),
        "button_pressed": bool(flags & 0x01),
        "network_confirmation_requested": bool(flags & 0x08),
    }

    if packet_type == 2:
        sensor_valid = bool(flags & 0x02)
        decoded["sensor_valid"] = sensor_valid
        if sensor_valid:
            decoded["co2_ppm"] = int.from_bytes(payload[19:21], "big")
            decoded["temperature_c"] = (
                int.from_bytes(payload[21:23], "big", signed=True) / 100.0
            )
            decoded["humidity_percent"] = (
                int.from_bytes(payload[23:25], "big") / 100.0
            )
        error_code = payload[25]
        decoded["sensor_error"] = SCD41_ERRORS.get(
            error_code, f"unknown_{error_code}"
        )
        if len(payload) >= 34:
            decoded["config_revision"] = int.from_bytes(payload[26:30], "big")
            decoded["report_interval_s"] = int.from_bytes(payload[30:34], "big")
        if len(payload) >= 36:
            battery_valid = bool(flags & 0x04)
            decoded["battery_valid"] = battery_valid
            if battery_valid:
                battery_mv = int.from_bytes(payload[34:36], "big")
                decoded["battery_mv"] = battery_mv
                decoded["battery_voltage_v"] = battery_mv / 1000.0
        if len(payload) >= 39:
            decoded["firmware_version"] = (
                f"{payload[36]}.{payload[37]}.{payload[38]}"
            )
        if len(payload) >= 47:
            decoded["reset_flags"] = int.from_bytes(payload[39:43], "big")
            decoded["sensor_failure_count"] = int.from_bytes(payload[43:45], "big")
            decoded["radio_failure_count"] = int.from_bytes(payload[45:47], "big")

    return decoded


def encode_usb_config_command(command: dict[str, Any]) -> bytes:
    """Encode one database command for the small receiver USB parser."""
    return (
        "CFG "
        f"{int(command['node_id'])} "
        f"{int(command['transaction_id'])} "
        f"{int(command['desired_revision'])} "
        f"{int(command['report_interval_s'])}\n"
    ).encode("ascii")
