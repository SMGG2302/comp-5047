# HollowHaven — Technical Manual

**Project:** COMP5047 · **Source:** `SMGG2302/comp-5047` · **Reviewed:** 23 October 2026  
**Scope:** Documentation of the code in the supplied `COMP5047.zip` and an inventory-level review of three supplied Blender model files. This is a prototype manual, not a certified wildlife deployment guide.

## 1. Purpose and current scope

HollowHaven is an ESP32-based prototype for monitoring the microclimate and nearby activity of a proposed Greater Glider (*Petauroides volans*) artificial hollow. The current implementation collects sensor telemetry, infers potential presence from a distance sensor, records a short audio clip on a presence-entry event, transfers telemetry and audio to a PC, and provides a local web interface for viewing, exporting and optionally analysing recordings with an external AI service.

**Implemented in source:** sensor acquisition, distance-based presence-state logic, audio capture, ESP32 Wi-Fi access point, TCP telemetry and audio transfer, local Node.js receiver/web UI, local WAV storage, optional manual audio API analysis, and tests.

**Not implemented:** automatic water-flow or valve actuation, closed-loop thermal regulation, validated animal species identification, or field-tested habitat suitability. The proposed perforated 3D-printed plastic enclosure, bark covering and water-based cooling belt are design concepts; their thermal performance must be tested independently.

## 2. System architecture

```text
SHT3x temperature/RH ─────┐
Wick moisture ADC ────────┤
VL53L0X distance ─────────┼──> ESP32-WROOM-32 (ESP-IDF / PlatformIO)
SEN0487 microphone ───────┘       │
                                   ├── TCP 9000: HOLLOWHAVEN/3 telemetry
                                   └── TCP 9001: HHW1 PCM audio + CRC32/ACK1
                                              │
                               Wi-Fi AP: HollowHaven-ESP32
                                              │
                                  Windows PC / Node.js server
                                              │
                          Local browser http://127.0.0.1:8765
                                   ├── Live status and charts
                                   ├── Session CSV and WAV files
                                   └── Optional manual AI audio analysis
                                           (requires internet access)
```

The ESP32 hotspot itself does **not** provide internet access. A PC may need to switch to an internet-connected network before sending a saved recording to the configured API.

## 3. Repository layout

| Path | Purpose |
|---|---|
| `README.md` | Overall project description and quick start |
| `COURSE_BRIEF.md`, `Draft.md`, `Iidea/` | Brief and proposal material |
| `project/firmware/main/` | ESP32 firmware: main loop, sensors, presence, audio, configuration |
| `project/firmware/components/st_tof/` | VL53L0X third-party driver and provenance |
| `project/shared/` | Shared telemetry/audio/presence protocol definitions |
| `project/pc_receiver/` | Optional native C receivers |
| `project/tests/` | C unit tests and Python integration tests |
| `website/server.js` | Node.js local HTTP and TCP receiver backend |
| `website/protocol.js` | Telemetry/WAV parsing and CRC32 logic |
| `website/public/` | HTML/CSS/JavaScript frontend |
| `website/test.js` | Node.js automated tests |
| `website/start.ps1`, `stop.ps1`, `HollowHaven.cmd` | Windows launcher and shutdown helpers |

## 4. Hardware and wiring

**Target:** classic ESP32-WROOM-32. All connected devices must share GND and use ESP32-compatible **3.3 V signals**. Power off before wiring.

| Device | Pins | ESP32 pins | Status |
|---|---|---|---|
| SHT3x temperature/humidity | VCC, GND, SDA, SCL | 3V3, GND, GPIO21, GPIO22 | Enabled |
| Capacitive wick-moisture sensor | VCC, GND, AO | 3V3, GND, GPIO34 | Enabled |
| VL53L0X time-of-flight distance sensor | VIN, GND, SDA, SCL | 3V3, GND, GPIO27, GPIO26 | Enabled |
| SEN0487 analog microphone | VCC, GND, A | 3V3, GND, GPIO33 | Enabled |
| DS18B20 external temperature | GPIO4 | — | Disabled |
| Light sensor | GPIO35 | — | Disabled |
| Water-level sensor | GPIO32 | — | Disabled |

VL53L0X `GPIO1` and `XSHUT` are not connected in the current prototype. Sensor enable flags, GPIO assignments, Wi-Fi settings and thresholds are defined in `project/firmware/main/project_config.h`.

**Important:** The wick reading is a raw ADC value (not a calibrated percentage of moisture). Do not treat the disabled light, water or external temperature fields as valid measurements.

## 5. Prerequisites

