#ifndef __BH1750_H_
#define __BH1750_H_


void BH1750_WriteByte(uint8_t Cmd);
void BH1750_Init(void);
uint16_t BH1750_ReceiveByte(void);
float BH1750_ReadLux(void);


#endif 

