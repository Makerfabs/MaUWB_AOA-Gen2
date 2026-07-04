#include "stm32f10x.h"


/***************************************************************************************************
 * 编码器宏定义
 ***************************************************************************************************/
#define ENCODER_TIM_PERIOD (u16)(65535)   //不可大于65535 因为F103的定时器是16位的。

/***************************************************************************************************
 * 电机控制宏定义
 ***************************************************************************************************/
#define TIM_FOUR_ARR 7200

#define PWMA1   TIM3->CCR3   //PB0
#define PWMA2   TIM3->CCR4   //PB1

#define PWMB1   TIM3->CCR1   //PA6
#define PWMB2   TIM3->CCR2   //Pa7

#define PWMC1   TIM4->CCR1   //PB6
#define PWMC2   TIM4->CCR2   //PB7

#define PWMD1   TIM4->CCR3   //PB8
#define PWMD2   TIM4->CCR4   //PB9

 //直行速度 三挡可调
#define  Initial_speed   100   //初始化速度
#define  Follow_speed    100   //跟随速度
#define  Rcsf_speed      70    //遥控速度
#define  Back_speed      30    //掉头与后退速度

//直行角度  不同发距离应该需要不同的直行角度
#define  Straight_angle 10

//通过左右轮的目标速度比值计算当前左右两轮速度值
#define  MW_percentage_1  0.9   
#define  MW_percentage_2  0.8
#define  MW_percentage_3  0.7
#define  MW_percentage_4  0.6
#define  MW_percentage_5  0.5
#define  MW_percentage_6  0.4
#define  MW_percentage_7  0.3
#define  MW_percentage_8  0.2

//绕远参数
#define Vehicle_width 10 //当前车辆宽度的一半

/***************************************************************************************************
 * 编码器函数
 ***************************************************************************************************/
void Encoder_Init_TIM2(void);
void EXTI_ALL_Init(void);
int Read_Encoder(u8 TIMX);
int Read_Encoder_Save(u8 TIMX);

/***************************************************************************************************
 * 电机控制函数
 ***************************************************************************************************/
//马达IO口设置
void Motor_Gpio_init(void);

//设置各马达的PWM值
void Set_Pwm_Motor1(int motor_a);
void Set_Pwm_Motor2(int motor_b);
void Set_Pwm_Motor3(int motor_c);
void Set_Pwm_Motor4(int motor_d);

//PID算法计算当前需要的PWM值
int Incremental_PI_A (int Encoder,int Target);
int Incremental_PI_B (int Encoder,int Target);
int Incremental_PI_C (int Encoder,int Target);
int Incremental_PI_D (int Encoder,int Target);

//PWM限制器，将PWM限制在7100
void Xianfu_Pwm(void);

//设置单个马达的转速
void motor_speed_A (int number);
void motor_speed_B (int number);
void motor_speed_C (int number);
void motor_speed_D (int number);


//根据角度值和距离值控制小车运动
void Angle_judgement(int Angual,uint16_t Speed_C);//角度判断


//四轮小车拐弯的两种方式 
//one 四轮同向差速转向 两轮快速,两轮慢速，差速转弯 Gear1快速 Gear2慢速
//two 四轮异向差速转向 两轮正转,两轮反转，差速转弯 Gear3正转 Gear4反转
void car_motor_wheel_speed_one(int Angual_1 ,int Gear1,float Gear2);
void car_motor_wheel_speed_two(int Angual_2 ,int Gear3,int Gear4);

//判断小车是转弯还是直行
void MW_motor_circular_motion(int Wheel_speed ,float radius,int Direction_selection);//绕圆控制
void Preset_State_Of_Motor(uint8_t Motor_State);//直线控制
void Car_Pwm_Direction(uint16_t FL_Dis,uint16_t FL_Ang,float FL_Speed);//跟随控制
/**************************************************************************
各类次要函数体
**************************************************************************/
void CAR_STOP(void);//控制小车停止
void ALL_motor_control (int A_speed,int B_speed,int C_speed,int D_speed);//用PID算法控制所有马达的转速
void ALL_motor_pwm_control (int A_Pwm,int B_Pwm,int C_Pwm,int D_Pwm);//直接设置所有马达的PWM值

