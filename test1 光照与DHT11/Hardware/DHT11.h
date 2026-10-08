#ifndef __DHT11_H_
#define __DHT11_H_

void DHT11_Init(void);
void DHT11_Start(void);
uint8_t DHT11_ReadBit(void);
uint8_t DHT11_ReadByte(void);
uint8_t DHT11_Read(float *temp, float *humi);


#endif
