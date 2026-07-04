
#include "UWB.h"
#include "PC.h"
#include "Generic.h"
#include "Generic_cmd.h"

#include "hal_usart.h"

#include "OSAL.h"

static Tx_Uart_Flag_t pc_tx_flag ={
	.uart_dma_tx = 0,
	.uart_dma_report = true,
	.uart_dma_index = 0
};

static Rx_Uart_Flag_t pc_rx_flag = {
	.timer = false
};
	
static QUEUE pc_rx_queue;


/*
 *	功能:串口发送函数1
 *	形参:buf(发送数组)，len(发送数组长度)
 *
 */
int PC_Uart_sendCmd_Dma(uint8_t *buf, uint16_t len)
{
	int ret = 0;
	if((pc_tx_flag.uart_dma_index + len) < sizeof(pc_tx_flag.uart_dma_tx_tmp))
	{
		/*赋值到缓存区*/
		memcpy(pc_tx_flag.uart_dma_tx_tmp + pc_tx_flag.uart_dma_index, buf, len);
		pc_tx_flag.uart_dma_index += len;
	}
	else
	{
//		while(1);
	}
	
	if(pc_tx_flag.uart_dma_report == true && pc_tx_flag.uart_dma_index != 0)
	{
		/*赋值到发送区*/
		memcpy(pc_tx_flag.uart_dma_tx, pc_tx_flag.uart_dma_tx_tmp, pc_tx_flag.uart_dma_index);
		USART1_SendBuffer(pc_tx_flag.uart_dma_tx, pc_tx_flag.uart_dma_index, false);
		pc_tx_flag.uart_tx_last_timer = portGetTickCnt();
		pc_tx_flag.uart_dma_report = false;
		pc_tx_flag.uart_dma_index = 0;
	}
	
	if (pc_tx_flag.uart_dma_report == false && (portGetTickCnt() - pc_tx_flag.uart_tx_last_timer) > 2000)
	{
		pc_tx_flag.uart_dma_report = true;
		DMA_ClearFlag(DMA1_IT_TC4); //清除全部中断标志
	}

	return ret;
}



int PC_Uart_recvCmd(uint8_t ch, Rx_Uart_Flag_t *rx_uart_flag)
{
	bool ret = false;
	switch (rx_uart_flag->state)
	{
		case SOP_STATE:
			if(ch == CMD_SOP)
			{
				rx_uart_flag->state = LEN_STATE1;
				rx_uart_flag->buf[rx_uart_flag->tmp_len++] = ch;
			}
			break;

		case LEN_STATE1:
			{
				rx_uart_flag->state = LEN_STATE2;
				rx_uart_flag->buf[rx_uart_flag->tmp_len++] = ch;
			}
			break;
		case LEN_STATE2:
			{
				int len = BUILD_UINT16(rx_uart_flag->buf[rx_uart_flag->tmp_len-1], ch);
				rx_uart_flag->state = DATA_STATE;
				rx_uart_flag->buf[rx_uart_flag->tmp_len++] = ch;
				rx_uart_flag->total_len = len + RX_UART_FIX_LEN;
				if(rx_uart_flag->total_len > RX_MAX_PROCOTOL_LEN)
				{
					//清零结构体
					memset(&rx_uart_flag->state,0,sizeof(Rx_Uart_Flag_t));
				}
			}
			break;
		case DATA_STATE:
			{
				rx_uart_flag->buf[rx_uart_flag->tmp_len++] = ch;
				if(rx_uart_flag->total_len - rx_uart_flag->tmp_len <= 0)
				{
					uint8_t check = get_Xor_CRC(rx_uart_flag->buf, CMD_START_INX, rx_uart_flag->total_len - RX_UART_FIX_LEN);
					uint8_t _check = rx_uart_flag->buf[rx_uart_flag->total_len - 2];
					uint8_t _footer = rx_uart_flag->buf[rx_uart_flag->total_len - 1];
					if( IS_RIGHT_FOOT(_footer) && IS_RIGHT_CHK(check, _check))
					{
						ret = true;
					}
					else
					{
					}

					rx_uart_flag->state = SOP_STATE;
					rx_uart_flag->tmp_len = 0;
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



/*
 * 串口1接受
 *
 */
void USART1_IRQHandler(void) 
{
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)		//溢出中断(参考官方文档[STM32F10x微控制器参考手册(2009年12月第10版)P541])
	{
		USART_ReceiveData(USART1);
	}

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		DMA_Cmd(DMA1_Channel5, DISABLE);
		USART1->DR;
	
		uint16_t rx_cacheIdx = (sizeof(pc_rx_cache) - DMA1_Channel5->CNDTR);
		
		// 入队列
		osal_Enqueue(&pc_rx_queue, 0, pc_rx_cache, rx_cacheIdx);
	
//		DMA_Cmd(DMA1_Channel5,DISABLE); //要关闭之后再设置，否则设置无法生效
		DMA1_Channel5->CNDTR = sizeof(pc_rx_cache);
		DMA_Cmd(DMA1_Channel5,ENABLE);
	}
}


/*
 * 串口1:DMA接受完成中断
 *
 */
void DMA1_Channel5_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_IT_TC5))
	{
		uint16_t rx_cacheIdx = sizeof(pc_rx_cache);

		// 入队列
		osal_Enqueue(&pc_rx_queue, 0, pc_rx_cache, rx_cacheIdx);

		DMA_Cmd(DMA1_Channel5,DISABLE); //要关闭之后再设置，否则设置无法生效
		DMA1_Channel5->CNDTR = sizeof(pc_rx_cache);
		DMA_Cmd(DMA1_Channel5,ENABLE);

		DMA_ClearFlag(DMA1_IT_TC5); //清除全部中断标志
	}
}

