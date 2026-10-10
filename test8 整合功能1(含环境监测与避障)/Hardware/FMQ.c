#include "stm32f10x.h"                  // Device header

void FMQ_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
}

void FMQ_Sound()
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
}

void FMQ_Close()
{
	GPIO_SetBits(GPIOB, GPIO_Pin_11);
}