- Windows PC for the documented launch scripts; Node.js **18+** for the website (prefer a supported LTS version).
- VS Code with PlatformIO IDE to compile/upload ESP32 firmware; an appropriate USB cable and driver.
- Optional: PlatformIO Core CLI, CMake and Visual Studio 2022 C++ Build Tools for native receiver builds.
- ESP32-WROOM-32 and the sensors listed above.
- Internet access for initial firmware toolchain installation and optional AI API calls.

The website has no required npm package installation; it uses built-in Node.js modules. Firmware dependencies are pinned in `project/firmware/platformio.ini`.

## 6. Build and flash the ESP32 firmware

1. Open `project/firmware/` as a PlatformIO project in VS Code.
2. Confirm wiring and the settings in `main/project_config.h`.
3. Use PlatformIO **Build**, then **Upload**.
4. Open a serial monitor at **115200 baud**.

CLI alternative, from the repository root:

```powershell
cd project/firmware
pio run
pio run -t upload --upload-port COM3
pio device monitor -p COM3 -b 115200
```

Replace `COM3` with the detected serial port. The configuration pins `espressif32@6.11.0`, ESP-IDF `3.50402.0`, board `esp32dev`, and the custom `partitions.csv` (needed for audio storage). Avoid non-ASCII filesystem paths if toolchain errors occur.

## 7. Start and use the local monitoring website

From the repository root:

```powershell
cd website
node server.js
```

Alternatively, double-click `website/HollowHaven.cmd` on Windows. Open **http://127.0.0.1:8765** in a browser.

1. Connect the PC Wi-Fi to **`HollowHaven-ESP32`** using the prototype password defined in `project_config.h` (the supplied code uses `HollowHaven5047`). Change this demonstration password before any deployment.
2. In the website, select **Connect ESP32**. The default device address is **`192.168.4.1`**.
3. Observe live temperature, humidity, wick ADC, distance, presence status and sound statistics. Unknown or stale readings must not be interpreted as evidence that no animal is present.
4. Open the recordings area to play received WAV files and view results.
5. For optional classification, configure the provider and API key, choose a saved recording, connect the PC to the internet if necessary, and **manually** select upload/analyse. Recordings are not uploaded automatically.
6. Stop the website using `website/stop.ps1` when launched in the background, or stop the terminal process with Ctrl+C when launched using `node server.js`.

**Avoid running the legacy C receiver at the same time as the website**, because the device allows only one connected client and receiver processes may compete for the same streams.

## 8. Presence detection and audio recording

The firmware uses a VL53L0X distance sensor, with provisional bench-test thresholds:

| Event | Logic |
|---|---|
| Potential presence entry | Distance **below 200 mm** for **3 consecutive valid readings** |
| Potential presence exit | Distance **above 250 mm** for **5 consecutive valid readings** |
| Invalid distance | Report invalid/unknown; do **not** infer absence |
| Audio recording | About **30 seconds** when an entry event is detected |

The time-of-flight distance sensor detects proximity or obstruction, **not species identity**. Thresholds must be calibrated for the actual geometry of the hollow and sensor placement. The audio sample rate is approximately **8192 Hz**; this is not suitable for comprehensive bat ultrasound detection.

## 9. Data communication and storage

### Telemetry

- ESP32 access point: `HollowHaven-ESP32`, default address `192.168.4.1`.
- TCP port **9000** carries a `HOLLOWHAVEN/3` telemetry stream, nominally every **2 seconds**.
- Each sample has **20 fields**: `version`, `boot_id`, `sequence`, `uptime_ms`, `inside_temp_c`, `inside_rh_pct`, `outside_temp_c`, `wick_raw`, `light_raw`, `water_raw`, `valid_mask`, `sound_rms`, `sound_p2p`, `sound_samples`, `sound_clipped`, `adc_overflows`, `distance_mm`, `animal_present`, `presence_valid`, `audio_recording`.
- A validity bitmask indicates which sensor readings are valid. Disabled or invalid ADC values use `-1`; invalid temperature values may use `nan`.

### Audio

- TCP port **9001** carries `HHW1` audio frames with a **32-byte header**, 16-bit mono PCM payload, and **CRC32**.
- The receiver verifies integrity, writes a WAV file, and only then sends **`ACK1`**. The implementation supports reconnection and retry behaviour.

### Local files

| Path (created at runtime) | Contents |
|---|---|
| `website/data/session_*.csv` | Session telemetry with reception timestamp |
| `website/data/results.json` | Stored audio classification results |
| `website/data/settings.json` | Non-secret settings |
| `website/recordings/*.wav` | Received or imported WAV recordings |
| `website/data/server.log`, `server-error.log` | Logs when using Windows launcher |

The website listens on **127.0.0.1:8765** by default, not on the LAN or public internet. API keys are kept in process memory rather than written to settings files. Do not commit recordings, logs, credentials, or runtime data.

