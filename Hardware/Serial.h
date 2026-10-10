#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

void Serial_Init(void);                                   // 初始化 USART1（PA9=TX / PA10=RX）
void Serial_SendString(char *str);                        // 发送字符串（以 '\0' 结尾）
void Serial_SendFloat(float num, uint8_t DecLen);         // 发送小数（避开 %f 坑），如 25.4
uint8_t Serial_GetLine(char *out);                        // 取出一条完整指令：返回1=取到了，0=还没收到

#endif
