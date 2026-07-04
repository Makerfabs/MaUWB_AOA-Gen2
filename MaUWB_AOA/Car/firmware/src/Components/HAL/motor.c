#include "motor.h"
#include "hal_iic.h"//使用其内的位带操作函数
#include "control.h"
/***************************************************************************************************
 * 编码器参数
 ***************************************************************************************************/
int Encoder_A_EXTI=0,Encoder_B_EXTI=0,Encoder_D_EXTI=0;//电机 A B D 编码器数据存储

/***************************************************************************************************
 * 电机控制参数
 ***************************************************************************************************/
float Velocity_KP=15,Velocity_KI=29,Velocity_KD=0.5;//50ms 7200预装载值  
//PID 算法数值
int Bias_A,Pwm_A,Last_bias_A,Integeal_bias_A;
int Bias_B,Pwm_B,Last_bias_B,Integeal_bias_B;
int Bias_C,Pwm_C,Last_bias_C,Integeal_bias_C;
int Bias_D,Pwm_D,Last_bias_D,Integeal_bias_D;
////轮胎堵转标志位
int Block_number,Block_time;
//马达变量
int Motor_A,Motor_B,Motor_C,Motor_D;
int Encoder_A,Encoder_B,Encoder_C,Encoder_D;


/*********************************************************编码器参数读取*********************************************************/
/**************************************************************************
函数名  ：Encoder_Init_TIM2
函数功能：将定时器2 设置为编码器模式， 读取电机C的编码器值
入口参数：无
返回  值：无 
**************************************************************************/
void Encoder_Init_TIM2(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure; 

	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//使能定时器3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能PB端口时钟	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;	//端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB

	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // 预分频器 
	TIM_TimeBaseStructure.TIM_Period = ENCODER_TIM_PERIOD-1; //设定计数器自动重装值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//选择时钟分频：不分频
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;////TIM向上计数  
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);//使用编码器模式3
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 10;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);//清除TIM的更新标志位
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//Reset counter
	TIM_SetCounter(TIM2,0);
	TIM_Cmd(TIM2, ENABLE); 
}


/**************************************************************************
函数名  ：Encoder_Init_TIM_Exit0  与  Encoder_Init_TIM_Exit1
函数功能：外部中断采集编码器 电机D  IO口 PC0 PC1
入口参数：无
返回  值：无 
**************************************************************************/
void Encoder_Init_TIM_Exit0(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;	            //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line=EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//上下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Encoder_Init_TIM_Exit1(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;	            //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource1);
	EXTI_InitStructure.EXTI_Line=EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//上下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);		
}

/**************************************************************************
函数名  ：Encoder_Init_TIM_Exit2  与  Encoder_Init_TIM_Exit3
函数功能：外部中断采集编码器 电机B  IO口 PC2 PC3
入口参数：无
返回  值：无
**************************************************************************/
void Encoder_Init_TIM_Exit2(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	            //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource2);
	EXTI_InitStructure.EXTI_Line=EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
void Encoder_Init_TIM_Exit3(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	            //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource3);
	EXTI_InitStructure.EXTI_Line=EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**************************************************************************
函数名  ：Encoder_Init_TIM_Exit6  与  Encoder_Init_TIM_Exit7
函数功能：外部中断采集编码器 电机A  IO口 PC6 PC7
入口参数：无
返回  值：无
**************************************************************************/
void Encoder_Init_TIM_Exit6(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		EXTI_InitTypeDef EXTI_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;	            //端口配置
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource6);
  	EXTI_InitStructure.EXTI_Line=EXTI_Line6;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//下降沿触发
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器
	  
   // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;	  
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;	
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);	
}

void Encoder_Init_TIM_Exit7(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//外部中断，需要使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //使能PA端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;	            //端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					      //根据设定参数初始化GPIOB 
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource7);
	
	EXTI_InitStructure.EXTI_Line=EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器
	  
	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
}

/**************************************************************************
函数名  ：EXTI_ALL_Init
函数功能：外部中断使能函数
入口参数：无
返回  值：无
**************************************************************************/
void EXTI_ALL_Init(void)
{
	Encoder_Init_TIM2();      //PA0  PA1
	Encoder_Init_TIM_Exit0(); //PC0
	Encoder_Init_TIM_Exit1(); //PC1
	Encoder_Init_TIM_Exit2(); //PC2
	Encoder_Init_TIM_Exit3(); //PC3
	Encoder_Init_TIM_Exit6(); //PC6
	Encoder_Init_TIM_Exit7(); //PC7
}

