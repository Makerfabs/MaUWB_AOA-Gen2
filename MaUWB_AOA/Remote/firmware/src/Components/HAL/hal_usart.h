#ifndef __HAL_UART_H
#define __HAL_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

	
/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/	
#include "stdio.h" 
#include "OSAL_Comdef.h"
#include "stm32f10x.h"	

	
	
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/
//#define HAL_LOG(fmt, ...)        _dbg_printf_DMA("[HAL] " fmt "\n", ##__VA_ARGS__)
#define HAL_LOG(fmt, ...)        ((void)0)

//#define SYS_LOG(fmt, ...)        _dbg_printf_DMA("[SYS] " fmt "\n", ##__VA_ARGS__)
#define SYS_LOG(fmt, ...)        ((void)0)

//#define UWB_LOG(fmt, ...)        _dbg_printf_DMA("[UWB] " fmt "\n", ##__VA_ARGS__)
#define UWB_LOG(fmt, ...)       ((void)0)


#define HAL_USART1					/*调试口*/
#define HAL_USART1_DMA

#define HAL_USART3
#define HAL_USART3_DMA

/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/
extern uint8_t  pc_rx_cache[512+256];				//PC     DMA接受地址
extern uint8_t  uwb_rx_cache[1024];				//UWB    DMA接受地址

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	
extern void _dbg_printf(const char *format,...);
extern void _dbg_printf_DMA(const char *format,...);

extern uint16_t USART1_SendBuffer(const char* buffer, uint16_t length, int flag);
extern uint16_t USART3_SendBuffer(const char* buffer, uint16_t length);

extern void HalUARTInit(void); 

static void HalUSART1_Init(u32 bound);
static void HalUSART1_IO_Init(void);
static void HalUSART1_DMA_Config(void);
static void HalUSART1_DMA_TX_NVIC_Config(void);
static void HalUSART1_IRQ_RX_NVIC_Config(void);



static void HalUSART3_Init(u32 bound);
static void HalUSART3_IO_Init(void);
static void HalUSART3_DMA_Config(void);
static void HalUSART3_DMA_TX_NVIC_Config(void);
static void HalUSART3_DMA_RX_NVIC_Config(void);
static void HalUSART3_IRQ_RX_NVIC_Config(void);


#ifdef __cplusplus
}
#endif

#endif	//__HAL_UART_H
