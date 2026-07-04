#include "hal_iic.h"
#include "math.h"
#include "stdio.h"
#include "delay.h"
/*********************************************************模拟IIC 操作*********************************************************/
//初始化IIC
void IIC_Init(void)
{	//PB10 -- SCL; PB11 -- SDA
	GPIO_InitTypeDef GPIO_InitStructure;
	//RCC->APB2ENR|=1<<4;//先使能外设IO PORTC时钟 
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );	
	   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
 
	IIC_SCL=1;
	IIC_SDA=0;
}

//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT();     //sda线输出
	IIC_SDA=1;	  	  
	IIC_SCL=1;
	Delay_us(4);
 	IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	Delay_us(4);
	IIC_SCL=0;//钳住I2C总线，准备发送或接收数据 
}

//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT();//sda线输出
	IIC_SCL=0;
	IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	Delay_us(4);
	IIC_SCL=1; 
	IIC_SDA=1;//发送I2C总线结束信号
	Delay_us(4);							   	
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	SDA_IN();      //SDA设置为输入  
	IIC_SDA=1;Delay_us(2);	   
	IIC_SCL=1;Delay_us(2);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL=0;//时钟输出0 	   
	return 0;  
} 
//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=0;
	Delay_us(2);
	IIC_SCL=1;
	Delay_us(2);
	IIC_SCL=0;
}
//不产生ACK应答		    
void IIC_NAck(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=1;
	Delay_us(2);
	IIC_SCL=1;
	Delay_us(2);
	IIC_SCL=0;
}					 				     
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void IIC_Send_Byte(u8 txd)
{                        
  u8 t;   
	SDA_OUT(); 	    
	IIC_SCL=0;//拉低时钟开始数据传输
	for(t=0;t<8;t++)
	{              
		IIC_SDA=(txd&0x80)>>7;
		txd<<=1; 	  
		Delay_us(1);   //对TEA5767这三个延时都是必须的
		IIC_SCL=1;
		Delay_us(1); 
		IIC_SCL=0;	
		Delay_us(1);
	}	 
} 	    
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
u8 IIC_Read_Byte(void)
{
	unsigned char i,receive=0;
	SDA_IN();//SDA设置为输入
  for(i=0;i<8;i++ )
	{
    IIC_SCL=0; 
    Delay_us(1);//2
		IIC_SCL=1;
    receive<<=1;
    if(READ_SDA)receive++;   
		Delay_us(1); 
  }					  
	return receive;
}

uint8_t I2C_ByteWrite(uint8_t DEV_No,uint8_t data, uint8_t WriteAddr)
{
	uint8_t EORR_BIT,SEND_BIT=0;

	/* 第1步：发起I2C总线启动信号 */
	IIC_Start();
	
	/* 第2步：发起控制字节并等待ACK回应，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	IIC_Send_Byte(DEV_No | I2C_WR);	/* 此处是写指令 */
	if (IIC_Wait_Ack() != 0){EORR_BIT=1;goto cmd_fail;}

	/* 第2步：写入地址 */
	//单字节地址
	IIC_Send_Byte(WriteAddr);
	if (IIC_Wait_Ack() != 0){EORR_BIT=2;goto cmd_fail;}

	/* 第3步：发送数据 若是复数的数据 重复这一步骤即可 */
	 IIC_Send_Byte(data);
	if (IIC_Wait_Ack() != 0){EORR_BIT=3;goto cmd_fail;}
	
	/* 第4步：发送完毕结束IIC */
	IIC_Stop();
	return 1;
	
cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
	//_dbg_printf("EORR_BIT:%d\r\n",EORR_BIT);
	//FIFO_BIT=1;
    IIC_Stop();
	return 0;
}


