#include "hal_usart.h"

#define DMA1_MEM_LEN 256//保存DMA每次数据传送的长度
char _dbg_TXBuff[DMA1_MEM_LEN];

/**************************************************************************USART1 DMA打印设置**************************************************************************/

/**************************************************************************
函数名  ：USART1_SendBuffer
函数功能：串口1发送函数
		  由于使用的是普通模式,在每次发送完成后,重装载通道
		  装载好通道后等待发送完成。
入口参数：buffer 发送的参数  length 发送的长度
返回  值：length 发送完毕，返回发送数组长度
**************************************************************************/ 
uint16_t USART1_SendBuffer(const char* buffer, uint16_t length)
{
	if( (buffer==NULL) || (length==0) )
	{
		return 0;
	}
 
	DMA_Cmd(DMA1_Channel4, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel4, length);
	DMA_Cmd(DMA1_Channel4, ENABLE);
	while(1)
	{
		if(DMA_GetITStatus(DMA1_IT_TC4)!=RESET)	//判断通道4传输完成
		{
			DMA_ClearFlag(DMA1_IT_TC4);//清除通道4传输完成标志
			break;
		}
	}
	return length;
}


/**************************************************************************
函数名  ：_dbg_printf
函数功能：DMA方式的printf
入口参数：format
返回  值：无
**************************************************************************/ 
void _dbg_printf(const char *format,...)
{
		uint32_t length;
		va_list args;
	 
		va_start(args, format);
		length = vsnprintf((char*)_dbg_TXBuff, sizeof(_dbg_TXBuff), (char*)format, args);
		va_end(args);
		USART1_SendBuffer((const char*)_dbg_TXBuff,length); 
}

/**************************************************************************
函数名  ：HalUASRT1_DMA_Send_init
函数功能：串口1 DMA设置
入口参数：DMA_CHx  设置的DMA 通道
		  cpar     DMA外设基地址
		  cmar     DMA内存基地址
返回  值：无
**************************************************************************/
void HalUASRT1_DMA_Send_init(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar)
{
		DMA_InitTypeDef DMA_InitStructure;

		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	  DMA_DeInit(DMA_CHx);   //将DMA的通道1寄存器重设为缺省值
		//DMA1_MEM_LEN=cndtr;
		DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;  //DMA外设基地址
		DMA_InitStructure.DMA_MemoryBaseAddr = cmar;  //DMA内存基地址
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  //数据传输方向，从内存读取发送到外设
		DMA_InitStructure.DMA_BufferSize = DMA1_MEM_LEN;  //DMA通道的DMA缓存的大小
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //外设地址寄存器不变
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址寄存器递增
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //数据宽度为8位
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //数据宽度为8位
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //工作在正常模式
		DMA_InitStructure.DMA_Priority = DMA_Priority_High; //DMA通道 x拥有中优先级 
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMA通道x没有设置为内存到内存传输
		DMA_Init(DMA_CHx, &DMA_InitStructure); 
		USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
		DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
}

/**************************************************************************
函数名  ：HalUASRT1_DMA_Config
函数功能：串口1DMA初始化
		  关闭RXNE和TC中断,开启IDLE
		  配置RX,TX传输的目的地址和源地址
入口参数：无
返回  值：无
**************************************************************************/
void HalUASRT1_DMA_Config(void)
{
		USART_ITConfig(USART1, USART_IT_TC,DISABLE);
		USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
		
		HalUASRT1_DMA_Send_init(DMA1_Channel4,(u32)&USART1->DR,(u32)_dbg_TXBuff);//TX
}

/**************************************************************************
函数名  ：HalUSART1_Init
函数功能：串口1基础参数设置
入口参数：无
返回  值：无
**************************************************************************/
void HalUSART1_Init(u32 bound)
{
	USART_InitTypeDef USART_InitStructure;

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART1, &USART_InitStructure); //初始化串口
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_Cmd(USART1, ENABLE);                    //使能串口
}

/**************************************************************************
函数名  ：HalUSART1_IO_Init
函数功能：串口1 IO初始化
入口参数：无
返回  值：无
**************************************************************************/
void HalUSART1_IO_Init(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
			//USART1_TX   GPIOA.9
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.9
	   
	  //USART1_RX	  GPIOA.10初始化
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.10  
}

/**************************************************************************USART2 接收设置**************************************************************************/

/**************************************************************************
函数名  ：HalUASRT2_NVIC_Config
函数功能：串口2中断优先级设置
入口参数：无
返回  值：无
**************************************************************************/
void HalUASRT2_NVIC_Config(void)
{
	//Usart2 NVIC 配置
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
}

/**************************************************************************
函数名  ：HalUSART2_Init
函数功能：串口2基础参数设置
入口参数：无
返回  值：无
**************************************************************************/
void HalUSART2_Init(u32 bound)
{
	USART_InitTypeDef USART_InitStructure;

	//USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

	USART_Init(USART2, &USART_InitStructure); //初始化串口
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
	USART_Cmd(USART2, ENABLE);                    //使能串口
}

/**************************************************************************
函数名  ：HalUSART2_IO_Init
函数功能：串口2 IO初始化
入口参数：无
返回  值：无
**************************************************************************/
void HalUSART2_IO_Init(void)
{
	  GPIO_InitTypeDef GPIO_InitStructure;
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART2，GPIOA时钟
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);	//使能USART2，GPIOA时钟
			//USART2_TX   GPIOA.2
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.2
	   
	  //USART2_RX	  GPIOA.3初始化
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA3
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA.3
}

void HalUARTInit ( void )
{
		HalUSART1_IO_Init();
		HalUSART1_Init(115200);
		HalUASRT1_DMA_Config();
	
		HalUSART2_IO_Init();
		HalUSART2_Init(115200);
		HalUASRT2_NVIC_Config();	
}





















