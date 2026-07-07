#include "ose_uwb.h"
#include "hal_timer.h"

volatile unsigned long local_time32_incr;				//本地时钟
volatile unsigned long period_tick_us;					//定时器1us时钟(1个周期60ms,uint32可计时,2982天)


uint32_t test_s_timer, test_e_timer, test_m_timer;


/******************************************
*               获取系统时钟，1ms读取1次
*******************************************/
unsigned long portGetTickCnt(void)
{
	return local_time32_incr;
}

// 单位:1us/0.001ms
unsigned long get_1us_tick_value(void)
{
	uint32_t tick_1us = 60000 * period_tick_us + TIM_GetCounter(TIM2);
	return tick_1us;
}

/******************************************
*               休眠函数1
*******************************************/
void sleep_ms(unsigned int time_ms)
{
	unsigned long end = portGetTickCnt() + time_ms;
	while ((signed long)(portGetTickCnt() - end) <= 0)
	    ;
}


/******************************************
*               设置SPI速率
*******************************************/
void SPI_ConfigFastRate(uint16_t scalingfactor)
{
	SPI_InitTypeDef SPI_InitStructure;

	SPI_I2S_DeInit(SPIx);

	// SPIx Mode setup
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = scalingfactor; //sets BR[2:0] bits - baudrate in SPI_CR1 reg bits 4-6
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 10;

	SPI_Init(SPIx, &SPI_InitStructure);

	// Enable SPIx
	SPI_Cmd(SPIx, ENABLE);
}
/******************************************
*               设置SPI低速率 低于3MHz
*******************************************/
void port_set_uwb_spi_slowrate(void)
{
	SPI_ConfigFastRate(SPI_BaudRatePrescaler_32);
}

/******************************************
*               设置SPI低速率 尽可能接近20MHz
*******************************************/
void port_set_uwb_spi_fastrate(void)
{
	SPI_ConfigFastRate(SPI_BaudRatePrescaler_4);
}


/*
 * 唤醒WAKEUP
 *
 */
void port_wakeup_uwb_chip(void)
{
	port_cs_uwb_chip(true); 
	sleep_ms(1);   
	port_cs_uwb_chip(false);  
	sleep_ms(6);
}

/*
 * 复位RESET
 *
 */
void port_reset_uwb_chip(void)
{
}

/*
 * 使能EN
 *
 */
void port_en_uwb_chip(bool set)
{
	if(set)
	{
		GPIO_SetBits(OSE_EN_PORT, OSE_EN_PIN);
	}
	else
	{
		GPIO_ResetBits(OSE_EN_PORT, OSE_EN_PIN);
	}
}

/*
 * 片选CS
 *
 */
void port_cs_uwb_chip(bool set)
{
	if(set)
	{
		GPIO_ResetBits(OSE_CS_PORT, OSE_CS_PIN);
	}
	else
	{
		GPIO_SetBits(OSE_CS_PORT, OSE_CS_PIN);
	}
}

/*
 * 中断IRQ
 *
 */
void port_irq_uwb_chip(bool set)
{
	// 配置成:中断口
	if(set)
	{
//		set_irq_to_exint();
	}
	// 配置成:GPIO口
	else
	{
//		set_irq_to_gpio();
	}
}

void port_pa_uwb_chip(paMode_e paMode)
{
	if(paMode == paMode_tx)
	{
		GPIO_SetBits(OSE_ENT_PORT, OSE_ENT_PIN);
		GPIO_ResetBits(OSE_ENT_PORT, OSE_ENR_PIN);
	}
	else if(paMode == paMode_rx)
	{
		GPIO_ResetBits(OSE_ENT_PORT, OSE_ENT_PIN);
		GPIO_SetBits(OSE_ENT_PORT, OSE_ENR_PIN);
	}
	else if(paMode == paMode_ByPass)
	{
		GPIO_ResetBits(OSE_ENT_PORT, OSE_ENT_PIN);
		GPIO_ResetBits(OSE_ENT_PORT, OSE_ENR_PIN);
	}
	else if(paMode == paMode_forBidden)
	{
		GPIO_SetBits(OSE_ENT_PORT, OSE_ENT_PIN);
		GPIO_SetBits(OSE_ENT_PORT, OSE_ENR_PIN);
	}
}


/******************************************
*               获取中断引脚状态
*******************************************/
ITStatus EXTI_GetITEnStatus(uint32_t EXTI_Line)
{
	ITStatus bitstatus = RESET;
	uint32_t enablestatus = 0;
	/* Check the parameters */
	assert_param(IS_GET_EXTI_LINE(EXTI_Line));

	enablestatus =  EXTI->IMR & EXTI_Line;
	if (enablestatus != (uint32_t)RESET)
	{
		bitstatus = SET;
	}
	else
	{
		bitstatus = RESET;
	}
	return bitstatus;
}



/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 */
 
int writetospi_serial
(
    uint16_t       headerLength,
    const uint8_t *headerBuffer,
    uint32_t       bodyLength,
    const uint8_t *bodyBuffer
)
{
	int i=0;

	int  stat ;
	for(i=0; i<headerLength; i++)
	{
		SPIx->DR = headerBuffer[i];

		while ((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

		SPIx->DR ;
	}

	for(i=0; i<bodyLength; i++)
	{
		SPIx->DR = bodyBuffer[i];

		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

		SPIx->DR ;
	}

//	if(bodyLength == 213)
//	{
//		test_m_timer = get_1us_tick_value();
//	}

	
	return 0;
} // end writetospi()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns -1 if there was an error
 */
 
int readfromspi_serial
(
    uint16_t       headerLength,
    const uint8_t *headerBuffer,
    uint32_t       readlength,
    uint8_t       *readBuffer
)
{

	int i=0;

	int  stat ;

	for(i=0; i<headerLength; i++)
	{
		SPIx->DR = headerBuffer[i];

		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

		readBuffer[0] = SPIx->DR ; // Dummy read as we write the header
	}

	for(i=0; i<readlength; i++)
	{
		SPIx->DR = 0;  // Dummy write as we read the message body

		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

		readBuffer[i] = SPIx->DR ;//port_SPIx_receive_data(); //this clears RXNE bit
	}

	return 0;
} // end readfromspi()



/**
 * 功能：ose中断回调函数
 *
 */
void process_ose_irq(void)
{
//	while(port_CheckEXT_IRQ() != 0)
	{
		ose_isr();
	} //while OSE IRQ line active
}


void disable_ose_irq(void)
{
	port_DisableEXT_IRQ();
	EXTI_ClearITPendingBit(OSE_IRQ_EXTI);
}

void enable_ose_irq(void)
{
	EXTI_ClearITPendingBit(OSE_IRQ_EXTI);
	port_EnableEXT_IRQ();
}



uint16_t SPI1_MDA_SendBuffer(const char* buffer, uint16_t length)
{
	DMA_Cmd(DMA1_Channel3, DISABLE);					//数据传输完成，关闭DMA4通道
	DMA1_Channel3->CNDTR = length;						//数据传输数目
	DMA1_Channel3->CMAR = (u32)buffer;					//内存地址
	DMA_Cmd(DMA1_Channel3, ENABLE); 					//使能DMA通道4
}