uint8_t I2C_BufferRead(uint8_t DEV_No, uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead)
{
	uint8_t ret = 1;
	uint8_t EORR_BIT,SEND_BIT=0;
	
	/* 第1步：发起I2C总线启动信号 */
	IIC_Start();
	
	/* 第2步：发起控制字节（一般1字节），高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	IIC_Send_Byte(DEV_No | I2C_WR);	/* 此处是写指令 */
	if (IIC_Wait_Ack() != 0){EORR_BIT=1;goto cmd_fail;}//等待器件应答
	
	/* 第3步：发送字节寄存器地址 （当前1个字节）*/
	IIC_Send_Byte(ReadAddr);//发送寄存器地址
	if (IIC_Wait_Ack() != 0){EORR_BIT=3;goto cmd_fail;}//等待器件应答
//多字节地址
//	while(write_length)
//	{
//		if(write_length==1)
//		{//发送最后一个字节
//			IIC_Send_Byte(write_buffer[SEND_BIT]);
//			if (IIC_Wait_Ack() != 0){EORR_BIT=3;goto cmd_fail;}
//			//_dbg_printf("结束\r\n");
//		}
//		else
//		{//正在发送数据
//			IIC_Send_Byte(write_buffer[SEND_BIT]);
//			if (IIC_Wait_Ack() != 0){EORR_BIT=2;goto cmd_fail;}
//		}
//		SEND_BIT++;
//		write_length--;
//	}
	
	/* 第4步：为后续切换IIC读取做准备*/
	//IIC_Stop();
	
	/* 第5步：重新启动I2C总线。下面开始读取数据 */
	IIC_Start();
	
	/* 第6步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	IIC_Send_Byte(DEV_No | I2C_RD);	/* 此处是读指令 */
	if (IIC_Wait_Ack() != 0){EORR_BIT=4;goto cmd_fail;}//等待器件应答
	/* 第8步：读取数据 */
	while(NumByteToRead)
    {
		if(NumByteToRead==1)
		{
			*pBuffer = IIC_Read_Byte();
			IIC_NAck();	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
			//_dbg_printf("结束\r\n");
		}
		else
		{
			*pBuffer = IIC_Read_Byte();	/* 读1个字节 */
			IIC_Ack();	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
		}
		pBuffer++; 
		NumByteToRead--;
       // IIC_NAck();	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
    }
	
	/* 第9步：发送完毕结束IIC */
    IIC_Stop();
	return 1;
cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
    /* 发送I2C总线停止信号 */
	_dbg_printf("EORR_BIT:%d\r\n",EORR_BIT);
	//FIFO_BIT=1;
    IIC_Stop();
	return 0;
}
/*********************************************************模拟IIC 操作*********************************************************/


/*********************************************************硬件IIC操作*********************************************************/

/*******************************************************************************
* 函数名  : I2C_init
* 描述    : STM32硬件I2C初始化
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 初始化IIC2，
*******************************************************************************/
void I2C2_Configuration(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure; 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

	/*STM32F103C8T6芯片的硬件I2C2: PB10 -- SCL; PB11 -- SDA */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;//I2C必须开漏输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_DeInit(I2C2);//使用I2C2
	I2C_InitStructure.I2C_Mode = I2C_Mode_SMBusHost;//I2C_Mode_SMBusHost  I2C_Mode_I2C
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0xFA;//主机的I2C地址,随便写的
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 305000;//最高400K,超300K 程序卡死概率大

	I2C_Init(I2C2, &I2C_InitStructure);//将设置参数写入 IIC2
	I2C_Cmd(I2C2, ENABLE);//使能IIC2
	
	I2C_AcknowledgeConfig(I2C2, ENABLE);//允许应答模式
	//I2C2->CR1 |= 0x80; // 解除限速 ？？？？
	
}
/*******************************************************************************
* 函数名  : I2C_reset
* 描述    : STM32硬件I2C复位
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 1.重置IIC标志位与IO口 防止IIC卡死 
			2.I2CX 重置 IIC1 或IIC2
*******************************************************************************/
void I2C_reset(I2C_TypeDef* I2Cx)
{
	if(I2Cx== I2C2)
	{
		I2C_GenerateSTOP(I2C2, ENABLE);
		I2C_DeInit(I2C2);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,DISABLE);
		Delay_ms(1);
		I2C2_Configuration();
	}
//	else if(I2Cx== I2C1)
//	{
//		I2C_GenerateSTOP(I2C1, ENABLE);
//		I2C_DeInit(I2C1);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,DISABLE);
//		Delay_ms(1);
//		I2C1_Configuration();
//	}
}
/*******************************************************************************
* 函数名  : IIC_WaitEvent
* 描述    : IIC 超时与等待函数
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 根据IIC通讯步骤对IIC进行操作，IIC通讯超时重置IIC
*******************************************************************************/
uint8_t IIC_WaitEvent(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT)
{
	uint32_t Timeout = 10000;
	while (I2C_CheckEvent(I2Cx, I2C_EVENT) != SUCCESS)
	{
		if (Timeout-- == 0){I2C_reset(I2Cx); return 1;_dbg_printf("\r\n I2C_EVENT %x \r\n",I2C_EVENT);}
	}
	return 0;
}

