#include "stm32f10x.h"
#include "SHT30.h"
#include "MYI2C.h"
#include "delay.h"

//地址定义（ADDR 接地）
#define SHT30_ADDR_WRITE    0x88    // 0x44 << 1
#define SHT30_ADDR_READ     0x89    // (0x44 << 1) | 1

//CRC-8 校验
//SHT30 使用多项式 0x31，初值 0xFF
static uint8_t SHT30_CRC8(uint8_t data1, uint8_t data2)
{
    uint8_t crc = 0xFF;
    uint8_t i;

    //第一个字节
    crc ^= data1;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x80) crc = (crc << 1) ^ 0x31;
        else            crc <<= 1;
    }

    //第二个字节
    crc ^= data2;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x80) crc = (crc << 1) ^ 0x31;
        else            crc <<= 1;
    }

    return crc;
}

//发送两字节命令
static void SHT30_SendCmd(uint8_t cmdHigh, uint8_t cmdLow)
{
    uint8_t ack;

    MYI2C_Start();

    MYI2C_SendByte(SHT30_ADDR_WRITE);       // 写地址 0x88
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(cmdHigh);                // 命令高字节
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(cmdLow);                 // 命令低字节
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_Stop();
}


void SHT30_Init(void)
{
    MYI2C_Init();
    Delay_ms(10);                           // 上电稳定
}

//读取温湿度
// 返回值：0 = 成功，1 = 通信失败或 CRC 校验失败
// temp 单位：℃，humi 单位：%RH */
uint8_t SHT30_ReadTempHumi(float *temp, float *humi)
{
    uint8_t ack;
    uint8_t tMSB, tLSB, tCRC;
    uint8_t hMSB, hLSB, hCRC;
    uint16_t rawTemp, rawHumi;

    //发送单次高重复性测量命令 0x24 0x00
    SHT30_SendCmd(0x24, 0x00);
    Delay_ms(20);                           // 高重复性约 15ms，留点余量

    //连续读 6 字节
    MYI2C_Start();

    MYI2C_SendByte(SHT30_ADDR_READ);        // 读地址 0x89
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return 1; }

    tMSB = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 温度高字节
    tLSB = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 温度低字节
    tCRC = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 温度 CRC
    hMSB = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 湿度高字节
    hLSB = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 湿度低字节
    hCRC = MYI2C_ReceiveByte();  MYI2C_SendAck(1);   // 湿度 CRC，NACK 结束

    MYI2C_Stop();

    //CRC 校验
    if (SHT30_CRC8(tMSB, tLSB) != tCRC) return 1;
    if (SHT30_CRC8(hMSB, hLSB) != hCRC) return 1;

    //拼接 16 位原始值
    rawTemp = ((uint16_t)tMSB << 8) | tLSB;
    rawHumi = ((uint16_t)hMSB << 8) | hLSB;

    //换算成物理量
    *temp = -45.0f + 175.0f * rawTemp / 65535.0f;
    *humi = 100.0f * rawHumi / 65535.0f;

    return 0;
}

