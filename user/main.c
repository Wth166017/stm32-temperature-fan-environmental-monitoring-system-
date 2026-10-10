#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "LED.h"
#include "OLED.h"
#include "motor.h"
#include "Time.h"
#include "Serial.h"                     // 迭代①：串口打印温度到电脑或手机
#include "Key.h"                        // 迭代③：按键菜单交互
#include "config.h"				// 全工程参数集中管理
#include "DS18B20.h"
#include <string.h>				// strncmp/strcmp（判断手机指令的命令头）

static uint8_t FanOn = 0;               // 风扇状态：1=开 0=关

/* ============ 菜单可调参数（上电取 config.h 的默认值） ============
   按键改的是"这三个变量"，不是改宏！
   config.h 里的是"出厂默认值"，按键长按模式键可恢复。 */
static float   OnThreshold  = TEMP_ON_THRESHOLD;    // 风扇启动阈值 (℃)
static float   OffThreshold = TEMP_OFF_THRESHOLD;   // 风扇停止阈值 (℃)
static uint8_t FanSpeed     = FAN_SPEED;            // 风扇速度 (0~100)

/* 当前选中的菜单项：0=启动阈值  1=停止阈值  2=风扇速度 */
static uint8_t MenuIndex = 0;

/* 上次通过串口上报温度的时间（配合 TickCount 控制上报间隔，非阻塞） */
static uint32_t LastSendTime = 0;

