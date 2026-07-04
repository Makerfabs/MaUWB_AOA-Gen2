#include <string.h>
#include <stdio.h>
#include <math.h>

#include "Generic.h"
#include "cmd.h"

#include "ose_uwb.h"
#include "tag.h"

#include "hal_timer.h"
#include "hal_flash.h"
#include "hal_spi.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "default_config.h"

param_block_t defaultFConfig  = DEFAULT_CONFIG;


System_uart_dma_t sys_uart_dma_buf ={
	.uart_dma_tx = 0,
	.uart_dma_report = true,
	.uart_dma_index = 0
};


//全局结构体
System_Para_t sys_para = {
	.start_count = 0x0000,
	.param_Config = DEFAULT_CONFIG
};

//备份全局结构体
System_Para_t sys_para_bak = {
	.start_count = 0x0000,
	.param_Config = DEFAULT_CONFIG
};


System_State_t sys_state = {
	.system_alarm.alarm = false,
	.system_alarm.alarm_timer = 0,

	.sys_work_mode = Sys_Operate_Mode_LowPower,
	.system_turnoff = false,
	.system_DoneState = Sys_DoneState_Work,

	.acc.quiet   = false,
};


/*
 * 功能:系统启动打印参数
 *
 * */
void App_Module_sys_para_debug(void)
{
	_dbg_printf("*******************PDoA 系统参数*******************\r\n");

	_dbg_printf("   UWB Module 编译时间 %s %s\n",__TIME__,__DATE__);

	_dbg_printf("   UWB Module 软件版本号：%s\r\n"  ,uwb_software_ver);
	_dbg_printf("   UWB Module 硬件版本号： %s\r\n"  ,uwb_hardware_ver);

	_dbg_printf("   UWB Module 标志位：%02X\r\n"    ,sys_para.flag);
	_dbg_printf("   UWB Module 开机次数：%d\r\n"  ,sys_para.start_count);
	_dbg_printf("   UWB Module 错误位1:%d, 错误位2:%d, 错误位3:%d, 错误位4:%d, 错误位5:%d\r\n",
							sys_para.HardFault_error_bit,
							sys_para.MemManage_error_bit,
							sys_para.BusFault_error_bit,
							sys_para.UsageFault_error_bit,
							sys_para.flash_error_count);
	_dbg_printf("***********************************************\r\n");

	_dbg_printf("\r\n");
}


/*
 *	校验累加和
 *
 */
uint32_t Check_Sum(uint32_t *Buf, uint8_t len)
{
	uint8_t i =0;
	uint32_t sum =0;
	uint32_t checksum =0;

	for(i=0; i<len; i++)
	{
		sum += *Buf++;
	}
	checksum = sum &0xffffffff;
	
	return checksum;
}

/*
 *	设备全局变量初始化
 *
 */
void App_Module_sys_para_Init(void)
{
	memset(&sys_para.flag, 0, sizeof(sys_para));
	sys_para.flag = 0xAAAA;
	sys_para.start_count = 0;

	sys_para.HardFault_error_bit	= 0;
	sys_para.MemManage_error_bit = 0 ;
	sys_para.BusFault_error_bit = 0;
	sys_para.UsageFault_error_bit = 0;
	sys_para.flash_error_count = 0;
	
	sys_para.param_Config = defaultFConfig;
}

/*
 *	应用层 写入非易失性数据(备份存储区)
 *
 */
void App_Module_Sys_Write_NVMBAK(void)
{
	uint32_t read_check_sum = 0;
	System_Para_t tmp_sys_para;
	sys_para.check_sum = Check_Sum(&sys_para.flag, sizeof(sys_para)/4 - 1);			//计算校验
	
	//写入备份存储区
	while(HalWrite_Flash(PAGE127_ADDR, &sys_para.flag, sizeof(sys_para)/4) == 0)
	{
		HalDelay_nMs(100);
	}
	
	//读取备份存储区
	HalRead_Flash(PAGE127_ADDR, &tmp_sys_para.flag, sizeof(sys_para)/4);
	read_check_sum = Check_Sum(&tmp_sys_para.flag, sizeof(sys_para)/4 - 1);;
	
	if(memcmp(&tmp_sys_para.flag, &sys_para.flag, sizeof(sys_para)) == 0 &&
		tmp_sys_para.check_sum == read_check_sum)
	{
//		_dbg_printf("备份存储区写入成功\n");
	}
}
  
/*
 *	应用层 写入非易失性数据(主存储区)
 *
 */
