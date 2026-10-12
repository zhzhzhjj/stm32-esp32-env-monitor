#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
#include "MYI2C.h"
#include "BH1750.h"
#include "SHT30.h"
#include "AHT20.h"
#include "ENS160.h"
#include "FMQ.h"
#include "Car.h"
#include "HCSR04.h"
#include "Serial.h"

float sht_temp, sht_humi;
float aht_temp, aht_humi;
uint8_t sht_status, aht_status;
uint16_t part_id;
ENS160_Data_t ens_data;
float distance;
float lux;
uint8_t alarm_level = 0;



// 数据打包发送到 ESP32
// 协议帧：0xAA + 温度 + 湿度 + 光照 + eCO2高 + eCO2低 + 校验 + 0x55
void Send_Data_To_ESP32(void)
{
    uint8_t tx_buffer[9];
    uint8_t lux_int = (uint8_t)(lux / 10.0f);//ESP32接收到的时候记得×10

    tx_buffer[0] = 0xAA;                                    // 帧头
    tx_buffer[1] = (uint8_t)(sht_temp + 40.0f);				// 温度  // 偏移 +40，范围 -40~215                       
    tx_buffer[2] = (uint8_t)sht_humi;                       // 湿度
    tx_buffer[3] = lux_int;                                 // 光照
    tx_buffer[4] = (uint8_t)(ens_data.eco2 >> 8);           // eCO2 高字节
    tx_buffer[5] = (uint8_t)(ens_data.eco2 & 0xFF);         // eCO2 低字节
	tx_buffer[6] = alarm_level;                             // 报警等级 0~4

    // 异或校验（除帧头帧尾外的 5 个数据字节）
    tx_buffer[7] = tx_buffer[1] ^ tx_buffer[2] ^ tx_buffer[3]
                 ^ tx_buffer[4] ^ tx_buffer[5] ^ tx_buffer[6];

    tx_buffer[8] = 0x55;                                    // 帧尾

    Serial_SendArray(tx_buffer, 9);
}


// 系统运行状态：0=静止（环境监测），1=移动（自动避障）
uint8_t system_is_moving = 0; 

// 自动避障逻辑
void Auto_Avoid(void)
{
    distance = Test_Distance();  // 读取前方距离

    // 前方安全（>30cm 或者读到了超量程错误码），全速前进
    if (distance > 30.0f || distance < 0)
    {
        Go_Ahead();
        Delay_ms(50);
    }
    else  // 距离过近，需要避障
    {
        Car_Stop();
        Delay_ms(200);
        Go_Back();
        Delay_ms(300);      // 后退
        Car_Stop();
        Delay_ms(300);

        //旋转扫描
        Self_Right();

        uint8_t found_way = 0;
        for (uint8_t i = 0; i < 20; i++)  // 最多扫描20次，每次100ms，约转一圈
        {
            Delay_ms(100);
            distance = Test_Distance();

            if (distance > 80.0f || distance < 0)
            {
                found_way = 1;
                break;
            }
        }

        Car_Stop();
        Delay_ms(300);

        //如果转了一圈都没找到路（死角），后退多一点再找
        if (found_way == 0)
        {
            Go_Back();
            Delay_ms(800);
            Car_Stop();
            Delay_ms(300);

            // 后退完再尝试右转找一条路
            Self_Right();
            for (uint8_t i = 0; i < 20; i++)
            {
                Delay_ms(100);
                distance = Test_Distance();
                if (distance > 80.0f || distance < 0)
                {
                    found_way = 1;
                    break;
                }
            }
            Car_Stop();
            Delay_ms(300);
        }
    }
}

