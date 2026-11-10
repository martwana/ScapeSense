# ScapeSense — Agent Notes

## Build & Test

```bash
# Build the React/Tailwind dashboard into data/
cd ui
npm install
npm test
npm run test:coverage   # keep src/lib coverage above 90%
npm run build
cd ..

# Build the LittleFS image without uploading
~/.platformio/penv/bin/pio run -e freenove-esp32-s3 -t buildfs

# Build firmware for the Freenove ESP32-S3 N16R8 target
~/.platformio/penv/bin/pio run -e freenove-esp32-s3

# Run host-side unit tests
~/.platformio/penv/bin/pio test -e native
```

The HX711 driver regression test uses a fake HX711 and runs independently of
PlatformIO (including tare/filter reset, overlapping requests and timeouts):

```bash
c++ -std=c++17 -Wall -Wextra -I test/host_load_cell_sensor -I include test/host_load_cell_sensor/main.cpp src/LoadCellSensor.cpp -o /tmp/scapesense-load-cell-test
/tmp/scapesense-load-cell-test
```

The configuration persistence test uses an in-memory Preferences fake. It tests
saved temperature limits, invalid/corrupt values, write failures and factory-reset
storage clearing without touching a device:

```bash
c++ -std=c++17 -Wall -Wextra -I test/host_config_manager -I include test/host_config_manager/main.cpp src/ConfigManager.cpp -o /tmp/scapesense-config-test
/tmp/scapesense-config-test
```

## Flashing

Connect the Freenove ESP32-S3 board via USB. Firmware and filesystem uploads are
separate:

```bash
# Application firmware
~/.platformio/penv/bin/pio run -e freenove-esp32-s3 -t upload

# React dashboard assets
~/.platformio/penv/bin/pio run -e freenove-esp32-s3 -t uploadfs
```

Run `npm run build` before `uploadfs` whenever the UI source changes.

## Product Identity

- Product/UI name: ScapeSense.
- Dashboard: `http://scapesense.local` (React app; `/dashboard` also works).
- Remote serial terminal: `http://scapesense.local/webserial`.
- Firmware updates: `http://scapesense.local/update`.
- Provisioning AP: `ScapeSense-XXXX`, password `scapesense`; the NetWizard
  captive portal is at `http://192.168.4.1`.
- Internal class names and the existing `BubbleCounter` NVS namespace remain
  unchanged so saved device configuration stays compatible.

## Hardware / Partitions

Target board: Freenove ESP32-S3 WROOM N16R8
(`freenove_esp32_s3_wroom_n16r8`, defined in `boards/`).

- 16 MB flash, 8 MB PSRAM
- `partitions/scapesense_16mb.csv`
- Two 4 MiB OTA application slots
- Approximately 7.9 MiB LittleFS
- 64 KiB coredump partition

GPIO mapping:

- Bubble signal ADC: GPIO2
- IR LED enable: GPIO18, active LOW
- DS18B20 data: GPIO1 with a 4.7 kΩ pull-up to 3.3 V
- pH analogue input: GPIO4; TDS analogue input: GPIO5
- CO₂ relay: GPIO8; dosing-pump MOSFET: GPIO9
- Stepper: STEP GPIO13, DIR GPIO14, ENABLE GPIO15 (active-low)
- GPIO19/20 are reserved for USB; all logic is 3.3 V
- New sensor/actuator assignments are reserved constants, not enabled drivers
- HX711 DOUT: GPIO10
- HX711 SCK: GPIO6
- BOOT button: GPIO0
- No OLED fitted on v1.0: `kDisplayEnabled = false` prevents I²C initialisation.
  Legacy OLED defaults remain only for compatibility; GPIO4/5 belong to pH/TDS.
- Camera: disconnected and not initialised

Hardware defaults are forced in `ProjectConfig.h`, so values previously saved in
NVS cannot override the fixed ESP32-S3 pin mapping.

## Dual-Core Sampling

- `BubbleAcquisition` runs the ADC sampler and `BubbleCounter` state machine in
  a dedicated FreeRTOS task pinned to core 0.
- The Arduino loop on the application core consumes bubble events and takes
  metrics snapshots; it does not poll the ADC.
- Runtime tuning, resets, and daily rollovers are sent to the acquisition task
  through a FreeRTOS command queue.

## Web Stack

- PlatformIO uses `espressif32@8.0.0` / Arduino-ESP32 3.3.12 because NetWizard
  2.x requires the newer `WiFi.AP` API.
- `StatusServer` owns the shared `AsyncWebServer` on port 80.
- NetWizard owns captive-portal provisioning routes, filtered to AP clients.
- The React dashboard is served from LittleFS and subscribes to SSE at
  `/api/events` (`status`, `config`, and `daily-stats` events). Reconnection sends
  fresh snapshots; settings commands remain HTTP POSTs. The JSON GET endpoints
  remain available for scripts.
- WebSerial is mounted at `/webserial`; ElegantOTA remains at `/update` and can
  update `firmware.bin` or the generated `littlefs.bin` filesystem image.
- `/api/*` endpoints remain available for exact programmatic control and use the
  same request queues as the dashboard controls.

## AWS IoT Reporting (development)

- `IotClient` (`src/IotClient.cpp`) runs its own FreeRTOS task using ESP-IDF's
  `esp_mqtt_client` over mutual TLS — no third-party MQTT library. Local
  monitoring never waits on it.
- Topics: `scapesense/{deviceId}/telemetry|events|commands|responses`;
  `deviceId` is the IoT Thing name and MQTT client ID.
