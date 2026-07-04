#include <tag.h>


#include <circ_buf.h>
#include <assert.h>
#include "config.h"
#include "cmd_fn.h"
#include "Generic.h"
#include "hal_key.h"
#include "ose_uwb.h"
#include "ose_app.h"
#include "ose_device.h"
static uint32_t test_mid, test_all, test_s, test_e;


/* 128-bit STS key */
static  ose_sts_key_t   sts_key = {0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F};
/* 128-bit STS IV */
static  ose_sts_iv_t    sts_iv  = {0};


// OSE无线参数
static cmd_config_tx_rx_req_t  config_txrx_para = {
    .control_bits = COFIG_BITMAP1 |COFIG_BITMAP2| COFIG_BITMAP3, /* control bits */
    .param = {
        0x01,         /* radio_setting_index */
        0,            /* dbb_params */
        0             /* rf_params */
    }
};
static twr_info_t    TwrInfo;
static uint8_t       test_flag;
//-----------------------------------------------------------------------------

/*********************************************************************************
 *
 *实现代码一：
 * 发送blink数据包有4种情况
 *  1)【1.1场景】blink发送失败
 *  2)【1.2场景】blink发送成功,开启一次接受,接受超时(进入休眠模式[TA_TXE_WAIT])
 *  3)【1.3场景】blink发送成功,开启一次接受,接受错误(进入休眠模式[TA_TXE_WAIT])
 *  4)【1.4场景】blink发送成功,开启一次接受,接受成功,但解析错误(进入休眠模式[TA_TXE_WAIT])
 *  5)【1.5场景】blink发送成功,开启一次接受,接受成功,解析成功(进入休眠模式[TA_TXE_WAIT])
 *
 * 发送poll数据包有4种情况
 *  1)【2.1场景】poll发送失败
 *  2)【2.2场景】poll发送成功,开启四次接受,接受超时
 *  3)【2.3场景】poll发送成功,开启四次接受,poll接受错误
 *  4)【2.4场景】poll接受成功,开启四次接受,但解析错误
 *  5)【2.5场景】poll接受成功,开启四次接受,解析成功
 *  6)【特殊场景】查看注①/注②
 *
 * 发送final数据包有2种情况
 *  1)【3.1场景】final发送失败(进入休眠模式[TA_TXE_WAIT])
 *  2)【3.2场景】final发送成功(进入休眠模式[TA_TXE_WAIT])
 *
 *
 *
 *实现代码二：
 * 时分多址
 *	1)接受rngcfg数据包时,进行时间校准
 *	2)发送final数据包时,进行时间校准
 *
 *
 *实现代码三：
 * 时钟同步
 *	1)基站接受到Poll数据包,记录下当前的时间
 *	2)标签发送Final数据包,统计当前的时间
 *
 *
 *
 *
 *********************************************************************************/




/*
 * 获取参数
 *
 */
twr_info_t * getTwrInfoPtr(void)
{
	return (&TwrInfo);
}


/*
 *  标签接受Resp超时设置
 *
 */
void tag_received_timeout_set(twr_info_t *pTwrInfo, uint32_t  timeout_us)
{
	pTwrInfo->testAppState = TA_RX_WAIT_DATA;
	ose_settrxdelay(OSE_RX_TIMEOUT_M,  timeout_us);
	int16_t state = ose_rxenable(OSE_START_RX_IMMEDIATE, 0);

	if(state == UWB_HCI_FAIL)
	{
		port_tx_msg("uwb_hci_fail\n", strlen("uwb_hci_fail\n"));
	}
}


/*
 * 发送函数
 *
 */
static error_e tx_start(tx_pckt_t * pTxPckt)
{
//	bool ret = DWT_SUCCESS;
//	uint8_t  txFlag = 0;

//	dwt_forcetrxoff();    //Stop the Receiver and Write Control and Data

//	/* 写入发送数据 */                        
//	dwt_writetxdata(pTxPckt->txDataLen, (uint8 *)&pTxPckt->arr, 0) ; 
//	/* 发送数据长度 */
//	dwt_writetxfctrl(pTxPckt->txDataLen, 0, 1);

//	//Setup for delayed Transmit
//	if(pTxPckt->delayedTxTimeH_sy != 0UL)
//	{
//		dwt_setdelayedtrxtime(pTxPckt->delayedTxTimeH_sy) ;
//	}

//	if(pTxPckt->txFlag & DWT_RESPONSE_EXPECTED)
//	{
//		dwt_setrxaftertxdelay(pTxPckt->delayedRxTime_sy);
//		dwt_setrxtimeout(pTxPckt->delayedRxTimeout_sy);
//	}

//	// Begin delayed TX of frame
//	txFlag = (pTxPckt->delayedTxTimeH_sy != 0UL) | (pTxPckt->txFlag);

//	if(dwt_starttx(txFlag) != DWT_SUCCESS)
//	{
//		ret = DWT_ERROR;
//	}

//	return (ret);
}

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------


/*
 * 节点接受blink数据包
 *
 */
