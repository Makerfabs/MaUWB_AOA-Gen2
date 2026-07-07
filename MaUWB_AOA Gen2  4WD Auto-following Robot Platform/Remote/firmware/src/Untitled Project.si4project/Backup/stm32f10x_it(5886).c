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
#include "hal_spi.h"
#include "hal_flash.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_key.h"
#include "Generic.h"

#include "ose_uwb.h"
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
	sys_para.HardFault_error_bit +=1;
	App_Module_Sys_Write_NVM();

	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //复位
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
	sys_para.MemManage_error_bit +=1;
	App_Module_Sys_Write_NVM();

	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //复位
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
	sys_para.BusFault_error_bit +=1;
	App_Module_Sys_Write_NVM();

	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //复位
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
	sys_para.UsageFault_error_bit +=1;
	App_Module_Sys_Write_NVM();
	
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
		NVIC_SystemReset(); //复位
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

	//按键报警时间
	if(sys_state.system_alarm.alarm_timer != 0)
	{
		if(--sys_state.system_alarm.alarm_timer == 0)
		{
			HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_OFF);
			sys_state.para_tag.tag_detail_para.is_alarm = false;
			sys_state.system_alarm.alarm = false;
			sys_state.system_alarm.alarm_timer = 0;
		}
	}


	// 两秒都未关机,则重新启动
	if(sys_state.system_turnoff == true && sys_state.system_DoneState != Sys_DoneState_DeepSleep)
	{
		if(portGetTickCnt() - sys_state.system_turnoff_stime  > 2000)
		{
			NVIC_SystemReset();
		}
	}
	
	// 检查定时器是否真正启动
	if(hal_timer.timer_state == HAL_TIMER_STATE_READY_OPEN && (portGetTickCnt() - hal_timer.timer_openTickCnt) >= 30)
	{
		TIM_Cmd(TIM4,ENABLE);
		hal_timer.timer_openTickCnt = portGetTickCnt();
	}
}

void EXTI9_5_IRQHandler(void)
{
//	_dbg_printf("EXTI9_5_IRQHandler 中断开始处理 \n");
	if(EXTI_GetITStatus(OSE_IRQ_EXTI) != RESET)
	{
		EXTI_ClearITPendingBit(OSE_IRQ_EXTI);
		process_ose_irq();
	}
	else if(EXTI_GetITStatus(HAL_KEY2_Left_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY2_Left_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY2_Left_PORT, HAL_KEY2_Left_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
			_dbg_printf("EXTI9_5_3IRQHandler\n");

//			_dbg_printf("按键2触发\n");
		}
	}
	else if(EXTI_GetITStatus(HAL_KEY3_Shut_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY3_Shut_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY3_Shut_PORT, HAL_KEY3_Shut_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
			_dbg_printf("EXTI9_5_4IRQHandler\n");

//			_dbg_printf("按键3触发\n");
		}
	}
}


void EXTI15_10_IRQHandler(void)
{
//	_dbg_printf("EXTI9_5_IRQHandler 中断开始处理 \n");
	if(EXTI_GetITStatus(HAL_KEY4_Right_EXTI_LIN) != RESET)
	{
		if(GPIO_ReadInputDataBit(HAL_KEY4_Right_PORT, HAL_KEY4_Right_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
//			_dbg_printf("按键4触发\n");
		}
		EXTI_ClearITPendingBit(HAL_KEY4_Right_EXTI_LIN);
	}
	else if(EXTI_GetITStatus(HAL_KEY5_Down_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY5_Down_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY5_Down_PORT, HAL_KEY5_Down_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
//			_dbg_printf("按键5触发\n");
		}
	}
	else if(EXTI_GetITStatus(HAL_KEY6_Remote_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY6_Remote_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY6_Remote_PORT, HAL_KEY6_Remote_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
//			_dbg_printf("按键6触发\n");
		}
	}
	else if(EXTI_GetITStatus(HAL_KEY7_Follow_EXTI_LIN) != RESET)
	{
		EXTI_ClearITPendingBit(HAL_KEY7_Follow_EXTI_LIN);
		if(GPIO_ReadInputDataBit(HAL_KEY7_Follow_PORT, HAL_KEY7_Follow_PIN) == 0)
		{
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
//			_dbg_printf("按键7触发\n");
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


void EXTI1_IRQHandler(void)
{
	if(EXTI_GetITStatus(HAL_KEY1_Up_EXTI_LIN) != RESET)
	{
		if(GPIO_ReadInputDataBit(HAL_KEY1_Up_PORT, HAL_KEY1_Up_PIN) == 0)
		{
//			_dbg_printf("按键1触发\n");
			HalPmu_SYSCLKConfig_STOP(1);
			HalKey_IT_Disable(true, false);
			HalTimer4_IT_Enable();
		}
		EXTI_ClearITPendingBit(HAL_KEY1_Up_EXTI_LIN);
	}
}



void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //清除TIMx更新中断标志 
		period_tick_us++;
	}
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/
