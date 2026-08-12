# MycoLogger USB serial protocol v1

The MycoLogger receiver presents itself to its host as a USB CDC/ACM serial
device. Current versions of Windows 10/11, Linux (including Raspberry Pi OS),
and macOS provide native CDC/ACM drivers.

The baud-rate selection has no electrical effect because this is native USB,
but host software may open the port using 115200 baud, 8 data bits, no parity,
and 1 stop bit for conventional serial-library defaults. Hardware and software
flow control must be disabled.

## Framing and encoding

- UTF-8 text (all v1 field names and values are ASCII)
- One complete JSON object per line
- A line feed (`0x0A`) terminates each record
- Unknown record types and fields must be ignored for forward compatibility
- The `v` field is the protocol major version

Example startup and status stream:

```json
{"v":1,"type":"hello","device":"mycologger-receiver","fw":"0.2.1","transport":"usb-cdc-acm"}
{"v":1,"type":"radio","state":"rx","model":"sx1262","frequency_hz":915000000,"modulation":"lora","sf":7,"bandwidth_hz":125000,"coding_rate":"4/5","sync_word":"private","tcxo_v":"1.8"}
{"v":1,"type":"status","uptime_ms":5000,"radio_busy":false,"button_pressed":false,"radio_state":"rx"}
```

The recurring status record always includes `radio_state`, so a host which opens
the port after the one-time startup records can still confirm radio
initialization. If initialization failed, the status instead includes
`radio_state:"error"`, `radio_error_code`, and `radio_status_raw`.

The receiver configures its SX1262 for continuous reception. Because this board
does not connect the radio's DIO1 interrupt output, the firmware polls the SX1262
IRQ status through SPI every 10 ms.

The initial link-test radio settings are:

- Carrier: 915.0 MHz
- LoRa spreading factor: 7
- Bandwidth: 125 kHz
- Coding rate: 4/5
- Preamble: 12 symbols
- Explicit header and payload CRC enabled
- Private LoRa sync word (`0x1424`)
- Maximum payload: 64 bytes
- DIO3-controlled TCXO: 1.8 V with a 5 ms startup timeout

Received payloads remain binary-safe by being hex encoded. Signal metrics use
integer units so host software never has to account for locale-specific decimal
formatting:

```json
{"v":1,"type":"packet","seq":1,"length":4,"rssi_dbm_x2":-210,"snr_db_quarters":28,"payload_hex":"01020304"}
```

Divide `rssi_dbm_x2` by 2 for dBm and `snr_db_quarters` by 4 for dB. The `seq`
counter starts at 1 after each receiver reboot.

Radio initialization or reception failures use explicit records:

```json
{"v":1,"type":"radio","state":"error","code":1,"status_raw":0}
{"v":1,"type":"radio_error","error":"crc"}
```

Radio initialization codes are `1` for a BUSY timeout and `2` for an invalid
status byte (commonly caused by an open or shorted MISO connection). Runtime
errors can be `crc`, `header`, `timeout`, or `bus`.

Button changes are reported as events:

```json
{"v":1,"type":"button","pressed":true}
{"v":1,"type":"button","pressed":false}
```

## Host port names

- Windows: `COM3`, `COM4`, etc.
- Linux/Raspberry Pi OS: usually `/dev/ttyACM0`
- macOS: usually `/dev/cu.usbmodem*`

The supplied `tools/read_receiver.py` program waits for the device and
automatically reconnects after unplugging. Pass a fixed port if desired:

```text
python tools/read_receiver.py COM6
```

Or omit the port to discover the receiver by its USB product name/prototype
VID/PID. This is usually preferable for unattended Raspberry Pi operation:

```text
python3 tools/read_receiver.py
```

On Linux, the collection service's account must have permission to open the
device. This is commonly handled through the `dialout` group or a dedicated
udev rule.

## USB identity note

The prototype currently retains STMicroelectronics' Cube-generated example
VID/PID (`0483:5740`). That is convenient for development, but it is not a
product USB identity. Before distributing hardware, assign a VID/PID that the
project has permission to use and update the descriptors and any Linux udev
rules together.