/* 限幅函数：把 value 限制在 [min, max] 之间 */
static float Clamp(float value, float min, float max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

/* 按键调整当前选中项（delta 为 +1 或 -1） */
static void AdjustMenu(int8_t delta)
{
	switch (MenuIndex)
	{
		case 0:   /* 启动阈值 */
			OnThreshold = Clamp(OnThreshold + delta, ON_THRESH_MIN, ON_THRESH_MAX);
			break;
		case 1:   /* 停止阈值 */
			OffThreshold = Clamp(OffThreshold + delta, OFF_THRESH_MIN, OFF_THRESH_MAX);
			break;
		case 2:   /* 风扇速度 */
			FanSpeed = (uint8_t)Clamp((float)(FanSpeed + delta), (float)SPEED_MIN, (float)SPEED_MAX);
			break;
	}
	/* 滞回保护：启动阈值必须 > 停止阈值，否则临界点会疯狂启停 */
	if (OnThreshold <= OffThreshold)
		OnThreshold = OffThreshold + 1.0f;
}

/* 处理按键事件 */
static void HandleKey(void)
{
	KeyEvent ev = Key_Scan();            // 非阻塞扫描，返回一个事件

	switch (ev)
	{
		case KEY_UP_SHORT:               // 短按模式键：切换选中项 0→1→2→0
			MenuIndex++;
			if (MenuIndex > 2) MenuIndex = 0;
			break;
		case KEY_UP_LONG:                // 长按模式键：恢复默认参数
			OnThreshold  = TEMP_ON_THRESHOLD;
			OffThreshold = TEMP_OFF_THRESHOLD;
			FanSpeed     = FAN_SPEED;
			break;
		case KEY_DOWN_SHORT:             // 短按调节键：当前项 +1
			AdjustMenu(+1);
			break;
		case KEY_DOWN_LONG:              // 长按调节键：当前项 -1
			AdjustMenu(-1);
			break;
		default:                         // 无事件
			break;
	}
}

/* 刷新菜单显示（第3、4行），' >' 标记当前选中项 */
static void ShowMenu(void)
{
	/* 第3行：>ON:30  >OFF:29 （哪个前面有 '>' 就是当前选中项） */
	OLED_ShowChar(3, 1, (MenuIndex == 0) ? '>' : ' ');
	OLED_ShowString(3, 2, "ON:");
	OLED_ShowNum(3, 5, (uint32_t)OnThreshold, 2);
	OLED_ShowChar(3, 8, (MenuIndex == 1) ? '>' : ' ');
	OLED_ShowString(3, 9, "OFF:");
	OLED_ShowNum(3, 12, (uint32_t)OffThreshold, 2);

	/* 第4行：>SPD:050 */
	OLED_ShowChar(4, 1, (MenuIndex == 2) ? '>' : ' ');
	OLED_ShowString(4, 2, "SPD:");
	OLED_ShowNum(4, 5, FanSpeed, 3);
}

/* ============================================================
 *  手机蓝牙指令处理（文本协议，大写，\n 结束）
 *    ON32  → 启动阈值=32
 *    OFF28 → 停止阈值=28
 *    SPD60 → 风扇速度=60
 * ============================================================ */

/* 从 str 的第 start 位开始，把连续的数字字符('0'~'9')转成整数
   原理：字符 '0'~'9' 减 '0' 就是数值 0~9；
        每多读一位，之前的数 ×10（十进制进位） */
static uint8_t ParseNumber(char *str, uint8_t start)
{
	uint8_t num = 0;
	uint8_t i = start;
	while (str[i] >= '0' && str[i] <= '9')
	{
		num = num * 10 + (uint8_t)(str[i] - '0');
		i++;
	}
	return num;
}

/* 解析并执行一条指令，执行完回 "OK xxx" 给手机确认 */
static void HandleCommand(char *cmd)
{
	if (strncmp(cmd, "ON", 2) == 0)            /* 前2字符是"ON" */
	{
		OnThreshold = Clamp((float)ParseNumber(cmd, 2), ON_THRESH_MIN, ON_THRESH_MAX);
		if (OnThreshold <= OffThreshold)       /* 滞回保护：启动阈值必须 > 停止阈值 */
			OnThreshold = OffThreshold + 1.0f;
		Serial_SendString("OK ON\r\n");
	}
	else if (strncmp(cmd, "OFF", 3) == 0)      /* 前3字符是"OFF"（注意要和ON区分，所以比较3位） */
	{
		OffThreshold = Clamp((float)ParseNumber(cmd, 3), OFF_THRESH_MIN, OFF_THRESH_MAX);
		if (OnThreshold <= OffThreshold)
			OnThreshold = OffThreshold + 1.0f;
		Serial_SendString("OK OFF\r\n");
	}
	else if (strncmp(cmd, "SPD", 3) == 0)
	{
		FanSpeed = (uint8_t)Clamp((float)ParseNumber(cmd, 3), (float)SPEED_MIN, (float)SPEED_MAX);
		Serial_SendString("OK SPD\r\n");
	}
	else
	{
		Serial_SendString("ERROR\r\n");         /* 不认识的指令，给手机反馈，方便排查 */
	}
}

int main(void)
{
	float Temperture;			// 温度变量：用 float 存，才能保留小数（如 25.36）

	LED_Init();
	Time_Init();
	OLED_Init();
	Motor_Init();
	Serial_Init();				// 迭代①：串口初始化（PA9/PA10）
	Key_Init();				// 迭代③：按键初始化（PB0/PB10）
	DS18B20_Init();				// DS18B20 数字温度传感器初始化（PB1）

	OLED_ShowString(1,1,"Temperture:");
	OLED_ShowChar(1, 16, 'C');	// 第16列显示温度单位 C（°C 简写）
	OLED_ShowString(2, 1, "Fan:");

	while(1)
	{
		/* ① 采集并显示温度（DS18B20 非阻塞：约每 800ms 更新一次，期间返回上一次值） */
		Temperture = DS18B20_GetTemp();				// 读温度（返回 float，如 25.37）
		OLED_ShowFloat(1, 12, Temperture, 2, 1);	// 显示"25.4"：整数2位 + 1位小数

		/* ② 串口上报温度：每隔 SERIAL_SEND_PERIOD_MS 发一次（非阻塞，不刷屏）
		      时间差用减法：TickCount 溢出回绕时减法仍正确 */
		if (TickCount - LastSendTime >= SERIAL_SEND_PERIOD_MS)
		{
			LastSendTime = TickCount;          // 记住本次发送时间
			Serial_SendString("Temp:");
			Serial_SendFloat(Temperture, 1);
			Serial_SendString("\r\n");
		}

		/* ③ 按键交互：切换菜单项 / 调节阈值和速度 */
		HandleKey();
		ShowMenu();

		/* ③+ 手机蓝牙指令：取一条完整指令，有就解析执行（ON/OFF/SPD） */
		char cmd[32];
		if (Serial_GetLine(cmd))
			HandleCommand(cmd);

		/* ④ 滞回控制：用"按键可调"的变量判断，而不是宏 */
		if(Temperture > OnThreshold)
		{
			FanOn = 1;
		}
		else if(Temperture < OffThreshold)
		{
			FanOn = 0;
			OLED_ShowString(2, 5, "OFF");
			LED_OFF();
		}

		/* ⑤ 执行风扇与 LED */
		if (FanOn)
		{
			Motor_SetSpeed(FanSpeed);			// 用可调速度，而不是写死的宏
			OLED_ShowString(2, 5, "ON ");
			LED_Twinkle();					// 风扇开着时：LED 闪烁（非阻塞）
		}
		else
		{
			Motor_Stop();
			OLED_ShowString(2,5,"OFF");
			LED_OFF();
		}

		Delay_ms(MAIN_LOOP_PERIOD_MS);				// 主循环轮询周期
	}
}
