/* @file    uwb_frames.h
 * @brief
 *          UWB message frames definitions and typedefs
 *
 * @author Decawave Software
 *
 * @attention Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __UWB_FRAMES__H__
#define __UWB_FRAMES__H__

#ifdef __cplusplus
extern "C" {
#endif


#pragma anon_unions

#include <stdint.h>
#include <stdbool.h>
#include "OSAL_Comdef.h"


#define MAX_ANCHOR_LIST_SIZE        (4)		//最大支持基站数量
#define MAX_TAG_LIST_SIZE           (1)		//最大支持标签数量



//UWB空中数据包-帧控制&MAC地址长度
#define FRAME_CTRL                  (0x4188)
#define STANDARD_FRAME_SIZE         (1024)


//UWB空中数据包-功能码下标&功能码
#define MSG_FN_IDX                  (2+1+2+2+2)
#define TWR_BLINK_MSG_FN            (0xC7)
#define TWR_RNGCFG_MSG_FN           (0x10)
#define TWR_POLL_MSG_FN             (0x1A)
#define TWR_RESP_MSG_FN             (0x1B)
#define TWR_FINAL_MSG_FN            (0x1C)


#pragma pack(push,1)
typedef struct
{
	int16_t dist;						//久凌新增-距离值(单位:cm)
	int16_t angle;						//久凌新增-角度值(单位:°)
}tof_t;

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
	uint16_t frameCtrl;					//控制帧(2)
	uint8_t  seqNum;					//通信序列号(1)
	uint16_t panID;						//个人网络(2)
	uint16_t destAddr;					//目标地址(2)
	uint16_t sourceAddr;				//源地址(2)
}mac_header_ss_t;

typedef struct
{
	uint8_t  fCode;						//功能码(1)
	uint64_t tagId;						//标签长地址(8)
}blink_t;

typedef struct
{
	uint8_t  fCode;						//功能码(1)
	uint64_t tagId;						//标签长地址(唯一ID)
	uint16_t tag_saddr;					//标签短地址
	uint16_t sframePeriod_ms;			//标签一个轮训周期
	uint16_t pollTxToFinalTx_us;		//标签发送Poll到发送Final时长
	uint16_t rxRespDlyRx_us;			//标签开启接受时长
	uint16_t tag_mode;					//标签模式	
	bool     slotCorr_valid;			//标签休眠修正时间有效值(基站开机前60秒为无效)	
	int32_t  slotCorr_ms;				//标签校正参数
}rngcfg_t;


typedef struct
{
	uint8_t  fCode;						//功能码
	uint8_t  rNum;						//序列号
	tag_detail_para_t t2a_sensor;		//标签传感器参数
}poll_t;

typedef struct
{
	uint8_t fCode;						//功能码
	int32_t slotCorr_ms;				//标签校正参数
	uint8_t rNum;						//序列号
	uint16_t a2t_usercmd;				//基站自定义命令
	tof_t last_tof;						//基站到该标签距离值(上一回合)
}resp_t;


typedef struct
{
	uint8_t fCode;						//功能码
	uint8_t rNum;						//序列号
	uint64_t pollTx_ts;					//标签Poll发送时间戳
	uint64_t responseRx_ts[MAX_ANCHOR_LIST_SIZE];//标签Resp接受时间戳
	uint64_t finalTx_ts;				//标签Final发送时间戳
	uint8_t  rxResponseMask;			//标签接受Resp有效位
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

	tof_t tof[MAX_ANCHOR_LIST_SIZE];	//标签到各个基站的距离/角度参数
}final_t;




typedef struct
{
	mac_header_ss_t mac;
	blink_t         blink;
	uint8_t         fcs[2];
}blink_msg_t;							//blink数据包


typedef struct
{
	mac_header_ss_t mac;
	rngcfg_t        rngcfg;
	uint8_t         fcs[2];
}rngcfg_msg_t;							//rngcfg数据包


typedef struct
{
	mac_header_ss_t mac;
	poll_t          poll;
	uint8_t         fcs[2];
}poll_msg_t;							//poll数据包

typedef struct
{
	mac_header_ss_t mac;
	resp_t          resp;
	uint8_t         fcs[2];
}resp_msg_t;							//resp数据包

typedef struct
{
	mac_header_ss_t mac;
	final_t         final;
	uint8_t         fcs[2];
}final_msg_t;							//final数据包
#pragma pack(pop)


#ifdef __cplusplus
}
#endif

#endif /* __UWB_FRAMES__H__ */