void prepare_twr_blink_msg(tx_pckt_t *pTxPckt, twr_info_t *pTwrInfo)
{
	// 填充blink数据
	{
		blink_msg_t *pTxMsg = (blink_msg_t *)pTxPckt->arr;

		pTxMsg->mac.frameCtrl             = FRAME_CTRL;
		pTxMsg->mac.seqNum                = pTxPckt->seqNum;
		pTxMsg->mac.sourceAddr            = 0xffff;
		pTxMsg->mac.destAddr              = 0xffff;
		pTxMsg->mac.panID                 = 0xffff;

		pTxMsg->blink.fCode               = TWR_BLINK_MSG_FN;
		memcpy(&pTxMsg->blink.tagId, &pTwrInfo->eui64, 8);
	}
	pTxPckt->txDataLen              = sizeof(blink_msg_t);
	pTxPckt->txState                = Twr_Tx_Blink_Sent;
	pTxPckt->is_send                = false;
	pTxPckt->seqNum                 = (pTxPckt->seqNum+1);;
}


/*
 * 节点发送rngcfg数据包
 *
 */
void prepare_twr_rngcfg_msg(rx_pckt_t  *pRxPckt, twr_info_t *pTwrInfo)
{
}

/*
 * 节点接受poll数据包
 *
 */
void prepare_twr_poll_msg(tx_pckt_t *pTxPckt, twr_info_t *pTwrInfo)
{
	// 填充poll数据
	{
		poll_msg_t *pTxMsg = (poll_msg_t *)pTxPckt->arr;

		pTxMsg->mac.frameCtrl             = FRAME_CTRL;
		pTxMsg->mac.seqNum                = pTxPckt->seqNum;
		pTxMsg->mac.sourceAddr            = pTwrInfo->twr_tag.general.tagAddr;
		pTxMsg->mac.destAddr              = 0xffff;
		pTxMsg->mac.panID                 = pTwrInfo->twr_tag.general.panID;

		pTxMsg->poll.fCode                = TWR_POLL_MSG_FN;
		pTxMsg->poll.rNum                 = ++pTwrInfo->twr_tag.measure.twr_sn;
		pTxMsg->poll.t2a_sensor           = App_Module_Get_SysState_Sensor();
	}
	pTxPckt->txDataLen              = sizeof(poll_msg_t);
	pTxPckt->txState                = Twr_Tx_Poll_Sent;
	pTxPckt->is_send                = false;
	pTxPckt->seqNum                 = (pTxPckt->seqNum+1);;
//	pTxPckt->txFlag                 = (DWT_START_TX_IMMEDIATE);
//	pTxPckt->delayedTxTimeH_sy      = 0;
//	pTxPckt->delayedRxTime_sy       = 0;
//	pTxPckt->delayedRxTimeout_sy    = 0;
}


/*
 * 节点发送resp数据包
 *
 */
void prepare_twr_resp_msg(rx_pckt_t *pRxPckt, twr_info_t *pTwrInfo)
{

}



/*
 * 节点接受final数据包
 *
 */
void prepare_twr_final_msg(tx_pckt_t *pTxPckt, twr_info_t *pTwrInfo)
{
	// 填充final数据
	{
		final_msg_t *pTxMsg = (final_msg_t *)pTxPckt->arr;

		pTxMsg->mac.frameCtrl             = FRAME_CTRL;
		pTxMsg->mac.seqNum                = pTxPckt->seqNum;
		pTxMsg->mac.sourceAddr            = pTwrInfo->twr_tag.general.tagAddr;
		pTxMsg->mac.destAddr              = 0xffff;
		pTxMsg->mac.panID                 = pTwrInfo->twr_tag.general.panID;

		pTxMsg->final.fCode               = TWR_FINAL_MSG_FN;
		pTxMsg->final.rNum                = pTwrInfo->twr_tag.measure.twr_sn;
		pTxMsg->final.pollTx_ts           = pTwrInfo->twr_tag.measure.twr_timestamp.tagPollTxTime;
		pTxMsg->final.finalTx_ts          = ose_merge_timestamp(pTwrInfo->twr_tag.measure.twr_timestamp.tagPollTxTime, pTwrInfo->twr_tag.env.tag_pollTxFinalTx_us);
		pTxMsg->final.rxResponseMask      = pTwrInfo->twr_tag.measure.rxResponseMask;
		pTxMsg->final.tag_acc.x           = sys_state.para_tag.tag_acc_x;
		pTxMsg->final.tag_acc.y           = sys_state.para_tag.tag_acc_y;
		pTxMsg->final.tag_acc.z           = sys_state.para_tag.tag_acc_z;
		pTxMsg->final.tag_gcc.x           = 0;
		pTxMsg->final.tag_gcc.y           = 0;
		pTxMsg->final.tag_gcc.z           = 0;
	}
	pTxPckt->txDataLen              = sizeof(final_msg_t);
	pTxPckt->txState                = Twr_Tx_Final_Sent;
	pTxPckt->is_send                = false;
	pTxPckt->seqNum                 = (pTxPckt->seqNum+1);
}



