#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
#include "MYI2C.h"
#include "BH1750.h"

float Persent;	

int main()
{
	OLED_Init();
	BH1750_Init();
	OLED_ShowString(1, 1, "LUX:");	
	
	while(1)
	{
		float lux = BH1750_ReadLux(); // 直接获取 Lux 值

        OLED_ShowNum(1, 5, (uint16_t)lux, 5); // 显示整数部分，占5位（比如 01200 Lux）
        OLED_ShowString(1, 11, "lx");

        Delay_ms(200); // 
	}
}
