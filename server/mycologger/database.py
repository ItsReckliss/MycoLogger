"""SQLite storage for measurements, node metadata, and configuration jobs."""

from __future__ import annotations

import sqlite3
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import UTC, datetime, timedelta
from pathlib import Path
from typing import Any, Iterator


NODES_SCHEMA = """
CREATE TABLE IF NOT EXISTS nodes (
    node_id                     INTEGER PRIMARY KEY,
    name                        TEXT NOT NULL,
    first_seen_utc              TEXT NOT NULL,
    last_seen_utc               TEXT NOT NULL,
    last_sequence               INTEGER,
    last_uptime_s               INTEGER,
    boot_session                INTEGER NOT NULL DEFAULT 1,
    packet_count                INTEGER NOT NULL DEFAULT 0,
    last_rssi_dbm_x2            INTEGER,
    last_snr_db_quarters        INTEGER,
    sensor_valid                INTEGER NOT NULL DEFAULT 0,
    sensor_error                TEXT,
    location                    TEXT NOT NULL DEFAULT '',
    notes                       TEXT NOT NULL DEFAULT '',
    active                      INTEGER NOT NULL DEFAULT 1,
    desired_report_interval_s   INTEGER NOT NULL DEFAULT 60,
    applied_report_interval_s   INTEGER NOT NULL DEFAULT 60,
    desired_downlink_window_ms  INTEGER NOT NULL DEFAULT 1500,
    applied_downlink_window_ms  INTEGER NOT NULL DEFAULT 1500,
    config_revision             INTEGER NOT NULL DEFAULT 0,
    applied_config_revision     INTEGER NOT NULL DEFAULT 0,
    last_config_transaction_id  INTEGER,
    last_config_status          TEXT,
    firmware_version           TEXT,
    firmware_updated_utc       TEXT,
    last_reset_flags           INTEGER,
    sensor_failure_count       INTEGER,
    radio_failure_count        INTEGER
)
"""

TUBS_SCHEMA = """
CREATE TABLE IF NOT EXISTS tubs (
    tub_id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name            TEXT NOT NULL COLLATE NOCASE UNIQUE,
    species         TEXT NOT NULL DEFAULT '',
    strain          TEXT NOT NULL DEFAULT '',
    started_on      TEXT,
    spawn_to_bulk_on TEXT,
    completed_on    TEXT,
    stage           TEXT NOT NULL DEFAULT 'colonizing',
    notes           TEXT NOT NULL DEFAULT '',
    active          INTEGER NOT NULL DEFAULT 1,
    archive_category TEXT NOT NULL DEFAULT '',
    lifecycle_reason TEXT NOT NULL DEFAULT '',
    contaminated_on TEXT,
    archived_utc    TEXT,
    first_flush_harvested INTEGER NOT NULL DEFAULT 0,
    created_utc     TEXT NOT NULL,
    updated_utc     TEXT NOT NULL
)
"""

PIN_DATES_SCHEMA = """
CREATE TABLE IF NOT EXISTS grow_pin_dates (
    pin_date_id     INTEGER PRIMARY KEY AUTOINCREMENT,
    tub_id          INTEGER NOT NULL REFERENCES tubs(tub_id) ON DELETE CASCADE,
    pin_date        TEXT NOT NULL,
    created_utc     TEXT NOT NULL,
    UNIQUE(tub_id, pin_date)
)
"""

PHOTOS_SCHEMA = """
CREATE TABLE IF NOT EXISTS grow_photos (
    photo_id        INTEGER PRIMARY KEY AUTOINCREMENT,
    tub_id          INTEGER NOT NULL REFERENCES tubs(tub_id) ON DELETE CASCADE,
    stored_name     TEXT NOT NULL UNIQUE,
    original_name   TEXT NOT NULL,
    media_type      TEXT NOT NULL,
    size_bytes      INTEGER NOT NULL,
    caption         TEXT NOT NULL DEFAULT '',
    taken_on        TEXT,
    taken_at_utc    TEXT,
    capture_time_source TEXT NOT NULL DEFAULT 'unknown',
    condition_measurement_id INTEGER REFERENCES measurements(id),
    condition_node_id INTEGER,
    conditions_recorded_utc TEXT,
    condition_time_delta_s INTEGER,
    condition_co2_ppm INTEGER,
    condition_temperature_c REAL,
    condition_humidity_percent REAL,
    condition_battery_mv INTEGER,
    uploaded_utc    TEXT NOT NULL
)
"""

SPAWN_JARS_SCHEMA = """
CREATE TABLE IF NOT EXISTS spawn_jars (
    jar_id                    INTEGER PRIMARY KEY AUTOINCREMENT,
    name                      TEXT NOT NULL COLLATE NOCASE UNIQUE,
    grain_type                TEXT NOT NULL DEFAULT '',
    prep_tek                  TEXT NOT NULL DEFAULT '',
    pressure_cooker_minutes   INTEGER,
    pressure_psi              REAL,
    dry_grain_grams_per_jar   REAL,
    jar_count                 INTEGER NOT NULL DEFAULT 1,
    pressure_cooked_on        TEXT,
    inoculated_on             TEXT,
    culture                   TEXT NOT NULL DEFAULT '',
    species                   TEXT NOT NULL DEFAULT '',
    notes                     TEXT NOT NULL DEFAULT '',
    status                    TEXT NOT NULL DEFAULT 'active',
    archive_category          TEXT NOT NULL DEFAULT '',
    lifecycle_reason          TEXT NOT NULL DEFAULT '',
    contaminated_on           TEXT,
    archived_utc              TEXT,
    spawned_to_tub_id         INTEGER REFERENCES tubs(tub_id),
    spawned_utc               TEXT,
    locked                    INTEGER NOT NULL DEFAULT 0,
    created_utc               TEXT NOT NULL,
    updated_utc               TEXT NOT NULL
)
"""

SPAWN_JAR_BREAK_SHAKES_SCHEMA = """
CREATE TABLE IF NOT EXISTS spawn_jar_break_shakes (
    break_shake_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    jar_id          INTEGER NOT NULL REFERENCES spawn_jars(jar_id) ON DELETE CASCADE,
    break_shake_on  TEXT NOT NULL,
    created_utc     TEXT NOT NULL,
    UNIQUE(jar_id, break_shake_on)
)
"""

SPAWN_JAR_PHOTOS_SCHEMA = """
CREATE TABLE IF NOT EXISTS spawn_jar_photos (
    photo_id             INTEGER PRIMARY KEY AUTOINCREMENT,
    jar_id               INTEGER NOT NULL REFERENCES spawn_jars(jar_id) ON DELETE CASCADE,
    stored_name          TEXT NOT NULL UNIQUE,
    original_name        TEXT NOT NULL,
    media_type           TEXT NOT NULL,
    size_bytes           INTEGER NOT NULL,
    caption              TEXT NOT NULL DEFAULT '',
    taken_on             TEXT,
    taken_at_utc         TEXT,
    capture_time_source  TEXT NOT NULL DEFAULT 'unknown',
    uploaded_utc         TEXT NOT NULL
)
"""

PROVISIONED_TRANSMITTERS_SCHEMA = """
CREATE TABLE IF NOT EXISTS provisioned_transmitters (
    hardware_uid       TEXT PRIMARY KEY,
    node_id            INTEGER NOT NULL UNIQUE,
    provisioned_utc    TEXT NOT NULL,
    last_provisioned_utc TEXT NOT NULL
)
"""

PROVISIONING_RESERVATIONS_SCHEMA = """
CREATE TABLE IF NOT EXISTS provisioning_reservations (
    reservation_token  TEXT PRIMARY KEY,
    hardware_uid       TEXT NOT NULL,
    node_id            INTEGER NOT NULL,
    created_utc        TEXT NOT NULL,
    expires_utc        TEXT NOT NULL,
    status             TEXT NOT NULL DEFAULT 'active'
)
"""

ASSIGNMENTS_SCHEMA = """
CREATE TABLE IF NOT EXISTS node_tub_assignments (
    assignment_id   INTEGER PRIMARY KEY AUTOINCREMENT,
    node_id         INTEGER NOT NULL REFERENCES nodes(node_id),
    tub_id          INTEGER NOT NULL REFERENCES tubs(tub_id),
    assigned_utc    TEXT NOT NULL,
    unassigned_utc  TEXT
)
"""

COMMANDS_SCHEMA = """
CREATE TABLE IF NOT EXISTS node_commands (
    transaction_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    node_id             INTEGER NOT NULL REFERENCES nodes(node_id),
    command_type        TEXT NOT NULL,
    desired_revision    INTEGER NOT NULL,
    report_interval_s   INTEGER NOT NULL,
    downlink_window_ms  INTEGER NOT NULL DEFAULT 1500,
    status              TEXT NOT NULL,
    created_utc         TEXT NOT NULL,
    last_sent_utc       TEXT,
    acknowledged_utc    TEXT,
    attempts            INTEGER NOT NULL DEFAULT 0,
    error               TEXT
)
"""

MEASUREMENTS_SCHEMA = """
CREATE TABLE IF NOT EXISTS measurements (
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,
    node_id                 INTEGER NOT NULL REFERENCES nodes(node_id),
    tub_id                  INTEGER REFERENCES tubs(tub_id),
    boot_session            INTEGER NOT NULL,
    received_utc            TEXT NOT NULL,
    receiver_sequence       INTEGER,
    packet_type             TEXT NOT NULL,
    tx_sequence             INTEGER NOT NULL,
    tx_uptime_s             INTEGER NOT NULL,
    button_pressed          INTEGER NOT NULL DEFAULT 0,
    co2_ppm                 INTEGER,
    temperature_c           REAL,
    humidity_percent        REAL,
    battery_mv              INTEGER,
    rssi_dbm_x2             INTEGER,
    snr_db_quarters         INTEGER,
    sensor_valid            INTEGER NOT NULL DEFAULT 0,
    sensor_error            TEXT,
    reset_flags             INTEGER,
    sensor_failure_count   INTEGER,
    radio_failure_count    INTEGER,
    UNIQUE(node_id, boot_session, tx_sequence)
)
"""