void twr_uwb_process(rx_pckt_t *pRxPckt, twr_info_t *pTwrInfo)
{
	uint8_t fCode = pRxPckt->arr[MSG_FN_IDX];
	// 标签已绑定
	if(pTwrInfo->twr_tag.general.is_bind == true)
	{
		//【2.5场景】
		if(fCode == TWR_RESP_MSG_FN && pRxPckt->rxDataLen == sizeof(resp_msg_t))
		{
			resp_msg_t *pRxMsg = (resp_msg_t *)pRxPckt->arr;
			if(pRxMsg->mac.destAddr == pTwrInfo->twr_tag.general.tagAddr &&
				pRxMsg->resp.rNum == pTwrInfo->twr_tag.measure.twr_sn)
			{
				tx_pckt_t *pTxPckt = &pTwrInfo->txPcktBuf.buf;
				uint8_t anc_sAddr = (pRxMsg->mac.sourceAddr % MAX_ANCHOR_LIST_SIZE);
				if(pRxMsg->mac.sourceAddr == GATEWAY_ADDR){
					pTwrInfo->twr_tag.measure.slotCorr_ms = 0;//pRxMsg->resp.slotCorr_ms;
					pTwrInfo->twr_tag.measure.a2t_usercmd = pRxMsg->resp.a2t_usercmd;
				}
				pTwrInfo->twr_tag.measure.rxResponseMask |= (1 << anc_sAddr);
				pTwrInfo->twr_tag.measure.twr_timestamp.tagRespRxTime[anc_sAddr] = pRxPckt->r_timeStamp;
				final_msg_t *pTxMsg = (final_msg_t *)pTxPckt->arr;
				pTxMsg->final.responseRx_ts[anc_sAddr] = pRxPckt->r_timeStamp;
				pTxMsg->final.tof[anc_sAddr].angle = pRxMsg->resp.last_tof.angle;
				pTxMsg->final.tof[anc_sAddr].dist  = pRxMsg->resp.last_tof.dist;
			}

			if(pRxMsg->mac.sourceAddr == 0)
			{port_tx_msg("resp A0\n", strlen("resp A0\n"));}
			else if(pRxMsg->mac.sourceAddr == 1)
			{port_tx_msg("resp A1\n", strlen("resp A0\n"));}
			else if(pRxMsg->mac.sourceAddr == 2)
			{port_tx_msg("resp A2\n", strlen("resp A0\n"));}
			else if(pRxMsg->mac.sourceAddr == 3)
			{port_tx_msg("resp A3\n", strlen("resp A0\n"));}
		}
		//【2.4场景】
		else
		{
		}


		// 注②：如果为最后一次打开接受且有数据收到，则发送final数据包
		if(pTwrInfo->twr_tag.measure.remainingRespToRx == -1 && pTwrInfo->txPcktBuf.buf.txState == Twr_Tx_Poll_Sent)
		{
			pTwrInfo->testAppState = TA_TXFINAL_WAIT_SEND;
		}
	}
	// 标签未绑定
	else
	{
	    //【1.5场景】
		if(fCode == TWR_RNGCFG_MSG_FN && pRxPckt->rxDataLen == sizeof(rngcfg_msg_t))
		{
			rngcfg_msg_t *pRxMsg = (rngcfg_msg_t *)pRxPckt->arr;
			if(pRxMsg->rngcfg.tagId == pTwrInfo->eui64)
			{
				pTwrInfo->twr_tag.general.is_bind           = true;
				pTwrInfo->twr_tag.general.tagAddr           = pRxMsg->rngcfg.tag_saddr;
				pTwrInfo->twr_tag.general.panID             = pRxMsg->mac.panID;

				pTwrInfo->twr_tag.env.sframePeriod_ms       = pRxMsg->rngcfg.sframePeriod_ms;
				pTwrInfo->twr_tag.env.tag_pollTxFinalTx_us  = pRxMsg->rngcfg.pollTxToFinalTx_us;
				pTwrInfo->twr_tag.env.tag_respRxTimeout_us  = pRxMsg->rngcfg.rxRespRxTimeout_us;
				pTwrInfo->twr_tag.env.tag_respRxDelay_us    = pRxMsg->rngcfg.rxRespDlyRx_us;

				pTwrInfo->twr_tag.measure.twr_sn            = 0;
				pTwrInfo->twr_tag.measure.faultyRangesCnt   = 0;
				pTwrInfo->twr_tag.measure.remainingRespToRx = 0;
				pTwrInfo->twr_tag.measure.rxResponseMask    = 0;
				pTwrInfo->twr_tag.measure.slotCorr_ms       = 0;//(pRxMsg->rngcfg.slotCorr_valid)?(pRxMsg->rngcfg.slotCorr_ms):(0);	

				
				_dbg_printf("标签绑定成功, 短地址:%04X, 超级帧周期:%d,%d, 时间校准:%d, tag_respRxTimeout_us:%d, tag_respRxDelay_us:%d\n", 
						pTwrInfo->twr_tag.general.tagAddr,
						pTwrInfo->twr_tag.env.sframePeriod_ms,
						pTwrInfo->twr_tag.env.tag_pollTxFinalTx_us,
						pTwrInfo->twr_tag.measure.slotCorr_ms,
						pTwrInfo->twr_tag.env.tag_respRxTimeout_us,
						pTwrInfo->twr_tag.env.tag_respRxDelay_us);
			}
		}
		//【1.4场景】
		else
		{
		}
		pTwrInfo->testAppState = TA_TXE_WAIT;
	}
}
/**
 * @brief tx_ok_cb 发送成功回调函数
 * @param cb_data 回调参数
 */
