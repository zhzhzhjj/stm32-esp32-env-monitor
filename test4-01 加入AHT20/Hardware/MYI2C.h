#ifndef __MYI2C_H_
#define __MYI2C_H_

#include "stm32f10x.h"   // 必须加，否则 uint8_t 会报错

//对I2C引脚的重映射，方便以后修改
#define MYI2C_SCL_PORT      GPIOB
#define MYI2C_SCL_PIN       GPIO_Pin_0
#define MYI2C_SCL_RCC       RCC_APB2Periph_GPIOB

#define MYI2C_SDA_PORT      GPIOB
#define MYI2C_SDA_PIN       GPIO_Pin_1
#define MYI2C_SDA_RCC       RCC_APB2Periph_GPIOB

void MYI2C_Init(void);
void MYI2C_Start(void);
void MYI2C_Stop(void);
void MYI2C_SendByte(uint8_t Byte);
uint8_t MYI2C_ReceiveByte(void);
void MYI2C_SendAck(uint8_t AckBit);
uint8_t MYI2C_ReceiveAck(void);

#endif

