#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
#include "MYI2C.h"
#include "BH1750.h"
#include "SHT30.h"

float temp,humi;
uint8_t sht30_status;

int main()
{
	OLED_Init();
	BH1750_Init();
	SHT30_Init();
	OLED_ShowString(1, 1, "LUX:");	
	OLED_ShowString(2, 1, "T:");
    OLED_ShowString(2, 9, "H:");
	
	while(1)
	{
		float lux = BH1750_ReadLux(); // 直接获取 Lux 值

        OLED_ShowNum(1, 5, (uint16_t)lux, 5); // 显示整数部分，占5位（比如 01200 Lux）
        OLED_ShowString(1, 11, "lx");
		
		sht30_status = SHT30_ReadTempHumi(&temp, &humi);
        
        if (sht30_status == 0) // 只有 CRC 校验成功才显示
        {
            // OLED_ShowNum 只能显示整数，所以把浮点数强转成整数
            // 建议显示乘以10后的整数，这样能保留1位小数
            uint16_t temp_display = (uint16_t)(temp * 10); 
            uint16_t humi_display = (uint16_t)(humi * 10);
            
            // 显示温度 (例如 276 代表 27.6℃)
            OLED_ShowNum(2, 3, temp_display / 10, 2);  // 整数部分
            OLED_ShowChar(2, 5, '.');                  // 小数点
            OLED_ShowNum(2, 6, temp_display % 10, 1);  // 小数部分
            OLED_ShowString(2, 7, "C");
            
            // 显示湿度 (例如 492 代表 49.2%)
            OLED_ShowNum(2, 11, humi_display / 10, 2);
            OLED_ShowChar(2, 13, '.');
            OLED_ShowNum(2, 14, humi_display % 10, 1);
            OLED_ShowString(2, 15, "%");
        }
        else
        {
            // 读取失败时显示 Error
            OLED_ShowString(2, 3, "Error  ");
        }

        Delay_ms(500); 
	}
}