static void tx_ok_cb(const ose_cb_data_t* cb_data)
{
	test_s = get_1us_tick_value();
//	uint8_t buf[64];
//	memset(buf, 0, sizeof(buf));
//	sprintf(buf, "tx_ok_cb:%d\n", get_1us_tick_value());
//	port_tx_msg(buf, strlen(buf));

	twr_info_t *pTwrInfo = getTwrInfoPtr();
	tx_pckt_t  *pTxPckt  = &pTwrInfo->txPcktBuf.buf;

	pTxPckt->t_timeStamp = cb_data->tx_done_time;
	pTxPckt->local_tickstamp_us = get_1us_tick_value();
	pTxPckt->is_send = true;

	
//	if(pTxPckt->txState == Twr_Tx_Poll_Sent)
//	{
//		pTwrInfo->twr_tag.measure.remainingRespToRx = MAX_ANCHOR_LIST_SIZE;   
//		pTwrInfo->twr_tag.measure.rxResponseMask = 0;
//		tag_received_timeout_set(pTwrInfo, false);  
//	}
}

/**
 * @brief rx_ok_cb 接受成功回调函数
 * @param cb_data 回调参数
 */
static void rx_ok_cb(const ose_cb_data_t* cb_data)
{
	port_tx_msg("rx_ok_cb\n", strlen("rx_ok_cb\n"));

	twr_info_t *pTwrInfo = getTwrInfoPtr();

	const int   size = sizeof(pTwrInfo->rxPcktBuf.buf) / sizeof(pTwrInfo->rxPcktBuf.buf[0]);
	int head = pTwrInfo->rxPcktBuf.head;
	int tail = pTwrInfo->rxPcktBuf.tail;

	
	/* 是否存在空位置 */
	if(CIRC_SPACE(head,tail,size) <= 0)
	{
		pTwrInfo->testAppState = TA_RXE_WAIT;
		return;
	}

	rx_pckt_t  *pRxPckt  = &pTwrInfo->rxPcktBuf.buf[head];
	pRxPckt->r_timeStamp    = cb_data->rx_ok_time;
	pRxPckt->local_tickstamp_us = get_1us_tick_value();
	pRxPckt->rxDataLen      = cb_data->rxdatalength;
	memcpy(pRxPckt->arr, cb_data->rxdata, pRxPckt->rxDataLen);

	/* 更改静态变量，用于处理 */
	head = (head + 1) & (size-1);
	pTwrInfo->rxPcktBuf.head = head;

//	if(pTwrInfo->txPcktBuf.buf.txState == Twr_Tx_Poll_Sent)
//	{
//		int sn = (MAX_ANCHOR_LIST_SIZE - pTwrInfo->twr_tag.measure.remainingRespToRx) - 1;
//		test_flag |= (0x01 << sn);
//		tag_received_timeout_set(pTwrInfo, false);
//	}
}

/**
 * @brief rx_to_cb 接受超时回调函数
 * @param cb_data 回调参数
 */
static void rx_to_cb(const ose_cb_data_t* cb_data)
{
//	uint32_t buf[64];
//	memset(buf, 0, sizeof(buf));
//	sprintf(buf, "rx_to_cb:%d\n", get_1us_tick_value() - test_s);
//	port_tx_msg(buf, strlen(buf));
	port_tx_msg("rx_to_cb\n", strlen("rx_to_cb\n"));

	twr_info_t *pTwrInfo = getTwrInfoPtr();
	if(pTwrInfo == NULL)
	{
		return;
	}

	if(pTwrInfo->txPcktBuf.buf.txState == Twr_Tx_Blink_Sent)
	{
		port_tx_msg("blink rx timeout\n", strlen("blink rx timeout\n"));
		//【1.2场景】/【1.3场景】
		pTwrInfo->testAppState = TA_TXE_WAIT;
	}
	else if(pTwrInfo->txPcktBuf.buf.txState == Twr_Tx_Poll_Sent)
	{
		//【2.2场景】/【2.3场景】
//		tag_received_timeout_set(pTwrInfo, true);
	}

}

/**
 * @brief tx_fail_cb 发送失败回调函数
 * @param cb_data 回调参数
 */
static void tx_fail_cb(const ose_cb_data_t* cb_data)
{
	port_tx_msg("tx_fail_cb\n", strlen("rx_fail_cb\n"));
//	_dbg_printf("tx_fail_cb\r\n");
}

