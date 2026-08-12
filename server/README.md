# MycoLogger server

The server application will run on the BTT Pi while remaining developable and
testable on Windows. Platform-specific values such as the receiver serial port,
database location, and HTTP bind address must come from configuration rather
than hard-coded paths.

## BTT Pi layout

- Repository checkout: `/home/pi/apps/mycologger`
- Python environment: `/home/pi/.venvs/mycologger`
- Persistent database: `/home/pi/mycologger-data/db`
- Runtime logs: `/home/pi/mycologger-data/logs`

The database stays outside the Git checkout so a pull, clean deployment, or
repository replacement cannot erase collected measurements.

## Development environments

Create a virtual environment independently on each computer and install the
same dependency file. Virtual environments must never be copied between
Windows and Linux or committed to Git.

```text
python -m pip install -r server/requirements.txt
```

On Linux, use the stable receiver path under `/dev/serial/by-id/` when
available. On Windows, automatic discovery or a configured `COM` port will be
used instead.

## Run the dashboard

From the repository root, install the dependencies into a local virtual
environment, then start the server:

```text
python -m venv .venv
.venv\Scripts\python -m pip install -r server\requirements.txt
.venv\Scripts\python server\run.py
```

Open `http://127.0.0.1:8080`. The JSON API is available under `/api`, with
interactive documentation at `/api/docs`.

Configuration is supplied through environment variables so the same code runs
on Windows and Linux:

- `MYCOLOGGER_HOST` defaults to `127.0.0.1`; the Pi service will use `0.0.0.0`.
- `MYCOLOGGER_PORT` defaults to `8080` to avoid the existing WOL service.
- `MYCOLOGGER_DATABASE_PATH` defaults to `server/data/mycologger.sqlite3`.
- `MYCOLOGGER_PHOTO_DIRECTORY` defaults to a `photos` directory beside the database.
- `MYCOLOGGER_RECEIVER_PORT` is normally left empty for automatic discovery.
- `MYCOLOGGER_RECEIVER_ENABLED` can disable serial ingestion for maintenance.

The server owns the receiver serial port. It discovers the MycoLogger USB CDC
device by product name or its current `0483:5740` VID/PID, reconnects after an
unplug, validates the NDJSON protocol, decodes radio packets, and stores SCD41
readings in SQLite. Do not run `tools/read_receiver.py` at the same time because
only one process can own a Windows COM port.

## Node configuration

Open the Nodes tab and use the cog on a node row. Display name, tub assignment,
location, notes, and active state are server metadata; the permanent radio node
ID never changes. Tub assignments have history, and each measurement keeps the
tub that was assigned when it arrived.

Changing the report interval creates a durable SQLite command. On that node's
next uplink, the server writes the command to the USB receiver, the receiver
transmits it during the node's 1.5-second listening window, and the node stores
the accepted value in its reserved flash page. The UI shows `queued`, `sent`,
`applied`, or `rejected`. A matching ACK applies the command immediately; the
node's next sensor packet also confirms the active revision in case the ACK was
missed.

## Current tubs and jars

Assigning a tub to a node creates its tile in Current Tubs. Tiles are arranged
three across on desktop and show the strain (falling back to the tub name),
permanent node ID, current readings, and hourly temperature, humidity, and CO2
averages from the last three days. Graphs expose one-hour, one-day, three-day,
seven-day, and one-month ranges. Each metric has a labeled, independently
scaled axis with a useful minimum span; axis bounds can expand for new extremes
but do not contract during routine refreshes. History begins with the current
node-to-tub assignment so reusing a transmitter does not mix two grows.

The grow detail dialog stores the tub name, species, strain, stage, spawn-to-bulk
date, completion date, multiple pin dates, and free-form notes. JPEG, PNG, and
WebP photos up to 10 MB can be staged in a browser-side queue and uploaded as a
batch. The server reads the embedded EXIF capture timestamp when present, with
an optional manual date/time override and a file/upload-time fallback. Each
photo permanently records the nearest measurement for that grow—including
temperature, humidity, CO2, battery voltage, node ID, reading time, and time
offset from the capture. Set `MYCOLOGGER_LOCAL_TIMEZONE` for EXIF timestamps
that do not include a UTC offset; the default is `America/New_York`.
At startup, legacy photo records without a timestamp are backfilled from their
preserved original files when readable metadata is present. Environmental
conditions are only attached when a grow measurement is within six hours of
the capture; distant readings are not presented as conditions at photo time.

Current Jars stores one record per physical jar. Bulk creation can add several
new jars with shared preparation details in one action, but it creates separate
records so culture, inoculation, break-and-shake dates, notes, and photos can
diverge later. Any selection of current jars can be spawned into the same tub.
Culture and species are stored on every jar. During spawning, jars are grouped
case-insensitively by culture: jars with the same culture may feed one tub, but
selecting APE and GT proposes and atomically creates separate inherited tubs.
The tub strain and species come directly from its source jars, and the API
rejects a group containing mismatched cultures or species. Each proposed tub
requires a spawn ratio, name, and STB date and can optionally receive an
available sensor node. The operation archives the selected jars and links every
full jar record and its photos into the tub's collapsed Spawn Jars section.
Inherited records are locked by default. Unlocking is an explicit,
warning-confirmed action, and locked records are also enforced by the API rather
than only by the page.

Tubs created from jars can be left without a sensor and assigned later from the
Nodes page. Their history and live readings remain empty until a node is
assigned; the spawn record is still available immediately.

Photo files live in `MYCOLOGGER_PHOTO_DIRECTORY`; SQLite stores their metadata
and environmental snapshot. On the Pi, keep this directory outside the Git
checkout alongside the persistent database so deployments cannot remove grow
records or photos.

## Transmitter provisioning

All transmitters use the same universal firmware. A blank configuration page
boots as silent Node 0 and produces four repeating LED flashes. The local
`tools/provision_transmitter.py` utility reads the STM32's immutable 96-bit UID
through ST-LINK, asks this server to reserve the lowest free node ID, flashes
and verifies the universal application and node configuration, then completes
the UID-to-node registration before resetting the MCU.

Provisioning reservations last five minutes. IDs already present in the nodes
table, permanently registered to another hardware UID, or actively reserved by
another provisioner are unavailable. The database uses unique active-reservation
indexes so simultaneous computers cannot receive the same ID. See
`tools/PROVISIONING.md` for graphical and command-line usage.

The provisioning API is currently intended for trusted LAN or Tailscale access.
Authentication should be added before exposing the server directly to the public
internet.
