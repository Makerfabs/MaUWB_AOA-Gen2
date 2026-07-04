#include "uwb.h"
#include "OSAL.h"
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "hal_usart.h"


static Rx_Uart_Flag_t rx_uart_flag;
static QUEUE uart_queue;
u8 Res;

enum UART_FRAME{
	UART_FRAME_CMD =2	
};


/**************************************************************************
函数名  ：Uart_Queue_Init
函数功能：队列数据初始化，防止队列卡顿
入口参数：无
返回  值：无 
**************************************************************************/ 
void Uart_Queue_Init(void)
{
	memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag));			

	osal_CreateQueue(&uart_queue, QUEUE_MSG_MAX);	
}

/**************************************************************************
函数名  ：get_Xor_CRC
函数功能：CRC 验证
入口参数：无
返回  值：无 
**************************************************************************/ 
uint8_t get_Xor_CRC(uint8_t *bytes, int offset, int len) 
{
	uint8_t xor_crc = 0;
	int i;
	for (i = 0; i < len; i++) {
		xor_crc ^= bytes[offset + i];
	}
	return xor_crc;
} 

/**************************************************************************
函数名  ：usart3_resv_msg_check
函数功能：msg 校验
入口参数：无
返回  值：无 
**************************************************************************/ 
static uint8_t usart3_resv_msg_check(Rx_Uart_Flag_t rx_uart_flag)
{
		uint8_t ret = FALSE;

		if(IS_RIGTH_FOOT(rx_uart_flag.buf[rx_uart_flag.total_len - 1])
#ifdef HW_RELEASE
			&&get_Xor_CRC(rx_uart_flag.buf, 2, rx_uart_flag.total_len - 4) == rx_uart_flag.buf[rx_uart_flag.total_len - 2]
#endif
			&&rx_uart_flag.total_len >= 11
			)
			ret = TRUE;
		return ret;
}


/**************************************************************************
函数名  ：usart3_recv_msg_handler
函数功能：接收数据验证
入口参数：无
返回  值：无 
**************************************************************************/ 
static uint8_t usart3_recv_msg_handler(uint8_t ch)
{
		uint8_t ret = FALSE;
		switch (rx_uart_flag.state)
		{
			case SOP_STATE://检测接收
				if (ch == CMD_SOP)
				{
					rx_uart_flag.state = LEN_STATE;//开始接收
					rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
					//rx_uart_flag.timer = true;
				}	
				break;
		
			case LEN_STATE://计算字节数
				{
					rx_uart_flag.state = DATA_STATE;			
					rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
					rx_uart_flag.total_len = ch + 2 + 1 + 1 + 1;//head + len + check + footer 计算数量
					if(rx_uart_flag.total_len > RX_UART_LEN)
					{
						//清零结构体
						memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag)); 	
					}
				}
				break;
			case DATA_STATE://
				{
					rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
					if(rx_uart_flag.total_len - rx_uart_flag.tmp_len <= 0)//检测到参数接收完毕
					{								
						if((ret = usart3_resv_msg_check(rx_uart_flag)) == TRUE)
						{
						}
						else
						{
#ifdef	HW_RTLS_ANC_UART_TEST				
							uint8 debug_buf[128];
							uint8 str_buf[128];
							osal_Hex2Str(rx_uart_flag.buf, str_buf, rx_uart_flag.total_len);
							sprintf(debug_buf, "error Ax:%04X, len:%d buf:%s\n", instancegetaddresses(),rx_uart_flag.total_len, str_buf);
							USART1_SendBuffer(debug_buf, strlen(debug_buf), TRUE);	
#endif
						}
						rx_uart_flag.state = SOP_STATE;
						rx_uart_flag.tmp_len = 0;
						//rx_uart_flag.timer = false;
					}
				}
				break;
			case FCS_STATE:
			case END_STATE:
			default:
			 break;
		}
		return ret;	
}

