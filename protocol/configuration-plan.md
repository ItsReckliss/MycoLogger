# MycoLogger configuration downlink plan

This file records the intended configuration architecture; configuration
commands are not enabled in the current test firmware.

## Runtime settings

Node firmware owns a single runtime configuration object. Initial fields are:

- node ID
- sensor report interval in milliseconds
- downlink receive-window duration in milliseconds

Future commands must validate a complete candidate configuration before
changing the active object. Persistent storage can be added separately so a
node may either retain settings across power loss or intentionally return to
compiled defaults.

## Planned exchange

1. A Raspberry Pi queues a targeted configuration transaction through the
   receiver's USB CDC/ACM JSON interface.
2. The node sends its normal sensor uplink.
3. After TX-done, the node opens a short LoRa receive window using the configured
   duration. A value around 1.5 seconds is the current design default.
4. If the receiver has a pending transaction for that node ID, it switches from
   receive to transmit after the uplink and sends the command inside the window.
5. The node validates the protocol version, target node ID, transaction ID,
   command, value ranges, and integrity data before applying anything.
6. The node includes the transaction ID and resulting configuration revision in
   its next uplink as an acknowledgment. The receiver then removes the queued
   transaction.

The receive window should remain disabled until this protocol exists; opening
an empty window after every report would waste battery. Commands should be
idempotent so retransmitting a transaction after a missed acknowledgment is
safe. Authentication and replay protection must be added before configuration
over radio is treated as trusted in a deployed system.
