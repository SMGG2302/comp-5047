#include "audio_record.h"
#include "audio_wire.h"
#include "project_config.h"
#include "esp_timer.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"

#define SLOT_SIZE 0xa0000u
#define SLOTS 4
#define DATA_START 4096u
#define BLOCK_SAMPLES 512
typedef struct { int64_t end_us; uint32_t count; int16_t pcm[BLOCK_SAMPLES]; } block_t;
typedef struct { uint8_t header[HH_AUDIO_HEADER]; bool pending, pinned; } slot_t;
static const esp_partition_t *partition;
static QueueHandle_t queue;
static SemaphoreHandle_t lock;
static slot_t slots[SLOTS];
static uint32_t boot_id, sequence;
static portMUX_TYPE gap_lock = portMUX_INITIALIZER_UNLOCKED;
static uint32_t gaps;
static bool active;
static int64_t trigger_us;

bool hh_audio_is_recording(void)
{
    portENTER_CRITICAL(&gap_lock); bool value=active; portEXIT_CRITICAL(&gap_lock);
    return value;
}
bool hh_audio_trigger(void)
{
    portENTER_CRITICAL(&gap_lock);
    bool accepted=!active;
    if(accepted) { active=true; trigger_us=esp_timer_get_time(); }
    portEXIT_CRITICAL(&gap_lock);
    if(accepted)ESP_LOGI("record","Presence event: recording 30 seconds");
    return accepted;
}
static void recording_done(void)
{
    portENTER_CRITICAL(&gap_lock); active=false; portEXIT_CRITICAL(&gap_lock);
}

void hh_audio_gap(void)
{ portENTER_CRITICAL(&gap_lock); ++gaps; portEXIT_CRITICAL(&gap_lock); }
static uint32_t gap_count(void)
{ uint32_t n; portENTER_CRITICAL(&gap_lock); n=gaps; portEXIT_CRITICAL(&gap_lock); return n; }

void hh_audio_push(const int16_t *pcm, size_t count, int64_t end_us)
{
    if (!count || !hh_audio_is_recording()) return;
    assert(count <= BLOCK_SAMPLES);
    block_t b = {.end_us=end_us, .count=(uint32_t)count};
    memcpy(b.pcm, pcm, count*sizeof(*pcm));
    if (xQueueSend(queue, &b, 0) != pdTRUE) hh_audio_gap();
}

static bool valid_header(const uint8_t *h)
{
    return memcmp(h,"HHW1",4)==0 && hh_le32(h+12)>0 && hh_le32(h+12)<=HH_AUDIO_MAX_BYTES/2
        && hh_le32(h+16)>=1000 && hh_le32(h+16)<=10000
        && ((hh_le32(h+28)>=29000 && hh_le32(h+28)<=32000) || (hh_le32(h+28)>=59000 && hh_le32(h+28)<=62000));
}

static int begin_slot(int next)
{
    xSemaphoreTake(lock,portMAX_DELAY);
    int chosen=-1;
    /* Prefer free/acknowledged slots, otherwise overwrite oldest cyclic slot. */
    for (int pass=0;pass<2 && chosen<0;++pass)
        for (int j=0;j<SLOTS;++j) {
            int k=(next+j)%SLOTS;
            if (!slots[k].pinned && (pass || !slots[k].pending)) { chosen=k; break; }
        }
    assert(chosen>=0); /* At most one network reader can pin a slot. */
    if (slots[chosen].pending) ESP_LOGW("record","Cache full: overwriting unacknowledged clip");
    slots[chosen].pending=false;
    ESP_ERROR_CHECK(esp_partition_erase_range(partition, chosen*SLOT_SIZE, DATA_START));
    xSemaphoreGive(lock);
    return chosen;
}

