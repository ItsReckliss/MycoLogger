# MycoLogger To-Do

This is the working roadmap for MycoLogger and should be reviewed after every
completed unit of work.

Maintenance rules:

- Add newly discovered work as soon as it becomes concrete.
- Mark completed and verified work with `[x]` and strikethrough.
- Keep completed entries when they provide useful project history; remove them
  only when they no longer add value.
- Update parent and child checkboxes together when an entire group is complete.
- Review this file before creating each local Git commit.

## Priority 1 - Correctness and reliability

- [x] ~~Finish validating corrected transmitter battery-voltage reporting.~~
  - [x] ~~Verify the external divider: PA0 measures 1.8427 V, corresponding to
        approximately 3.6854 V at the battery.~~
  - [x] ~~Correct the STM32U031 external ADC selection to PA0/ADC1_IN4 and use
        the verified STM32U031F6 internal VREFINT channel 12.~~
  - [x] ~~Convert the divider and VREFINT separately so each EOC corresponds to
        a fresh data-register result.~~
  - [x] ~~Build and flash transmitter v0.6.3 to Node 1.~~
  - [x] ~~Verify the live dashboard reports 3.759 V, consistent with the
        1.8427 V divider measurement and approximately 3.6854 V battery.~~
  - [x] ~~Calibrate the final result by 0.9992 against a simultaneous 3.752 V
        battery-lead measurement in transmitter v0.6.5.~~
- [ ] Correct the SCD41 measurement strategy.
  - [x] ~~Bench-compare the first and immediate second single-shot results
        after one load-switch power cycle.~~ Node 1 measured 1847 ppm followed
        by 1864 ppm (+17 ppm) five seconds later; keep collecting comparisons.
  - [ ] Retain the option to add configurable `discard_first_single_shot`
        behavior if later environmental testing justifies it.
  - [x] ~~Add remotely configurable downlink receive-window duration.~~
  - [ ] Tune the downlink receive window to the minimum reliable value on the
        physical receiver/server path.
  - [x] ~~Disable and persist SCD41 automatic self-calibration for the
        power-cycled, high-CO2 tub deployment.~~
  - [ ] Use idle single-shot mode for shorter report intervals where it is more
        efficient than power cycling.
  - [ ] Use power-cycled single-shot mode only for sufficiently long intervals.
  - [ ] Decide how SCD41 calibration will work inside tubs; do not assume that
        automatic self-calibration will regularly see fresh outdoor air.
  - [ ] Add configurable temperature compensation/offset.
- [x] Add an independent watchdog to both transmitter and receiver firmware.
  - [x] ~~Add and bench-test an approximately 8-second IWDG in the transmitter.~~
    - [x] ~~Implement and build transmitter v0.7.0, refreshed only by successful
          main-loop progress.~~
    - [x] ~~Reconnect the ST-Link, flash Node 1, and deliberately verify IWDGRSTF.~~
  - [x] Add and bench-test an independent watchdog in the receiver firmware.
    - [x] ~~Implement receiver v0.7.0 watchdog code and expose raw reset flags
          in status JSON.~~
    - [x] Build, flash the physical receiver, and deliberately verify an IWDG
          reset.
- [ ] Report transmitter diagnostics.
  - [x] ~~Report and cache transmitter firmware version.~~
  - [x] ~~Capture the RCC reset-cause flags immediately at boot.~~
  - [x] ~~Report and cache reset cause over the radio.~~
  - [ ] Add a persistent transmitter boot count.
  - [x] ~~Report and cache sensor and radio operation-failure counts.~~
- [x] ~~Make persistent transmitter configuration resilient to power loss.~~
  - [x] ~~Use two flash records/pages with generation counters and checksums.~~
  - [x] ~~Write and verify a new record before retiring the previous record.~~
  - [x] ~~Test loss of power during configuration updates by erasing the older
        page on Node 1, resetting it, and confirming it reported normally from
        the surviving newer page; restore and verify both records afterward.~~
- [ ] Perform extended bench testing of the boot link-check exchange, remote
      configuration, retries, duplicate packets, and receiver disconnects.

## Priority 2 - Battery life

