# HollowHaven

English | [简体中文](README.zh-CN.md)

An ESP32-based artificial tree-hollow prototype for exploring habitat care for the greater glider (*Petauroides volans*). HollowHaven combines environmental monitoring, local automatic drip control and distance-triggered audio recording with a bilingual dashboard. The project is a coursework prototype; long-term wildlife suitability and species-recognition accuracy have not been established.

## Documentation

Updated 6 November 2026. All three document sets are available as PDFs with editable LaTeX packages.

| Document | Purpose | English | Chinese | LaTeX source |
| --- | --- | --- | --- | --- |
| Project report | Design rationale, implementation, evaluation and limitations | [PDF](docs/report/report-en.pdf) | [PDF](docs/report/report-zh.pdf) | [ZIP](docs/report/report-latex.zip) |
| User manual | First-use quick guide, dashboard operation, maintenance and fault response | [PDF](docs/manuals/HollowHaven-User-Manual-English.pdf) | [PDF](docs/manuals/HollowHaven-User-Manual-Chinese.pdf) | [ZIP](docs/manuals/HollowHaven-User-Manual-LaTeX.zip) |
| Technical manual | Architecture, wiring, assembly, software deployment and calibration | [PDF](docs/manuals/HollowHaven-Technical-Manual-English.pdf) | [PDF](docs/manuals/HollowHaven-Technical-Manual-Chinese.pdf) | [ZIP](docs/manuals/HollowHaven-Technical-Manual-LaTeX.zip) |

For electrical connections, use the [pin-by-pin wiring table](docs/wiring/Wiring-Table.en.md) and [current setup](project/CURRENT_SETUP.md), and check the actual module markings. The technical manual includes an illustrative wiring image; some power stubs are visually separated from the pads. Mechanical dimensions and manufacturing details marked as pending still require completion.

## Current capabilities

- ESP32-WROOM-32 reads SHT3x temperature/humidity, two capacitive wick sensors, analog light and water-level sensors, VL53L0X distance and a SEN0487 microphone.
- An MG90S servo controls gravity-fed water delivery to the external wicking layer. Control runs on the ESP32 independently of the receiving computer.
- A proximity entry event triggers approximately 30 seconds of audio. Continuous presence does not repeatedly trigger entry recordings.
- The local dashboard provides live states, trends, maintenance notifications, session CSV export, WAV playback/download/import and English/Chinese controls.
- Audio analysis is optional and manually initiated through a configured provider. Results are unverified candidates, not confirmed animal identities.

## Quick start

### Prepare the device

1. Follow the technical manual and [hardware setup](project/README.md). Use compatible 3.3 V sensor signals and an external regulated 5 V servo supply with a common ground.
2. Copy `project/firmware/main/wifi_credentials.example.h` to `wifi_credentials.h` in the same folder and enter the local 2.4 GHz Wi-Fi credentials. The real credentials file is ignored by Git.
3. Build and upload `project/firmware` with PlatformIO. Complete ground-level electrical, valve and water-path checks before installation. Firmware supports a primary network and an optional fallback.

### Start monitoring

1. Connect the computer to the device's local network; Wi-Fi or Ethernet is suitable if the devices can communicate. Disable router client isolation.
2. Install Node.js 18 or later. From `website`, run `node server.js`, or double-click `website/HollowHaven.cmd` on Windows. The server does not require `npm install`.
3. Open [http://127.0.0.1:8766](http://127.0.0.1:8766). Save Device IP as `auto`, then connect. If discovery fails, use the actual IP from the device's serial output.
4. Confirm that sensor readings and timestamps keep updating. Use the header language selector as needed. Configure an analysis provider only if audio analysis is required, then explicitly select a recording and upload it.
5. Keep the computer awake and the server running for continuous reception. To stop reception on Windows, run `./stop.ps1` from `website` in PowerShell.

Closing the browser leaves the server running. Stopping the server or losing the network does not stop ESP32 watering control. To stop the physical system, manually isolate water before disconnecting power: power loss does not guarantee valve closure.

## Operating behaviour and limits

| Area | Current behaviour |
| --- | --- |
| Network | Router station mode; UDP 9002 discovery, TCP 9000 telemetry and TCP 9001 audio. The web server binds to `127.0.0.1:8766`. |
| Watering | Valid light ADC <500 and temperature >30°C request opening. After fully opening, hold at least 60 seconds and check every minute; close only below 26°C. Subsequent light changes do not end the cycle; invalid temperature keeps it open until a later check. |
| Maintenance | Low water produces a refill notification, not automatic shutoff. A wick still dry after 60 seconds fully open produces a possible blockage notification. |
| Observability | Valve status is a command, not measured position or flow. ADC values are not calibrated millilitres, lux or moisture percentages. A displayed “below 50 ml” label is not a volume measurement. |
| Data gaps | The computer cannot record live telemetry while disconnected or asleep; full missing telemetry history is not backfilled. Device audio buffering is finite. |
| Validation | Control temperatures are engineering settings, not validated animal comfort limits. Distance does not identify a species; audio does not cover ultrasonic bat identification. |

There is no maximum watering duration or low-water interlock. Plan refilling and a manual isolation method. Diagnosis based on falling water level and an unresponsive moisture sensor remains a manual check, not an implemented combined automatic alarm. Detailed thresholds and calibration belong in the technical manual and source code.

## Repository and evaluation data

| Path | Contents |
| --- | --- |
| [project/](project/README.md) | ESP-IDF C firmware, shared control/protocol code, native receivers and tests |
| [website/](website/README.md) | Node.js service, bilingual dashboard and tests |
| [docs/manuals/](docs/manuals/) | User and technical manuals, PDFs and LaTeX archives |
| [docs/report/](docs/report/) | Current bilingual project report and LaTeX archive |
| [modeling/](modeling/) | Blender models: `HollowHaven_scaled.blend`, `box2.blend`, `funnel.blend` |
| [Ten-day dataset](experiments/nest-microclimate-measurements/README.md) | Workbook, CSV records and standalone chart viewer |
| [Draft.md](Draft.md), [idea/](idea/), [COURSE_BRIEF.md](COURSE_BRIEF.md) | Proposal, design history and course brief |

The team identifies the ten-day dataset as measured data. Its README records the correction of legacy simulation labels; collection dates, site, instruments, calibration and channel-level processing details remain to be supplied. The records alone do not establish wildlife-welfare benefits. The standalone dataset viewer is separate from the live dashboard. The older `docs/COMP5047_Project_report.pdf` is retained as a historical report; use `docs/report/` for the revised reports.

## Verification and local data

Run the dashboard tests from `website`:

```sh
node --test test.js i18n.test.js gemini.test.js
```

The optional `node ui-check.js` browser check requires Playwright and Edge. Native C build/test instructions are in the [hardware README](project/README.md). Software tests do not replace hardware or field validation.

Telemetry is stored in `website/data/session_*.csv`, recordings in `website/recordings/`, and analysis results in `website/data/results.json`. Back up these local files. Runtime data, real Wi-Fi credentials, API keys and build caches are excluded from Git. API keys remain in server process memory and must be re-entered after restart. Uploading a recording requires explicit user action and may incur provider charges. The VL53L0X driver retains its [license and provenance](project/firmware/components/st_tof/PROVENANCE.md).
