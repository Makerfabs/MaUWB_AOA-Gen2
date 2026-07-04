#ifndef __APP_GENERIC_H
#define __APP_GENERIC_H

#ifdef __cplusplus
extern "C"
{
#endif
/*
 *	V1.0.0：(20250623)
 *			一、第一个版本
 *			二、OTA转发有问题
 */
 
/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include <stdbool.h>

#include "Generic_CMD.h"
#include "OSAL_Comdef.h"

#include "hal_led.h"


	
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/
#define IS_FISRT_POEWRON(x) ((x) != 0xAAAA)

//softword ver
#define uwb_software_ver "v01_00_000"
#define uwb_hardware_ver "v01_00_000"
/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

typedef enum{
	Sys_Operate_Mode_INVAILD            = 0x0000,
	/*未接电源*/
	Sys_Operate_Mode_LowPower           = 0x0001,		//低功耗
	Sys_Operate_Mode_USB_NON_CONNET     = 0x0002, 		//未连接USB充电线

	/*接电源*/
	Sys_Operate_Mode_USB_CONNET         = 0x0004, 		//连接USB，正在充电
	Sys_Operate_Mode_USB_Battery_FULL   = 0x0008		//电池电量充足
}sys_work_mode_e;	


typedef struct
{
	uint32_t          last_recvUwbTime;		//超过2秒未接收到数据集,
	
	sys_work_mode_e   sys_work_mode;		//系统模式(无效/配置中/工作中)
	
	tag_detail_para_t tag_detail_para;
}System_State_t;





/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/
extern System_State_t sys_state;
/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
extern void App_Module_Init(void);

/********串口/LED/按键/充电/电量事件处理***********/
extern void SYS_task(void);
extern void App_Module_Sys_Deal_IO_LED_Event(void);
static void App_Module_Sys_Deal_UART_CMD_Event(void);
static void App_Module_Sys_Deal_IO_POWER_EVENT(void);
static void App_Modelu_Sys_Deal_IO_KEY_Event();

extern void App_Moudle_Device_SetMode(int key_sn);

extern tag_detail_para_t App_Module_Get_SysState_Sensor(void);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif
#endif//__APP_GENERIC_H

