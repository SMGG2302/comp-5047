# ESP32 Hardware & Firmware (v4)

English | [简体中文](README.zh-CN.md)

Current ESP32-WROOM-32 configuration: router Wi-Fi, dual wicking-cloth sensors, light-controlled valve and live maintenance status. The earlier hotspot and servo sweep settings are historical and do not describe the current firmware.

## Wiring

Disconnect power before wiring. All grounds must be common; analog signals must stay within 0–3.3 V.

| Module | ESP32 connection |
| --- | --- |
| SHT3x | VCC 3V3, GND, SDA GPIO21, SCL GPIO22 |
| Wicking cloth Left 1 | VCC 3V3, GND, AO GPIO34 |
| Wicking cloth Left 2 | VCC 3V3, GND, AO GPIO36 (VP) |
| Light sensor | 3.3 V compatible supply/output, GND, AO GPIO39 (VN/SVN) |
| Water level sensor | 3.3 V compatible supply/output, GND, AO GPIO32 |
| SEN0487 microphone | VCC 3V3, GND, A GPIO33 |
| VL53L0X module | VIN 3V3, GND, SDA GPIO27, SCL GPIO26; GPIO1/XSHUT unused |
| MG90S valve servo | Signal GPIO13; external regulated 5 V supply with common GND |

DS18B20 is disabled. Verify the pin labels and supply requirements of your specific sensor modules.

## Network and upload

Copy `firmware/main/wifi_credentials.example.h` to `wifi_credentials.h` and enter the router’s 2.4 GHz credentials. Keep this ignored file local. Other configuration is in `firmware/main/project_config.h`.

Open `firmware` in PlatformIO, or run:

```powershell
cd project/firmware
pio run
pio run -t upload --upload-port COM3
pio device monitor -p COM3 -b 115200
```

Replace COM3 with the actual USB serial port. The project pins espressif32 6.11.0 and ESP-IDF package 3.50402.0. First build downloads the toolchain. Preserve `partitions.csv`, which defines the audio cache partition.

The machine-specific Windows helper `firmware/build-ascii.ps1 -Upload -Port COM3` copies saved sources to `C:\HollowHavenBuild` and uses the configured `C:\.platformio` installation to avoid non-ASCII build-path issues. Check those local paths before using it on another computer.

Start the [dashboard](../website/README.md) at `http://127.0.0.1:8766`. Use `auto` discovery or the DHCP IP printed in the serial monitor. Both devices must be on the same LAN without client isolation. Discovery uses UDP 9002, v4 telemetry TCP 9000 and HHW1 audio TCP 9001.

## Control rules

- Light ADC <500 opens to 1200 μs; ≥500 or invalid light closes to 2500 μs. Starts closed. Temperature and water level do not currently control the valve.
- Servo pulses change by 10 μs every 100 ms (about 13 seconds for full travel), with continuous 50 Hz PWM. Status is a command, without mechanical position feedback.
- Moisture <2000 is wet, ≥2300 dry; intermediate readings retain the previous state. Initial intermediate/invalid readings are unknown.
- Once fully open for 60000 ms, either cloth still dry triggers possible blockage. Closing resets the timer. Unknown moisture is not treated as dry.
- Water ADC <50 triggers the configured “below 50 ml” refill label, not a continuous volume estimate.
- Presence: 3 consecutive readings <190 mm enter; 5 consecutive readings ≥220 mm leave. Invalid distance is separate from departure. Entry triggers approximately 30 seconds of audio.

## Structure and tests

`firmware/` contains ESP-IDF C code, `shared/` contains protocols/state logic, `pc_receiver/` contains optional C receivers, and `tests/` contains C and Python tests.

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, `build_receiver.ps1` needs CMake and Visual Studio C++ Build Tools. `run_receiver.ps1` and `run_audio_receiver.ps1` launch optional native receivers; avoid competing with the dashboard. The native audio receiver currently builds on Windows only. Third-party driver [provenance and license](firmware/components/st_tof/PROVENANCE.md) are preserved.
