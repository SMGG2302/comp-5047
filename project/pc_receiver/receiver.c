#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET socket_t;
#define SOCKET_BAD INVALID_SOCKET
#define close_socket closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int socket_t;
#define SOCKET_BAD (-1)
#define close_socket close
#endif

#include "telemetry.h"
#define CSV_HEADER "received_utc," HH_COLUMNS

static volatile sig_atomic_t stopped;
static void on_signal(int number) { (void)number; stopped = 1; }

static void wait_retry(void)
{
    for (int i = 0; i < 20 && !stopped; ++i) {
#ifdef _WIN32
        Sleep(100);
#else
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 100000000};
        nanosleep(&delay, NULL);
#endif
    }
}

static bool nonblocking(socket_t s, bool enabled)
{
#ifdef _WIN32
    u_long mode = enabled ? 1UL : 0UL;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) == 0;
#endif
}

static socket_t connect_to(const char *host, unsigned short port)
{
    socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == SOCKET_BAD) return SOCKET_BAD;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1 || !nonblocking(s, true)) goto failed;
    int result = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (result != 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) goto failed;
#else
        if (errno != EINPROGRESS) goto failed;
#endif
        fd_set write_set, error_set;
        FD_ZERO(&write_set); FD_ZERO(&error_set);
        FD_SET(s, &write_set); FD_SET(s, &error_set);
        struct timeval timeout = {5, 0};
#ifdef _WIN32
        result = select(0, NULL, &write_set, &error_set, &timeout);
#else
        result = select(s + 1, NULL, &write_set, &error_set, &timeout);
#endif
        if (result <= 0) goto failed;
        int so_error = 0;
#ifdef _WIN32
        int size = (int)sizeof(so_error);
#else
        socklen_t size = sizeof(so_error);
#endif
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&so_error, &size) != 0 || so_error) goto failed;
    }
    if (!nonblocking(s, false)) goto failed;
#ifdef _WIN32
    DWORD receive_timeout = 10000;
#else
    struct timeval receive_timeout = {10, 0};
#endif
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receive_timeout,
                   sizeof(receive_timeout)) != 0) goto failed;
    return s;
failed:
    close_socket(s);
    return SOCKET_BAD;
}

static FILE *open_csv(const char *path)
{
    FILE *file = fopen(path, "a+b");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) goto failed;
    long size = ftell(file);
    if (size < 0) goto failed;
    if (size == 0) {
        if (fprintf(file, CSV_HEADER "\n") < 0 || fflush(file) != 0) goto failed;
    } else {
        char header[HH_LINE_CAP];
        if (fseek(file, 0, SEEK_SET) != 0 || !fgets(header, sizeof(header), file)) goto failed;
        header[strcspn(header, "\r\n")] = '\0';
        if (strcmp(header, CSV_HEADER) != 0) {
            fprintf(stderr, "Existing CSV has a different header; choose a new file.\n");
            goto failed;
        }
        if (fseek(file, -1L, SEEK_END) != 0 || fgetc(file) != '\n') {
            fprintf(stderr, "Existing CSV ends with an incomplete line; choose a new file.\n");
            goto failed;
        }
    }
    if (fseek(file, 0, SEEK_END) != 0) goto failed;
    return file;
failed:
    fclose(file);
    return NULL;
}

static bool timestamp_utc(char *dest, size_t size)
{
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) return false;
    struct tm utc;
#ifdef _WIN32
    if (gmtime_s(&utc, &ts.tv_sec) != 0) return false;
#else
    if (!gmtime_r(&ts.tv_sec, &utc)) return false;
#endif
    char seconds[32];
    if (!strftime(seconds, sizeof(seconds), "%Y-%m-%dT%H:%M:%S", &utc)) return false;
    int n = snprintf(dest, size, "%s.%03ldZ", seconds, ts.tv_nsec / 1000000L);
    return n > 0 && (size_t)n < size;
}

static bool positive_uint(const char *str, unsigned long maximum, unsigned long *value)
{
    if (!*str) return false;
    for (const char *p = str; *p; ++p) if (*p < '0' || *p > '9') return false;
    char *end;
    errno = 0;
    unsigned long n = strtoul(str, &end, 10);
    if (errno || *end || !n || n > maximum) return false;
    *value = n;
    return true;
}