void App_Module_Sys_Write_NVM(void)
{
	uint32_t read_check_sum = 0;
	System_Para_t tmp_sys_para;
	sys_para.check_sum = Check_Sum(&sys_para.flag, sizeof(sys_para)/4 - 1);			//计算校验

	//写入主存储区
	while(HalWrite_Flash(PAGE126_ADDR, &sys_para.flag, sizeof(sys_para)/4) == 0)
	{
		HalDelay_nMs(100);
	}

	//读取主存储区
	HalRead_Flash(PAGE126_ADDR, &tmp_sys_para.flag, sizeof(sys_para)/4);
	read_check_sum = Check_Sum(&tmp_sys_para.flag, sizeof(sys_para)/4 - 1);;
	
	if(memcmp(&tmp_sys_para.flag, &sys_para.flag, sizeof(sys_para)) == 0 &&
		tmp_sys_para.check_sum == read_check_sum)
	{
//		_dbg_printf("主存储区写入成功,再写入备份存储区\n");
		App_Module_Sys_Write_NVMBAK();
	}	
}


/*
 *	应用层 读取非易失性数据
 *
 */
void App_Module_Sys_Read_NVM(void)
{
	HalRead_Flash(PAGE126_ADDR, &sys_para.flag, sizeof(sys_para)/4);     //主存储区读取
	HalRead_Flash(PAGE127_ADDR, &sys_para_bak.flag, sizeof(sys_para)/4); //备份存储区读取
}

/*
 *	读取flash中配置数据
 *
 */
void App_Module_sys_para_read()
{
	bool is_back2factory = false;

	App_Module_Sys_Read_NVM();
	
	
	if(sys_para.flag != 0xAAAA && sys_para_bak.flag != 0xAAAA
		&& sys_para.flag == sys_para_bak.flag)
	{
		is_back2factory = true;     
		_dbg_printf("设备首次上电\n");
	}
	else
	{
		uint32_t read_check_sum, read_check_sum_bak;
		read_check_sum = Check_Sum(&sys_para.flag, sizeof(sys_para)/4 - 1);
		read_check_sum_bak = Check_Sum(&sys_para_bak.flag, sizeof(sys_para)/4 - 1);

		sys_para.start_count +=1;    
		sys_para_bak.start_count +=1;   

		if(read_check_sum == sys_para.check_sum && read_check_sum_bak == sys_para_bak.check_sum
			&& memcmp(&sys_para.flag, &sys_para_bak.flag, sizeof(sys_para)) == 0)
		{
			_dbg_printf("Master/Slaver flash normal\n");
			App_Module_Sys_Write_NVM();
		}
		else if(read_check_sum == sys_para.check_sum)
		{
			_dbg_printf("Master flash normal,Slaver flash loss\n");
			sys_para.flash_error_count++;
			App_Module_Sys_Write_NVM();
		}
		else if(read_check_sum_bak == sys_para_bak.check_sum)
		{
			_dbg_printf("Master flash loss,Slaver normal\n");
			sys_para = sys_para_bak;
			sys_para.flash_error_count++;
			App_Module_Sys_Write_NVM();
		}
		else
		{
			is_back2factory = true;     
			_dbg_printf("Master/Slaver flash loss\n");
		}		
		_dbg_printf("ReadCheck MasterCheck:%08X SlaverCheck:%08X\n", read_check_sum, read_check_sum_bak);
	}
	
	/*恢复出厂模式*/
	if(is_back2factory == true)
	{
		App_Module_sys_para_Init();
		App_Module_Sys_Write_NVM();
		NVIC_SystemReset();      
    }
}

void App_Module_sys_startup_check()
{
	// RTC时钟校准
	HalRTC_Calibation(1000);
	
	/*开机电压检测*/
	HalKey_IT_Disable();
	for(;;)
	{
		HalIWDG_Feed();
		uint8_t CHRG = GPIO_ReadInputDataBit(HAL_KEY1_PORT, HAL_KEY1_PIN);		//充电IO口
		float battery = (float)HalAdcValueGet(HAL_ADC1CH) / 4096 * 3.3 * 2;		//电池电压
		_dbg_printf("CHRG:%d, battery low:%.2f\n", CHRG, battery);

		if(CHRG == 0 && battery < HAL_LOW_BATTERY)								//正在充电中
		{
			App_Module_Sys_Deal_UART_CMD_Event();
			HalLedSet(HAL_LED1, HAL_LED_MODE_ON);
			HalLedSet(HAL_LED3, HAL_LED_MODE_ON);
			HalDelay_nMs(200);
			HalLedSet(HAL_LED1, HAL_LED_MODE_OFF);
			HalLedSet(HAL_LED3, HAL_LED_MODE_OFF);
			HalDelay_nMs(200);
			continue;
		}
		
		break;
	}
	HalKey_IT_Enable();
}


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
	bool is_get =false;
	if(tmp_loop_timer + HalRTC_GetCount(2000) <= RTC_GetCounter())
	{
		tmp_loop_timer = RTC_GetCounter();
		is_get = true;
	}

	if(is_get == false)
	{
		return;
	}    

	static int battery_full_index = 0;
	/*
	 *
	 *	正在充电/未完成：		CHRG=0	TDBY=1
	 *  正在充电/完成:			CHRG=1	TDBY=0
	 *	未充电：  				CHRG=1	TDBY=1
	 *
	 */
	uint8_t CHRG = GPIO_ReadInputDataBit(HAL_KEY1_PORT, HAL_KEY1_PIN);
	uint8_t TDBY = GPIO_ReadInputDataBit(HAL_KEY2_PORT, HAL_KEY2_PIN);
	float vol;
	
	uint8_t battery_state = HalAdcGetState(&vol, sys_para.param_Config.s.userConfig.device_alarmbattery);
	
	if(CHRG == 1 && TDBY == 1)
	{
		if(battery_state ==  Hal_Battery_State_Low)
		{
			 sys_state.sys_work_mode = (Sys_Operate_Mode_LowPower);
		}
		else
		{
			sys_state.sys_work_mode = (Sys_Operate_Mode_USB_NON_CONNET);
		}
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
		sys_state.sys_work_mode = Sys_Operate_Mode_USB_NON_CONNET;
	}
	
	sys_state.para_tag.tag_detail_para.is_chrg = CHRG;
	sys_state.para_tag.tag_detail_para.is_tdby = TDBY;
	sys_state.para_tag.tag_detail_para.battery_val = (uint16_t)(vol * 100.0);
	if(battery_state == Hal_Battery_State_Low)
		sys_state.para_tag.tag_detail_para.is_lowbattery = 1;
	else
		sys_state.para_tag.tag_detail_para.is_lowbattery = 0;
    
