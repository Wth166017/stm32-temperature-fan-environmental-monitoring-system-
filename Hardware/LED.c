#include "stm32f10x.h"                  // Device header
#include "Time.h"                  // Device header
#include "config.h"               // 全工程参数集中管理

static uint32_t LastBlinkTime = 0;    // 上次翻转的时刻（ms）

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	GPIO_ResetBits(GPIOA,GPIO_Pin_2);
}

void LED_ON(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_2);
}

void LED_OFF(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_2);
}

void LED_Twinkle(void)                          // 主循环里每轮调用一次即可
{
    if (TickCount - LastBlinkTime >= LED_BLINK_HALF_MS)   // 距上次翻转过了闪烁半周期？
    {
        LastBlinkTime = TickCount;              // 记录本次翻转时刻
        if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2))   // 读引脚：现在是亮的？
            LED_OFF();                          //   亮 → 灭
        else
            LED_ON();                           //   灭 → 亮
    }
}