@dataclass(frozen=True)
class StoreResult:
    inserted: bool
    boot_session: int


@contextmanager
def connect(database_path: Path) -> Iterator[sqlite3.Connection]:
    database_path.parent.mkdir(parents=True, exist_ok=True)
    connection = sqlite3.connect(database_path, timeout=10)
    try:
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        connection.execute("PRAGMA journal_mode = WAL")
        connection.execute("PRAGMA busy_timeout = 5000")
        yield connection
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()


def _utc_now() -> str:
    return datetime.now(UTC).isoformat()


def _column_names(connection: sqlite3.Connection, table: str) -> set[str]:
    return {
        str(row["name"])
        for row in connection.execute(f"PRAGMA table_info({table})").fetchall()
    }


def _migrate_nodes(connection: sqlite3.Connection) -> None:
    columns = _column_names(connection, "nodes")
    additions = {
        "boot_session": "INTEGER NOT NULL DEFAULT 1",
        "packet_count": "INTEGER NOT NULL DEFAULT 0",
        "location": "TEXT NOT NULL DEFAULT ''",
        "notes": "TEXT NOT NULL DEFAULT ''",
        "active": "INTEGER NOT NULL DEFAULT 1",
        "desired_report_interval_s": "INTEGER NOT NULL DEFAULT 60",
        "applied_report_interval_s": "INTEGER NOT NULL DEFAULT 60",
        "desired_downlink_window_ms": "INTEGER NOT NULL DEFAULT 1500",
        "applied_downlink_window_ms": "INTEGER NOT NULL DEFAULT 1500",
        "config_revision": "INTEGER NOT NULL DEFAULT 0",
        "applied_config_revision": "INTEGER NOT NULL DEFAULT 0",
        "last_config_transaction_id": "INTEGER",
        "last_config_status": "TEXT",
        "firmware_version": "TEXT",
        "firmware_updated_utc": "TEXT",
        "last_reset_flags": "INTEGER",
        "sensor_failure_count": "INTEGER",
        "radio_failure_count": "INTEGER",
    }
    for name, declaration in additions.items():
        if name not in columns:
            connection.execute(f"ALTER TABLE nodes ADD COLUMN {name} {declaration}")


def _migrate_commands(connection: sqlite3.Connection) -> None:
    if "downlink_window_ms" not in _column_names(connection, "node_commands"):
        connection.execute(
            "ALTER TABLE node_commands ADD COLUMN downlink_window_ms INTEGER NOT NULL DEFAULT 1500"
        )


def _migrate_measurements(connection: sqlite3.Connection) -> None:
    columns = _column_names(connection, "measurements")
    if columns and "boot_session" not in columns:
        connection.execute("ALTER TABLE measurements RENAME TO measurements_legacy")
        connection.execute(MEASUREMENTS_SCHEMA)
        connection.execute(
            """
            INSERT INTO measurements (
                node_id, boot_session, received_utc, packet_type,
                tx_sequence, tx_uptime_s, co2_ppm, temperature_c,
                humidity_percent, rssi_dbm_x2, snr_db_quarters,
                sensor_valid, sensor_error
            )
            SELECT
                node_id, 1, received_utc, 'sensor_reading',
                COALESCE(tx_sequence, id), COALESCE(tx_uptime_s, 0),
                co2_ppm, temperature_c, humidity_percent,
                rssi_dbm_x2, snr_db_quarters, sensor_valid, sensor_error
            FROM measurements_legacy
            """
        )
        connection.execute("DROP TABLE measurements_legacy")
        columns = _column_names(connection, "measurements")
    if columns and "tub_id" not in columns:
        connection.execute(
            "ALTER TABLE measurements ADD COLUMN tub_id INTEGER REFERENCES tubs(tub_id)"
        )
    if columns and "battery_mv" not in columns:
        connection.execute(
            "ALTER TABLE measurements ADD COLUMN battery_mv INTEGER"
        )
    additions = {
        "reset_flags": "INTEGER",
        "sensor_failure_count": "INTEGER",
        "radio_failure_count": "INTEGER",
    }
    for name, declaration in additions.items():
        if columns and name not in columns:
            connection.execute(
                f"ALTER TABLE measurements ADD COLUMN {name} {declaration}"
            )


def _migrate_tubs(connection: sqlite3.Connection) -> None:
    columns = _column_names(connection, "tubs")
    additions = {
        "spawn_to_bulk_on": "TEXT",
        "completed_on": "TEXT",
        "stage": "TEXT NOT NULL DEFAULT 'colonizing'",
        "spawn_ratio": "TEXT NOT NULL DEFAULT ''",
        "archive_category": "TEXT NOT NULL DEFAULT ''",
        "lifecycle_reason": "TEXT NOT NULL DEFAULT ''",
        "contaminated_on": "TEXT",
        "archived_utc": "TEXT",
        "first_flush_harvested": "INTEGER NOT NULL DEFAULT 0",
    }
    for name, declaration in additions.items():
        if name not in columns:
            connection.execute(f"ALTER TABLE tubs ADD COLUMN {name} {declaration}")


def _migrate_spawn_jars(connection: sqlite3.Connection) -> None:
    columns = _column_names(connection, "spawn_jars")
    additions = {
        "species": "TEXT NOT NULL DEFAULT ''",
        "archive_category": "TEXT NOT NULL DEFAULT ''",
        "lifecycle_reason": "TEXT NOT NULL DEFAULT ''",
        "contaminated_on": "TEXT",
        "archived_utc": "TEXT",
    }
    for name, declaration in additions.items():
        if name not in columns:
            connection.execute(
                f"ALTER TABLE spawn_jars ADD COLUMN {name} {declaration}"
            )


def _migrate_photos(connection: sqlite3.Connection) -> None:
    columns = _column_names(connection, "grow_photos")
    additions = {
        "taken_at_utc": "TEXT",
        "capture_time_source": "TEXT NOT NULL DEFAULT 'unknown'",
        "condition_measurement_id": "INTEGER REFERENCES measurements(id)",
        "condition_node_id": "INTEGER",
        "conditions_recorded_utc": "TEXT",
        "condition_time_delta_s": "INTEGER",
        "condition_co2_ppm": "INTEGER",
        "condition_temperature_c": "REAL",
        "condition_humidity_percent": "REAL",
        "condition_battery_mv": "INTEGER",
    }
    for name, declaration in additions.items():
        if name not in columns:
            connection.execute(
                f"ALTER TABLE grow_photos ADD COLUMN {name} {declaration}"
            )


def initialize(database_path: Path) -> None:
    with connect(database_path) as connection:
        connection.execute(NODES_SCHEMA)
        _migrate_nodes(connection)
        connection.execute(TUBS_SCHEMA)
        _migrate_tubs(connection)
        connection.execute(
            """
            UPDATE tubs SET archive_category = 'past_grow',
                archived_utc = COALESCE(archived_utc, updated_utc)
            WHERE active = 0 AND archive_category = ''
            """
        )
        connection.execute(ASSIGNMENTS_SCHEMA)
        connection.execute(PIN_DATES_SCHEMA)
        connection.execute(PHOTOS_SCHEMA)
        _migrate_photos(connection)
        connection.execute(SPAWN_JARS_SCHEMA)
        _migrate_spawn_jars(connection)
        connection.execute(
            """
            UPDATE spawn_jars SET archive_category = 'archived_jar',
                archived_utc = COALESCE(archived_utc, updated_utc)
            WHERE status = 'archived' AND archive_category = ''
            """
        )
        connection.execute(SPAWN_JAR_BREAK_SHAKES_SCHEMA)
        connection.execute(SPAWN_JAR_PHOTOS_SCHEMA)
        connection.execute(PROVISIONED_TRANSMITTERS_SCHEMA)
        connection.execute(PROVISIONING_RESERVATIONS_SCHEMA)
        connection.execute(COMMANDS_SCHEMA)
        _migrate_commands(connection)
        _migrate_measurements(connection)
        connection.execute(MEASUREMENTS_SCHEMA)
        connection.execute(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS idx_one_active_tub_per_node
                ON node_tub_assignments(node_id)
                WHERE unassigned_utc IS NULL
            """
        )
        connection.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_measurements_node_received
                ON measurements(node_id, received_utc DESC)
            """
        )
        connection.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_node_commands_pending
                ON node_commands(node_id, status, transaction_id DESC)
            """
        )
        connection.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_provisioning_reservations_active
                ON provisioning_reservations(status, expires_utc, node_id)
            """
        )
        connection.execute(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS idx_one_active_reservation_per_node
                ON provisioning_reservations(node_id)
                WHERE status = 'active'
            """
        )
        connection.execute(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS idx_one_active_reservation_per_uid
                ON provisioning_reservations(hardware_uid)
                WHERE status = 'active'
            """
        )
        connection.execute("PRAGMA user_version = 10")


def _active_tub_id(connection: sqlite3.Connection, node_id: int) -> int | None:
    row = connection.execute(
        """
        SELECT tub_id FROM node_tub_assignments
        WHERE node_id = ? AND unassigned_utc IS NULL
        """,
        (node_id,),
    ).fetchone()
    return int(row["tub_id"]) if row else None


