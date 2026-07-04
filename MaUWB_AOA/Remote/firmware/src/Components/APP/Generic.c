#include <string.h>
#include <stdio.h>
#include <math.h>

#include "Generic.h"
#include "cmd.h"

#include "hal_timer.h"
#include "hal_adc.h"
#include "hal_usart.h"
#include "hal_key.h"

System_State_t sys_state = {
	.sys_work_mode = Sys_Operate_Mode_LowPower,
	
	.tag_detail_para.mode = 1,
};

/*
 * 功能:获取系统状态
 * 形参：系统状态指针
 * 返参：	1)Sys_Operate_Mode_LowPower
 *			2)Sys_Operate_Mode_USB_NON_CONNET
 *			3)Sys_Operate_Mode_USB_CONNET
 *			4)Sys_Operate_Mode_USB_Battery_FULL
 *
 * */
void App_Module_Sys_Deal_IO_POWER_EVENT(void)
{
	static uint32_t tmp_loop_timer = 0;
	
	if(!IS_TIMEOUT(portGetTickCnt(), tmp_loop_timer, 1000))
	{
		return;
	}

	tmp_loop_timer = portGetTickCnt();

	static int battery_full_index = 0;
	/*
	 *
	 *	正在充电/未完成：		CHRG=0	TDBY=1
	 *  正在充电/完成:			CHRG=1	TDBY=0
	 *	未充电：  				CHRG=1	TDBY=1
	 *
	 */
	uint8_t CHRG = GPIO_ReadInputDataBit(HAL_KEY_CHRG_PORT, HAL_KEY_CHRG_PIN);
	uint8_t TDBY = GPIO_ReadInputDataBit(HAL_KEY_PGOOD_PORT, HAL_KEY_PGOOD_PIN);
	float vol;
	
	uint8_t battery_state = HalAdcGetState(&vol);
	
	if(CHRG == 1 && TDBY == 1)
	{
		sys_state.sys_work_mode = (battery_state ==  Hal_Battery_State_Low)?(Sys_Operate_Mode_LowPower):(Sys_Operate_Mode_USB_NON_CONNET);
	}
	else if(CHRG == 0 && TDBY == 1)
	{
		sys_state.sys_work_mode = (Sys_Operate_Mode_USB_CONNET);
	}
	else if(CHRG == 1 && TDBY == 0)
	{
		sys_state.sys_work_mode = (Sys_Operate_Mode_USB_Battery_FULL);
	}
	else
	{
		sys_state.sys_work_mode = (Sys_Operate_Mode_USB_NON_CONNET);
	}
	
	sys_state.tag_detail_para.is_chrg = CHRG;
	sys_state.tag_detail_para.is_tdby = TDBY;
	sys_state.tag_detail_para.battery_val = (uint16_t)(vol * 100.0);
	sys_state.tag_detail_para.is_lowbattery = (battery_state == Hal_Battery_State_Low);

#if 0
	SYS_LOG("CHRG:%d, TDBY:%d , 电池:%.2lf\n",CHRG, TDBY, vol);
#endif
}


/*******************************************************************************
* 函数名  : App_Module_Init
* 描述    : 初始化函数
* 输入    : 无
* 输出    : 无
* 返回值  : int:返回值为一个16位整形数
* 注意    : 无
*******************************************************************************/
void App_Module_Init(void)
{
	/*开机状态*/
	for(int i = 0; i < 3; i++)
	{
		HalLedSet(HAL_LED1, HAL_LED_MODE_ON);
		HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_ON);
		HalDelay_nMs(200);
		HalLedSet(HAL_LED1, HAL_LED_MODE_OFF);
		HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_OFF);
		HalDelay_nMs(200);
	}
	
	_dbg_printf("JL system Init\n");
}



