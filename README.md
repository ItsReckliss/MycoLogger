# MycoLogger

MycoLogger is a battery-powered environmental monitoring system for mushroom
grows. STM32 transmitter nodes periodically power an SCD41 sensor, measure CO2,
temperature, humidity, and battery voltage, then send the reading over 915 MHz
LoRa. A USB receiver forwards packets to a Python service that stores them in
SQLite and serves a browser dashboard.

This file is the project entry point and development handoff. Keep it accurate
when architecture, hardware, firmware versions, deployment state, or major
working assumptions change. The detailed, actively maintained roadmap is
[TO-DO.md](TO-DO.md).

## System overview

```text
SCD41 + battery
      |
STM32U031 transmitter + SX1262
      |
  915 MHz LoRa
      |
SX1262 receiver + STM32F042
      |
 USB CDC/ACM serial (NDJSON)
      |
Python receiver service -> SQLite -> FastAPI dashboard
```

The intended production host is a BTT Pi v1.2 on the local network and
Tailscale. Development and previewing currently happen on Windows; the same
Python server is designed to run on Windows, Linux, macOS, and Raspberry Pi OS
with environment-specific paths and serial-port names.

## Current development state

Last reviewed: 2026-08-13.

- Server: v0.8.0, currently running locally on the Windows development PC at
  `http://127.0.0.1:8080`.
- Transmitter source/build and Node 1: v0.8.7.
- Physical test transmitter: Node 1, UID `0F0023000650335848323020`, currently
  flashed with v0.8.7 and registered to that permanent node ID. Its independent
  watchdog was bench-tested through an intentional unrefreshed timeout and
  confirmed by the captured `IWDGRSTF` reset-cause flag.
- Receiver source/build: v0.9.0. The physical receiver remains on v0.8.0;
  its watchdog reset has been bench-tested, but boot-link configuration delivery
  and downlink-window configuration await a user-performed USB flash.
- Node 1 reports transmitter firmware `0.8.7` and its original 60-second
  interval after a real erased-page recovery test. Its current bench-test
  interval is 20 seconds. The SCD41 automatic self-calibration disable command
  is sent at every sensor power-up because tub operation will not provide
  regular fresh-air exposure; a CRC-checked readback verified the setting.
- The BTT Pi target is `mycopi.local`, but deployment of the current server as a
  managed Pi service is not complete.
- Git remote: `https://github.com/ItsReckliss/MycoLogger.git`.
- Local work is committed by coherent change. Pushes are intentionally batched
  until a meaningful amount of work is ready.

Battery status: the physical PA0 divider measured `1.8427 V`, corresponding to
approximately `3.6854 V` at the battery through the equal-value divider. Live
ST-LINK inspection proved PA0 uses ADC channel 4 on this MCU, while its internal
VREFINT uses channel 12. Transmitter v0.6.5 converts those channels separately
to avoid stale EOC results and applies a `0.9992` calibration factor derived
from a simultaneous `3.752 V` battery-lead measurement.

Transmitter v0.8.2 enables the STM32 independent watchdog with an approximately
8-second LSI-clocked timeout. The main state-machine loop refreshes it once per
 second only after it completes a full pass, while dead loops and fault handlers deliberately do not. The watchdog
is frozen when a debugger halts the core, and the raw RCC reset-cause flags are
captured at boot in `g_boot_reset_flags`. Every normal sensor packet also carries
the reset flags and saturating sensor/radio operation-failure counters for the
current boot; the server stores each snapshot and exposes the latest values on
the node API. Recoverable sensor/radio failures are retried and counted rather
than causing watchdog resets.

Transmitter v0.8.5 puts the SX1262 into warm-start sleep whenever no radio
operation is active. Its LoRa configuration is retained and the driver wakes it
with NSS before the next transmit or downlink receive window. Node 1 completed
three 20-second post-flash reports with a zero radio-failure count; current
draw still needs power-profiler measurement.

The dashboard classifies reported battery voltage as dead at or below 3.35 V,
critical through 3.50 V, low through 3.70 V, medium below 4.20 V, and full at
or above 4.20 V. Battery fields use a thin colored lower accent; assigned tub
cards show a warning `!` for low/critical/dead states, and Diagnostics reports
the affected node. These server-side thresholds are intentionally fixed for now;
making them per-account/per-node settings is a later configuration feature.