/**
 * @brief rx_fail_cb 接受失败回调函数
 * @param cb_data 回调参数
 */
static void rx_fail_cb(const ose_cb_data_t* cb_data)
{
	port_tx_msg("rx_fail_cb\n", strlen("rx_fail_cb\n"));

	rx_to_cb(cb_data);
}

/**
 * @brief crc_fail_cb crc校验失败回调函数
 * @param cb_data 回调参数
 */
static void crc_fail_cb(const ose_cb_data_t* cb_data)
{
	port_tx_msg("crc_fail_cb\n", strlen("crc_fail_cb\n"));
	rx_to_cb(cb_data);
//	_dbg_printf("crc_fail_cb\r\n");
}




/**
 * 应用程序主循环
 *
 * */
int testapprun(twr_info_t *pTwrInfo)
{
	param_block_t *pbss = get_pbssConfig();
	switch (pTwrInfo->testAppState)
	{
		case TA_INIT :
			{
				pTwrInfo->eui64 = ChipID;
				
				pTwrInfo->twr_tag.general.is_bind = false;
				pTwrInfo->twr_tag.general.tagAddr = 0;
				pTwrInfo->twr_tag.general.panID   = 0;
				pTwrInfo->twr_tag.measure.twr_sn  = 0;
				pTwrInfo->twr_tag.measure.faultyRangesCnt  = 0;
				pTwrInfo->twr_tag.measure.remainingRespToRx = 0;
				pTwrInfo->twr_tag.measure.rxResponseMask = 0;
				pTwrInfo->twr_tag.measure.slotCorr_ms = 0;
				

				ose_set_tx_rx_antennadelay(pbss->s.baseConfig.antTx, pbss->s.baseConfig.antRx);		// 设置天线接受&发送延时
				ose_set_device_info(OSE_DEVICE_INFO_ALL, 0, 0, 0);
				ose_configureframefilter(OSE_FF_DISABLE,OSE_FF_DATA_EN|OSE_FF_PANID_EN);

				pTwrInfo->testAppState = (pTwrInfo->twr_tag.general.is_bind)?(TA_TXPOLL_WAIT_SEND):(TA_TXBLINK_WAIT_SEND);
			}
			break;

		case TA_SLEEP_DONE :
			{
				if(pTwrInfo->twr_tag.machine_period.event != DWT_SIG_SLEEP_TIMEOUT)
				{
					pTwrInfo->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
					break;
				}

//#if (DEEP_SLEEP == 1 && WORK_SLEEP == 1)
//				uint32_t dwWakeup_starttime = portGetTickCnt();
//				port_wakeup_dw1000_fast();
//				dwt_setlnapamode( (pbss->s.userConfig.device_pa)?(/*DWT_LNA_ENABLE|*/DWT_PA_ENABLE):(DWT_LNA_PA_DISABLE) );
//				dwt_setfinegraintxseq( (pbss->s.userConfig.device_pa)?(0):(1));
//				dwt_settxantennadelay(app.pConfig->s.baseConfig.antTx); //Tx antenna delay is not preserved during sleep
//				pTwrInfo->twr_tag.machine_period.dwWakeUpTimeCorr_ms = portGetTickCnt() - dwWakeup_starttime;
////				uint8_t buf[64];
////				memset(buf, 0, sizeof(buf));
////				sprintf(buf, "wakeup success %04X, dwWakeUpTimeCorr_ms:%d\n",dwt_readdevid(), pTwrInfo->twr_tag.machine_period.dwWakeUpTimeCorr_ms);
////				port_tx_msg(buf, strlen(buf));
//#endif

				// 休眠清除
				pTwrInfo->twr_tag.machine_period.event = 0;
				pTwrInfo->twr_tag.machine_period.DwCanSleep = false;
				
				pTwrInfo->done = INST_NOT_DONE_YET;
				pTwrInfo->testAppState = (pTwrInfo->twr_tag.general.is_bind)?(TA_TXPOLL_WAIT_SEND):(TA_TXBLINK_WAIT_SEND);
			}
			break;

		case TA_TXBLINK_WAIT_SEND :
			{
				tx_pckt_t *pTxPckt = &pTwrInfo->txPcktBuf.buf;

				uint32_t time_s = portGetTickCnt();
				prepare_twr_blink_msg(pTxPckt, pTwrInfo);
				// 立即发送[使用滴答时钟监测,消耗1毫秒内]
				int16_t state = ose_starttx(OSE_START_TX_IMMEDIATE,(uint8_t*)&pTxPckt->arr, sizeof(blink_msg_t), 50);
				if(state == UWB_TX_OK)
				{
//					port_tx_msg("send blink ok\n", strlen("send blink ok\n"));
					pTwrInfo->testAppState = TA_TX_WAIT_CONF;
				}
				else
				{
					//【1.1场景】
					pTwrInfo->testAppState = TA_TXE_WAIT;
					uint8_t buf[32];
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "send blink fault:%d, :%d\n", state, get_1us_tick_value());
					port_tx_msg(buf, strlen(buf));
				}
			}
			break;
			
		case TA_TXPOLL_WAIT_SEND:
			{
				tx_pckt_t *pTxPckt = &pTwrInfo->txPcktBuf.buf;
				uint8_t buf[64];
				memset(buf, 0, sizeof(buf));
				sprintf(buf, "send poll:%d, %d, %d\n", test_mid, test_all, test_e - test_s);
				port_tx_msg(buf, strlen(buf));
//				port_tx_msg("send poll\n", strlen("send poll\n"));
				prepare_twr_poll_msg(pTxPckt, pTwrInfo);
				// 立即发送[使用滴答时钟监测,消耗1毫秒内]

				
				uint32_t time_s = get_1us_tick_value();
				int16_t state = ose_starttx(OSE_START_TX_IMMEDIATE,(uint8_t*)&pTxPckt->arr, sizeof(poll_msg_t), 50);	//消耗618us
				if(state == UWB_TX_OK)
				{
					uint32_t time_m = get_1us_tick_value();
//					uint8_t buf[64];
//					memset(buf, 0, sizeof(buf));
//					sprintf(buf, "send poll ok:%d\n", get_1us_tick_value());											//消耗300us
//					port_tx_msg(buf, strlen(buf));

					ose_settrxdelay(OSE_RX_TIMEOUT_M, 1000);											//2us
					ose_rxenable(OSE_START_RX_IMMEDIATE, 1000);											//立即开启接受
					pTwrInfo->testAppState = TA_TX_WAIT_CONF;
				}
				else
				{
					//【2.1场景】
					pTwrInfo->testAppState = TA_TXE_WAIT;
					uint8_t buf[32];
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "send poll fault:%d, :%d\n", state, get_1us_tick_value());
					port_tx_msg(buf, strlen(buf));
				}

//				test_flag = 0;
			}
			break;
			
			
		case TA_TXFINAL_WAIT_SEND:
			{    
				//【3.1场景】
				if(pTwrInfo->twr_tag.measure.rxResponseMask == 0)
				{
					uint8_t buf[32];
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "Invalid rxResonseMask:%d\n", pTwrInfo->twr_tag.measure.twr_sn);
					port_tx_msg(buf, strlen(buf));
					pTwrInfo->twr_tag.measure.faultyRangesCnt++;
					pTwrInfo->twr_tag.general.is_bind = (pTwrInfo->twr_tag.measure.faultyRangesCnt > app.pConfig->s.userConfig.faultyRanges)?(false):(true);
					pTwrInfo->testAppState = TA_TXE_WAIT;
					break;
				}
				//【3.2场景】
				else
				{
					pTwrInfo->twr_tag.measure.faultyRangesCnt = 0;
				}

				tx_pckt_t *pTxPckt = &pTwrInfo->txPcktBuf.buf;
				
				prepare_twr_final_msg(pTxPckt, pTwrInfo);
				// 立即发送[使用滴答时钟监测,消耗1毫秒内]
				int16_t state = ose_starttx(OSE_START_TX_IMMEDIATE,(uint8_t*)&pTxPckt->arr, sizeof(final_msg_t), 50);
				if(state == UWB_TX_OK)
				{
					port_tx_msg("send final ok\n", strlen("send final ok\n"));
					pTwrInfo->testAppState = TA_TX_WAIT_CONF;
				}
				else
				{
					//【3.1场景】
					pTwrInfo->testAppState = TA_TXE_WAIT;
					port_tx_msg("send final fault\n", strlen("send final fault\n"));
				}
			}
			break;
			
		case TA_TX_WAIT_CONF:
			{
				if(pTwrInfo->txPcktBuf.buf.is_send == true)
				{
//					port_tx_msg("is_send\n", strlen("is_send\n"));
					pTwrInfo->txPcktBuf.buf.is_send = false;
					switch(pTwrInfo->txPcktBuf.buf.txState)
					{
						case Twr_Tx_Blink_Sent:
							{
								// 立即启动接受,并且接受超时RC_RX_TIMEOUT_US,
								tag_received_timeout_set(pTwrInfo, RC_RX_TIMEOUT_US);
							}
							break;
						case Twr_Tx_Poll_Sent:
							{
								pTwrInfo->twr_tag.measure.remainingRespToRx = MAX_ANCHOR_LIST_SIZE;   
								pTwrInfo->twr_tag.measure.rxResponseMask = 0;
								tag_received_timeout_set(pTwrInfo, pTwrInfo->twr_tag.measure.remainingRespToRx*pTwrInfo->twr_tag.env.tag_respRxDelay_us); 
							}
							break;
						case Twr_Tx_Final_Sent:
							{
								// 清除发送数据包
								memset(&((final_msg_t*)pTwrInfo->txPcktBuf.buf.arr)->final, 0, sizeof(final_t));
								
								//切换成休眠模式
//								uint8_t buf[64];
//								memset(buf, 0, sizeof(buf));
//								sprintf(buf, "发送final数据包成功:%02X\n", test_flag);
//								port_tx_msg(buf, strlen(buf));  
								pTwrInfo->testAppState = TA_TXE_WAIT;
							}
							break;
						default:
							break;
					}
				}
				pTwrInfo->done = INST_NOT_DONE_YET;
			}
			break;
        
		case TA_TXE_WAIT:
			{
//#if (DEEP_SLEEP == 1 && WORK_SLEEP == 1)
////				dwt_entersleep();
//				pTwrInfo->twr_tag.general.dw_sleep = true;

////				port_tx_msg("dw3000进入休眠模式\n", strlen("dw3000进入休眠模式\n"));
//#endif
				port_tx_msg("TA_TXE_WAIT\n", strlen("TA_TXE_WAIT\n"));
				pTwrInfo->testAppState = TA_SLEEP_DONE;
				pTwrInfo->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO; //don't sleep here but kick off the Sleep timer countdown
				pTwrInfo->twr_tag.machine_period.DwCanSleep = true;
			}
			break;
		
		case TA_RX_WAIT_DATA:
			{
				App_Module_Sys_Work_Mode_Event();
				int head, tail, size;
				size = sizeof(pTwrInfo->rxPcktBuf.buf) / sizeof(pTwrInfo->rxPcktBuf.buf[0]);
				head = pTwrInfo->rxPcktBuf.head;
				tail = pTwrInfo->rxPcktBuf.tail;
				if(CIRC_CNT(head,tail,size) > 0)
				{
					//twr基站数据处理 
					twr_uwb_process(&pTwrInfo->rxPcktBuf.buf[tail], pTwrInfo);
					tail = (tail + 1) & (size-1);
					pTwrInfo->rxPcktBuf.tail = tail;
					
					pTwrInfo->testAppState = TA_TXE_WAIT;
				}
				// 如果大于10ms,则异常退出
	//			else if(get_1us_tick_value() - pTwrInfo->txPcktBuf.buf.local_tickstamp_us > 10000)
	//			{
	//				pTwrInfo->testAppState = TA_TXE_WAIT;
	//				port_tx_msg("rx longtime error\n", strlen("rx longtime error\n"));
	//			}
			}
			break;
		default:
			break;
	}

	return pTwrInfo->done;
}


