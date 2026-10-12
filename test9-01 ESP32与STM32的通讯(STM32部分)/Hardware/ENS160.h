#ifndef __ENS160_H
#define __ENS160_H

#include "stm32f10x.h"

//地址定义（ADD 悬空，7位地址 0x53）
#define ENS160_ADDR_WRITE    0xA6    // 0x53 << 1
#define ENS160_ADDR_READ     0xA7    // (0x53 << 1) | 1

//寄存器地址
#define ENS160_REG_PART_ID       0x00
#define ENS160_REG_OPMODE        0x10
#define ENS160_REG_CONFIG        0x11
#define ENS160_REG_COMMAND       0x12
#define ENS160_REG_TEMP_IN       0x13
#define ENS160_REG_RH_IN         0x15
#define ENS160_REG_DEVICE_STATUS 0x20
#define ENS160_REG_DATA_AQI      0x21
#define ENS160_REG_DATA_TVOC     0x22
#define ENS160_REG_DATA_ECO2     0x24

//操作模式
#define ENS160_OPMODE_DEEP_SLEEP 0x00
#define ENS160_OPMODE_IDLE       0x01
#define ENS160_OPMODE_STANDARD   0x02
#define ENS160_OPMODE_RESET      0xF0

//数据包结构体
typedef struct
{
    uint8_t  aqi;    // 1~5
    uint16_t tvoc;   // ppb
    uint16_t eco2;   // ppm
} ENS160_Data_t;

void    ENS160_Init(void);
uint8_t ENS160_ReadPartID(uint16_t *id);
void    ENS160_SetMode(uint8_t mode);
void    ENS160_SetEnv(float temp, float humi);
uint8_t ENS160_ReadData(ENS160_Data_t *data);

#endif
