#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "config.h"                     // 全工程参数集中管理（波特率、引脚）
#include <stdio.h>                      // 提供 FILE 类型（给 fputc 用）

/* ============================================================
 *  Serial.c —— 串口 USART1 驱动（迭代方向①：温度打印到电脑或手机）
 *
 *  硬件接线（CH340 USB-TTL 模块）：
 *      CH340 RXD  ←  STM32 PA9 (TX)
 *      CH340 TXD  →  STM32 PA10 (RX)
 *      CH340 GND  —— STM32 GND（必须共地！）
 *   注意：TX 接对方 RX，RX 接对方 TX（交叉接）
 * ============================================================ */

void Serial_Init(void)
{
	/* 打开 GPIOA 和 USART1 的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	/* PA9 = TX：复用推挽输出（由 USART1 外设控制输出） */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin  = SERIAL_TX_GPIO_PIN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SERIAL_TX_GPIO_PORT, &GPIO_InitStruct);

	/* PA10 = RX：浮空输入（接收外部数据） */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Pin  = SERIAL_RX_GPIO_PIN;
	GPIO_Init(SERIAL_RX_GPIO_PORT, &GPIO_InitStruct);

	/* USART1 参数：波特率来自 config.h，8位数据 1停止位 无校验 */
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = SERIAL_BAUDRATE;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;  /* 收发都开：RX 留给以后蓝牙/上位机指令 */
	USART_Init(USART1, &USART_InitStruct);

	USART_Cmd(USART1, ENABLE);           /* 启动 USART1 */

	/* ===== 接收中断配置（手机蓝牙指令） ===== */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  /* 使能"收到一个字节"中断 */
	NVIC_EnableIRQ(USART1_IRQn);                     /* 打开 USART1 在 NVIC 的总开关（同 Time.c 风格） */
}

/* ============================================================
 *  接收部分：中断逐字节收 → 拼成一行 → 主循环用 GetLine 取走解析
 *  手机每条指令以 '\n' 结尾（APP 里"换行符"保持勾选）
 * ============================================================ */
#define SERIAL_CMD_BUF_LEN 32
static char cmdBuf[SERIAL_CMD_BUF_LEN];          /* 存放一条指令的字符 */
static volatile uint8_t cmdLen = 0;              /* 已收到的字符数 */
static volatile uint8_t cmdReady = 0;            /* 1=收到 '\n'，一条指令完整了 */

/* USART1 中断服务函数（名字固定，it.c 里没有同名函数，不冲突） */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		char ch = (char)USART_ReceiveData(USART1);  /* 读数据寄存器，读的同时自动清 RXNE 标志 */

		if (ch == '\n')                             /* 换行符 → 一条指令结束 */
			cmdReady = 1;
		else if (ch != '\r' && cmdLen < SERIAL_CMD_BUF_LEN - 1)
			cmdBuf[cmdLen++] = ch;                  /* 普通字符存起来；'\r' 忽略（APP 可能发 \r\n） */
	}
}

/* 主循环调用：有完整指令就拷到 out 并返回 1，同时清空缓冲区准备收下次 */
uint8_t Serial_GetLine(char *out)
{
	if (cmdReady)
	{
		uint8_t i;
		for (i = 0; i < cmdLen; i++) out[i] = cmdBuf[i];
		out[cmdLen] = '\0';              /* 补字符串结束符，main 里才能用 strncmp/strcmp */
		cmdLen = 0;
		cmdReady = 0;
		return 1;
	}
	return 0;
}

/* 发送单个字符（内部函数） */
static void Serial_SendChar(char ch)
{
	USART_SendData(USART1, (uint8_t)ch);
	/* 等待发送数据寄存器空（TXE=1 才能发下一个） */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/* 发送字符串：循环发送每个字符，遇到 '\0' 结束 */
void Serial_SendString(char *str)
{
	while (*str != '\0')
	{
		Serial_SendChar(*str);
		str++;
	}
}

/* 发送小数（自己拆数字，避开 MicroLIB 不支持 %f 的坑）
   例：Serial_SendFloat(25.36, 1)  → 发送 "25.4"
   例：Serial_SendFloat(25.36, 2)  → 发送 "25.36"
   例：Serial_SendFloat(25.00, 0)  → 发送 "25" */
void Serial_SendFloat(float num, uint8_t DecLen)
{
	char buf[20];               /* 临时缓冲区，拼好一次性发送 */
	uint8_t len = 0;            /* 已写入的字符数 */
	uint8_t i;

	/* ① 负数处理：先输出负号，再转成正数 */
	if (num < 0) { buf[len++] = '-'; num = -num; }

	/* ② 四舍五入：乘 10^DecLen 后 +0.5 取整（如 25.36×10+0.5=254.1→254） */
	float factor = 1.0f;
	for (i = 0; i < DecLen; i++) factor *= 10.0f;
	uint32_t rounded = (uint32_t)(num * factor + 0.5f);

	/* ③ 拆出整数部分和小数部分 */
	uint32_t int_part = rounded / (uint32_t)factor;
	uint32_t dec_part = rounded % (uint32_t)factor;

	/* ④ 整数部分：从低位往高位取数字，存 tmp 再反转（如 25 → '2','5'） */
	char tmp[12];
	uint8_t n = 0;
	if (int_part == 0) tmp[n++] = '0';
	while (int_part > 0)
	{
		tmp[n++] = '0' + (int_part % 10);
		int_part /= 10;
	}
	for (i = 0; i < n; i++) buf[len++] = tmp[n - 1 - i];

	/* ⑤ 小数部分：先点小数点，再输出 DecLen 位（高位不足补 '0'，如 25.04） */
	if (DecLen > 0)
	{
		buf[len++] = '.';
		char dtmp[6];
		uint8_t dn = 0;
		if (dec_part == 0) dtmp[dn++] = '0';
		while (dec_part > 0)
		{
			dtmp[dn++] = '0' + (dec_part % 10);
			dec_part /= 10;
		}
		for (i = 0; i < DecLen - dn; i++) buf[len++] = '0';     /* 高位补 0 */
		for (i = 0; i < dn; i++) buf[len++] = dtmp[dn - 1 - i]; /* 反转输出 */
	}

	buf[len] = '\0';            /* 结束符 */
	Serial_SendString(buf);
}

/* printf 重定向：把 printf 的输出送到串口
   注意：需要 Keil 勾选 MicroLIB（Options → Target → Code Generation → Use MicroLIB）
   且 MicroLIB 下 printf 不支持 %f —— 打印小数请用 Serial_SendFloat */
int fputc(int ch, FILE *f)
{
	Serial_SendChar((char)ch);
	return ch;
}
