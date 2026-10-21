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

#include "audio_wire.h"
#include <io.h>
#include <direct.h>

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
    DWORD receive_timeout = 90000;
#else
    struct timeval receive_timeout = {90, 0};
#endif
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receive_timeout,
                   sizeof(receive_timeout)) != 0) goto failed;
    return s;
failed:
    close_socket(s);
    return SOCKET_BAD;
}

static bool receive_all(socket_t s, void *buffer, size_t n)
{
    char *p=buffer;
    while(n && !stopped) {
        int k=recv(s,p,(int)n,0);
        if(k<=0)return false;
        p+=k; n-=(size_t)k;
    }
    return n==0;
}

static bool send_ack(socket_t s, const uint8_t *h)
{
    uint8_t ack[12]; memcpy(ack,"ACK1",4); memcpy(ack+4,h+4,8);
    size_t sent=0;
    while(sent<sizeof(ack)) {
        int n=send(s,(const char*)ack+sent,(int)(sizeof(ack)-sent),0);
        if(n<=0)return false;
        sent+=(size_t)n;
    }
    return true;
}

static void wav_header(uint8_t *wav, uint32_t bytes, uint32_t rate)
{
    memset(wav,0,44); memcpy(wav,"RIFF",4); hh_put32(wav+4,bytes+36);
    memcpy(wav+8,"WAVEfmt ",8); hh_put32(wav+16,16);
    wav[20]=1; wav[22]=1; hh_put32(wav+24,rate); hh_put32(wav+28,rate*2);
    wav[32]=2; wav[34]=16; memcpy(wav+36,"data",4); hh_put32(wav+40,bytes);
}

/* 1 saved / duplicate, 0 transient transport error, -1 local disk or protocol error. */
static int receive_clip(socket_t sock, const char *folder)
{
    uint8_t h[32], wav[44], buffer[4096];
    if(!receive_all(sock,h,sizeof(h)))return 0;
    uint32_t count=hh_le32(h+12), rate=hh_le32(h+16), duration=hh_le32(h+28);
    if(memcmp(h,"HHW1",4)!=0 || !count || count>HH_AUDIO_MAX_BYTES/2
       || rate<1000 || rate>10000 || !((duration>=29000 && duration<=32000) || (duration>=59000 && duration<=62000))) {
        fprintf(stderr,"Invalid audio header; stopped.\n"); return -1;
    }
    uint32_t bytes=count*2, crc=0xffffffffu;
    char target[1024], temporary[1040];
    int length=snprintf(target,sizeof(target),"%s/clip_%08" PRIx32 "_%06" PRIu32 "_gaps%" PRIu32 ".wav",
        folder,hh_le32(h+4),hh_le32(h+8),hh_le32(h+24));
    if(length<0 || (size_t)length>=sizeof(target))return -1;
    snprintf(temporary,sizeof(temporary),"%s.part",target);
    FILE *file=fopen(temporary,"wb");
    if(!file) { perror("Cannot create WAV");return -1; }
    wav_header(wav,bytes,rate);
    bool disk_ok=fwrite(wav,1,sizeof(wav),file)==sizeof(wav);
    bool network_ok=true;
    for(uint32_t pos=0;disk_ok && pos<bytes;) {
        uint32_t n=bytes-pos; if(n>sizeof(buffer))n=sizeof(buffer);
        if(!receive_all(sock,buffer,n)) { network_ok=false;break; }
        crc=hh_crc(crc,buffer,n);
        disk_ok=fwrite(buffer,1,n,file)==n;
        pos+=n;
    }
    if(fflush(file)!=0 || _commit(_fileno(file))!=0)disk_ok=false;
    if(fclose(file)!=0)disk_ok=false;
    if(!disk_ok || !network_ok || (crc^0xffffffffu)!=hh_le32(h+20)) {
        remove(temporary);
        fprintf(stderr,"Clip incomplete / CRC mismatch / disk error; not acknowledged.\n");
        return disk_ok?0:-1;
    }
    /* Replace only the same boot/sequence file, atomically after a verified retry. */
    if(!MoveFileExA(temporary,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr,"Cannot finalize WAV (Windows error %lu); not acknowledged.\n",GetLastError());return -1;
    }
    printf("Saved %s | %.3f s | %" PRIu32 " Hz | gap events=%" PRIu32 "\n",
        target,(double)count/rate,rate,hh_le32(h+24));
    if(hh_le32(h+24))fprintf(stderr,"WARNING: this clip has sampling gaps; do not treat it as uninterrupted audio.\n");
    return send_ack(sock,h)?1:0;
}

int main(int argc, char **argv)
{
    const char *host="192.168.4.1", *folder="recordings";
    unsigned limit=0, port=9001;
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i],"--help")==0) {
            puts("audio_receiver [--host IPv4] [--port 9001] [--output-dir folder] [--max-clips N]\n"
                 "One WAV per presence event (30 seconds); also accepts legacy minute clips. Ctrl+C stops. Connect to ESP32 Wi-Fi first.");return 0;
        }
        if(i+1>=argc)return 2;
        const char *key=argv[i++], *value=argv[i];
        if(strcmp(key,"--host")==0)host=value;
        else if(strcmp(key,"--output-dir")==0)folder=value;
        else if(strcmp(key,"--port")==0 || strcmp(key,"--max-clips")==0) {
            char *end; unsigned long n=strtoul(value,&end,10);
            if(!*value || *end || !n || n>65535)return 2;
            if(strcmp(key,"--port")==0)port=(unsigned)n;else limit=(unsigned)n;
        } else return 2;
    }
    if(_mkdir(folder)!=0 && errno!=EEXIST) { perror("Output directory");return 1; }
    WSADATA wsa; if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)return 1;
    signal(SIGINT,on_signal);signal(SIGTERM,on_signal);setvbuf(stdout,NULL,_IONBF,0);
    printf("Audio receiver: %s:%u -> %s. Waiting for completed event clips...\n",host,port,folder);
    unsigned saved=0;int result=0;
    while(!stopped && (!limit || saved<limit)) {
        socket_t s=connect_to(host,(unsigned short)port);
        if(s==SOCKET_BAD) { fprintf(stderr,"Wi-Fi unavailable; retrying...\n");wait_retry();continue; }
        while(!stopped && (!limit || saved<limit)) {
            int status=receive_clip(s,folder);
            if(status<0) { result=1;stopped=1;break; }
            if(!status)break;
            ++saved;
        }
        close_socket(s);
        if(!stopped && (!limit || saved<limit))wait_retry();
    }
    WSACleanup();return result;
}

