#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "AD.h"
#include "DHT11.h"

uint16_t Light_Int,Light_Dec, ADValue;
float Light_Value;
float temp, shidu;
uint32_t dht_cnt = 0;

int main(void)
{
	OLED_Init();
	AD_Init();
	DHT11_Init();
	
	OLED_ShowString(1, 1, "Light: 00.00%");
	OLED_ShowString(2,1,"temp :");
	OLED_ShowString(3,1,"shidu:");	
	
	while (1)
	{
		ADValue = AD_GetValue();
		Light_Value = 100.0 - ((float)ADValue / 4095.0 * 100.0);
		
		Light_Int = (uint16_t)Light_Value;                  
		Light_Dec = (uint16_t)(Light_Value * 100) % 100; 
		
		OLED_ShowNum(1, 8, Light_Int, 2);
		OLED_ShowNum(1, 11, Light_Dec, 2);
		
		if (dht_cnt++ % 10 == 0) {
    if (DHT11_Read(&temp, &shidu) == 0) {

        OLED_ShowNum (2, 7,  (uint16_t)temp,  2);                 // 整数 2 位
        OLED_ShowChar(2, 9,  '.');                                // ← 小数点
        OLED_ShowNum (2, 10, (uint16_t)(temp  * 10) % 10, 1);     // 小数 1 位


        OLED_ShowNum (3, 7,  (uint16_t)shidu, 2);
        OLED_ShowChar(3, 9,  '.');
        OLED_ShowNum (3, 10, (uint16_t)(shidu * 10) % 10, 1);
    }
}
		
		Delay_ms(100);
	}
}
