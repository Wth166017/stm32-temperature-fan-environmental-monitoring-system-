#ifndef __KEY_H
#define __KEY_H

/* 按键事件定义：
   KEY_UP_SHORT   → 模式键（PB0）短按：切换菜单选中项
   KEY_UP_LONG    → 模式键（PB0）长按：恢复默认参数
   KEY_DOWN_SHORT → 调节键（PB10）短按：当前项 +1
   KEY_DOWN_LONG  → 调节键（PB10）长按：当前项 -1 */
typedef enum
{
	KEY_NONE = 0,   /* 无事件 */
	KEY_UP_SHORT,
	KEY_UP_LONG,
	KEY_DOWN_SHORT,
	KEY_DOWN_LONG
} KeyEvent;

void Key_Init(void);
KeyEvent Key_Scan(void);   /* 主循环每轮调用一次，返回一个按键事件 */

#endif
