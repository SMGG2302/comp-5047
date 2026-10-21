"""Developer-only transport test. Both deployed applications are written in C.
No hardware, Wi-Fi settings, external service or third-party Python package used.
"""
import csv
import pathlib
import socket
import subprocess
import sys
import tempfile
import threading
import time

COLUMNS = "version,boot_id,sequence,uptime_ms,inside_temp_c,inside_rh_pct,outside_temp_c,wick_raw,light_raw,water_raw,valid_mask,sound_rms,sound_p2p,sound_samples,sound_clipped,adc_overflows,distance_mm,animal_present,presence_valid,audio_recording"
GREETING = ("#HOLLOWHAVEN/3 " + COLUMNS + "\n").encode()


def sample(seq, boot=123):
    return f"3,{boot},{seq},{seq*2000},30.50,60.25,32.00,2000,3000,1500,127,12.50,64,10000,2,0,100,1,1,1\n".encode()


def run(executable):
    with tempfile.TemporaryDirectory(prefix="hollowhaven-test-") as temporary:
        output = pathlib.Path(temporary) / "data.csv"
        errors = []
        with socket.socket() as server:
            server.bind(("127.0.0.1", 0))
            server.listen(1)
            server.settimeout(20)
            port = server.getsockname()[1]

            def mock():
                try:
                    # An unrelated server must not be mistaken for the ESP32.
                    connection, _ = server.accept()
                    with connection:
                        connection.sendall(b"NOT THE DEVICE\n")
                    connection, _ = server.accept()
                    with connection:
                        payload = GREETING + sample(1)
                        # Every boundary, including within floats/header, can split.
                        for start in range(0, len(payload), 7):
                            connection.sendall(payload[start:start+7])
                        connection.sendall(b"x" * 800 + b"\n")
                        connection.sendall(b"3,123,99,2000,=1+1,60,32,1,2,3,31\n")
                        connection.sendall(b"binary\x00payload\n")
                        connection.sendall(b"3,123,2,4000,nan,nan,nan,-1,-1,-1,0,nan,-1,0,0,0,-1,0,0,0\n")
                        connection.sendall(sample(3)[:15])  # incomplete line lost
                    connection, _ = server.accept()
                    with connection:
                        connection.sendall(GREETING)
                        # Duplicate latest sample after reconnect must be suppressed.
                        connection.sendall(b"3,123,2,4000,nan,nan,nan,-1,-1,-1,0,nan,-1,0,0,0,-1,0,0,0\n")
                        connection.sendall(sample(4) + sample(5) + sample(1, boot=456))
                        time.sleep(0.2)
                except Exception as exc:
                    errors.append(exc)

            thread = threading.Thread(target=mock, daemon=True)
            thread.start()
            result = subprocess.run(
                [str(executable), "--host", "127.0.0.1", "--port", str(port),
                 "--output", str(output), "--max-samples", "5"],
                capture_output=True, text=True, timeout=30)
            thread.join(timeout=2)
            assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
            assert not thread.is_alive(), "Mock did not finish"
            assert not errors, errors
            with output.open(newline="", encoding="utf-8") as file:
                rows = list(csv.DictReader(file))
            assert [row["sequence"] for row in rows] == ["1", "2", "4", "5", "1"], rows
            assert rows[1]["valid_mask"] == "0" and rows[1]["inside_temp_c"] == "nan"
            assert rows[-1]["boot_id"] == "456"
            assert rows[-1]["sound_rms"] == "12.50" and rows[-1]["sound_samples"] == "10000"
            assert all(row["received_utc"].endswith("Z") for row in rows)
            assert "Sequence gap" in result.stderr
            assert "New ESP32 boot" in result.stderr
            assert "Unexpected protocol" in result.stderr

        # Existing output must never be silently overwritten or joined to a torn row.
        for content in ("wrong header\n", "received_utc," + COLUMNS + "\npartial"):
            output.write_text(content, encoding="utf-8")
            result = subprocess.run([str(executable), "--output", str(output)],
                                    capture_output=True, timeout=5)
            assert result.returncode != 0
            assert output.read_text(encoding="utf-8") == content
        print("PASS: framing, malformed/binary/overlong data, reconnect, duplicate, gap, reboot, CSV protection")


if __name__ == "__main__":
    run(pathlib.Path(sys.argv[1]).resolve())
