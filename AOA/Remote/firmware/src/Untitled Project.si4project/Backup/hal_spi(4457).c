#include "hal_spi.h"


void HalSPI1_Init(void)
{
	SPI_InitTypeDef SPI_InitStructure;

	SPI_I2S_DeInit(SPI1);

	/* 使能APB2上的 SPI */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	// 设置SPI1模式
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI1_PRESCALER;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 10;

	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_SSOutputCmd(SPI1, DISABLE);
	SPI_Cmd(SPI1, ENABLE);
}

#ifdef HAL_SPI_OSE
void HalSPI1_Ose_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能APB2上的GPIOA和GPIOB */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,ENABLE);

	// SPI1 SCK与MOSI设置
	GPIO_InitStructure.GPIO_Pin = SPI1_SCK_PIN | SPI1_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI1_PORT, &GPIO_InitStructure);

	// SPI1 MISO设置
	GPIO_InitStructure.GPIO_Pin = SPI1_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_Init(SPI1_PORT, &GPIO_InitStructure);

	// SPI1 CS设置
	GPIO_InitStructure.GPIO_Pin = OSE_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OSE_CS_PORT, &GPIO_InitStructure);
	GPIO_SetBits(OSE_CS_PORT, OSE_CS_PIN);

	// RST 引脚
	GPIO_InitStructure.GPIO_Pin = OSE_RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OSE_RST_PORT, &GPIO_InitStructure);
	GPIO_SetBits(OSE_RST_PORT, OSE_RST_PIN);


	// EN引脚
	GPIO_InitStructure.GPIO_Pin = OSE_EN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OSE_EN_PORT, &GPIO_InitStructure);
	GPIO_SetBits(OSE_EN_PORT, OSE_EN_PIN);

	// READY引脚
	GPIO_InitStructure.GPIO_Pin = OSE_READY_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OSE_READY_PORT, &GPIO_InitStructure);
	GPIO_SetBits(OSE_READY_PORT, OSE_READY_PIN);


	// ENT/ENR引脚
	GPIO_InitStructure.GPIO_Pin = OSE_ENT_PIN | OSE_ENR_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OSE_ENT_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(OSE_ENT_PORT, OSE_ENT_PIN | OSE_ENR_PIN);



	GPIO_InitStructure.GPIO_Pin = OSE_IRQ_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(OSE_IRQ_PORT, &GPIO_InitStructure); 

	
}


void HalSPI1_Ose_Exit_Init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	GPIO_EXTILineConfig(OSE_IRQ_EXTI_PORT, OSE_IRQ_EXTI_PIN);

	EXTI_InitStructure.EXTI_Line = OSE_IRQ_EXTI;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;	//MPW3 IRQ polarity is high by default
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

void HalSPI1_Ose_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = OSE_IRQ_EXTI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

	NVIC_Init(&NVIC_InitStructure);
}


void HalSPI1_Ose_Init(void)
{
	HalSPI1_Ose_IO_Init();
	HalSPI1_Ose_Exit_Init();
	HalSPI1_Ose_NVIC_Config();
}
#endif


void HalSpiInit( void )
{
	HalSPI1_Init();

#ifdef HAL_SPI_OSE
	HalSPI1_Ose_Init();
#endif
}

