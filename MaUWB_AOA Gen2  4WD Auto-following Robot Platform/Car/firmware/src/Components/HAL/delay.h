#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"


/***************************************************************************************************
 * àÖàª¶¨Ê±Æ÷º¯Êý
 ***************************************************************************************************/
void SysTick_Init(void);
void Delay_us(u32 nTime);
void Delay_ms(u32 nTime);  
void TimingDelay_Decrement(void);
unsigned long portGetTickCnt(void);
#endif