int main(int argc, char **argv)
{
    const char *host = "192.168.4.1";
    const char *output = "hollowhaven_data.csv";
    unsigned short port = 9000;
    unsigned long limit = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            puts("receiver [--host IPv4] [--port 9000] [--output file.csv] [--max-samples N]\n"
                 "Connect to the ESP32 Wi-Fi first. Ctrl+C stops. Existing compatible CSV is appended.");
            return 0;
        }
        if (i + 1 >= argc) { fprintf(stderr, "Missing option value. Use --help.\n"); return 2; }
        const char *option = argv[i];
        const char *value = argv[++i];
        unsigned long number;
        if (strcmp(option, "--host") == 0) host = value;
        else if (strcmp(option, "--output") == 0) output = value;
        else if (strcmp(option, "--port") == 0 && positive_uint(value, 65535, &number)) port = (unsigned short)number;
        else if (strcmp(option, "--max-samples") == 0 && positive_uint(value, 1000000000UL, &number)) limit = number;
        else { fprintf(stderr, "Invalid option/value: %s. Use --help.\n", option); return 2; }
    }
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
#endif
    struct in_addr check;
    if (inet_pton(AF_INET, host, &check) != 1) { fprintf(stderr, "Host must be an IPv4 address.\n"); return 2; }
    FILE *csv = open_csv(output);
    if (!csv) { fprintf(stderr, "Cannot open output safely: %s\n", output); return 1; }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Receiver -> %s:%u; CSV: %s\n", host, (unsigned)port, output);
    unsigned long accepted = 0;
    bool have_previous = false;
    uint32_t previous_boot = 0, previous_seq = 0;
    int exit_code = 0;
    while (!stopped && (!limit || accepted < limit)) {
        socket_t s = connect_to(host, port);
        if (s == SOCKET_BAD) {
            fprintf(stderr, "Connection unavailable; check Wi-Fi. Retrying in 2 seconds.\n");
            wait_retry();
            continue;
        }
        fprintf(stderr, "Connected. Waiting for protocol header.\n");
        bool greeted = false, dropping = false, end_connection = false;
        size_t used = 0;
        char line[HH_LINE_CAP];
        while (!stopped && !end_connection && (!limit || accepted < limit)) {
            char chunk[1024];
            int n = (int)recv(s, chunk, sizeof(chunk), 0);
            if (n <= 0) break;
            for (int i = 0; i < n; ++i) {
                unsigned char c = (unsigned char)chunk[i];
                if (c != '\n') {
                    if (c == '\0' || used + 1 >= sizeof(line)) dropping = true;
                    if (!dropping) line[used++] = (char)c;
                    continue;
                }
                if (dropping) {
                    fprintf(stderr, "Discarded overlong/binary record.\n");
                    dropping = false; used = 0;
                    continue;
                }
                line[used] = '\0';
                if (used && line[used - 1] == '\r') line[--used] = '\0';
                used = 0;
                if (!greeted) {
                    if (strcmp(line, HH_GREETING) != 0) {
                        fprintf(stderr, "Unexpected protocol header. Reconnecting.\n");
                        end_connection = true;
                        break;
                    }
                    greeted = true;
                    continue;
                }
                hh_sample_t sample;
                if (!hh_parse_sample(line, &sample)) {
                    fprintf(stderr, "Discarded malformed sample.\n");
                    continue;
                }
                if (have_previous && sample.boot_id == previous_boot && sample.sequence == previous_seq) continue;
                if (have_previous && sample.boot_id == previous_boot && sample.sequence != previous_seq + 1u)
                    fprintf(stderr, "Sequence gap/reordering: %" PRIu32 " -> %" PRIu32 "\n", previous_seq, sample.sequence);
                if (have_previous && sample.boot_id != previous_boot) fprintf(stderr, "New ESP32 boot detected.\n");
                char canonical[HH_LINE_CAP], timestamp[40];
                int length = hh_format_sample(canonical, sizeof(canonical), &sample);
                if (length < 0 || !timestamp_utc(timestamp, sizeof(timestamp))
                    || fprintf(csv, "%s,%s", timestamp, canonical) < 0 || fflush(csv) != 0) {
                    fprintf(stderr, "CSV write failed; stopping to avoid silent data loss.\n");
                    exit_code = 1; stopped = 1; break;
                }
                previous_boot = sample.boot_id; previous_seq = sample.sequence; have_previous = true;
                ++accepted;
                printf("#%" PRIu32 " T=%.2f C RH=%.2f %% outside=%.2f C wick=%d light=%d water=%d valid=0x%02X sound_rms=%.2f p2p=%d n=%" PRIu32 " clip=%" PRIu32 " adc_gaps=%" PRIu32 " distance_mm=%d animal_present=%s presence_valid=%s recording=%s\n",
                    sample.sequence, (double)sample.inside_temp_c, (double)sample.inside_rh_pct,
                    (double)sample.outside_temp_c, sample.wick_raw, sample.light_raw,
                    sample.water_raw, sample.valid_mask, (double)sample.sound_rms, sample.sound_p2p,
                    sample.sound_samples, sample.sound_clipped, sample.adc_overflows,
                    sample.distance_mm, sample.animal_present?"true":"false",
                    sample.presence_valid?"true":"false", sample.audio_recording?"true":"false");
                if (limit && accepted >= limit) break;
            }
        }
        close_socket(s);
        if (!stopped && (!limit || accepted < limit)) {
            fprintf(stderr, "Disconnected/receive timeout; partial line discarded. Retrying.\n");
            wait_retry();
        }
    }
    if (fclose(csv) != 0) exit_code = 1;
#ifdef _WIN32
    WSACleanup();
#endif
    printf("Saved %lu samples.\n", accepted);
    return exit_code;
}
