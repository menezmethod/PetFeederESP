# PetFeederESP

Wi-Fi and Bluetooth pet feeder firmware for the ESP32. Feed on a schedule, feed
on demand from a phone, and provision Wi-Fi over BLE without ever opening a
serial monitor. Pairs with the [pet_feeder_esp_ui](https://github.com/menezmethod/pet_feeder_esp_ui)
Flutter app.

Based on the [FAT PET FEEDER](https://www.youtube.com/watch?v=sCOkuyH7CPo) by
[Mom Will Be Proud • DIY channel](https://www.youtube.com/channel/UCqVfFr35soUMdDQoXc3hoOw).

## Features

- **BLE Wi-Fi provisioning** — connect from the app, pick a network from a live scan (or enter one manually), send credentials. No serial cable, no manual BLE terminal commands.
- **Up to 6 feeding schedules**, each with its own time and day-of-week mask (every day / weekdays / weekends / custom).
- **Manual feeding** from the app or the physical button, with a hard ceiling on dispense duration regardless of what's requested.
- **MQTT over TLS**, authenticated (username/password), against a self-hosted broker — not a public/shared one.
- **OTA updates** — checks a `version.json` published with each GitHub Release and flashes over MQTT command, no cable required after the first flash.
- **NTP time sync** with a bounded timeout, so an unreachable time server can't hang boot.

## Hardware

**Electronics (this repo's scope):**

| Part | Notes |
|---|---|
| ESP32-WROOM-32D dev board | Any ESP32 dev board with the same pinout works |
| SpringRC SM-S4303R continuous rotation servo | Signal on GPIO 23 |
| N-channel MOSFET or small signal relay | Gates power to the servo, driven from GPIO 22 — the ESP32 can't source the servo's current directly, and cutting power when idle avoids jitter |
| Momentary push button | GPIO 18 to GND — uses the ESP32's internal pull-up, no external resistor needed |
| 5V power supply, ~1A+ | Sized for the servo's stall current, not just its running current |

**Mechanical (not in this repo):** the hopper, auger/dispensing mechanism, and enclosure
follow the [FAT PET FEEDER](https://www.youtube.com/watch?v=sCOkuyH7CPo) design this
project is based on — see that video for the mechanical build. If you've built the
mechanical side, a PR adding photos or a parts list here would help the next person a lot
(see `CONTRIBUTING.md`).

## Firmware setup

1. Install [PlatformIO](https://platformio.org/) (CLI or the VSCode extension).
2. Copy the secrets template and fill in your own broker credentials:
   ```bash
   cp src/config_secrets.h.example src/config_secrets.h
   ```
   `config_secrets.h` is gitignored — never commit real credentials.
3. Point `MQTT_BROKER_URI` / `MQTT_PORT` in `src/config.h` at your own broker, and replace `src/mqtt_ca_cert.h` with your broker's CA certificate (public half only).
4. Build and flash:
   ```bash
   pio run --target upload
   ```
   The partition table (`min_spiffs.csv`) reserves dual OTA app slots — don't switch to a scheme without a second OTA slot, or OTA updates stop working.

## First-time Wi-Fi setup

1. Power on the ESP32 — it advertises over BLE as `ESP_FEEDER` while unprovisioned.
2. Open the companion app, connect over Bluetooth, and pick a Wi-Fi network from the live scan (or type one in).
3. The feeder saves the credentials to flash and connects. BLE advertising stops once Wi-Fi is up, and reopens automatically if the connection ever drops for more than 30 seconds — so re-provisioning after a router change doesn't need a factory reset.

## Publishing an OTA release

1. Bump `FIRMWARE_VERSION` in `src/config.h` and build (`pio run`).
2. Create a GitHub Release with `.pio/build/esp32dev/firmware.bin` attached, plus a `version.json`:
   ```json
   { "version": "1.2.0", "url": "https://github.com/menezmethod/PetFeederESP/releases/download/v1.2.0/firmware.bin" }
   ```
3. Devices already in the field pick it up next time an OTA check is triggered over MQTT.

## Security notes

- MQTT is TLS-encrypted and credential-authenticated against a broker you control, not a public one.
- The OTA download intentionally does not pin GitHub's TLS certificate (`setInsecure()`) — GitHub Releases redirects across hosts with different CA chains, and reaching this code path already requires valid MQTT credentials. See the comment in `ota_manager.cpp` for the full trade-off.
- BLE provisioning has no pairing/bonding — anyone with physical BLE range while the feeder is advertising can read/write Wi-Fi credentials. Advertising is only active while unprovisioned or briefly after a connection drop, not continuously.
- Wi-Fi and MQTT credentials are stored in flash (NVS) unencrypted at rest, as ESP32 flash encryption is not enabled. Acceptable for a device that isn't expected to leave your house; if that changes, enable flash encryption and secure boot.

## Contributing

Contributions are welcome — see `CONTRIBUTING.md` for what's most useful right now
(hardware photos, in particular: none exist in this repo yet).

## Acknowledgments

Built on the [FAT PET FEEDER](https://www.youtube.com/watch?v=sCOkuyH7CPo) by
[Mom Will Be Proud • DIY channel](https://www.youtube.com/channel/UCqVfFr35soUMdDQoXc3hoOw).

## License

MIT — see `LICENSE`.
