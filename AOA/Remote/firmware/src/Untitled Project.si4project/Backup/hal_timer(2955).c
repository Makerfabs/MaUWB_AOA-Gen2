#include "hal_timer.h"
#include "hal_led.h"
#include "hal_key.h"

#include "ose_uwb.h"
#include "Generic.h"

static volatile uint32_t SysTickDelayTime;

hal_timer_t hal_timer = {
	.timer_state = HAL_TIMER_STATE_CLOSE,
	.timer_openTickCnt = 0,
	.timer_Cnt=0
};

/*******************************************************************************
* 函数名  : Delay_nMs
* 描述    : 延时程序(n毫秒)
* 输入    : nm：延时时间(n毫秒)
* 输出    : 无
* 返回    : 无 
* 说明    : 无
*******************************************************************************/
void HalDelay_nMs(uint32_t nms)
{ 
	SysTickDelayTime = nms;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;	//使能滴答定时器 
	while(SysTickDelayTime != 0);				//等待延时时间到
}

void Sleep_us(uint16_t time)
{    
	uint16_t i=0;
	while(time--)
	{
		i=10;  //自己定义
		while(i--);
	}
}


/*******************************************************************************
* 函数名  : SysTickDelayTime_Counter
* 描述    : 获取节拍程序
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 在SysTick中断程序SysTick_Handler()调用(stm32f10x_it.c)
*******************************************************************************/ 
void HalDelayTime_Counter(void)
{
	if (SysTickDelayTime > 0)
	{
		SysTickDelayTime--;
	}
}

/*******************************************************************************
* 函数名  : HalTimeSysTick_Init
* 描述    : 初始化系统滴答时钟SysTick
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 1)、SystemFrequency / 1000		1ms中断一次
*			2)、SystemFrequency / 100000	10us中断一次
*			3)、SystemFrequency / 1000000	1us中断一次
*			计算方法:(SystemFrequency / Value)个系统时钟节拍中断一次
*******************************************************************************/
void HalTimeSysTick_Init(void)
{
	if (SysTick_Config(SystemCoreClock / CLOCKS_PER_SEC))
	{
		/* Capture error */
		while (1);
	}
	NVIC_SetPriority (SysTick_IRQn, 12);
}

/*******************************************************************************
* 函数名  : HALTimer2_Init
* 描述    : Timer2初始化配置
* 输入    : 无
* 输出    : 无
* 返回    : 无 
* 说明    : 无
*******************************************************************************/
void HalTimer2_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);		//使能Timer2时钟		
	TIM_TimeBaseStructure.TIM_Period = 60000;					//设置在下一个更新事件装入活动的自动重装载寄存器周期的值(计数到5000为500ms)
	TIM_TimeBaseStructure.TIM_Prescaler = 71;					//设置用来作为TIMx时钟频率除数的预分频值(10KHz的计数频率)
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//设置时钟分割:TDTS = TIM_CKD_DIV1
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);				//根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE ); 				//使能TIM2指定的中断
	TIM_Cmd(TIM2, ENABLE);  									//使能TIMx外设
}

void HalIWDG_Init(unsigned char prer,unsigned int rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);	//使能对寄存器IWDG_PR和IWDG_RLR的写操作
	IWDG_SetPrescaler(prer);						//设置IWDG预分频值:设置IWDG预分频值为64
	IWDG_SetReload(rlr);							//设置IWDG重装载值
	IWDG_ReloadCounter();							//按照IWDG重装载寄存器的值重装载IWDG计数器
	IWDG_Enable();									//使能IWDG
}

void HalTimer4_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 , ENABLE);
	
	TIM_DeInit(TIM4);											/* 重新启动定时器 */
	
	TIM_TimeBaseStructure.TIM_Period=TIM4_ARR;					/* 自动重装载寄存器周期的值(计数值)100ms */
	TIM_TimeBaseStructure.TIM_Prescaler=TIM4_PSC;				/* 时钟预分频数 72M/72 */
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;		/* 外部时钟采样分频 */
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;	/* 向上计数模式 */
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);						/* 清除溢出中断标志 */
	
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);					/* 开启中断触发*/
	
	TIM_Cmd(TIM4,DISABLE);										/* 关闭定时器 */ 
}


void HalTimer4_NVIC_Config()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*中断优先级NVIC设置*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;				//TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能IRQ通道
	NVIC_Init(&NVIC_InitStructure); 							//初始化NVIC寄存器
}

void HalTimer2_NVIC_Config()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*中断优先级NVIC设置*/
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//使能IRQ通道
	NVIC_Init(&NVIC_InitStructure); 							//初始化NVIC寄存器	 
	
}

void HalTimer4_IT_Enable()
{
	hal_timer.timer_state = HAL_TIMER_STATE_READY_OPEN;
	hal_timer.timer_openTickCnt = portGetTickCnt();
	hal_timer.timer_Cnt = 0;

	Key_Scan(portGetTickCnt());									//防止触发失效
	TIM_Cmd(TIM4,ENABLE);
}

void HalTimer4_IT_Disable()
{
	hal_timer.timer_state = HAL_TIMER_STATE_CLOSE;
	hal_timer.timer_openTickCnt = 0;
	hal_timer.timer_Cnt = 0;

	TIM_Cmd(TIM4,DISABLE);
}


void TIM4_IRQHandler(void)   //TIM4中断
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)			//检查TIM3更新中断发生与否
	{
		hal_timer.timer_state = HAL_TIMER_STATE_OPEN;
		hal_timer.timer_Cnt++;
		
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);				//清除TIMx更新中断标志
		Key_Scan(portGetTickCnt());
	}
}

void HalIWDG_Feed(void)
{
#ifdef HAL_IWDG
	IWDG_ReloadCounter();										//看门狗喂狗
#endif
}


void HalTimerInit(void)
{
#ifdef HAL_SysTick
	HalTimeSysTick_Init();
#endif


#ifdef HAL_TIMER2
	HalTimer2_Init();
	HalTimer2_NVIC_Config();
#endif

#ifdef HAL_TIMER4
	HalTimer4_Init();
	HalTimer4_NVIC_Config();
#endif
	
#ifdef HAL_IWDG
//	HalIWDG_Init(6, 0xFFF);										//最大约32.768秒的独立看门狗
#endif	
}