void instance_run(void)
{
	twr_info_t  *pTwrInfo = getTwrInfoPtr();
	int done = pTwrInfo->done = INST_NOT_DONE_YET;
	
	while(done == INST_NOT_DONE_YET)
	{
		done = testapprun(pTwrInfo);
	}

	/* 发送Blink完成标志并进入休眠模式，此时开启定时器 */
	if(done == INST_DONE_WAIT_FOR_NEXT_EVENT_TO)
	{
	
		pTwrInfo->twr_tag.machine_period.interval_in_ms = (pTwrInfo->twr_tag.general.is_bind)?(pTwrInfo->twr_tag.measure.slotCorr_ms + pTwrInfo->twr_tag.env.sframePeriod_ms):(1000+rand()%5);
		
		// ①进入休眠时使用
		pTwrInfo->twr_tag.machine_period.interval_in_ms = (pTwrInfo->twr_tag.machine_period.interval_in_ms > pTwrInfo->twr_tag.machine_period.dwWakeUpTimeCorr_ms)?
																(pTwrInfo->twr_tag.machine_period.interval_in_ms - pTwrInfo->twr_tag.machine_period.dwWakeUpTimeCorr_ms):
																(pTwrInfo->twr_tag.machine_period.interval_in_ms);
		// ②全速工作时使用
	//	pTwrInfo->twr_tag.machine_period.nextWakeUpTime_ms = pTwrInfo->twr_tag.machine_period.preWakeUpTime_ms +  pTwrInfo->twr_tag.env.sframePeriod_ms/* + pTwrInfo->twr_tag.machine_period.interval_in_ms*/;
		pTwrInfo->twr_tag.machine_period.nextWakeUpTime_ms = pTwrInfo->twr_tag.machine_period.preWakeUpTime_ms +  100/* + pTwrInfo->twr_tag.machine_period.interval_in_ms*/;
		pTwrInfo->twr_tag.machine_period.timeron = true;
		pTwrInfo->twr_tag.measure.slotCorr_ms = 0;
	}

	if(pTwrInfo->twr_tag.machine_period.timeron == true)
	{
//		_dbg_printf("进入休眠:%d, nextWakeUpTime_ms:%d, dwWakeUpTimeCorr_ms:%d, portGetTickCount:%d\n", 
//							pTwrInfo->twr_tag.machine_period.interval_in_ms, 
//							pTwrInfo->twr_tag.machine_period.nextWakeUpTime_ms, 
//							pTwrInfo->twr_tag.machine_period.dwWakeUpTimeCorr_ms,
//							portGetTickCnt());

		// 充电时，stm32不休眠
		if(1/*sys_state.sys_work_mode >= Sys_Operate_Mode_USB_CONNET || 
			sys_state.system_alarm.alarm == true || 
			sys_state.system_turnoff == true ||
			!HalKey_Loose_Judge()*/)
		{
			if(pTwrInfo->twr_tag.machine_period.nextWakeUpTime_ms <= portGetTickCnt())
			{
				pTwrInfo->twr_tag.machine_period.event = DWT_SIG_SLEEP_TIMEOUT;
			}
		}
		else
		{
			HalPmu_Enter(pTwrInfo->twr_tag.machine_period.interval_in_ms, false, false);
			pTwrInfo->twr_tag.machine_period.event = DWT_SIG_SLEEP_TIMEOUT;
		}

		if(pTwrInfo->twr_tag.machine_period.event == DWT_SIG_SLEEP_TIMEOUT)
		{
//			_dbg_printf("退出休眠，准备工作\n");
			pTwrInfo->twr_tag.machine_period.timeron = false;
			pTwrInfo->twr_tag.machine_period.preWakeUpTime_ms = portGetTickCnt();
//			port_DisableEXT_IRQ(); 
//			dwt_forcetrxoff();				//disable DW1000
//			port_EnableEXT_IRQ();
		}
	}
}