static void recorder(void *arg)
{
    (void)arg;
    static uint8_t page[4096];
    block_t b;
    int slot=-1, next_slot=0;
    uint32_t used=0, bytes=0, offset=DATA_START, crc=0xffffffffu, start_gaps=gap_count();
    int64_t start=0;
    for (;;) {
        if (xQueueReceive(queue,&b,pdMS_TO_TICKS(500))!=pdTRUE) {
            portENTER_CRITICAL(&gap_lock);
            bool timed_out=active && esp_timer_get_time()-trigger_us>35000000;
            portEXIT_CRITICAL(&gap_lock);
            if(timed_out) {
                ESP_LOGE("record","Recording timed out; incomplete clip discarded");
                slot=-1; start=0; recording_done();
            }
            continue;
        }
        portENTER_CRITICAL(&gap_lock);
        bool accept=active && b.end_us>=trigger_us;
        portEXIT_CRITICAL(&gap_lock);
        if(!accept)continue;
        if(slot<0) {
            slot=begin_slot(next_slot); used=bytes=0; offset=DATA_START;
            crc=0xffffffffu; start=b.end_us; start_gaps=gap_count();
        }
        if (bytes+b.count*2>HH_AUDIO_MAX_BYTES) {
            ESP_LOGE("record","Audio slot capacity exceeded; discarding incomplete clip");
            hh_audio_gap(); next_slot=(slot+1)%SLOTS;
            slot=-1; start=0; recording_done(); continue;
        }
        const uint8_t *source=(const uint8_t *)b.pcm;
        uint32_t left=b.count*2;
        crc=hh_crc(crc,source,left); bytes+=left;
        while (left) {
            uint32_t n=sizeof(page)-used; if(n>left)n=left;
            memcpy(page+used,source,n); used+=n; source+=n; left-=n;
            if (used==sizeof(page)) {
                ESP_ERROR_CHECK(esp_partition_erase_range(partition,slot*SLOT_SIZE+offset,sizeof(page)));
                ESP_ERROR_CHECK(esp_partition_write(partition,slot*SLOT_SIZE+offset,page,used));
                offset+=used; used=0;
            }
        }
        if (b.end_us-start < (int64_t)HH_AUDIO_DURATION_MS*1000) continue;
        if (used) {
            ESP_ERROR_CHECK(esp_partition_erase_range(partition,slot*SLOT_SIZE+offset,sizeof(page)));
            ESP_ERROR_CHECK(esp_partition_write(partition,slot*SLOT_SIZE+offset,page,used));
        }
        uint32_t duration=(uint32_t)((b.end_us-start)/1000);
        uint32_t samples=bytes/2;
        uint32_t rate=(uint32_t)(((uint64_t)samples*1000000+(b.end_us-start)/2)/(b.end_us-start));
        uint8_t meta[40]; memset(meta,0xff,sizeof(meta)); memcpy(meta,"HHW1",4);
        hh_put32(meta+4,boot_id); hh_put32(meta+8,++sequence); hh_put32(meta+12,samples);
        hh_put32(meta+16,rate); hh_put32(meta+20,crc^0xffffffffu);
        hh_put32(meta+24,gap_count()-start_gaps); hh_put32(meta+28,duration);
        hh_put32(meta+36,hh_crc(0xffffffffu,meta,32)^0xffffffffu);
        xSemaphoreTake(lock,portMAX_DELAY);
        ESP_ERROR_CHECK(esp_partition_write(partition,slot*SLOT_SIZE,meta,sizeof(meta)));
        memcpy(slots[slot].header,meta,32); slots[slot].pending=true;
        xSemaphoreGive(lock);
        ESP_LOGI("record","Event clip ready: clip=%lu samples=%lu rate=%lu duration_ms=%lu gaps=%lu",
            (unsigned long)sequence,(unsigned long)samples,(unsigned long)rate,(unsigned long)duration,
            (unsigned long)hh_le32(meta+24));
        next_slot=(slot+1)%SLOTS; slot=-1; start=0;
        recording_done();
    }
}

static bool transfer(int fd, void *buffer, size_t n, bool sending)
{
    uint8_t *p=buffer;
    while(n) {
        int k=sending?send(fd,p,n,0):recv(fd,p,n,0);
        if(k<0 && errno==EINTR)continue;
        if(k<=0)return false;
        p+=k; n-=(size_t)k;
    }
    return true;
}