Grow charts retain every valid stored measurement in the selected time range;
they do not bucket or average readings for display. Hovering a chart, or
touching and holding on mobile, reveals a vertical dotted cursor with the
timestamp and the three values nearest that point. This is practical at the
normal five-minute-or-longer reporting cadence and keeps short-interval bench
test data available for inspection.

Transmitter v0.8.7 enters STM32 Stop 2 only after radio/sensor/downlink/LED
state is idle. The internal RTC wake-up timer runs from LSI-derived 1 Hz time
and can arm a single sleep of up to 65,536 seconds; PC14's falling-edge EXTI
wakes an immediate manual measurement. The Node 1 option byte `IWDG_STOP` was
changed to freeze the watchdog during Stop mode; otherwise the firmware
intentionally falls back to normal WFI. Node 1 completed normal 20-second
report cycles with the sensor powered off, MCU in Stop 2, and radio in warm-start
sleep between reports. Button wake still awaits a physical press test.

Receiver v0.8.0 source also enables its independent watchdog. Because the STM32F042
LSI has a much wider specified tolerance, its 12.8-second nominal setting is
approximately 8.5 to 17.1 seconds across the full oscillator range. Receiver
status JSON includes the raw `reset_flags` value for watchdog-reset diagnosis.
The v0.7.0 receiver image was physically verified through a deliberately
unrefreshed watchdog timeout.

## Hardware

### Transmitter

- MCU: STM32U031F6P6
- Radio: SX1262 915 MHz TCXO module
- Sensor: Sensirion SCD41
- Sensor power: TPS22918 load switch controlled by PB7
- Battery sense: PA0 through a 470 kOhm / 470 kOhm divider and 10 nF capacitor
- Next transmitter revision: retain the always-connected 470 kOhm divider;
  expected divider draw is only about 5 mAh/month, so no divider load switch is
  planned.
- Next transmitter revision: 4-pin JST-SH SWD connector carrying SWDIO, SWCLK,
  NRST, and GND; the LDO powers the MCU while flashing, and accessible pads are
  retained for recovery.
- Next transmitter revision: 2 mm x 2 mm `I_In`/`I_Out` current-measurement
  pads between the main switch and regulator input, bridged by an optional 0201
  0-ohm jumper.
