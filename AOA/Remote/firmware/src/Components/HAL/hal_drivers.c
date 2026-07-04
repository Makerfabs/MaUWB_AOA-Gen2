#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_usart.h"
#include "hal_adc.h"

/**************************************************************************************************
 * @fn      Hal_DriverInit
 *
 * @brief   Initialize HW - These need to be initialized before anyone.
 *
 * @param   task_id - Hal TaskId
 *
 * @return  None
 **************************************************************************************************/
void Hal_Driver_Init (void)
{
	/* CFG 中断优先级组*/
	Hal_NVIC_Init(HAL_NVIC_GROUP_SN);
	
	/* TIMER */
#if (defined HAL_TIMER) && (HAL_TIMER == TRUE)
	HalTimerInit();//滴答定时器+看门狗+定时器2设置
#endif

	/* LED */
#if (defined HAL_LED) && (HAL_LED == TRUE)
	HalLedInit(true);//LED显示
#endif

	/* UART */
#if (defined HAL_USART) && (HAL_USART == TRUE)
	HalUARTInit();//串口1DMA输出
#endif

	/* ADC */
#if (defined HAL_ADC) && (HAL_ADC == TRUE)
	HalAdcInit();
#endif

	/* KEY */
#if (defined HAL_KEY) && (HAL_KEY == TRUE)
	HalKeyInit();
#endif
}