## 10. Optional AI audio analysis

The website supports a configurable Gemini-based audio analysis workflow and a custom service endpoint. The default model in `website/server.js` is `gemini-2.5-flash`, but availability and provider behaviour may change. Audio analysis occurs **only after the user selects a recording and requests upload**.

Candidate categories include Greater Glider, other possums/gliders, birds, audible bat sounds, insects, human speech, ambient noise, and unknown. Outputs are **unvalidated model suggestions**, not confirmed species detections. A returned score is not necessarily a calibrated identification probability. Keep the `unknown` category for weak or ambiguous audio.

A custom endpoint accepts `POST` with `Content-Type: audio/wav`, optional Bearer authorization, and JSON responses such as:

```json
{"label":"unknown","evidence":"weak audio","uncertainty":"needs verification"}
```

or

```json
{"predictions":[{"label":"bird","score":0.7},{"label":"unknown","score":0.3}]}
```

Use HTTPS for non-local custom endpoints.

## 11. Testing and validation

### Website tests

```powershell
cd website
node --test test.js
```

The repository tests telemetry parsing, WAV format checks, CRC32, simulated TCP/audio behaviour, persistence-before-ACK, API mocks, configuration/secret handling and export functionality.

### Optional native receiver and tests

```powershell
cd project
powershell -ExecutionPolicy Bypass -File ./build_receiver.ps1
powershell -ExecutionPolicy Bypass -File ./run_receiver.ps1
```

For CMake-based tests:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The native audio receiver is currently Windows-specific. Python integration tests are under `project/tests/`. The optional `website/ui-check.js` browser check requires additional Playwright setup.

**Validation status:** The repository describes simulated integration and UI checks. This review did **not** flash an ESP32, connect real sensors, test the water-cooling structure, or validate AI species identification against labelled animal recordings. Record actual test runs and outcomes separately before claiming hardware performance.

## 12. Troubleshooting

| Symptom | Checks / resolution |
|---|---|
| Website does not open | Confirm Node.js 18+, run `node server.js`, inspect terminal or `website/data/server-error.log`, verify port 8765 is free |
| Cannot connect to ESP32 | Check power, serial output, PC Wi-Fi connection to ESP32 hotspot, device IP `192.168.4.1`, and absence of competing receiver |
| Temperature or ADC value missing | Check wiring, common ground, 3.3 V compatibility and enabled flags in `project_config.h`; disabled sensors intentionally return invalid values |
| Presence falsely detected | Recalibrate VL53L0X geometry and 200/250 mm thresholds; confirm valid consecutive readings |
| No WAV appears | Check microphone wiring, actual entry trigger, audio TCP port 9001, CRC/retry status, free disk space and recordings folder |
| Audio classification fails | Verify API configuration/key, provider quota, internet connectivity, supported WAV file and explicit upload action |
| Firmware build fails | Verify PlatformIO toolchain installation, board/port, and that `partitions.csv` remains in the project |
| Moisture reading seems inaccurate | `wick_raw` is an uncalibrated ADC value; collect dry/wet calibration samples before interpreting it |

## 13. Proposed thermal-control research (not yet implemented)

The proposed habitat combines a **3D-printed perforated plastic enclosure**, **bark covering**, and a **water-based cooling belt**. These features are hypotheses to test rather than proven cooling improvements. The current code does not actuate water flow, regulate valves, or run a closed-loop cooling algorithm.

For future evaluation, collect internal/external temperature, relative humidity, solar/light exposure, wick moisture, water level/consumption, actuator state and time of day. Compare an unchanged plastic baseline against successive configurations: perforation, bark covering, and cooling belt. Under matched heating conditions, evaluate peak internal temperature, inside–outside temperature difference, heating rate, time above chosen *experimental* thresholds, humidity changes, and water use. Experimental thresholds must not be presented as established Greater Glider comfort or safety limits.

If a water-cooling belt is added, document whether it relies on evaporation, water circulation, or temporary heat storage, where heat is rejected, how water is replenished, and how wetness or excessive humidity is prevented. Any eventual real-wildlife deployment requires ecological oversight and independent habitat-safety evaluation.

## 14. 3D-printed mechanical design and supplied Blender assets

### 14.1 Model files and verification status

Three native Blender assets were supplied separately from the code archive. They are **design files**, not evidence of manufactured or assembled parts. The files were successfully checked for file existence and decompressed to recognizable Blender project headers; Blender itself was not available in this review environment, so mesh geometry, dimensions, wall thickness, manifold status, modifier results, visual appearance and assembly alignment **have not been verified**.