#if 0
	_dbg_printf("CHRG:%d, TDBY:%d , 电池:%.2lf\n",CHRG, TDBY, vol);
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
	App_Module_CMD_Queue_Init();

	HalDelay_nMs(500);

	App_Module_sys_para_read();

	App_Module_sys_para_debug();

	App_Module_sys_startup_check();
	
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
}



void App_Module_Sys_Deal_IO_LED_Event(void)
{
	static uint32_t tmp_loop_timer = 0;
	bool is_get =false;
	
	twr_info_t  *pTwrInfo = getTwrInfoPtr();
	int  mode = App_Module_Get_SysState_Mode();
	int  period = (pTwrInfo->twr_tag.general.is_bind == true)?(200):(1000);

	if(tmp_loop_timer + HalRTC_GetCount(period) <= RTC_GetCounter())
	{
		tmp_loop_timer = RTC_GetCounter();
		is_get = true;
	}

	if(is_get == false)
	{
		return;
	}


	//选择LED灯闪烁
	if(mode == Sys_Operate_Mode_USB_NON_CONNET)			//绿灯闪烁
	{
		HalLedSet(HAL_LED1, HAL_LED_MODE_ON);
		Sleep_us(500);
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);  
	}
	else if(mode == Sys_Operate_Mode_LowPower)			//红灯闪烁
	{
		HalLedSet(HAL_LED2, HAL_LED_MODE_ON);
		Sleep_us(500);
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);  
	}
#ifdef HW_TAG_3_LED
	else if(mode == Sys_Operate_Mode_USB_CONNET)		//蓝灯闪烁
	{
		HalLedSet(HAL_LED3, HAL_LED_MODE_ON);
		Sleep_us(500);  
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
	}
	else if(mode == Sys_Operate_Mode_USB_Battery_FULL)	//蓝灯常亮
	{
		HalLedSet(HAL_LED3, HAL_LED_MODE_ON);
		Sleep_us(500);
	}
#else
	else if(mode == Sys_Operate_Mode_USB_CONNET)		//蓝灯闪烁
	{
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
		Sleep_us(500);  
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
	}
	else if(mode == Sys_Operate_Mode_USB_Battery_FULL)	//蓝灯常亮
	{
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
		Sleep_us(500);
	}
#endif
}


/*
 * 功能:串口数据接受处理
 *
 * */
void App_Module_Sys_Deal_UART_CMD_Event(void)
{
	App_Module_Process_USART_CMD();
	
	//如果缓存区仍有数据未发送，则发送
	if(sys_uart_dma_buf.uart_dma_index != 0)
	{
		port_tx_msg(NULL, 0);
	}
}


/*
 * 功能:按键处理
 *
 * */
void App_Modelu_Sys_Deal_IO_KEY_Event(void)
{
	// 按键长按 && 按键松开 && 非睡眠模式
	if(sys_state.system_turnoff == true && HalKey_Loose_Judge() && sys_state.system_DoneState != Sys_DoneState_DeepSleep)
	{
		sys_state.system_DoneState = Sys_DoneState_DeepSleep;
		twr_info_t *pTwrInfo = getTwrInfoPtr();
		if(pTwrInfo->twr_tag.general.dw_sleep == false)
		{
//			_dbg_printf("deviceID:%04X\n  ",dwt_readdevid());
//			dwt_entersleep();
			HalDelay_nMs(10);
		}

		while(true)
		{
			if(HalKey_Loose_Judge())
			{
				HalPmu_Enter(0xffff, false, false);		//10秒唤醒喂狗
			}
		}
	}
}





