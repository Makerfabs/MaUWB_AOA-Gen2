#include "stm32f10x.h"
#include "OSAL_Comdef.h"
#include <stdarg.h>
#include "stdio.h" 
/***************************************************************************************************
 * ´®¿Úº¯Êý
 ***************************************************************************************************/
void _dbg_printf(const char *format,...);

void HalUARTInit ( void );	

void HalUASRT1_NVIC_Config(void);

void HalUASRT2_NVIC_Config(void);

void HalUSART1_Init(u32 bound);

void HalUSART2_Init(u32 bound);

void HalUSART1_IO_Init(void);

void HalUSART2_IO_Init(void);