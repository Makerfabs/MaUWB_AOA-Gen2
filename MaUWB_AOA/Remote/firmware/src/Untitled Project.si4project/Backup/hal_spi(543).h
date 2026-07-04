#ifndef __HAL_SPI_H
#define __HAL_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include "stm32f10x.h"	
#include "string.h"	
#include "OSAL_Comdef.h"

/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/

#define HAL_SPI_OSE


//DECAWAVE SPI端口定义
#define SPI1_PRESCALER                 SPI_BaudRatePrescaler_32

//Decawave SPI端口A
#define OSE_RST_PORT                   GPIOA
#define OSE_RST_PIN                    GPIO_Pin_0	//PA0
#define OSE_CS_PORT                    GPIOA
#define OSE_CS_PIN                     GPIO_Pin_4	//PA4
#define OSE_EN_PORT                    GPIOB
#define OSE_EN_PIN                     GPIO_Pin_0	//PB0
#define OSE_READY_PORT                 GPIOB
#define OSE_READY_PIN                  GPIO_Pin_11	//PA11
#define OSE_IRQ_PORT                   GPIOB
#define OSE_IRQ_PIN                    GPIO_Pin_5	//PB5
#define OSE_ENT_PORT                   GPIOC
#define OSE_ENT_PIN                    GPIO_Pin_11	//PC11
#define OSE_ENR_PORT                   GPIOC
#define OSE_ENR_PIN                    GPIO_Pin_10	//PC10


#define OSE_IRQ_EXTI                   EXTI_Line5
#define OSE_IRQ_EXTI_PORT              GPIO_PortSourceGPIOB
#define OSE_IRQ_EXTI_PIN               GPIO_PinSource5
#define OSE_IRQ_EXTI_IRQn              EXTI9_5_IRQn


//Decawave 公共端口
#define SPI1_PORT                      GPIOA
#define SPI1_SCK_PIN                   GPIO_Pin_5
#define SPI1_MISO_PIN                  GPIO_Pin_6
#define SPI1_MOSI_PIN                  GPIO_Pin_7

#define SPIx                           SPI1

/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/


/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/



/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	

/*
 * Initialize SPI Service.
 */
extern void HalSpiInit(void);

static void HalSPI1_Init(void);

static void HalSPI1_Ose_Init(void);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif
