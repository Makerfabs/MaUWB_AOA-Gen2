#include "UWB.h"
#include "Generic.h"
#include "Generic_cmd.h"

#include "hal_usart.h"

#include "OSAL.h"


// UWB AT命令集
// ①SETSEN (x1)					//x1为32bit的透传数据
// ②SETSLP (x1)					//x1为十进制的数据,单位ms
// ③GETID   


static Tx_Uart_Flag_t uwb_tx_flag ={
	.uart_dma_tx = 0,
	.uart_dma_report = true,
	.uart_dma_index = 0
};

static Rx_Uart_Flag_t uwb_rx_flag = {
	.timer = false
};
	
static QUEUE uwb_rx_queue;


/*
 *	功能:串口发送函数1
 *	形参:buf(发送数组)，len(发送数组长度)
 *
 */
int UWB_Uart_sendCmd_Dma(uint8_t *buf, uint16_t len)
{
	int ret = 0;
	if((uwb_tx_flag.uart_dma_index + len) < sizeof(uwb_tx_flag.uart_dma_tx_tmp))
	{
		/*赋值到缓存区*/
		memcpy(uwb_tx_flag.uart_dma_tx_tmp + uwb_tx_flag.uart_dma_index, buf, len);
		uwb_tx_flag.uart_dma_index += len;
	}
	else
	{
//		while(1);
	}
	
	if(uwb_tx_flag.uart_dma_report == true && uwb_tx_flag.uart_dma_index != 0)
	{
		/*赋值到发送区*/
		memcpy(uwb_tx_flag.uart_dma_tx, uwb_tx_flag.uart_dma_tx_tmp, uwb_tx_flag.uart_dma_index);
		USART3_SendBuffer(uwb_tx_flag.uart_dma_tx, uwb_tx_flag.uart_dma_index);
		uwb_tx_flag.uart_tx_last_timer = portGetTickCnt();
		uwb_tx_flag.uart_dma_report = false;
		uwb_tx_flag.uart_dma_index = 0;
	}
	
	if(uwb_tx_flag.uart_dma_report == false && (portGetTickCnt() - uwb_tx_flag.uart_tx_last_timer) > 2000)
	{
		uwb_tx_flag.uart_dma_report = true;
		DMA_ClearFlag(DMA1_IT_TC2); //清除全部中断标志
	}

	return ret;
}



/*
 * 串口3接受(UWB口)
 *
 */
void USART3_IRQHandler(void) 
{
	if(USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)		//溢出中断(参考官方文档[STM32F10x微控制器参考手册(2009年12月第10版)P541])
	{
		USART_ReceiveData(USART3);
	}

	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		DMA_Cmd(DMA1_Channel3, DISABLE);
		USART3->DR;
	
		uint16_t rx_cacheIdx = (sizeof(uwb_rx_cache) - DMA1_Channel3->CNDTR);

		// 入队列
		osal_Enqueue(&uwb_rx_queue, 0, uwb_rx_cache, rx_cacheIdx);
	
//		DMA_Cmd(DMA1_Channel3,DISABLE); //要关闭之后再设置，否则设置无法生效
		DMA1_Channel3->CNDTR = sizeof(uwb_rx_cache);
		DMA_Cmd(DMA1_Channel3,ENABLE);
	}
}


/*
 * 串口3:DMA接受完成中断
 *
 */
void DMA1_Channel3_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_IT_TC3))
	{
		uint16_t rx_cacheIdx = sizeof(uwb_rx_cache);

		// 入队列
		osal_Enqueue(&uwb_rx_queue, 0, uwb_rx_cache, rx_cacheIdx);

		DMA_Cmd(DMA1_Channel3,DISABLE); //要关闭之后再设置，否则设置无法生效
		DMA1_Channel3->CNDTR = sizeof(uwb_rx_cache);
		DMA_Cmd(DMA1_Channel3,ENABLE);

		DMA_ClearFlag(DMA1_IT_TC3); //清除全部中断标志
	}
}

/*
 * 串口3DMA发送完成中断
 *
 */
void DMA1_Channel2_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA1_IT_TC2))
	{
		uwb_tx_flag.uart_dma_report = true;
		DMA_ClearFlag(DMA1_IT_TC2); //清除全部中断标志
	}
}




int UWB_Process_USART_RecvCMD(void)
{
	Message *uwb_deal_cache = NULL;  // 关键：栈上只有4字节指针！

	if (osal_Dequeue_Ptr(&uwb_rx_queue, &uwb_deal_cache) == false) {
		return -1;
	}
	
	// 安全检查
	if (uwb_deal_cache == NULL || uwb_deal_cache->len == 0 || uwb_deal_cache->len > QUEUE_MSG_LEN) {
		return -2;
	}

	for (int i = 0; i < uwb_deal_cache->len; i++)
	{
		if (PC_Uart_recvCmd(uwb_deal_cache->buf[i], &uwb_rx_flag)) {
			uint16_t msg_len = uwb_rx_flag.total_len;
			uint8_t cmd_type = uwb_rx_flag.buf[CMD_TYPE_IDX];
			
			switch(cmd_type)
			{
				case Cmd_Type_aoa_position:
					{
						sys_state.last_recvUwbTime = portGetTickCnt();
						
						uint64_t        uwbId           = ((Msg_uart_t *)(uwb_rx_flag.buf))->cmd.s_laddr;
						aoa_position_t *aoa_position    = (aoa_position_t *)(((Msg_uart_t *)(uwb_rx_flag.buf))->cmd.buf);
						uint32_t        tag_detail_para = *(uint32_t*)&sys_state.tag_detail_para;
						
						if (tag_detail_para != aoa_position->tag_detailpara) {
							uint8_t buf[64];
							memset(buf, 0, sizeof(buf));
							sprintf(buf, "setsen %08X\r\n", tag_detail_para);
							App_Module_format_build(Cmd_Type_tagcfg, Cmd_Direct_S_Req, uwbId, buf, strlen(buf), DEV_UWB);

//							UWB_LOG("sensor diff:%d, %08X %08X\n", tag_detail_para != aoa_position->tag_detailpara, tag_detail_para, aoa_position->tag_detailpara);
						}
					}
					break;
				case Cmd_Type_anccfg:
				case Cmd_Type_tagcfg:
				case Cmd_Type_Firmware_update:
					{
						// 透传给UWB
						PC_Uart_sendCmd_Dma(uwb_rx_flag.buf, uwb_rx_flag.total_len);
					}
					break;
					
				default:
					break;
			}
		}
	}
}

int UWB_Process_USART_SendCMD(void)
{
	if(uwb_tx_flag.uart_dma_index != 0)
	{
		UWB_Uart_sendCmd_Dma(NULL, 0);
	}
}


int UWB_Init(void)
{
	memset(&uwb_rx_flag, 0, sizeof(Rx_Uart_Flag_t));
	osal_CreateQueue(&uwb_rx_queue, QUEUE_MSG_MAX);
//	HalUwbSet(true);

	USART_Cmd(USART3, ENABLE);
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);	
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);							//DMA接收缓冲区满中断使能
	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
}

/*
 * 功能@1.接受串口数据
 *     @2.发送串口数据
 */
int UWB_task(void)
{
	// ①接受串口数据
	UWB_Process_USART_RecvCMD();

	// ②发送串口数据
	UWB_Process_USART_SendCMD();
}

