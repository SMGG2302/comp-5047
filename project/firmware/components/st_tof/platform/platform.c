#include "vl53l0x_platform.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV d, uint8_t reg, uint8_t *p, uint32_t n)
{
    uint8_t buffer[257];
    if (n > 256) return VL53L0X_ERROR_INVALID_PARAMS;
    buffer[0] = reg;
    memcpy(buffer + 1, p, n);
    return i2c_master_transmit(d->i2c, buffer, n + 1, 50) == ESP_OK
        ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV d, uint8_t reg, uint8_t *p, uint32_t n)
{
    return i2c_master_transmit_receive(d->i2c, &reg, 1, p, n, 50) == ESP_OK
        ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV d, uint8_t r, uint8_t v)
{ return VL53L0X_WriteMulti(d,r,&v,1); }
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV d, uint8_t r, uint16_t v)
{ uint8_t b[]={v>>8,v}; return VL53L0X_WriteMulti(d,r,b,2); }
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV d, uint8_t r, uint32_t v)
{ uint8_t b[]={v>>24,v>>16,v>>8,v}; return VL53L0X_WriteMulti(d,r,b,4); }
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV d, uint8_t r, uint8_t *v)
{ return VL53L0X_ReadMulti(d,r,v,1); }
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV d, uint8_t r, uint16_t *v)
{ uint8_t b[2]={0}; int e=VL53L0X_ReadMulti(d,r,b,2); if(!e)*v=((uint16_t)b[0]<<8)|b[1]; return e; }
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV d, uint8_t r, uint32_t *v)
{ uint8_t b[4]={0}; int e=VL53L0X_ReadMulti(d,r,b,4); if(!e)*v=((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3]; return e; }
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV d, uint8_t r, uint8_t mask, uint8_t bits)
{ uint8_t v=0; int e=VL53L0X_RdByte(d,r,&v); return e?e:VL53L0X_WrByte(d,r,(v&mask)|bits); }
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV d)
{ (void)d; vTaskDelay(pdMS_TO_TICKS(1)); return 0; }
