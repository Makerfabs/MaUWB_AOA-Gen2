#include "hal_pmu.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_spi.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_usart.h"
#include "hal_rtc.h"
#include "OSAL_Comdef.h"

#include "ose_uwb.h"

bool       is_sleep   = false;
enterPmu_e enterPmu   = enterPmu_ivalid;			//关机状态


void HalPmu_LowpowerCfg(enterPmu_e enterPmu)
{
	// 停止滴答时钟
	SysTick->CTRL  &= ~(SysTick_CTRL_CLKSOURCE_Msk | 
				   SysTick_CTRL_TICKINT_Msk   | 
				   SysTick_CTRL_ENABLE_Msk);

	//清所有的外部中断请求位和RTC闹钟标志位，否则停止模式进入流程会被跳过，程序继续运行
	EXTI->PR = 0x00;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);	//JTAG disable
	GPIO_InitTypeDef GPIO_InitStructure; 

	if(enterPmu == enterPmu_turnOff)
	{
		GPIO_InitStructure.GPIO_Pin = ~((HAL_KEY6_Remote_PIN | HAL_KEY7_Follow_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOA, &GPIO_InitStructure);


		GPIO_InitStructure.GPIO_Pin = ~((OSE_EN_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
	else if(enterPmu == enterPmu_deepSleep)
	{
		GPIO_InitStructure.GPIO_Pin = ~((HAL_KEY6_Remote_PIN | HAL_KEY7_Follow_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOA, &GPIO_InitStructure);


		GPIO_InitStructure.GPIO_Pin = ~((OSE_EN_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
	else if(enterPmu == enterPmu_Sleep)
	{
		GPIO_InitStructure.GPIO_Pin = ~((OSE_RST_PIN | HAL_KEY6_Remote_PIN | HAL_KEY7_Follow_PIN | HAL_LED1_PIN | HAL_LED2_PIN | HAL_LED3_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOA, &GPIO_InitStructure);


		GPIO_InitStructure.GPIO_Pin = ~((OSE_EN_PIN | OSE_IRQ_PIN | OSE_READY_PIN)/*|
										(is_acc_iqr_enable?LIS3DIRQ:0)*/);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}

//	if(is_acc_iqr_enable == true){HalSPI1_Lis3dh_IT_Enable();}else{HalSPI1_Lis3dh_IT_Disable();}
	
	ADC_Cmd(ADC1,DISABLE);
	
	SPI_I2S_DeInit(SPI1);

	SPI_Cmd(SPI1, DISABLE);

	is_sleep = true;
}


int HalPmuInit(void)
{
	/* 使能电源管理单元的时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
}

/**
 * @brief 进入停机模式(耗时0-1毫秒)
 * @param delay 休眠时间
 * @param is_acc_iqr_enable 是否启动加速度唤醒
 */
void HalPmu_Enter(int delay , bool is_acc_iqr_enable)
{
	enterPmu = (delay == 0xffff && is_acc_iqr_enable == false)?(enterPmu_turnOff):( (delay == 0xffff)?(enterPmu_deepSleep):(enterPmu_Sleep) );

	if(enterPmu == enterPmu_turnOff || enterPmu == enterPmu_deepSleep)
	{
		_dbg_printf("D\n");
		RTCAlarm_Set(15000);
	}
	else
	{
		RTCAlarm_Set(delay);
	}



	HalPmu_LowpowerCfg(enterPmu);
	/* 进入停止模式，设置电压调节器为低功耗模式，等待中断唤醒*/
	PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI);
}

/**
  * @brief  停机唤醒后配置系统时钟: 使能 HSE, PLL
  *         并且选择PLL作为系统时钟.
  * @param  None
  * @retval None
  */
void HalPmu_SYSCLKConfig_STOP(uint8_t val)
{
	uint8_t tmp_is_sleep = is_sleep;
	if(is_sleep == true)
	{
		SystemInit();
		RCC_Configuration_part();
		HalTimerInit();
		HalLedInit();
		HalUARTInit();
		// 开机状态初始化
		if(enterPmu != enterPmu_turnOff)
		{
			HalAdcInit();
			HalSpiInit(false);
		}
		HalKeyInit();
		is_sleep = false;
	}
//	_dbg_printf("被唤醒方式:%d, is_sleep:%d\n",val, tmp_is_sleep);
}

/***************************************************************************************************
***************************************************************************************************/
