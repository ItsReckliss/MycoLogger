from __future__ import annotations

import tempfile
import unittest
import json
from datetime import UTC, datetime
from io import BytesIO
from pathlib import Path
from types import SimpleNamespace

from mycologger.database import (
    archive_grow,
    archive_spawn_jar,
    add_spawn_jar_photo,
    add_grow_photo,
    cancel_node_reservation,
    complete_node_provisioning,
    create_spawn_jar,
    create_spawn_jars,
    delete_grow,
    delete_spawn_jar,
    delete_grow_photo,
    get_current_grows,
    get_archived_grows,
    get_counts,
    get_grow,
    get_node,
    get_nodes,
    get_spawn_jar,
    get_spawn_jars,
    get_provisioning_status,
    initialize,
    reserve_node_id,
    set_spawn_jar_locked,
    spawn_jar_to_tub,
    spawn_jar_groups_to_tubs,
    spawn_jars_to_tub,
    update_grow,
    update_node_settings,
    update_spawn_jar,
)
from mycologger.protocol import decode_radio_payload
from mycologger.photo_metadata import extract_capture_time
from mycologger.receiver import ReceiverService, find_receiver_port
from PIL import Image
from tools.provision_transmitter import (
    CONFIG_SIZE,
    CONFIG_LAYOUT_VERSION,
    CONFIG_MAGIC,
    CONFIG_PROVISIONING_MARKER,
    build_config_image,
    build_redundant_config_images,
    fnv1a32,
    parse_config_image,
    validate_firmware_image,
)


def sensor_record(
    *,
    node_id: int = 1,
    tx_sequence: int = 12,
    uptime_s: int = 60,
    co2_ppm: int = 733,
    temperature_centi_c: int = 2312,
    humidity_centi_percent: int = 5544,
    config_revision: int | None = None,
    report_interval_s: int | None = None,
    battery_mv: int | None = None,
    firmware_version: tuple[int, int, int] | None = None,
    reset_flags: int | None = None,
    sensor_failure_count: int | None = None,
    radio_failure_count: int | None = None,
    network_confirmation_requested: bool = False,
) -> dict:
    payload = bytearray(b"MYCO")
    payload.extend((1, 2))
    payload.extend(node_id.to_bytes(4, "big"))
    payload.extend(tx_sequence.to_bytes(4, "big"))
    payload.extend(uptime_s.to_bytes(4, "big"))
    payload.append(
        0x02
        | (0x04 if battery_mv is not None else 0)
        | (0x08 if network_confirmation_requested else 0)
    )
    payload.extend(co2_ppm.to_bytes(2, "big"))
    payload.extend(temperature_centi_c.to_bytes(2, "big", signed=True))
    payload.extend(humidity_centi_percent.to_bytes(2, "big"))
    payload.append(0)
    if config_revision is not None and report_interval_s is not None:
        payload.extend(config_revision.to_bytes(4, "big"))
        payload.extend(report_interval_s.to_bytes(4, "big"))
    if battery_mv is not None:
        if config_revision is None or report_interval_s is None:
            payload.extend((0).to_bytes(4, "big"))
            payload.extend((60).to_bytes(4, "big"))
        payload.extend(battery_mv.to_bytes(2, "big"))
    if firmware_version is not None:
        if len(payload) < 36:
            payload.extend(bytes(36 - len(payload)))
        payload.extend(firmware_version)
    if reset_flags is not None:
        assert sensor_failure_count is not None
        assert radio_failure_count is not None
        if len(payload) < 39:
            payload.extend(bytes(39 - len(payload)))
        payload.extend(reset_flags.to_bytes(4, "big"))
        payload.extend(sensor_failure_count.to_bytes(2, "big"))
        payload.extend(radio_failure_count.to_bytes(2, "big"))
    return {
        "v": 1,
        "type": "packet",
        "seq": tx_sequence,
        "length": len(payload),
        "rssi_dbm_x2": -44,
        "snr_db_quarters": 36,
        "payload_hex": payload.hex(),
    }


