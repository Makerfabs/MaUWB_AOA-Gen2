#ifndef __APP_GENERIC_H
#define __APP_GENERIC_H

#ifdef __cplusplus
extern "C"
{
#endif
/*
 *	V3.0.0:发布版本(ST)
 *			一.50Hz防碰撞手环
 *				①增加报警功能
 *				②增加电压功能
 *				③增加充电状态显示功能
 *			二.解决有时候长时间不进入停机模式问题 唤醒关闭TIM4定时器
 *			三.可能存在问题,按键导致RTC唤醒失效
 *			四.解决低功耗唤醒通信异常,增加1ms延时
 *
 *	V3.0.1:
 *			一.低功耗唤醒通信异常，ADC校验ADC_StartCalibration(HAL_ADC1_SN),唤醒时不打开
 *			二.死机情况1[定时器中断优先级0级， DW1000中断优先级1级]
 *			三.死机情况2[增加冗余is_timeout]
 *			四.去掉串口DMA发送
 *			五.去除唤醒时,高速率
 *
 *	V3.0.2:
 *			一.去掉串口DMA发送
 *			二.去除唤醒时,高速率	
 *
 *	V3.0.3:(20230804)
 *			一.发现问题，唤醒后立即读取adc采集，造成中断无法及时响应
 *
 *	V3.0.4:(20230808)
 *			一.发现问题，ADC采集enable
 *			二.增加用户看门狗
 *
 *	V3.0.5:(20230809)
 *			一.增加spi初始化失败处理
 *
 *	V3.0.6:(20230818)
 *			一.增加看门狗
 *
 *	V3.2.3：(20230823)
 *			一、与dw3000软件版本保持一致 
 *
 *	V3.2.4：(20230824)
 *			一、全速工作唤醒和低功耗工作唤醒 时间不匹配修改bug
 *
 *	V3.2.5：(20230824)
 *			一、接受超时冗余150->50
 *
 *	V3.2.6：(20230824)
 *			一、去掉手环开机关所有uwb功能(严重bug开机,无法初始化)
 *
 *	V3.2.7：(20230913)
 *			一、基站A0-基站A3距离缩短问题(app.pConfig->s.baseConfig.antTx + tmp64) & MASK_40BIT;
 *
 *	V3.2.8：(20240515)
 *			一、标签在关机的时候会自启动,原因可能为按键误触发,解决方法在关闭定时器时,延迟一个毫秒
 *			二、关机时,需要加延迟，否者可能dw1000关机失败
 *
 *	V3.2.9：(20240528)
 *			一、标签dw1000休眠后,关机又进入dwt_deepsleep函数,是否CS唤醒,所以做了判断
 *			二、标签休眠结束后.没有开启功放
 *			三、喂狗时间改为25秒,进入休眠时切记要先喂狗
 *
 *	V3.3.0：(20240529)
 *			一、低功耗rtc唤醒时间10秒-》15秒(有些基站大于25秒就是不行,不明原因,但是关闭看门狗,他确实25秒中断一次)
 *			二、goto deepSleep改为d减少唤醒时长
 *
 *	V3.3.1：(20240601)
 *			一、唤醒后,按键处在异常状态,导致一直进入休眠失败(严重bug,导致耗电剧增).
 *			二、将唤醒的处理放在终端中执行App_Moudle_Device_TurnOn();
 *
 *	V3.3.2：(20240602)
 *			一、调用了HalTimer4_IT_Enable,但定时器未开启,导致按键失效
 *			二、在滴答时钟中,确认定时器是否启动,如果未启动,则再次调用启动
 *
 *	V3.3.3：(20240812)
 *			一、开启PA功能,通信正常,开启LNA功能,通信失效(原因不详)
 *			二、默认开启PA功能
 *			三、如果编译等级设置为0,发现功耗变大一半,设置成3则正常功耗
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
#ifdef HW_TAG_3_LED
	#define uwb_software_ver "v03_03_003(3Led)"
#else
	#define uwb_software_ver "v03_02_008(2Led)"
#endif
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
} System_State_t;

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

