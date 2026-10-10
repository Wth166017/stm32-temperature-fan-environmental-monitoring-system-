#ifndef __DS18B20_H
#define __DS18B20_H

#include <stdint.h>

/* DS18B20 数字温度传感器（单总线 One-Wire，数据线 PB1） */

void    DS18B20_Init(void);      /* 初始化：PB1 配开漏输出，总线空闲为高 */
uint8_t DS18B20_Reset(void);     /* 复位 + 检测传感器：返回 0=在线，1=离线 */
float   DS18B20_GetTemp(void);   /* 非阻塞读温度(℃)：主循环反复调用即可，转换期间返回上一次温度 */

#endif
