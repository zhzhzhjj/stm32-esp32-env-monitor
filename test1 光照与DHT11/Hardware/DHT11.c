#include "stm32f10x.h"
#include "Delay.h"

/* 端口宏定义，方便以后改引脚 */
#define DHT11_PORT   GPIOB
#define DHT11_PIN    GPIO_Pin_12       // ← 改成 PB12
#define DHT11_RCC    RCC_APB2Periph_GPIOB

/* ---------- 1. 初始化：PB12 开漏输出 ---------- */
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;      // ← PB12
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);

    GPIO_SetBits(DHT11_PORT, DHT11_PIN);            // 释放总线
}

/* ---------- 2. 起始信号 ---------- */
void DHT11_Start(void)
{
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);          // 拉低 ≥18ms
    Delay_ms(20);
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);            // 释放
    Delay_us(30);
}

/* ---------- 3. 新增：带超时的等待电平函数 ---------- */
static uint8_t DHT11_WaitLevel(BitAction level, uint16_t timeout_us)
{
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) != level) {
        if (--timeout_us == 0) return 1;            // 超时返回 1
        Delay_us(1);
    }
    return 0;
}

/* ---------- 4. 读一个位（修正时序） ---------- */
uint8_t DHT11_ReadBit(void)
{
    /* 每位以 50us 低电平开始 */
    if (DHT11_WaitLevel(Bit_RESET, 200)) return 0;  // 等低电平
    if (DHT11_WaitLevel(Bit_SET,   200)) return 0;  // 等上升沿

    Delay_us(40);                                   // 40us 后判断
    if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_SET) {
        DHT11_WaitLevel(Bit_RESET, 200);            // 等高电平结束
        return 1;
    }
    return 0;
}

/* ---------- 5. 读一个字节（不用改） ---------- */
uint8_t DHT11_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        data |= DHT11_ReadBit();
    }
    return data;
}

/* ---------- 6. 完整读取（补响应等待 + 返回值） ---------- */
uint8_t DHT11_Read(float *temp, float *humi)
{
    uint8_t data[5] = {0};

    DHT11_Start();

    /* 等 DHT11 响应：80us 低 + 80us 高 */
    if (DHT11_WaitLevel(Bit_RESET, 200)) return 1;
    if (DHT11_WaitLevel(Bit_SET,   200)) return 1;
    if (DHT11_WaitLevel(Bit_RESET, 200)) return 1;

    /* 读 5 个字节 */
    for (uint8_t i = 0; i < 5; i++) {
        data[i] = DHT11_ReadByte();
    }

    /* 校验：前 4 字节和 == 第 5 字节 */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
        return 2;

    *humi = data[0] + data[1] / 10.0f;
    *temp = data[2] + data[3] / 10.0f;
    return 0;                                       // 0 = 成功
}