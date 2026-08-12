# MycoLogger transmitter provisioning

The provisioner runs on the computer physically connected to the transmitter's
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
when provisioning from another network. Leave Node ID set to `Automatic` for
the server to reserve the lowest free ID.

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

The STM32 hardware UID is read before an ID is reserved. The universal
application and the 32-byte configuration page are then written and verified.
The server registration is completed before the MCU is reset into normal
operation. If flashing or server finalization fails, the utility attempts to
erase the configuration page and return the transmitter to silent Node 0, then
cancels the reservation.

After a successful provision and reset, the transmitter gives a one-time
six-flash debug-LED flurry. The firmware clears the provisioner's confirmation
marker from the configuration page, so normal later boots do not repeat it.

An unprovisioned transmitter gives four repeating debug-LED flashes and does
not send radio packets. Normal application firmware updates preserve the final
2 KB configuration page as long as a full-chip erase is not requested.
