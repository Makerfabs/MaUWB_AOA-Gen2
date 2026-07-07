#include "stm32f10x.h"	
#include "string.h"	
#include "stdbool.h"	
#include "OSAL.h"
#include "OSAL_Comdef.h"

#define USE_RSSI_TAG // USE_RSSI_TAG 有信号强度输出的基站  USE_RSSI_TAGX 没有信号强度输出的基站
#define RX_UART_LEN 220	
#define CMD_SOP                    	 0x2A  
#define CMD_FOOT                     0x23 

#define SOP_STATE      0x00
#define LEN_STATE      0x01
#define DATA_STATE     0x02
#define FCS_STATE      0x03
#define END_STATE      0x04

#define IS_RIGTH_FOOT(x)	(x == CMD_FOOT)

/***************************************************************************************************
 * 数据结构体
 ***************************************************************************************************/
#pragma pack(push,1)

typedef struct
{
	uint16_t angle_360;
	uint16_t angle_dir;
}motorAngle_t;//360度结构体

typedef enum{
	Cmd_Direct_S_Req = 0,
	Cmd_Direct_C_Resp = 1,
	Cmd_Direct_C_Req = 2,
	Cmd_Direct_S_Resp = 3,
	Cmd_Direct_S_Report = 4,
	Cmd_Direct_C_Report = 5
}Cmd_Direct_e;//命令帧方向

typedef enum
{
	Cmd_Type_uwb_position        = 0x01,	//UWB定位数据帧
	Cmd_Type_uwb_anccfg          = 0x02,	//UWB基站配置帧
	Cmd_Type_uwb_tagcfg          = 0x03,	//UWB标签配置帧
	Cmd_Type_uwb_heart           = 0x04,	//UWB心跳包帧
	Cmd_Type_uwb_transport       = 0x05,	//UWB透传帧
	Cmd_Type_uwb_Firmware_update = 0x10,	//UWB升级数据包

	Cmd_Type_rtk_position        = 0x21,	//RTK定位数据帧
	Cmd_Type_rtk_Nodecfg         = 0x22,	//RTK固定站配置帧
	Cmd_Type_rtk_Tagcfg          = 0x23,	//RTK移动站配置帧
	Cmd_Type_rtk_NodeAt          = 0x24,	//RTKat数据帧(固定站)
	Cmd_Type_rtk_TagAt           = 0x25,	//RTKat数据帧(移动站)
	Cmd_Type_rtk_Firmware_update = 0x30,	//RTK升级数据包

	Cmd_Type_aoa_rssi_position     = 0x64,	//UWB基站定位帧(RSSI)
	
	// TWR子类型
	Cmd_Type_twr_position_ancPos = 0x50,	//基站自定位

	// AOA子类型
	Cmd_Type_aoa_position_dbg      = 0x61,
	Cmd_Type_cloud_cfg           = 0x62,	//云台基站配置帧
	Cmd_Type_cloud_Firmware_update = 0x63,	//云台升级包

	// DEO子类型
	Cmd_Type_deo_position_high   = 0x70,	//DEO高速率

	// RTK子类型
	
}Cmd_Type_e;//命令帧类型

typedef struct
{
	uint8_t state;
	uint8_t total_len;    //统计接受总的个数 
	uint8_t tmp_len;			//目前接受个数
	uint8_t buf[RX_UART_LEN];
	bool timer;
}Rx_Uart_Flag_t;//串口接收数据暂存结构体

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
}tag_detail_para_t;//遥控命令结构体

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
}aoa_position_t;//AOA参数结构体

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
		int16_t  rssi;					//信号强度
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
	motorAngle_t motorAngle;
	uint8_t  reserve[5];
}aoa_rssi_position_t;//U1-AOA参数结构体 带信号强度


//新格式测试
typedef struct
{
	uint8_t  header;
	uint16_t length;
	struct{
		uint64_t s_laddr;
		uint64_t d_laddr;
		uint8_t  type;
		uint8_t  direct;
		uint8_t  buf[RX_UART_LEN];
	}cmd;
	uint8_t  check;
	uint8_t  footer;
}Msg_uart_t;//总数据储存

typedef struct
{
	
	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
	}tag_tof_Ax[4];//
	
	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
		int16_t  rssi;					//信号强度
	}More_tag_tof_Ax[4];
	
	int Angual_ONE;
	int distance_ONE;
	
	tag_detail_para_t  para_t;
	
	uint8_t check;
	uint8_t foot;
	
}General_t;//解析数据暂存结构体



#pragma pack(pop)


/***************************************************************************************************
 * 串口函数
 ***************************************************************************************************/
void Uart_Queue_Init(void);

bool get_AOA_data(Message *msg);

void App_Module_uint8_AoA(uint8_t *buf,General_t *aoa);

uint8_t AOA_Car_Hex_Resolution(uint8_t *buf,General_t *aoa);


