#include "hal_key.h"
#include "hal_usart.h"

hal_key_t hal_key = {
	.mode = HAL_KEY_MODE_PRESS_INVALID,		//按键模式,|①未按下|②短按|③长按 
	.key_sn = HAL_KEY_INVALID,
	.key_state = HAL_KEY_STATE_RECORD_LOOSE,
	.key_done = false,
	.press_timer=0,							//按下时长
	.loose_timer=0,							//松开时间
	
	.long_timer=3000,						//长按规定时间
	.dcd_timer=500,							//短按规定时间
	.handle_timer = 100						//双击/长按按键处理时长冗余
};


void HalKey_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 					 	//定义GPIO结构体

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);	//JTAG disable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);				//使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = HAL_KEY1_Up_PIN|HAL_KEY5_Down_PIN|HAL_KEY6_Remote_PIN|HAL_KEY7_Follow_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(HAL_KEY1_Up_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = HAL_KEY2_Left_PIN|HAL_KEY3_Shut_PIN|HAL_KEY4_Right_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(HAL_KEY2_Left_PORT, &GPIO_InitStructure);
}


void HalKey_Exit_Init(int state)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	
	GPIO_EXTILineConfig(HAL_KEY1_Up_PORT_SOURCE, HAL_KEY1_Up_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY1_Up_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY2_Left_PORT_SOURCE, HAL_KEY2_Left_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY2_Left_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY3_Shut_PORT_SOURCE, HAL_KEY3_Shut_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY3_Shut_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY4_Right_PORT_SOURCE, HAL_KEY4_Right_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY4_Right_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY5_Down_PORT_SOURCE, HAL_KEY5_Down_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY5_Down_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY6_Remote_PORT_SOURCE, HAL_KEY6_Remote_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY6_Remote_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
	
	GPIO_EXTILineConfig(HAL_KEY7_Follow_PORT_SOURCE, HAL_KEY7_Follow_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = HAL_KEY7_Follow_EXTI_LIN;/*中断线*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;/*触发模式*/
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;/*触发信号*/
	EXTI_InitStructure.EXTI_LineCmd = state;/*使能中断线*/
	EXTI_Init(&EXTI_InitStructure);/*调用库函数，初始化EXTI*/
}


void HalKey_NVIC_Config(int state)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY1_Up_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY2_Left_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY3_Shut_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY4_Right_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY5_Down_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY6_Remote_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
	
	NVIC_InitStructure.NVIC_IRQChannel = HAL_KEY7_Follow_EXIT_IRQN;/*配置选中的中断向量*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;/*配置抢占优先级*/
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;/*配置响应优先级*/
	NVIC_InitStructure.NVIC_IRQChannelCmd = state;/*使能中断向量*/
	NVIC_Init(&NVIC_InitStructure);/*调用库函数，初始化中断向量*/
}



void HalKey_IT_Enable(bool exitConfig, bool nvicConfig)
{
	if(exitConfig == true)
	{
		HalKey_Exit_Init(1);
	}

	if(nvicConfig == true)
	{
		HalKey_NVIC_Config(1);
	}
}

void HalKey_IT_Disable(bool exitConfig, bool nvicConfig)
{
	if(exitConfig == true)
	{
		HalKey_Exit_Init(0);
	}

	if(nvicConfig == true)
	{
		HalKey_NVIC_Config(0);
	}
}



static HAL_KEY_STATE HalKey_IO_Read(HAL_KEY_SN keys)
{
	HAL_KEY_STATE ret = HAL_KEY_STATE_RECORD_LOOSE;
	switch(keys)
	{
		case HAL_KEY1:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY1_Up_PORT, HAL_KEY1_Up_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		case HAL_KEY2:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY2_Left_PORT, HAL_KEY2_Left_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		case HAL_KEY3:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY3_Shut_PORT, HAL_KEY3_Shut_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;
		
		case HAL_KEY4:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY4_Right_PORT, HAL_KEY4_Right_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		case HAL_KEY5:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY5_Down_PORT, HAL_KEY5_Down_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		case HAL_KEY6:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY6_Remote_PORT, HAL_KEY6_Remote_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		case HAL_KEY7:
		{
			if(GPIO_ReadInputDataBit(HAL_KEY7_Follow_PORT, HAL_KEY7_Follow_PIN) == 0)
				ret = HAL_KEY_STATE_RECORD_PRESS;
		}
		break;

		default:
		break;
	}
	return ret;
}


/*
 * 按键参数初始化
 *
 */
void scan_keypara_init(void)
{
	hal_key.mode = HAL_KEY_MODE_PRESS_INVALID;
	hal_key.key_sn = HAL_KEY_INVALID;
	hal_key.key_state = HAL_KEY_STATE_RECORD_PRESS;
	hal_key.key_done = false;
	hal_key.press_timer=0;			//按下时长
	hal_key.loose_timer=0;			//松开时间
	
	hal_key.long_timer=3000;		//长按规定时间
	hal_key.dcd_timer=300;			//双击规定间隔时间
	hal_key.handle_timer = 100;		//处理时长
}


/*
 * 按键循环
 *
 */