static void client(int fd)
{
    struct timeval timeout={.tv_sec=10,.tv_usec=0};
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    uint8_t buffer[2048], header[32], ack[12];
    for (;;) {
        int slot=-1;
        xSemaphoreTake(lock,portMAX_DELAY);
        for(int k=0;k<SLOTS;++k) if(slots[k].pending) {
            slot=k; slots[k].pinned=true; memcpy(header,slots[k].header,32); break;
        }
        xSemaphoreGive(lock);
        if(slot<0) {
            /* Detect a disconnected idle client without sending audio before a minute is ready. */
            fd_set fds; FD_ZERO(&fds); FD_SET(fd,&fds); struct timeval poll={0};
            if(select(fd+1,&fds,NULL,NULL,&poll)!=0)return;
            vTaskDelay(pdMS_TO_TICKS(200)); continue;
        }
        bool ok=transfer(fd,header,32,true);
        uint32_t bytes=hh_le32(header+12)*2;
        for(uint32_t pos=0;ok && pos<bytes;) {
            uint32_t n=bytes-pos; if(n>sizeof(buffer))n=sizeof(buffer);
            ok=esp_partition_read(partition,slot*SLOT_SIZE+DATA_START+pos,buffer,n)==ESP_OK;
            if(ok)ok=transfer(fd,buffer,n,true);
            pos+=n;
        }
        if(ok)ok=transfer(fd,ack,12,false) && memcmp(ack,"ACK1",4)==0
            && hh_le32(ack+4)==hh_le32(header+4) && hh_le32(ack+8)==hh_le32(header+8);
        xSemaphoreTake(lock,portMAX_DELAY);
        if(ok) {
            uint32_t acknowledged=0;
            ESP_ERROR_CHECK(esp_partition_write(partition,slot*SLOT_SIZE+32,&acknowledged,4));
            slots[slot].pending=false;
        }
        slots[slot].pinned=false;
        xSemaphoreGive(lock);
        if(!ok)return;
        ESP_LOGI("record","Computer acknowledged clip %lu",(unsigned long)hh_le32(header+8));
    }
}

static void server(void *arg)
{
    (void)arg;
    for(;;) {
        int fd=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
        if(fd<0) { vTaskDelay(pdMS_TO_TICKS(1000));continue; }
        int reuse=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
        struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(9001),.sin_addr.s_addr=htonl(INADDR_ANY)};
        if(bind(fd,(struct sockaddr*)&a,sizeof(a))<0 || listen(fd,1)<0) {
            close(fd); vTaskDelay(pdMS_TO_TICKS(1000));continue;
        }
        ESP_LOGI("record","Event audio server: TCP port 9001");
        for(;;) {
            int c=accept(fd,NULL,NULL); if(c<0)break;
            client(c); shutdown(c,SHUT_RDWR);close(c);
        }
        close(fd);
    }
}

void hh_audio_init(uint32_t boot)
{
    boot_id=boot;
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,0x40,"audio");
    assert(partition && partition->size>=SLOTS*SLOT_SIZE);
    lock=xSemaphoreCreateMutex(); queue=xQueueCreate(16,sizeof(block_t)); assert(lock && queue);
    for(int k=0;k<SLOTS;++k) {
        uint8_t meta[40]; ESP_ERROR_CHECK(esp_partition_read(partition,k*SLOT_SIZE,meta,sizeof(meta)));
        if(valid_header(meta) && hh_le32(meta+32)==0xffffffffu
           && hh_le32(meta+36)==(hh_crc(0xffffffffu,meta,32)^0xffffffffu)) {
            memcpy(slots[k].header,meta,32);slots[k].pending=true;
        }
    }
    BaseType_t result=xTaskCreate(recorder,"audio_flash",6144,NULL,5,NULL); assert(result==pdPASS);
    result=xTaskCreate(server,"audio_send",6144,NULL,4,NULL); assert(result==pdPASS);
}
