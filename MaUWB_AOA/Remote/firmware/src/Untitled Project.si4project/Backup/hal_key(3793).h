#ifndef __HAL_KEY_H
#define __HAL_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include <stdbool.h>
#include "stm32f10x.h"	
#include "OSAL_Comdef.h"



/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/		

#define xHAL_KEY_POWER_ISR
#define HAL_KEY_ACT_ISR
	

#define HAL_KEY1_PORT               GPIOA
#define HAL_KEY1_PIN                GPIO_Pin_1
#define HAL_KEY1_PORT_SOURCE        GPIO_PortSourceGPIOA
#define HAL_KEY1_PIN_SOURCE         GPIO_PinSource1
#define HAL_KEY1_EXTI_LIN           EXTI_Line1
#define HAL_KEY1_EXIT_IRQN          EXTI1_IRQn

#define HAL_KEY2_PORT               GPIOB 
#define HAL_KEY2_PIN                GPIO_Pin_9
#define HAL_KEY2_PORT_SOURCE        GPIO_PortSourceGPIOB
#define HAL_KEY2_PIN_SOURCE         GPIO_PinSource9
#define HAL_KEY2_EXTI_LIN           EXTI_Line9
#define HAL_KEY2_EXIT_IRQN          EXTI9_5_IRQn

#define HAL_KEY3_PORT               GPIOB 
#define HAL_KEY3_PIN                GPIO_Pin_8
#define HAL_KEY3_PORT_SOURCE        GPIO_PortSourceGPIOB
#define HAL_KEY3_PIN_SOURCE         GPIO_PinSource8
#define HAL_KEY3_EXTI_LIN           EXTI_Line8
#define HAL_KEY3_EXIT_IRQN          EXTI9_5_IRQn

#define HAL_KEY4_PORT               GPIOB 
#define HAL_KEY4_PIN                GPIO_Pin_10
#define HAL_KEY4_PORT_SOURCE        GPIO_PortSourceGPIOB
#define HAL_KEY4_PIN_SOURCE         GPIO_PinSource10
#define HAL_KEY4_EXTI_LIN           EXTI_Line10
#define HAL_KEY4_EXIT_IRQN          EXTI15_10_IRQn

#define HAL_KEY5_PORT               GPIOA
#define HAL_KEY5_PIN                GPIO_Pin_15
#define HAL_KEY5_PORT_SOURCE        GPIO_PortSourceGPIOA
#define HAL_KEY5_PIN_SOURCE         GPIO_PinSource15
#define HAL_KEY5_EXTI_LIN           EXTI_Line15
#define HAL_KEY5_EXIT_IRQN          EXTI15_10_IRQn

#define HAL_KEY6_PORT               GPIOA
#define HAL_KEY6_PIN                GPIO_Pin_12
#define HAL_KEY6_PORT_SOURCE        GPIO_PortSourceGPIOA
#define HAL_KEY6_PIN_SOURCE         GPIO_PinSource12
#define HAL_KEY6_EXTI_LIN           EXTI_Line12
#define HAL_KEY6_EXIT_IRQN          EXTI15_10_IRQn

#define HAL_KEY7_RCC                RCC_APB2Periph_GPIOA
#define HAL_KEY7_PORT               GPIOA
#define HAL_KEY7_PIN                GPIO_Pin_11
#define HAL_KEY7_PORT_SOURCE        GPIO_PortSourceGPIOA
#define HAL_KEY7_PIN_SOURCE         GPIO_PinSource11
#define HAL_KEY7_EXTI_LIN           EXTI_Line11
#define HAL_KEY7_EXIT_IRQN          EXTI15_10_IRQn


/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/
typedef enum {
	HAL_KEY_INVALID = 0,
	HAL_KEY1,
	HAL_KEY2,
	HAL_KEY3,
	HAL_KEY4,
	HAL_KEY5,
	HAL_KEY6,
	HAL_KEY7
}HAL_KEY_SN;
	
typedef enum {
	HAL_KEY_MODE_PRESS_INVALID = 0, //按键初始化状态
	HAL_KEY_MODE_PRESS_ING, 		//按键正在处理
	HAL_KEY_MODE_PRESS_DOUBE_JUDGE, //按键短按/双击判断
	HAL_KEY_MODE_PRESS_ERROR,		//按键误按
	HAL_KEY_MODE_PRESS_SHORT,		//按键短按
	HAL_KEY_MODE_PRESS_DOUBLE,		//按键双击
	HAL_KEY_MODE_PRESS_LONG 		//按键长按
}HAL_KEY_MODE;

typedef enum{
	HAL_KEY_STATE_RECORD_PRESS, 	//按键按下
	HAL_KEY_STATE_RECORD_LOOSE, 	//按键松开
}HAL_KEY_STATE;


typedef struct{
	uint8_t mode;			//按键模式,|①未按下|②短按|③长按 
	uint8_t key_sn; 		//按键分辨(哪一个按键)
	uint8_t key_state;		//按键状态
	uint8_t key_done;		//按键事件处理
	uint32_t press_timer;	//按下时长
	uint32_t loose_timer;	//松开时间
	
	uint16_t long_timer;	//长按规定时间
	uint16_t dcd_timer; 	//双击规定间隔时间(double chick duration)

	uint16_t handle_timer;	//双击/长按按键处理时长冗余
}hal_key_t;


extern hal_key_t hal_key;
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	

/*
 * Initialize the Key Service
 */
extern void HalKeyInit( void );

extern void HalKey3_IT_Enable(void);

extern void HalKey3_IT_Disable(void);

extern bool HalKey_Loose_Judge(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif
