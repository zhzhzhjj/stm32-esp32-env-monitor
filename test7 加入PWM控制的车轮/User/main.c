#include "stm32f10x.h"
#include "OLED.h"
#include "Delay.h"
#include "HCSR04.h"
#include "Car.h"
#include "Motor.h"
#include "PWM.h"


float distance;

int main(void)
{
    OLED_Init();
	OLED_ShowString(1,1,"GP!");
	Car_Init();
	while(1)
	{
		Go_Ahead();
	}
}
