#ifndef __AHT20_H
#define __AHT20_H

//重映射读和写
#define AHT20_ADDR_WRITE    0x70    // 0x38 << 1
#define AHT20_ADDR_READ     0x71    // (0x38 << 1) | 1

void    AHT20_Init(void);
uint8_t AHT20_ReadTempHumi(float *temp, float *humi);

#endif
