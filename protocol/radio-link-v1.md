# MycoLogger radio packets v1

The sensor-test transmitter sends one packet per completed one-minute sensor
cycle. Multi-byte integers use network byte order (most-significant byte
first).

| Offset | Size | Field | Value/meaning |
|---:|---:|---|---|
| 0 | 4 | Magic | ASCII `MYCO` |
| 4 | 1 | Protocol version | `1` |
| 5 | 1 | Packet type | `1` = link test |
| 6 | 4 | Provisioned Node ID | `1` through `4294967294`; Node `0` never transmits |
| 10 | 4 | TX sequence | Increments once per started transmission |
| 14 | 4 | TX uptime | Whole seconds since MCU boot |
| 18 | 1 | Flags | Bit 0 is set while the debug button is pressed |

Packet type `2` extends this common 19-byte header with an SCD41 measurement:

| Offset | Size | Field | Value/meaning |
|---:|---:|---|---|
| 18 | 1 | Flags | Bit 0 = button pressed; bit 1 = sensor fields valid; bit 2 = battery voltage valid; bit 3 = network confirmation requested |
| 19 | 2 | CO2 | Unsigned concentration in ppm |
| 21 | 2 | Temperature | Signed hundredths of a degree Celsius |
| 23 | 2 | Relative humidity | Unsigned hundredths of percent RH |
| 25 | 1 | Sensor diagnostic | `0` for success; nonzero SCD41 failure stage |
| 26 | 4 | Configuration revision | Last accepted configuration revision |
| 30 | 4 | Report interval | Current interval in whole seconds |
| 34 | 2 | Battery voltage | Battery input in millivolts; valid when flag bit 2 is set |
| 36 | 1 | Firmware major | Semantic firmware-version major component |
| 37 | 1 | Firmware minor | Semantic firmware-version minor component |
| 38 | 1 | Firmware patch | Semantic firmware-version patch component |
| 39 | 4 | Reset flags | Raw STM32 RCC reset-cause flags captured at this boot |
| 43 | 2 | Sensor failures | Saturating SCD41-operation failure count since this boot |
| 45 | 2 | Radio failures | Saturating SX1262 operation failure count since this boot |

Older 26-byte type-2 packets remain valid; the configuration fields are an
optional extension understood by server version 0.2 and later. The battery
field is a further optional extension, so older 34-byte firmware remains
compatible with the current server. Transmitter v0.6.0 adds the optional
three-byte version extension. It is repeated in every measurement so a server
restart, database restore, or missed first packet repairs the cached version
without a special radio exchange.

Transmitter v0.8.0 adds the optional eight-byte diagnostics extension. Its
counters restart at every MCU reset, so the server stores each reading's
snapshot and exposes the newest values on the node. They represent recoverable
operation failures; a failed sensor conversion or radio operation is reported
and retried, not treated as a reason to reset the device. The watchdog is for a
firmware hang that prevents a full state-machine pass from completing.

## Configuration downlink

Type `0x80` is a 22-byte server-to-node configuration transaction:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | `MYCO` magic |
| 4 | 1 | Protocol version `1` |
| 5 | 1 | Packet type `0x80` |
| 6 | 4 | Target permanent node ID |
| 10 | 4 | Transaction ID |
| 14 | 4 | Desired configuration revision |
| 18 | 4 | Report interval in seconds |

Type `0x81` is a 23-byte immediate node-to-server acknowledgment. It contains
the node ID, transaction ID, current revision, one-byte status, and current
report interval. Status `0` means applied; nonzero values indicate stale data,
an invalid interval, or a flash write failure.

Type `0x82` is a 14-byte receiver-to-node link acknowledgment. Bytes 6-9 hold
the addressed node ID and bytes 10-13 echo the request/check sequence of the
packet which requested confirmation. A node accepts it only when both fields
match its current request.

Type `3` is the matching 14-byte node-to-receiver boot link check. Bytes 6-9
contain the node ID and bytes 10-13 contain a short-lived check sequence. The
receiver answers it locally without forwarding it to the database service.

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
The press also starts a non-blocking node-identification pattern on the
transmitter debug LED: node 1 flashes once, node 3 flashes three times, and so
on. Holding the button does not hold the LED on. Periodic transmissions and
configuration acknowledgements do not illuminate the LED, preserving battery
power; boot, unprovisioned, sensor-fault, and radio-fault patterns remain
available for diagnostics.

When a new receiver configuration command is successfully persisted, the
transmitter gives one six-flash acknowledgement flurry. Invalid, stale, and
duplicate commands do not trigger it. The ST-LINK provisioner requests the same
one-time flurry after its final reset; firmware consumes and clears that marker
so later ordinary power cycles do not repeat the provisioning indication.

On every ordinary power-up, the transmitter immediately sends a type `3` link
check while the SCD41 measurement proceeds independently. Its LED remains solid
while it waits for the addressed `0x82` response. The receiver returns the ACK
after a short radio-turnaround delay. The node tries up to three times. Success
turns the LED off, normally within one short radio round trip rather than after
the five-second sensor conversion. If all attempts time out, two slower flashes
report the failure and the LED stays off to prevent an unavailable receiver
from draining the battery. Later normal sensor reports set the network
confirmation flag until contact is restored; eventual recovery produces one
short six-flash flurry.

The receiver's debug LED blinks the decoded node ID after each valid `MYCO`
packet: node 1 blinks once, node 3 blinks three times, and so on. Invalid or
unrecognized packets do not start an LED sequence.

The radio parameters match the receiver: 915 MHz LoRa, SF7, 125 kHz bandwidth,
coding rate 4/5, 12-symbol preamble, explicit header, payload CRC enabled,
standard IQ, and private sync word `0x1424`. The testing firmware uses 0 dBm
transmit power for close-range bench testing.

`tools/read_receiver.py` preserves the receiver's original `payload_hex` field
and adds a `decoded` object for packets matching this format.
