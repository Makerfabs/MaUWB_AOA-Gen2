#include "timer.h"
#include "stm32f10x_iwdg.h"

/**************************************************************************
函数名  ：IWDG_Init
函数功能：初始化独立看门狗
入口参数：prer:分频数:0~7(只有低3位有效!),分频因子=4*2^prer.但最大值只能是256!
		  rlr:重装载寄存器值:低11位有效.时间计算(大概):Tout=((4*2^prer)*rlr)/40 (ms).
返回  值：无 
**************************************************************************/
void HalIwdgInit(u8 prer,u16 rlr) 
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //使能对寄存器IWDG_PR和IWDG_RLR的写操作	
	IWDG_SetPrescaler(prer);	//设置IWDG预分频值:设置IWDG预分频值为64
	IWDG_SetReload(rlr);			//设置IWDG重装载值
	IWDG_ReloadCounter(); 		//按照IWDG重装载寄存器的值重装载IWDG计数器
	IWDG_Enable();						//使能IWDG
}

void HalIwdgFeed(void)//看门狗喂狗
{
	IWDG_ReloadCounter();			//reload		
}

/**************************************************************************
函数名  ：TIM1_NVIC_Configuration
函数功能：定时器1中断配置函数
入口参数：无
返回  值：无 
**************************************************************************/
void TIM1_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure; 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  													
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;	  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**************************************************************************
函数名  ：TIM1_Configuration
函数功能：定时器1配置函数
入口参数：Tim1_Arr  自动重装载寄存器周期的值(计数值)
		  Tim1_Psc  时钟预分频数
		  时间计算(大概) Tout== (PSC + 1) * (ARR + 1) / 72000000(72M),当前200ms一次中断
返回  值：无 
**************************************************************************/
void TIM1_Configuration(int Tim1_Arr,int Tim1_Psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);
	
	TIM_DeInit(TIM1); 										/* 重新启动定时器 */
	
	TIM_TimeBaseStructure.TIM_Period=Tim1_Arr;						/* 自动重装载寄存器周期的值(计数值) */
	TIM_TimeBaseStructure.TIM_Prescaler=Tim1_Psc; 					/* 时钟预分频数 72M/72 */
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 		/* 外部时钟采样分频 */
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; 	/* 向上计数模式 */
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update); 					/* 清除溢出中断标志 */
	
	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);					/* 开启中断触发*/ 	 
	
	TIM_Cmd(TIM1,DISABLE);											/* 关闭定时器 */ 

}

/**************************************************************************
函数名  ：Timer_Init
函数功能：定时器初始化，并设置看门狗
入口参数：无
返回  值：无 
**************************************************************************/
void Timer_Init(void)
{
	HalIwdgInit(7,625);
	TIM1_NVIC_Configuration();    //8s 中断一次
	TIM1_Configuration(1999,7199);//200ms 中断一次
	
	TIM_Cmd(TIM1,ENABLE);
}
