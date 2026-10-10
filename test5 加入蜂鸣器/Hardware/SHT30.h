#ifndef  __SHT30_H_
#define  __SHT30_H_


void SHT30_Init(void);
uint8_t SHT30_ReadTempHumi(float *temp, float *humi);


#endif