void Key_Scan(uint32_t timer)
{
	static uint16_t handle_start_timer = 0;
	bool key_judge = false;

	switch(hal_key.mode)
	{
		//按键未操作
		case HAL_KEY_MODE_PRESS_INVALID:
		{
#if 0
			if(HalKey_IO_Read(HAL_KEY3) == HAL_KEY_STATE_RECORD_PRESS)
			{
				hal_key.key_sn = HAL_KEY3;
			}

			//任意按键按下
			if(hal_key.key_sn != HAL_KEY_INVALID)
			{
				hal_key.press_timer = timer;
				hal_key.key_state = HAL_KEY_STATE_RECORD_PRESS;
				hal_key.mode = HAL_KEY_MODE_PRESS_ING;
			}
#endif
			for(int key = HAL_KEY1; key <= HAL_KEY7; key++)
			{
				if(HalKey_IO_Read(key) == HAL_KEY_STATE_RECORD_PRESS)
				{
					hal_key.key_sn = key;
					hal_key.press_timer = timer;
					hal_key.key_state = HAL_KEY_STATE_RECORD_PRESS;
					hal_key.mode = HAL_KEY_MODE_PRESS_ING;
					break;
				}
			}
		}
		break;

		//按键正在处理
		case HAL_KEY_MODE_PRESS_ING:
		{
			if((timer - hal_key.press_timer) > hal_key.long_timer)					//长按按键
			{
				handle_start_timer = timer;
				hal_key.key_done = false;
				hal_key.mode = HAL_KEY_MODE_PRESS_LONG;
			}
			else if(HalKey_IO_Read(hal_key.key_sn) == HAL_KEY_STATE_RECORD_LOOSE)	//短按按键
			{
				hal_key.loose_timer = timer;
				hal_key.key_state = HAL_KEY_STATE_RECORD_LOOSE;
				hal_key.mode = HAL_KEY_MODE_PRESS_DOUBE_JUDGE;
			}
		}
		break;

		//按键短按/双击判断
		case HAL_KEY_MODE_PRESS_DOUBE_JUDGE:
		{
			bool button_double_press = false;

			if(HalKey_IO_Read(hal_key.key_sn) == HAL_KEY_STATE_RECORD_PRESS)
			button_double_press = true;


			if(button_double_press == true && 
			((timer - hal_key.loose_timer) < hal_key.dcd_timer))
			{
				handle_start_timer = timer;
				hal_key.key_done = false;
				hal_key.mode = HAL_KEY_MODE_PRESS_DOUBLE;
			}
			else if((timer - hal_key.loose_timer) > hal_key.dcd_timer)
			{
				handle_start_timer = timer;
				hal_key.key_done = false;
				// 没有双击,但短按
				hal_key.mode = (hal_key.loose_timer - hal_key.press_timer >= 100)?(HAL_KEY_MODE_PRESS_SHORT):(HAL_KEY_MODE_PRESS_ERROR);
			}
		}
		break;

		//误按
		case HAL_KEY_MODE_PRESS_ERROR:
		{
			_dbg_printf("error pressed:%d,:%d\n", hal_key.key_sn, hal_key.loose_timer - hal_key.press_timer);
			hal_key.key_done = true;
			key_judge = true;
		}
		break;

		//短按
		case HAL_KEY_MODE_PRESS_SHORT:
		{
			_dbg_printf("single pressed:%d\n",hal_key.key_sn);
			App_Moudle_Device_TurnOn();
			hal_key.key_done = true;
			key_judge = true;
		}
		break;

		//双击
		case HAL_KEY_MODE_PRESS_DOUBLE:
		{
			if(hal_key.key_done == false)
			{
				_dbg_printf("double pressed:%d\n",hal_key.key_sn);
				hal_key.key_done = true;
				App_Moudle_Device_TurnOn();
				App_Moudle_Device_SetAlarm();
			}
		}
		break;

		//长按
		case HAL_KEY_MODE_PRESS_LONG:
		{
			if(hal_key.key_done == false)
			{
				_dbg_printf("long pressed:%d\n",hal_key.key_sn);
				hal_key.key_done = true;
				App_Moudle_Device_TurnOn();
				App_Moudle_Device_TurnOff();
			}
		}
		break;

		default:
			_dbg_printf("按键无效模式\n");
		break;
	}

	if((hal_key.mode == HAL_KEY_MODE_PRESS_LONG) || (hal_key.mode == HAL_KEY_MODE_PRESS_DOUBLE))
	{
		if((timer - handle_start_timer) > hal_key.handle_timer && HalKey_IO_Read(hal_key.key_sn) == HAL_KEY_STATE_RECORD_LOOSE)
		{
			key_judge = true;
		}
	}

	if(key_judge == true)
	{
//		_dbg_printf("key rtc2\n");
		scan_keypara_init();
		HalKey_IT_Enable(true, true);
		HalTimer4_IT_Disable();
	}
}

//
//返回值true :无按键事件
//返回值false:有按键事件
//
bool HalKey_Loose_Judge(void)
{
	bool ret = false;
	if(hal_key.mode == HAL_KEY_MODE_PRESS_INVALID)
		ret = true;

	return ret;
}

//
//返回值true :有效按键事件(防止STM32 内部上拉 导致误触发的bug)
//返回值false:无效按键事件
//
bool HalKey_ActiveChick_Judge(void)
{
	bool ret = false;
	if(hal_key.mode >= HAL_KEY_MODE_PRESS_SHORT)
		ret = true;

	return ret;
}

















/***************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initialize Key Service
 *
 * @param   init - pointer to void that contains the initialized value
 *
 * @return  None
 ***************************************************************************************************/
void HalKeyInit (void)
{
#ifdef HAL_KEY_POWER_ISR

#endif

#ifdef HAL_KEY_ACT_ISR
	HalKey_IO_Init();
	HalKey_IT_Enable(1, 1);
#endif
}


/***************************************************************************************************
***************************************************************************************************/
