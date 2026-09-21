#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "LED.h"
#include "OLED.h"
#include "motor.h"
#include "Temperture Sensor.h"
#include "Time.h"
#include "config.h"				// 全工程参数集中管理（温度阈值、风扇速度等）

static uint8_t FanOn = 0;

int main(void)
{
	float Temperture;			// 温度变量：用 float 存，才能保留小数（如 25.36）

	LED_Init();
	Time_Init();
	OLED_Init();
	Motor_Init();
	TempertureSensor_Init();
	OLED_ShowString(1,1,"Temperture:");
	OLED_ShowChar(1, 16, 'C');	// 第5列显示温度单位 C（°C 简写）
	OLED_ShowString(2, 1, "Fan:");
	while(1)
	{
		Temperture = GetTemperture();				// 读温度（返回 float，如 25.36）
		OLED_ShowFloat(1, 12, Temperture, 2, 1);	// 显示"25.4"：整数2位 + 1位小数
		if(Temperture > TEMP_ON_THRESHOLD)
		{
			FanOn = 1;

		}
		else if(Temperture < TEMP_OFF_THRESHOLD)
		{
			FanOn = 0;
			OLED_ShowString(2, 5, "OFF");
			LED_OFF();
		}
		if (FanOn) 
		{ 
			Motor_SetSpeed(FAN_SPEED);
			OLED_ShowString(2, 5, "ON ");
			LED_Twinkle();					// 风扇开着时：LED 闪烁（非阻塞，每轮查一次） }
		}
			else       
		{
			Motor_Stop(); 
			OLED_ShowString(2,5,"OFF"); 
			LED_OFF();
		}
		Delay_ms(MAIN_LOOP_PERIOD_MS);					// 主循环轮询周期
	}
}
