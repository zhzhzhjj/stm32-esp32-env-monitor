#include "stm32f10x.h"
#include "ENS160.h"
#include "MYI2C.h"
#include "Delay.h"

//连续写：先发寄存器地址，再发 len 个字节
static void ENS160_WriteRegs(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t ack, i;

    MYI2C_Start();

    MYI2C_SendByte(ENS160_ADDR_WRITE);
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(reg);
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    for (i = 0; i < len; i++)
    {
        MYI2C_SendByte(buf[i]);
        ack = MYI2C_ReceiveAck();
        if (ack != 0) { MYI2C_Stop(); return; }
    }

    MYI2C_Stop();
}

//连续读：先写寄存器地址，重复起始后读 len 个字节
static void ENS160_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t ack, i;

    MYI2C_Start();

    MYI2C_SendByte(ENS160_ADDR_WRITE);
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_SendByte(reg);
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    MYI2C_Start();

    MYI2C_SendByte(ENS160_ADDR_READ);
    ack = MYI2C_ReceiveAck();
    if (ack != 0) { MYI2C_Stop(); return; }

    for (i = 0; i < len; i++)
    {
        buf[i] = MYI2C_ReceiveByte();
        if (i < len - 1)
            MYI2C_SendAck(0);   // 后面还有，继续读
        else
            MYI2C_SendAck(1);   // 最后一字节，NACK
    }

    MYI2C_Stop();
}

//写 8 位寄存器
static void ENS160_WriteReg(uint8_t reg, uint8_t data)
{
    ENS160_WriteRegs(reg, &data, 1);
}

//读 8 位寄存器
static uint8_t ENS160_ReadReg(uint8_t reg)
{
    uint8_t data;
    ENS160_ReadRegs(reg, &data, 1);
    return data;
}

//写 16 位寄存器（小端序：低字节在前）
static void ENS160_WriteReg16(uint8_t reg, uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(data & 0xFF);   // 低字节
    buf[1] = (uint8_t)(data >> 8);     // 高字节
    ENS160_WriteRegs(reg, buf, 2);
}

//读 16 位寄存器（小端序）
static uint16_t ENS160_ReadReg16(uint8_t reg)
{
    uint8_t buf[2];
    ENS160_ReadRegs(reg, buf, 2);
    return ((uint16_t)buf[1] << 8) | buf[0];
}

//初始化
void ENS160_Init(void)
{
    MYI2C_Init();
    Delay_ms(20);                               // 上电稳定

    ENS160_SetMode(ENS160_OPMODE_IDLE);         // 先进入 IDLE
    Delay_ms(5);

    ENS160_WriteReg(ENS160_REG_COMMAND, 0xCC);  // 清除 GPR
    Delay_ms(5);
}

//读 PART_ID
// 返回值：0 = 成功，1 = 失败。正常 ID 为 0x0160
uint8_t ENS160_ReadPartID(uint16_t *id)
{
    *id = ENS160_ReadReg16(ENS160_REG_PART_ID);
    return (*id == 0x0160) ? 0 : 1;
}

//设置操作模式
void ENS160_SetMode(uint8_t mode)
{
    ENS160_WriteReg(ENS160_REG_OPMODE, mode);
}

//写入温湿度补偿
// temp 单位：℃，humi 单位：%RH
void ENS160_SetEnv(float temp, float humi)
{
    //温度：实际值 × 64，转 int16_t
    int16_t t = (int16_t)(temp * 64.0f);
    ENS160_WriteReg16(ENS160_REG_TEMP_IN, (uint16_t)t);

    //湿度：实际值 × 512，转 uint16_t
    uint16_t rh = (uint16_t)(humi * 512.0f);
    ENS160_WriteReg16(ENS160_REG_RH_IN, rh);
}

//读取空气质量数据
// 返回值：0 = 成功，1 = 无新数据或通信失败
uint8_t ENS160_ReadData(ENS160_Data_t *data)
{
    uint8_t status;
    uint8_t buf[2];

    //检查 DEVICE_STATUS 的 bit1（NEWDAT）
    status = ENS160_ReadReg(ENS160_REG_DEVICE_STATUS);
    if ((status & 0x02) == 0)
        return 1;

    //AQI：1 字节
    data->aqi = ENS160_ReadReg(ENS160_REG_DATA_AQI);

    //TVOC：2 字节小端
    ENS160_ReadRegs(ENS160_REG_DATA_TVOC, buf, 2);
    data->tvoc = ((uint16_t)buf[1] << 8) | buf[0];

    //eCO2：2 字节小端
    ENS160_ReadRegs(ENS160_REG_DATA_ECO2, buf, 2);
    data->eco2 = ((uint16_t)buf[1] << 8) | buf[0];

    return 0;
}