def config_ack_record(
    *, node_id: int, transaction_id: int, revision: int, interval_s: int
) -> dict:
    payload = bytearray(b"MYCO")
    payload.extend((1, 0x81))
    payload.extend(node_id.to_bytes(4, "big"))
    payload.extend(transaction_id.to_bytes(4, "big"))
    payload.extend(revision.to_bytes(4, "big"))
    payload.append(0)
    payload.extend(interval_s.to_bytes(4, "big"))
    return {
        "v": 1,
        "type": "packet",
        "seq": 100,
        "length": len(payload),
        "rssi_dbm_x2": -40,
        "snr_db_quarters": 32,
        "payload_hex": payload.hex(),
    }


def link_check_record(*, node_id: int = 1, tx_sequence: int = 1) -> dict:
    payload = bytearray(b"MYCO")
    payload.extend((1, 3))
    payload.extend(node_id.to_bytes(4, "big"))
    payload.extend(tx_sequence.to_bytes(4, "big"))
    return {
        "v": 1,
        "type": "packet",
        "seq": tx_sequence,
        "length": len(payload),
        "rssi_dbm_x2": -44,
        "snr_db_quarters": 36,
        "payload_hex": payload.hex(),
    }


class DiscoveryTests(unittest.TestCase):
    def test_prefers_product_name(self) -> None:
        ports = [
            SimpleNamespace(
                device="COM9",
                product="MycoLogger Receiver",
                description="USB Serial",
                manufacturer="MycoLogger",
                vid=0x1234,
                pid=0x5678,
            )
        ]
        self.assertEqual(find_receiver_port(available_ports=ports), "COM9")

    def test_matches_prototype_vid_pid(self) -> None:
        ports = [
            SimpleNamespace(
                device="/dev/ttyACM0",
                product=None,
                description="STM32 Virtual ComPort",
                manufacturer="STMicroelectronics",
                vid=0x0483,
                pid=0x5740,
            )
        ]
        self.assertEqual(find_receiver_port(available_ports=ports), "/dev/ttyACM0")

    def test_requested_port_must_be_present(self) -> None:
        ports = [SimpleNamespace(device="COM4")]
        self.assertEqual(find_receiver_port("com4", ports), "COM4")
        self.assertIsNone(find_receiver_port("COM6", ports))


class ProtocolTests(unittest.TestCase):
    def test_decodes_sensor_packet(self) -> None:
        decoded = decode_radio_payload(sensor_record())
        self.assertIsNotNone(decoded)
        assert decoded is not None
        self.assertEqual(decoded["node_id"], 1)
        self.assertEqual(decoded["co2_ppm"], 733)
        self.assertEqual(decoded["temperature_c"], 23.12)
        self.assertEqual(decoded["humidity_percent"], 55.44)
        self.assertTrue(decoded["sensor_valid"])

    def test_rejects_bad_payload(self) -> None:
        self.assertIsNone(
            decode_radio_payload({"v": 1, "type": "packet", "payload_hex": "00"})
        )

    def test_decodes_battery_voltage_extension(self) -> None:
        decoded = decode_radio_payload(sensor_record(battery_mv=4123))
        self.assertIsNotNone(decoded)
        assert decoded is not None
        self.assertTrue(decoded["battery_valid"])
        self.assertEqual(decoded["battery_mv"], 4123)
        self.assertEqual(decoded["battery_voltage_v"], 4.123)

    def test_decodes_network_confirmation_request(self) -> None:
        decoded = decode_radio_payload(
            sensor_record(network_confirmation_requested=True)
        )
        self.assertIsNotNone(decoded)
        assert decoded is not None
        self.assertTrue(decoded["network_confirmation_requested"])

    def test_decodes_transmitter_firmware_version(self) -> None:
        decoded = decode_radio_payload(
            sensor_record(battery_mv=3987, firmware_version=(0, 6, 0))
        )
        self.assertIsNotNone(decoded)
        assert decoded is not None
        self.assertEqual(decoded["firmware_version"], "0.6.0")

    def test_provisioning_config_matches_firmware_layout(self) -> None:
        import struct

        image = build_config_image(2, 900, 1500)
        self.assertEqual(len(image), CONFIG_SIZE)
        values = struct.unpack("<10I", image)
        self.assertEqual(
            values[:6],
            (CONFIG_MAGIC, CONFIG_LAYOUT_VERSION, 1, 2, 900000, 1500),
        )
        self.assertEqual(values[6:8], (0, CONFIG_PROVISIONING_MARKER))
        self.assertEqual(values[9], fnv1a32(image[:36]))

        parsed = parse_config_image(image)
        self.assertIsNotNone(parsed)
        assert parsed is not None
        self.assertEqual(parsed["node_id"], 2)
        self.assertEqual(parsed["report_interval_s"], 900)

    def test_provisioning_creates_two_valid_ordered_records(self) -> None:
        first, second = build_redundant_config_images(2, 900, 1500,
                                                       revision=4,
                                                       last_transaction_id=9)
        parsed_first = parse_config_image(first)
        parsed_second = parse_config_image(second)
        self.assertIsNotNone(parsed_first)
        self.assertIsNotNone(parsed_second)
        assert parsed_first is not None and parsed_second is not None
        self.assertEqual(parsed_first["generation"], 1)
        self.assertEqual(parsed_second["generation"], 2)
        self.assertEqual(parsed_second["revision"], 4)

    def test_invalid_provisioning_config_is_not_reused(self) -> None:
        image = bytearray(build_config_image(2, 900, 1500))
        image[8] ^= 0x01
        self.assertIsNone(parse_config_image(bytes(image)))

    def test_universal_hex_does_not_overlap_config_page(self) -> None:
        validate_firmware_image(
            Path(__file__).resolve().parents[2]
            / "firmware"
            / "transmitter"
            / "provisioning"
            / "MycoLogger-Transmitter-Universal.hex"
        )