/**************************************************************************
函数名  ：Read_Encoder
函数功能：单位时间读取编码器计数并清除计数
入口参数：无
返回  值：无
**************************************************************************/
int Read_Encoder(u8 TIMX)
{
	int Encoder_TIM;    
	switch(TIMX)
	{
		case 1:  Encoder_TIM=(short)Encoder_A_EXTI;  Encoder_A_EXTI=0; break;
		case 2:  Encoder_TIM=(short)Encoder_B_EXTI;  Encoder_B_EXTI=0; break;
		case 3:  Encoder_TIM= (short)TIM2 -> CNT;  TIM2 -> CNT=0;break;	
		case 4:  Encoder_TIM=(short)-Encoder_D_EXTI;  Encoder_D_EXTI=0; break;
		default:  Encoder_TIM=0;
	}
	return Encoder_TIM;
}

/**************************************************************************
函数名  ：Read_Encoder_Save
函数功能：单位时间读取编码器计数并保留计数
入口参数：无
返回  值：无
**************************************************************************/
 int Read_Encoder_Save(u8 TIMX)
{
    int Encoder_TIM;    
   switch(TIMX)
	 {
	   case 1:  Encoder_TIM=(short)Encoder_A_EXTI;break;
		 case 2:  Encoder_TIM=(short)Encoder_B_EXTI;break;
		 case 3:  Encoder_TIM= (short)TIM2 -> CNT;break;	
		 case 4:  Encoder_TIM=(short)-Encoder_D_EXTI;break;
		 default:  Encoder_TIM=0;
	 }
		return Encoder_TIM;
}

/**************************************************************************
函数名  ：EXTI1_IRQHandler  与  EXTI3_IRQHandler  与  EXTI9_5_IRQHandler
函数功能：EXTI中断服务函数  通过外部中断实现编码器计数
入口参数：无
返回  值：无
**************************************************************************/
/**************************************************************************
电机D  EXTI外部中断
**************************************************************************/
void EXTI1_IRQHandler(void)//PC1
{			
	EXTI->PR=1<<1;  //清除LINE上的中断标志位
	if(PCin(1)==1)//PC1 上升沿
	{
	if(PCin(0)==0)  Encoder_D_EXTI++;   
	else            Encoder_D_EXTI--;
	}
	else
	{
	if(PCin(0)==0)  Encoder_D_EXTI--;
	else            Encoder_D_EXTI++;
	}		
}
void EXTI0_IRQHandler(void)  //PC0
{
	EXTI->PR=1<<0;  //清除LINE上的中断标志位
	if(PCin(0)==0)//PC0下降沿
	{
	if(PCin(1)==0)   Encoder_D_EXTI++;
	else             Encoder_D_EXTI--;
	}
	else//PC0上升沿
	{
	if(PCin(1)==0)   Encoder_D_EXTI--;
	else             Encoder_D_EXTI++;
	}		
}
/**************************************************************************
电机B  EXTI外部中断
**************************************************************************/
void EXTI3_IRQHandler(void)//PC3
{			
		EXTI->PR=1<<3;  //清除LINE上的中断标志位
	if(PCin(3)==1)
	{
	if(PCin(2)==0)  Encoder_B_EXTI++;   
	else            Encoder_B_EXTI--;
	}
	else
	{
	if(PCin(2)==0)  Encoder_B_EXTI--;
	else            Encoder_B_EXTI++;
	}		
}
void EXTI2_IRQHandler(void)  //PC2
{
	EXTI->PR=1<<2;  //清除LINE上的中断标志位
	if(PCin(2)==0)
	{
	if(PCin(3)==0)   Encoder_B_EXTI++;
	else             Encoder_B_EXTI--;
	}
	else
	{
	if(PCin(3)==0)   Encoder_B_EXTI--;
	else             Encoder_B_EXTI++;
	}		
}
/**************************************************************************
电机A  EXTI外部中断
**************************************************************************/
void EXTI9_5_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line6)!=RESET)
	{
		EXTI->PR=1<<6;
		if(PCin(6)==0)
		{
		if(PCin(7)==0)  Encoder_A_EXTI++;   
		else            Encoder_A_EXTI--;
		}
		else
		{
		if(PCin(7)==0)  Encoder_A_EXTI--;
		else            Encoder_A_EXTI++;
		}	
	}
	if(EXTI_GetFlagStatus(EXTI_Line7)!=RESET)
	{
		EXTI->PR=1<<7;
		if(PCin(7)==1)
		{
		if(PCin(6)==0)  Encoder_A_EXTI++;
		else            Encoder_A_EXTI--;
		}
		else
		{
		if(PCin(6)==0)  Encoder_A_EXTI--;
		else            Encoder_A_EXTI++;
		}	
	}
}


