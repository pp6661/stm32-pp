#include "led.h"
#include "delay.h"

void LED_Init(void)
{
    // 开启 GPIOC 时钟（根据实际连接修改）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    // 配置为推挽输出模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;     // 对应PB3引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
    
    // 初始状态：LED关闭（低电平）
    LED_Off();  
}

void LED_Toggle(void)
{
    // 翻转电平
    GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, 
                  (BitAction)((1 - GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED_GPIO_PIN))));
}

void LED_On(void)
{
    // 高电平点亮LED（触发报警）
    GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, Bit_SET);  
}

void LED_Off(void)
{
    // 低电平熄灭LED（关闭报警）
    GPIO_WriteBit(LED_GPIO_PORT, LED_GPIO_PIN, Bit_RESET);
}
