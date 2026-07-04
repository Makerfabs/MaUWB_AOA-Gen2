#include "tof_i2c.h"
#include "hal_iic.h"
/**************************************************************************
函数功能：过滤TOF传感器的距离
入口参数：当前距离与传感器编号
返回  值：Kalman_X[Group]
**************************************************************************/
float Kalman_filtering_TOF (int TOF_distance,int Group)
{
	static float Kalman_K,Kalman_X[5],Kalman_Z[5],Kalman_EST=50,Kalman_ME[5];
  
	Kalman_ME[Group]=fabs(Kalman_Z[Group]-TOF_distance);
	Kalman_K=Kalman_EST/(Kalman_EST+Kalman_ME[Group]);
	Kalman_X[Group]=Kalman_X[Group]+Kalman_K*(TOF_distance-Kalman_X[Group]);
	//更新
	Kalman_Z[Group]=TOF_distance;//为下一次滤波做准备
	return Kalman_X[Group];
}

void TOF250_READ(TOF_Distance *TOF_D)
{
		static int FAST_READ=1;
		int LEFT,MIDDLE_R,MIDDLE_L,RIGHT;
		int TOF_Distance_Max=100;
		u8 buf_L[3]={0},buf_MR[3]={0},buf_ML[3]={0},buf_R[3]={0};
	
		
//		I2C_BufferRead(TOF_L, buf_R, DISTANCE_data, 2);
		//_dbg_printf("/%X  %X  %X\r\n",buf_R[0],buf_R[1],buf_R[2]);
		//IIC读取距离
		I2C_READ_BUFFER(TOF_R,DISTANCE_data,buf_R,2);//读出右传感器距离
//		Delay_us(50); //这里的延时很重要	
		I2C_READ_BUFFER(TOF_MR,DISTANCE_data,buf_MR,2);//读出右传感器距离
//		Delay_us(50); //这里的延时很重要	
		I2C_READ_BUFFER(TOF_ML,DISTANCE_data,buf_ML,2);//读出右传感器距离
//		Delay_us(50); //这里的延时很重要	
		I2C_READ_BUFFER(TOF_L,DISTANCE_data,buf_L,2);//读出右传感器距离
		//_dbg_printf("/%X  %X  %X\r\n",buf_R[0],buf_R[1],buf_R[2]);
		
		//解析读取数据
		RIGHT=   buf_R[0]<<8|buf_R[1];		if(RIGHT>=TOF_Distance_Max){RIGHT=TOF_Distance_Max;}
		MIDDLE_R=buf_MR[0]<<8|buf_MR[1];  if(MIDDLE_R>=TOF_Distance_Max){MIDDLE_R=TOF_Distance_Max;}
		MIDDLE_L=buf_ML[0]<<8|buf_ML[1];	if(MIDDLE_L>=TOF_Distance_Max){MIDDLE_L=TOF_Distance_Max;}
		LEFT=    buf_L[0]<<8|buf_L[1];		if(LEFT>=TOF_Distance_Max){LEFT=TOF_Distance_Max;}
		
		//数据滤波
		if(RIGHT==0){}else {TOF_D->RIGHT_MM= Kalman_filtering_TOF (RIGHT,3);}
		if(MIDDLE_R==0){}else {TOF_D->MIDDLE_R_MM= Kalman_filtering_TOF (MIDDLE_R,2);}
		if(MIDDLE_L==0){}else {TOF_D->MIDDLE_L_MM= Kalman_filtering_TOF (MIDDLE_L,1);}
		if(LEFT==0){}else {TOF_D->LEFT_MM= Kalman_filtering_TOF (LEFT,0);}
//	   _dbg_printf("/%d  %d  %d  %d \n",LEFT,MIDDLE_R,MIDDLE_L,RIGHT);
		//_dbg_printf("/%d  %d  %d  %d \r\n",TOF_D->RIGHT_MM,TOF_D->MIDDLE_R_MM,TOF_D->MIDDLE_L_MM,TOF_D->LEFT_MM);
	
}