def _reconcile_reported_config(
    connection: sqlite3.Connection,
    node_id: int,
    decoded: dict[str, Any],
    received_utc: str,
) -> None:
    """Use an uplink as confirmation and recover cleanly after DB replacement."""
    if "config_revision" not in decoded or "report_interval_s" not in decoded:
        return

    reported_revision = int(decoded["config_revision"])
    reported_interval = int(decoded["report_interval_s"])
    reported_window = int(decoded.get("downlink_window_ms") or 1500)
    node = connection.execute(
        """
        SELECT config_revision, applied_config_revision
        FROM nodes WHERE node_id = ?
        """,
        (node_id,),
    ).fetchone()
    if node is None or reported_revision < int(node["applied_config_revision"]):
        return

    connection.execute(
        """
        UPDATE nodes SET applied_report_interval_s = ?, applied_downlink_window_ms = ?,
            applied_config_revision = ? WHERE node_id = ?
        """,
        (reported_interval, reported_window, reported_revision, node_id),
    )

    pending = connection.execute(
        """
        SELECT transaction_id, desired_revision, report_interval_s, downlink_window_ms
        FROM node_commands
        WHERE node_id = ? AND status IN ('queued', 'sent')
        ORDER BY transaction_id DESC LIMIT 1
        """,
        (node_id,),
    ).fetchone()
    if pending is not None:
        if (
            reported_revision == int(pending["desired_revision"])
            and reported_interval == int(pending["report_interval_s"])
            and reported_window == int(pending["downlink_window_ms"])
        ):
            connection.execute(
                """
                UPDATE node_commands SET status = 'applied',
                    acknowledged_utc = ?, error = NULL
                WHERE transaction_id = ?
                """,
                (received_utc, int(pending["transaction_id"])),
            )
            connection.execute(
                """
                UPDATE nodes SET last_config_status = 'applied',
                    last_config_transaction_id = ? WHERE node_id = ?
                """,
                (int(pending["transaction_id"]), node_id),
            )
        return

    desired_revision = int(node["config_revision"])
    if reported_revision > desired_revision:
        connection.execute(
            """
            UPDATE nodes SET desired_report_interval_s = ?, desired_downlink_window_ms = ?,
                config_revision = ?, last_config_status = 'applied'
            WHERE node_id = ?
            """,
            (reported_interval, reported_window, reported_revision, node_id),
        )
    elif reported_revision == desired_revision:
        connection.execute(
            "UPDATE nodes SET last_config_status = 'applied' WHERE node_id = ?",
            (node_id,),
        )


