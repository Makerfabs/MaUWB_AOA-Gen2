#include <stdarg.h>
#include <stdbool.h>
#include "hal_usart.h"


uint8_t  pc_rx_cache[512+256];					//PC      DMA接受地址
uint8_t  uwb_rx_cache[1024];					//uwb     DMA接受地址


/*
 *******************************************************************************
 *   串口1发送函数
 *	 由于使用的是普通模式,在每次发送完成后,重装载通道
 *	 装载好通道后等待发送完成。
 *******************************************************************************
 */
uint16_t USART1_SendBuffer(const char* buffer, uint16_t length, int flag)
{
	if(flag == true)
	{
		for(int i = 0; i < length; i++)
		{
			USART1->SR;

			/* e.g. 给USART写一个字符 */
			USART_SendData(USART1, (uint8_t) buffer[i]);

			/* 循环直到发送完成 */
			while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
		}
	}
	else
	{
		DMA_Cmd(DMA1_Channel4, DISABLE);					//数据传输完成，关闭DMA4通道
		DMA1_Channel4->CNDTR = length;						//数据传输数目
		DMA1_Channel4->CMAR = (u32)buffer;					//内存地址
		DMA_Cmd(DMA1_Channel4, ENABLE); 					//使能DMA通道4
	}
	return length;
}

/*
 *******************************************************************************
 *   串口2发送函数
 *	 由于使用的是普通模式,在每次发送完成后,重装载通道
 *	 装载好通道后等待发送完成。
 *******************************************************************************
 */
uint16_t USART3_SendBuffer(const char* buffer, uint16_t length)
{
	DMA_Cmd(DMA1_Channel2, DISABLE);					//数据传输完成，关闭DMA4通道
	DMA1_Channel2->CNDTR = length;						//数据传输数目
	DMA1_Channel2->CMAR = (u32)buffer;					//内存地址
	DMA_Cmd(DMA1_Channel2, ENABLE); 					//使能DMA通道4
	return length;
}


/*
 *******************************************************************************
 *		DMA方式的_dbg_printf
 *******************************************************************************
 */
void _dbg_printf(const char *format,...)
{
	static char _dbg_TXBuff[512];

	uint32_t length;
	va_list args;
 
	va_start(args, format);
	length = vsnprintf((char*)_dbg_TXBuff, sizeof(_dbg_TXBuff), (char*)format, args);
	va_end(args);

	USART1_SendBuffer((const char*)_dbg_TXBuff,length, true); 
}


/*
 *******************************************************************************
 *		DMA方式的_dbg_printf
 *******************************************************************************
 */
void _dbg_printf_DMA(const char *format,...)
{
	static char _dbg_TXBuff[256];

	uint32_t length;
	va_list args;
 
	va_start(args, format);
	length = vsnprintf((char*)_dbg_TXBuff, sizeof(_dbg_TXBuff), (char*)format, args);
	va_end(args);

	PC_Uart_sendCmd_Dma((const char*)_dbg_TXBuff,length); 
}

/*
 *******************************************************************************
 *		串口1
 *******************************************************************************
 */
void HalUSART1_Init(u32 bound)
{
	USART_InitTypeDef USART_InitStructure;

	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;								//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;				//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;					//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;						//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;			//收发模式

	USART_Init(USART1, &USART_InitStructure);								//初始化串口
//	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);							//开启串口接受中断
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);							//开启串口接受中断
	USART_Cmd(USART1, ENABLE);												//使能串口
}

void HalUSART1_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	//USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;									//PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;								//复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);										//初始化GPIOA.9

	//USART1_RX   GPIOA.10初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;									//PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;								//上拉输入(因为RX不上拉，悬空的话，电路的某些信号影响（比如射频，大功率器件），就容易误触发，进入接收中断。改成上拉就可以解决。)
	GPIO_Init(GPIOA, &GPIO_InitStructure);										//初始化GPIOA.10  
}