/*
 * 串口1DMA发送完成中断
 *
 */
void DMA1_Channel4_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_IT_TC4))
	{
		pc_tx_flag.uart_dma_report = true;
		DMA_ClearFlag(DMA1_IT_TC4); //清除全部中断标志
	}
}



int PC_Process_USART_RecvCMD(void)
{
	Message *pc_deal_cache = NULL;

	if (osal_Dequeue_Ptr(&pc_rx_queue, &pc_deal_cache) == false) {
		return -1;
	}
	
	// 安全检查
	if (pc_deal_cache == NULL || pc_deal_cache->len == 0 || pc_deal_cache->len > QUEUE_MSG_LEN) {
		return -2;
	}

	for (int i = 0; i < pc_deal_cache->len; i++)
	{
		if (PC_Uart_recvCmd(pc_deal_cache->buf[i], &pc_rx_flag)) {
//			//验证基站MAC是否正确
//			if (((Msg_uart_t *)pc_rx_flag.buf)->cmd.d_laddr != Get_ChipID() && ((Msg_uart_t *)pc_rx_flag.buf)->cmd.d_laddr != DEVICE_SERIAL) {
//				return -1;
//			}

			uint16_t msg_len = pc_rx_flag.total_len;
			uint8_t cmd_type = pc_rx_flag.buf[CMD_TYPE_IDX];
			
			switch(cmd_type)
			{
				case Cmd_Type_anccfg:
				case Cmd_Type_tagcfg:
				case Cmd_Type_Firmware_update:
					{
						// 透传给UWB
						UWB_Uart_sendCmd_Dma(pc_rx_flag.buf, pc_rx_flag.total_len);
					}
					break;
					
				default:
					break;
			}
		}
	}
}

int PC_Process_USART_SendCMD(void)
{
	if(pc_tx_flag.uart_dma_index != 0)
	{
		PC_Uart_sendCmd_Dma(NULL, 0);
	}
}


int PC_Init(void)
{
	memset(&pc_rx_flag, 0, sizeof(Rx_Uart_Flag_t));
	osal_CreateQueue(&pc_rx_queue, QUEUE_MSG_MAX);
	
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);							//DMA接收缓冲区满中断使能
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
}

/*
 * 功能@1.接受串口数据
 *     @2.发送串口数据
 */
int PC_task(void)
{
	// ①接受串口数据
	PC_Process_USART_RecvCMD();

	// ②发送串口数据
	PC_Process_USART_SendCMD();
}



