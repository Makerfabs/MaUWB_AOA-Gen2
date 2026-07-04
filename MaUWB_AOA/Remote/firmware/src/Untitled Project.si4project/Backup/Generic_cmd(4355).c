#include "Generic.h"
#include "Generic_cmd.h"

#include "hal_usart.h"
#include "hal_flash.h"

#include "OSAL.h"

enum UART_FRAME{
	UART_FRAME_AT = 1,
	UART_FRAME_CMD =2
};

static QUEUE uart_queue;
static Rx_Uart_Flag_t rx_uart_flag = {
	.timer = false
};



uint64_t ChipID = 0;


/**
  * @brief  Get_ChipID(STM32唯一ID为96bit,通过两两相加的方式，获取一个64bit的唯一ID)
  * @param  无
  * @retval 返回64位的的唯一ID
  */
uint64_t Get_ChipID(void)
{
	uint64_t ChipID64 = 0;
	uint64_t ChipUniqueID64[3];
	uint32_t ChipUniqueID[3];
	
	ChipUniqueID[0] = *(__IO u32 *)(0X1FFFF7F0); // 高字节
	ChipUniqueID[1] = *(__IO u32 *)(0X1FFFF7EC); // 
	ChipUniqueID[2] = *(__IO u32 *)(0X1FFFF7E8); // 低字节

	ChipUniqueID64[0] =  (uint64_t)(((uint64_t)ChipUniqueID[1] << 32) | ChipUniqueID[2]);
	ChipUniqueID64[1] =  (uint64_t)(((uint64_t)ChipUniqueID[0] << 32) | ChipUniqueID[2]);
	ChipUniqueID64[2] =  (uint64_t)(((uint64_t)ChipUniqueID[0] << 32) | ChipUniqueID[1]);
	
	for(int i = 0; i < 3; i++)
	{
		ChipID64 += ChipUniqueID64[i];
	}
	
//	_dbg_printf("%08X %08X %08X, %08X %08X\n", ChipUniqueID[0],ChipUniqueID[1],ChipUniqueID[2] , (uint32_t)(ChipID64 >> 32), (uint32_t)(ChipID64));
//	
//	uint8_t buf[8];
//	memcpy(buf, (uint8_t *)(&ChipID64), 8);
//	_dbg_printf("字节数组:%02X%02X%02X%02X%02X%02X%02X%02X\n",
//		buf[0],
//	buf[1],
//	buf[2],
//	buf[3],
//	buf[4],
//	buf[5],
//	buf[6],
//	buf[7]);
//	
	return ChipID64;
}

/**
  * @brief  Get_ChipID(STM32唯一ID为96bit,通过两两相加的方式，获取一个64bit的唯一ID)
  * @param  无
  * @retval 返回64位的的唯一ID
  */
uint32_t Get_Short_ChipID(void)
{
	uint32_t addr32h, addr32l;
	addr32h = (uint32_t)(ChipID >> 32);
	addr32l = (uint32_t)(ChipID);

	return ((addr32h+addr32l)&999999) ;
}


int App_Module_CMD_Queue_Init(void)
{
	memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag));
	
	osal_CreateQueue(&uart_queue, QUEUE_MSG_MAX);
	
	ChipID = Get_ChipID();
}


int App_Module_Process_USART_CMD(void)
{
	Message msg;
	if(osal_Dequeue(&uart_queue, &msg) == TRUE)
	{
		uint8_t cmd_type = msg.buf[CMD_TYPE_IDX];
		switch(cmd_type)
		{
			case Cmd_Type_aoa_position:
				break;
			
			case Cmd_Type_anccfg:
			case Cmd_Type_tagcfg:
				{
					char at_command[256];
					int command_length = (msg.len - RX_UART_FIX_LEN - 1 - 1 - 8 - 8);
					memset(at_command, 0, sizeof(at_command));
					memcpy(at_command, (char *)((Msg_uart_t *)(msg.buf))->cmd.buf,  command_length);
					command_parser(at_command);
				}
				break;
				
			case Cmd_Type_Firmware_update:
				{
					Msg_uart_t *msg_uart = (Msg_uart_t *)msg.buf;
					if(msg_uart->cmd.buf[0] == Firmware_Reset)
					{
						NVIC_SystemReset();
					}
				}
				break;
				
			default:
				break;
		}
	}
}

/*
 *	功能:串口发送函数1
 *	形参:buf(发送数组)，len(发送数组长度)
 *
 */
int App_Module_Uart_Send(uint8_t *buf, uint16_t len)
{
	int ret = 0;
	if((sys_uart_dma_buf.uart_dma_index + len) < sizeof(sys_uart_dma_buf.uart_dma_tx_tmp))
	{
		/*赋值到缓存区*/
		memcpy(sys_uart_dma_buf.uart_dma_tx_tmp + sys_uart_dma_buf.uart_dma_index, buf, len);
		sys_uart_dma_buf.uart_dma_index += len;
	}
	else
	{
//		while(1);
	}
	
	if(sys_uart_dma_buf.uart_dma_report == true && sys_uart_dma_buf.uart_dma_index != 0)
	{
		/*赋值到发送区*/
		memcpy(sys_uart_dma_buf.uart_dma_tx, sys_uart_dma_buf.uart_dma_tx_tmp, sys_uart_dma_buf.uart_dma_index);
		USART1_SendBuffer(sys_uart_dma_buf.uart_dma_tx, sys_uart_dma_buf.uart_dma_index, false);
		sys_uart_dma_buf.uart_dma_report = false;
		sys_uart_dma_buf.uart_dma_index = 0;
	}

	return ret;
}

