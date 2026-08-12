# MycoLogger transmitter flash utility

The flash utility runs on the computer physically connected to the transmitter's
ST-LINK. The MycoLogger web server may run on the Pi, another computer on the
LAN, or a Tailscale peer.

## Requirements

- Python 3.10 or newer (Tkinter is used by the graphical interface).
- STM32CubeProgrammer, including `STM32_Programmer_CLI`.
- ST-LINK wired to SWDIO, SWCLK, NRST, and common ground.
- The transmitter powered at 3.3 V.
- Network access to the MycoLogger server.

On Windows, double-click `tools/run_provisioner.cmd`. From a terminal on any
supported platform:

```text
python tools/provision_transmitter.py
```

The default server is `http://mycopi.local:8080`. Enter a Tailscale URL or IP
when flashing from another network. Leave Node ID set to `Automatic`. A known
MCU UID keeps its registered ID; a new UID receives the lowest free ID. Entering
an explicit free ID intentionally assigns or renumbers the transmitter. An ID
already present in radio history, registered to another UID, or held by an
active reservation produces an error and never falls back to another number.
For nodes created before UID registration existed, Automatic mode may adopt the
matching historical ID only after the utility has read that same valid ID from
the transmitter's reserved flash. An ID registered to another UID remains an
error.

The equivalent non-graphical command is:

```text
python tools/provision_transmitter.py --cli \
  --server http://mycopi.local:8080 \
  --report-interval 900
```

Use `--node-id 12` only when a particular unused ID is required. The server
checks IDs already seen over radio, permanently provisioned hardware UIDs, and
active five-minute reservations.

## Failure behavior

The STM32 hardware UID and current configuration are read before an ID is
reserved. On a normal update, the existing node ID and parameters are written
back and verified after the selected firmware image. Clear the keep-parameters
checkbox only when the entered report interval and downlink window should
replace the saved values; the node ID still follows the Automatic/explicit
rules above.

The application and 32-byte configuration record are then written and verified.
The server registration is completed before the MCU is reset into normal
operation. If an update or server finalization fails, the utility restores the
previous configuration for an existing node. A brand-new failed provisioning
is returned to silent Node 0. In both cases it cancels the reservation.

After a successful provision and reset, the transmitter gives a one-time
six-flash debug-LED flurry. The firmware clears the provisioner's confirmation
marker from the configuration page, so normal later boots do not repeat it.

An unprovisioned transmitter gives four repeating debug-LED flashes and does
not send radio packets. The utility always rewrites and verifies the reserved
configuration, so identity retention does not depend on the selected firmware
image or programmer erase behavior.
