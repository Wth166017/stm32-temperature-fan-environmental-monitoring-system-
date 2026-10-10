#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Time.h"                       // 要用到 TickCount 做非阻塞计时
#include "config.h"
#include "DS18B20.h"

/* ============================================================
 *  底层引脚操作（单总线 = 开漏输出 + 外部上拉电阻）
 *  开漏模式下：
 *    输出 0 = 引脚内部接到 GND（"按住"，线变低）
 *    输出 1 = 引脚内部断开（"松手"，靠模块上拉电阻把线拉回高）
 *  所以"写1"不是主动推高，而是"放手不管"。
 * ============================================================ */
#define DQ_LOW()    GPIO_ResetBits(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)
#define DQ_HIGH()   GPIO_SetBits(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)
#define DQ_READ()   GPIO_ReadInputDataBit(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)

/* ------------------------------------------------------------
 *  初始化：五步法（开时钟 → 配GPIO → 初始状态）
 *  DS18B20 是纯协议器件，没有"外设配置/使能"那两步，只配 GPIO
 * ------------------------------------------------------------ */
void DS18B20_Init(void)
{
	/* ① 开 GPIOB 时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* ② PB1 配开漏输出（★不是推挽！推挽会强推高电平，从机拉不动线） */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Pin   = DS18B20_GPIO_PIN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS18B20_GPIO_PORT, &GPIO_InitStruct);   // ★★端口必须是 GPIOB（之前错写成 GPIOA）

	/* ③ 空闲状态：松手，总线被上拉为高电平 */
	DQ_HIGH();
}

/* ------------------------------------------------------------
 *  复位时序：每次通信的第一步，顺带检测传感器在不在
 *  返回 0 = 从机应答了（在线）；1 = 无应答（离线/接线错）
 * ------------------------------------------------------------ */
uint8_t DS18B20_Reset(void)
{
	uint8_t presence;

	DQ_LOW();
	Delay_us(500);          /* 主机拉低至少 480us */

	DQ_HIGH();
	Delay_us(70);           /* 松手后等 70us，进入从机应答窗口（从机在15~60us内拉低） */

	presence = DQ_READ();   /* 读到 0 = 从机正把线拉低 = 在线；读到 1 = 没人应答 */

	Delay_us(430);          /* 补足复位周期（500+70+430=1000us），防止后续时序错位 */

	return presence;
}

/* ------------------------------------------------------------
 *  写一个字节（8位），低位先传（LSB first）
 *  每一位占用一个 ≥60us 的时隙，时隙之间至少 1us 间隔
 * ------------------------------------------------------------ */
void DS18B20_WriteByte(uint8_t byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		if (byte & 0x01)            /* 当前要发的位 = 1 */
		{
			DQ_LOW();
			Delay_us(2);            /* 拉低 1~15us（短脉冲） */
			DQ_HIGH();              /* 马上松手 → 上拉变高，从机读到 1 */
			Delay_us(60);           /* 保持到时隙结束（时隙总长 ≥60us） */
		}
		else                        /* 当前要发的位 = 0 */
		{
			DQ_LOW();
			Delay_us(60);           /* 整段时隙都拉低（60~120us），从机读到 0 */
			DQ_HIGH();
			Delay_us(2);            /* 时隙间小间隔 */
		}
		byte >>= 1;                 /* 右移一位，准备发下一位 */
	}
}

/* ------------------------------------------------------------
 *  读一个字节（8位），低位先收（LSB first）
 *  主机先拉低 1~15us 发起读时隙，松手后从机在 15us 内输出 0/1
 * ------------------------------------------------------------ */
uint8_t DS18B20_ReadByte(void)
{
	uint8_t i, byte = 0;
	for (i = 0; i < 8; i++)
	{
		DQ_LOW();
		Delay_us(2);                /* 拉低 1~15us，开一个读时隙的"口子" */
		DQ_HIGH();                  /* 松手，让从机控制电平 */
		Delay_us(10);               /* 再等一会，必须在拉低后的 15us 内完成采样 */

		if (DQ_READ())              /* 读到高 = 该位是 1，把对应位置1；读到低 = 0，不用动 */
			byte |= (uint8_t)(1 << i);

		Delay_us(50);               /* 补够读时隙（总长 ≥60us） */
	}
	return byte;
}

/* ------------------------------------------------------------
 *  非阻塞读温度 —— 内部是一个三状态状态机
 *
 *  DS18B20 在 12 位精度下转换一次要 ≥750ms，绝不能 Delay_ms 傻等
 *  （傻等会卡住主循环，LED闪烁、按键、显示全部停摆）。
 *  本函数每次主循环被调用一次，用 TickCount 判断转换是否完成；
 *  转换期间直接返回上一次的温度，主循环照常跑别的事。
 * ------------------------------------------------------------ */
float DS18B20_GetTemp(void)
{
	static uint8_t  state     = 0;      /* 0=启动转换  1=等待转换  2=读取结果 */
	static uint32_t startTime = 0;      /* 本轮转换开始的时刻(TickCount) */
	static float    temp      = 0.0f;   /* 最近一次有效温度，转换期间返回它 */
	uint16_t raw;                       /* DS18B20 发回来的 16 位原始数据 */

	switch (state)
	{
		case 0:   /* 阶段A：复位 → 跳过ROM(0xCC) → 启动温度转换(0x44) */
			DS18B20_Reset();
			DS18B20_WriteByte(0xCC);   /* Skip ROM：板上只有一个传感器，跳过寻址 */
			DS18B20_WriteByte(0x44);   /* Convert T：命令它开始测温 */
			startTime = TickCount;     /* 记下开始时刻 */
			state = 1;
			break;

		case 1:   /* 等待：时间没到就什么都不做（函数末尾返回旧温度） */
			if (TickCount - startTime >= DS18B20_CONVERT_MS)
				state = 2;             /* 转换时间到了，下一步去读 */
			break;

		case 2:   /* 阶段B：复位 → 跳过ROM(0xCC) → 读暂存器(0xBE) → 读2字节 */
			DS18B20_Reset();
			DS18B20_WriteByte(0xCC);
			DS18B20_WriteByte(0xBE);   /* Read Scratchpad：命令它交出结果 */
			raw  = DS18B20_ReadByte();              /* 第1字节：温度低字节 */
			raw |= (uint16_t)DS18B20_ReadByte() << 8; /* 第2字节：温度高字节 */

			/* 换算：16位是二进制补码（转 int16_t 后负温度也正确），
			   12位精度下 1 LSB = 0.0625℃ */
			temp = (int16_t)raw * DS18B20_TEMP_FACTOR;

			state = 0;                 /* 立刻启动下一轮转换，循环测温 */
			break;
	}

	return temp;
}