| Supplied asset | Design role indicated by filename | Internal object names observed | Verification limits |
|---|---|---|---|
| `HollowHaven_bottom.blend` | Bottom/base component | `Cylinder` | Shape, interfaces, drainage and size not confirmed |
| `HollowHaven_watercollector.blend` | Water-collection component | `Cone`, `Cylinder` | Water capacity, slope, outlet and flow path not confirmed |
| `HollowHaven支架.blend` | Support/bracket structure (支架) | `柱体`, `柱体.001`; Boolean Math node names also present | Mechanical support, fasteners and mating dimensions not confirmed |

The roles above are inferred from filenames, not verified from rendered geometry. The object names are extracted from the native Blender files and do not prove final printable topology.

### 14.2 Proposed physical integration

The intended concept, based on the team's design description, combines a perforated plastic hollow, an external bark layer and a water-based cooling belt, together with the three modelled components. The following is a **proposed integration sequence**, not a verified assembly instruction:

1. Position the **support/bracket** as the structural mounting component, subject to checking load paths and attachment points in Blender.
2. Fit the **bottom/base** and inspect how it interfaces with the hollow body and any drainage or sensor mounting features.
3. Position the **water collector** so that collected or supplied water can reach the proposed cooling belt. The actual path and outlet geometry must be confirmed in the CAD assembly.
4. Keep the **water-cooling region separated from electronics and the animal-facing cavity** until leakage, humidity and electrical-safety tests have been completed.
5. Add bark or bark-like covering only after verifying that it does not obstruct necessary vents, drainage, inspection access or serviceability.

A potential architecture for design discussion is:

```text
Rain / water supply (not yet confirmed)
                 |
      Water collector [CAD asset]
                 |
      Water routing / reservoir [TO DESIGN OR VERIFY]
                 |
      Cooling belt [proposed; mechanism unverified]
                 |
      Perforated 3D-printed shell + bark covering
                 |
      Inner hollow microclimate + sensors
                 |
      Base and mounting bracket [CAD assets]
```

This is a conceptual dependency diagram, **not** a verified fluid or mechanical schematic. In particular, the existing firmware's wick-moisture reading does not demonstrate that the water collector, water belt or an actuator has been integrated.

### 14.3 CAD-to-print checklist

Before including dimensioned assembly diagrams or printing instructions in the final submission, open each `.blend` file in Blender and record:

- Scene units and bounding dimensions in millimetres; exact part names and versions.
- Mesh manifold/watertight status, wall thickness, overhangs, tolerances and required supports.
- Material/filament, printing orientation, infill, layer height and post-processing plan.
- Connection interfaces among the base, support, collector, shell and water belt.
- Water inlet, outlet, drainage, overflow and potential leak points.
- Sensor cable routing, electrical isolation, ventilation paths and access for cleaning.
- Photographs or exported renders of the assembly and screenshots of the dimensions.

Do not infer print settings or fit tolerances from filenames alone. If a dimensioned STL/3MF export or Blender render is provided, this section can be expanded into a verified fabrication and assembly guide.

### 14.4 Thermal and environmental validation

The combined plastic/perforation/bark/water design is an **untested thermal hypothesis**. Compare a baseline plastic enclosure against controlled variants (perforated shell, bark covering, and cooling belt) under matched external temperature and illumination. Log internal/external temperature, relative humidity, cooling-belt wetness, water use and sensor/actuator states. Report peak temperature, heating rate, inside–outside difference and high-temperature exposure duration. Separately test leak containment, excessive humidity, structural integrity and serviceability before making habitat-suitability claims.

## 15. Security, limitations and responsible use

- Change the hard-coded demonstration Wi-Fi password before deployment.
- Never include an API key in the repository, screenshots or manual.
- Local browser access does not imply internet connectivity or remote access.
- Do not interpret distance sensor activation as verified animal occupancy.
- Do not interpret generic AI classification as validated Greater Glider detection.
- Do not claim active thermal regulation, cooling performance, or animal welfare benefit until measured and evaluated.
- Third-party VL53L0X driver provenance and licensing are documented in `project/firmware/components/st_tof/PROVENANCE.md`.

## 16. Source files and design assets reviewed

`README.md`; `project/README.md`; `project/firmware/main/project_config.h`; `project/firmware/platformio.ini`; `project/firmware/main/main.c`; `project/shared/telemetry.h`; `website/README.md`; `website/package.json`; `website/server.js`; `website/protocol.js`; `website/start.ps1`; `website/test.js`.

**Design assets supplied:** `HollowHaven_bottom.blend`; `HollowHaven_watercollector.blend`; `HollowHaven支架.blend`. The native files were inspected at a file-format/object-name level only; no geometry render or printability validation was performed.

**Document status:** Source-grounded draft with explicitly marked mechanical-design inferences. Hardware pinouts and runtime instructions are based on repository documentation and configuration; they should be checked on the team's physical prototype before final submission.