void HalUASRT1_DMA_Config(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

#ifdef HAL_USART1_DMA
	//TX发送DMA
	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(USART1->DR);			//DMA外设基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						//外设作为数据传输目的地
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度8bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 	//内存数据宽度8bit
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 							//正常模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; 					//优先级：高
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//非内存到内存
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);	

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);							//配置DMA发送完成后产生中断
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);							//使能USART的DMA发送请求

	//RX接受DMA
	DMA_DeInit(DMA1_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(USART1->DR);			//DMA外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)(&pc_rx_cache[0]);			//DMA内存基地址
	DMA_InitStructure.DMA_BufferSize = sizeof(pc_rx_cache);					//DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度8bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 	//内存数据宽度8bit
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 						//
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; 					//优先级：高
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//非内存到内存
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);	

	DMA_Cmd(DMA1_Channel5, ENABLE);											//数据传输完成，关闭DMA4通道
	DMA1_Channel5->CNDTR = sizeof(pc_rx_cache);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, DISABLE);							//DMA接收缓冲区满中断使能
	USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);

#endif
}

void HalUSART1_DMA_TX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void HalUSART1_DMA_RX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void HalUSART1_IRQ_RX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*Usart1 NVIC配置*/
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=10;				//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;						//从优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;							//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);											//根据指定的参数初始化VIC寄存器 
}







/*
 *******************************************************************************
 *		串口3
 *******************************************************************************
 */
void HalUSART3_Init(u32 bound)
{
	USART_InitTypeDef USART_InitStructure;

	//USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;								//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;				//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;					//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;						//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;			//收发模式

	USART_Init(USART3, &USART_InitStructure);								//初始化串口
//	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);							//开启串口接受中断
	USART_ITConfig(USART3, USART_IT_IDLE, DISABLE);							//开启串口接受中断
	USART_Cmd(USART3, DISABLE);                    							//使能串口
}

void HalUSART3_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);					//使能USART3，GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);					//使能USART3，GPIOA时钟
	//USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//USART3_RX   GPIOB.11初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
}


void HalUSART3_DMA_Config(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

#ifdef HAL_USART3_DMA
	// TX发送DMA配置(DMA1_Chanel2)
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(USART3->DR);			//DMA外设基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						//外设作为数据传输目的地
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度8bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 	//内存数据宽度8bit
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 							//正常模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; 					//优先级：高
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//非内存到内存
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);	

	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);							//配置DMA发送完成后产生中断
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);							//使能USART的DMA发送请求
	
	
	// RX接受DMA配置(DMA_Chanel3)
	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(USART3->DR);			//DMA外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)(uwb_rx_cache);			//DMA内存基地址
	DMA_InitStructure.DMA_BufferSize = sizeof(uwb_rx_cache[0]);					//DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;						//
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度8bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte; 	//内存数据宽度8bit
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 						//
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; 					//优先级：高
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;							//非内存到内存
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);	

	DMA_Cmd(DMA1_Channel3, ENABLE);											//数据传输完成，关闭DMA4通道
	DMA1_Channel3->CNDTR = sizeof(uwb_rx_cache[0]);
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, DISABLE);							//DMA接收缓冲区满中断使能
	USART_DMACmd(USART3, USART_DMAReq_Rx, DISABLE);
#endif
}

void HalUSART3_DMA_TX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void HalUSART3_DMA_RX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void HalUSART3_IRQ_RX_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*Usart4 NVIC配置*/
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;				//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;						//从优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;							//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);											//根据指定的参数初始化VIC寄存器 
}









void HalUARTInit(void)
{
#ifdef HAL_USART1
		HalUSART1_IO_Init();
		HalUSART1_Init(115200);
	#ifdef HAL_USART1_DMA
		HalUASRT1_DMA_Config(); 
		HalUSART1_DMA_TX_NVIC_Config();
//		HalUSART1_DMA_RX_NVIC_Config(); 
		HalUSART1_IRQ_RX_NVIC_Config();
	#endif
#endif

	
#ifdef HAL_USART3
	HalUSART3_IO_Init();
	HalUSART3_Init(115200);
	#ifdef HAL_USART3_DMA
		HalUSART3_DMA_Config(); 
		HalUSART3_DMA_TX_NVIC_Config();
		HalUSART3_DMA_RX_NVIC_Config();
		HalUSART3_IRQ_RX_NVIC_Config();
	#endif
#endif
}


