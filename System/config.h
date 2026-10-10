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
// 串口 USART1（方向①：温度打印到电脑）
#define SERIAL_TX_GPIO_PORT     GPIOA
#define SERIAL_TX_GPIO_PIN      GPIO_Pin_9
#define SERIAL_RX_GPIO_PORT     GPIOA
#define SERIAL_RX_GPIO_PIN      GPIO_Pin_10
// 按键（方向③：菜单交互。上拉输入：默认高电平，按下接地=低电平）
#define KEY_UP_GPIO_PORT        GPIOB
#define KEY_UP_GPIO_PIN         GPIO_Pin_0
#define KEY_DOWN_GPIO_PORT      GPIOB
#define KEY_DOWN_GPIO_PIN       GPIO_Pin_10
// DS18B20 数字温度传感器（单总线，开漏输出+模块自带4.7k上拉）
#define DS18B20_GPIO_PORT       GPIOB
#define DS18B20_GPIO_PIN        GPIO_Pin_1

/* ==================== 二、应用参数（业务逻辑） ==================== */
/* ---- 温度阈值（滞回控制） ---- */
#define TEMP_ON_THRESHOLD       30.0f     // 温度超过它：风扇启动 (℃)
#define TEMP_OFF_THRESHOLD      29.0f     // 温度降到它以下：风扇停止 (℃)
/* ---- 风扇 ---- */
#define FAN_SPEED               50        // 风扇速度（占空比百分比 0~100）
/* ---- 主循环 ---- */
#define MAIN_LOOP_PERIOD_MS     100        // 主循环轮询周期 (ms)
/* ---- LED 闪烁 ---- */
#define LED_BLINK_HALF_MS       500       // 闪烁半周期 (ms)：500→亮0.5s灭0.5s=1s周期
/* ---- 串口 ---- */
#define SERIAL_BAUDRATE         9600     // 波特率。接 HC-05 蓝牙（默认9600）时可改 9600 免配 AT
#define SERIAL_SEND_PERIOD_MS   3000     // 温度上报间隔(ms)：1000=每秒1条；嫌快改大、嫌慢改小
/* ---- 按键交互 ---- */
#define KEY_DEBOUNCE_MS         10        // 消抖时间 (ms)：按下后等这么久再确认，防抖动
#define KEY_LONG_PRESS_MS       800       // 长按判定时间 (ms)：按住超过它算长按
/* ---- 菜单可调参数范围（按键调节的上下限） ---- */
#define MENU_STEP               1         // 每按一次调节量（℃ 或 %）
#define ON_THRESH_MIN           20.0f     // 启动阈值下限
#define ON_THRESH_MAX           60.0f     // 启动阈值上限
#define OFF_THRESH_MIN          15.0f     // 停止阈值下限
#define OFF_THRESH_MAX          59.0f     // 停止阈值上限
#define SPEED_MIN               0         // 速度下限
#define SPEED_MAX               100       // 速度上限

/* ==================== 三、传感器参数 ==================== */
/* ---- DS18B20 数字温度传感器（当前使用） ---- */
#define DS18B20_CONVERT_MS      800        // 12位转换等待(ms)：规格750ms，留50ms裕量
#define DS18B20_TEMP_FACTOR     0.0625f    // 12位分辨率：原始值 1 LSB = 0.0625℃

/* ---- NTC 热敏电阻（已废弃，保留备查；换用 DS18B20 后不再参与编译调用） ---- */
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
