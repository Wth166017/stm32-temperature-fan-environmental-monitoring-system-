#include "stm32f10x.h"                  // Device header
#include "Key.h"
#include "Time.h"                       // 用 TickCount 做非阻塞计时
#include "config.h"                     // 全工程参数集中管理（引脚、消抖时间）

/* ============================================================
 *  Key.c —— 按键驱动（迭代方向③：菜单交互）
 *
 *  硬件接线：
 *      按键一端接 STM32 引脚，另一端接 GND
 *      引脚配置为"上拉输入"：不按时默认高电平(1)，按下接地变低电平(0)
 *      所以"读到 0" = 按下了
 *
 *  防抖原理（软件消抖）：
 *      按键按下瞬间电平会抖动，所以不马上信它。
 *      先记下"检测到按下的时刻"，等 KEY_DEBOUNCE_MS(10ms) 后再读一次：
 *      还是按下 → 真按下了；变成松开 → 是抖动，忽略。
 *
 *  短按/长按判定（非阻塞，用 TickCount）：
 *      确认按下后开始计时：
 *      松开时若没到 800ms → 短按
 *      按住超过 800ms       → 长按（松开时才报告一次，避免重复触发）
 * ============================================================ */

/* 每个按键的跟踪状态（状态机：0→1→2→3） */
typedef struct
{
	uint8_t  State;        /* 0=空闲  1=消抖中  2=已确认按下  3=已触发长按 */
	uint32_t PressTime;    /* 检测到按下的时刻 (ms) */
} KeyState;

static KeyState UpKey;     /* 模式键的跟踪状态 */
static KeyState DownKey;   /* 调节键的跟踪状态 */

void Key_Init(void)
{
	/* 两个键默认都在 GPIOB。若改到其他端口，需在这里补对应时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;      /* 上拉输入：默认高，按下接地变低 */
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStruct.GPIO_Pin = KEY_UP_GPIO_PIN;//0
	GPIO_Init(KEY_UP_GPIO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = KEY_DOWN_GPIO_PIN;//10
	GPIO_Init(KEY_DOWN_GPIO_PORT, &GPIO_InitStruct);
}

/* 扫描一个按键的内部状态机，返回它当前产生的事件（若无事件返回 KEY_NONE） */
static KeyEvent ScanOneKey(KeyState *ks, GPIO_TypeDef *Port, uint16_t Pin,
						   KeyEvent ShortEvent, KeyEvent LongEvent)
{
	uint8_t level = GPIO_ReadInputDataBit(Port, Pin);   /* 1=松开  0=按下 */

	switch (ks->State)
	{
	case 0:   /* 空闲：检测到按下 → 记时刻，进消抖 */
		if (level == 0)
		{
			ks->PressTime = TickCount;
			ks->State = 1;
		}
		break;

	case 1:   /* 消抖中：等 KEY_DEBOUNCE_MS 后再判断 */
		if (level == 1)
		{
			ks->State = 0;                    /* 松开 = 抖动，忽略，回空闲 */
		}
		else if (TickCount - ks->PressTime >= KEY_DEBOUNCE_MS)
		{
			ks->State = 2;                    /* 10ms 后还是按下 = 真按下 */
		}
		break;

	case 2:   /* 已确认按下：等松开（短按）或等超时（长按） */
		if (level == 1)
		{
			ks->State = 0;                    /* 松开了 → 短按事件 */
			return ShortEvent;
		}
		else if (TickCount - ks->PressTime >= KEY_LONG_PRESS_MS)
		{
			ks->State = 3;                    /* 按住超过 800ms → 进入长按态 */
		}
		break;

	case 3:   /* 已触发长按：等松开才报告，防止一直触发 */
		if (level == 1)
		{
			ks->State = 0;                    /* 松开 → 长按事件 */
			return LongEvent;
		}
		break;
	}
	return KEY_NONE;                          /* 其他情况无事件 */
}

/* 扫描两个按键（主循环每轮调用一次） */
KeyEvent Key_Scan(void)
{
	KeyEvent ev;

	/* 先看模式键，再看调节键（同一轮几乎不可能两个键同时松开） */
	ev = ScanOneKey(&UpKey, KEY_UP_GPIO_PORT, KEY_UP_GPIO_PIN, KEY_UP_SHORT, KEY_UP_LONG);
	if (ev != KEY_NONE) return ev;

	ev = ScanOneKey(&DownKey, KEY_DOWN_GPIO_PORT, KEY_DOWN_GPIO_PIN, KEY_DOWN_SHORT, KEY_DOWN_LONG);
	return ev;
}
