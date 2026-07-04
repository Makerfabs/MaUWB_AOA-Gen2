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

#define HAL_KEY_POWER_ISR
#define HAL_KEY_ACT_ISR

//充电IO口
#define HAL_KEY_PGOOD_PORT              GPIOB
#define HAL_KEY_PGOOD_PIN               GPIO_Pin_13

#define HAL_KEY_CHG_PORT                GPIOB
#define HAL_KEY_CHG_PIN                 GPIO_Pin_12

// 上(PA1)
#define HAL_KEY1_Up_PORT                GPIOB
#define HAL_KEY1_Up_PIN                 GPIO_Pin_8

// 左(PB9)
#define HAL_KEY2_Left_PORT              GPIOB
#define HAL_KEY2_Left_PIN               GPIO_Pin_9

// 暂停(PB8)
#define HAL_KEY3_Shut_PORT              GPIOA 
#define HAL_KEY3_Shut_PIN               GPIO_Pin_1
#define HAL_KEY3_Shut_PORT_SOURCE       GPIO_PortSourceGPIOA
#define HAL_KEY3_Shut_PIN_SOURCE        GPIO_PinSource1
#define HAL_KEY3_Shut_EXTI_LIN          EXTI_Line1
#define HAL_KEY3_Shut_EXIT_IRQN         EXTI1_IRQn

// 右(PB10)
#define HAL_KEY4_Right_PORT             GPIOB 
#define HAL_KEY4_Right_PIN              GPIO_Pin_10

// 下(PA15)
#define HAL_KEY5_Down_PORT              GPIOA
#define HAL_KEY5_Down_PIN               GPIO_Pin_15

// 遥控(PA12)
#define HAL_KEY6_Remote_PORT            GPIOA
#define HAL_KEY6_Remote_PIN             GPIO_Pin_12
#define HAL_KEY6_Remote_PORT_SOURCE     GPIO_PortSourceGPIOA
#define HAL_KEY6_Remote_PIN_SOURCE      GPIO_PinSource12
#define HAL_KEY6_Remote_EXTI_LIN        EXTI_Line12
#define HAL_KEY6_Remote_EXIT_IRQN       EXTI15_10_IRQn

// 跟随(PA11)
#define HAL_KEY7_Follow_PORT            GPIOA
#define HAL_KEY7_Follow_PIN             GPIO_Pin_11
#define HAL_KEY7_Follow_PORT_SOURCE     GPIO_PortSourceGPIOA
#define HAL_KEY7_Follow_PIN_SOURCE      GPIO_PinSource11
#define HAL_KEY7_Follow_EXTI_LIN        EXTI_Line11
#define HAL_KEY7_Follow_EXIT_IRQN       EXTI15_10_IRQn


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
	HAL_KEY7,
	HAL_KEYCNT,
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
extern void HalKeyInit(void);

extern void HalKey_IT_Enable(void);

extern void HalKey_IT_Disable(void);

extern bool HalKey_Loose_Judge(void);

extern HAL_KEY_STATE HalKey_IO_Read(HAL_KEY_SN keys);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif
