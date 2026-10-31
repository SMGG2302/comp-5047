# HollowHaven Local Dashboard

English | [简体中文](README.zh-CN.md)

A local Node.js dashboard for ESP32 telemetry, water-valve status, maintenance alerts, recordings and optional audio analysis. English is the default. Select **English / 中文** in the header; the browser remembers the choice. Navigation, dynamic statuses, tooltips, alerts, chart labels, logs and application errors switch language without interrupting reception or playback.

## Start and connect

```powershell
cd website
node server.js
```

Requires Node.js 18 or newer; no npm dependencies are required. On Windows, double-click `HollowHaven.cmd`. Open [http://127.0.0.1:8766](http://127.0.0.1:8766). Closing the page leaves the receiver running; `stop.ps1` stops the background service started by the Windows launcher.

ESP32 and computer must share the router’s local network. ESP32 uses 2.4 GHz Wi-Fi; the computer can use the same router’s 5 GHz Wi-Fi or Ethernet. Disable client isolation. Set Device IP to `auto` for discovery (UDP 9002), or enter the IP printed by the serial monitor. Telemetry uses TCP 9000; audio uses TCP 9001. Avoid running another receiver that competes for device connections.

## Live monitoring

The status bar shows refill needs, direct sunlight, valve commands and two wicking-cloth moisture states. Hover or keyboard-focus a status to see ADC values and rules. Temperature, air humidity, infrared distance and sound statistics appear below. See the [current hardware rules](../project/README.md).

Water below 50 ADC triggers the configured “below 50 ml” refill label, not a continuous volume conversion. Dry cloth after one minute fully open triggers a possible blockage alert, which needs physical inspection. Invalid or stale data stays unknown and does not establish animal departure.

## Audio analysis

Select a WAV file, configure Gemini or a custom API, then click **Upload & analyze audio**. Reception, playback and import never upload automatically. Enter the API key in Settings; it lives only in server memory and is cleared on restart. Changing provider or endpoint without a replacement key clears the old key. API usage may incur provider charges.

Gemini is asked to return new results in the selected interface language. Existing results and custom-service responses keep their original language, as do filenames and user settings. General audio models have not been validated for this project’s wildlife species. Candidate labels are provisional, model scores are not calibrated probabilities, and these recordings cannot identify ultrasonic calls not captured by the microphone.

Custom endpoints receive HTTP POST raw WAV with `Content-Type: audio/wav` and optional `Authorization: Bearer …`. HTTPS is required except for local testing. Return one of:

```json
{"label":"Unknown","evidence":"Weak signal","uncertainty":"A clearer recording is needed"}
```

```json
[{"label":"Bird","score":0.7},{"label":"Unknown","score":0.3}]
```

Or wrap the array in `{"predictions":[...]}`.

## Local storage and protocol

- `recordings/`: received and imported WAV files.
- `data/session_*.csv`: session telemetry, available through CSV export.
- `data/results.json`: saved analysis results; `data/settings.json`: settings without API keys.
- Browser local storage key `hollowhaven.language`: interface language only.
- v4 telemetry and legacy v3 input; HHW1 audio with CRC32 and ACK1 only after successful persistence.

The HTTP server binds only to `127.0.0.1:8766`. It is not a LAN-shared or public dashboard.

## Verification

```powershell
node --test test.js i18n.test.js
node ui-check.js
```

The browser check is optional and requires Playwright and Edge. Set `HH_PLAYWRIGHT_PATH` to an installed Playwright module path if needed, and `HH_UI_URL` for a non-default dashboard URL. Checks cover language switching/persistence, live alerts, settings preservation, archive empty states and desktop/mobile layout. Unit and simulated integration tests do not validate physical hardware or wildlife identification accuracy.