- Debug button: PC14, active low
- Debug LED: PF3, active high
- Radio SPI/control:
  - MOSI PB8
  - MISO PA7
  - SCK PA5
  - NSS PA3
  - BUSY PA1
  - DIO1 PB1
  - REST PA8 (the radio module manufacturer's net name)
- SCD41 software I2C:
  - SDA PA11
  - SCL PA12

### Receiver

- MCU: STM32F042F6P6
- Radio: SX1262 915 MHz TCXO module
- Native USB device: CDC/ACM serial
- Debug LED: PA1
- Debug button: PA2, active low
- Radio SPI/control:
  - REST PA3
  - NSS PA4
  - SCK PA5
  - MISO PA6
  - MOSI PA7
  - BUSY PB1
- The receiver PCB does not connect SX1262 DIO1, so firmware polls radio IRQ
  status over SPI.

Editable EasyEDA projects and manufacturing exports belong under `hardware/`.
They have not all been exported into the repository yet. See
[hardware/README.md](hardware/README.md) for the required layout and release
rules.

## Firmware behavior

### Transmitter

The transmitter uses a universal application image. Two board-specific records
in the final two 2 KB flash pages store its node ID, report interval,
receive-window duration, configuration revision, last transaction ID,
generation, and checksum. An erased or invalid record produces silent Node 0;
it does not transmit until provisioned. Remote updates write and verify the
older page first, so an interrupted update restarts from the surviving record.

Normal behavior:

- Power-up starts an addressed LoRa link check. It double-blinks once per second
  for up to 30 seconds, stops on confirmation, and shows five final blinks if no
  receiver answers; scheduled transmissions still continue afterward.
- A scheduled report powers the SCD41, takes a fresh single-shot measurement,
  reads battery voltage, transmits, and opens a short downlink window.
- Pressing the debug button requests a fresh sensor conversion and immediate
  report, then flashes the node ID count.
- Automatic transmissions do not blink the LED.
- Accepted receiver configuration produces a short acknowledgement flurry.
- v0.6.0 includes its semantic firmware version in every sensor packet. Repeating
  three bytes makes version caching recover automatically after server restarts
  or missed packets.

The SCD41 power/conversion strategy is functional but not final. Stabilization,
calibration, temperature offset, and the idle-versus-power-cycle threshold are
Priority 1 work in the to-do list.

On 2026-08-13, a direct power-cycle comparison reported 1847 ppm / 25.42 C /
36.87 %RH for the first single shot, then 1864 ppm / 25.57 C / 36.88 %RH for a
second single shot begun immediately while the load switch remained on. The
17 ppm CO2 difference does not justify permanently discarding the first result
on this board yet; retain a configurable test option while more conditions are
sampled.

### Receiver

The receiver continuously listens for LoRa packets and emits one JSON object per
line over USB. It blinks its LED according to the received node ID. Receiver
v0.8.0 forwards a boot link check to the host, leaves a 350 ms configuration
turnaround, then answers it; this lets queued configuration reach a node during
the boot check as well as its normal receive window.

Receiver v0.6.0:

- includes `fw` in each five-second USB status record;
- answers the ASCII USB lines `INFO` and `VERSION` with a fresh `hello` record;
- still emits its identity at boot.

The recurring status/query mechanism means the host no longer has to open the
COM port quickly enough to catch the boot message.

## Identity, provisioning, and firmware updates

The STM32 factory 96-bit UID identifies physical hardware to the server. The UID
is not the radio node ID. The assigned node ID is stored in the transmitter's
reserved flash record and is what normal firmware uses on air.

Launch the graphical utility on Windows with:

```text
tools\run_provisioner.cmd
```

Or run it directly:

```text
.venv\Scripts\python.exe tools\provision_transmitter.py
```

Identity rules:

- Automatic + known UID: retain the registered node ID.
- Automatic + new UID: allocate the lowest available ID.
- Explicit available ID: intentionally assign or renumber to that ID.
- Explicit occupied/reserved ID: fail clearly; never silently choose another.
- A legacy node that predates UID registration may adopt its matching historical
  database ID only when the utility reads that same valid ID from its flash and
  no other UID/reservation owns it.

The utility reads the UID and existing configuration before flashing, writes the
selected ELF/HEX/BIN application, rewrites the reserved record, verifies it, and
then completes server registration. Keep existing parameters checked for normal
firmware updates. Detailed usage and failure behavior are in
[tools/PROVISIONING.md](tools/PROVISIONING.md).

Current transmitter build:

```text
firmware/transmitter/MycoLogger Node/Debug/MycoLogger Node.elf
```

Current receiver build:

```text
firmware/receiver/MycoReceiver/Debug/MycoReceiver.elf
```

## Radio and USB protocols

- Radio: 915 MHz LoRa, SF7, 125 kHz bandwidth, coding rate 4/5, private sync
  word `0x1424`, explicit header, payload CRC.
- USB: native CDC/ACM, conventionally opened as 115200 8N1; one UTF-8 JSON
  object per line.
- Sensor packets include node ID, sequence, uptime, sensor values/diagnostic,
  active configuration, battery voltage, and transmitter firmware version.
- Configuration is a targeted, revisioned, idempotent downlink of report
  interval and downlink receive-window duration, followed by a transmitter
  acknowledgement. Uplinks also report active configuration so the server can
  recover after a missed ACK or database replacement.

Authoritative layouts:

- [protocol/radio-link-v1.md](protocol/radio-link-v1.md)
- [protocol/usb-serial-v1.md](protocol/usb-serial-v1.md)
- [protocol/configuration-plan.md](protocol/configuration-plan.md)

## Server and dashboard

The server uses FastAPI, pyserial, SQLite, and a dependency-light HTML/CSS/JS
frontend. The receiver service automatically discovers the USB receiver,
reconnects after removal, validates NDJSON, decodes packets, stores measurements,
and dispatches pending node commands.

Implemented dashboard/data features include:

- live node readings, RSSI/SNR, battery field, firmware version, and config state;
- configurable report interval and downlink receive-window duration delivered
  through the receiver;
- current tubs with stable-scale, vertically separated temperature, humidity,
  and CO2 graph lanes, dotted window-average guides, and selectable 1 hour,
  1 day, 3 day, 7 day, and 1 month ranges; every valid stored measurement is
  plotted, with hover/touch value inspection;
- current spawn jars, preparation/inoculation/break-and-shake records, and
  culture/species-aware spawning into tubs;
- explicit tub and jar lifecycle actions, including contamination outcomes,
  sensor release, permanent deletion, and an Archive split into Past Grows,
  Failed Grows, Archived Jars, and Failed Jars;
- multiple queued photo uploads, EXIF capture times, and environmental conditions
  associated with the nearest grow measurement;
- node/tub assignment history so reusing a sensor does not relabel old data.

The top-right Account menu identifies the current local deployment as **MycoPi
Admin** and provides UI placeholders for future username/password login and
“Keep me logged in” support. Its **Save & shut down server** control is
functional: after confirmation it requests a graceful server stop, including
orderly receiver-service shutdown. It is intentionally unauthenticated only
while this is a local development deployment; production use must wait for the
planned account authorization work in `TO-DO.md`.

### Future multi-account ownership

The server, not radio traffic, will be the authority for node ownership.
Transmitters retain their immutable MCU UID and permanent radio node ID; neither
stores an account username. Future tables will include `accounts`, a current
`owner_account_id` on nodes, and an append-only node-ownership history. An
administrator provisions hardware, explicitly claims it for an account, and
uses an audited transfer action to move it later. Grows, jars, photos,
measurements, and configuration requests are scoped to that owner, preserving
the old account's historical data after a transfer. Authenticated per-node
configuration must be added before account ownership is treated as a security
boundary on radio.

Server-specific setup, environment variables, persistent-data paths, and Pi
layout are documented in [server/README.md](server/README.md).

### Windows development run

From the repository root:

```text
python -m venv .venv
.venv\Scripts\python.exe -m pip install -r server\requirements.txt
.venv\Scripts\python.exe server\run.py
```

Open `http://127.0.0.1:8080`. Only one process may own the receiver COM port, so
do not run `tools/read_receiver.py` while the server receiver service is active.

### Intended Pi deployment

- Checkout: `/home/pi/apps/mycologger`
- Virtual environment: `/home/pi/.venvs/mycologger`
- Persistent data: `/home/pi/mycologger-data/`
- Bind address: `0.0.0.0:8080`
- Access on LAN: `http://mycopi.local:8080`
- Remote private access: Pi Tailscale address on port 8080

Do not commit the Pi password, Tailscale credentials, databases, uploaded photos,
virtual environments, or other secrets/runtime data.

## Repository map

```text
firmware/   STM32CubeIDE transmitter and receiver projects
hardware/   EasyEDA sources, references, and manufacturing exports
protocol/   Radio, USB, and remote-configuration specifications
server/     FastAPI application, dashboard, SQLite layer, and tests
tools/      Receiver reader and ST-LINK flash/provisioning utility
pi/         Pi-specific deployment assets as they are added
docs/       Additional project documentation
TO-DO.md    Living prioritized roadmap and completed foundation
```

Build output under each STM32 project's `Debug/` directory is generally
generated. The provisioner defaults to the current transmitter Debug ELF so its
linked two-page configuration reservation matches the record format it writes;
select a release HEX only after it has been exported from that same build.

## Verification

Run the Python tests from the repository root on Windows:

```text
$env:PYTHONPATH="$PWD;$PWD\server"
.venv\Scripts\python.exe -m unittest discover -s server\tests -v
```

Both STM32 projects should also be rebuilt in CubeIDE after firmware changes.
The transmitter link must remain below `0x08007000`, which is the start of its
two reserved configuration pages.

## Development workflow

- Read this file and [TO-DO.md](TO-DO.md) before substantial work.
- Treat `TO-DO.md` as living state: add newly discovered work and mark verified
  completions after each coherent change.
- Update this README when current versions, deployment status, architecture,
  hardware mappings, or major decisions change.
- Make one local Git commit per coherent, verified change.
- Do not push every commit automatically; batch pushes until there is a useful
  milestone or an explicit reason to publish.
- Preserve SQLite databases and uploaded photos outside deployable Git checkouts.
- Never commit credentials or machine-specific secrets.

## Immediate priorities

The complete ordered list is in [TO-DO.md](TO-DO.md). The most important current
items are:

1. Flash and verify receiver v0.8.0 on the physical USB receiver.
2. Correct and validate the SCD41 measurement/power strategy.
3. Perform extended testing of the completed boot link/configuration exchange.
4. Reduce transmitter consumption with SX1262 sleep and STM32 Stop 2.
5. Finish the managed Pi deployment, backups, and health monitoring.
6. Export the complete editable EasyEDA hardware projects and revision-matched
   manufacturing files, then develop and test printable transmitter and
   receiver enclosures with editable CAD sources kept under `hardware/`.