- [ ] Measure a complete transmitter energy budget with a power profiler.
  - [ ] Idle/sleep current.
  - [ ] SCD41 startup and conversion energy.
  - [ ] LoRa transmit energy.
  - [ ] LoRa receive-window and boot-check energy.
  - [ ] LDO, load-switch, LED, and resistor-divider losses.
- [ ] Put the SX1262 into sleep mode between radio operations.
- [ ] Replace the current SysTick-driven `__WFI()` loop with STM32 Stop 2.
  - [ ] Wake from an RTC or low-power timer for scheduled reports.
  - [ ] Wake from the debug button interrupt.
  - [ ] Disable SysTick while sleeping and restore clocks correctly on wake.
- [ ] Recalculate expected battery life from measured energy per report instead
      of component typical values alone.
- [ ] Add configurable low-battery and critical-battery thresholds.
- [ ] Consider adaptive reporting at low battery.
- [ ] For the next PCB revision, switch the high-value battery divider so it
      draws current only while the ADC is measuring.

## Priority 3 - Radio robustness and scale

- [ ] Add deterministic per-node timing jitter so nodes with identical report
      intervals do not remain synchronized and collide repeatedly.
- [ ] Track packet delivery statistics using node ID, boot session, sequence,
      and uptime.
  - [ ] Delivery percentage.
  - [ ] Missed-packet count.
  - [ ] Consecutive misses.
  - [ ] RSSI and SNR history.
  - [ ] Last successful bidirectional contact.
- [ ] Run indoor and outdoor range tests with final enclosures and antennas.
- [ ] Choose production LoRa transmit power and verify regulatory EIRP limits.
- [ ] Add authenticated configuration downlinks and replay protection before
      treating radio configuration as secure in an untrusted environment.
- [ ] Define protocol/firmware compatibility rules and an upgrade path for
      future packet versions.

## Priority 4 - Dashboard and operations

- [ ] Add alert rules and a dashboard alert view.
  - [ ] Node offline.
  - [ ] Low or rapidly falling battery.
  - [ ] Repeated sensor failures.
  - [ ] Configuration stuck in queued/sent state.
  - [ ] Excessive packet loss.
  - [ ] Temperature, humidity, or CO2 outside user-defined limits.
- [ ] Expand node detail diagnostics.
  - [x] ~~Show cached transmitter firmware version.~~
  - [ ] Add reset cause, link quality, packet delivery, and battery trend.
- [ ] Add calibration history and maintenance notes for each sensor node.
- [ ] Add CSV/JSON export for measurements and grow records.
- [ ] Add database retention/downsampling rules if long-term raw data growth
      becomes significant.
- [ ] Add user authentication and protect provisioning/configuration endpoints,
      even when primarily accessed through Tailscale.
- [ ] Add clear UI warnings when the receiver firmware and node protocol
      versions are incompatible.

## Priority 5 - Pi deployment and data safety

- [ ] Finish deploying the current server and receiver service to the Pi.
- [ ] Run the server as a managed service with automatic restart and logs.
- [ ] Add automated backups for both SQLite and uploaded photos.
  - [ ] Store backups outside the Git checkout.
  - [ ] Copy backups to another machine or encrypted remote destination.
  - [ ] Keep multiple dated generations.
  - [ ] Test a complete restore periodically.
- [ ] Add health monitoring for disk space, database integrity, receiver USB
      connection, and service uptime.
- [ ] Document the Windows-to-Pi development, test, deploy, and rollback flow.
- [x] ~~Connect the repository to the intended GitHub remote.~~
- [ ] Establish a predictable release and tagging process.

## Hardware revision ideas

- [ ] Develop printable enclosures for both current devices.
  - [ ] Measure the transmitter PCB, battery, antenna, switch, debug button,
        programming pads, and SCD41 airflow/clearance requirements.
  - [ ] Design and prototype a transmitter enclosure with sensor ventilation,
        antenna clearance, switch/button access, battery retention, a practical
        tub-mounting method, and external access to the next-revision SWD
        programming connector beside the charging USB-C port.
  - [ ] Measure the receiver PCB, antenna, USB connector, buttons, LEDs, and
        programming/bootloader access requirements.
  - [ ] Design and prototype a receiver enclosure with USB access, visible LEDs,
        usable buttons, antenna clearance, ventilation, and stable mounting.
  - [ ] Print and test fit both enclosures, revise tolerances, and verify that
        neither enclosure significantly degrades LoRa range or sensor response.
  - [ ] Commit editable CAD sources, revisioned STL/3MF exports, print settings,
        hardware requirements, assembly notes, and photos under `hardware/`.
