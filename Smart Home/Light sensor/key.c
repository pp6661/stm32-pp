#include "key.h"
#include "delay.h" // 需要用到延时函数进行消抖

/**
 * @brief  按键GPIO初始化
 * @param  无
 * @retval 无
 */
void Key_Init(void)
{
    // 定义GPIO初始化结构体变量
    GPIO_InitTypeDef GPIO_InitStructure;

    // RCC_APB2PeriphClockCmd 函数用于使能或失能APB2外设的时钟
    // RCC_APB2Periph_GPIOA 是GPIOA端口的时钟宏定义
    // ENABLE 表示使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置按键GPIO为上拉输入模式（按键按下时引脚拉低）
    // GPIO_Mode 设置GPIO工作模式为上拉输入
    // 在上拉输入模式下，引脚在没有外部信号时通过内部电阻拉高到VCC
    // 当按键按下时，引脚被拉低到GND。
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    // GPIO_Speed 设置GPIO的输出速度
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // KEY1 (PA15) 的GPIO初始化
    // KEY1_GPIO_PIN 是宏定义，代表KEY1连接的引脚号 (PA15)
    GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_PIN;
    // 调用GPIO_Init函数，根据配置好的结构体初始化KEY1所在的GPIO端口
    GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);

    // KEY2 (PA12) 的GPIO初始化
    // KEY2_GPIO_PIN 是宏定义，代表KEY2连接的引脚号 (PA12)
    GPIO_InitStructure.GPIO_Pin = KEY2_GPIO_PIN;
    // 调用GPIO_Init函数，根据配置好的结构体初始化KEY2所在的GPIO端口
    GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);

    // KEY3 (PA11) 的GPIO初始化
    // KEY3_GPIO_PIN 是宏定义，代表KEY3连接的引脚号 (PA11)
    GPIO_InitStructure.GPIO_Pin = KEY3_GPIO_PIN;
    // 调用GPIO_Init函数，根据配置好的结构体初始化KEY3所在的GPIO端口
    GPIO_Init(KEY3_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief  获取按键值（带简单消抖）
 * @param  无
 * @retval 按下的按键编号（1-3），无按键按下返回0
 */
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0; // 定义并初始化按键编号变量，0表示没有按键按下

    // 检测KEY1 (PA15 - 模式切换键)
    // GPIO_ReadInputDataBit 函数用于读取指定GPIO引脚的输入状态
    // 如果读取到低电平 (Bit_RESET)，表示按键被按下
    if (GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == Bit_RESET) // Bit_RESET表示低电平
    {
        delay_ms(20); // 延时20ms进行消抖，等待按键机械抖动结束
        // 再次确认按键状态，防止是抖动或干扰引起的误判
        if (GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == Bit_RESET) // 再次确认是否真的按下
        {
            KeyNum = 1; // 如果确认按下，则将按键编号设置为1
        }
        // 等待按键松开。这个while循环会一直执行，直到按键被释放（引脚恢复高电平）
        // 这样可以确保一次按键只触发一次有效的检测，防止按键长按导致多次触发
        while (GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == Bit_RESET); // 等待按键松开
        delay_ms(20); // 松开延时消抖，等待按键松开后的机械抖动结束
    }
    // 检测KEY2 (PA12 - 增加阈值键)
    // 使用else if，确保每次只检测一个按键，并且在多个按键同时按下时，优先检测到第一个按键
    else if (GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == Bit_RESET)
    {
        delay_ms(20); // 延时消抖
        if (GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == Bit_RESET)
        {
            KeyNum = 2; // 设置按键编号为2
        }
        while (GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == Bit_RESET); // 等待按键松开
        delay_ms(20); // 松开延时消抖
    }
    // 检测KEY3 (PA11 - 减少阈值键)
    else if (GPIO_ReadInputDataBit(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == Bit_RESET)
    {
        delay_ms(20); // 延时消抖
        if (GPIO_ReadInputDataBit(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == Bit_RESET)
        {
            KeyNum = 3; // 设置按键编号为3
        }
        while (GPIO_ReadInputDataBit(KEY3_GPIO_PORT, KEY3_GPIO_PIN) == Bit_RESET); // 等待按键松开
        delay_ms(20); // 松开延时消抖
    }

    return KeyNum; // 返回检测到的按键编号，如果无按键按下则返回0
}
