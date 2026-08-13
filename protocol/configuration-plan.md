# MycoLogger configuration downlink plan

This file records the configuration architecture implemented by transmitter
firmware, receiver firmware, and the server.

## Runtime settings

Node firmware owns a single persistent configuration object. Current fields are:

- permanent node ID (not remotely changeable)
- sensor report interval in milliseconds
- downlink receive-window duration in milliseconds
- configuration revision and last applied transaction ID

The final two 2 KB transmitter flash pages are reserved for independently valid
copies of this object. Each has a magic, layout version, generation counter, and
checksum, which reject erased or incompatible data. When an actual new
configuration transaction is accepted, firmware writes and verifies the older
page first; the newest valid record always wins at boot. A power loss during an
erase or write therefore falls back to the other page.

Application firmware is universal and compiles with Node ID 0. An erased or
invalid configuration page therefore enters a silent provisioning state: the
node does not initialize normal sensing/radio reporting and gives four repeating
LED flashes. `tools/provision_transmitter.py` reads the STM32's factory 96-bit
UID through ST-LINK, reserves an unused ID from the server, writes this same
application image plus a board-specific configuration object, verifies it, and
registers the UID-to-node mapping before reset. A valid saved node ID is loaded
independently of the application's default, so normal firmware updates preserve
identity when the reserved pages are not erased.

## Implemented exchange

1. The server saves a targeted configuration transaction in SQLite.
2. The node sends its normal sensor uplink.
3. After TX-done, the node opens a short LoRa receive window using the configured
   duration. A value around 1.5 seconds is the current design default.
4. The server writes `CFG <node> <transaction> <revision> <interval_s>` over
   USB immediately after that node's uplink. The receiver switches from receive
   to transmit and sends the binary command inside the window.
5. The node validates the protocol version, target node ID, revision, command,
   and value ranges before applying anything. LoRa payload CRC protects the
   radio frame in transit.
6. The node writes the candidate configuration to flash and immediately sends a
   type-`0x81` acknowledgment. The server marks the transaction Applied only
   after receiving the matching node ID and transaction ID.

Transactions are idempotent, so a missed acknowledgment can safely cause the
same transaction to be sent after the next uplink. Every sensor uplink also
reports the node's applied revision and interval; this confirms a change if the
immediate acknowledgment was lost and lets a rebuilt Pi database learn the
device's existing state. Revisions reject stale commands, while transaction IDs
correlate acknowledgments without requiring the new server database to continue
an old numeric sequence. Authentication and stronger replay protection must be
added before radio configuration is treated as trusted in a hostile deployment.

## Server-owned metadata

Display name, location, notes, active state, and tub assignment never travel to
the node. Tub assignment uses timestamped history, and each measurement records
the tub active at receipt time so moving hardware cannot relabel older data.