/*********************************************************电机控制*********************************************************/

/**************************************************************************
函数名  ：TIM3_Configuration  TIM4_Configuration
函数功能：定时器3与4 PWM设置
入口参数：motor_x 需要输入的PWM值
返回  值：无
**************************************************************************/
void TIM3_Configuration(int arr,int psc)
{
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		TIM_OCInitTypeDef TIM_OCInitStructure;
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);
		
		TIM_TimeBaseStructure.TIM_Period = arr;							//设置在下一个更新事件装入活动的自动重装载寄存器周期的值 80K
		TIM_TimeBaseStructure.TIM_Prescaler = psc;						//设置用来作为 TIMx 时钟频率除数的预分频值 不分频
		TIM_TimeBaseStructure.TIM_ClockDivision = 0; 					//设置时钟分割:TDTS = Tck_tim
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 	//向上计数
		TIM_TimeBaseStructure.TIM_RepetitionCounter=0;					//重复寄存器，用于自动更新
		TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 				//②初始化 TIMx

		
		TIM_ARRPreloadConfig(TIM3, ENABLE); //使能 TIMx 在 ARR 上的预装载寄存器
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //脉宽调制模式 通道1
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
		TIM_OCInitStructure.TIM_OutputNState=TIM_OutputNState_Disable;//使能互补端输出
		TIM_OCInitStructure.TIM_OCIdleState=TIM_OCIdleState_Reset; //死区后输出状态
		TIM_OCInitStructure.TIM_OCNIdleState=TIM_OCNIdleState_Reset;//死区后互补端输出状态
		TIM_OCInitStructure.TIM_Pulse = 0; 							//设置待装入捕获比较寄存器的脉冲值
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性高
		TIM_OCInitStructure.TIM_OCNPolarity=TIM_OCNPolarity_High; //设置互补端输出极性
		
		TIM_OC1Init(TIM3, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC2Init(TIM3, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC3Init(TIM3, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC4Init(TIM3, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_Cmd(TIM3, ENABLE); //④使能 TIM2
		
		TIM_CtrlPWMOutputs(TIM3,ENABLE); //⑤MOE 主输出使能
		
		TIM_SetAutoreload(TIM3, arr);
		TIM_SetCompare1(TIM3,0);
		TIM_SetCompare2(TIM3,0);
		TIM_SetCompare3(TIM3,0);
		TIM_SetCompare4(TIM3,0);
		
		return;
}
void TIM4_Configuration(int arr,int psc)
{
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		TIM_OCInitTypeDef TIM_OCInitStructure;
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 , ENABLE);
		
		TIM_TimeBaseStructure.TIM_Period = arr;							//设置在下一个更新事件装入活动的自动重装载寄存器周期的值 80K
		TIM_TimeBaseStructure.TIM_Prescaler = psc;						//设置用来作为 TIMx 时钟频率除数的预分频值 不分频
		TIM_TimeBaseStructure.TIM_ClockDivision = 0; 					//设置时钟分割:TDTS = Tck_tim
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 	//向上计数
		TIM_TimeBaseStructure.TIM_RepetitionCounter=0;					//重复寄存器，用于自动更新
		TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); 				//②初始化 TIMx

		
		TIM_ARRPreloadConfig(TIM4, ENABLE); //使能 TIMx 在 ARR 上的预装载寄存器
		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //脉宽调制模式 通道1
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
		TIM_OCInitStructure.TIM_OutputNState=TIM_OutputNState_Disable;//使能互补端输出
		TIM_OCInitStructure.TIM_OCIdleState=TIM_OCIdleState_Reset; //死区后输出状态
		TIM_OCInitStructure.TIM_OCNIdleState=TIM_OCNIdleState_Reset;//死区后互补端输出状态
		TIM_OCInitStructure.TIM_Pulse = 0; 							//设置待装入捕获比较寄存器的脉冲值
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性高
		TIM_OCInitStructure.TIM_OCNPolarity=TIM_OCNPolarity_High; //设置互补端输出极性
		
		TIM_OC1Init(TIM4, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC2Init(TIM4, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC3Init(TIM4, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_OC4Init(TIM4, &TIM_OCInitStructure); 			//③初始化外设 TIMx	
		TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable); 	//CH1 预装载使能
		
		TIM_Cmd(TIM4, ENABLE); //④使能 TIM2
		
		TIM_CtrlPWMOutputs(TIM4,ENABLE); //⑤MOE 主输出使能
		
		TIM_SetAutoreload(TIM4, arr);
		TIM_SetCompare1(TIM4,0);
		TIM_SetCompare2(TIM4,0);
		TIM_SetCompare3(TIM4,0);
		TIM_SetCompare4(TIM4,0);
		
		return;
}
/**************************************************************************
函数名  ：Motor_Gpio_init
函数功能：设置马达各个电机IO口
入口参数：无
返回  值：无
**************************************************************************/
void Motor_Gpio_init(void)
{
	 GPIO_InitTypeDef GPIO_InitStructure;
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);// 
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);//
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); 
   // 设置该引脚为复用输出功能,输出TIM3 CH3 CH4 PWM脉冲波形  
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1 |GPIO_Pin_6|GPIO_Pin_7 |GPIO_Pin_8|GPIO_Pin_9; //TIM3_CH3 //TIM3_CH4
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOB, &GPIO_InitStructure);
   // Motor_Gpio();
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7 ;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	 // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	TIM3_Configuration(7199,0);
	TIM4_Configuration(7199,0);
}