- Telemetry publishes every `backgroundIntervalMs` (default 5 min), or
  `liveIntervalMs` (default 2 s) while a `live` command window is active
  (capped at 1 h). Flow start/stop events publish immediately at QoS 1 and
  expire after 24 h. A retained `online`/`offline` LWT marks presence.
- Device credentials for the dev proof live in LittleFS: drop the dev
  certificate + private key into `data/iot/device.crt` and
  `data/iot/device.key` before `pio run -t buildfs` + `uploadfs`. `data/` is
  gitignored — never commit certs. The endpoint uses the built-in ESP-IDF CA
  bundle (Amazon Root CA 1 covers `*-ats.iot.*.amazonaws.com`).
- Configure via `POST /api/iot/config` (form fields `enabled=1`, `endpoint`,
  `deviceId`, optional `backgroundIntervalMs`/`liveIntervalMs`), validated by
  `validIotConfig` and persisted as the `iot_config` NVS blob in the
  `BubbleCounter` namespace. Status surfaces as `iot*` fields in
  `/api/status`.
- Protocol helpers (config validation, telemetry JSON, command parsing,
  topics) live in `include/IotProtocol.h` and are covered by
  `test/test_iot_protocol`.
- UI source lives in `ui/`; generated files in `data/` are build output.
- SVG branding is imported from the sibling `../Logos/` folder and bundled as
  data URLs by esbuild. Keep that folder alongside `ESP_Firmware` when building.
- Daily history offers Graph/Table views over all 30 retained days plus today.
  Average BPS includes idle time across the UK calendar day (DST-aware); today
  uses elapsed time since midnight. Start/end are the recorded flow boundaries.
- Settings shows a bounded 120-sample raw/baseline ADC trend from SSE status
  snapshots; this is not full-speed acquisition data.
- The animated background (`ui/src/components/AquariumBackground.jsx`) draws
  logo-style plants, substrate, fish and bottom-left bubbles on one canvas.
  Geometry and motion live in `ui/src/lib/scene.js` so they can be unit tested.
- The DS18B20 dashboard card shows `temperatureC` only when connected and
  `temperatureValid` is true; otherwise it shows `--`. The sensor uses GPIO1.
- Basic view hides technical diagnostics and calibration; Advanced reveals them.
  View/theme are browser preferences (`scapesense-mode`, `scapesense-theme`).
  Both themes use CSS variables; icon/logo SVGs are algae-green CSS masks.
- Temperature target/tolerance default to 24 ±2 °C and are stored together in NVS
  (`temp_limits`). POST `/api/temperature/config` accepts `targetC` (0–40) and
  `toleranceC` (0.1–10). Values are published in status; `settings-result` SSE
  confirms storage success/failure. Limits are monitoring-only, not heater control.
- Widget borders show healthy/warning/danger with text labels. Bubble-rate alerts
  use the applied `maxReliableBps`. CO₂ bands are low <20%, very low <10%,
  critical <5%, all red with increasing emphasis. Empty (0%) flashes its border,
  except with reduced motion. With a valid empty-cylinder weight configured,
  gross weight below it means Not present; otherwise readings within ±10 g of
  zero do. Temperature is abnormal outside target ± tolerance.
  Offline values are not healthy.
- Optional BPS target is saved in NVS `bubble_target`, exposed as `bubbleTargetBps`
  in status/export and set via POST `/api/bubbles/target` (`targetBps`, 0–20).
  Zero disables it. Basic settings show the optional field; dashboard compares
  live reliable readings against a ±10% visual band. It never controls actuators
  or changes the independent maximum reliable bubble-rate threshold.
- JSON export whitelists public readings/configuration/history; never add credentials.
- POST `/api/device/reset-wifi` requires `confirmation=RESET WIFI`; POST
  `/api/device/factory-reset` requires `confirmation=ERASE`. Both require matching
  browser Origin/Host and queue a delayed restart. Factory reset clears the
  BubbleCounter namespace (including calibration/history) and NetWizard credentials;
  Wi-Fi-only reset preserves other settings. Never invoke either on a real device
  without explicit confirmation for that action.
- NetWizard and WebSerial are AGPL-3.0. Check licensing before distributing
  firmware or source.

## USB Serial / Native CDC

The Freenove board exposes native USB serial (`/dev/cu.usbmodem...`). The
firmware is built with:

```text
-DARDUINO_USB_CDC_ON_BOOT=1 -DARDUINO_USB_MODE=1
```

## Cylinder Monitoring

Cylinder monitoring stores the gross empty-cylinder weight and nominal CO2
capacity in NVS. Percent remaining is calculated from
`(measured gross weight - empty cylinder weight) / capacity`.

## Daily Statistics

- Daily rollover uses UK local time via the POSIX timezone string
  `GMT0BST,M3.5.0/1,M10.5.0/2`, including GMT/BST transitions.
- CO2 flow starts after at least 12 bubbles in a rolling 60-second window
  (0.2 BPS sustained for one minute).
- Flow stops after five minutes without another qualifying one-minute window.
- The current day and 30 completed days are persisted in NVS every five minutes
  and whenever activity starts, stops, resets, or rolls over.
- Daily Avg BPS divides the whole day's bubble count by accumulated confirmed
  flow-session seconds, excluding idle gaps between stopped/restarted sessions.
  The NVS schema v2 migrates the existing v1 layout without discarding history;
  those old records only have first-start/last-stop timestamps, so their flow
  durations and displayed averages are estimates marked with `~`.