void App_Module_Sys_Deal_IO_LED_Event(void)
{
	/*
	 * 黄灯:未绑定(常亮)
	 * 红灯:低电量[遥控模式:(慢闪2s)/跟随模式:[快闪0.5s])
	 * 绿灯:正常点亮[遥控模式:(慢闪2s)/跟随模式:[快闪0.5s])
	 * 蓝灯:充电[遥控模式:(慢闪2s)/跟随模式:[快闪0.5s]),充满(常亮)
	 */

	static uint32_t tmp_loop_timer = 0;
	static bool state  = false;

	int  mode = sys_state.sys_work_mode;
	int  period = (sys_state.tag_detail_para.mode == 0)?(200):(1000);


	if(!IS_TIMEOUT(portGetTickCnt(), tmp_loop_timer, period))
	{
		return;
	}

	tmp_loop_timer = portGetTickCnt();

	// 超过2秒未接受到定位帧,则认为未绑定
	bool is_bind = IS_TIMEOUT(portGetTickCnt(), sys_state.last_recvUwbTime, 2000)?(false):(true);

	state = (state == true)?(false):(true);

	HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);


	//选择LED灯闪烁
	if(mode == Sys_Operate_Mode_USB_NON_CONNET)			//绿灯闪烁
	{
		if(is_bind == true)
		{
			HalLedSet(HAL_LED1, (state == true)?(HAL_LED_MODE_ON):(HAL_LED_MODE_OFF));
		}
		else
		{
			HalLedSet(HAL_LED1, HAL_LED_MODE_ON);
		}
	}
	else if(mode == Sys_Operate_Mode_LowPower)			//红灯闪烁
	{
		HalLedSet(HAL_LED2, (state == true)?(HAL_LED_MODE_ON):(HAL_LED_MODE_OFF));
	}
	else if(mode == Sys_Operate_Mode_USB_CONNET)		//蓝灯闪烁
	{
		HalLedSet(HAL_LED3, (state == true)?(HAL_LED_MODE_ON):(HAL_LED_MODE_OFF));
	}
	else if(mode == Sys_Operate_Mode_USB_Battery_FULL)	//蓝灯常亮
	{
		HalLedSet(HAL_LED3, HAL_LED_MODE_ON);
	}
}




/*
 * 功能:按键处理
 *
 * */
void App_Modelu_Sys_Deal_IO_KEY_Event(void)
{
	sys_state.tag_detail_para.turn_up = (GPIO_ReadInputDataBit(HAL_KEY1_Up_PORT, HAL_KEY1_Up_PIN) == 1)?(false):(true) ;
	sys_state.tag_detail_para.turn_left = (GPIO_ReadInputDataBit(HAL_KEY2_Left_PORT, HAL_KEY2_Left_PIN) == 1)?(false):(true) ;
	sys_state.tag_detail_para.turn_right = (GPIO_ReadInputDataBit(HAL_KEY4_Right_PORT, HAL_KEY4_Right_PIN) == 1)?(false):(true) ;
	sys_state.tag_detail_para.turn_down = (GPIO_ReadInputDataBit(HAL_KEY5_Down_PORT, HAL_KEY5_Down_PIN) == 1)?(false):(true) ;
}

/*
 * 功能:双击报警
 *
 * */
void App_Moudle_Device_SetAlarm(void)
{
	if(sys_state.tag_detail_para.is_alarm == true) {
		sys_state.tag_detail_para.is_alarm = false;
	}
	else {
		sys_state.tag_detail_para.is_alarm = true;
	}
}



void App_Moudle_Device_SetMode(int key_sn)
{
	// 遥控器(KEY6)
	sys_state.tag_detail_para.mode = (key_sn == HAL_KEY6)?(1):(0);
//	SYS_LOG("Mode:%d\n", sys_state.tag_detail_para.mode);
}



void SYS_task(void)
{
	HalIWDG_Feed();
	App_Module_Sys_Deal_IO_POWER_EVENT();
	App_Module_Sys_Deal_IO_LED_Event();
	App_Modelu_Sys_Deal_IO_KEY_Event();
}

tag_detail_para_t App_Module_Get_SysState_Sensor(void)
{
	tag_detail_para_t sensor;
	memset(&sensor, 0, sizeof(tag_detail_para_t));

	sensor.is_lowbattery = sys_state.tag_detail_para.is_lowbattery;	//是否低电量报警
	sensor.is_alarm      = sys_state.tag_detail_para.is_alarm;		//是否报警
	sensor.is_chrg       = sys_state.tag_detail_para.is_chrg;		//是否充电
	sensor.is_tdby       = sys_state.tag_detail_para.is_tdby;		//是否充满
	sensor.battery_val   = sys_state.tag_detail_para.battery_val;

	sensor.turn_up       = sys_state.tag_detail_para.turn_up;		//(遥控专用)向前
	sensor.turn_down     = sys_state.tag_detail_para.turn_down;		//(遥控专用)向后
	sensor.turn_left     = sys_state.tag_detail_para.turn_left;		//(遥控专用)向左
	sensor.turn_right    = sys_state.tag_detail_para.turn_right;	//(遥控专用)向右
	sensor.mode          = sys_state.tag_detail_para.mode;			//(遥控专用)模式
	sensor.recal         = sys_state.tag_detail_para.recal;			//(遥控专用)召回
	sensor.lock          = sys_state.tag_detail_para.lock;			//(遥控专用)上锁
	
	sensor.dev_type      = 0;		//
	sensor.reserve       = 0;		//预留
	
	return sensor;
}