/*
 *	功能:串口发送函数2
 *	形参:buf(发送数组)，len(发送数组长度)
 *
 */
void port_tx_msg(uint8_t *buf, uint16_t len)
{
	// 1:DMA_UART 0:UART
	if(11)
	{
		App_Module_Uart_Send(buf, len);
	}
	else
	{
		USART1_SendBuffer((const char*)buf, len, true);
	}
}

uint8_t get_Xor_CRC(uint8_t *bytes, int offset, int len) 
{
	uint8_t xor_crc = 0;
	int i;
	for (i = 0; i < len; i++) {
		xor_crc ^= bytes[offset + i];
	}
	    
	return xor_crc;
}  


int App_Module_format_conver_uint8(Msg_uart_t msg, uint8_t *buf)
{
	int index = 0;
	buf[index++] = msg.header;
	buf[index++] = LO_UINT16(msg.length);
	buf[index++] = HI_UINT16(msg.length);
	
	if(msg.length != 0)
	{
		memcpy(&buf[index], &msg.cmd, msg.length);
		index += msg.length;
	}
	
	buf[index++] = get_Xor_CRC(buf, CMD_START_INX, msg.length);
	buf[index++] = msg.footer;

	return index;
}

int App_Module_format_build(Cmd_Type_e type, Cmd_Direct_e direct, uint64_t d_longaddr, uint8_t *buf, uint16_t buf_len)
{
	Msg_uart_t msg;
	uint8_t send_buf[256];
	int send_len;
	

	msg.header = CMD_SOP;
	msg.length = 1 + 1 + 8 + 8 + buf_len;

	{
		msg.cmd.s_laddr = ChipID;
		msg.cmd.d_laddr = d_longaddr;
		msg.cmd.type = type;
		msg.cmd.direct = direct;
		if(buf_len != 0){memcpy(msg.cmd.buf, buf, buf_len);}
	}
	
	msg.check = 0;
	msg.footer = CMD_FOOT;
	send_len = App_Module_format_conver_uint8(msg, send_buf);
	
	port_tx_msg(send_buf, send_len);
}



static uint8_t usart1_recv_msg_handler(uint8_t ch)
{
	bool ret = false;
	switch (rx_uart_flag.state)
	{
		case SOP_STATE:
			if(ch == CMD_SOP)
			{
				rx_uart_flag.state = LEN_STATE1;
				rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
			}
			break;

		case LEN_STATE1:
			{
				rx_uart_flag.state = LEN_STATE2;
				rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
			}
			break;
		case LEN_STATE2:
			{
				int len = BUILD_UINT16(rx_uart_flag.buf[rx_uart_flag.tmp_len-1], ch);
				rx_uart_flag.state = DATA_STATE;
				rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
				rx_uart_flag.total_len = len + RX_UART_FIX_LEN;
				if(rx_uart_flag.total_len > RX_MAX_PROCOTOL_LEN)
				{
					//清零结构体
					memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag));
				}
			}
			break;
		case DATA_STATE:
			{
				rx_uart_flag.buf[rx_uart_flag.tmp_len++] = ch;
				if(rx_uart_flag.total_len - rx_uart_flag.tmp_len <= 0)
				{
					uint8_t check = get_Xor_CRC(rx_uart_flag.buf, CMD_START_INX, rx_uart_flag.total_len - RX_UART_FIX_LEN);
					uint8_t _check = rx_uart_flag.buf[rx_uart_flag.total_len - 2];
					uint8_t _footer = rx_uart_flag.buf[rx_uart_flag.total_len - 1];
					if( IS_RIGHT_FOOT(_footer) && IS_RIGHT_CHK(check, _check))
					{
						ret = true;
					}
					else
					{
					}

					rx_uart_flag.state = SOP_STATE;
					rx_uart_flag.tmp_len = 0;
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

void USART1_IRQHandler(void) 
{
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)	//溢出中断(参考官方文档[STM32F10x微控制器参考手册(2009年12月第10版)P541])
	{
		USART_ReceiveData(USART1);
	}

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)		//接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		uint8_t Res = USART_ReceiveData(USART1); //读取接收到的数据(USART1->DR)
		if(usart1_recv_msg_handler(Res))
		{
			//写入队列
			Message msg;
			memset(&msg, 0, sizeof(msg));
			msg.flag = UART_FRAME_CMD;
			msg.len = rx_uart_flag.total_len ;
			memcpy(msg.buf, rx_uart_flag.buf, msg.len);
			osal_Enqueue(&uart_queue, msg);

			// 异常中断复位处理
			if(msg.buf[CMD_TYPE_IDX] == Cmd_Type_Firmware_update)
			{
				if(((Msg_uart_t *)msg.buf)->cmd.buf[0] == Firmware_Reset)
				{
					NVIC_SystemReset();
				}
			}
			
//			memset(&rx_uart_flag.state,0,sizeof(rx_uart_flag));
		}
	}
}
/*
 * 串口1:DMA发送完成中断
 *
 */
void DMA1_Channel4_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_FLAG_TC4))
	{
		sys_uart_dma_buf.uart_dma_report = true;
		DMA_ClearFlag(DMA1_FLAG_TC4); //清除全部中断标志
	}
}