/*******************************************************************************
* 函数名  ：I2C_ByteWrite
* 描述    ：按照地址写入单个数据
* 输入    ：1.I2Cx 确认哪个IIC
			2.DEV_No 目标ID
			3.WriteAddr 寄存地址
			4.data 具体数据
* 输出    : 无
* 返回    : 无
* 说明    : 无
*******************************************************************************/
void I2C_ByWrite(I2C_TypeDef* I2Cx,uint8_t DEV_No,uint8_t WriteAddr,uint8_t data)
{
	/* Send STRAT condition */
	I2C_GenerateSTART(I2Cx, ENABLE);

	/* Test on EV5 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT);  

	/* Send EEPROM address for write */
	I2C_Send7bitAddress(I2Cx, DEV_No, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

	/* Send the EEPROM's internal address to write to */
	I2C_SendData(I2Cx, WriteAddr);

	/* Test on EV8 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED);

	/* Send the byte to be written */
	I2C_SendData(I2Cx, data); 

	/* Test on EV8 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED);

	/* Send STOP condition */
	I2C_GenerateSTOP(I2Cx, ENABLE);
}
/*******************************************************************************
* 函数名  : I2C_BufferWrite
* 描述    : 按照地址写入数组
* 输入    : 1.I2Cx 确认哪个IIC
			2.DEV_No目标ID
			3.WriteAddr寄存地址
			4.pBuffer写入数据
			5.NumByteToWrite写入数据数量
* 输出    : 无
* 返回    : 无
* 说明    : 无
*******************************************************************************/
uint8_t I2C_BufferWrite(I2C_TypeDef* I2Cx,uint8_t DEV_No,uint8_t WriteAddr,uint8_t *pBuffer,uint8_t NumByteToWrite)
{
	int  rest_bit=0;
	//_dbg_printf("IIC Write busy %x\r\n",pBuffer);
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY))
	{
		rest_bit++;
		if(rest_bit>200000){I2C_reset(I2Cx);}
		_dbg_printf("IIC Write busy1\r\n");
	}
	/* Send START condition */
	I2C_GenerateSTART(I2Cx, ENABLE);

	/* Test on EV5 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT); 

	/* Send EEPROM address for write */
	I2C_Send7bitAddress(I2Cx, DEV_No, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);  

	/* Send the EEPROM's internal address to write to */    
	I2C_SendData(I2Cx, WriteAddr);  

	/* Test on EV8 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED);

	/* While there is data to be written */
	while(NumByteToWrite--)  
	{
		/* Send the current byte */
		I2C_SendData(I2Cx, *pBuffer); 

		/* Point to the next byte to be written */
		pBuffer++; 

		/* Test on EV8 and clear it */
		IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED);
	}

	/* Send STOP condition */
	I2C_GenerateSTOP(I2Cx, ENABLE);

	return 0;
}

/*******************************************************************************
* 函数名  : I2C_BufRead
* 描述    : 按照地址读取数据
* 输入    : 1.I2Cx 确认哪个IIC
			2.DEV_No目标ID
			3.ReadAddr寄存器地址
			4.pBuffer读取数据存放数组
			5.NumByteToRead读取的数据量
* 输出    : 无
* 返回    : 无
* 说明    : 无
*******************************************************************************/
void I2C_BufRead(I2C_TypeDef* I2Cx,uint8_t DEV_No, uint8_t ReadAddr,uint8_t* pBuffer, uint16_t NumByteToRead)
{
	int  rest_bit=0;
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY))
	{
		rest_bit++;
		if(rest_bit>200000){I2C_reset(I2Cx);}
		_dbg_printf("IIC Write busy2\r\n");
	}
	