/**************************************************************************
函数名  ：Set_Pwm_Motor1  与  Set_Pwm_Motor2  与  Set_Pwm_Motor3  与  Set_Pwm_Motor4
函数功能：赋值给PWM寄存器  预装载值7200 预分频值0
入口参数：motor_x 需要输入的PWM值
返回  值：无
**************************************************************************/

void Set_Pwm_Motor1(int motor_a)
{
    	if(motor_a>0)			PWMA1=TIM_FOUR_ARR,PWMA2=TIM_FOUR_ARR-motor_a;
			else 	            PWMA2=TIM_FOUR_ARR,PWMA1=TIM_FOUR_ARR+motor_a;
}

void Set_Pwm_Motor2(int motor_b)
{
		  if(motor_b<0)			PWMB2=TIM_FOUR_ARR,PWMB1=TIM_FOUR_ARR+motor_b;
			else 	            PWMB1=TIM_FOUR_ARR,PWMB2=TIM_FOUR_ARR-motor_b;
}
void Set_Pwm_Motor3(int motor_c)
{
    	if(motor_c<0)			PWMC1=TIM_FOUR_ARR,PWMC2=TIM_FOUR_ARR+motor_c;
			else 	            PWMC2=TIM_FOUR_ARR,PWMC1=TIM_FOUR_ARR-motor_c;
}
void Set_Pwm_Motor4(int motor_d)
{
		  if(motor_d>0)			PWMD2=TIM_FOUR_ARR,PWMD1=TIM_FOUR_ARR-motor_d;
			else 	            PWMD1=TIM_FOUR_ARR,PWMD2=TIM_FOUR_ARR+motor_d;
}
/**************************************************************************
函数名  ：PID数据重置 堵转时重置速度
函数功能：设置马达各个电机IO口
入口参数：无
返回  值：无
**************************************************************************/
void PID_Reset_Data(int Pwm_Number)
{
	 switch(Pwm_Number)
				{
					case 1:if(Pwm_A>=0){Pwm_A=7100;}else{Pwm_A=-7100;}Last_bias_A=0;Integeal_bias_A=0;break;// 重置速度
					case 2:if(Pwm_B>=0){Pwm_B=7100;}else{Pwm_B=-7100;}Last_bias_B=0;Integeal_bias_B=0;break;// 重置速度
					case 3:if(Pwm_C>=0){Pwm_C=7100;}else{Pwm_C=-7100;}Last_bias_C=0;Integeal_bias_C=0;break;// 重置速度
					case 4:if(Pwm_D>=0){Pwm_D=7100;}else{Pwm_D=-7100;}Last_bias_D=0;Integeal_bias_D=0;break;// 重置速度
					default:break;
				}
}

/**************************************************************************
函数名  ：Incremental_PI_A  与  Incremental_PI_B  与  Incremental_PI_C  与  Incremental_PI_D
函数功能：增量PI控制器
入口参数：编码器测量值，目标速度
返回  值：电机PWM
根据增量式离散PID公式 
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)代表本次偏差 
e(k-1)代表上一次的偏差  以此类推 
pwm代表增量输出
在我们的速度控制闭环系统里面，只使用PI控制
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)
**************************************************************************/

