#include "Periph_init.h"
#include "delay.h"

/**************************************************************************LED初始化**************************************************************************/

/**************************************************************************
函数名  ：LED_Init
函数功能：LED IO初始化
入口参数：无
返回  值：无 
**************************************************************************/
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(LED_RCC,ENABLE);//使能PORTB时钟
	GPIO_InitStructure.GPIO_Pin  = LED1_PIN|LED2_PIN;//PC4蓝灯  PC5红灯
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置为推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(LED_GP, &GPIO_InitStructure);//初始化GPIOB14	
//	GPIO_SetBits(LED_GP,LED1_PIN);
//	GPIO_SetBits(LED_GP,LED2_PIN);
}

/**************************************************************************
函数名  ：Led_Test
函数功能：LED 测试函数 点亮LED 500ms然后熄灭500ms 循环2次
入口参数：无
返回  值：无 
**************************************************************************/
void Led_Test(void)
{
	LED2(OFF);LED1(OFF);
	Delay_ms(300);
	LED2(ON);LED1(ON);
	Delay_ms(300);
	LED2(OFF);LED1(OFF);
	Delay_ms(300);
	LED2(ON);LED1(ON);
	Delay_ms(300);
	LED2(OFF);LED1(OFF);
}

/**************************************************************************蜂鸣器初始化**************************************************************************/

/**************************************************************************
函数名  ：Beep_Init
函数功能：蜂鸣器IO初始化
入口参数：无
返回  值：无 
**************************************************************************/
void Beep_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;    
	RCC_APB2PeriphClockCmd(BEEP_RCC, ENABLE);  //使能PD端口时钟
	GPIO_InitStructure.GPIO_Pin  = BEEP_PIN;  //蜂鸣器-->PD2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置为推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //IO口速度为50MHZ
 	GPIO_Init(BEEP_GP, &GPIO_InitStructure);//根据参数初始化PB12	
	GPIO_SetBits(BEEP_GP,BEEP_PIN);				 //PB12 输出高
}
/**************************************************************************
函数名  ：Beep_Test
函数功能：蜂鸣器测试函数
入口参数：int Beep_number 控制蜂鸣器鸣叫频率
返回  值：无 
**************************************************************************/ 
void Beep_Test(int Beep_number)
{
	BEEP(ON);  
	Delay_ms(50*Beep_number);
	BEEP(OFF);
	Delay_ms(50*Beep_number);
}

/**************************************************************************
函数名  ：Periph_init
函数功能：指示外设初始化,并设置开机状态 指示灯闪烁两次，蜂鸣器响0.5s
入口参数：无
返回  值：无 
**************************************************************************/ 
void Periph_init(void)
{
	LED_Init();
	Led_Test();
	Beep_Init();
	Beep_Test(10);
}