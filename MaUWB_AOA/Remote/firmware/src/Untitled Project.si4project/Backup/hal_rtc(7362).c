#include "hal_rtc.h"
#include "hal_key.h"

//#define PWR_FLAG_WU               ((uint32_t)0x00000001)	//唤醒标志位
//#define PWR_FLAG_SB               ((uint32_t)0x00000002)	//待机标志位
//#define PWR_FLAG_PVDO             ((uint32_t)0x00000004)	//PVD监测标志位


hal_rtc_t hal_rtc = {
	.valid = false,
	.start_calibation = false,
	.scale = 1.0,
	.calibation_clock = 5000,
	.systick_clock = 0
};


void HalRTC_Exit_Init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	/* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}


/*
 * 函数名：RTC_NVIC_Config
 * 描述  ：RCT中断分组设置
 * 输入  ：无
 * 输出  ：无
 */	 
void HalRTC_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;			//RTCAlarm_IRQn中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;	//先占优先级1位,从优先级3位
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//先占优先级0位,从优先级4位
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能该通道中断
	NVIC_Init(&NVIC_InitStructure);								//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}

uint8_t HalRTCInit (void)
{
	//检查是不是第一次配置时钟
	uint8_t temp=0;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟
	PWR_BackupAccessCmd(ENABLE);												//使能允许后备寄存器访问

	{
		BKP_DeInit();
#ifdef USE_CLOCK_LSE
		RCC_LSEConfig(RCC_LSE_ON);
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET&&temp<250)			//检查指定的RCC标志位设置与否,等待低速晶振就绪
		{
			temp++;
			HalDelay_nMs(10);
		}
		if(temp>=250)return -1;													//初始化时钟失败,晶振有问题
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);									//选择LSE外部RTC时钟源
#else
		RCC_LSICmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET&&temp<250)			//检查指定的RCC标志位设置与否,等待低速晶振就绪
		{
			temp++;
			HalDelay_nMs(10);
		}
		if(temp>=250)return -1;													//初始化时钟失败,晶振有问题
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);									//选择LSI内部RTC时钟源
#endif

		RCC_RTCCLKCmd(ENABLE);													//使能RTC时钟
		RTC_WaitForLastTask();													//等待写入完成
		RTC_WaitForSynchro();													//等待RTC寄存器与APB1同步
		RTC_ITConfig(RTC_IT_ALR, ENABLE);										//使能ALR中断
		RTC_WaitForLastTask();													//等待写入完成
		RTC_EnterConfigMode();													//进入配置RTC模式
		RTC_SetPrescaler(31+1);													//设置RTC时钟分频：是RTC定时周期为1ms，RTC周期=RTCCLK/RTC_PR = 32.768KHz/31+1 = 1.024ms
		RTC_WaitForLastTask();													//等待写入完成
		RTC_ExitConfigMode();													//退出配置模式
		PWR_BackupAccessCmd(DISABLE);											//禁止后备寄存器访问
	}

	HalRTC_Exit_Init();
	HalRTC_NVIC_Config();

	return 0; //ok
}

void HalRTC_Calibation(int sleep_ms)
{
	hal_rtc.valid            = false;
	hal_rtc.start_calibation = true;
	hal_rtc.scale            = 1.0;
	hal_rtc.calibation_clock = sleep_ms;
	hal_rtc.systick_clock    = portGetTickCnt();
	
	RTCAlarm_Set((uint32_t)(hal_rtc.calibation_clock / hal_rtc.scale));
}


//设置闹钟时间
void RTCAlarm_Set(int sleep_ms)
{
	sleep_ms = HalRTC_GetCount(sleep_ms);

	{
		EXTI_ClearITPendingBit(EXTI_Line17);									//防止误触发
		RTC_ClearFlag(RTC_FLAG_ALR);											//防止误触发
		
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
		PWR_BackupAccessCmd(ENABLE);											//使能允许后备寄存器访问
		RTC_WaitForLastTask();													//等待写入完成
		RTC_EnterConfigMode();													//进入配置RTC模式
		RTC_WaitForLastTask();													//等待写入完成
		RTC_SetAlarm(RTC_GetCounter()+ sleep_ms);								//设置报警时间
		RTC_WaitForLastTask();													//等待写入完成
		RTC_ExitConfigMode();													//退出配置模式
		PWR_BackupAccessCmd(DISABLE);											//禁止后备寄存器访问
	}

	HalRTC_Exit_Init();
	HalRTC_NVIC_Config();
	RTC_ITConfig(RTC_IT_ALR, ENABLE);											//使能ALR中断
}

void RTCAlarm_Close(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	/* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn; 		//RTCAlarm_IRQn中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;	//先占优先级1位,从优先级3位
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//先占优先级0位,从优先级4位
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;			//使能该通道中断
	NVIC_Init(&NVIC_InitStructure); 							//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	PWR_WakeUpPinCmd(DISABLE);
}




uint32_t HalRTC_GetCount(int sleep_ms)
{
	sleep_ms = (hal_rtc.valid == true)?((uint32_t)(sleep_ms / hal_rtc.scale)):(sleep_ms);
	return sleep_ms; 
}


//RTC闹钟中断
void RTCAlarm_IRQHandler(void)
{
	RTC_ITConfig(RTC_IT_ALR, DISABLE);
	EXTI_ClearITPendingBit(EXTI_Line17);
	RTC_ClearITPendingBit(RTC_IT_ALR);
	RTC_WaitForLastTask(); 

//	EXTI_ClearITPendingBit(HAL_KEY3_EXTI_LIN);

	/* Check if the Wake-Up flag is set */
	if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
	{
		/* Clear Wake Up flag */
		PWR_ClearFlag(PWR_FLAG_WU);					////一般没用
	}

	if(hal_rtc.valid == false && hal_rtc.start_calibation == true)
	{
		hal_rtc.valid = true;
		hal_rtc.start_calibation = false;
		hal_rtc.scale = (portGetTickCnt() - hal_rtc.systick_clock) / (hal_rtc.calibation_clock *1.0);

		_dbg_printf("portGetTickCnt:%d, scale:%.3f, rtcCount%d\n", 
								portGetTickCnt(),
								hal_rtc.scale,
								RTC_GetCounter());
	}
	/*重新开始*/
	HalPmu_SYSCLKConfig_STOP(2);
}


