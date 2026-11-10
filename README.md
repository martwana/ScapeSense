# ScapeSense

Local-first aquarium monitoring: an ESP32-S3 counts CO₂ bubbles and serves a
React dashboard on the LAN; an optional Firebase service delivers push
notifications to an installable PWA. Local monitoring never depends on cloud
connectivity.

## Layout

| Path | Contents |
|---|---|
| `ESP_Firmware/` | PlatformIO firmware + React dashboard (`ui/` → built into `data/`) |
| `Firebase/` | Functions API, Firestore rules/indexes, customer PWA |
| `infrastructure/` | Terraform for Firebase/GCP (see `infrastructure/README.md`) |
| `Logos/` | Brand assets |

## Build & test matrix

```bash
# Dashboard UI (Node 20+)
cd ESP_Firmware/ui && npm install && npm test && npm run test:coverage && npm run build

# Firmware + filesystem image (builds UI output first)
~/.platformio/penv/bin/pio run -e freenove-esp32-s3
~/.platformio/penv/bin/pio run -e freenove-esp32-s3 -t buildfs

# Native firmware tests
~/.platformio/penv/bin/pio test -e native

# Host tests (see ESP_Firmware/AGENTS.md for ConfigManager equivalent)
c++ -std=c++17 -Wall -Wextra -I ESP_Firmware/test/host_load_cell_sensor -I ESP_Firmware/include \
    ESP_Firmware/test/host_load_cell_sensor/main.cpp ESP_Firmware/src/LoadCellSensor.cpp \
    -o /tmp/scapesense-load-cell-test && /tmp/scapesense-load-cell-test

# Firebase — use Node 22 (functions + web)
cd Firebase/functions && npm ci && npm run test:coverage && npm audit
cd Firebase/web && npm ci && npm run test:coverage && npm audit && npm run build
```

Device access: `http://scapesense.local` (dashboard), `/webserial`, `/update`.
Firmware and filesystem are flashed separately — see `ESP_Firmware/AGENTS.md`.

## Licensing

Firmware links NetWizard and WebSerial, both **AGPL-3.0**. Distribution
obligations must be resolved (license compliance or replacement libraries)
before commercial firmware/source distribution.