def store_packet(
    database_path: Path,
    decoded: dict[str, Any],
    radio_record: dict[str, Any],
    received_utc: str,
) -> StoreResult:
    """Upsert one node and store one sensor packet without duplicate rows."""
    node_id = int(decoded["node_id"])
    tx_sequence = int(decoded["tx_sequence"])
    tx_uptime_s = int(decoded["tx_uptime_s"])
    packet_type = str(decoded["packet_type"])
    sensor_packet = packet_type == "sensor_reading"

    with connect(database_path) as connection:
        previous = connection.execute(
            """
            SELECT boot_session, last_sequence, last_uptime_s,
                   sensor_valid, sensor_error, last_reset_flags,
                   sensor_failure_count, radio_failure_count
            FROM nodes WHERE node_id = ?
            """,
            (node_id,),
        ).fetchone()

        if previous is None:
            boot_session = 1
            sensor_valid = bool(decoded.get("sensor_valid", False))
            sensor_error = decoded.get("sensor_error")
            connection.execute(
                """
                INSERT INTO nodes (
                    node_id, name, first_seen_utc, last_seen_utc,
                    last_sequence, last_uptime_s, boot_session, packet_count,
                    last_rssi_dbm_x2, last_snr_db_quarters,
                    sensor_valid, sensor_error, firmware_version,
                    firmware_updated_utc, last_reset_flags,
                    sensor_failure_count, radio_failure_count
                ) VALUES (?, ?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    node_id,
                    f"Node {node_id}",
                    received_utc,
                    received_utc,
                    tx_sequence,
                    tx_uptime_s,
                    boot_session,
                    radio_record.get("rssi_dbm_x2"),
                    radio_record.get("snr_db_quarters"),
                    int(sensor_valid),
                    sensor_error,
                    decoded.get("firmware_version"),
                    received_utc if decoded.get("firmware_version") else None,
                    decoded.get("reset_flags"),
                    decoded.get("sensor_failure_count"),
                    decoded.get("radio_failure_count"),
                ),
            )
        else:
            boot_session = int(previous["boot_session"])
            previous_uptime = previous["last_uptime_s"]
            previous_sequence = previous["last_sequence"]
            if (
                previous_uptime is not None
                and tx_uptime_s < int(previous_uptime)
                and previous_sequence is not None
                and tx_sequence < int(previous_sequence)
            ):
                boot_session += 1

            sensor_valid = (
                bool(decoded.get("sensor_valid", False))
                if sensor_packet
                else bool(previous["sensor_valid"])
            )
            sensor_error = (
                decoded.get("sensor_error")
                if sensor_packet
                else previous["sensor_error"]
            )
            connection.execute(
                """
                UPDATE nodes SET
                    last_seen_utc = ?, last_sequence = ?, last_uptime_s = ?,
                    boot_session = ?, packet_count = packet_count + 1,
                    last_rssi_dbm_x2 = ?, last_snr_db_quarters = ?,
                    sensor_valid = ?, sensor_error = ?,
                    firmware_version = COALESCE(?, firmware_version),
                    firmware_updated_utc = CASE WHEN ? IS NOT NULL
                        THEN ? ELSE firmware_updated_utc END,
                    last_reset_flags = COALESCE(?, last_reset_flags),
                    sensor_failure_count = COALESCE(?, sensor_failure_count),
                    radio_failure_count = COALESCE(?, radio_failure_count)
                WHERE node_id = ?
                """,
                (
                    received_utc,
                    tx_sequence,
                    tx_uptime_s,
                    boot_session,
                    radio_record.get("rssi_dbm_x2"),
                    radio_record.get("snr_db_quarters"),
                    int(sensor_valid),
                    sensor_error,
                    decoded.get("firmware_version"),
                    decoded.get("firmware_version"),
                    received_utc,
                    decoded.get("reset_flags"),
                    decoded.get("sensor_failure_count"),
                    decoded.get("radio_failure_count"),
                    node_id,
                ),
            )

        _reconcile_reported_config(connection, node_id, decoded, received_utc)

        inserted = False
        if sensor_packet:
            cursor = connection.execute(
                """
                INSERT OR IGNORE INTO measurements (
                    node_id, tub_id, boot_session, received_utc,
                    receiver_sequence, packet_type, tx_sequence, tx_uptime_s,
                    button_pressed, co2_ppm, temperature_c, humidity_percent,
                    battery_mv, rssi_dbm_x2, snr_db_quarters,
                    sensor_valid, sensor_error, reset_flags,
                    sensor_failure_count, radio_failure_count
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                (
                    node_id,
                    _active_tub_id(connection, node_id),
                    boot_session,
                    received_utc,
                    radio_record.get("seq"),
                    packet_type,
                    tx_sequence,
                    tx_uptime_s,
                    int(bool(decoded.get("button_pressed", False))),
                    decoded.get("co2_ppm"),
                    decoded.get("temperature_c"),
                    decoded.get("humidity_percent"),
                    decoded.get("battery_mv"),
                    radio_record.get("rssi_dbm_x2"),
                    radio_record.get("snr_db_quarters"),
                    int(bool(decoded.get("sensor_valid", False))),
                    decoded.get("sensor_error"),
                    decoded.get("reset_flags"),
                    decoded.get("sensor_failure_count"),
                    decoded.get("radio_failure_count"),
                ),
            )
            inserted = cursor.rowcount == 1

    return StoreResult(inserted=inserted, boot_session=boot_session)


NODE_QUERY = """
    SELECT
        n.node_id, n.name, n.first_seen_utc, n.last_seen_utc,
        n.last_sequence, n.last_uptime_s, n.boot_session, n.packet_count,
        n.last_rssi_dbm_x2, n.last_snr_db_quarters,
        n.sensor_valid, n.sensor_error, n.location, n.notes, n.active,
        n.desired_report_interval_s, n.applied_report_interval_s,
        n.desired_downlink_window_ms, n.applied_downlink_window_ms,
        n.config_revision, n.applied_config_revision,
        n.last_config_transaction_id, n.last_config_status,
        n.firmware_version, n.firmware_updated_utc,
        n.last_reset_flags, n.sensor_failure_count, n.radio_failure_count,
        m.co2_ppm, m.temperature_c, m.humidity_percent, m.battery_mv,
        t.tub_id, t.name AS tub_name,
        c.transaction_id AS pending_transaction_id,
        c.status AS command_status, c.attempts AS command_attempts
    FROM nodes AS n
    LEFT JOIN measurements AS m ON m.id = (
        SELECT id FROM measurements
        WHERE node_id = n.node_id
        ORDER BY received_utc DESC, id DESC LIMIT 1
    )
    LEFT JOIN node_tub_assignments AS a
        ON a.node_id = n.node_id AND a.unassigned_utc IS NULL
    LEFT JOIN tubs AS t ON t.tub_id = a.tub_id
    LEFT JOIN node_commands AS c ON c.transaction_id = (
        SELECT transaction_id FROM node_commands
        WHERE node_id = n.node_id AND status IN ('queued', 'sent')
        ORDER BY transaction_id DESC LIMIT 1
    )
"""


def _format_node(row: sqlite3.Row) -> dict[str, Any]:
    node = dict(row)
    node["sensor_valid"] = bool(node["sensor_valid"])
    node["active"] = bool(node["active"])
    node["rssi_dbm"] = (
        node["last_rssi_dbm_x2"] / 2
        if node["last_rssi_dbm_x2"] is not None
        else None
    )
    node["snr_db"] = (
        node["last_snr_db_quarters"] / 4
        if node["last_snr_db_quarters"] is not None
        else None
    )
    node["battery_voltage_v"] = (
        node["battery_mv"] / 1000.0
        if node["battery_mv"] is not None
        else None
    )
    if node["command_status"] is None:
        node["command_status"] = node["last_config_status"] or "applied"
    return node


def get_nodes(database_path: Path) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        rows = connection.execute(NODE_QUERY + " ORDER BY n.node_id").fetchall()
    return [_format_node(row) for row in rows]


def get_node(database_path: Path, node_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            NODE_QUERY + " WHERE n.node_id = ?", (node_id,)
        ).fetchone()
    return _format_node(row) if row else None


def update_node_settings(
    database_path: Path,
    node_id: int,
    *,
    name: str,
    tub_name: str,
    location: str,
    notes: str,
    active: bool,
    report_interval_s: int,
    downlink_window_ms: int = 1500,
) -> dict[str, Any]:
    """Apply server metadata and queue a device update when needed."""
    now = _utc_now()
    with connect(database_path) as connection:
        node = connection.execute(
            "SELECT desired_report_interval_s, desired_downlink_window_ms, config_revision FROM nodes WHERE node_id = ?",
            (node_id,),
        ).fetchone()
        if node is None:
            raise KeyError(node_id)

        connection.execute(
            """
            UPDATE nodes SET name = ?, location = ?, notes = ?, active = ?
            WHERE node_id = ?
            """,
            (name.strip(), location.strip(), notes.strip(), int(active), node_id),
        )

        requested_tub = tub_name.strip()
        current_tub_id = _active_tub_id(connection, node_id)
        requested_tub_id: int | None = None
        if requested_tub:
            tub = connection.execute(
                "SELECT tub_id FROM tubs WHERE name = ? COLLATE NOCASE",
                (requested_tub,),
            ).fetchone()
            if tub:
                requested_tub_id = int(tub["tub_id"])
            else:
                cursor = connection.execute(
                    """
                    INSERT INTO tubs (name, created_utc, updated_utc)
                    VALUES (?, ?, ?)
                    """,
                    (requested_tub, now, now),
                )
                requested_tub_id = int(cursor.lastrowid)

        if current_tub_id != requested_tub_id:
            connection.execute(
                """
                UPDATE node_tub_assignments SET unassigned_utc = ?
                WHERE node_id = ? AND unassigned_utc IS NULL
                """,
                (now, node_id),
            )
            if requested_tub_id is not None:
                connection.execute(
                    """
                    INSERT INTO node_tub_assignments (
                        node_id, tub_id, assigned_utc
                    ) VALUES (?, ?, ?)
                    """,
                    (node_id, requested_tub_id, now),
                )

        if (report_interval_s != int(node["desired_report_interval_s"]) or
                downlink_window_ms != int(node["desired_downlink_window_ms"])):
            revision = int(node["config_revision"]) + 1
            connection.execute(
                """
                UPDATE node_commands SET status = 'superseded'
                WHERE node_id = ? AND status IN ('queued', 'sent')
                """,
                (node_id,),
            )
            cursor = connection.execute(
                """
                INSERT INTO node_commands (
                    node_id, command_type, desired_revision,
                    report_interval_s, downlink_window_ms, status, created_utc
                ) VALUES (?, 'set_config', ?, ?, ?, 'queued', ?)
                """,
                (node_id, revision, report_interval_s, downlink_window_ms, now),
            )
            transaction_id = int(cursor.lastrowid)
            connection.execute(
                """
                UPDATE nodes SET desired_report_interval_s = ?, desired_downlink_window_ms = ?,
                    config_revision = ?, last_config_transaction_id = ?,
                    last_config_status = 'queued'
                WHERE node_id = ?
                """,
                (report_interval_s, downlink_window_ms, revision, transaction_id, node_id),
            )

    updated = get_node(database_path, node_id)
    assert updated is not None
    return updated


def get_tubs(database_path: Path) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        rows = connection.execute(
            """
            SELECT t.*, COUNT(a.assignment_id) AS assigned_nodes
            FROM tubs AS t
            LEFT JOIN node_tub_assignments AS a
                ON a.tub_id = t.tub_id AND a.unassigned_utc IS NULL
            WHERE t.active = 1 AND t.archive_category = ''
            GROUP BY t.tub_id ORDER BY t.name COLLATE NOCASE
            """
        ).fetchall()
    tubs = [dict(row) for row in rows]
    for tub in tubs:
        tub["active"] = bool(tub["active"])
    return tubs


def get_pending_command(database_path: Path, node_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            """
            SELECT * FROM node_commands
            WHERE node_id = ? AND status IN ('queued', 'sent')
            ORDER BY transaction_id DESC LIMIT 1
            """,
            (node_id,),
        ).fetchone()
    return dict(row) if row else None


def mark_command_sent(database_path: Path, transaction_id: int) -> None:
    now = _utc_now()
    with connect(database_path) as connection:
        connection.execute(
            """
            UPDATE node_commands SET status = 'sent', last_sent_utc = ?,
                attempts = attempts + 1 WHERE transaction_id = ?
            """,
            (now, transaction_id),
        )
        connection.execute(
            """
            UPDATE nodes SET last_config_status = 'sent'
            WHERE node_id = (
                SELECT node_id FROM node_commands WHERE transaction_id = ?
            )
            """,
            (transaction_id,),
        )


def acknowledge_command(
    database_path: Path,
    *,
    node_id: int,
    transaction_id: int,
    config_revision: int,
    report_interval_s: int,
    downlink_window_ms: int,
    status: int,
) -> bool:
    """Mark an acknowledged transaction applied or rejected."""
    now = _utc_now()
    command_status = "applied" if status == 0 else "rejected"
    with connect(database_path) as connection:
        command = connection.execute(
            """
            SELECT transaction_id FROM node_commands
            WHERE transaction_id = ? AND node_id = ?
            """,
            (transaction_id, node_id),
        ).fetchone()
        if command is None:
            return False
        connection.execute(
            """
            UPDATE node_commands SET status = ?, acknowledged_utc = ?, error = ?
            WHERE transaction_id = ?
            """,
            (
                command_status,
                now,
                None if status == 0 else f"node_status_{status}",
                transaction_id,
            ),
        )
        if status == 0:
            connection.execute(
                """
                UPDATE nodes SET applied_report_interval_s = ?, applied_downlink_window_ms = ?,
                    applied_config_revision = ?, last_config_transaction_id = ?,
                    last_config_status = 'applied'
                WHERE node_id = ?
                """,
                (report_interval_s, downlink_window_ms, config_revision, transaction_id, node_id),
            )
        else:
            connection.execute(
                """
                UPDATE nodes SET last_config_transaction_id = ?,
                    last_config_status = 'rejected' WHERE node_id = ?
                """,
                (transaction_id, node_id),
            )
    return True


GROW_QUERY = """
    SELECT
        t.tub_id, t.name, t.species, t.strain, t.started_on,
        t.spawn_to_bulk_on, t.completed_on, t.stage, t.spawn_ratio,
        t.notes, t.active, t.archive_category, t.lifecycle_reason,
        t.contaminated_on, t.archived_utc, t.first_flush_harvested,
        t.created_utc, t.updated_utc,
        a.assignment_id, a.assigned_utc,
        n.node_id, n.name AS node_name, n.location, n.last_seen_utc,
        n.sensor_valid, n.sensor_error,
        m.received_utc AS reading_utc, m.co2_ppm, m.temperature_c,
        m.humidity_percent, m.battery_mv, m.rssi_dbm_x2, m.snr_db_quarters,
        (SELECT COUNT(*) FROM grow_photos p WHERE p.tub_id = t.tub_id)
            AS photo_count
    FROM tubs AS t
    LEFT JOIN node_tub_assignments AS a ON a.assignment_id = (
        SELECT assignment_id FROM node_tub_assignments
        WHERE tub_id = t.tub_id ORDER BY assigned_utc DESC, assignment_id DESC
        LIMIT 1
    )
    LEFT JOIN nodes AS n ON n.node_id = a.node_id
    LEFT JOIN measurements AS m ON m.id = (
        SELECT id FROM measurements
        WHERE tub_id = t.tub_id
        ORDER BY received_utc DESC, id DESC LIMIT 1
    )
"""


def _grow_history(
    connection: sqlite3.Connection,
    *,
    node_id: int,
    tub_id: int,
    assigned_utc: str,
    hours: int,
) -> list[dict[str, Any]]:
    lower_bound = datetime.now(UTC) - timedelta(hours=hours)
    try:
        assigned = datetime.fromisoformat(assigned_utc)
        if assigned.tzinfo is None:
            assigned = assigned.replace(tzinfo=UTC)
        lower_bound = max(lower_bound, assigned)
    except ValueError:
        pass

    rows = connection.execute(
        """
        SELECT
            received_utc AS recorded_utc,
            co2_ppm,
            temperature_c,
            humidity_percent
        FROM measurements
        WHERE node_id = ? AND tub_id = ? AND received_utc >= ? AND sensor_valid = 1
        ORDER BY received_utc, id
        """,
        (node_id, tub_id, lower_bound.isoformat()),
    ).fetchall()
    return [dict(row) for row in rows if row["recorded_utc"] is not None]


def _format_grow(
    connection: sqlite3.Connection,
    row: sqlite3.Row,
    *,
    hours: int,
    include_details: bool,
) -> dict[str, Any]:
    grow = dict(row)
    grow["active"] = bool(grow["active"])
    grow["first_flush_harvested"] = bool(grow["first_flush_harvested"])
    grow["sensor_valid"] = bool(grow["sensor_valid"])
    grow["title"] = grow["strain"] or grow["name"]
    grow["rssi_dbm"] = (
        grow["rssi_dbm_x2"] / 2 if grow["rssi_dbm_x2"] is not None else None
    )
    grow["snr_db"] = (
        grow["snr_db_quarters"] / 4
        if grow["snr_db_quarters"] is not None
        else None
    )
    grow["battery_voltage_v"] = (
        grow["battery_mv"] / 1000.0
        if grow["battery_mv"] is not None
        else None
    )
    grow["history"] = (
        _grow_history(
            connection,
            node_id=int(grow["node_id"]),
            tub_id=int(grow["tub_id"]),
            assigned_utc=str(grow["assigned_utc"]),
            hours=hours,
        )
        if grow["node_id"] is not None and grow["assigned_utc"] is not None
        else []
    )
    grow["pin_dates"] = []
    grow["photos"] = []
    if include_details:
        grow["pin_dates"] = [
            str(item["pin_date"])
            for item in connection.execute(
                """
                SELECT pin_date FROM grow_pin_dates
                WHERE tub_id = ? ORDER BY pin_date
                """,
                (grow["tub_id"],),
            ).fetchall()
        ]
        grow["photos"] = [
            _format_photo(item)
            for item in connection.execute(
                """
                SELECT photo_id, original_name, media_type, size_bytes,
                       caption, taken_on, taken_at_utc, capture_time_source,
                       condition_measurement_id, condition_node_id,
                       conditions_recorded_utc, condition_time_delta_s,
                       condition_co2_ppm, condition_temperature_c,
                       condition_humidity_percent, condition_battery_mv,
                       uploaded_utc
                FROM grow_photos WHERE tub_id = ?
                ORDER BY COALESCE(taken_on, uploaded_utc) DESC, photo_id DESC
                """,
                (grow["tub_id"],),
            ).fetchall()
        ]
        jar_rows = connection.execute(
            """
            SELECT jar_id FROM spawn_jars
            WHERE spawned_to_tub_id = ? ORDER BY jar_id
            """,
            (grow["tub_id"],),
        ).fetchall()
        grow["spawn_jars"] = [
            jar
            for item in jar_rows
            if (jar := _get_spawn_jar_with_connection(
                connection, int(item["jar_id"])
            )) is not None
        ]
        # Kept for API compatibility with v0.6.0 clients.
        grow["spawn_jar"] = grow["spawn_jars"][0] if grow["spawn_jars"] else None
    return grow


def get_current_grows(database_path: Path, *, hours: int = 72) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        rows = connection.execute(
            GROW_QUERY + " WHERE t.active = 1 AND t.archive_category = '' "
            "ORDER BY t.updated_utc DESC, t.tub_id"
        ).fetchall()
        return [
            _format_grow(connection, row, hours=hours, include_details=False)
            for row in rows
        ]


def get_archived_grows(
    database_path: Path, *, category: str, hours: int = 72
) -> list[dict[str, Any]]:
    if category not in {"past_grow", "failed_grow"}:
        raise ValueError("Invalid grow archive category")
    with connect(database_path) as connection:
        rows = connection.execute(
            GROW_QUERY + " WHERE t.archive_category = ? "
            "ORDER BY t.archived_utc DESC, t.tub_id DESC",
            (category,),
        ).fetchall()
        return [
            _format_grow(connection, row, hours=hours, include_details=False)
            for row in rows
        ]


def get_grow(
    database_path: Path, tub_id: int, *, hours: int = 72
) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            GROW_QUERY + " WHERE t.tub_id = ?", (tub_id,)
        ).fetchone()
        if row is None:
            return None
        return _format_grow(connection, row, hours=hours, include_details=True)


