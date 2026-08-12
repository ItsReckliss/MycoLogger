"""Cross-platform discovery and ingestion for the USB receiver."""

from __future__ import annotations

import json
import logging
import sqlite3
import threading
from dataclasses import asdict, dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Any, Callable, Iterable

import serial
from serial.tools import list_ports

from .database import (
    acknowledge_command,
    get_pending_command,
    mark_command_sent,
    store_packet,
)
from .protocol import decode_radio_payload, encode_usb_config_command


LOGGER = logging.getLogger(__name__)
MYCOLOGGER_VID = 0x0483
MYCOLOGGER_PID = 0x5740


@dataclass
class ReceiverState:
    enabled: bool = True
    connected: bool = False
    verified: bool = False
    port: str | None = None
    message: str = "Searching for receiver"
    radio_state: str | None = None
    radio_model: str | None = None
    frequency_hz: int | None = None
    firmware_version: str | None = None
    last_record_utc: str | None = None
    last_packet_utc: str | None = None
    records_received: int = 0
    packets_received: int = 0
    measurements_stored: int = 0
    duplicates_ignored: int = 0
    commands_sent: int = 0
    config_acks_received: int = 0
    invalid_records: int = 0
    storage_errors: int = 0
    last_error: str | None = None


def find_receiver_port(
    requested_port: str | None = None,
    available_ports: Iterable[Any] | None = None,
) -> str | None:
    """Find the configured port or a USB device matching the receiver."""
    ports = list(available_ports if available_ports is not None else list_ports.comports())

    if requested_port:
        requested = requested_port.casefold()
        for port in ports:
            if str(port.device).casefold() == requested:
                return str(port.device)
        return None

    for port in ports:
        identity = " ".join(
            str(value)
            for value in (
                getattr(port, "product", None),
                getattr(port, "description", None),
                getattr(port, "manufacturer", None),
            )
            if value
        ).casefold()
        if "mycologger" in identity or "myco receiver" in identity:
            return str(port.device)

    for port in ports:
        if (
            getattr(port, "vid", None) == MYCOLOGGER_VID
            and getattr(port, "pid", None) == MYCOLOGGER_PID
        ):
            return str(port.device)
    return None


class ReceiverService:
    """Own the serial port, reconnect automatically, and ingest records."""

    def __init__(
        self,
        database_path: Path,
        *,
        enabled: bool = True,
        requested_port: str | None = None,
        baud: int = 115200,
        retry_seconds: float = 1.0,
    ) -> None:
        self.database_path = database_path
        self.enabled = enabled
        self.requested_port = requested_port
        self.baud = baud
        self.retry_seconds = max(retry_seconds, 0.1)
        self._state = ReceiverState(enabled=enabled)
        if not enabled:
            self._state.message = "Receiver service disabled"
        self._state_lock = threading.Lock()
        self._stop_event = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        if not self.enabled or (self._thread and self._thread.is_alive()):
            return
        self._stop_event.clear()
        self._thread = threading.Thread(
            target=self._run,
            name="mycologger-receiver",
            daemon=True,
        )
        self._thread.start()

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread:
            self._thread.join(timeout=3)
        self._set_state(connected=False, verified=False, port=None)

    def snapshot(self) -> dict[str, Any]:
        with self._state_lock:
            return asdict(self._state)

    def _set_state(self, **changes: Any) -> None:
        with self._state_lock:
            for key, value in changes.items():
                setattr(self._state, key, value)

    def _increment(self, field: str) -> None:
        with self._state_lock:
            setattr(self._state, field, getattr(self._state, field) + 1)

    def _run(self) -> None:
        while not self._stop_event.is_set():
            port = find_receiver_port(self.requested_port)
            if port is None:
                self._set_state(
                    connected=False,
                    verified=False,
                    port=None,
                    message="Waiting for USB receiver",
                )
                self._stop_event.wait(self.retry_seconds)
                continue

            try:
                with serial.Serial(port, self.baud, timeout=1) as receiver:
                    LOGGER.info("USB receiver opened on %s", port)
                    self._set_state(
                        connected=True,
                        verified=False,
                        port=port,
                        message="Receiver connected; waiting for data",
                        last_error=None,
                    )
                    # v0.6+ answers immediately; older receiver firmware
                    # safely ignores this backward-compatible query.
                    receiver.write(b"INFO\n")
                    while not self._stop_event.is_set():
                        raw_line = receiver.readline()
                        if raw_line:
                            self.process_line(raw_line, command_writer=receiver.write)
            except (serial.SerialException, OSError) as exc:
                LOGGER.warning("Receiver disconnected from %s: %s", port, exc)
                self._set_state(
                    connected=False,
                    verified=False,
                    port=None,
                    message="Receiver disconnected; reconnecting",
                    last_error=str(exc),
                )
                self._stop_event.wait(self.retry_seconds)

    def process_line(
        self,
        raw_line: bytes,
        command_writer: Callable[[bytes], Any] | None = None,
    ) -> bool:
        """Validate and ingest one NDJSON line. Exposed for deterministic tests."""
        try:
            record = json.loads(raw_line.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            self._increment("invalid_records")
            return False
        if not isinstance(record, dict) or record.get("v") != 1:
            self._increment("invalid_records")
            return False

        now = datetime.now(UTC).isoformat()
        record_type = record.get("type")
        self._increment("records_received")
        self._set_state(
            verified=True,
            last_record_utc=now,
            message="Receiver online",
        )

        if record_type == "status":
            changes: dict[str, Any] = {"radio_state": record.get("radio_state")}
            if record.get("fw"):
                changes["firmware_version"] = record["fw"]
            self._set_state(**changes)
            return True
        if record_type == "hello":
            self._set_state(firmware_version=record.get("fw"))
            return True
        if record_type == "radio":
            self._set_state(
                radio_state=record.get("state"),
                radio_model=record.get("model"),
                frequency_hz=record.get("frequency_hz"),
            )
            return True
        if record_type != "packet":
            return True

        decoded = decode_radio_payload(record)
        if decoded is None:
            self._increment("invalid_records")
            return False

        self._increment("packets_received")
        self._set_state(last_packet_utc=now)

        if decoded["packet_type"] == "config_ack":
            acknowledged = acknowledge_command(
                self.database_path,
                node_id=int(decoded["node_id"]),
                transaction_id=int(decoded["transaction_id"]),
                config_revision=int(decoded["config_revision"]),
                report_interval_s=int(decoded["report_interval_s"]),
                status=int(decoded["config_status"]),
            )
            if acknowledged:
                self._increment("config_acks_received")
            return acknowledged

        try:
            result = store_packet(self.database_path, decoded, record, now)
        except (OSError, sqlite3.Error, ValueError, TypeError, KeyError) as exc:
            LOGGER.exception("Could not store receiver packet")
            self._increment("storage_errors")
            self._set_state(last_error=str(exc))
            return False

        if decoded["packet_type"] == "sensor_reading":
            self._increment(
                "measurements_stored" if result.inserted else "duplicates_ignored"
            )

        if command_writer is not None:
            command = get_pending_command(
                self.database_path, int(decoded["node_id"])
            )
            if command is not None:
                command_writer(encode_usb_config_command(command))
                mark_command_sent(
                    self.database_path, int(command["transaction_id"])
                )
                self._increment("commands_sent")
        return True