- [ ] Export the complete editable EasyEDA transmitter project into
      `hardware/transmitter/easyeda/`.
- [ ] Export the complete editable EasyEDA receiver project into
      `hardware/receiver/easyeda/`.
- [ ] Add revision-matched Gerbers, BOMs, pick-and-place files, PDF schematics,
      board renders, and fabrication notes for both existing boards.
- [ ] Review measured regulator and load-switch quiescent current against the
      intended battery-life target.
- [ ] Add accessible current-measurement points or a removable power jumper.
- [ ] Add a small keyed SMD JST-style SWD programming connector to the next
      transmitter revision.
  - [ ] Place it near the USB-C charging connector so charging and programming
        use one accessible enclosure area.
  - [ ] Route SWDIO, SWCLK, NRST, GND, and VTREF/3.3 V sensing; do not use the
        connector to power the transmitter unless that behavior is explicitly
        designed and protected.
  - [ ] Choose a readily sourced locking/keyed series and document its exact
        pinout, mating housing, contacts, and ST-LINK adapter cable.
  - [ ] Orient and label the connector to prevent a reversed programming cable,
        and verify assembly clearance and repeated mating durability.
  - [ ] Ensure the final transmitter enclosure exposes the connector without
        opening the case while protecting it from accidental shorts and debris.
- [ ] Add ESD protection and review environmental protection for sensor and
      external-access points.
- [ ] Review antenna placement, enclosure detuning, ground clearance, and RF
      keep-outs using the final physical enclosure.
- [ ] Consider a hardware-controlled battery divider for the next transmitter.
- [ ] Consider a fuel gauge only if voltage-based state-of-charge proves too
      inaccurate for the selected battery chemistry and load profile.

## Future "Pro" receiver

- [ ] Explore a four-sector receiver using four SX1262 radios and directional
      antennas for simultaneous 360-degree spatial diversity.
- [ ] Select a larger MCU with enough SPI/GPIO/DMA resources for four radios.
- [ ] Share SPI data/clock lines while providing separate NSS, BUSY, DIO1, and
      reset control for each radio.
- [ ] Deduplicate packets received by multiple sectors and retain per-sector
      RSSI/SNR measurements.
- [ ] Send downlinks through the strongest sector while ensuring only one radio
      transmits at a time.
- [ ] Study antenna isolation, sector overlap, enclosure layout, and legal EIRP.
- [ ] Compare the four-radio design against a simpler elevated omnidirectional
      receiver and a switched-antenna design before committing hardware.

## Completed foundation

- [x] ~~Add a guarded, double-clickable GitHub push helper for accumulated local commits.~~
- [x] ~~Add a root project overview and continuation handoff in `README.md`.~~
- [x] ~~Transmitter reads SCD41 temperature, humidity, and CO2.~~
- [x] ~~Transmitter reports battery voltage.~~
- [x] ~~Receiver exposes packets through cross-platform USB CDC/ACM serial.~~
- [x] ~~Receiver service automatically discovers, reconnects, and stores readings.~~
- [x] ~~Web dashboard shows nodes, tubs, jars, graphs, notes, and photos with captured conditions.~~
- [x] ~~Separate tub graph metrics into readable lanes with dotted average guides.~~
- [x] ~~Add jar/tub archive, contamination classification, sensor release, and guarded permanent deletion.~~
- [x] ~~Report interval can be changed through the receiver and acknowledged by the transmitter.~~
- [x] ~~Universal firmware and the ST-LINK provisioner assign permanent node IDs using MCU UIDs and server reservations.~~
- [x] ~~Flash utility preserves known UID assignments, supports explicit safe renumbering, and verifies the reserved configuration.~~
- [x] ~~Receiver and transmitter firmware versions are queryable/reported, cached, and displayed by the server.~~
- [x] ~~The transmitter debug button requests a fresh reading and identifies the node.~~
- [x] ~~Transmitter and receiver implement a bounded power-on link confirmation.~~
- [x] ~~Automatic transmissions do not unnecessarily illuminate the transmitter debug LED.~~
