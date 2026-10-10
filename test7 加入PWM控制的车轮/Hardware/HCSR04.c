#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void HCSR04_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    // Trig -> PB12，推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Echo -> PB13，下拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // TIM4 配置：1us 计一次数
    TIM_InternalClockConfig(TIM4);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 60000 - 1;   // ARR
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;   // PSC，1MHz
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
}

/*
 * 返回值：
 *   > 0  : 距离（单位 cm）
 *  -1.0f : Echo 一直不拉高（没收到回响/模块没工作）
 *  -2.0f : Echo 一直拉高不放（接线错误或模块损坏）
 *  -3.0f : 距离超出量程（大于约4米）
 */
float Test_Distance(void)
{
    uint32_t timeout = 0;
    uint16_t time_us = 0;
    float distance = 0.0f;

    // 1. Trig 拉高 20us，触发一次测距
    GPIO_SetBits(GPIOB, GPIO_Pin_12);
    Delay_us(20);
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);

    // 2. 等待 Echo 拉高（收到回响）
    timeout = 0;
    while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == RESET)
    {
        timeout++;
        if (timeout > 50000) return -1.0f;   // 等不到 Echo 变高
    }

    // 3. Echo 变高，清零并启动定时器
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);

    // 4. 等待 Echo 拉低（回响结束）
    timeout = 0;
    while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == SET)
    {
        timeout++;
        if (timeout > 50000)
        {
            TIM_Cmd(TIM4, DISABLE);
            return -2.0f;   // Echo 一直高电平不放
        }
    }

    // 5. 关闭定时器，读取高电平持续时间（单位 us）
    TIM_Cmd(TIM4, DISABLE);
    time_us = TIM_GetCounter(TIM4);

    // 6. 计算距离：距离 = 时间(us) * 0.017 (cm)
    //    声速 340m/s = 0.034 cm/us，来回除以2 => 0.017
    distance = time_us * 0.017f;

    // 7. 量程保护，超过约 4 米视为无效
    if (distance > 400.0f) return -3.0f;

    // 8. 冷却一下，避免回波干扰下一次测距
    Delay_ms(60);

    return distance;
}
