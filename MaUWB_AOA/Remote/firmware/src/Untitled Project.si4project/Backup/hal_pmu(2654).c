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

bool is_sleep = false;
bool shutdown = false;			//关机状态

//停止SysTick
void STOP_SysTick(void) 
{
	SysTick->CTRL  &= ~(SysTick_CTRL_CLKSOURCE_Msk | 
				   SysTick_CTRL_TICKINT_Msk   | 
				   SysTick_CTRL_ENABLE_Msk);
}

void HalPmu_LowpowerCfg(bool is_acc_iqr_enable, bool is_uwb_iqr_enable)
{
	STOP_SysTick(); //停止SysTick
	//清所有的外部中断请求位和RTC闹钟标志位，否则停止模式进入流程会被跳过，程序继续运行
	EXTI->PR = 0x00;//清所有的外部中断请求位	

	GPIO_InitTypeDef GPIO_InitStructure; 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);	//JTAG disable

	GPIO_InitStructure.GPIO_Pin = ~((HAL_KEY3_PIN)/*|
									(is_acc_iqr_enable?LIS3DIRQ:0)|
									(is_uwb_iqr_enable?DW1000_IRQ_PIN:0)*/);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	if(is_acc_iqr_enable == true){HalSPI1_Lis3dh_IT_Enable();}else{HalSPI1_Lis3dh_IT_Disable();}
	if(is_uwb_iqr_enable == true){port_EnableEXT_IRQ();}else{port_DisableEXT_IRQ();}
	
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

void HalPmu_Enter(int delay , bool is_acc_iqr_enable, bool is_uwb_iqr_enable)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	if(delay == 0xffff)
	{
		_dbg_printf("D\n");
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC,ENABLE);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		shutdown = true;

		
		RTCAlarm_Set(15000);
//		RTCAlarm_Close();
	}
	else
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

//		GPIO_InitStructure.GPIO_Pin = ~HAL_BEEP_PIN;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;   
//		GPIO_Init(GPIOA, &GPIO_InitStructure);
		RTCAlarm_Set(delay);
	}


	HalPmu_LowpowerCfg(is_acc_iqr_enable, is_uwb_iqr_enable);
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
		if(shutdown == false)
		{
			HalAdcInit();
			HalSpiInit();
		}
		HalKeyInit();
		is_sleep = false;
	}
//	_dbg_printf("被唤醒方式:%d, is_sleep:%d\n",val, tmp_is_sleep);
}

/***************************************************************************************************
***************************************************************************************************/