int Incremental_PI_A (int Encoder,int Target)
{
	Bias_A=Target-Encoder;
	Integeal_bias_A+=Bias_A;
	Pwm_A+=Velocity_KP*(Bias_A-Last_bias_A)+Velocity_KI*Bias_A+Velocity_KD*Integeal_bias_A;   //增量式PI控制器
	if(Integeal_bias_A>=600 || Integeal_bias_A<=-600) {PID_Reset_Data(1);Block_number++;}     //堵转控制
	Last_bias_A=Bias_A;	 //保存上一次偏差 
	//_dbg_printf("Integeal_bias_A  %d  ",Encoder);
	return Pwm_A;   	 //增量输出
}

 int Incremental_PI_B (int Encoder,int Target)
{ 	
	Bias_B=Target-Encoder;
	Integeal_bias_B+=Bias_B;
	Pwm_B+=Velocity_KP*(Bias_B-Last_bias_B)+Velocity_KI*Bias_B+Velocity_KD*Integeal_bias_B;   //增量式PI控制器
	if(Integeal_bias_B>=600 || Integeal_bias_B<=-600) {PID_Reset_Data(2);Block_number++;}     //堵转控制
	Last_bias_B=Bias_B;	                   //保存上一次偏差 
	//_dbg_printf("Integeal_bias_B  %d  ",Encoder);
	return Pwm_B;                         //增量输出
}

 int Incremental_PI_C (int Encoder,int Target)
{
	Bias_C=Target-Encoder;
	Integeal_bias_C+=Bias_C;
	//	 Xianfu_Pwm_KD();
	Pwm_C+=Velocity_KP*(Bias_C-Last_bias_C)+Velocity_KI*Bias_C+Velocity_KD*Integeal_bias_C;   //增量式PI控制器
	if(Integeal_bias_C>=600 || Integeal_bias_C<=-600) {PID_Reset_Data(3);Block_number++;}     //堵转控制
	Last_bias_C=Bias_C;	                   //保存上一次偏差 
	//	  _dbg_printf("Integeal_bias_C  %d  ",Encoder);	 
	return Pwm_C;                         //增量输出
}

int Incremental_PI_D (int Encoder,int Target)
{
	Bias_D=Target-Encoder;
	Integeal_bias_D+=Bias_D;
	//	 Xianfu_Pwm_KD();
	Pwm_D+=Velocity_KP*(Bias_D-Last_bias_D)+Velocity_KI*Bias_D+Velocity_KD*Integeal_bias_D;   //增量式PI控制器
	if(Integeal_bias_D>=600 || Integeal_bias_D<=-600) {PID_Reset_Data(4);Block_number++;}     //堵转控制
	Last_bias_D=Bias_D;	                   //保存上一次偏差 
	//	  _dbg_printf("Integeal_bias_D  %d \n",Encoder);	 
	return Pwm_D;                         //增量输出
}

/**************************************************************************
函数名  ：Xianfu_Pwm
函数功能：限制PWM赋值
入口参数：无
返回  值：无
**************************************************************************/
void Xianfu_Pwm(void)
{
	int Amplitude=7100;    //===PWM满幅是7199 限制在7100
	if(Motor_A<-Amplitude) Motor_A=-Amplitude;	if(Motor_A>Amplitude)  Motor_A=Amplitude;	
	if(Motor_B<-Amplitude) Motor_B=-Amplitude;	if(Motor_B>Amplitude)  Motor_B=Amplitude;	
	if(Motor_C<-Amplitude) Motor_C=-Amplitude;	if(Motor_C>Amplitude)  Motor_C=Amplitude;	
	if(Motor_D<-Amplitude) Motor_D=-Amplitude;	if(Motor_D>Amplitude)  Motor_D=Amplitude;	
}

