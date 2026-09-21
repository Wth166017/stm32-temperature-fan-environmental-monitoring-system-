#ifndef __TIME_H
#define __TIME_H

void Time_Init(void);
void TIM3_IRQHandler(void);
extern volatile uint32_t TickCount;
#endif