/**************************************************************************
函数名  ：USART2_IRQHandler
函数功能：USART2接收中断
入口参数：无
返回  值：无 
**************************************************************************/ 
void USART2_IRQHandler(void) 
{
		if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  
		{ 
			USART_ClearITPendingBit(USART2,USART_IT_RXNE);
			Res = USART_ReceiveData(USART2); //读取接收到的数据
			if(usart3_recv_msg_handler(Res))
			{
				//写入队列
				Message msg;
				msg.flag = UART_FRAME_CMD;
				msg.len = rx_uart_flag.total_len;
				memset(msg.buf,0,sizeof(msg.buf));			
				memcpy(msg.buf, rx_uart_flag.buf, msg.len);
				osal_Enqueue(&uart_queue, msg);
		
				//清零结构体
				memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag));			
			}
		}
}

/**************************************************************************
函数名  ：get_AOA_data
函数功能：判断是否接收到 AOA数据
入口参数：无
返回  值：无 
**************************************************************************/ 
bool get_AOA_data(Message *msg)
{		
		return osal_Dequeue(&uart_queue, msg);
}

/**************************************************************************
函数名  ：AOA_Car_Hex_Resolution
函数功能：AOA 数据解析
入口参数：无
返回  值：无 
**************************************************************************/ 
uint8_t AOA_Car_Hex_Resolution(uint8_t *buf,General_t *aoa)
{
		tag_detail_para_t t2a_s;
		Msg_uart_t *msg_uart = (Msg_uart_t *)(buf);
//		
#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
		if(msg_uart->cmd.type== Cmd_Type_aoa_rssi_position  && msg_uart->cmd.direct==Cmd_Direct_C_Report)//判断 命令帧类型与方向
		{
			aoa_rssi_position_t *aoa_position = (aoa_rssi_position_t *)(msg_uart->cmd.buf);//提取整体数据
			memcpy(&aoa->More_tag_tof_Ax, &aoa_position->tag_tof_Ax, sizeof(aoa->More_tag_tof_Ax));//复制
#else  //不带信号强度
		if(msg_uart->cmd.type== Cmd_Type_uwb_position  && msg_uart->cmd.direct==Cmd_Direct_C_Report)//判断 命令帧类型与方向
		{
			aoa_position_t *aoa_position = (aoa_position_t *)(msg_uart->cmd.buf);//提取整体数据
			memcpy(&aoa->tag_tof_Ax, &aoa_position->tag_tof_Ax, sizeof(aoa->tag_tof_Ax));//复制
#endif
//			//参数赋值
			*(uint32_t *)(&t2a_s)=(aoa_position->tag_detailpara);//获取遥控命令
			memcpy(&aoa->para_t, &t2a_s, sizeof(t2a_s));//复制
#if 1
//			_dbg_printf("dev_type:%d  lock:%d  recal:%d  mode:%d  turn_right:%d  turn_left:%d  turn_down:%d  turn_up:%d\n",aoa->para_t.dev_type,aoa->para_t.lock,
//																															aoa->para_t.recal,aoa->para_t.mode,aoa->para_t.turn_right,
//																															aoa->para_t.turn_left,aoa->para_t.turn_down,aoa->para_t.turn_up);
		#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
//			_dbg_printf("1 %d %d %d   %d %d %d \r\n",aoa->More_tag_tof_Ax[0].angle,aoa->More_tag_tof_Ax[0].range,aoa->More_tag_tof_Ax[0].rssi,
//														aoa->More_tag_tof_Ax[1].angle,aoa->More_tag_tof_Ax[1].range,aoa->More_tag_tof_Ax[1].rssi);
		#else  //不带信号强度
			_dbg_printf("2 %d %d %d %d %d %d %d %d\r\n",aoa->tag_tof_Ax[0].angle,aoa->tag_tof_Ax[0].range,
										aoa->tag_tof_Ax[1].angle,aoa->tag_tof_Ax[1].range,
										aoa->tag_tof_Ax[2].angle,aoa->tag_tof_Ax[2].range,
										aoa->tag_tof_Ax[3].angle,aoa->tag_tof_Ax[3].range);
		#endif
#endif
			
			return 1;
		}
		return 0;
}