/**************************************************************************
函数名  ：motor_speed_A  与  motor_speed_B 与  motor_speed_C  与  motor_speed_D
函数功能：通过限制编码器的值来限制轮胎转速 A左前轮 D右前轮  B左后轮 C右后轮
入口参数：int number 当前需要的速度
返回  值：无
**************************************************************************/
void motor_speed_A(int number)
{
	Encoder_A=Read_Encoder(1);                                    //读取当前编码器的值
//	Encoder_A=kalmanFilter_ALL(Read_Encoder(1),1,Filter_Q,Filter_R);//0.338,3
	Motor_A=Incremental_PI_A(Encoder_A,number);                   //===速度闭环控制计算电机A最终PWM      
	Xianfu_Pwm();                                                 //===PWM限幅
	Set_Pwm_Motor1(Motor_A);
}
void motor_speed_B(int number)
{
	Encoder_B=Read_Encoder(2);                                   //读取当前编码器的值
//	Encoder_B=kalmanFilter_ALL(Read_Encoder(2),2,Filter_Q,Filter_R);//0.338,3
	Motor_B=Incremental_PI_B(Encoder_B,number);                  //===速度闭环控制计算电机B最终PWM 
	Xianfu_Pwm();                                                //===PWM限幅
	Set_Pwm_Motor2(Motor_B);
}
void motor_speed_C(int number)
{
	Encoder_C=Read_Encoder(3);                                   //读取当前编码器的值
//	Encoder_C=kalmanFilter_ALL(Read_Encoder(3),3,Filter_Q,Filter_R);//0.338,3
	Motor_C=Incremental_PI_C(Encoder_C,number);                  //===速度闭环控制计算电机A最终PWM      
	Xianfu_Pwm();                                                //===PWM限幅
	Set_Pwm_Motor3(Motor_C);
}
void motor_speed_D(int number)
{
	Encoder_D=Read_Encoder(4);                                   //读取当前编码器的值
	//Encoder_D=kalmanFilter_ALL(Read_Encoder(4),4,Filter_Q,Filter_R);//0.338,3
	Motor_D=Incremental_PI_D(Encoder_D,number);                  //===速度闭环控制计算电机A最终PWM      
	Xianfu_Pwm();                                                //===PWM限幅
	Set_Pwm_Motor4(Motor_D);
}

/**************************************************************************
函数名  ：CAR_STOP
函数功能：车辆急停
入口参数：无
返回  值：无
**************************************************************************/
void CAR_STOP(void)
{
	Read_Encoder(1);Read_Encoder(2);Read_Encoder(3);Read_Encoder(4);//清除编码器计数
	Set_Pwm_Motor1(0);Set_Pwm_Motor2(0);Set_Pwm_Motor3(0);Set_Pwm_Motor4(0);//PWM 归0
}


/**************************************************************************
函数名  ：CAR_SLOW_STOP
函数功能：车辆缓停，每次循环按比例减弱PWM电压，达到缓停效果
入口参数：无
返回  值：无
**************************************************************************/
void CAR_SLOW_STOP(void)
{
	int i,ALL_PWM;
	if(((abs(Motor_A)+abs(Motor_B)+abs(Motor_C)+abs(Motor_D))/4)<2000)
	{CAR_STOP();}
	else 
	{
			Motor_A=Motor_A*7/10;Set_Pwm_Motor1(Motor_A);
			Motor_B=Motor_B*7/10;Set_Pwm_Motor2(Motor_B);
			Motor_C=Motor_C*7/10;Set_Pwm_Motor3(Motor_C);
			Motor_D=Motor_D*7/10;Set_Pwm_Motor4(Motor_D);
//			USART4_printf("/Motor_A %d\n",Motor_A);
	}	
}

/**************************************************************************
函数名  ：ALL_motor_control 
函数功能：控制所有的轮胎
入口参数：speed 需要的马达速度
返回  值：无
**************************************************************************/
void ALL_motor_control (int A_speed,int B_speed,int C_speed,int D_speed)//通过PID算法控制马达
{
	motor_speed_A(A_speed);
	motor_speed_B(B_speed);
	motor_speed_C(C_speed);
	motor_speed_D(D_speed);
}
/**************************************************************************
函数名  ：ALL_motor_pwm_control
函数功能：控制所有的轮胎
入口参数：A_Pwm 需要的马达Pwm
返回  值：无
**************************************************************************/
void ALL_motor_pwm_control (int A_Pwm,int B_Pwm,int C_Pwm,int D_Pwm)//通过直接设置PWM值控制马达
{
	Set_Pwm_Motor1(A_Pwm);
	Set_Pwm_Motor2(B_Pwm);
	Set_Pwm_Motor3(C_Pwm);
	Set_Pwm_Motor4(D_Pwm);
}

/**************************************************************************
函数名  ：MW_Radius_Compute
函数功能：通过转速比计算当前麦克纳姆轮小车运动半径
入口参数：MW_P 当前预设比例 
返回  值：无
**************************************************************************/
float MW_Radius_Compute(float MW_P)
{
	float MW_proportion;
	MW_proportion=(Vehicle_width*MW_P+Vehicle_width)/(1-MW_P);
	return MW_proportion;
}