def update_grow(
    database_path: Path,
    tub_id: int,
    *,
    name: str,
    species: str,
    strain: str,
    stage: str,
    spawn_to_bulk_on: str | None,
    completed_on: str | None,
    pin_dates: list[str],
    notes: str,
    active: bool,
    spawn_ratio: str = "",
) -> dict[str, Any]:
    now = _utc_now()
    with connect(database_path) as connection:
        exists = connection.execute(
            "SELECT tub_id FROM tubs WHERE tub_id = ?", (tub_id,)
        ).fetchone()
        if exists is None:
            raise KeyError(tub_id)
        connection.execute(
            """
            UPDATE tubs SET name = ?, species = ?, strain = ?, stage = ?,
                spawn_to_bulk_on = ?, completed_on = ?, spawn_ratio = ?,
                notes = ?, active = ?, updated_utc = ? WHERE tub_id = ?
            """,
            (
                name.strip(),
                species.strip(),
                strain.strip(),
                stage,
                spawn_to_bulk_on,
                completed_on,
                spawn_ratio.strip(),
                notes.strip(),
                int(active),
                now,
                tub_id,
            ),
        )
        connection.execute("DELETE FROM grow_pin_dates WHERE tub_id = ?", (tub_id,))
        for pin_date in sorted(set(pin_dates)):
            connection.execute(
                """
                INSERT INTO grow_pin_dates (tub_id, pin_date, created_utc)
                VALUES (?, ?, ?)
                """,
                (tub_id, pin_date, now),
            )
    updated = get_grow(database_path, tub_id)
    if updated is None:
        raise KeyError(tub_id)
    return updated


def archive_grow(
    database_path: Path,
    tub_id: int,
    *,
    contaminated: bool,
    first_flush_harvested: bool,
    occurred_on: str,
    reason: str,
) -> dict[str, Any]:
    """Finish or fail a tub while preserving its complete historical record."""
    now = _utc_now()
    category = (
        "failed_grow"
        if contaminated and not first_flush_harvested
        else "past_grow"
    )
    with connect(database_path) as connection:
        existing = connection.execute(
            "SELECT tub_id, archive_category FROM tubs WHERE tub_id = ?", (tub_id,)
        ).fetchone()
        if existing is None:
            raise KeyError(tub_id)
        if str(existing["archive_category"]):
            raise ValueError("Grow is already archived")
        connection.execute(
            """
            UPDATE tubs SET active = 0, stage = 'complete',
                completed_on = COALESCE(completed_on, ?), archive_category = ?,
                lifecycle_reason = ?, contaminated_on = ?, archived_utc = ?,
                first_flush_harvested = ?, updated_utc = ?
            WHERE tub_id = ?
            """,
            (
                occurred_on,
                category,
                reason.strip(),
                occurred_on if contaminated else None,
                now,
                int(first_flush_harvested),
                now,
                tub_id,
            ),
        )
        connection.execute(
            """
            UPDATE node_tub_assignments SET unassigned_utc = ?
            WHERE tub_id = ? AND unassigned_utc IS NULL
            """,
            (now, tub_id),
        )
    result = get_grow(database_path, tub_id)
    assert result is not None
    return result


def delete_grow(database_path: Path, tub_id: int) -> list[str]:
    """Permanently delete a tub while retaining measurements and source jars."""
    now = _utc_now()
    with connect(database_path) as connection:
        exists = connection.execute(
            "SELECT tub_id FROM tubs WHERE tub_id = ?", (tub_id,)
        ).fetchone()
        if exists is None:
            raise KeyError(tub_id)
        photo_names = [
            str(row["stored_name"])
            for row in connection.execute(
                "SELECT stored_name FROM grow_photos WHERE tub_id = ?", (tub_id,)
            ).fetchall()
        ]
        connection.execute(
            "UPDATE measurements SET tub_id = NULL WHERE tub_id = ?", (tub_id,)
        )
        connection.execute(
            "DELETE FROM node_tub_assignments WHERE tub_id = ?", (tub_id,)
        )
        connection.execute(
            """
            UPDATE spawn_jars SET spawned_to_tub_id = NULL,
                status = 'archived', archive_category = 'archived_jar',
                lifecycle_reason = CASE WHEN lifecycle_reason = ''
                    THEN 'Source tub was deleted' ELSE lifecycle_reason END,
                archived_utc = COALESCE(archived_utc, ?), updated_utc = ?
            WHERE spawned_to_tub_id = ?
            """,
            (now, now, tub_id),
        )
        connection.execute("DELETE FROM tubs WHERE tub_id = ?", (tub_id,))
    return photo_names


