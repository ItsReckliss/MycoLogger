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

- [ ] Correct the SCD41 measurement strategy.
  - [ ] Discard the first single-shot CO2 result after physically powering the
        sensor on, as required for stabilization.
  - [ ] Use idle single-shot mode for shorter report intervals where it is more
        efficient than power cycling.
  - [ ] Use power-cycled single-shot mode only for sufficiently long intervals.
  - [ ] Decide how SCD41 calibration will work inside tubs; do not assume that
        automatic self-calibration will regularly see fresh outdoor air.
  - [ ] Add configurable temperature compensation/offset.
- [ ] Add an independent watchdog to both transmitter and receiver firmware.
- [ ] Report transmitter boot count, reset cause, firmware version, sensor
      failure count, and radio failure count.
- [ ] Make persistent transmitter configuration resilient to power loss.
  - [ ] Use two flash records/pages with generation counters and checksums.
  - [ ] Write and verify a new record before retiring the previous record.
  - [ ] Test loss of power during configuration updates.
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
- [ ] Add firmware version, reset cause, link quality, packet delivery, and
      battery trend to each node detail view.
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

- [ ] Export the complete editable EasyEDA transmitter project into
      `hardware/transmitter/easyeda/`.
- [ ] Export the complete editable EasyEDA receiver project into
      `hardware/receiver/easyeda/`.
- [ ] Add revision-matched Gerbers, BOMs, pick-and-place files, PDF schematics,
      board renders, and fabrication notes for both existing boards.
- [ ] Review measured regulator and load-switch quiescent current against the
      intended battery-life target.
- [ ] Add accessible current-measurement points or a removable power jumper.
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

- [x] ~~Transmitter reads SCD41 temperature, humidity, and CO2.~~
- [x] ~~Transmitter reports battery voltage.~~
- [x] ~~Receiver exposes packets through cross-platform USB CDC/ACM serial.~~
- [x] ~~Receiver service automatically discovers, reconnects, and stores readings.~~
- [x] ~~Web dashboard shows nodes, tubs, jars, graphs, notes, and photos with captured conditions.~~
- [x] ~~Report interval can be changed through the receiver and acknowledged by the transmitter.~~
- [x] ~~Universal firmware and the ST-LINK provisioner assign permanent node IDs using MCU UIDs and server reservations.~~
- [x] ~~The transmitter debug button requests a fresh reading and identifies the node.~~
- [x] ~~Transmitter and receiver implement a bounded power-on link confirmation.~~
- [x] ~~Automatic transmissions do not unnecessarily illuminate the transmitter debug LED.~~
