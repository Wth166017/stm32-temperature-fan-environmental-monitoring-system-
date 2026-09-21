#ifndef __CONFIG_H
#define __CONFIG_H

/* ============================================================
 *  config.h —— 全工程"魔法数字"集中管理中心
 *  改任何参数（温度阈值、风扇速度、传感器系数），只改这里！
 * ============================================================ */

/* ==================== 一、板级引脚定义 ==================== */
/* 以后换板子/换接线，改这里就行（驱动文件逐步引用这些宏） */
// 温度传感器 NTC 模拟输出
#define NTC_GPIO_PORT           GPIOA
#define NTC_GPIO_PIN            GPIO_Pin_1
#define NTC_ADC_CHANNEL         ADC_Channel_1
// LED
#define LED_GPIO_PORT           GPIOA
#define LED_GPIO_PIN            GPIO_Pin_2
// 电机：PWM 调速 + 方向控制
#define MOTOR_PWM_GPIO_PORT     GPIOA
#define MOTOR_PWM_GPIO_PIN      GPIO_Pin_3
#define MOTOR_DIR1_GPIO_PORT    GPIOA
#define MOTOR_DIR1_GPIO_PIN     GPIO_Pin_4
#define MOTOR_DIR2_GPIO_PORT    GPIOA
#define MOTOR_DIR2_GPIO_PIN     GPIO_Pin_5
// OLED 软件 I2C
#define OLED_SCL_GPIO_PORT      GPIOB
#define OLED_SCL_GPIO_PIN       GPIO_Pin_8
#define OLED_SDA_GPIO_PORT      GPIOB
#define OLED_SDA_GPIO_PIN       GPIO_Pin_9

/* ==================== 二、应用参数（业务逻辑） ==================== */
/* ---- 温度阈值（滞回控制） ---- */
#define TEMP_ON_THRESHOLD       30.0f     // 温度超过它：风扇启动 (℃)
#define TEMP_OFF_THRESHOLD      29.0f     // 温度降到它以下：风扇停止 (℃)
/* ---- 风扇 ---- */
#define FAN_SPEED               50        // 风扇速度（占空比百分比 0~100）
/* ---- 主循环 ---- */
#define MAIN_LOOP_PERIOD_MS     200        // 主循环轮询周期 (ms)
/* ---- LED 闪烁 ---- */
#define LED_BLINK_HALF_MS       500       // 闪烁半周期 (ms)：500→亮0.5s灭0.5s=1s周期

/* ==================== 三、传感器参数（NTC 热敏电阻） ==================== */
/* 换 NTC 型号时改这一组 */
#define ADC_VREF                3.3f       // ADC 参考电压 (V)
#define ADC_MAX_VALUE            4095.0f   // 12位 ADC 满量程
#define NTC_R1_OHM              10000.0f   // 模块内分压电阻 R1 (Ω)
#define NTC_R0_OHM               10000.0f   // NTC 在 25℃ 的标称电阻 R0 (Ω)
#define NTC_B_VALUE              3950.0f    // NTC 的 B 值
#define NTC_T0_KELVIN            298.15f    // T0 = 25℃ 对应的开尔文温度
#define CELSIUS_OFFSET_K         273.15f    // 摄氏 ↔ 开尔文 换算常数

/* ==================== 四、定时器参数 ==================== */
/* ---- 电机 PWM (TIM2) ---- */
#define MOTOR_TIM_PSC           27         // TIM2 预分频
#define MOTOR_TIM_ARR           99         // TIM2 自动重装载（PWM 周期）
/* ---- 系统节拍 (TIM3) ---- */
#define TICK_TIM_PSC            71         // TIM3 预分频 → 72M/(71+1)=1MHz
#define TICK_TIM_ARR            999        // TIM3 自动重装载 → 1ms 节拍

#endif
