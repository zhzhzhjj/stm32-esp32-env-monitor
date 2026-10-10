#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
#include "MYI2C.h"
#include "BH1750.h"
#include "SHT30.h"
#include "AHT20.h"

float sht_temp, sht_humi;
float aht_temp, aht_humi;
uint8_t sht_status, aht_status;

int main()
{
	OLED_Init();
	BH1750_Init();
	SHT30_Init();
	AHT20_Init();

	OLED_ShowString(1, 1, "LUX:");
	OLED_ShowString(2, 1, "S");
	OLED_ShowString(3, 1, "A");

	while(1)
	{
		//读取亮度
		float lux = BH1750_ReadLux();
		OLED_ShowNum(1, 5, (uint16_t)lux, 5);
		OLED_ShowString(1, 11, "lx");

		//读取两个传感器
		sht_status = SHT30_ReadTempHumi(&sht_temp, &sht_humi);
		aht_status = AHT20_ReadTempHumi(&aht_temp, &aht_humi);

		//显示 SHT30
		if (sht_status == 0)
		{
			uint16_t t = (uint16_t)(sht_temp * 10);
			uint16_t h = (uint16_t)(sht_humi * 10);

			OLED_ShowNum(2, 3, t / 10, 2);
			OLED_ShowChar(2, 5, '.');
			OLED_ShowNum(2, 6, t % 10, 1);
			OLED_ShowString(2, 7, "C");

			OLED_ShowNum(2, 9, h / 10, 2);
			OLED_ShowChar(2, 11, '.');
			OLED_ShowNum(2, 12, h % 10, 1);
			OLED_ShowString(2, 13, "%");
		}
		else
		{
			OLED_ShowString(2, 3, "Error  ");
		}

		//显示 AHT20
		if (aht_status == 0)
		{
			uint16_t t = (uint16_t)(aht_temp * 10);
			uint16_t h = (uint16_t)(aht_humi * 10);

			OLED_ShowNum(3, 3, t / 10, 2);
			OLED_ShowChar(3, 5, '.');
			OLED_ShowNum(3, 6, t % 10, 1);
			OLED_ShowString(3, 7, "C");

			OLED_ShowNum(3, 9, h / 10, 2);
			OLED_ShowChar(3, 11, '.');
			OLED_ShowNum(3, 12, h % 10, 1);
			OLED_ShowString(3, 13, "%");
		}
		else
		{
			OLED_ShowString(3, 3, "Error  ");
		}

		Delay_ms(1000);
	}
}
