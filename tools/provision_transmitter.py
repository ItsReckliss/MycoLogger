#!/usr/bin/env python3
"""Flash or provision one MycoLogger transmitter through an ST-LINK."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Callable


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIRMWARE = (
    PROJECT_ROOT
    / "firmware"
    / "transmitter"
    / "provisioning"
    / "MycoLogger-Transmitter-Universal.hex"
)
UID_ADDRESS = 0x1FFF3E50
UID_SIZE = 12
CONFIG_ADDRESS = 0x08007800
CONFIG_PAGE = 15
CONFIG_SIZE = 32
CONFIG_MAGIC = 0x4D594346
CONFIG_LAYOUT_VERSION = 1
CONFIG_PROVISIONING_MARKER = 0x50524F56


class ProvisioningError(RuntimeError):
    pass


def fnv1a32(data: bytes) -> int:
    value = 2166136261
    for byte in data:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def build_config_image(
    node_id: int,
    report_interval_s: int,
    downlink_window_ms: int,
    *,
    revision: int = 0,
    last_transaction_id: int = CONFIG_PROVISIONING_MARKER,
) -> bytes:
    if not 1 <= node_id <= 0xFFFFFFFE:
        raise ProvisioningError("Node ID must be between 1 and 4294967294")
    if not 15 <= report_interval_s <= 604800:
        raise ProvisioningError("Report interval must be between 15 and 604800 seconds")
    if not 100 <= downlink_window_ms <= 60000:
        raise ProvisioningError("Downlink window must be between 100 and 60000 ms")
    values = (
        CONFIG_MAGIC,
        CONFIG_LAYOUT_VERSION,
        node_id,
        report_interval_s * 1000,
        downlink_window_ms,
        revision,
        last_transaction_id,
    )
    body = struct.pack("<7I", *values)
    return body + struct.pack("<I", fnv1a32(body))


def parse_config_image(image: bytes) -> dict[str, int] | None:
    """Decode a valid reserved-page record without trusting erased flash."""
    if len(image) != CONFIG_SIZE:
        return None
    values = struct.unpack("<8I", image)
    if (
        values[0] != CONFIG_MAGIC
        or values[1] != CONFIG_LAYOUT_VERSION
        or not 1 <= values[2] <= 0xFFFFFFFE
        or values[3] % 1000 != 0
        or not 15 <= values[3] // 1000 <= 604800
        or not 100 <= values[4] <= 60000
        or values[7] != fnv1a32(image[:28])
    ):
        return None
    return {
        "node_id": values[2],
        "report_interval_s": values[3] // 1000,
        "downlink_window_ms": values[4],
        "revision": values[5],
        "last_transaction_id": values[6],
    }


def validate_firmware_image(path: Path) -> None:
    if not path.is_file():
        raise ProvisioningError(f"Firmware file not found: {path}")
    suffix = path.suffix.casefold()
    if suffix == ".bin":
        if path.stat().st_size > (CONFIG_ADDRESS - 0x08000000):
            raise ProvisioningError(
                "Binary firmware overlaps the reserved configuration page"
            )
        return
    if suffix not in {".hex", ".ihex"}:
        return
    upper_address = 0
    for line_number, raw_line in enumerate(
        path.read_text(encoding="ascii").splitlines(), start=1
    ):
        line = raw_line.strip()
        if not line:
            continue
        try:
            record = bytes.fromhex(line.removeprefix(":"))
        except ValueError as exc:
            raise ProvisioningError(
                f"Invalid Intel HEX data on line {line_number}"
            ) from exc
        if len(record) < 5 or (sum(record) & 0xFF) != 0:
            raise ProvisioningError(
                f"Invalid Intel HEX checksum on line {line_number}"
            )
        size = record[0]
        offset = int.from_bytes(record[1:3], "big")
        record_type = record[3]
        data = record[4 : 4 + size]
        if len(data) != size:
            raise ProvisioningError(f"Truncated Intel HEX line {line_number}")
        if record_type == 0x04:
            if size != 2:
                raise ProvisioningError("Invalid extended linear address record")
            upper_address = int.from_bytes(data, "big") << 16
        elif record_type == 0x00 and size:
            start = upper_address + offset
            end = start + size
            if start < (CONFIG_ADDRESS + 0x800) and end > CONFIG_ADDRESS:
                raise ProvisioningError(
                    "Firmware image contains data in the reserved configuration page"
                )


def find_programmer(explicit: str | None = None) -> Path:
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit).expanduser())
    environment = os.environ.get("STM32_PROGRAMMER_CLI")
    if environment:
        candidates.append(Path(environment).expanduser())
    discovered = shutil.which("STM32_Programmer_CLI.exe") or shutil.which(
        "STM32_Programmer_CLI"
    )
    if discovered:
        candidates.append(Path(discovered))
    candidates.extend(
        [
            Path(r"F:\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"),
            Path(
                "C:\\Program Files\\STMicroelectronics\\STM32Cube\\"
                "STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe"
            ),
            Path("/opt/st/stm32cubeprogrammer/bin/STM32_Programmer_CLI"),
            Path(
                "/Applications/STMicroelectronics/STM32Cube/"
                "STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/"
                "MacOs/bin/STM32_Programmer_CLI"
            ),
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise ProvisioningError(
        "STM32_Programmer_CLI was not found. Install STM32CubeProgrammer or "
        "set STM32_PROGRAMMER_CLI to its full path."
    )


def api_request(
    server: str,
    method: str,
    path: str,
    payload: dict[str, object] | None = None,
    timeout: float = 12.0,
) -> dict[str, object]:
    url = f"{server.rstrip('/')}{path}"
    data = None
    headers = {"Accept": "application/json"}
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        try:
            detail = json.loads(exc.read().decode("utf-8")).get("detail")
        except (ValueError, AttributeError):
            detail = None
        raise ProvisioningError(str(detail or f"Server returned HTTP {exc.code}")) from exc
    except (urllib.error.URLError, TimeoutError) as exc:
        raise ProvisioningError(f"Could not reach MycoLogger server at {server}: {exc}") from exc


class STM32Programmer:
    def __init__(
        self,
        executable: Path,
        *,
        probe_serial: str | None = None,
        frequency_khz: int = 1000,
        log: Callable[[str], None] = print,
    ) -> None:
        self.executable = executable
        self.probe_serial = probe_serial
        self.frequency_khz = frequency_khz
        self.log = log

    def _connection(self) -> list[str]:
        arguments = [
            "-c",
            "port=SWD",
            "mode=UR",
            "reset=HWrst",
            f"freq={self.frequency_khz}",
        ]
        if self.probe_serial:
            arguments.append(f"sn={self.probe_serial}")
        return arguments

    def run(self, *arguments: str, timeout: int = 90) -> str:
        command = [str(self.executable), *self._connection(), *arguments, "-q"]
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
            creationflags=(
                subprocess.CREATE_NO_WINDOW
                if os.name == "nt" and hasattr(subprocess, "CREATE_NO_WINDOW")
                else 0
            ),
        )
        output = "\n".join(
            item.strip() for item in (completed.stdout, completed.stderr) if item.strip()
        )
        if completed.returncode != 0 or "Error:" in output or "Error :" in output:
            raise ProvisioningError(
                output or f"STM32CubeProgrammer exited with code {completed.returncode}"
            )
        return output

    def read_memory(self, address: int, size: int) -> bytes:
        with tempfile.TemporaryDirectory(prefix="mycologger-read-") as directory:
            destination = Path(directory) / "readback.bin"
            self.run("-halt", "-u", hex(address), str(size), str(destination))
            if not destination.is_file():
                raise ProvisioningError("STM32CubeProgrammer did not create a readback file")
            content = destination.read_bytes()
            if len(content) != size:
                raise ProvisioningError(
                    f"Expected {size} readback bytes but received {len(content)}"
                )
            return content

    def read_hardware_uid(self) -> str:
        self.log("Reading the STM32 hardware UID...")
        return self.read_memory(UID_ADDRESS, UID_SIZE).hex().upper()

    def flash_and_verify(self, firmware: Path, config: bytes) -> None:
        if not firmware.is_file():
            raise ProvisioningError(f"Firmware file not found: {firmware}")
        with tempfile.TemporaryDirectory(prefix="mycologger-provision-") as directory:
            config_path = Path(directory) / "node-config.bin"
            config_path.write_bytes(config)
            self.log(f"Flashing universal firmware: {firmware.name}")
            self.run("-halt", "-w", str(firmware), "-v", timeout=150)
            self.log("Writing the reserved node configuration page...")
            self.run(
                "-halt", "-w", str(config_path), hex(CONFIG_ADDRESS), "-v", timeout=90
            )
            self.log("Reading the configuration back for byte-for-byte verification...")
            readback = self.read_memory(CONFIG_ADDRESS, len(config))
            if readback != config:
                raise ProvisioningError("Configuration readback did not match the written image")

    def rollback_unprovisioned(self) -> None:
        self.log("Rolling the transmitter back to silent/unprovisioned state...")
        self.run("-halt", "-e", str(CONFIG_PAGE), "-rst", timeout=60)

    def restore_config(self, config: bytes) -> None:
        self.log("Restoring the transmitter's previous node configuration...")
        with tempfile.TemporaryDirectory(prefix="mycologger-restore-") as directory:
            config_path = Path(directory) / "node-config.bin"
            config_path.write_bytes(config)
            self.run("-halt", "-w", str(config_path), hex(CONFIG_ADDRESS), "-v", timeout=90)

    def reset(self) -> None:
        self.run("-rst", timeout=30)


def provision(
    *,
    server: str,
    firmware: Path,
    programmer_path: Path,
    requested_node_id: int | None,
    report_interval_s: int,
    downlink_window_ms: int,
    probe_serial: str | None,
    preserve_existing_config: bool = True,
    log: Callable[[str], None] = print,
) -> dict[str, object]:
    validate_firmware_image(firmware)
    programmer = STM32Programmer(
        programmer_path, probe_serial=probe_serial, log=log
    )
    hardware_uid = programmer.read_hardware_uid()
    log(f"Detected transmitter UID: {hardware_uid}")
    existing_config_image = programmer.read_memory(CONFIG_ADDRESS, CONFIG_SIZE)
    existing_config = parse_config_image(existing_config_image)
    if existing_config is not None:
        log(
            "Detected existing Node "
            f"{existing_config['node_id']} configuration "
            f"({existing_config['report_interval_s']} s interval)."
        )
    status = api_request(
        server,
        "GET",
        "/api/provisioning/status?"
        + urllib.parse.urlencode({"hardware_uid": hardware_uid}),
    )
    registered = status.get("registered")
    if isinstance(registered, dict):
        log(f"This transmitter is already registered as Node {registered['node_id']}.")
        if (
            existing_config is not None
            and existing_config["node_id"] != int(registered["node_id"])
        ):
            raise ProvisioningError(
                f"Hardware contains Node {existing_config['node_id']}, but UID "
                f"{hardware_uid} is registered as Node {registered['node_id']}. "
                "Resolve this identity conflict before flashing."
            )
    payload: dict[str, object] = {"hardware_uid": hardware_uid}
    effective_node_id = (
        requested_node_id
        if requested_node_id is not None
        else (existing_config["node_id"] if existing_config is not None else None)
    )
    if effective_node_id is not None:
        payload["requested_node_id"] = effective_node_id
    reservation_response = api_request(
        server, "POST", "/api/provisioning/reservations", payload
    )
    reservation = reservation_response["reservation"]
    if not isinstance(reservation, dict):
        raise ProvisioningError("Server returned an invalid reservation")
    token = str(reservation["reservation_token"])
    node_id = int(reservation["node_id"])
    log(f"Reserved Node {node_id} until {reservation['expires_utc']}.")
    identity_changed = (
        existing_config is not None and existing_config["node_id"] != node_id
    )
    if existing_config is not None and preserve_existing_config and not identity_changed:
        config = existing_config_image
        report_interval_s = existing_config["report_interval_s"]
        downlink_window_ms = existing_config["downlink_window_ms"]
        log("Keeping the existing node ID and device parameters.")
    elif existing_config is not None and identity_changed:
        config = build_config_image(
            node_id,
            (existing_config["report_interval_s"]
             if preserve_existing_config else report_interval_s),
            (existing_config["downlink_window_ms"]
             if preserve_existing_config else downlink_window_ms),
        )
        report_interval_s = (
            existing_config["report_interval_s"]
            if preserve_existing_config else report_interval_s
        )
        downlink_window_ms = (
            existing_config["downlink_window_ms"]
            if preserve_existing_config else downlink_window_ms
        )
        log(
            f"Explicitly renumbering Node {existing_config['node_id']} to "
            f"Node {node_id}; other device parameters are "
            f"{'preserved' if preserve_existing_config else 'replaced'}."
        )
    elif existing_config is not None:
        changed = (
            report_interval_s != existing_config["report_interval_s"]
            or downlink_window_ms != existing_config["downlink_window_ms"]
        )
        config = build_config_image(
            node_id,
            report_interval_s,
            downlink_window_ms,
            revision=existing_config["revision"] + (1 if changed else 0),
            last_transaction_id=existing_config["last_transaction_id"],
        )
        log("Keeping the existing node ID and applying the entered parameters.")
    else:
        config = build_config_image(node_id, report_interval_s, downlink_window_ms)
    flash_attempted = False
    registration_completed = False
    try:
        flash_attempted = True
        programmer.flash_and_verify(firmware, config)
        log("Registering the verified transmitter with the MycoLogger server...")
        completed = api_request(
            server,
            "POST",
            "/api/provisioning/complete",
            {"hardware_uid": hardware_uid, "reservation_token": token},
        )
        registration_completed = True
        programmer.reset()
        log(f"Flash complete. The transmitter is Node {node_id}.")
        return {
            "hardware_uid": hardware_uid,
            "node_id": node_id,
            "report_interval_s": report_interval_s,
            "server": server,
            "registration": completed.get("transmitter"),
        }
    except Exception:
        if flash_attempted and not registration_completed:
            try:
                if existing_config is not None:
                    programmer.restore_config(existing_config_image)
                else:
                    programmer.rollback_unprovisioned()
            except Exception as rollback_error:
                log(f"WARNING: automatic hardware rollback failed: {rollback_error}")
        if not registration_completed:
            try:
                api_request(
                    server,
                    "POST",
                    "/api/provisioning/cancel",
                    {"hardware_uid": hardware_uid, "reservation_token": token},
                )
            except Exception as cancel_error:
                log(f"WARNING: reservation cancellation failed: {cancel_error}")
        raise


def cli_main(arguments: argparse.Namespace) -> int:
    programmer = find_programmer(arguments.programmer)
    requested = None if arguments.node_id in (None, 0) else arguments.node_id
    result = provision(
        server=arguments.server,
        firmware=Path(arguments.firmware).expanduser().resolve(),
        programmer_path=programmer,
        requested_node_id=requested,
        report_interval_s=arguments.report_interval,
        downlink_window_ms=arguments.downlink_window,
        preserve_existing_config=arguments.keep_existing_config,
        probe_serial=arguments.probe_serial,
    )
    print(json.dumps(result, indent=2))
    return 0


def gui_main(arguments: argparse.Namespace) -> int:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    root = tk.Tk()
    root.title("MycoLogger Transmitter Flash Utility")
    root.geometry("760x590")
    root.minsize(680, 520)

    server = tk.StringVar(value=arguments.server)
    firmware = tk.StringVar(value=str(Path(arguments.firmware).resolve()))
    programmer = tk.StringVar(value=str(find_programmer(arguments.programmer)))
    node_id = tk.StringVar(value="Automatic")
    interval = tk.StringVar(value=str(arguments.report_interval))
    downlink = tk.StringVar(value=str(arguments.downlink_window))
    probe_serial = tk.StringVar(value=arguments.probe_serial or "")
    keep_existing = tk.BooleanVar(value=True)
    status = tk.StringVar(value="Connect an ST-LINK and powered transmitter, then flash.")

    outer = ttk.Frame(root, padding=16)
    outer.pack(fill="both", expand=True)
    outer.columnconfigure(1, weight=1)

    def row(label: str, variable: tk.StringVar, index: int) -> ttk.Entry:
        ttk.Label(outer, text=label).grid(row=index, column=0, sticky="w", padx=(0, 10), pady=5)
        entry = ttk.Entry(outer, textvariable=variable)
        entry.grid(row=index, column=1, sticky="ew", pady=5)
        return entry

    row("MycoLogger server", server, 0)
    firmware_entry = row("Firmware image", firmware, 1)
    ttk.Button(
        outer,
        text="Browse",
        command=lambda: firmware.set(
            filedialog.askopenfilename(
                initialfile=Path(firmware.get()).name,
                filetypes=[("STM32 firmware", "*.elf *.hex *.bin"), ("All files", "*.*")],
            ) or firmware.get()
        ),
    ).grid(row=1, column=2, padx=(8, 0))
    row("STM32 Programmer CLI", programmer, 2)
    row("ST-LINK serial (optional)", probe_serial, 3)
    node_entry = row("Node ID", node_id, 4)
    node_entry.configure(validate="none")
    row("Report interval (seconds)", interval, 5)
    row("Downlink window (ms)", downlink, 6)
    ttk.Checkbutton(
        outer,
        text="Keep existing device parameters (Automatic retains node ID)",
        variable=keep_existing,
    ).grid(row=7, column=0, columnspan=3, sticky="w", pady=(6, 2))

    ttk.Label(outer, textvariable=status).grid(
        row=8, column=0, columnspan=3, sticky="w", pady=(12, 6)
    )
    log_box = tk.Text(outer, height=15, wrap="word", state="disabled")
    log_box.grid(row=9, column=0, columnspan=3, sticky="nsew")
    outer.rowconfigure(9, weight=1)

    def append_log(message: str) -> None:
        def update() -> None:
            log_box.configure(state="normal")
            log_box.insert("end", message.rstrip() + "\n")
            log_box.see("end")
            log_box.configure(state="disabled")
            status.set(message)
        root.after(0, update)

    def run_provisioning() -> None:
        provision_button.configure(state="disabled")

        def worker() -> None:
            try:
                requested_text = node_id.get().strip()
                requested = (
                    None
                    if not requested_text or requested_text.casefold() == "automatic"
                    else int(requested_text)
                )
                result = provision(
                    server=server.get().strip(),
                    firmware=Path(firmware.get()).expanduser().resolve(),
                    programmer_path=find_programmer(programmer.get().strip()),
                    requested_node_id=requested,
                    report_interval_s=int(interval.get()),
                    downlink_window_ms=int(downlink.get()),
                    preserve_existing_config=keep_existing.get(),
                    probe_serial=probe_serial.get().strip() or None,
                    log=append_log,
                )
                root.after(
                    0,
                    lambda: messagebox.showinfo(
                        "Flash complete",
                        f"Transmitter {result['hardware_uid']} is now Node {result['node_id']}.",
                    ),
                )
            except Exception as exc:
                append_log(f"ERROR: {exc}")
                root.after(0, lambda: messagebox.showerror("Flash failed", str(exc)))
            finally:
                root.after(0, lambda: provision_button.configure(state="normal"))

        threading.Thread(target=worker, daemon=True).start()

    provision_button = ttk.Button(
        outer, text="Flash transmitter", command=run_provisioning
    )
    provision_button.grid(row=10, column=0, columnspan=3, sticky="e", pady=(12, 0))
    firmware_entry.focus_set()
    root.mainloop()
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", default="http://mycopi.local:8080")
    parser.add_argument("--firmware", default=str(DEFAULT_FIRMWARE))
    parser.add_argument("--programmer", help="Full path to STM32_Programmer_CLI")
    parser.add_argument("--probe-serial", help="Use a specific ST-LINK serial number")
    parser.add_argument("--node-id", type=int, help="Specific node ID; omit for automatic")
    parser.add_argument("--report-interval", type=int, default=900)
    parser.add_argument("--downlink-window", type=int, default=1500)
    parser.add_argument(
        "--replace-existing-config",
        dest="keep_existing_config",
        action="store_false",
        help="Apply entered parameters; Automatic still retains a known UID's node ID",
    )
    parser.set_defaults(keep_existing_config=True)
    parser.add_argument("--cli", action="store_true", help="Run without the graphical interface")
    return parser


def main() -> int:
    arguments = build_parser().parse_args()
    try:
        return cli_main(arguments) if arguments.cli else gui_main(arguments)
    except (ProvisioningError, ValueError, subprocess.TimeoutExpired) as exc:
        print(f"Provisioning failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