/**************************************************************************
函数名  ：car_motor_wheel_speed_one  与  car_motor_wheel_speed_two
函数功能：根据角度改变电机速率
入口参数：Angual 当前角度 ,Gear1 预设车辆中心速度 ,Gear2 预设车辆内外轮转速比
返回  值：无
四轮小车拐弯的两种方式 
one 四轮同向差速转向 两轮快速,两轮慢速，差速转弯
two 四轮异向差速转向 两轮正转,两轮反转，差速转弯（暂不使用）
**************************************************************************/
void car_motor_wheel_speed_one(int Angual_1 ,int Gear1,float Gear2)//
{
	float MW_Radius=(Vehicle_width*Gear2+Vehicle_width)/(1-Gear2);//计算绕圆半径
	if(Angual_1>0)//x<0时方向右 x>0时方向为左。
		{
			MW_motor_circular_motion(Gear1 ,MW_Radius,0);
//			ALL_motor_control(-Gear2,Gear2,Gear1,-Gear1);
//			_dbg_printf("/ → → → → → → / \n");
		}
		else if(Angual_1<0)
		{
			MW_motor_circular_motion(Gear1 ,MW_Radius,1);
//			ALL_motor_control(-Gear1,Gear1,Gear2,-Gear2);
//			_dbg_printf("/ ← ← ← ← ← ← / \n");
		}	
}
void car_motor_wheel_speed_two(int Angual_2 ,int Gear3,int Gear4)
{
		if(Angual_2>0)//x<0时方向右 x>0时方向为左。
		{
			ALL_motor_control(-Gear4,-Gear4,Gear3,Gear3);
//			_dbg_printf("/ ← ← ← ← ← ← / \n"); 
		}
		else if(Angual_2<0)
		{
			ALL_motor_control(Gear3,Gear3,-Gear4,-Gear4);
//			_dbg_printf("/ → → → → → → / \n");
		}	
}

/**************************************************************************
函数名  ：Angle_judgement
函数功能：角度判断小车的转向速度
入口参数：Angual
返回  值：无
**************************************************************************/	
void Angle_judgement(int Angual,uint16_t Speed_C)
{  
	//大角度转弯
	if(Angual<=-80 || Angual>=80)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_8);}	
	else if(Angual<=-70 || Angual>=70)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_7);}	
	else if(Angual<=-60 || Angual>=60)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_6);}	
	else if(Angual<=-50 || Angual>=50)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_4);}	
	else if(Angual<=-40 || Angual>=40)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_4);}
	else if(Angual<=-30 || Angual>=30)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_2);}
	//小角度转弯
	else if(Angual<=-15 || Angual>=15)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_1);}
	else if(Angual<=-Straight_angle || Angual>=Straight_angle)
	{car_motor_wheel_speed_one( Angual,Speed_C,MW_percentage_1);} 
}


/**************************************************************************
函数名  ：MW_motor_circular_motion
函数功能：麦克纳姆轮 或 普通轮 绕圆运动 
入口参数：Wheel_speed 车辆行驶速度  radius 绕圆半径 Direction_selection转弯方式
返回  值：无
**************************************************************************/
void MW_motor_circular_motion(int Wheel_speed ,float radius,int Direction_selection)
{
	static float V_outside,V_inside;
	float a,b;
	a=Wheel_speed*1.00;b=radius*1.00;
	V_outside=a+Vehicle_width*(a/b);V_inside=a-Vehicle_width*(a/b);
	if(Direction_selection==0)
	{ALL_motor_control (V_inside,V_inside,V_outside,V_outside);}//左上
	else if(Direction_selection==1)
	{ALL_motor_control (V_outside,V_outside,V_inside,V_inside);}//右上
	else if(Direction_selection==2)
	{ALL_motor_control (-V_outside,-V_outside,-V_inside,-V_inside);}//左下
	else if(Direction_selection==3)
	{ALL_motor_control (-V_inside,-V_inside,-V_outside,-V_outside);}//右下
//	_dbg_printf("/V_outside:%0.2f    V_inside:%0.2f   radius:%0.2f  \n",V_outside,V_inside,radius);
}


