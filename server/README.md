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
