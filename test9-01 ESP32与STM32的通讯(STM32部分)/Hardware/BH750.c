#include "stm32f10x.h"                  // Device header
#include "MYI2C.h"
#include "Delay.h"

#define BH1750_Address           0x46

#define BH1750_POWER_ON          0x01
#define BH1750_RESET             0x07
#define BH1750_CONT_H_RES_MODE   0x10   // 连续高分辨率模式

void BH1750_WriteByte(uint8_t Cmd)
{
    uint8_t ack;

    MYI2C_Start();

    MYI2C_SendByte(BH1750_Address);      // 写地址 0x46
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(Cmd);                 // 直接发命令字节
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_Stop();
}

void BH1750_Init(void)
{
    MYI2C_Init();

    BH1750_WriteByte(BH1750_POWER_ON);      // 上电
    Delay_ms(10);

    BH1750_WriteByte(BH1750_RESET);         // 复位
    Delay_ms(10);

    BH1750_WriteByte(BH1750_CONT_H_RES_MODE); // 连续高分辨率模式
    Delay_ms(180);                          // 等待首次测量完成
}

uint16_t BH1750_ReceiveByte(void)
{
    uint8_t ack;
    uint8_t high, low;
    uint16_t raw;

    MYI2C_Start();

    MYI2C_SendByte(BH1750_Address | 0x01);  // 读地址 0x47
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return 0; }

    high = MYI2C_ReceiveByte();                // 读高字节
    MYI2C_SendAck(0);                       // 发送 ACK，继续读

    low = MYI2C_ReceiveByte();                // 读低字节
    MYI2C_SendAck(1);                       // 发送 NACK，结束读取

    MYI2C_Stop();

    raw = ((uint16_t)high << 8) | low;
    return raw;
}

float BH1750_ReadLux(void)  
{
    uint16_t raw;
    float lux;

    raw = BH1750_ReceiveByte();          // 读取原始数据 (0 ~ 65535)
    
    // BH1750 连续高分辨率模式下的物理公式：Lux = raw / 1.2
    lux = (float)raw / 1.2f;             

    return lux;
}

/*
注意事项：最后一个读取百分比的函数刚开始是打算直接用raw得到的数字直接表示百分比的，但是实际上如果用raw的数字
来表示百分比的话，那么室内，手电筒直照等情况也就只会有百分之零点零几到百分之几之间不等，最后只好使用官方手册推
荐的lux来表示光照强度了。
*/
