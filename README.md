# SmartSafe

ESP32-CAM based RFID access-control and IoT safe prototype.

SmartSafe combines embedded firmware, a physical RFID authorization path, a solenoid-lock driver, event capture, Firebase Realtime Database synchronization, a web dashboard, KiCad PCB sources and Fusion 360 mechanical files.

## What this project demonstrates

- C++ firmware on ESP32-CAM with FreeRTOS tasks and a non-blocking state-machine design.
- Local RFID allowlist authorization using an RC522 reader.
- Fail-closed handling for missing credentials, missing TLS CA data and unprovisioned OTA authentication.
- Solenoid/MOSFET lock-control path with explicit actuator-command state.
- Firebase RTDB event synchronization with an offline queue.
- Append-only event logging and role-separated database rules for device/admin/operator access.
- PlatformIO build and mock-test workflow with GitHub Actions checks.
- KiCad schematic/PCB sources and Fusion 360 enclosure/mechanical sources.

## Architecture

~~~text
RFID allowlist
      |
      v
ESP32-CAM firmware ---> MOSFET / solenoid lock
      |                         |
      |                         +--> commanded lock state
      v
Firebase RTDB <--------- device status and audit events
      ^
      |
Authorized dashboard ---> alarm command request
~~~

The physical RFID decision is local to the device. The cloud and web dashboard do not directly unlock the safe.

## Repository map

- FIRMWARE/Platform.IO/ - ESP32-CAM firmware, mocks and PlatformIO configuration
- FIRMWARE/WEB/akilli-kasa-dashboard/ - dashboard, Firebase config template and RTDB rules
- PCB/ - KiCad schematic and PCB sources
- 3D/ - Fusion 360 source files
- docs/ - physical bring-up and acceptance checklist
- SECURITY.md - security assumptions and reporting guidance

## Local configuration

Secrets are intentionally excluded from Git. Copy the templates locally:

~~~bash
cd FIRMWARE/Platform.IO
cp include/secrets.example.h include/secrets.h

cd ../WEB/akilli-kasa-dashboard/web
cp firebase-config.example.js firebase-config.js
~~~

Do not commit Wi-Fi credentials, Firebase service-account keys, device passwords, OTA secrets, tokens or personal event images. Use separate Firebase identities for the device and dashboard, and deploy the RTDB rules before connecting a real device.

## Build and test

~~~bash
cd FIRMWARE/Platform.IO
pio run -e esp32cam
pio run -e esp32cam-hardware
pio run -e esp32cam-production
./wokwi_test.sh
~~~

The Wokwi/mock path checks firmware behavior and RFID flows; it is not proof of physical lock safety, cloud availability or hardware electrical correctness. Follow the physical bring-up checklist before energizing a real actuator.

## Security boundaries

- RC522 UID matching is not cryptographic authentication and cards can be cloned.
- is_locked represents the commanded actuator state because the prototype has no physical lock-position sensor.
- A solenoid must use an appropriate external supply, common ground and flyback protection.
- ESP32-CAM and RC522 logic are 3.3 V; signal levels and MOSFET gate drive must be verified against the relevant datasheets.
- This is a prototype and must not be treated as the sole security layer for valuable assets.

## Project scope

This is a personal engineering and learning project. It includes AI-assisted development in parts; the repository documents the implemented architecture, security decisions, test paths and limitations rather than claiming production readiness.

## License

MIT License. See LICENSE.