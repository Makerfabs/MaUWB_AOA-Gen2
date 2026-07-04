#ifndef __STRUCT_DEF_H
#define __STRUCT_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OSAL.h"

#define BUF_MAXSIZE   (1024)

typedef enum
{
	DEV_Invalid = 0,
	DEV_PC      = 1,
	DEV_UWB     = 2,
}dev_e;



#pragma pack(push,1)


typedef struct
{
	uint8_t uart_dma_tx[BUF_MAXSIZE];
	uint8_t uart_dma_tx_tmp[BUF_MAXSIZE];
	int     uart_dma_report;
	int     uart_dma_index;
	
	uint32_t uart_tx_last_timer;		//串口最后发送时间
}Tx_Uart_Flag_t;


// 接受(同类型)数据包结构体
typedef struct
{
	uint8_t  state;
	uint16_t total_len;					//统计接受总的个数
	uint16_t tmp_len;					//目前接受个数
	uint8_t  buf[BUF_MAXSIZE];
	
	uint32_t timer;
	uint8_t  type;						//RTK数据包时使用
}Rx_Uart_Flag_t;


typedef struct
{
	uint8_t uart_dma_tx[BUF_MAXSIZE/2 + 128];
	uint8_t uart_dma_tx_tmp[BUF_MAXSIZE/2 + 128];
	int uart_dma_report;
	int uart_dma_index;
	
	uint32_t uart_tx_last_timer;		//串口发送完成时间
}Tx_UartSM_Flag_t;						//缩减版数据



typedef struct
{
	uint8_t uart_dma_tx[BUF_MAXSIZE*2];
	uint8_t uart_dma_tx_tmp[BUF_MAXSIZE*2];
	int uart_dma_report;
	int uart_dma_index;
	
	uint32_t uart_tx_last_timer;		//串口最后发送时间
}Tx_UartLG_Flag_t;						//加长版数据


typedef struct
{
	uint8_t cache[BUF_MAXSIZE];
	int     cacheLen;
}Tx_Dtu_Flag_t;

typedef struct
{
	char* cmd;
	char* expect_response;
	uint8_t retry;
}Tx_Init_Cmd_t;



#pragma pack(pop)


#ifdef __cplusplus
}
#endif
#endif//__Deo_Apply_H
