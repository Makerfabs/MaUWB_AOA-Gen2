#include "stm32f10x.h"

/***************************************************************************************************
 * 基础外设 IO 定义
 ***************************************************************************************************/
#define LED_GP      GPIOC
#define LED_RCC     RCC_APB2Periph_GPIOC
#define LED1_PIN     GPIO_Pin_4
#define LED2_PIN     GPIO_Pin_5

#define BEEP_GP      GPIOD
#define BEEP_RCC     RCC_APB2Periph_GPIOD
#define BEEP_PIN     GPIO_Pin_2

/***************************************************************************************************
 * 基础外设 IO操作
 ***************************************************************************************************/
#define ON  0
#define OFF 1
#define LED1(a) if(a) \
											GPIO_SetBits(LED_GP,LED1_PIN); \
								else \
											GPIO_ResetBits(LED_GP,LED1_PIN)//蓝灯

								
#define LED2(a) if(a) \
											GPIO_SetBits(LED_GP,LED2_PIN); \
								else \
											GPIO_ResetBits(LED_GP,LED2_PIN)//红灯

#define BEEP(a) if(a) \
											GPIO_SetBits(BEEP_GP,BEEP_PIN); \
								else \
											GPIO_ResetBits(BEEP_GP,BEEP_PIN)
								

/***************************************************************************************************
 * 基础外设初始化
 ***************************************************************************************************/
void Periph_init(void);