def add_grow_photo(
    database_path: Path,
    tub_id: int,
    *,
    stored_name: str,
    original_name: str,
    media_type: str,
    size_bytes: int,
    caption: str = "",
    taken_on: str | None = None,
    taken_at_utc: str | None = None,
    capture_time_source: str = "unknown",
) -> dict[str, Any]:
    with connect(database_path) as connection:
        exists = connection.execute(
            "SELECT tub_id FROM tubs WHERE tub_id = ?", (tub_id,)
        ).fetchone()
        if exists is None:
            raise KeyError(tub_id)

        conditions, condition_delta_s = _nearest_photo_conditions(
            connection, tub_id, taken_at_utc
        )
        cursor = connection.execute(
            """
            INSERT INTO grow_photos (
                tub_id, stored_name, original_name, media_type, size_bytes,
                caption, taken_on, taken_at_utc, capture_time_source,
                condition_measurement_id, condition_node_id,
                conditions_recorded_utc, condition_time_delta_s,
                condition_co2_ppm, condition_temperature_c,
                condition_humidity_percent, condition_battery_mv, uploaded_utc
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                tub_id,
                stored_name,
                original_name,
                media_type,
                size_bytes,
                caption.strip(),
                taken_on,
                taken_at_utc,
                capture_time_source,
                conditions["id"] if conditions is not None else None,
                conditions["node_id"] if conditions is not None else None,
                conditions["received_utc"] if conditions is not None else None,
                condition_delta_s,
                conditions["co2_ppm"] if conditions is not None else None,
                conditions["temperature_c"] if conditions is not None else None,
                conditions["humidity_percent"] if conditions is not None else None,
                conditions["battery_mv"] if conditions is not None else None,
                _utc_now(),
            ),
        )
        photo_id = int(cursor.lastrowid)
        row = connection.execute(
            "SELECT * FROM grow_photos WHERE photo_id = ?", (photo_id,)
        ).fetchone()
    assert row is not None
    return _format_photo(row)


def _format_photo(row: sqlite3.Row) -> dict[str, Any]:
    photo = dict(row)
    photo["condition_battery_voltage_v"] = (
        photo["condition_battery_mv"] / 1000.0
        if photo.get("condition_battery_mv") is not None
        else None
    )
    return photo


PHOTO_CONDITION_MAX_DELTA_S = 6 * 60 * 60


def _nearest_photo_conditions(
    connection: sqlite3.Connection,
    tub_id: int,
    taken_at_utc: str | None,
) -> tuple[sqlite3.Row | None, int | None]:
    if taken_at_utc is None:
        return None, None
    conditions = connection.execute(
        """
        SELECT id, node_id, received_utc, co2_ppm, temperature_c,
               humidity_percent, battery_mv
        FROM measurements
        WHERE tub_id = ? AND sensor_valid = 1
        ORDER BY ABS(julianday(received_utc) - julianday(?)), id DESC
        LIMIT 1
        """,
        (tub_id, taken_at_utc),
    ).fetchone()
    if conditions is None:
        return None, None
    captured = datetime.fromisoformat(taken_at_utc)
    recorded = datetime.fromisoformat(str(conditions["received_utc"]))
    delta_s = round(abs((recorded - captured).total_seconds()))
    if delta_s > PHOTO_CONDITION_MAX_DELTA_S:
        return None, None
    return conditions, delta_s


def get_photos_needing_metadata(database_path: Path) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        rows = connection.execute(
            """
            SELECT * FROM grow_photos
            WHERE taken_at_utc IS NULL OR capture_time_source = 'unknown'
            ORDER BY photo_id
            """
        ).fetchall()
    return [dict(row) for row in rows]


def update_grow_photo_capture_metadata(
    database_path: Path,
    photo_id: int,
    *,
    taken_on: str,
    taken_at_utc: str,
    capture_time_source: str,
) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        photo = connection.execute(
            "SELECT * FROM grow_photos WHERE photo_id = ?", (photo_id,)
        ).fetchone()
        if photo is None:
            return None
        conditions, delta_s = _nearest_photo_conditions(
            connection, int(photo["tub_id"]), taken_at_utc
        )
        connection.execute(
            """
            UPDATE grow_photos SET
                taken_on = ?, taken_at_utc = ?, capture_time_source = ?,
                condition_measurement_id = ?, condition_node_id = ?,
                conditions_recorded_utc = ?, condition_time_delta_s = ?,
                condition_co2_ppm = ?, condition_temperature_c = ?,
                condition_humidity_percent = ?, condition_battery_mv = ?
            WHERE photo_id = ?
            """,
            (
                taken_on,
                taken_at_utc,
                capture_time_source,
                conditions["id"] if conditions is not None else None,
                conditions["node_id"] if conditions is not None else None,
                conditions["received_utc"] if conditions is not None else None,
                delta_s,
                conditions["co2_ppm"] if conditions is not None else None,
                conditions["temperature_c"] if conditions is not None else None,
                conditions["humidity_percent"] if conditions is not None else None,
                conditions["battery_mv"] if conditions is not None else None,
                photo_id,
            ),
        )
        updated = connection.execute(
            "SELECT * FROM grow_photos WHERE photo_id = ?", (photo_id,)
        ).fetchone()
    return _format_photo(updated) if updated is not None else None


def get_grow_photo(database_path: Path, photo_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            "SELECT * FROM grow_photos WHERE photo_id = ?", (photo_id,)
        ).fetchone()
    return dict(row) if row else None


def delete_grow_photo(database_path: Path, tub_id: int, photo_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            "SELECT * FROM grow_photos WHERE photo_id = ? AND tub_id = ?",
            (photo_id, tub_id),
        ).fetchone()
        if row is None:
            return None
        connection.execute("DELETE FROM grow_photos WHERE photo_id = ?", (photo_id,))
    return dict(row)


def _format_spawn_jar_photo(row: sqlite3.Row) -> dict[str, Any]:
    return dict(row)


def _get_spawn_jar_with_connection(
    connection: sqlite3.Connection, jar_id: int
) -> dict[str, Any] | None:
    row = connection.execute(
        """
        SELECT j.*,
               t.name AS spawned_to_tub_name,
               (SELECT COUNT(*) FROM spawn_jar_photos p WHERE p.jar_id = j.jar_id)
                   AS photo_count
        FROM spawn_jars AS j
        LEFT JOIN tubs AS t ON t.tub_id = j.spawned_to_tub_id
        WHERE j.jar_id = ?
        """,
        (jar_id,),
    ).fetchone()
    if row is None:
        return None
    jar = dict(row)
    jar["locked"] = bool(jar["locked"])
    jar["break_shake_dates"] = [
        str(item["break_shake_on"])
        for item in connection.execute(
            """
            SELECT break_shake_on FROM spawn_jar_break_shakes
            WHERE jar_id = ? ORDER BY break_shake_on
            """,
            (jar_id,),
        ).fetchall()
    ]
    jar["photos"] = [
        _format_spawn_jar_photo(item)
        for item in connection.execute(
            """
            SELECT photo_id, original_name, media_type, size_bytes, caption,
                   taken_on, taken_at_utc, capture_time_source, uploaded_utc
            FROM spawn_jar_photos WHERE jar_id = ?
            ORDER BY COALESCE(taken_at_utc, uploaded_utc) DESC, photo_id DESC
            """,
            (jar_id,),
        ).fetchall()
    ]
    return jar


def get_spawn_jars(
    database_path: Path, *, status: str | None = "active"
) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        query = "SELECT jar_id FROM spawn_jars"
        parameters: tuple[Any, ...] = ()
        if status is not None:
            query += " WHERE status = ?"
            parameters = (status,)
        query += " ORDER BY updated_utc DESC, jar_id DESC"
        ids = connection.execute(query, parameters).fetchall()
        return [
            jar
            for item in ids
            if (jar := _get_spawn_jar_with_connection(connection, int(item["jar_id"])))
            is not None
        ]


def get_spawn_jar(database_path: Path, jar_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        return _get_spawn_jar_with_connection(connection, jar_id)


def archive_spawn_jar(
    database_path: Path,
    jar_id: int,
    *,
    contaminated: bool,
    occurred_on: str,
    reason: str,
) -> dict[str, Any]:
    now = _utc_now()
    status = "failed" if contaminated else "archived"
    category = "failed_jar" if contaminated else "archived_jar"
    with connect(database_path) as connection:
        existing = connection.execute(
            "SELECT status FROM spawn_jars WHERE jar_id = ?", (jar_id,)
        ).fetchone()
        if existing is None:
            raise KeyError(jar_id)
        if str(existing["status"]) != "active":
            raise ValueError("Only a current jar can be archived")
        connection.execute(
            """
            UPDATE spawn_jars SET status = ?, archive_category = ?,
                lifecycle_reason = ?, contaminated_on = ?, archived_utc = ?,
                locked = 1, updated_utc = ? WHERE jar_id = ?
            """,
            (
                status,
                category,
                reason.strip(),
                occurred_on if contaminated else None,
                now,
                now,
                jar_id,
            ),
        )
    result = get_spawn_jar(database_path, jar_id)
    assert result is not None
    return result


def delete_spawn_jar(database_path: Path, jar_id: int) -> list[str]:
    with connect(database_path) as connection:
        exists = connection.execute(
            "SELECT jar_id FROM spawn_jars WHERE jar_id = ?", (jar_id,)
        ).fetchone()
        if exists is None:
            raise KeyError(jar_id)
        photo_names = [
            str(row["stored_name"])
            for row in connection.execute(
                "SELECT stored_name FROM spawn_jar_photos WHERE jar_id = ?",
                (jar_id,),
            ).fetchall()
        ]
        connection.execute("DELETE FROM spawn_jars WHERE jar_id = ?", (jar_id,))
    return photo_names


def create_spawn_jar(
    database_path: Path,
    *,
    name: str,
    grain_type: str,
    prep_tek: str,
    pressure_cooker_minutes: int | None,
    pressure_psi: float | None,
    dry_grain_grams_per_jar: float | None,
    jar_count: int,
    pressure_cooked_on: str | None,
    inoculated_on: str | None,
    culture: str,
    species: str,
    break_shake_dates: list[str],
    notes: str,
) -> dict[str, Any]:
    now = _utc_now()
    with connect(database_path) as connection:
        cursor = connection.execute(
            """
            INSERT INTO spawn_jars (
                name, grain_type, prep_tek, pressure_cooker_minutes,
                pressure_psi, dry_grain_grams_per_jar, jar_count,
                pressure_cooked_on, inoculated_on, culture, species, notes,
                status, locked, created_utc, updated_utc
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'active', 0, ?, ?)
            """,
            (
                name.strip(), grain_type.strip(), prep_tek.strip(),
                pressure_cooker_minutes, pressure_psi, dry_grain_grams_per_jar,
                jar_count, pressure_cooked_on, inoculated_on, culture.strip(),
                species.strip(), notes.strip(), now, now,
            ),
        )
        jar_id = int(cursor.lastrowid)
        for break_shake_on in sorted(set(break_shake_dates)):
            connection.execute(
                """
                INSERT INTO spawn_jar_break_shakes
                    (jar_id, break_shake_on, created_utc) VALUES (?, ?, ?)
                """,
                (jar_id, break_shake_on, now),
            )
    result = get_spawn_jar(database_path, jar_id)
    assert result is not None
    return result


def create_spawn_jars(
    database_path: Path,
    *,
    quantity: int,
    name: str,
    grain_type: str,
    prep_tek: str,
    pressure_cooker_minutes: int | None,
    pressure_psi: float | None,
    dry_grain_grams_per_jar: float | None,
    pressure_cooked_on: str | None,
    inoculated_on: str | None,
    culture: str,
    species: str,
    break_shake_dates: list[str],
    notes: str,
) -> list[dict[str, Any]]:
    """Create one database record per physical jar in one transaction."""
    now = _utc_now()
    base_name = name.strip()
    names = (
        [base_name]
        if quantity == 1
        else [f"{base_name} #{index}" for index in range(1, quantity + 1)]
    )
    jar_ids: list[int] = []
    with connect(database_path) as connection:
        for jar_name in names:
            cursor = connection.execute(
                """
                INSERT INTO spawn_jars (
                    name, grain_type, prep_tek, pressure_cooker_minutes,
                    pressure_psi, dry_grain_grams_per_jar, jar_count,
                    pressure_cooked_on, inoculated_on, culture, species, notes,
                    status, locked, created_utc, updated_utc
                ) VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, 'active', 0, ?, ?)
                """,
                (
                    jar_name, grain_type.strip(), prep_tek.strip(),
                    pressure_cooker_minutes, pressure_psi,
                    dry_grain_grams_per_jar, pressure_cooked_on,
                    inoculated_on, culture.strip(), species.strip(), notes.strip(),
                    now, now,
                ),
            )
            jar_id = int(cursor.lastrowid)
            jar_ids.append(jar_id)
            for break_shake_on in sorted(set(break_shake_dates)):
                connection.execute(
                    """
                    INSERT INTO spawn_jar_break_shakes
                        (jar_id, break_shake_on, created_utc) VALUES (?, ?, ?)
                    """,
                    (jar_id, break_shake_on, now),
                )
    results = [get_spawn_jar(database_path, jar_id) for jar_id in jar_ids]
    return [result for result in results if result is not None]


def update_spawn_jar(
    database_path: Path,
    jar_id: int,
    *,
    name: str,
    grain_type: str,
    prep_tek: str,
    pressure_cooker_minutes: int | None,
    pressure_psi: float | None,
    dry_grain_grams_per_jar: float | None,
    jar_count: int,
    pressure_cooked_on: str | None,
    inoculated_on: str | None,
    culture: str,
    species: str,
    break_shake_dates: list[str],
    notes: str,
) -> dict[str, Any]:
    now = _utc_now()
    with connect(database_path) as connection:
        existing = connection.execute(
            "SELECT locked FROM spawn_jars WHERE jar_id = ?", (jar_id,)
        ).fetchone()
        if existing is None:
            raise KeyError(jar_id)
        if bool(existing["locked"]):
            raise PermissionError("Spawn jar record is locked")
        connection.execute(
            """
            UPDATE spawn_jars SET name = ?, grain_type = ?, prep_tek = ?,
                pressure_cooker_minutes = ?, pressure_psi = ?,
                dry_grain_grams_per_jar = ?, jar_count = ?,
                pressure_cooked_on = ?, inoculated_on = ?, culture = ?,
                species = ?, notes = ?, updated_utc = ? WHERE jar_id = ?
            """,
            (
                name.strip(), grain_type.strip(), prep_tek.strip(),
                pressure_cooker_minutes, pressure_psi, dry_grain_grams_per_jar,
                jar_count, pressure_cooked_on, inoculated_on, culture.strip(),
                species.strip(), notes.strip(), now, jar_id,
            ),
        )
        connection.execute(
            "DELETE FROM spawn_jar_break_shakes WHERE jar_id = ?", (jar_id,)
        )
        for break_shake_on in sorted(set(break_shake_dates)):
            connection.execute(
                """
                INSERT INTO spawn_jar_break_shakes
                    (jar_id, break_shake_on, created_utc) VALUES (?, ?, ?)
                """,
                (jar_id, break_shake_on, now),
            )
    result = get_spawn_jar(database_path, jar_id)
    assert result is not None
    return result


def set_spawn_jar_locked(
    database_path: Path, jar_id: int, *, locked: bool
) -> dict[str, Any]:
    with connect(database_path) as connection:
        cursor = connection.execute(
            "UPDATE spawn_jars SET locked = ?, updated_utc = ? WHERE jar_id = ?",
            (int(locked), _utc_now(), jar_id),
        )
        if cursor.rowcount == 0:
            raise KeyError(jar_id)
    result = get_spawn_jar(database_path, jar_id)
    assert result is not None
    return result


def spawn_jar_groups_to_tubs(
    database_path: Path,
    groups: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    """Atomically create one tub per same-culture group of physical jars."""
    if not groups:
        raise ValueError("Select at least one current jar")
    now = _utc_now()
    prepared: list[dict[str, Any]] = []
    seen_jar_ids: set[int] = set()
    seen_node_ids: set[int] = set()
    seen_tub_names: set[str] = set()
    created_tub_ids: list[int] = []
    with connect(database_path) as connection:
        for group in groups:
            jar_ids = list(dict.fromkeys(int(item) for item in group["jar_ids"]))
            if not jar_ids:
                raise ValueError("Each tub must contain at least one jar")
            if seen_jar_ids.intersection(jar_ids):
                raise ValueError("A jar cannot be spawned into more than one tub")
            seen_jar_ids.update(jar_ids)

            jars: list[sqlite3.Row] = []
            for jar_id in jar_ids:
                jar = connection.execute(
                    "SELECT * FROM spawn_jars WHERE jar_id = ?", (jar_id,)
                ).fetchone()
                if jar is None:
                    raise KeyError(jar_id)
                if str(jar["status"]) != "active":
                    raise ValueError("Only current jars can be spawned to a tub")
                jars.append(jar)

            cultures = {
                str(jar["culture"]).strip().casefold()
                for jar in jars if str(jar["culture"]).strip()
            }
            if len(cultures) != 1 or any(not str(jar["culture"]).strip() for jar in jars):
                raise ValueError(
                    "Every jar in a tub must have the same non-empty culture name"
                )
            species_values = {
                str(jar["species"]).strip().casefold()
                for jar in jars if str(jar["species"]).strip()
            }
            if len(species_values) > 1:
                raise ValueError(
                    "Jars with mismatched species cannot be spawned into one tub"
                )

            tub_name = str(group["tub_name"]).strip()
            normalized_tub_name = tub_name.casefold()
            if not tub_name or normalized_tub_name in seen_tub_names:
                raise ValueError("Each resulting tub needs a unique name")
            seen_tub_names.add(normalized_tub_name)
            if connection.execute(
                "SELECT tub_id FROM tubs WHERE name = ? COLLATE NOCASE", (tub_name,)
            ).fetchone() is not None:
                raise sqlite3.IntegrityError("Tub name already exists")

            spawn_ratio = str(group["spawn_ratio"]).strip()
            if not spawn_ratio:
                raise ValueError("Enter a spawn ratio for every resulting tub")

            node_id = group.get("node_id")
            if node_id is not None:
                node_id = int(node_id)
                if node_id in seen_node_ids:
                    raise ValueError("A sensor node cannot be assigned to two new tubs")
                seen_node_ids.add(node_id)
                if connection.execute(
                    "SELECT node_id FROM nodes WHERE node_id = ?", (node_id,)
                ).fetchone() is None:
                    raise LookupError(node_id)
                if connection.execute(
                    """
                    SELECT assignment_id FROM node_tub_assignments
                    WHERE node_id = ? AND unassigned_utc IS NULL
                    """,
                    (node_id,),
                ).fetchone() is not None:
                    raise RuntimeError("That node is already assigned to a tub")

            prepared.append({
                "jar_ids": jar_ids,
                "jars": jars,
                "tub_name": tub_name,
                "node_id": node_id,
                "spawn_ratio": spawn_ratio,
                "spawn_to_bulk_on": str(group["spawn_to_bulk_on"]),
                "notes": str(group.get("notes", "")).strip(),
                "culture": str(jars[0]["culture"]).strip(),
                "species": next(
                    (str(jar["species"]).strip() for jar in jars
                     if str(jar["species"]).strip()),
                    "",
                ),
            })

        for group in prepared:
            cursor = connection.execute(
                """
                INSERT INTO tubs (
                    name, species, strain, started_on, spawn_to_bulk_on,
                    stage, spawn_ratio, notes, active, created_utc, updated_utc
                ) VALUES (?, ?, ?, ?, ?, 'colonizing', ?, ?, 1, ?, ?)
                """,
                (
                    group["tub_name"], group["species"], group["culture"],
                    group["spawn_to_bulk_on"],
                    group["spawn_to_bulk_on"], group["spawn_ratio"],
                    group["notes"], now, now,
                ),
            )
            tub_id = int(cursor.lastrowid)
            created_tub_ids.append(tub_id)
            if group["node_id"] is not None:
                connection.execute(
                    """
                    INSERT INTO node_tub_assignments
                        (node_id, tub_id, assigned_utc) VALUES (?, ?, ?)
                    """,
                    (group["node_id"], tub_id, now),
                )
            for jar_id in group["jar_ids"]:
                connection.execute(
                    """
                    UPDATE spawn_jars SET status = 'spawned',
                        spawned_to_tub_id = ?, spawned_utc = ?, locked = 1,
                        updated_utc = ? WHERE jar_id = ?
                    """,
                    (tub_id, now, now, jar_id),
                )

    grows = [get_grow(database_path, tub_id) for tub_id in created_tub_ids]
    assert all(grow is not None for grow in grows)
    return [grow for grow in grows if grow is not None]


def spawn_jars_to_tub(
    database_path: Path,
    jar_ids: list[int],
    *,
    tub_name: str,
    node_id: int | None,
    species: str,
    strain: str,
    spawn_to_bulk_on: str,
    notes: str,
    spawn_ratio: str = "Not recorded",
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    # species and strain are retained in this compatibility wrapper; the
    # authoritative values now always come from the source jars.
    del species, strain
    grows = spawn_jar_groups_to_tubs(database_path, [{
        "jar_ids": jar_ids,
        "tub_name": tub_name,
        "node_id": node_id,
        "spawn_ratio": spawn_ratio,
        "spawn_to_bulk_on": spawn_to_bulk_on,
        "notes": notes,
    }])
    grow = grows[0]
    return grow, list(grow["spawn_jars"])


def spawn_jar_to_tub(
    database_path: Path,
    jar_id: int,
    **kwargs: Any,
) -> tuple[dict[str, Any], dict[str, Any]]:
    grow, jars = spawn_jars_to_tub(database_path, [jar_id], **kwargs)
    return grow, jars[0]


def add_spawn_jar_photo(
    database_path: Path,
    jar_id: int,
    *,
    stored_name: str,
    original_name: str,
    media_type: str,
    size_bytes: int,
    caption: str,
    taken_on: str,
    taken_at_utc: str,
    capture_time_source: str,
) -> dict[str, Any]:
    with connect(database_path) as connection:
        jar = connection.execute(
            "SELECT locked FROM spawn_jars WHERE jar_id = ?", (jar_id,)
        ).fetchone()
        if jar is None:
            raise KeyError(jar_id)
        if bool(jar["locked"]):
            raise PermissionError("Spawn jar record is locked")
        cursor = connection.execute(
            """
            INSERT INTO spawn_jar_photos (
                jar_id, stored_name, original_name, media_type, size_bytes,
                caption, taken_on, taken_at_utc, capture_time_source, uploaded_utc
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                jar_id, stored_name, original_name, media_type, size_bytes,
                caption.strip(), taken_on, taken_at_utc, capture_time_source,
                _utc_now(),
            ),
        )
        row = connection.execute(
            "SELECT * FROM spawn_jar_photos WHERE photo_id = ?",
            (int(cursor.lastrowid),),
        ).fetchone()
    assert row is not None
    return _format_spawn_jar_photo(row)