//	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY))
//	{
//		_dbg_printf("IIC Read busy2\r\n");
//	}//有时会卡在这里
	/* Send START condition */
	I2C_GenerateSTART(I2Cx, ENABLE);

	/* Test on EV5 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT);

	/* Send EEPROM address for write */
	I2C_Send7bitAddress(I2Cx, DEV_No, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

	/* Clear EV6 by setting again the PE bit */
	I2C_Cmd(I2Cx, ENABLE);

	/* Send the EEPROM's internal address to write to */
	I2C_SendData(I2Cx, ReadAddr);  

	/* Test on EV8 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED);

	/* Send STRAT condition a second time */  
	I2C_GenerateSTART(I2Cx, ENABLE);

	/* Test on EV5 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT);

	/* Send EEPROM address for read */
	I2C_Send7bitAddress(I2Cx, DEV_No, I2C_Direction_Receiver);

	/* Test on EV6 and clear it */
	IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
	/* While there is data to be read */
	while(NumByteToRead)  
	{
		if(NumByteToRead == 1)
		{
			/* Disable Acknowledgement */
			I2C_AcknowledgeConfig(I2Cx, DISABLE);
			/* Send STOP Condition */
			I2C_GenerateSTOP(I2Cx, ENABLE);
		}
		/* Test on EV7 and clear it */
		if(IIC_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) == 0)  
		{
			/* Read a byte from the EEPROM */
			*pBuffer = I2C_ReceiveData(I2Cx);
			/* Point to the next location where the byte read will be saved */
			pBuffer++;
			/* Decrement the read bytes counter */
			NumByteToRead--;
		}
	}
	/* Enable Acknowledgement to be ready for another reception */
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
}
/*******************************************************************************
* 函数名  : I2C_READ_BUFFER
* 描述    : 按照地址读取数据
* 输入    : 1.SlaveAddr目标ID
			2.readAddr寄存器地址
			3.pBuffer读取数据存放数组
			4.NumByteToRead读取的数据量
* 输出    : 无
* 返回    : 无
* 说明    : 函数经过优化不会造成卡顿
*******************************************************************************/
int I2C_READ_BUFFER(u8 SlaveAddr,u8 readAddr,u8* pBuffer,u16 NumByteToRead)
{ 
	int I2C_TIME,I2C_Number=1000;
	I2C_TIME=0; 
	while((I2C_GetFlagStatus(I2C2,I2C_FLAG_BUSY)) && (I2C_TIME<=I2C_Number)){I2C_TIME++;}
	if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
	//	while(I2C_GetFlagStatus(I2C2,I2C_FLAG_BUSY));
	
	I2C_GenerateSTART(I2C2,ENABLE);I2C_TIME=0;//开启信号
	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_MODE_SELECT)&& (I2C_TIME<=I2C_Number)){I2C_TIME++;}	//清除 EV5
	if(I2C_TIME>I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_MODE_SELECT));	//清除 EV5
	
	I2C_Send7bitAddress(I2C2,SlaveAddr, I2C_Direction_Transmitter);I2C_TIME=0; //写入器件地址
//	Delay_us(50); //这里的延时很重要	
	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)&& (I2C_TIME<=I2C_Number)){I2C_TIME++;}//清除 EV6
	if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));//清除 EV6
	
	I2C_Cmd(I2C2,ENABLE);
	I2C_SendData(I2C2,readAddr);I2C_TIME=0;//发送读的地址
//	Delay_us(50); //这里的延时很重要	
	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_BYTE_TRANSMITTED)&& (I2C_TIME<=I2C_Number)){I2C_TIME++;} //清除 EV8
	if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); //清除 EV8
	
	I2C_GenerateSTART(I2C2,ENABLE);I2C_TIME=0; //开启信号
	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_MODE_SELECT)&& (I2C_TIME<=I2C_Number)){I2C_TIME++;} //清除 EV5
	if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_MODE_SELECT)); //清除 EV5
	
	I2C_Send7bitAddress(I2C2,SlaveAddr,I2C_Direction_Receiver);I2C_TIME=0;//将器件地址传出，主机为读
//	Delay_us(50); //这里的延时很重要	
	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)&& (I2C_TIME<=I2C_Number)){I2C_TIME++;}//清除EV6
	if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//	while(!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));//清除EV6
	
	while(NumByteToRead){
		if(NumByteToRead == 1){ //只剩下最后一个数据时进入 if 语句
			I2C_AcknowledgeConfig(I2C2,DISABLE); //最后有一个数据时关闭应答位
			I2C_GenerateSTOP(I2C2,ENABLE);	//最后一个数据时使能停止位
		}
		while((!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_BYTE_RECEIVED)) && (I2C_TIME<=I2C_Number)){I2C_TIME++;}
		{
			if(I2C_TIME>=I2C_Number){I2C_TIME=0;I2C_reset(I2C2);return 0;}
//		if(I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_BYTE_RECEIVED)){ //读取数据
			*pBuffer = I2C_ReceiveData(I2C2);//调用库函数将数据取出到 pBuffer

			pBuffer++; //指针移位
			NumByteToRead--; //字节数减 1 
		}
	}
	I2C_AcknowledgeConfig(I2C2,ENABLE);
	return 1;
}