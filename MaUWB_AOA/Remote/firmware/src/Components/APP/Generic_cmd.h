#ifndef __GENERIC_AT_H
#define __GENERIC_AT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include "OSAL.h"
#include "struct_def.h"
#include <stdbool.h>

/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/
#define CMD_SOP                 (0x2A)
#define CMD_FOOT                (0x23)

#define SOP_STATE               (0x00)
#define LEN_STATE1              (0x01)
#define LEN_STATE2              (0x02)
#define DATA_STATE              (0x03)
#define FCS_STATE               (0x04)
#define END_STATE               (0x05)
	
#define RX_UART_FIX_LEN         (1 + 2 + 1 + 1)                // General_t固定数据长度
#define IS_RIGHT_SOP(x)         (x == CMD_SOP)
#define IS_RIGHT_CHK(x,y)       (x == y)
#define IS_RIGHT_FOOT(x)        (x == CMD_FOOT)
	
#define CMD_START_INX           (3)
#define CMD_TYPE_IDX            (CMD_START_INX+8+8)


#define DEVICE_SERIAL           (0xDDDDDDDDDDDDDDDD)
#define DEVICE_CONSOLE          (0xEEEEEEEEEEEEEEEE)

#define RX_MAX_PROCOTOL_LEN     (1024)
#define RX_UART_LEN             (RX_MAX_PROCOTOL_LEN)





/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/
#pragma pack(push,1)



typedef enum{
	Cmd_Direct_S_Req = 0,
	Cmd_Direct_C_Resp = 1,
	Cmd_Direct_C_Req = 2,
	Cmd_Direct_S_Resp = 3,
	Cmd_Direct_S_Report = 4,
	Cmd_Direct_C_Report = 5
}Cmd_Direct_e;

typedef enum{
	Cmd_Type_aoa_position    = 0x01,


	Cmd_Type_anccfg          = 0x02,
	Cmd_Type_tagcfg          = 0x03,

	Cmd_Type_Firmware_update = 0x10
}Cmd_Type_e;

typedef struct
{
	uint32_t is_lowbattery:1;			//是否低电量报警
	uint32_t is_alarm:1;				//是否报警
	uint32_t is_chrg:1;					//是否充电
	uint32_t is_tdby:1;					//是否充满
	uint32_t battery_val:10;			//电池电压350=3.50V
	uint32_t is_offset_range_zero_bit:1;//距离校正位
	uint32_t is_offset_pdoa_zero_bit:1;	//角度校正位

	uint32_t turn_up:1;					//(遥控专用)向前
	uint32_t turn_down:1;				//(遥控专用)向后
	uint32_t turn_left:1;				//(遥控专用)向左
	uint32_t turn_right:1;			    //(遥控专用)向右
	uint32_t mode:3;					//(遥控专用)模式
	uint32_t recal:1;					//(遥控专用)召回
	uint32_t lock:1;					//(遥控专用)上锁
	
	uint32_t dev_type:3;				//设备类型(0:学习板 1:手环 2:遥控器)
	uint32_t reserve:4;					//预留
}tag_detail_para_t;

typedef struct
{
	uint32_t timer;
	uint16_t anc_addr16;
	uint16_t tag_addr16;
	uint8_t  tag_sn;
	uint8_t  tag_mask;
	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
	}tag_tof_Ax[4];
	uint32_t tag_detailpara;
	struct{
		uint16_t x;
		uint16_t y;
		uint16_t z;
	}tag_acc;
	struct{
		uint16_t x;
		uint16_t y;
		uint16_t z;
	}tag_gcc;
	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
		int32_t  raw_degrees;
	}tof_self;
	uint8_t  reserve[8];
}aoa_position_t;


typedef struct
{
	uint8_t  header;
	uint16_t length;
	struct{
		uint64_t s_laddr;
		uint64_t d_laddr;
		uint8_t  type;
		uint8_t  direct;
		uint8_t  buf[1024];
	}cmd;
	uint8_t  check;
	uint8_t  footer;
}Msg_uart_t;
#pragma pack(pop)

/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
extern int App_Module_CMD_Queue_Init(void);

extern int App_Module_format_build(Cmd_Type_e type, Cmd_Direct_e direct, uint64_t d_longaddr, uint8_t *buf, uint16_t buf_len, dev_e dev);

extern int App_Module_Process_USART_CMD(void);
#ifdef __cplusplus
}
#endif
#endif//__GENERIC_AT_H

