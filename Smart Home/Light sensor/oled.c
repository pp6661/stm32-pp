#include "oled.h"
#include "OLED_Font.h"

/* OLED I2C 引脚初始化 */
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能GPIOB时钟

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;      // 开漏输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     // 输出速率50MHz
    GPIO_InitStructure.GPIO_Pin = OLED_SCL | OLED_SDA;    // 配置SCL和SDA引脚
    GPIO_Init(OLED_PROT, &GPIO_InitStructure);            // 初始化GPIO

    OLED_W_SCL(1); // 设置SCL高电平
    OLED_W_SDA(1); // 设置SDA高电平
}

/* I2C起始信号 */
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

/* I2C停止信号 */
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* 通过I2C发送一个字节 */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) // 循环8次 逐位取出 Byte 的数据
    {
        OLED_W_SDA(Byte & (0x80 >> i)); // 从高位开始依次发送 
        OLED_W_SCL(1); // 拉高SCL产生时钟，产生时钟信号的上升沿，告诉从设备（OLED）当前SDA上的数据位有效，可以读取了
        OLED_W_SCL(0); // 拉低SCL 为下一位数据传输做准备
    }
    OLED_W_SCL(1); // 额外一个时钟周期，跳过应答
    OLED_W_SCL(0); //SCL拉低，完成时钟周期
}

/* 向OLED写入命令 */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start(); // 启动I2C通信
    OLED_I2C_SendByte(0x78); // OLED地址，写模式
    OLED_I2C_SendByte(0x00); // 写命令控制字 指示接下来发送的是命令
    OLED_I2C_SendByte(Command); // 写入命令
    OLED_I2C_Stop();
}

/* 向OLED写入数据 */
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78); // OLED地址，写模式
    OLED_I2C_SendByte(0x40); // 写数据控制字
    OLED_I2C_SendByte(Data); // 写入数据
    OLED_I2C_Stop();
}

/* 设置OLED显示位置 */
void OLED_SetCursor(uint8_t Y, uint8_t X) //Y（通常代表OLED的行）和 X（代表列）
{
    OLED_WriteCommand(0xB0 | Y);                    // 设置行地址
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    // 设置列地址高4位
    OLED_WriteCommand(0x00 | (X & 0x0F));           // 设置列地址低4位
}

/* 清屏 */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++) // OLED共8页
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++) // 每页128列
        {
            OLED_WriteData(0x00); // 写0清除像素
        }
    }
}

/* 显示单个ASCII字符 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i; // 循环计数器
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8); // 上半部分行和列
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]); // 显示上半部分
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); // 下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]); // 显示下半部分
    }
}

/* 显示字符串 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i; 
    for (i = 0; String[i] != '\0'; i++) //当遇到字符串的结束符 '\0' 时循环停止
    {
        OLED_ShowChar(Line, Column + i, String[i]); // 逐字符显示
    }
}

/* 显示汉字（16x16点阵） */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t num)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 16); // 上半部分位置
    for (i = 0; i < 16; i++)
    {
        OLED_WriteData(Hzk1[num][i]); // 显示上半部分
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 16); // 下半部分
    for (i = 0; i < 16; i++)
    {
        OLED_WriteData(Hzk1[num][i + 16]); // 显示下半部分
    }
}

/* 次方函数，返回X的Y次幂 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/* 显示正整数（十进制） */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* 显示带符号整数（十进制） */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1; //用于存储数字的绝对值
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/* 显示十六进制数（正整数） */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/* 显示二进制数（正整数） */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/* OLED初始化 */
void OLED_Init(void)
{
    uint32_t i, j;

    for (i = 0; i < 1000; i++)         // 上电延时
    {
        for (j = 0; j < 1000; j++);
    }

    OLED_I2C_Init();                  // 初始化I2C引脚

    OLED_WriteCommand(0xAE);         // 关闭显示

    OLED_WriteCommand(0xD5);         // 设置时钟分频比
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);         // 设置多路复用率
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);         // 设置显示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);         // 设置显示起始行

    OLED_WriteCommand(0xA1);         // 设置左右方向正常

    OLED_WriteCommand(0xC8);         // 设置上下方向正常

    OLED_WriteCommand(0xDA);         // 设置COM引脚配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);         // 设置对比度
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);         // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);         // 设置VCOMH电压
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);         // 设置显示输出（A4正常输出）

    OLED_WriteCommand(0xA6);         // 设置正常显示模式（A6正常 A7反色）

    OLED_WriteCommand(0x8D);         // 设置充电泵使能
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);         // 打开OLED显示

    OLED_Clear();                    // 清屏
}
