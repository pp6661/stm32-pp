#include "stm32f10x.h"     // SPL 库主头文件
#include "oled.h"          // OLED 驱动头文件（需实现 OLED_Init/OLED_Clear/OLED_ShowString 等）
#include "bh1750.h"        // BH1750 驱动头文件
#include "stdio.h"         // 用于 sprintf 格式化字符串
#include "delay.h"         // 延时函数头文件

// 引入 SPL 库外设头文件
#include "stm32f10x_rcc.h"   // 时钟控制
#include "stm32f10x_gpio.h"  // GPIO 控制
#include "stm32f10x_i2c.h"   // I2C 外设控制
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// 私有变量
uint16_t lux_value = 0;    // 存储光照度值
char lux_str[20] = {0};    // 格式化光照度字符串
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
// 定义 BH1750 使用的硬件 I2C 外设
#define BH1750_I2C_PERIPH I2C1  

// OLED 引脚宏（根据电路：PB14=SCL，PB15=SDA ）
#ifndef OLED_SCL_GPIO_PIN
#define OLED_SCL_GPIO_PORT    GPIOB
#define OLED_SCL_GPIO_PIN     GPIO_Pin_14
#define OLED_SDA_GPIO_PORT    GPIOB
#define OLED_SDA_GPIO_PIN     GPIO_Pin_15
#endif
/* USER CODE END 0 */
// ============================================================================
// 函数名称：Error_Handler
// 功能描述：错误处理函数，可扩展 LED 闪烁等报错逻辑
// ============================================================================
void Error_Handler(void) //当这个函数被执行时，微控制器会进入一个无限循环，有效地暂停所有正常的程序执行。
{
    while (1)
    {
        // 可在此添加“报错 LED 闪烁”等逻辑，例如：
        // GPIO_SetBits(GPIOC, GPIO_Pin_13);
        // delay_ms(200);
        // GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        // delay_ms(200);
    }
}

// ============================================================================
// 函数名称：SystemClock_Config
// 功能描述：系统时钟配置（配置为 72MHz 示例，根据硬件晶振调整）
// ============================================================================
void SystemClock_Config(void)
{
    RCC_DeInit();                  // 重置 RCC 寄存器，确保在进行新的时钟配置之前，所有旧的设置都被清除，避免潜在的冲突
    RCC_HSEConfig(RCC_HSE_ON);     // 使能外部高速时钟（HSE）
    
    if (RCC_WaitForHSEStartUp() == SUCCESS) // 等待 HSE 稳定
    {
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // 使能 FLASH 预取
        FLASH_SetLatency(FLASH_Latency_2);                    // FLASH 延时 2 周期，确保从 Flash 读取数据的稳定性

        // 配置总线分频：AHB=72MHz，APB2=72MHz，APB1=36MHz
        RCC_HCLKConfig(RCC_SYSCLK_Div1);   //AHB 是高性能总线，连接 CPU、DMA、Flash 等
        RCC_PCLK2Config(RCC_HCLK_Div1);    //APB2 连接高速外设，如 GPIO、ADC、TIM1 等
        RCC_PCLK1Config(RCC_HCLK_Div2);    //APB1 连接低速外设，如 USART2/3、I2C、SPI2/3、TIM2/3/4 等

        // 配置锁相环PLL：HSE 作为时钟源，8MHz * 9 = 72MHz（外部晶振 8MHz）
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9); 
        RCC_PLLCmd(ENABLE);               // 使能 PLL
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // 等待 PLL 锁定

        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // 选择 PLL 作为系统时钟
        while (RCC_GetSYSCLKSource() != 0x08);     // 等待系统时钟切换到 PLL
    }
    else
    {
        Error_Handler(); // HSE 启动失败，进入错误循环
    }
}

// ============================================================================
// 函数名称：MX_GPIO_Init
// 功能描述：GPIO 初始化（配置 I2C、OLED 引脚）
// ============================================================================
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 使能 GPIOB 时钟（BH1750、OLED 均用到 GPIOB）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 2. 配置 BH1750 硬件 I2C 引脚（PB6=SCL，PB7=SDA）
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD; // 复用开漏输出（硬件 I2C 要求）
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. 配置 OLED 软件 I2C 引脚（PB14=SCL，PB15=SDA ，开漏输出）
    GPIO_InitStruct.GPIO_Pin = OLED_SCL_GPIO_PIN | OLED_SDA_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD; // 普通开漏输出（软件模拟 I2C）
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_PORT, &GPIO_InitStruct);
}

// ============================================================================
// 函数名称：MX_I2C1_Init
// 功能描述：I2C1 初始化（BH1750 硬件 I2C 驱动）
// ============================================================================
void MX_I2C1_Init(void)
{
    I2C_InitTypeDef I2C_InitStruct = {0};

    // 使能 I2C1 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // 配置 I2C 参数
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;             // I2C 模式
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;      // 占空比 2
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;               // 自身地址（未使用可填 0）
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;             // 使能应答
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // 7 位地址
    I2C_InitStruct.I2C_ClockSpeed = 400000;              // 400KHz 高速模式

    I2C_Init(I2C1, &I2C_InitStruct); // 初始化 I2C1
    I2C_Cmd(I2C1, ENABLE);           // 使能 I2C1 外设
}

int main(void)
{
    /* 1. 系统初始化 */
    SystemClock_Config(); // 配置系统时钟为 72MHz
    MX_GPIO_Init();       // 初始化 GPIO（I2C、OLED 引脚）
    MX_I2C1_Init();       // 初始化硬件 I2C1（BH1750 用）
    delay_init(72);       // 延时函数初始化（参数为系统时钟 72MHz）
    OLED_Init();          // 初始化 OLED 显示屏
    OLED_Clear();         // 清屏

    /* 2. 初始化 BH1750：连续高分辨率模式 */
    if (BH1750_Init(BH1750_I2C_PERIPH, BH1750_MODE_CONTINUOUS_HIGH_RES_MODE) != BH1750_OK)
    {
        Error_Handler(); // 传感器初始化失败，进入错误循环
    }

    /* 3. 主循环：持续读取光照度并显示 */
    while (1)
    {
        // 读取 BH1750 光照度数据
        if (BH1750_ReadLux(BH1750_I2C_PERIPH, &lux_value) == BH1750_OK)
        {
            // 格式化光照度字符串（如 "Lux: 123 lx" ）
            sprintf(lux_str, "Lux: %d lx", lux_value);

            OLED_Clear();                   // 清屏
            OLED_ShowString(1, 1, "Light Sensor"); // 第 1 行显示标题 
            OLED_ShowString(3, 1, "By: LiLu 15"); 
            OLED_ShowString(2, 1, lux_str);       // 第 3 行显示光照度值
        }
        else
        {
            OLED_Clear();
            OLED_ShowString(1, 1, "Sensor Error!"); // 读取失败提示
            OLED_ShowString(3, 1, "By: LiLu 15");  // 错误信息时也显示名字
        }

        delay_ms(500); // 每隔 500ms 读取一次
    }
}
