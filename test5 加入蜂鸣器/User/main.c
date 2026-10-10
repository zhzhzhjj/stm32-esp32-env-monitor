#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
#include "MYI2C.h"
#include "BH1750.h"
#include "SHT30.h"
#include "AHT20.h"
#include "ENS160.h"
#include "FMQ.h"



float sht_temp, sht_humi;
float aht_temp, aht_humi;
uint8_t sht_status, aht_status;
uint16_t part_id;
ENS160_Data_t ens_data;

int main()
{
	OLED_Init();
	BH1750_Init();
	SHT30_Init();
	AHT20_Init();
	ENS160_Init();
	FMQ_Init();

	//读 PART_ID 确认通信，正常为 0x0160
	ENS160_ReadPartID(&part_id);

	//进入标准模式，开始气体测量
	ENS160_SetMode(ENS160_OPMODE_STANDARD);

	OLED_ShowString(1, 1, "LUX:");
	OLED_ShowString(1, 11, "lx");
	OLED_ShowString(2, 1, "T:");
	OLED_ShowString(2, 7, "C");
	OLED_ShowString(2, 9, "H:");
	OLED_ShowString(2, 15, "%");
	OLED_ShowString(3, 1, "AQI:");
	OLED_ShowString(3, 7, "eCO2:");
	OLED_ShowString(4, 1, "TVOC:");

	while(1)
	{
		//读取光照
		float lux = BH1750_ReadLux();
		OLED_ShowNum(1, 5, (uint16_t)lux, 5);

		//读取 SHT30 温湿度
		sht_status = SHT30_ReadTempHumi(&sht_temp, &sht_humi);
		if (sht_status == 0)
		{
			uint16_t t = (uint16_t)(sht_temp * 10);
			uint16_t h = (uint16_t)(sht_humi * 10);

			OLED_ShowNum(2, 3, t / 10, 2);
			OLED_ShowChar(2, 5, '.');
			OLED_ShowNum(2, 6, t % 10, 1);

			OLED_ShowNum(2, 11, h / 10, 2);
			OLED_ShowChar(2, 13, '.');
			OLED_ShowNum(2, 14, h % 10, 1);
		}
		else
		{
			OLED_ShowString(2, 3, "Error  ");
		}

		//读取 AHT20 温湿度，写入 ENS160 做补偿
		aht_status = AHT20_ReadTempHumi(&aht_temp, &aht_humi);
		if (aht_status == 0)
		{
			ENS160_SetEnv(aht_temp, aht_humi);
		}

		//读取 ENS160 数据
		if (ENS160_ReadData(&ens_data) == 0)
		{
			OLED_ShowNum(3, 5, ens_data.aqi, 1);  //aqi:空气质量指数,从1-5表示极好到极差
			
			OLED_ShowNum(3, 12, ens_data.eco2, 4);// eCO2：等效二氧化碳
			                                      //   数值(ppm)       空气质量     建议
			                                      //   400 左右        室外水平     空气清新
			                                      //   400 ~ 1000     室内良好    正常
			                                      //   1000 ~ 2000    轻度污浊    建议开窗通风
			                                      //   > 2000         污浊       必须通风
			OLED_ShowNum(4, 6, ens_data.tvoc, 5); // TVOC：总挥发性有机物
			                                      //   数值(ppb)      空气质量    说明
			                                      //   0 ~ 500       良好       正常室内空气
			                                      //  500 ~ 1000     轻度污染    有异味或新家具，建议通风
			                                      // 1000 ~ 3000     中度污染    明显气味，可能不适
			                                      //   > 3000        重度污染    必须通风，避免长时间停留
		}
		if (sht_temp > 35.0f || sht_humi > 80.0f || lux < 50 || ens_data.eco2 > 1000) 
        {
            FMQ_Sound();
			Delay_ms(30);
			FMQ_Close();
			Delay_ms(30);
        } 
        else 
        {
            FMQ_Close();
        }


		Delay_ms(1000);
	}
}

