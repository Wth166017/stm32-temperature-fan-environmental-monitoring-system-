#include "stm32f10x.h"                  // Device header
#include "math.h"                       // 数学库：logf()（求自然对数 ln）在这个库里
#include "config.h"                     // 全工程参数集中管理（传感器系数）
/* 函数：温度传感器初始化
   作用：把 PA1 配置成模拟输入，并配置好 ADC1 外设 */
void TempertureSensor_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;      // PA1 配置为模拟输入
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);   // 打开 ADC1 时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);                    // ADC 时钟 = 72MHz/6 = 12MHz
	
	ADC_InitTypeDef  ADC_InitStruct;
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;      // 单次转换
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;   // 数据右对齐
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  // 软件触发
	ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;       // 独立模式
	ADC_InitStruct.ADC_NbrOfChannel = 1;                  // 转换通道数=1
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;            // 非扫描模式
	ADC_Init(ADC1,&ADC_InitStruct);
	
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,1,ADC_SampleTime_55Cycles5);  // 规则通道：采1号通道(PA1)
	ADC_Cmd(ADC1,ENABLE);                                 // 使能 ADC1
	
	/* ADC 校准（上电后必须校准一次，否则读数不准）*/
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1) == SET);
}

/* 函数：读一次 ADC 原始值
   返回值：0~4095 的数字（对应 PA1 上的电压 0~3.3V）
   流程：软件启动转换 → 等待转换结束(EOC标志) → 读走结果 */
static uint16_t GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);                       // 启动转换
	while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET);        // 等转换完成
	return ADC_GetConversionValue(ADC1);                         // 读结果
}

/* 函数：获取温度（摄氏度）
   返回值：float，如 25.36 表示 25.36℃
   三步换算：
   ① ADC数字 → 电压：  Vout = 3.3 × ADC / 4095
   ② 电压 → NTC电阻：  R = 10000 × Vout / (3.3 − Vout)
   ③ 电阻 → 温度：     用B值公式（B=3950，R0=10K@25℃）*/
float GetTemperture(void)
{
	uint16_t adc_value = GetValue();     // ① 读 ADC 原始值（必须接住返回值！）
	
	/* 第①步：ADC数字 → 电压（单位V）*/
	float Vout = ADC_VREF * adc_value / ADC_MAX_VALUE;
	
	/* 第②步：电压 → NTC电阻（分压公式反推，单位Ω）
	   R_ntc = R1 × Vout / (VCC − Vout)，R1=模块内分压电阻=10K */
	float R_ntc = NTC_R1_OHM * Vout / (ADC_VREF - Vout);
	if (R_ntc <= 0.0f) R_ntc = 1.0f;     // 防除零保护：温度极高时 Vout 接近3.3V
	
	/* 第③步：电阻 → 温度（B值公式）
	   1/T = 1/T0 + ln(R/R0)/B
	   T0 = 298.15（即25℃），R0 = 10000（25℃时阻值），B = 3950
	   算出的 T 是开尔文温度，减 273.15 得摄氏温度 */
	float kelvin = 1.0f / (1.0f/NTC_T0_KELVIN + logf(R_ntc/NTC_R0_OHM)/NTC_B_VALUE);

	return kelvin - CELSIUS_OFFSET_K;             // 开尔文 → 摄氏，返回 25.36 这种值
}