// 环境监测逻辑
void Environment_Monitor(void)
{
    // 读取光照
    lux = BH1750_ReadLux();
	OLED_ShowNum(1, 5, (uint16_t)lux, 5);
    
    // 读取 SHT30 温湿度
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
    
    // 读取 AHT20 温湿度，写入 ENS160 做补偿
    aht_status = AHT20_ReadTempHumi(&aht_temp, &aht_humi);
    if (aht_status == 0)
    {
        ENS160_SetEnv(aht_temp, aht_humi);
    }
    
    // 读取 ENS160 数据
    if (ENS160_ReadData(&ens_data) == 0)
    {
        OLED_ShowNum(3, 5, ens_data.aqi, 1);  // AQI：空气质量指数，1-5 表示极好到极差
        OLED_ShowNum(3, 12, ens_data.eco2, 4); // eCO2：等效二氧化碳
                                              //   数值(ppm)       空气质量     建议
                                              //   400 左右        室外水平     空气清新
                                              //   400 ~ 1000     室内良好    正常
                                              //   1000 ~ 2000    轻度污浊    建议开窗通风
                                              //   > 2000         污浊       必须通风
        OLED_ShowNum(4, 6, ens_data.tvoc, 5); // TVOC：总挥发性有机物
                                              //   数值(ppb)      空气质量    说明
                                              //   0 ~ 500       良好       正常室内空气
                                              //   500 ~ 1000     轻度污染    有异味或新家具，建议通风
                                              //   1000 ~ 3000    中度污染    明显气味，可能不适
                                              //   > 3000        重度污染    必须通风，避免长时间停留
    }
    
    // 蜂鸣器预警逻辑
    alarm_level = 0;
    if (sht_status == 0 && aht_status == 0)
    {
        if (sht_temp > 35.0f) alarm_level++;   // 温度超标
        if (sht_humi > 80.0f) alarm_level++;   // 湿度超标
        if (lux < 50)         alarm_level++;   // 光照过暗
        if (ens_data.eco2 > 1000) alarm_level++; // 空气质量差
    }
    
	Send_Data_To_ESP32();//发送数据到ESP32;
    // 分级报警
    // "响的时长"来区分密集度
    if (alarm_level == 0)
    {
        // 正常：静音
        FMQ_Close();
        Delay_ms(1000);          // 补齐 1 秒
    }
    else if (alarm_level == 1)
    {
        // 轻度：1 秒内响 1 次
        FMQ_Sound();
        Delay_ms(80);
        FMQ_Close();
        Delay_ms(920);           // 补齐 1 秒
    }
    else if (alarm_level == 2)
    {
        // 中度：1 秒内响 2 次
        FMQ_Sound(); Delay_ms(60); FMQ_Close();
        Delay_ms(440);           // 间隔
        FMQ_Sound(); Delay_ms(60); FMQ_Close();
        Delay_ms(440);           // 补齐 1 秒
    }
    else if (alarm_level == 3)
    {
        // 重度：1 秒内响 4 次
        FMQ_Sound(); Delay_ms(50); FMQ_Close();
        Delay_ms(200);
        FMQ_Sound(); Delay_ms(50); FMQ_Close();
        Delay_ms(200);
        FMQ_Sound(); Delay_ms(50); FMQ_Close();
        Delay_ms(200);
        FMQ_Sound(); Delay_ms(50); FMQ_Close();
        Delay_ms(200);           // 补齐 1 秒
    }
    else if (alarm_level >= 4)
    {
        // 紧急：1 秒内响 8 次（几乎连续）
        for (uint8_t i = 0; i < 8; i++)
        {
            FMQ_Sound();
            Delay_ms(50);
            FMQ_Close();
            Delay_ms(50);
        }
        Delay_ms(200);           // 补齐 1 秒
    }


	

	
}

int main()
{
    // 初始化所有外设
    OLED_Init();
    BH1750_Init();
    SHT30_Init();
    AHT20_Init();
    ENS160_Init();
    FMQ_Init();
    Car_Init();
    HCSR04_Init();
	Serial_Init();

    // 读 PART_ID 确认通信，正常为 0x0160
    ENS160_ReadPartID(&part_id);
    // 进入标准模式，开始气体测量
    ENS160_SetMode(ENS160_OPMODE_STANDARD);

    // 显示静态UI框架
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
		if (Serial_GetRxFlag() == 1)
		{
			uint8_t cmd = Serial_GetRxData();
			if (cmd == 'F') system_is_moving = 1;
			else if (cmd == 'S') system_is_moving = 0;
		}
		
        // 根据状态机执行
        if (system_is_moving == 1)
        {
            // 移动模式：自动避障，不读取 I2C 传感器
            Auto_Avoid();
        }
        else
        {
            // 静止模式：电机停转，进行环境监测
            Car_Stop();
            Environment_Monitor();
              //            Delay_ms(1000); // 等待 ENS160 更新数据（已经在蜂鸣器那里延时过1s了）
        }
        
        // 测试时手动修改 system_is_moving 的值：0=静止监测，1=移动避障
    }
}
