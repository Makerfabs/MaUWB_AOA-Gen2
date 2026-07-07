#ifndef __HAL_TIMER_H
#define __HAL_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include "OSAL_Comdef.h"
#include "stm32f10x.h"	

	
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/	
#define HAL_SysTick

#define HAL_TIMER4
#define HAL_IWDG

#define CLOCKS_PER_SEC      1000


#define TIM4_ARR            99		//10ms
#define TIM4_PSC            7199

extern volatile unsigned long local_time32_incr;
/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/
	
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	


static void HalTimeSysTick_Init(void);
static void HalIWDG_Init(unsigned char prer,unsigned int rlr);
extern void HalIWDG_Feed(void);
extern void HalTimerInit(void);
extern void HalDelay_nMs(uint32_t nms);
extern void Sleep_us(uint16_t time);

extern unsigned long portGetTickCnt(void);

extern void HalTimer4_IT_Enable();

extern void HalTimer4_IT_Disable();

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif//__HAL_TIMER_H

