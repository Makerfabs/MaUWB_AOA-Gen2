#ifndef __HAL_LED_H
#define __HAL_LED_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include "stm32f10x.h"	
#include "string.h"	
#include "OSAL_Comdef.h"
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/		

//LED端口定义
#define HAL_LED_RCC         RCC_APB2Periph_GPIOA
#define HAL_LED_PORT        GPIOA

#define HAL_LED1_PIN        GPIO_Pin_3			//绿灯(工作指示灯)
#define HAL_LED2_PIN        GPIO_Pin_2			//红灯(报警指示灯)
#define HAL_LED3_PIN        NULL				//蓝灯(充电指示灯)
#define HAL_LED4_PIN        NULL
	
#define HAL_BEEP_RCC        RCC_APB2Periph_GPIOA
#define HAL_BEEP_PORT       GPIOA					
#define HAL_BEEP_PIN        NULL			   

/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/
enum Sys_mode_led_blink{
	Sys_mode_led_blink_invalid = 0,

	/*闪烁一次周期 */
	Sys_mode_led_blink_fast_time = 1,			//0.1s
	Sys_mode_led_blink_slow_time = 10,			//1s
};


typedef enum {
	HAL_LED1 = 0,			//绿灯
	HAL_LED2,				//红灯
	HAL_LED3,				//蓝灯
	HAL_LED4,
	HAL_LED_ALL
}HAL_LED_SN;

typedef enum {
	HAL_LED_MODE_ON = 0,
	HAL_LED_MODE_OFF = 1,
	HAL_LED_MODE_TOGGLE
}HAL_LED_MODE;

typedef enum {
	HAL_BEEP1 = 1,
	HAL_BEEP_ALL
}HAL_BEEP_SN;

typedef enum {
	HAL_BEEP_MODE_ON = 0,
	HAL_BEEP_MODE_OFF = 1,
	HAL_BEEP_MODE_TOGGLE
}HAL_BEEP_MODE;

typedef struct{
	uint8_t mode;				//工作状态 0：ON 	1：OFF 	2：TOGGLE
	uint16_t period;			//周期
}hal_led_t;

typedef struct{
		uint8_t mode;			//工作状态 0：ON 	1：OFF 	2：TOGGLE
		uint16_t period;		//周期
}hal_beep_t;
/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/

extern hal_led_t hal_led[HAL_LED_ALL];
extern hal_beep_t hal_beep[HAL_BEEP_ALL];

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	

/*
 * 使能LED
 */
extern void HalLedInit( void );

/*
 * 设置LED 打开/关闭/切换
 */
extern void HalLedSet (HAL_LED_SN leds, HAL_LED_MODE mode);

/*
 * 设置BEEP 打开/关闭/切换
 */
extern void HaBeepSet (HAL_BEEP_SN beeps, HAL_BEEP_MODE mode);


extern int HalLed_Mode_Set(HAL_LED_SN led, HAL_LED_MODE mode, uint16_t period);


extern void HalLed_Blink(uint16_t count);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif
