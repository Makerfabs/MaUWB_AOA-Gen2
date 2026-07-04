#ifndef __APP_GENERIC_H
#define __APP_GENERIC_H

#ifdef __cplusplus
extern "C"
{
#endif
/*
 *	V2.0.0：(20250114)
 *			一、遥控发布版本
 *			二、未完成按键功能
 *			三、按键不够稳定,配合U1会出现hardfault错误
 *
 *	V2.0.1：(20250117)
 *			一、遥控发布版本
 *			二、未完成按键功能
 *			三、按键不够稳定,配合U1会出现hardfault错误
 */
 
/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include <stdbool.h>

#include "Generic_CMD.h"
#include "OSAL_Comdef.h"

#include "default_config.h"
#include "hal_led.h"


	
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/
#define IS_FISRT_POEWRON(x) ((x) != 0xAAAA)

//softword ver
#define uwb_software_ver "v02_00_000"
#define uwb_hardware_ver "v01_00_000"
/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

typedef enum{
	Sys_Operate_Mode_INVAILD			= 0x0000,
	/*未接电源*/
	Sys_Operate_Mode_LowPower			= 0x0001,		//低功耗
	Sys_Operate_Mode_USB_NON_CONNET 	= 0x0002, 		//未连接USB充电线

	/*接电源*/
	Sys_Operate_Mode_USB_CONNET			= 0x0004, 		//连接USB，正在充电
	Sys_Operate_Mode_USB_Battery_FULL 	= 0x0008		//电池电量充足
}sys_work_mode_e;	

typedef enum 
{
    Sys_DoneState_Work = 0,								//(开机)
    Sys_DoneState_Stop = 1,								//(开机)外部中断|加速度|RTC时钟可唤醒
    Sys_DoneState_DeepSleep ,							//(关机)深度睡眠
}Sys_Done_State;
typedef struct
{
	uint32_t flag;
	uint32_t start_count;

	uint32_t HardFault_error_bit;
	uint32_t MemManage_error_bit;
	uint32_t BusFault_error_bit;
	uint32_t UsageFault_error_bit;
	
	param_block_t param_Config;
	
	uint32_t flash_error_count;
	uint32_t check_sum;							//校验和:用于flash验证
} System_Para_t; 

typedef struct
{
	struct {							//系统报警
		uint16_t alarm;					//系统报警-开启
		uint32_t alarm_timer;			//系统报警-剩余报警时长
	}system_alarm;

	sys_work_mode_e sys_work_mode;		//系统模式(无效/配置中/工作中) 
	bool           system_turnoff;		//系统关机变量
	Sys_Done_State system_DoneState;	//系统当前状态 (工作/睡眠/深度睡眠)
	uint32_t       system_turnoff_stime;//系统关机开始时间

	struct{
		tag_detail_para_t tag_detail_para;
		uint16_t tag_acc_x;
		uint16_t tag_acc_y;
		uint16_t tag_acc_z;
		uint16_t tag_gcc_x;
		uint16_t tag_gcc_y;
		uint16_t tag_gcc_z;
	}para_tag;							//标签上报参数

	struct{
		int  G_count;					//加速度传感器参数
		bool quiet;
	}acc;
}System_State_t;

typedef struct
{
	uint8_t uart_dma_tx[1024];
	uint8_t uart_dma_tx_tmp[1024];
	int uart_dma_report;
	int uart_dma_index;
}System_uart_dma_t;




/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/
extern System_Para_t sys_para;
extern System_State_t sys_state;
extern System_uart_dma_t sys_uart_dma_buf;
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
extern void App_Module_Init(void);

/********非易失性数据(读/写)操作***********/
extern void App_Module_Sys_Write_NVM(void);
extern void App_Module_Sys_Read_NVM(void);


/********串口/LED/按键/充电/电量事件处理***********/
extern void App_Module_Sys_Work_Mode_Event(void);
extern void App_Module_Sys_Deal_IO_LED_Event(void);
static void App_Module_Sys_Deal_UART_CMD_Event(void);
static void App_Module_Sys_Deal_IO_POWER_EVENT(void);
static void App_Modelu_Sys_Deal_IO_KEY_Event();
static void App_Module_Sys_Deal_SPI_LIS3DH_Event(void);
static void App_Module_Display_SysState_Para(void);


extern void App_Moudle_Device_SetMode(int key_sn);
extern void App_Moudle_Device_TurnOn(void);
extern void App_Moudle_Device_TurnOff(void);

extern void App_Module_Set_SysState_Mode_Para(sys_work_mode_e sys_work_mode);
extern uint16_t App_Module_Get_SysState_Mode(void);
extern void App_Moudle_Device_SetAlarm(void);
extern bool App_Module_Get_SysState_Alarm(void);


extern tag_detail_para_t App_Module_Get_SysState_Sensor(void);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif//__APP_GENERIC_H

