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
//#include "OSAL_Comdef.h"
//#include "hal_spi.h"
//#include "hal_flash.h"
//#include "hal_led.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "Generic.h"

//#include "uwb.h"
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

extern int Wait_time;
extern uint8_t Prevent_send_bit;
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
	/* o to infinite loop when Memory Manage exception occurs */
	while (1)
	{
//		NVIC_SystemReset(); //复位
	}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{


	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
//		NVIC_SystemReset(); //复位
	}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
	
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
//		NVIC_SystemReset(); //复位
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
	static int count = 0;
	local_time32_incr++;//系统时间
	HalDelayTime_Counter();
	
		if(local_time32_incr%3000 == 0)//1s 判断一次
	{
			HalAdcGetState();
	}
	
	
	if(local_time32_incr%100 == 0)//100ms 判断一次
	{
		if(sys_data.eth_rst_time != 0) 								//计数10s 恢复出厂模式模块
		{

			if(--sys_data.eth_rst_time == 0)
			{
				USART4_printf("复位完毕\r\n");
				HalEth_Rst_Off();
			}
		}
		else
		{
			LED_flicker(count++,10);
		}
	}
}


void DMA1_Channel4_IRQHandler(void)
{
		if(DMA_GetFlagStatus(DMA1_FLAG_TC4))      
	{    
		if(Prevent_send_bit==1)//发送完1包开始计时
		{
			Prevent_send_bit=3;//开始计时
			Wait_time=portGetTickCnt();
//			USART4_printf("计时 %d\r\n",Wait_time);//回复
			//开始计时
		}
		
		sys_data.Data_UART_Busy=true;
		DMA_Cmd(DMA1_Channel4, DISABLE);//数据传输完成，关闭DMA4通道
		DMA_ClearFlag(DMA1_FLAG_TC4); 	//清除DMA1通道7数据完成标志
		USART_ClearFlag(USART1,USART_FLAG_TC);
		USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE); //关闭USART的DMA发送请求		
	}		
}
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(HAL_KEY3_EXTI_LIN) != RESET)
	{
		if(GPIO_ReadInputDataBit(HAL_KEY3_PORT, HAL_KEY3_PIN) == 0){
			HalKey3_IT_Disable();//关闭中断线
			HalTimer4_IT_Enable();//开启定时器1
		}
		/* Clear EXTI Line 9 Pending Bit */
		EXTI_ClearITPendingBit(HAL_KEY3_EXTI_LIN);
	}
		if(EXTI_GetITStatus(HAL_KEY2_EXTI_LIN) != RESET)
	{
		if(GPIO_ReadInputDataBit(HAL_KEY2_PORT, HAL_KEY2_PIN) == 0){
//			HalKey3_IT_Disable();//关闭中断线
//			HalTimer4_IT_Enable();//开启定时器1
//			USART4_printf("按键按下\r\n");
		}
		/* Clear EXTI Line 9 Pending Bit */
		EXTI_ClearITPendingBit(HAL_KEY2_EXTI_LIN);
	}
	
	

}






