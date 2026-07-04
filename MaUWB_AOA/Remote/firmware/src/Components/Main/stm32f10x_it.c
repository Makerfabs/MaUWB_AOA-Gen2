/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "OSAL_Comdef.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_key.h"
#include "Generic.h"

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
	
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	_dbg_printf("HardFault_Handler\n");

	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //¸´Î»
	}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
	_dbg_printf("MemManage_Handler\n");

	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //¸´Î»
	}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
	_dbg_printf("BusFault_Handler\n");

	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //¸´Î»
	}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
	_dbg_printf("UsageFault_Handler\n");
	
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //¸´Î»
	}
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
extern void HalDelayTime_Counter(void);
void SysTick_Handler(void)
{
	local_time32_incr++;
	
	HalDelayTime_Counter();
}

void EXTI1_IRQHandler(void)
{
	// ÔÝÍ£°´¼ü
	if(EXTI_GetITStatus(HAL_KEY3_Shut_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY3_Shut_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY3_Shut_PORT, HAL_KEY3_Shut_PIN) == 0)
		{
			HalKey_IT_Disable();
			HalTimer4_IT_Enable();
		}
	}
}


void EXTI15_10_IRQHandler(void)
{
	// Ò£¿Ø°´¼ü
	if(EXTI_GetITStatus(HAL_KEY6_Remote_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY6_Remote_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY6_Remote_PORT, HAL_KEY6_Remote_PIN) == 0)
		{
			HalKey_IT_Disable();
			HalTimer4_IT_Enable();
		}
	}
	// ¸úËæ°´¼ü
	else if(EXTI_GetITStatus(HAL_KEY7_Follow_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY7_Follow_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY7_Follow_PORT, HAL_KEY7_Follow_PIN) == 0)
		{
			HalKey_IT_Disable();
			HalTimer4_IT_Enable();
		}
	}
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/


/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