class PhotoMetadataTests(unittest.TestCase):
    def test_extracts_exif_original_time_and_offset(self) -> None:
        output = BytesIO()
        exif = Image.Exif()
        exif[36867] = "2026:08:12 14:30:05"
        exif[36881] = "-04:00"
        Image.new("RGB", (8, 8), "green").save(output, "JPEG", exif=exif)

        capture = extract_capture_time(output.getvalue(), "America/New_York")
        self.assertIsNotNone(capture)
        assert capture is not None
        self.assertEqual(capture.source, "exif")
        self.assertEqual(capture.local_date, "2026-08-12")
        self.assertEqual(capture.utc, datetime(2026, 8, 12, 18, 30, 5, tzinfo=UTC))


class IngestionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_directory = tempfile.TemporaryDirectory()
        self.database_path = Path(self.temp_directory.name) / "test.sqlite3"
        initialize(self.database_path)
        self.service = ReceiverService(self.database_path)

    def tearDown(self) -> None:
        self.temp_directory.cleanup()

    def ingest(self, record: dict) -> bool:
        import json

        return self.service.process_line(
            (json.dumps(record, separators=(",", ":")) + "\n").encode()
        )

    def test_status_verifies_receiver_without_database_row(self) -> None:
        self.assertTrue(
            self.ingest(
                {
                    "v": 1,
                    "type": "status",
                    "uptime_ms": 5000,
                    "radio_state": "rx",
                }
            )
        )
        self.assertTrue(self.service.snapshot()["verified"])
        counts = get_counts(self.database_path)
        self.assertEqual(counts["nodes"], 0)
        self.assertEqual(counts["measurements"], 0)

    def test_receiver_firmware_is_cached_from_status_and_query_response(self) -> None:
        self.assertTrue(
            self.ingest(
                {
                    "v": 1,
                    "type": "status",
                    "uptime_ms": 5000,
                    "radio_state": "rx",
                    "fw": "0.6.0",
                }
            )
        )
        self.assertEqual(self.service.snapshot()["firmware_version"], "0.6.0")

        self.assertTrue(
            self.ingest(
                {
                    "v": 1,
                    "type": "hello",
                    "device": "mycologger-receiver",
                    "fw": "0.6.1",
                }
            )
        )
        self.assertEqual(self.service.snapshot()["firmware_version"], "0.6.1")

    def test_receiver_reset_flags_are_cached_from_status(self) -> None:
        self.assertTrue(
            self.ingest(
                {
                    "v": 1,
                    "type": "status",
                    "uptime_ms": 5000,
                    "radio_state": "rx",
                    "reset_flags": 0x20000000,
                }
            )
        )
        self.assertEqual(self.service.snapshot()["reset_flags"], 0x20000000)

    def test_sensor_packet_creates_node_and_measurement(self) -> None:
        self.assertTrue(self.ingest(sensor_record()))
        counts = get_counts(self.database_path)
        self.assertEqual(counts["nodes"], 1)
        self.assertEqual(counts["measurements"], 1)
        node = get_nodes(self.database_path)[0]
        self.assertEqual(node["co2_ppm"], 733)
        self.assertEqual(node["rssi_dbm"], -22)

    def test_link_check_delivers_a_queued_config_without_storing_a_measurement(self) -> None:
        self.assertTrue(self.ingest(sensor_record()))
        update_node_settings(
            self.database_path,
            1,
            name="Node 1",
            tub_name="",
            location="",
            notes="",
            active=True,
            report_interval_s=120,
        )
        commands: list[bytes] = []
        self.assertTrue(
            self.service.process_line(
                (json.dumps(link_check_record()) + "\n").encode(),
                command_writer=commands.append,
            )
        )
        self.assertEqual(len(commands), 1)
        self.assertTrue(commands[0].startswith(b"CFG 1 "))
        self.assertEqual(get_counts(self.database_path)["measurements"], 1)

    def test_provisioning_reserves_next_free_id_and_registers_uid(self) -> None:
        self.assertTrue(self.ingest(sensor_record(node_id=1)))
        uid_a = "00112233445566778899AABB"
        uid_b = "FFEEDDCCBBAA998877665544"
        status = get_provisioning_status(self.database_path, uid_a)
        self.assertEqual(status["next_available_node_id"], 2)
        reservation = reserve_node_id(
            self.database_path,
            hardware_uid=uid_a,
            reservation_token="a" * 32,
            requested_node_id=None,
            ttl_seconds=300,
        )
        self.assertEqual(reservation["node_id"], 2)
        self.assertEqual(
            get_provisioning_status(self.database_path, uid_b)[
                "next_available_node_id"
            ],
            3,
        )
        with self.assertRaisesRegex(ValueError, "already in use or reserved"):
            reserve_node_id(
                self.database_path,
                hardware_uid=uid_b,
                reservation_token="b" * 32,
                requested_node_id=2,
                ttl_seconds=300,
            )
        registered = complete_node_provisioning(
            self.database_path,
            reservation_token="a" * 32,
            hardware_uid=uid_a,
        )
        self.assertEqual(registered["node_id"], 2)
        reprovision = reserve_node_id(
            self.database_path,
            hardware_uid=uid_a,
            reservation_token="c" * 32,
            requested_node_id=None,
            ttl_seconds=300,
        )
        self.assertEqual(reprovision["node_id"], 2)
        self.assertTrue(
            cancel_node_reservation(
                self.database_path,
                reservation_token="c" * 32,
                hardware_uid=uid_a,
            )
        )

        explicit = reserve_node_id(
            self.database_path,
            hardware_uid=uid_a,
            reservation_token="d" * 32,
            requested_node_id=7,
            ttl_seconds=300,
        )
        self.assertEqual(explicit["node_id"], 7)
        renumbered = complete_node_provisioning(
            self.database_path,
            reservation_token="d" * 32,
            hardware_uid=uid_a,
        )
        self.assertEqual(renumbered["node_id"], 7)
        with self.assertRaisesRegex(ValueError, "already in use or reserved"):
            reserve_node_id(
                self.database_path,
                hardware_uid=uid_b,
                reservation_token="e" * 32,
                requested_node_id=7,
                ttl_seconds=300,
            )

    def test_battery_voltage_is_stored_and_exposed(self) -> None:
        self.assertTrue(self.ingest(sensor_record(battery_mv=3987)))
        node = get_nodes(self.database_path)[0]
        self.assertEqual(node["battery_mv"], 3987)
        self.assertEqual(node["battery_voltage_v"], 3.987)

    def test_pre_provisioner_node_can_be_adopted_only_with_flash_proof(self) -> None:
        self.assertTrue(self.ingest(sensor_record(node_id=1)))
        uid_a = "00112233445566778899AABB"
        uid_b = "FFEEDDCCBBAA998877665544"

        with self.assertRaisesRegex(ValueError, "already in use or reserved"):
            reserve_node_id(
                self.database_path,
                hardware_uid=uid_a,
                reservation_token="f" * 32,
                requested_node_id=1,
                ttl_seconds=300,
            )

        adopted = reserve_node_id(
            self.database_path,
            hardware_uid=uid_a,
            reservation_token="1" * 32,
            requested_node_id=1,
            ttl_seconds=300,
            claim_existing_node=True,
        )
        self.assertEqual(adopted["node_id"], 1)
        complete_node_provisioning(
            self.database_path,
            reservation_token="1" * 32,
            hardware_uid=uid_a,
        )

        with self.assertRaisesRegex(ValueError, "already in use or reserved"):
            reserve_node_id(
                self.database_path,
                hardware_uid=uid_b,
                reservation_token="2" * 32,
                requested_node_id=1,
                ttl_seconds=300,
                claim_existing_node=True,
            )

    def test_transmitter_firmware_is_cached_on_node(self) -> None:
        self.assertTrue(
            self.ingest(sensor_record(firmware_version=(0, 6, 0)))
        )
        node = get_nodes(self.database_path)[0]
        self.assertEqual(node["firmware_version"], "0.6.0")
        self.assertIsNotNone(node["firmware_updated_utc"])

    def test_transmitter_diagnostics_are_stored_and_exposed(self) -> None:
        self.assertTrue(
            self.ingest(
                sensor_record(
                    firmware_version=(0, 8, 0),
                    reset_flags=0x20000000,
                    sensor_failure_count=3,
                    radio_failure_count=5,
                )
            )
        )
        node = get_nodes(self.database_path)[0]
        self.assertEqual(node["last_reset_flags"], 0x20000000)
        self.assertEqual(node["sensor_failure_count"], 3)
        self.assertEqual(node["radio_failure_count"], 5)

    def test_uplink_restores_device_config_after_database_replacement(self) -> None:
        self.assertTrue(
            self.ingest(sensor_record(config_revision=7, report_interval_s=900))
        )
        node = get_node(self.database_path, 1)
        assert node is not None
        self.assertEqual(node["config_revision"], 7)
        self.assertEqual(node["applied_config_revision"], 7)
        self.assertEqual(node["desired_report_interval_s"], 900)
        self.assertEqual(node["applied_report_interval_s"], 900)

    def test_duplicate_sensor_packet_is_not_stored_twice(self) -> None:
        record = sensor_record()
        self.assertTrue(self.ingest(record))
        self.assertTrue(self.ingest(record))
        self.assertEqual(get_counts(self.database_path)["measurements"], 1)
        self.assertEqual(self.service.snapshot()["duplicates_ignored"], 1)

    def test_sequence_restart_after_reboot_uses_new_boot_session(self) -> None:
        self.assertTrue(self.ingest(sensor_record(tx_sequence=90, uptime_s=900)))
        self.assertTrue(self.ingest(sensor_record(tx_sequence=1, uptime_s=10)))
        self.assertEqual(get_counts(self.database_path)["measurements"], 2)
        self.assertEqual(get_nodes(self.database_path)[0]["boot_session"], 2)

    def test_invalid_json_is_counted(self) -> None:
        self.assertFalse(self.service.process_line(b"not json\n"))
        self.assertEqual(self.service.snapshot()["invalid_records"], 1)

    def test_metadata_and_tub_assignment_are_saved(self) -> None:
        self.assertTrue(self.ingest(sensor_record()))
        node = update_node_settings(
            self.database_path,
            1,
            name="Fruiting monitor",
            tub_name="Tub A",
            location="Shelf 2",
            notes="Test assignment",
            active=True,
            report_interval_s=60,
        )
        self.assertEqual(node["name"], "Fruiting monitor")
        self.assertEqual(node["tub_name"], "Tub A")
        self.assertEqual(node["location"], "Shelf 2")
        self.assertEqual(node["command_status"], "applied")

    def test_current_grow_details_history_and_photos(self) -> None:
        self.assertTrue(self.ingest(sensor_record(tx_sequence=1)))
        assigned = update_node_settings(
            self.database_path,
            1,
            name="Fruiting monitor",
            tub_name="Tub A",
            location="Shelf 2",
            notes="",
            active=True,
            report_interval_s=60,
        )
        self.assertTrue(self.ingest(sensor_record(tx_sequence=2, co2_ppm=812)))
        tub_id = int(assigned["tub_id"])
        updated = update_grow(
            self.database_path,
            tub_id,
            name="Tub A",
            species="Pleurotus ostreatus",
            strain="Blue Oyster",
            stage="fruiting",
            spawn_to_bulk_on="2026-08-01",
            completed_on=None,
            pin_dates=["2026-08-09", "2026-08-08"],
            notes="First flush",
            active=True,
        )
        self.assertEqual(updated["title"], "Blue Oyster")
        self.assertEqual(updated["pin_dates"], ["2026-08-08", "2026-08-09"])
        self.assertEqual(updated["history"][-1]["co2_ppm"], 812.0)

        grows = get_current_grows(self.database_path)
        self.assertEqual(len(grows), 1)
        self.assertEqual(grows[0]["node_id"], 1)
        self.assertEqual(grows[0]["stage"], "fruiting")

        photo = add_grow_photo(
            self.database_path,
            tub_id,
            stored_name="test.jpg",
            original_name="pins.jpg",
            media_type="image/jpeg",
            size_bytes=128,
            taken_on="2026-08-12",
            taken_at_utc=datetime.now(UTC).isoformat(),
            capture_time_source="exif",
        )
        self.assertEqual(photo["capture_time_source"], "exif")
        self.assertEqual(photo["condition_co2_ppm"], 812)
        self.assertEqual(photo["condition_node_id"], 1)
        detailed = get_grow(self.database_path, tub_id)
        assert detailed is not None
        self.assertEqual(detailed["photos"][0]["original_name"], "pins.jpg")
        removed = delete_grow_photo(self.database_path, tub_id, photo["photo_id"])
        self.assertIsNotNone(removed)

    def test_grow_archive_classification_releases_node_and_preserves_history(self) -> None:
        self.assertTrue(self.ingest(sensor_record(tx_sequence=1)))
        assigned = update_node_settings(
            self.database_path,
            1,
            name="Node 1",
            tub_name="Failed tub",
            location="",
            notes="",
            active=True,
            report_interval_s=60,
        )
        self.assertTrue(self.ingest(sensor_record(tx_sequence=2, co2_ppm=900)))
        tub_id = int(assigned["tub_id"])
        archived = archive_grow(
            self.database_path,
            tub_id,
            contaminated=True,
            first_flush_harvested=False,
            occurred_on="2026-08-21",
            reason="Trich before pins",
        )
        self.assertEqual(archived["archive_category"], "failed_grow")
        self.assertEqual(archived["history"][-1]["co2_ppm"], 900.0)
        self.assertEqual(get_current_grows(self.database_path), [])
        failed = get_archived_grows(
            self.database_path, category="failed_grow"
        )
        self.assertEqual([grow["tub_id"] for grow in failed], [tub_id])
        node = get_node(self.database_path, 1)
        assert node is not None
        self.assertIsNone(node["tub_id"])
        self.assertEqual(delete_grow(self.database_path, tub_id), [])
        self.assertIsNone(get_grow(self.database_path, tub_id))
        self.assertEqual(get_counts(self.database_path)["measurements"], 2)

    def test_jar_archive_failure_and_permanent_deletion(self) -> None:
        common = dict(
            grain_type="millet",
            prep_tek="",
            pressure_cooker_minutes=120,
            pressure_psi=15,
            dry_grain_grams_per_jar=300,
            jar_count=1,
            pressure_cooked_on="2026-08-12",
            inoculated_on="2026-08-13",
            culture="GT",
            species="Psilocybe cubensis",
            break_shake_dates=[],
            notes="",
        )
        archived_jar = create_spawn_jar(
            self.database_path, name="Clean extra", **common
        )
        failed_jar = create_spawn_jar(
            self.database_path, name="Contaminated jar", **common
        )
        archived_jar = archive_spawn_jar(
            self.database_path,
            archived_jar["jar_id"],
            contaminated=False,
            occurred_on="2026-08-20",
            reason="No longer needed",
        )
        failed_jar = archive_spawn_jar(
            self.database_path,
            failed_jar["jar_id"],
            contaminated=True,
            occurred_on="2026-08-21",
            reason="Green growth",
        )
        self.assertEqual(archived_jar["archive_category"], "archived_jar")
        self.assertEqual(failed_jar["archive_category"], "failed_jar")
        self.assertEqual(len(get_spawn_jars(self.database_path, status="archived")), 1)
        self.assertEqual(len(get_spawn_jars(self.database_path, status="failed")), 1)
        self.assertEqual(delete_spawn_jar(
            self.database_path, failed_jar["jar_id"]
        ), [])
        self.assertIsNone(get_spawn_jar(self.database_path, failed_jar["jar_id"]))

    def test_spawn_jar_rolls_into_locked_sensor_optional_tub(self) -> None:
        jar = create_spawn_jar(
            self.database_path,
            name="Millet batch A",
            grain_type="millet",
            prep_tek="No soak, no simmer",
            pressure_cooker_minutes=120,
            pressure_psi=15,
            dry_grain_grams_per_jar=300,
            jar_count=4,
            pressure_cooked_on="2026-08-10",
            inoculated_on="2026-08-11",
            culture="Golden Teacher LC",
            species="Psilocybe cubensis",
            break_shake_dates=["2026-08-15", "2026-08-18"],
            notes="Clean recovery",
        )
        photo = add_spawn_jar_photo(
            self.database_path,
            jar["jar_id"],
            stored_name="jar-test.jpg",
            original_name="jar.jpg",
            media_type="image/jpeg",
            size_bytes=128,
            caption="Before shake",
            taken_on="2026-08-15",
            taken_at_utc="2026-08-15T12:00:00+00:00",
            capture_time_source="exif",
        )
        grow, archived = spawn_jar_to_tub(
            self.database_path,
            jar["jar_id"],
            tub_name="Golden Teacher tub",
            node_id=None,
            species="Psilocybe cubensis",
            strain="Golden Teacher",
            spawn_to_bulk_on="2026-08-20",
            notes="CVG",
        )
        self.assertEqual(get_spawn_jars(self.database_path), [])
        self.assertIsNone(grow["node_id"])
        self.assertTrue(archived["locked"])
        self.assertEqual(grow["spawn_jar"]["photos"][0]["photo_id"], photo["photo_id"])
        with self.assertRaises(PermissionError):
            update_spawn_jar(
                self.database_path,
                jar["jar_id"],
                name="Changed",
                grain_type="",
                prep_tek="",
                pressure_cooker_minutes=None,
                pressure_psi=None,
                dry_grain_grams_per_jar=None,
                jar_count=1,
                pressure_cooked_on=None,
                inoculated_on=None,
                culture="",
                species="",
                break_shake_dates=[],
                notes="",
            )
        unlocked = set_spawn_jar_locked(
            self.database_path, jar["jar_id"], locked=False
        )
        self.assertFalse(unlocked["locked"])

    def test_bulk_created_jars_are_individual_and_can_share_one_tub(self) -> None:
        jars = create_spawn_jars(
            self.database_path,
            quantity=3,
            name="Uninoculated millet",
            grain_type="millet",
            prep_tek="No soak, no simmer",
            pressure_cooker_minutes=120,
            pressure_psi=15,
            dry_grain_grams_per_jar=300,
            pressure_cooked_on="2026-08-12",
            inoculated_on=None,
            culture="Golden Teacher",
            species="Psilocybe cubensis",
            break_shake_dates=[],
            notes="",
        )
        self.assertEqual(
            [jar["name"] for jar in jars],
            [
                "Uninoculated millet #1",
                "Uninoculated millet #2",
                "Uninoculated millet #3",
            ],
        )
        self.assertTrue(all(jar["jar_count"] == 1 for jar in jars))
        grow, spawned = spawn_jars_to_tub(
            self.database_path,
            [jars[0]["jar_id"], jars[2]["jar_id"]],
            tub_name="Two jar tub",
            node_id=None,
            species="",
            strain="",
            spawn_to_bulk_on="2026-08-20",
            notes="",
        )
        self.assertEqual(len(spawned), 2)
        self.assertEqual(len(grow["spawn_jars"]), 2)
        self.assertTrue(all(jar["locked"] for jar in grow["spawn_jars"]))
        current = get_spawn_jars(self.database_path)
        self.assertEqual([jar["jar_id"] for jar in current], [jars[1]["jar_id"]])

    def test_mixed_cultures_require_separate_inherited_tubs(self) -> None:
        common = dict(
            grain_type="millet",
            prep_tek="",
            pressure_cooker_minutes=120,
            pressure_psi=15,
            dry_grain_grams_per_jar=300,
            jar_count=1,
            pressure_cooked_on="2026-08-12",
            inoculated_on="2026-08-13",
            species="Psilocybe cubensis",
            break_shake_dates=[],
            notes="",
        )
        ape = create_spawn_jar(
            self.database_path, name="APE jar", culture="APE", **common
        )
        gt = create_spawn_jar(
            self.database_path, name="GT jar", culture="GT", **common
        )
        with self.assertRaisesRegex(ValueError, "same non-empty culture"):
            spawn_jars_to_tub(
                self.database_path,
                [ape["jar_id"], gt["jar_id"]],
                tub_name="Invalid mixed tub",
                node_id=None,
                species="ignored",
                strain="ignored",
                spawn_to_bulk_on="2026-08-20",
                spawn_ratio="1:2",
                notes="",
            )
        grows = spawn_jar_groups_to_tubs(
            self.database_path,
            [
                {
                    "jar_ids": [ape["jar_id"]],
                    "tub_name": "APE tub",
                    "node_id": None,
                    "spawn_ratio": "1:2",
                    "spawn_to_bulk_on": "2026-08-20",
                    "notes": "",
                },
                {
                    "jar_ids": [gt["jar_id"]],
                    "tub_name": "GT tub",
                    "node_id": None,
                    "spawn_ratio": "1:3",
                    "spawn_to_bulk_on": "2026-08-20",
                    "notes": "",
                },
            ],
        )
        self.assertEqual([grow["strain"] for grow in grows], ["APE", "GT"])
        self.assertEqual(
            [grow["species"] for grow in grows],
            ["Psilocybe cubensis", "Psilocybe cubensis"],
        )
        self.assertEqual([grow["spawn_ratio"] for grow in grows], ["1:2", "1:3"])

    def test_report_interval_command_is_written_and_acknowledged(self) -> None:
        self.assertTrue(self.ingest(sensor_record()))
        queued = update_node_settings(
            self.database_path,
            1,
            name="Node 1",
            tub_name="",
            location="",
            notes="",
            active=True,
            report_interval_s=300,
        )
        transaction_id = queued["pending_transaction_id"]
        output: list[bytes] = []
        import json

        next_packet = sensor_record(tx_sequence=13, uptime_s=120)
        raw = (json.dumps(next_packet) + "\n").encode()
        self.assertTrue(self.service.process_line(raw, command_writer=output.append))
        self.assertEqual(
            output,
            [f"CFG 1 {transaction_id} 1 300\n".encode()],
        )
        sent = get_node(self.database_path, 1)
        assert sent is not None
        self.assertEqual(sent["command_status"], "sent")

        self.assertTrue(
            self.ingest(
                config_ack_record(
                    node_id=1,
                    transaction_id=transaction_id,
                    revision=1,
                    interval_s=300,
                )
            )
        )
        applied = get_node(self.database_path, 1)
        assert applied is not None
        self.assertEqual(applied["command_status"], "applied")
        self.assertEqual(applied["applied_report_interval_s"], 300)

    def test_next_uplink_confirms_config_when_ack_was_missed(self) -> None:
        self.assertTrue(
            self.ingest(sensor_record(config_revision=0, report_interval_s=60))
        )
        queued = update_node_settings(
            self.database_path,
            1,
            name="Node 1",
            tub_name="",
            location="",
            notes="",
            active=True,
            report_interval_s=300,
        )
        transaction_id = queued["pending_transaction_id"]
        output: list[bytes] = []
        import json

        old_config_packet = sensor_record(
            tx_sequence=13,
            uptime_s=120,
            config_revision=0,
            report_interval_s=60,
        )
        self.assertTrue(
            self.service.process_line(
                (json.dumps(old_config_packet) + "\n").encode(),
                command_writer=output.append,
            )
        )
        self.assertEqual(output, [f"CFG 1 {transaction_id} 1 300\n".encode()])

        new_config_packet = sensor_record(
            tx_sequence=14,
            uptime_s=180,
            config_revision=1,
            report_interval_s=300,
        )
        self.assertTrue(self.ingest(new_config_packet))
        applied = get_node(self.database_path, 1)
        assert applied is not None
        self.assertEqual(applied["command_status"], "applied")
        self.assertEqual(applied["applied_config_revision"], 1)
        self.assertEqual(applied["applied_report_interval_s"], 300)


if __name__ == "__main__":
    unittest.main()