def get_spawn_jar_photo(database_path: Path, photo_id: int) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        row = connection.execute(
            "SELECT * FROM spawn_jar_photos WHERE photo_id = ?", (photo_id,)
        ).fetchone()
    return dict(row) if row is not None else None


def delete_spawn_jar_photo(
    database_path: Path, jar_id: int, photo_id: int
) -> dict[str, Any] | None:
    with connect(database_path) as connection:
        jar = connection.execute(
            "SELECT locked FROM spawn_jars WHERE jar_id = ?", (jar_id,)
        ).fetchone()
        if jar is None:
            return None
        if bool(jar["locked"]):
            raise PermissionError("Spawn jar record is locked")
        row = connection.execute(
            "SELECT * FROM spawn_jar_photos WHERE photo_id = ? AND jar_id = ?",
            (photo_id, jar_id),
        ).fetchone()
        if row is None:
            return None
        connection.execute(
            "DELETE FROM spawn_jar_photos WHERE photo_id = ?", (photo_id,)
        )
    return dict(row)


def _expire_provisioning_reservations(
    connection: sqlite3.Connection, now: str
) -> None:
    connection.execute(
        """
        UPDATE provisioning_reservations SET status = 'expired'
        WHERE status = 'active' AND expires_utc <= ?
        """,
        (now,),
    )