void App_Module_Display_SysState_Para(void)
{
	static unsigned tmp_loop_timer = 0;
	bool is_get =false;
	if(tmp_loop_timer + HalRTC_GetCount(5000) <= RTC_GetCounter())
	{
		tmp_loop_timer = RTC_GetCounter();
		is_get = true;
	}

	if(is_get == false)
	{
		return;
	}

	uint8_t buf[128];
	sprintf(buf, "sysstate Xg:%d,Yg:%d,Zg:%d G_count:%d CHRG:%d, TDBY:%d, battery:%d, alarm:%d, sys_work_mode:%d\n", 
			sys_state.para_tag.tag_acc_x,
			sys_state.para_tag.tag_acc_y,
			sys_state.para_tag.tag_acc_z,
			sys_state.acc.G_count,
			sys_state.para_tag.tag_detail_para.is_chrg,
			sys_state.para_tag.tag_detail_para.is_tdby,
			sys_state.para_tag.tag_detail_para.battery_val,
			sys_state.para_tag.tag_detail_para.is_alarm,
			sys_state.sys_work_mode
			);

	_dbg_printf("%s", buf);
}


/*
 * 功能:单击开机
 *
 * */
void App_Moudle_Device_TurnOn(void)
{
	if(sys_state.system_turnoff == true)
	{
		HalDelay_nMs(1);
		NVIC_SystemReset();
	}
}


/*
 * 功能:关机
 *
 * */
void App_Moudle_Device_TurnOff(void)
{
	{
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
		HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_ON);
		HalDelay_nMs(1000);
		HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
		HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_OFF);
	}
	sys_state.system_turnoff = true;
	sys_state.system_turnoff_stime = portGetTickCnt();
}

/*
 * 功能:双击报警
 *
 * */
void App_Moudle_Device_SetAlarm(void)
{
	//开机状态
	if(sys_state.system_DoneState != Sys_DoneState_DeepSleep)
	{
		HalBeepSet(HAL_BEEP1, HAL_BEEP_MODE_ON);            

		sys_state.para_tag.tag_detail_para.is_alarm = true;
		sys_state.system_alarm.alarm = true;
		sys_state.system_alarm.alarm_timer = 10000;
	}
}


bool App_Module_Get_SysState_Alarm(void)
{
	return sys_state.system_alarm.alarm;
}


bool App_Module_Get_SysState_Quiet(void)
{
	return sys_state.acc.quiet;
}



void App_Module_Set_SysState_Mode_Para(sys_work_mode_e sys_work_mode)
{
	sys_state.sys_work_mode = sys_work_mode;
}

uint16_t App_Module_Get_SysState_Mode(void)
{
	return (uint16_t)(sys_state.sys_work_mode);
}


void App_Module_Sys_Work_Mode_Event(void)
{
	HalIWDG_Feed();
	
	App_Module_Sys_Deal_UART_CMD_Event();
	App_Module_Sys_Deal_IO_POWER_EVENT();
	App_Module_Sys_Deal_IO_LED_Event();
	App_Modelu_Sys_Deal_IO_KEY_Event();

	App_Module_Display_SysState_Para();
}

tag_detail_para_t App_Module_Get_SysState_Sensor(void)
{
	tag_detail_para_t sensor;
	memset(&sensor, 0, sizeof(tag_detail_para_t));

	sensor.is_lowbattery = sys_state.para_tag.tag_detail_para.is_lowbattery;	//是否低电量报警
	sensor.is_alarm      = sys_state.para_tag.tag_detail_para.is_alarm;			//是否报警
	sensor.is_chrg       = sys_state.para_tag.tag_detail_para.is_chrg;			//是否充电
	sensor.is_tdby       = sys_state.para_tag.tag_detail_para.is_tdby;			//是否充满
	sensor.battery_val   = sys_state.para_tag.tag_detail_para.battery_val;

	sensor.turn_up       = 0 ;						//(遥控专用)向前
	sensor.turn_down     = 0;						//(遥控专用)向后
	sensor.turn_left     = 0;						//(遥控专用)向左
	sensor.turn_right    = 0;			    		//(遥控专用)向右
	sensor.mode          = 0;						//(遥控专用)模式
	sensor.recal         = 0;						//(遥控专用)召回
	sensor.lock          = 0;						//(遥控专用)上锁
	
	sensor.dev_type      = DEFAULT_DEVICE_TYPE;		//设备类型(0:学习板 1:手环 2:遥控器)
	sensor.reserve       = 0;						//预留
	
	return sensor;
}