void tag_pdoa_task(void)
{
	_dbg_printf("tag start\n");
	do{
		instance_run();
		App_Module_Sys_Work_Mode_Event();
	}while(1);
}

/**
 *
 * @brief tag_process_init U1节点初始化
 */
int tag_process_init()
{
	twr_info_t *pTwrInfo = getTwrInfoPtr();

	memset(pTwrInfo, 0 , sizeof(twr_info_t));
	pTwrInfo->testAppState = TA_INIT;							//状态配置

	disable_ose_irq();

	/* 配置低速SPI,2MHz */
	port_set_uwb_spi_slowrate();

	/* OSE使能 */
	if(ose_initialise(OSE_CRC_DISABLE) == OSE_ERROR)
	{
		_dbg_printf("Failed to initialize UWB %s at %d line.\r\n", __FUNCTION__, __LINE__);
	}
	
	/* OSE配置读取 */
	ose_txrx_para_t trx_para;									//UWB信道参数
	if (ose_get_txrx_para(&trx_para, GET_FROM_ROM, 0x02) == OSE_ERROR) {
		_dbg_printf("Failed to get configuration parameters %s at %d line.\r\n", __FUNCTION__, __LINE__);
	}
	
	/* 配置OSE信道 */
	/* STS mode1*/
	trx_para.dbb_params.sts_mode = 1;
	/* STS active segment length in one segment;0: 16;1: 32;2: 64;3: 128;4: 256 */
	trx_para.dbb_params.sts_sgm_length = 2;
	/* STS segment number;0: 1 segment;1: 2 segment*/
	trx_para.dbb_params.sts_sgm_num = 1;
	/* STS gap length;0: 512 chips;1: 1024 chips */
	trx_para.dbb_params.sts_gap_len = 0;
	/* cir accu time */
	trx_para.dbb_params.sts_cir_accu_num_trig = 24;
	
	/* Override the current configuration */
	config_txrx_para.param.dbb_params = trx_para.dbb_params;
	/* Transmit power configuration parameters: 0 - 120 */
	config_txrx_para.param.rf_params.tx_power = 45;
	/* Channel number: 5 or 9 */
	config_txrx_para.param.rf_params.uwb_channel = 9;
	/* Configure OSE IC.*/
	if (ose_configure(&config_txrx_para) == OSE_ERROR) {
		_dbg_printf("UWB configure failed %s at %d line.\r\n", __FUNCTION__, __LINE__);
	}

	_dbg_printf("rate:%d:%d\n", trx_para.dbb_params.date_rate, trx_para.dbb_params.phr_rate);
	
	/* Configure the STS key and IV value and  angle mode */
	ose_configurests(&sts_key, &sts_iv, NO_ANGLE);

	/* 配置中断方式.*/
	ose_setinterrupt(OSE_INT_TX_SUCCESS|OSE_INT_TX_FAIL|OSE_INT_RX_SUCCESS|OSE_INT_RX_FAIL|OSE_INT_RX_TIMEOUT, OSE_INT_ENABLE);
	/* 配置回调函数 */
	ose_setcallbacks(tx_ok_cb, rx_ok_cb, rx_to_cb, tx_fail_cb, rx_fail_cb, crc_fail_cb);
	/* 使能时钟 */
	if(ose_enter_soc_state(SYS_PLL_62M5) != OSE_SUCCESS) 
	{
		_dbg_printf("Failed to configure sysclk %s at %d line.\r\n", __FUNCTION__, __LINE__);
	}
	sleep_ms(10);
	/* 配置高速SPI */
	port_set_uwb_spi_fastrate();
}


void tag_helper(void)
{
	uwb_init();

	uwb_start();

	read_version();
	
	tag_process_init();

	enable_ose_irq();
}

void tag_task(void)
{
	load_bssConfig();	//读取flash

	tag_helper();		//设备初始化配置

	tag_pdoa_task();

	for(;;);
}
//-----------------------------------------------------------------------------

