#ifndef __OLED_H
#define __OLED_H

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
/* OLED显示小数（正数）：
   Line      行，1~4
   Column    列，1~16
   Number    要显示的小数（如 25.36）
   IntLength 整数部分显示几位（如 2 → 显示"25"）
   DecLength 小数部分显示几位（如 1 → 显示"4"，即 25.4）*/
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntLength, uint8_t DecLength);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
