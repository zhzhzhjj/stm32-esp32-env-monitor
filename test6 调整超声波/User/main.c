#include "stm32f10x.h"
#include "OLED.h"
#include "Delay.h"
#include "HCSR04.h"

float distance;

int main(void)
{
    OLED_Init();
    HCSR04_Init();

    // 1~5列是 "DIST:"，6~8列留给数字，11~12列是 "cm"
    OLED_ShowString(1, 1, "DIST:");
    OLED_ShowString(1, 11, "cm  "); 

    while(1)
    {
        distance = Test_Distance();

        if (distance == -1.0f || distance == -2.0f)
        {
            // 读不到数据（或者太远超时）
            OLED_ShowString(1, 6, "Err  ");
        }
        else if (distance == -3.0f)
        {
            // 超出量程
            OLED_ShowString(1, 6, "Out  ");
        }
        else
        {
            // 显示距离，占3位，放在第6列开始
            OLED_ShowNum(1, 6, (uint16_t)distance, 3);
        }

        Delay_ms(200);
    }
}