/**************************************************************************
函数名  ：Preset_State_Of_Motor
函数功能：车辆直线控制
入口参数：车辆控制命令
返回  值：无
说    明：无
**************************************************************************/
void Preset_State_Of_Motor(uint8_t Motor_State)//
{
	int C_Speed=AVG.Car_Speed;
	float MW_Radius1=(Vehicle_width*MW_percentage_5+Vehicle_width)/(1-MW_percentage_6);//计算转弯半径
	switch(Motor_State)
	{
		case RCSF:   
			ALL_motor_control(C_Speed,C_Speed,C_Speed,C_Speed);
//			_dbg_printf("遥控直行\n");
		break;
		
		case FOLLOW_STRAIGHT:   
			ALL_motor_control(C_Speed,C_Speed,C_Speed,C_Speed);
//			_dbg_printf("跟随直行\n");
		break;
		
		case STOP_MOTOR://速度为0
			CAR_STOP();
//			_dbg_printf("停止\n"); 
		break;
		
		case BACKWARD:  
			ALL_motor_control(-C_Speed,-C_Speed,-C_Speed,-C_Speed);
//			_dbg_printf("后退\n"); 
		break;
		
		case SPOT_LEFT_TURN:
//			_dbg_printf("原地左转\n"); 
			C_Speed=Back_speed;//减慢速度
			ALL_motor_control(-C_Speed,-C_Speed,C_Speed,C_Speed);
		break;
		
		case SPOT_RIGHT_TURN:   
			C_Speed=Back_speed;//减慢速度
			ALL_motor_control(C_Speed,C_Speed,-C_Speed,-C_Speed);
//			_dbg_printf("原地右转\n"); 
		break;
		
		case LEFT_BACKWARD://左倒车
			C_Speed=Back_speed;//减慢速度
			MW_motor_circular_motion(C_Speed ,MW_Radius1,3);
//			_dbg_printf("左倒车\n"); 
		break;
		
		case RIGHT_BACKWARD://右倒车
			C_Speed=Back_speed;//减慢速度
			MW_motor_circular_motion(C_Speed ,MW_Radius1,2);
//			_dbg_printf("右倒车\n"); 
		break;
		
		
		default: break;//_dbg_printf("未知状态\n"); break;
		
	}
}

void Car_Pwm_Direction(uint16_t FL_Dis,uint16_t FL_Ang,float FL_Speed)
{ 
	static int TRA_DMG=0;//初始设置 5米
	signed char Angle;
	uint16_t Foll_Dis,Distance;//
	uint8_t Tag_bit,Tof_bit,Dis_Red=20;//距离冗余 与直行角度
	
	Foll_Dis=FL_Dis;//跟随距离赋值
	Tag_bit=AVG.Basic_Directions;//标签位置参数赋值
	Tof_bit=AVG.Tof_Directions;//TOF位置参数赋值
	
	Distance=AVG.Distance_filter;//A0基站距离赋值
	Angle=AVG.Angle_filter;//A0基站角度赋值
	
//	_dbg_printf("/Check_Sta: %d\n",Ck_Sta);
	if(TRA_DMG ==0){TRA_DMG=Foll_Dis+Dis_Red;}
	
	if(Distance<TRA_DMG || Tag_bit !=0 || Tof_bit != 0)//进入安全距离 或 标签处于后方 或 车辆前方有障碍 时停止跟随
	{
		TRA_DMG=Foll_Dis+Dis_Red;
		
		if(Distance<TRA_DMG)//车辆安全距离内停止
		{
			Preset_State_Of_Motor(STOP_MOTOR);//停止
//			_dbg_printf("停止:%d %d \r\n",Angle,Distance,TRA_DMG);
		}
		else if(Tof_bit != 0)//根据TOF位置避障
		{
			Preset_State_Of_Motor(Tof_bit);
//			_dbg_printf("避障:%d %d \r\n",Angle,Distance,TRA_DMG);
		}
		else if(Tag_bit !=0)//根据标签位置掉头
		{
			Preset_State_Of_Motor(Tag_bit);
//			_dbg_printf("掉头:%d %d \r\n",Angle,Distance,TRA_DMG);
		}
	}
	else if(ABS(Angle)<=Straight_angle )//直行
	{
		TRA_DMG=Foll_Dis-Dis_Red;
		Preset_State_Of_Motor(FOLLOW_STRAIGHT);
		//_dbg_printf("直行:%d %d %d\r\n",Angle,Distance,TRA_DMG);
	}
	else
	{
		Angle_judgement(Angle,FL_Speed);
//		Set_Turn_Level(Angle,Straight_angle);
		TRA_DMG=Foll_Dis-Dis_Red;
		//_dbg_printf("跟随:%d %d %d\r\n",Angle,Distance,TRA_DMG);
	}
	
//	_dbg_printf("O:%d  %d  %d \r\n",Tag_bit,Angle,Distance);
}








