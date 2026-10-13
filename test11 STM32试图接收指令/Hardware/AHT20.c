#include "stm32f10x.h"
#include "AHT20.h"
#include "MYI2C.h"
#include "Delay.h"



//CRC-8 校验
//AHT20 使用多项式 0x31，初值 0xFF，对前 6 字节计算
static uint8_t AHT20_CRC8(uint8_t d0, uint8_t d1, uint8_t d2,
                          uint8_t d3, uint8_t d4, uint8_t d5)
{
    uint8_t crc = 0xFF;
    uint8_t i;

    crc ^= d0;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }
    crc ^= d1;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }
    crc ^= d2;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }
    crc ^= d3;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }
    crc ^= d4;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }
    crc ^= d5;
    for (i = 0; i < 8; i++) { if (crc & 0x80) crc = (crc << 1) ^ 0x31; else crc <<= 1; }

    return crc;
}

//发送命令
//hasParam: 1=发命令 + 两个参数，0=只发命令
static void AHT20_SendCmd(uint8_t cmd, uint8_t p1, uint8_t p2, uint8_t hasParam)
{
    uint8_t ack;

    MYI2C_Start();

    MYI2C_SendByte(AHT20_ADDR_WRITE);       // 写地址 0x70
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(cmd);                    // 命令码
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    if (hasParam)
    {
        MYI2C_SendByte(p1);                 // 参数 1
        ack = MYI2C_ReceiveAck();
        if (ack != 0) { MYI2C_Stop(); return; }

        MYI2C_SendByte(p2);                 // 参数 2
        ack = MYI2C_ReceiveAck();
        if (ack != 0) { MYI2C_Stop(); return; }
    }

    MYI2C_Stop();
}


void AHT20_Init(void)
{
    MYI2C_Init();
    Delay_ms(40);                           // 上电稳定，手册要求 >40ms

    AHT20_SendCmd(0xBE, 0x08, 0x00, 1);     // 初始化/校准命令
    Delay_ms(10);
}

//读取温湿度
// 返回值：0 = 成功，1 = 通信失败或 CRC 校验失败
// temp 单位：℃，humi 单位：%RH */
uint8_t AHT20_ReadTempHumi(float *temp, float *humi)
{
    uint8_t ack;
    uint8_t st;
    uint8_t d0, d1, d2, d3, d4, crc;
    uint32_t rawTemp, rawHumi;

    //触发测量命令 0xAC 0x33 0x00
    AHT20_SendCmd(0xAC, 0x33, 0x00, 1);
    Delay_ms(80);                           // 测量约 75~80ms

    //连续读 7 字节
    MYI2C_Start();

    MYI2C_SendByte(AHT20_ADDR_READ);        // 读地址 0x71
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return 1; }

    st  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 状态字
    d0  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 湿度高 8 位
    d1  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 湿度中 8 位
    d2  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 湿度低 4 位 + 温度高 4 位
    d3  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 温度中 8 位
    d4  = MYI2C_ReceiveByte();  MYI2C_SendAck(0);   // 温度低 8 位
    crc = MYI2C_ReceiveByte();  MYI2C_SendAck(1);   // CRC，NACK 结束

    MYI2C_Stop();

    //CRC 校验
    if (AHT20_CRC8(st, d0, d1, d2, d3, d4) != crc) return 1;

    //拼接 20 位原始值
    //湿度：d0 为高 8 位，d1 为中 8 位，d2 的高 4 位为低 4 位
    rawHumi = ((uint32_t)d0 << 12) | ((uint32_t)d1 << 4) | (d2 >> 4);

    //温度：d2 的低 4 位为高 4 位，d3 为中 8 位，d4 为低 8 位
    rawTemp = ((uint32_t)(d2 & 0x0F) << 16) | ((uint32_t)d3 << 8) | d4;

    //换算成物理量
    *humi = (float)rawHumi / 1048576.0f * 100.0f;
    *temp = (float)rawTemp / 1048576.0f * 200.0f - 50.0f;

    return 0;
}
