#include "Generic.h"
#include "Generic_cmd.h"

#include "hal_usart.h"
#include "struct_def.h"
#include "OSAL.h"


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

int App_Module_format_build(Cmd_Type_e type, Cmd_Direct_e direct, uint64_t d_longaddr, uint8_t *buf, uint16_t buf_len, dev_e dev)
{
	int     send_Len = 1 + 2 + (1 + 1 + 8 + 8 + buf_len) + 1 + 1;
	uint8_t send_buf[1024];
	Msg_uart_t *msg = (Msg_uart_t *)send_buf;
	

	{
		msg->header = CMD_SOP;
		msg->length = 1 + 1 + 8 + 8 + buf_len;

		msg->cmd.s_laddr = DEVICE_CONSOLE;
		msg->cmd.d_laddr = d_longaddr;
		msg->cmd.type = type;
		msg->cmd.direct = direct;
		if(buf_len != 0){memcpy(msg->cmd.buf, buf, buf_len);}
	}
	
	send_buf[send_Len - 2]  = get_Xor_CRC((uint8_t *)msg, CMD_START_INX, msg->length);;
	send_buf[send_Len - 1] = CMD_FOOT;
	
	if(dev == DEV_PC)
	{
		PC_Uart_sendCmd_Dma(send_buf, send_Len);
	}
	else if(dev == DEV_UWB)
	{
		UWB_Uart_sendCmd_Dma(send_buf, send_Len);
	}
}

