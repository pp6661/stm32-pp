#include "bh1750.h"
#include "delay.h" 
#include "stm32f10x_i2c.h" 

// -----------------------------------------------------------
// 辅助函数：I2C 写字节 (SPL )
// -----------------------------------------------------------
BH1750_STATUS I2C_WriteBytes_SPL(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t* pBuffer, uint16_t NumByteToWrite)
{
    // 1. 发送起始条件
    I2C_GenerateSTART(I2Cx, ENABLE);
    // 等待EV5：SB=1, MSL=1, BUSY=1
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 2. 发送设备地址和写方向
    I2C_Send7bitAddress(I2Cx, DeviceAddr, I2C_Direction_Transmitter);
    // 等待EV6：ADDR=1, MSL=1, BUSY=1, TXE=1, TRA=1
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    // 3. 发送数据
    while(NumByteToWrite--)
    {
        I2C_SendData(I2Cx, *pBuffer++);
        // 等待EV8：TXE=1, BUSY=1, TRA=1, MSL=1
        while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
    }

    // 4. 发送停止条件
    I2C_GenerateSTOP(I2Cx, ENABLE);
    // 等待总线空闲
    while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));

    return BH1750_OK;
}

// -----------------------------------------------------------
// 辅助函数：I2C 读字节 (SPL )
// -----------------------------------------------------------
BH1750_STATUS I2C_ReadBytes_SPL(I2C_TypeDef* I2Cx, uint8_t DeviceAddr, uint8_t* pBuffer, uint16_t NumByteToRead)
{
    // 1. 发送起始条件
    I2C_GenerateSTART(I2Cx, ENABLE);
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 2. 发送设备地址和读方向
    I2C_Send7bitAddress(I2Cx, DeviceAddr | 0x01, I2C_Direction_Receiver); // | 0x01 表示读
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    // 3. 读取数据
    while(NumByteToRead)
    {
        if (NumByteToRead == 1)
        {
            I2C_AcknowledgeConfig(I2Cx, DISABLE); // 读取最后一个字节时发送NACK
            I2C_GenerateSTOP(I2Cx, ENABLE);       // 接收最后一个字节后发送停止条件
        }

        // 等待EV7：RXNE=1
        while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED));
        *pBuffer++ = I2C_ReceiveData(I2Cx); // 读取数据
        NumByteToRead--;
    }

    I2C_AcknowledgeConfig(I2Cx, ENABLE); // 恢复ACK使能
    while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY)); // 等待总线空闲

    return BH1750_OK;
}

// -----------------------------------------------------------
// BH1750 驱动函数 
// -----------------------------------------------------------

BH1750_STATUS BH1750_Init(I2C_TypeDef *I2Cx, uint8_t mode)
{
    // 重置传感器
    if(BH1750_OK != BH1750_Reset(I2Cx)) return BH1750_ERROR;
    delay_ms(10); // 传感器复位后需要一点时间

    // 设置初始模式
    if(BH1750_OK != BH1750_SetMode(I2Cx, mode)) return BH1750_ERROR;

    // 可以选择设置默认 MTreg，但通常在设置模式时已经生效
    // if(BH1750_OK == BH1750_SetMtreg(I2Cx, BH1750_DEFAULT_MTREG))
    //     return BH1750_OK;
    return BH1750_OK; // 如果前面的步骤都成功，则返回OK
}


//	向 BH1750 光照传感器发送一个复位命令

BH1750_STATUS BH1750_Reset(I2C_TypeDef *I2Cx)
{
	uint8_t cmd = BH1750_RESET;
	return I2C_WriteBytes_SPL(I2Cx, BH1750_ADDRESS, &cmd, 1);
}

//	向 BH1750 光照传感器发送一个命令，以设置其工作模式。

BH1750_STATUS BH1750_SetMode(I2C_TypeDef *I2Cx, uint8_t mode)
{
    // 简单的模式检查，实际可能需要更严格的验证
    if (!((mode == BH1750_MODE_CONTINUOUS_HIGH_RES_MODE) ||
          (mode == BH1750_MODE_CONTINUOUS_HIGH_RES_MODE_2) ||
          (mode == BH1750_MODE_CONTINUOUS_LOW_RES_MODE) ||
          (mode == BH1750_MODE_ONE_TIME_HIGH_RES_MODE) ||
          (mode == BH1750_MODE_ONE_TIME_HIGH_RES_MODE_2) ||
          (mode == BH1750_MODE_ONE_TIME_LOW_RES_MODE))) {
        return BH1750_ERROR; // 无效模式
    }

	return I2C_WriteBytes_SPL(I2Cx, BH1750_ADDRESS, &mode, 1);
}


//	设置 BH1750 光照传感器的测量时间寄存器

BH1750_STATUS BH1750_SetMtreg(I2C_TypeDef *I2Cx, uint8_t mtreg)
{
	if (mtreg < 31 || mtreg > 254) { //如果传入的 mtreg 值小于 31 或大于 254，则认为这是一个无效值
		return BH1750_ERROR;
	}

	uint8_t tmp[2];
	tmp[0] = (0x40 | (mtreg >> 5)); // 高位字节
	tmp[1] = (0x60 | (mtreg & 0x1F)); // 低位字节

    // BH1750 MTreg设置需要分两次发送
	if(BH1750_OK != I2C_WriteBytes_SPL(I2Cx, BH1750_ADDRESS, &tmp[0], 1)) return BH1750_ERROR;
    if(BH1750_OK != I2C_WriteBytes_SPL(I2Cx, BH1750_ADDRESS, &tmp[1], 1)) return BH1750_ERROR;

    return BH1750_OK;
}


//	从 BH1750 光照传感器读取原始测量数据，并将其转换为实际的勒克斯 (Lux) 值

BH1750_STATUS BH1750_ReadLux(I2C_TypeDef *I2Cx, uint16_t *lux)
{
	uint8_t tmp[2];
    float result;

    // 等待测量完成 (根据模式设置的测量时间)
    // 简化处理，实际应用中可以根据具体模式设置不同的延迟
    // 持续高分辨率模式和单次高分辨率模式典型测量时间120ms
    // 低分辨率模式典型测量时间16ms
    // 这里简单延时，或者在main中根据模式进行延时
    delay_ms(150); // 给传感器足够时间完成测量

	if(BH1750_OK == I2C_ReadBytes_SPL(I2Cx, BH1750_ADDRESS, tmp, 2))
	{  //将第一个字节 (tmp[0]) 左移 8 位，使其成为 16 位数据的高字节，将左移后的高字节与第二个字节 (tmp[1]) 进行按位或操作，使其成为 16 位数据的低字节。
		result = (float)((tmp[0] << 8) | (tmp[1])); 
        *lux = (uint16_t)(result / BH1750_CONVERSION_FACTOR); //勒克斯值计算和转换，将计算出的浮点数勒克斯值强制转换为 uint16_t 类型，无小数部分
		return BH1750_OK;
	}
	return BH1750_ERROR;
}
