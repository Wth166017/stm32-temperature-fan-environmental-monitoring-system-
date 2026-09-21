#include "stm32f10x.h"                  // Device header
#include "config.h"                     // 全工程参数集中管理
volatile uint32_t TickCount = 0;        // 节拍计数，TIM3 每 1ms 中断加 1
void Time_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	
	TIM_InternalClockConfig(TIM3);
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStruct;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = TICK_TIM_ARR;
	TIM_TimeBaseInitStruct.TIM_Prescaler = TICK_TIM_PSC;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
	
	TIM_ClearFlag(TIM3,TIM_FLAG_Update);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);   // 使能 TIM3 的"更新中断"
	TIM_Cmd(TIM3,ENABLE);                      // 启动 TIM3

	NVIC_EnableIRQ(TIM3_IRQn);                 // ★关键：打开 NVIC 中断总开关！
	                                           // 没有这行，TIM3 的中断请求到不了 CPU，
	                                           // 中断函数永远不会执行，TickCount 永远是 0
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        TickCount++;
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  // 清中断标志
    }
}