def _occupied_node_ids(connection: sqlite3.Connection) -> set[int]:
    occupied = {
        int(row["node_id"])
        for row in connection.execute("SELECT node_id FROM nodes").fetchall()
    }
    occupied.update(
        int(row["node_id"])
        for row in connection.execute(
            "SELECT node_id FROM provisioned_transmitters"
        ).fetchall()
    )
    occupied.update(
        int(row["node_id"])
        for row in connection.execute(
            """
            SELECT node_id FROM provisioning_reservations
            WHERE status = 'active'
            """
        ).fetchall()
    )
    return occupied


def get_provisioning_status(
    database_path: Path, hardware_uid: str | None = None
) -> dict[str, Any]:
    now = _utc_now()
    with connect(database_path) as connection:
        _expire_provisioning_reservations(connection, now)
        registered = None
        reservation = None
        if hardware_uid is not None:
            registered_row = connection.execute(
                """
                SELECT hardware_uid, node_id, provisioned_utc,
                       last_provisioned_utc
                FROM provisioned_transmitters WHERE hardware_uid = ?
                """,
                (hardware_uid,),
            ).fetchone()
            registered = dict(registered_row) if registered_row else None
            reservation_row = connection.execute(
                """
                SELECT reservation_token, hardware_uid, node_id, created_utc,
                       expires_utc, status
                FROM provisioning_reservations
                WHERE hardware_uid = ? AND status = 'active'
                ORDER BY created_utc DESC LIMIT 1
                """,
                (hardware_uid,),
            ).fetchone()
            reservation = dict(reservation_row) if reservation_row else None
        occupied = _occupied_node_ids(connection)
        next_id = 1
        while next_id in occupied:
            next_id += 1
    return {
        "hardware_uid": hardware_uid,
        "registered": registered,
        "active_reservation": reservation,
        "next_available_node_id": next_id,
        "occupied_node_ids": sorted(occupied),
    }


def reserve_node_id(
    database_path: Path,
    *,
    hardware_uid: str,
    reservation_token: str,
    requested_node_id: int | None,
    ttl_seconds: int,
    claim_existing_node: bool = False,
) -> dict[str, Any]:
    now_dt = datetime.now(UTC)
    now = now_dt.isoformat()
    expires = (now_dt + timedelta(seconds=ttl_seconds)).isoformat()
    with connect(database_path) as connection:
        _expire_provisioning_reservations(connection, now)
        existing_reservation = connection.execute(
            """
            SELECT * FROM provisioning_reservations
            WHERE hardware_uid = ? AND status = 'active'
            ORDER BY created_utc DESC LIMIT 1
            """,
            (hardware_uid,),
        ).fetchone()
        if existing_reservation is not None:
            if (
                requested_node_id is not None
                and requested_node_id != int(existing_reservation["node_id"])
            ):
                raise ValueError(
                    "This transmitter already has a different active reservation"
                )
            return dict(existing_reservation)

        registered = connection.execute(
            "SELECT node_id FROM provisioned_transmitters WHERE hardware_uid = ?",
            (hardware_uid,),
        ).fetchone()
        registered_node_id = int(registered["node_id"]) if registered else None
        occupied = _occupied_node_ids(connection)
        if registered_node_id is not None:
            occupied.discard(registered_node_id)
        node_id = requested_node_id or registered_node_id
        if node_id is None:
            node_id = 1
            while node_id in occupied:
                node_id += 1
        if node_id <= 0 or node_id >= 0xFFFFFFFF:
            raise ValueError("Node ID must be between 1 and 4294967294")
        if node_id in occupied:
            registered_conflict = connection.execute(
                """
                SELECT 1 FROM provisioned_transmitters
                WHERE node_id = ? AND hardware_uid != ?
                """,
                (node_id, hardware_uid),
            ).fetchone()
            reservation_conflict = connection.execute(
                """
                SELECT 1 FROM provisioning_reservations
                WHERE node_id = ? AND hardware_uid != ? AND status = 'active'
                """,
                (node_id, hardware_uid),
            ).fetchone()
            historical_node = connection.execute(
                "SELECT 1 FROM nodes WHERE node_id = ?",
                (node_id,),
            ).fetchone()
            may_adopt_history = (
                claim_existing_node
                and requested_node_id == node_id
                and historical_node is not None
                and registered_conflict is None
                and reservation_conflict is None
            )
            if not may_adopt_history:
                raise ValueError(f"Node ID {node_id} is already in use or reserved")

        connection.execute(
            """
            INSERT INTO provisioning_reservations (
                reservation_token, hardware_uid, node_id, created_utc,
                expires_utc, status
            ) VALUES (?, ?, ?, ?, ?, 'active')
            """,
            (reservation_token, hardware_uid, node_id, now, expires),
        )
        row = connection.execute(
            "SELECT * FROM provisioning_reservations WHERE reservation_token = ?",
            (reservation_token,),
        ).fetchone()
    assert row is not None
    return dict(row)


def complete_node_provisioning(
    database_path: Path,
    *,
    reservation_token: str,
    hardware_uid: str,
) -> dict[str, Any]:
    now = _utc_now()
    with connect(database_path) as connection:
        _expire_provisioning_reservations(connection, now)
        reservation = connection.execute(
            """
            SELECT * FROM provisioning_reservations
            WHERE reservation_token = ?
            """,
            (reservation_token,),
        ).fetchone()
        if reservation is None:
            raise KeyError(reservation_token)
        if str(reservation["status"]) != "active":
            raise ValueError(f"Reservation is {reservation['status']}")
        if str(reservation["hardware_uid"]) != hardware_uid:
            raise ValueError("Reservation belongs to a different hardware UID")
        node_id = int(reservation["node_id"])
        conflict = connection.execute(
            """
            SELECT hardware_uid FROM provisioned_transmitters
            WHERE node_id = ? AND hardware_uid != ?
            """,
            (node_id, hardware_uid),
        ).fetchone()
        if conflict is not None:
            raise ValueError(f"Node ID {node_id} was registered by another device")
        existing = connection.execute(
            """
            SELECT provisioned_utc FROM provisioned_transmitters
            WHERE hardware_uid = ?
            """,
            (hardware_uid,),
        ).fetchone()
        first_provisioned = (
            str(existing["provisioned_utc"]) if existing is not None else now
        )
        connection.execute(
            """
            INSERT INTO provisioned_transmitters (
                hardware_uid, node_id, provisioned_utc, last_provisioned_utc
            ) VALUES (?, ?, ?, ?)
            ON CONFLICT(hardware_uid) DO UPDATE SET
                node_id = excluded.node_id,
                last_provisioned_utc = excluded.last_provisioned_utc
            """,
            (hardware_uid, node_id, first_provisioned, now),
        )
        connection.execute(
            """
            UPDATE provisioning_reservations SET status = 'completed'
            WHERE reservation_token = ?
            """,
            (reservation_token,),
        )
        device = connection.execute(
            "SELECT * FROM provisioned_transmitters WHERE hardware_uid = ?",
            (hardware_uid,),
        ).fetchone()
    assert device is not None
    return dict(device)


def cancel_node_reservation(
    database_path: Path, *, reservation_token: str, hardware_uid: str
) -> bool:
    with connect(database_path) as connection:
        cursor = connection.execute(
            """
            UPDATE provisioning_reservations SET status = 'cancelled'
            WHERE reservation_token = ? AND hardware_uid = ? AND status = 'active'
            """,
            (reservation_token, hardware_uid),
        )
    return cursor.rowcount > 0


def get_provisioned_transmitters(database_path: Path) -> list[dict[str, Any]]:
    with connect(database_path) as connection:
        rows = connection.execute(
            """
            SELECT hardware_uid, node_id, provisioned_utc, last_provisioned_utc
            FROM provisioned_transmitters ORDER BY node_id
            """
        ).fetchall()
    return [dict(row) for row in rows]


def get_counts(database_path: Path) -> dict[str, int]:
    with connect(database_path) as connection:
        node_count = connection.execute("SELECT COUNT(*) FROM nodes").fetchone()[0]
        measurement_count = connection.execute(
            "SELECT COUNT(*) FROM measurements"
        ).fetchone()[0]
        pending_count = connection.execute(
            "SELECT COUNT(*) FROM node_commands WHERE status IN ('queued', 'sent')"
        ).fetchone()[0]
        grow_count = connection.execute(
            "SELECT COUNT(*) FROM tubs WHERE active = 1"
        ).fetchone()[0]
        jar_count = connection.execute(
            "SELECT COUNT(*) FROM spawn_jars WHERE status = 'active'"
        ).fetchone()[0]
    return {
        "nodes": node_count,
        "measurements": measurement_count,
        "pending_commands": pending_count,
        "current_grows": grow_count,
        "current_jars": jar_count,
    }
