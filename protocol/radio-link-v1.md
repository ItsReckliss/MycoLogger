# MycoLogger radio packets v1

The sensor-test transmitter sends one packet per completed one-minute sensor
cycle. Multi-byte integers use network byte order (most-significant byte
first).

| Offset | Size | Field | Value/meaning |
|---:|---:|---|---|
| 0 | 4 | Magic | ASCII `MYCO` |
| 4 | 1 | Protocol version | `1` |
| 5 | 1 | Packet type | `1` = link test |
| 6 | 4 | Node ID | Prototype transmitter is `1` |
| 10 | 4 | TX sequence | Increments once per started transmission |
| 14 | 4 | TX uptime | Whole seconds since MCU boot |
| 18 | 1 | Flags | Bit 0 is set while the debug button is pressed |

Packet type `2` extends this common 19-byte header with an SCD41 measurement:

| Offset | Size | Field | Value/meaning |
|---:|---:|---|---|
| 18 | 1 | Flags | Bit 0 = button pressed; bit 1 = sensor fields valid |
| 19 | 2 | CO2 | Unsigned concentration in ppm |
| 21 | 2 | Temperature | Signed hundredths of a degree Celsius |
| 23 | 2 | Relative humidity | Unsigned hundredths of percent RH |
| 25 | 1 | Sensor diagnostic | `0` for success; nonzero SCD41 failure stage |

Sensor diagnostic values are:

| Value | Meaning |
|---:|---|
| 0 | Success |
| 1-11 | Command, read, CRC, or conversion failure stage |
| 12 | SCL held low |
| 13 | SDA held low |
| 14 | SCD41 address NACK |
| 15 | Command high-byte NACK |
| 16 | Command low-byte NACK |

The battery-oriented transmitter uses power-cycled SCD41 single-shot mode. For
the debug build, a cycle starts once per minute: PB7 enables the load switch,
firmware waits for sensor startup, requests one measurement, waits for the
five-second conversion, reads and CRC-checks it, disables PB7, then transmits
one type-2 packet. If sensor initialization, conversion, or CRC validation
fails, the packet is still sent with the sensor-valid flag clear.

A debounced transmitter button press requests the same complete fresh-sample
cycle immediately. It does not retransmit cached sensor data. If a conversion
is already underway, that in-progress fresh conversion satisfies the request.

The receiver's debug LED blinks the decoded node ID after each valid `MYCO`
packet: node 1 blinks once, node 3 blinks three times, and so on. Invalid or
unrecognized packets do not start an LED sequence.

The radio parameters match the receiver: 915 MHz LoRa, SF7, 125 kHz bandwidth,
coding rate 4/5, 12-symbol preamble, explicit header, payload CRC enabled,
standard IQ, and private sync word `0x1424`. The testing firmware uses 0 dBm
transmit power for close-range bench testing.

`tools/read_receiver.py` preserves the receiver's original `payload_hex` field
and adds a `decoded` object for packets matching this format.
