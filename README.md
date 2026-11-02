# HollowHaven — Shelter Environment & Sound Monitor

English | [简体中文](README.zh-CN.md)

Current v4 reference: [Chinese technical manual](HollowHaven_技术手册.md) and [current setup](project/CURRENT_SETUP.md), covering wiring, router networking, port 8766, temperature- and light-controlled watering and maintenance alerts.

HollowHaven is an ESP32 prototype for studying tree-hollow shelter conditions for the Greater glider (*Petauroides volans*). It measures temperature, humidity, two wicking-cloth moisture channels, light, water level and distance. A nearby object triggers an approximately 30-second audio recording. A local dashboard displays telemetry, receives recordings and offers optional, manual audio analysis through an API.

## Quick start

1. Follow the [hardware setup](project/README.md). Copy `project/firmware/main/wifi_credentials.example.h` to `wifi_credentials.h` and enter your router’s 2.4 GHz Wi-Fi credentials. This local credentials file is ignored by Git.
2. Build and upload `project/firmware` with PlatformIO. See the hardware README for the Windows ASCII-path build helper and serial-port selection.
3. Connect the computer to the same router. The computer may use Wi-Fi or Ethernet; client isolation must be disabled.
4. With Node.js 18 or newer installed, run `node server.js` in `website`, or double-click `website/HollowHaven.cmd` on Windows. No `npm install` is required.
5. Open [http://127.0.0.1:8766](http://127.0.0.1:8766). Leave Device IP as `auto` and click **Connect ESP32**. If discovery fails, enter the IPv4 address printed by the ESP32 serial monitor.
6. Use the **English / 中文** selector in the header to change language. English is the default; the browser remembers your choice. Switching language keeps your connection, settings and audio playback intact.
7. In **Sound archive**, play local recordings. To analyze one, configure your API in Settings and explicitly click **Upload & analyze audio**. Internet access is required for cloud analysis.

## Repository

| Directory | Contents |
| --- | --- |
| [project/](project/README.md) | ESP-IDF C firmware, wiring, shared protocols, native receivers and tests |
| [website/](website/README.md) | Local Node.js service, bilingual dashboard, API integration and tests |

[Draft.md](Draft.md) contains the proposal; [COURSE_BRIEF.md](COURSE_BRIEF.md) contains the course brief. The dashboard is a prototype debugging and research-data tool.

Wiring spreadsheet: [接线表.xlsx](接线表.xlsx).

## Current hardware and control rules

The current board is ESP32-WROOM-32. See the [hardware README](project/README.md) for wiring.

- **Network:** router station mode; UDP 9002 discovery, TCP 9000 telemetry and TCP 9001 audio. The dashboard listens only on `127.0.0.1:8766`.
- **Valve control:** valid light ADC <500 and temperature >30°C opens to 1200 μs. Once fully open, hold at least one minute, then check temperature once per minute: close to 2500 μs only below 26°C; otherwise hold another minute. Light changes do not end the cycle; invalid temperature holds until a later check. Starts closed; water level remains a maintenance alert.
- **Moisture:** below 2000 is wet; 2300 or above is dry. Intermediate values retain the previous state. Uninitialized or invalid readings are unknown.
- **Maintenance:** a cloth still dry after one minute fully open triggers a possible blockage alert. Water ADC below 50 triggers a refill alert; the “below 50 ml” label is a configured threshold, not a continuous volume measurement.
- **Presence:** three consecutive readings below 190 mm mark possible occupancy; five at or above 220 mm clear it. Invalid distance readings do not imply departure.

Valve status reports the servo command, not a measured mechanical position. ADC moisture readings are not calibrated moisture percentages. Physical cooling, water flow and animal sound recognition require field validation. Infrared does not establish animal identity. Audio sampling does not cover ultrasonic bat identification.

## Language and analysis results

Navigation, live states, tooltips, maintenance alerts, charts, settings, logs and application errors support English and Chinese. New Gemini analyses are requested in the language selected when you start the upload. Existing AI results, filenames, custom-service labels and user-entered settings keep their original content; switching the interface does not translate or re-upload them.

## Tests and local data

```powershell
cd website
node --test test.js i18n.test.js gemini.test.js
```

The optional browser check is `node ui-check.js` (requires Playwright and Edge). Native C tests are documented in the [hardware README](project/README.md). Simulated tests do not replace physical sensor, valve, network or wildlife validation.

Recordings, session data, API keys, local Wi-Fi credentials and build caches are excluded from Git. API keys are held only in the running server’s memory; a server restart clears them. Recordings upload only when explicitly requested. The VL53L0X driver retains its license and [provenance](project/firmware/components/st_tof/PROVENANCE.md).

## User manuals and current status (2 November 2026)

- [English user manual (PDF)](docs/manuals/HollowHaven-User-Manual-English.pdf)
- [Chinese user manual (PDF)](docs/manuals/HollowHaven-User-Manual-Chinese.pdf)
- [English manual source](docs/User-Manual.en.md) and [Chinese manual source](docs/用户手册.zh-CN.md)
- [Ten-day simulation data and English charts](experiments/nest-microclimate-simulation/README.md) (synthetic data, not field measurements)

The PDFs include clickable contents and bookmarks, setup, operation, maintenance, intended users and a minimal-intervention design principle. Section 7.3 describes manual diagnosis and replacement of an unresponsive capacitive sensor; this combined water-level/moisture fault criterion is not yet an automatic firmware or dashboard alarm.

Firmware supports a primary Wi-Fi network and an optional fallback, trying three failed connection attempts before switching. Set credentials locally using the example header. Current valve control opens above 30°C with qualifying light and checks every minute after fully opening, closing only below 26°C. Low water does not automatically stop watering. The dashboard uses the Gemini Interactions API for manually requested analysis; API keys remain in process memory only.

These documents are updated with project commits, not automatically from live sensor readings. The current setup and user manuals take precedence over older historical descriptions in the original technical manual.
