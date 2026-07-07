#include "control.h"
#include "uwb.h"
#include "hal_usart.h"
#include "math.h"
#include "delay.h"
#include "oled_i2c.h"
#include "tof_i2c.h"
#include "motor.h"
#include "Periph_init.h"
#include "hal_iic.h"
TOF_Distance TOF_D;
AOA_DATA AVG={0};

/**************************************************************************
函数名  ：单角度处理核心逻辑
函数功能：过滤单个差异较大的角度
入口参数：current_angle  需要过滤的角度   last_angle    上一次角度
          output_num     超限次数         output_thresh 超限阈值
          inverse_thresh 异向角变化阈值   same_thresh   同向角变化阈值
返回  值：过滤角度 
**************************************************************************/ 
static void process_single_angle(int16_t* current_angle, int* last_angle,
								int* output_num,uint8_t output_thresh,
								int inverse_thresh, int same_thresh) {
	int curr = *current_angle;
	int last = *last_angle;
	int outnum=*output_num;
									
	// 计算实际角度差（考虑圆周特性）
	int delta = curr - last;

	// 判断角度变化方向
	int is_same_direction = (last * curr) >= 0;  // 同向标志

	// 应用动态阈值
	int threshold = is_same_direction ? same_thresh : inverse_thresh;

	// 跳变超限处理
	if(delta == curr && last==0)//初次进入时
	{//因为 current_angle 有数据是 是不为0的，故只有在参数在第一次开始滤波时。
	 //才会触发该条件
		*current_angle=curr; // 更新滤波值
		*last_angle = curr;  // 更新历史值
		*output_num=0;
	}
	else if(outnum>=output_thresh)//连续超限的情况下
	{
		*current_angle=curr; // 更新滤波值
		*last_angle = curr;  // 更新历史值
		*output_num=0;
	}
	else if(abs(delta) > threshold)//超过阈值 不使用该值
	{
		*current_angle = *last_angle;  // 保持上次有效值
		(*output_num)++;
	}
	else //未超过阈值 使用该值
	{
		*current_angle=curr; // 更新滤波值
		*last_angle = curr;  // 更新历史值
	}
}

/**************************************************************************
函数名  ：AOA_Angle_Filter
函数功能：过滤所有差异较大的角度，无数据的基站不做滤波
入口参数：AVG 需要过滤的结构体   
          inverse_thresh  异向角变化阈值   
          same_thresh     同向角变化阈值
          output_thresh   超限阈值
返回  值：过滤角度 
**************************************************************************/ 
void AOA_Angle_Filter(AOA_DATA* AVG, int inverse_thresh, int same_thresh,
                      uint8_t output_thresh) 
{
    static int last_angles[4] = {0}; // 存储4个基站的历史角度
    static int output_num[4] = {0};  // 存储前4个基站的连续超限次数

    for(int i=0; i<4; i++) 
	{
#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
		//确定数据有效则，处理More_tag_tof_Ax数组
		if(AVG->More_tag_tof_Ax[i].angle !=0 && AVG->More_tag_tof_Ax[i].range !=0)
		{
			process_single_angle(&AVG->More_tag_tof_Ax[i].angle,&last_angles[i],
								&output_num[i],output_thresh,
								inverse_thresh, 
								same_thresh);
		}
#else
		//确定数据有效则，处理处理tag_tof_Ax数组
		if(AVG->tag_tof_Ax[i].angle !=0 && AVG->tag_tof_Ax[i].range !=0)
		{
			process_single_angle(&AVG->tag_tof_Ax[i].angle,&last_angles[i],
								&output_num[i],output_thresh,
								inverse_thresh, 
								same_thresh);
		}
#endif
	}
}

/**************************************************************************
函数名  ：Kalman_filtering
函数功能：简单的卡尔曼滤波，无数据的基站不做滤波
入口参数：AVG       数据存储位置
返回  值：无
参数含义: 
**************************************************************************/ 
void kalman_filter(AOA_DATA *AVG) 
{
	static KalmanState angle_states[4] = {0};//定义角度滤波的卡尔曼参数
	static KalmanState range_states[4] = {0};//定义距离滤波的卡尔曼参数
	
	for (int i = 0; i < 4; ++i) {
		// 获取原始测量值
		int16_t meas_angle; uint16_t meas_range;

	#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
		meas_angle = AVG->More_tag_tof_Ax[i].angle;
		meas_range = AVG->More_tag_tof_Ax[i].range;
	#else
		meas_angle = AVG->tag_tof_Ax[i].angle;
		meas_range = AVG->tag_tof_Ax[i].range;
	#endif

		// 角度滤波处理
		if (angle_states[i].p == 0) { // 首次初始化
			angle_states[i].x = (float)meas_angle;
			angle_states[i].p = 1.0f;
			angle_states[i].q = 0.4f;  // 过程噪声
			angle_states[i].r = 0.7f;  // 测量噪声
		} else {
			// 预测阶段
			angle_states[i].p += angle_states[i].q;
			
			// 更新阶段
			float k = angle_states[i].p / (angle_states[i].p + angle_states[i].r);
			angle_states[i].x += k * ((float)meas_angle - angle_states[i].x);
			angle_states[i].p *= (1 - k);
		}

		// 距离滤波处理
		if (range_states[i].p == 0) { // 首次初始化
			range_states[i].x = (float)meas_range;
			range_states[i].p = 1.0f;
			range_states[i].q = 0.3f;
			range_states[i].r = 0.5f;
		} else {
			// 预测阶段
			range_states[i].p += range_states[i].q;
			
			// 更新阶段
			float k = range_states[i].p / (range_states[i].p + range_states[i].r);
			range_states[i].x += k * ((float)meas_range - range_states[i].x);
			range_states[i].p *= (1 - k);
		}

		// 回写滤波结果
	#ifdef USE_RSSI_TAG
		AVG->More_tag_tof_Ax[i].angle = (int16_t)roundf(angle_states[i].x);
		AVG->More_tag_tof_Ax[i].range = (uint16_t)roundf(range_states[i].x);
	#else
		AVG->tag_tof_Ax[i].angle = (int16_t)roundf(angle_states[i].x);
		AVG->tag_tof_Ax[i].range = (uint16_t)roundf(range_states[i].x);
	#endif
	}
}

/**************************************************************************
函数名  ：TOF10120_Judgment
函数功能：根据当前距离值判断是否应该转向
		  0~25为警戒模式  在警戒模式下红灯亮，任意传感器距离满足条件就进行避障
入口参数：四个传感器距离 
返回  值：int 数值
**************************************************************************/ 

int TOF10120_Judgment (float T_L,float T_ML,float T_MR,float T_R)//距离判断
{
	//避障模式标志位    左旋 -3    ↙ -2   ↖ -1   跟随 0   ↗ 1   ↘2   右旋 3    
  static int Car_mode,Car_number=0,car_move=NO_ACTION,turn_number,turn_move=0,move_time=0;
	//Car_number 计数判断前方是否有障碍
	//car_move 返回值，确定车辆以何种方式避障
	//turn_move 车辆掉头方向锁死，防止其来回掉头
	//move_time 延长车辆5个判断周期的避障时间
	int  Barrier=0,Car_status=(T_L+T_ML)-(T_MR+T_R);//
	//第一步  判断警戒范围内是否有障碍
	if (T_L<37 || T_ML<37 || T_MR<37 || T_R<37)//在3次连续检测车辆前有无障碍 25  25  25  25
	{if(Car_number>=0){Car_number=0;Car_number--;}Car_number--;}
	else if(T_L>=37 && T_ML>=37 && T_MR>=37  && T_R>=37){if(Car_number<=0){Car_number=0;Car_number++;}Car_number++;}// 25 25 25 25
	if(Car_number<=-3){Car_number=0;Car_mode=1;}//警戒模式
	else if(Car_number>=3 && car_move==NO_ACTION){Car_number=0;Car_mode=0;}//退出警戒模式
  
  // 判断车辆是否需要掉头
	if(T_L<10){Barrier++;}if(T_ML<=15){Barrier++;}if(T_MR<=15){Barrier++;}if(T_R<10){Barrier++;}
	if(Barrier==4){turn_number=1;}//车辆遇到无法直接避过的障碍
	else if(Barrier<=0){turn_number=0;turn_move=0;}//当车辆前方至少两个方向无遮挡时
	
	if(Car_mode==1 && turn_move==0)
	{
		if(turn_number==1)//当车辆需要掉头时
		{ 
			if(Car_status<0){car_move=SPOT_LEFT_TURN;}//左掉头
			else if(Car_status>=0){car_move=SPOT_RIGHT_TURN;}//右掉头
		}
		else if(T_L<14 || T_ML<12 || T_MR<12 || T_R<14)//当车辆需要倒车时// 12  10  10  12 
		{
			if(Car_status<0){car_move=LEFT_BACKWARD;}//左后倒车
			else if(Car_status>=0){car_move=RIGHT_BACKWARD;}//右后倒车
			move_time=3;
		}
		else if(T_L<19 || T_ML<22 || T_MR<22 || T_R<19)//当车辆需要转弯时17  20  20  17
		{
			if(Car_status<0){car_move=LEFT_BACKWARD;}//左转
			else if(Car_status>=0){car_move=RIGHT_BACKWARD;}//右转
			move_time=3;
		}
		else if(move_time<=0){car_move=0;}//继续进行跟随
	}
	if(move_time>=0){move_time--;}//延长转弯时间防止边角碰撞
	if(car_move==SPOT_LEFT_TURN || car_move==SPOT_RIGHT_TURN){turn_move=1;}//当车辆进行掉头运动是锁死方向，防止车辆反复运动
	//_dbg_printf("/car_move:%d\n",car_move);
	return car_move;
}

/**************************************************************************
函数名  ：AOA_Control
函数功能：AOA遥控控制
入口参数：aoa  接收到的AOA参数
返回  值：int 数值 控制参数
**************************************************************************/
int AOA_Control(AOA_DATA *AVG)
{
	 uint8_t return_bit=0;
	 
	 //判断是否在遥控模式下
	 if(AVG->Aoa_para_t.mode==1)//遥控控制   
	 {
			return_bit=STOP_MOTOR;//如果下列命令无效 则停止
			if(AVG->Aoa_para_t.turn_up==1){return_bit=RCSF;}//往前
			else if(AVG->Aoa_para_t.turn_down==1){return_bit=BACKWARD;}//向后
			else if(AVG->Aoa_para_t.turn_left==1){return_bit=SPOT_LEFT_TURN;}//向左
			else if(AVG->Aoa_para_t.turn_right==1){return_bit=SPOT_RIGHT_TURN;}//向右
	 }
	_dbg_printf("lock:%d  recal:%d  mode:%d  turn_right:%d  turn_left:%d  turn_down:%d  turn_up:%d\n",AVG->Aoa_para_t.lock,AVG->Aoa_para_t.recal,AVG->Aoa_para_t.mode,
																									AVG->Aoa_para_t.turn_right,AVG->Aoa_para_t.turn_left,AVG->Aoa_para_t.turn_down,
																									AVG->Aoa_para_t.turn_up);
		
	 return return_bit;
}


/**************************************************************************
函数名  ：Oled_And_Tof_Control
函数功能：OLED读取 与 TOF数据读取与应用
入口参数：AOA距离角度 与 遥控模式
返回  值：返回TOF距离判断结果
注：由于TOF与OLED硬件冲突同时使用硬件IIC容易导致卡顿，而TOF使用模拟IIC读取有一定能读
	故使用模拟IIC设置OLED，硬件IIC读取TOF
**************************************************************************/
int Oled_And_Tof_Control(int Aoa_Ang,int Aoa_Dis,uint8_t YK_mode)
{
	int16_t Tof_bit=0;
	
	//软件OLED屏打印
	I2C_GenerateSTOP(I2C2, ENABLE);
	I2C_DeInit(I2C2);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,DISABLE);
	IIC_Init();	//软件IIC  
	OLED_ALL_Display(YK_mode);
	OLED_Follow_Data_Display(Aoa_Dis,Aoa_Ang);
	
	//硬件TOF应用
	I2C2_Configuration();//硬件IIC
	TOF250_READ(&TOF_D);
	Tof_bit=TOF10120_Judgment(TOF_D.LEFT_MM,TOF_D.MIDDLE_L_MM,TOF_D.MIDDLE_R_MM,TOF_D.RIGHT_MM);//距离判断;
//	TOF_D.LEFT_MM=100;
//	TOF_D.MIDDLE_L_MM=100;
//	TOF_D.MIDDLE_R_MM=100;
//	TOF_D.RIGHT_MM=100;
	return Tof_bit;
}

/**************************************************************************
函数名  ：AOA_Pattern_recognition
函数功能：设备跟随模式判断
入口参数：AVG  接收到的AOA参数
返回  值：当前判断的模式 
**************************************************************************/
uint8_t AOA_Pattern_recognition(AOA_DATA *AVG)
{
	static uint8_t mode=NO_FULL,mode_bit0=0,mode_bit1=0,mode_bit2=0,mode_bit3=0;
	
	if(AVG->Aoa_para_t.mode==1 && AVG->Aoa_para_t.recal==0 ){mode_bit0=0; mode_bit1++; mode_bit2=0; mode_bit3=0;}//遥控模式 
	else if(AVG->Aoa_para_t.mode==1 && AVG->Aoa_para_t.recal==1 ){mode_bit0=0; mode_bit1=0; mode_bit2++; mode_bit3=0;}//召回模式 
	else if(AVG->Aoa_para_t.mode==0 && AVG->Aoa_para_t.recal==0 ){mode_bit0=0; mode_bit1=0; mode_bit2=0; mode_bit3++;}//跟随模式 
	
	if(mode_bit1>=5){mode=Remote_mode;}//_dbg_printf("遥控\n");}
	else if(mode_bit2>=5){mode=Recall_mode;}//_dbg_printf("召回\n");}
	else if(mode_bit3>=5){mode=Follow_mode;}//_dbg_printf("跟随\n");}
	
	return mode;
}

/**************************************************************************
函数名  ：UpdateAoaData
函数功能：读取非零数据
入口参数：aoa 数据
返回  值：无
**************************************************************************/
uint8_t UpdateAoaData(General_t *aoa)
{
	
	#ifdef USE_RSSI_TAG //带信号强度
			 // 更新More_tag_tof_Ax数组（过滤0值）
		for (int i = 0; i < 4; i++) 
		{
			// 更新angle字段（int16_t）
			if (aoa->More_tag_tof_Ax[i].angle != 0) {
				AVG.More_tag_tof_Ax[i].angle = aoa->More_tag_tof_Ax[i].angle;
			}
			
			// 更新range字段（uint16_t）
			if (aoa->More_tag_tof_Ax[i].range != 0) {
				AVG.More_tag_tof_Ax[i].range = aoa->More_tag_tof_Ax[i].range;
			}
			
			// 更新rssi字段（int16_t）
			if (aoa->More_tag_tof_Ax[i].rssi != 0) {
				AVG.More_tag_tof_Ax[i].rssi = aoa->More_tag_tof_Ax[i].rssi;
			}
		}
	#else  //不带信号强度
		for (int i = 0; i < 4; i++) 
		{
			// 更新angle字段（int16_t）
			if (aoa->More_tag_tof_Ax[i].angle != 0) {
				AVG.tag_tof_Ax[i].angle = aoa->tag_tof_Ax[i].angle;
			}
			
			// 更新range字段（uint16_t）
			if (aoa->More_tag_tof_Ax[i].range != 0) {
				AVG.tag_tof_Ax[i].range = aoa->tag_tof_Ax[i].range;
			}
		}
	#endif
}

/**************************************************************************
函数名  ：Aoa_Pos_Check
函数功能：双主基站AOA，前后判断(带信号强度)
入口参数：Base_Dis 基站间距(cm)    Signal_Rato  信号强度阈值  Data_Clear 标准位清空
返回  值：Car_Sta 开机默认正前
**************************************************************************/
uint8_t Aoa_Pos_Check(float Base_Dis,float Signal_Rato,uint8_t Data_Clear)
{
	int Dist_diff;
    int Signal_diff ;
	float Distance_Threshold = Base_Dis * (2.0f/3);//判断距离为间隔的三分之二
	uint8_t  All_Rato=15,Wide_Angle_Lim=40,Car_Sta=0;  
	static signed char Dist_FB_Dir=FULL_ON,Dist_Dir=FULL_ON,Signal_Dir=FULL_ON,Angle_Dir=FULL_ON;
	static signed char Dist_bit=FULL_ON,Signal_bit=FULL_ON,Angle_bit=FULL_ON,Tag_Sta=FRONT;
	signed char A0_Angle=AVG.More_tag_tof_Ax[0].angle,
				A1_Angle=AVG.More_tag_tof_Ax[1].angle;
	
	Dist_diff = AVG.More_tag_tof_Ax[0].range-AVG.More_tag_tof_Ax[1].range;
	Signal_diff = AVG.More_tag_tof_Ax[0].rssi - AVG.More_tag_tof_Ax[1].rssi;
	
	/**************** 各个参数前后单独判断 ****************/
	//距离前后判断
	if(abs(Dist_diff)>=Distance_Threshold  && Dist_diff<0){if(Dist_bit<0){Dist_bit=0;};Dist_bit++;}//在前方
	else if(abs(Dist_diff)>=Distance_Threshold  && Dist_diff>0){if(Dist_bit>0){Dist_bit=0;};Dist_bit--;}//在后方
	
	//信号强度判断
	if(abs(Signal_diff)>=Signal_Rato &&  Signal_diff>0){if(Signal_bit<0){Signal_bit=0;};Signal_bit++;}//在前方
	else if(abs(Signal_diff)>=Signal_Rato && Signal_diff<0){if(Signal_bit>0){Signal_bit=0;};Signal_bit--;}//在后方
	
	//角度+-判断
	if(A0_Angle>0 &&  A1_Angle<0){if(Angle_bit<0){Angle_bit=0;};Angle_bit++;}//在右侧方
	else if(A0_Angle<0 &&  A1_Angle>0){if(Angle_bit>0){Angle_bit=0;};Angle_bit--;}//在左侧
	
	if(Dist_bit>=All_Rato){Dist_bit=All_Rato;Dist_Dir=FRONT;}
	else if(Dist_bit<=-All_Rato){Dist_bit=-All_Rato;Dist_Dir=BACK;}//距离差判断，前后
	
	if(Signal_bit>=All_Rato){Signal_bit=All_Rato;Signal_Dir=FRONT;}
	else if(Signal_bit<=-All_Rato){Signal_bit=-All_Rato;Signal_Dir=BACK;}//信号强度判断，前后
	
	if(Angle_bit>=All_Rato){Angle_bit=All_Rato;Angle_Dir=LEFT_SIDE;}
	else if(Angle_bit<=-All_Rato){Angle_bit=-All_Rato;Angle_Dir=RIGHT_SIDE;}//角度值判断，左右
	
//	_dbg_printf("A0  A:%d D:%d R:%d   A1  A:%d D:%d R:%d  DIR D:%d S:%d  BIT D:%d S:%d A:%d\r\n",
//		AVG.More_tag_tof_Ax[0].angle,AVG.More_tag_tof_Ax[0].range,AVG.More_tag_tof_Ax[0].rssi,
//		AVG.More_tag_tof_Ax[1].angle,AVG.More_tag_tof_Ax[1].range,AVG.More_tag_tof_Ax[1].rssi,
//		Dist_diff,Signal_diff,
//		Dist_Dir,Signal_Dir,Angle_Dir);
	
	/**************** 具体区域判断 ****************/
	//前后方确认
	if(Dist_Dir == FRONT && Signal_Dir== FRONT )//前方
	{
		Dist_FB_Dir=FRONT;
	}
	else if(Dist_Dir == BACK && Signal_Dir== BACK)//后方
	{
		Dist_FB_Dir=BACK;
	}
	
	//首先进行 前后
	if(Dist_FB_Dir== FRONT )//前方
	{
		Tag_Sta=TAG_FRONT;
	}
	else if(Dist_FB_Dir== BACK)//后方
	{
		if(abs(A1_Angle)<Wide_Angle_Lim)//在后方时 A1 基站角度较为可信
		{
			Tag_Sta=TAG_BACK;
		}
		else 
		{
			if(Angle_Dir==RIGHT_SIDE){Tag_Sta=TAG_RIGHT_BACK;}//在右侧
			else if(Angle_Dir==LEFT_SIDE){Tag_Sta=TAG_LEFT_BACK;}//在左侧
		}//在侧方
	}
	
	//根据标签方向判断处理应该处于的状态
	if(Tag_Sta==TAG_FRONT)
	{//处于前方
		Car_Sta=NO_ACTION;//不执行 跟随即可
	}
	else if(Tag_Sta==TAG_BACK)
	{//处于正后方
		Car_Sta=(Angle_Dir==LEFT_SIDE)?SPOT_LEFT_TURN:SPOT_RIGHT_TURN;//不执行 跟随即可
	}
	else if(Tag_Sta==TAG_RIGHT_BACK || Tag_Sta==TAG_LEFT_BACK)
	{//处于 左后方 或者 右后方
		Car_Sta=(Tag_Sta==TAG_LEFT_BACK)?SPOT_LEFT_TURN:SPOT_RIGHT_TURN;//不执行 跟随即可
	}
//	_dbg_printf("A0  A:%d D:%d R:%d   A1  A:%d D:%d R:%d  BIT D:%d S:%d A:%d, Car_Sta:%d\r\n",
//		AVG.More_tag_tof_Ax[0].angle,AVG.More_tag_tof_Ax[0].range,AVG.More_tag_tof_Ax[0].rssi,
//		AVG.More_tag_tof_Ax[1].angle,AVG.More_tag_tof_Ax[1].range,AVG.More_tag_tof_Ax[1].rssi,
//		Dist_Dir,Signal_Dir,Angle_Dir,Car_Sta);
////	
	if(Data_Clear==1)
	{//重置数据
		Dist_FB_Dir=FULL_ON;Dist_Dir=FULL_ON;Signal_Dir=FULL_ON;Angle_Dir=FULL_ON;
		Dist_bit=FULL_ON;Signal_bit=FULL_ON;Angle_bit=FULL_ON;Tag_Sta=FRONT;
	}
//	_dbg_printf("A0  A:%d D:%d R:%d   A1  A:%d D:%d R:%d  DIR D:%d S:%d  BIT D:%d S:%d A:%d Car_Sta:%d\r\n",
//		AVG.More_tag_tof_Ax[0].angle,AVG.More_tag_tof_Ax[0].range,AVG.More_tag_tof_Ax[0].rssi,
//		AVG.More_tag_tof_Ax[1].angle,AVG.More_tag_tof_Ax[1].range,AVG.More_tag_tof_Ax[1].rssi,
//		Dist_diff,Signal_diff,
//		Dist_Dir,Signal_Dir,Angle_Dir,Car_Sta);
	return Car_Sta;
}

/**************************************************************************
函数名  ：AOA2_DATA_JUDGE
函数功能：判断其他基站是否有效,该参数根据实际需求应用。
入口参数：AVG 所有基站的参数 
          Test_N 计数上限
          An_N   使用基站个数(最大4个)
返回  值：return STOP_MOTOR 或 NO_ACTION  建议停车
**************************************************************************/
uint8_t AOA2_DATA_JUDGE(General_t *aoa,uint8_t Test_N,uint8_t Base_N) 
{
    static int AOA_N[5]= {0};
    static int prev_values[5] = {0}; // 新增静态数组存储前值
    int i, AOA=0,An_N;
	uint16_t meas_range;
	
	An_N=(Base_N>=4)?4:Base_N;
	
	 for(i=0; i<An_N; i++) 
	{
		#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
			meas_range = aoa->More_tag_tof_Ax[i].range;
		#else
			meas_range = aoa->tag_tof_Ax[i].range;
		#endif
//		if(meas_range == prev_values[i] ) {AOA_N[i]++;} 
		if(meas_range == 0 ) {AOA_N[i]++;} 
		else {AOA_N[i] = 0;   prev_values[i] = meas_range;}// 更新前值,并重新计数
		
	}
	
    // 循环判断逻辑修正
    for(i=0; i<An_N; i++) 
	{
        if(AOA_N[i] >= Test_N) {AOA++;}
    }
	
//	_dbg_printf("A0  A:%d D:%d    A1  A:%d D:%d    A2  A:%d D:%d \r\n",
//					AVG.More_tag_tof_Ax[0].angle,AVG.More_tag_tof_Ax[0].range,
//					AVG.More_tag_tof_Ax[1].angle,AVG.More_tag_tof_Ax[1].range,
//					AVG.More_tag_tof_Ax[2].angle,AVG.More_tag_tof_Ax[2].range);
	
    if(AOA != 0){return STOP_MOTOR;} else {return NO_ACTION;}
}

/**************************************************************************
函数名  ：follow_car_task
函数功能：1.判断A0基站是否有数据
          2.过滤数据，去除跳变大的数据
          3.对数据进行初步分析，判断标签所在位置
入口参数：无
返回  值：基站是否有数据
**************************************************************************/
uint8_t follow_car_task(void)
{
	Message msg;
	General_t aoa;
	//数据缓冲
	static uint8_t Buffer_index=0;          // 缓冲索引
	static uint8_t data_buffer[BUFFER_SIZE] = {0}; // 数据缓冲池
	uint8_t Valid_count = 0,Valid_Bit=20;
	
	if(get_AOA_data(&msg) == true)
	{
		if(AOA_Car_Hex_Resolution(msg.buf,&aoa))//数据解析 
		{
			// 更新缓冲池（最新数据置1）
			data_buffer[Buffer_index] = 1;  Buffer_index = (Buffer_index + 1) % BUFFER_SIZE;
			
			AVG.Inactive_BS=AOA2_DATA_JUDGE(&aoa,50,2);//判断其余基站是否有效

			UpdateAoaData(&aoa);//更新AOA 角度值

			AOA_Angle_Filter(&AVG,60,35,25);//角度跳变过滤

			kalman_filter(&AVG);//简单的卡尔曼滤波
			
			memcpy(&AVG.Aoa_para_t, &aoa.para_t, sizeof(aoa.para_t));//复制遥控值
		}
	}
	else 
	{
		// 无数据时填充0
		data_buffer[Buffer_index] = 0;   Buffer_index = (Buffer_index + 1) % BUFFER_SIZE;
	}
	
//U1-AOA-数据 33HZ左右   follow_car_task 10ms运行一次，即100HZ
//data_buffer缓冲数组50个数据，即时间跨度500ms。
//理论上有16个数据， 当缓冲区内有 10个有效数据以上时，表明数据正常接收
	for(uint8_t i=0; i<BUFFER_SIZE; i++){Valid_count += data_buffer[i];}//统计有效数据个数
	
	if(Valid_count >= Valid_Bit ? 1 : 0)
	{
		AVG.Remote_control=AOA_Control(&AVG);//遥控按钮判断
		
		//多基站判断

		AVG.Basic_Directions=Aoa_Pos_Check(35,3,0);//前后判断
		
		if(AVG.Inactive_BS == STOP_MOTOR){AVG.Basic_Directions = NO_ACTION ;}
		
//		
		if(AVG.Inactive_BS==STOP_MOTOR)//多基站的情况下，有任意基站下线，车辆停止
		{
			AVG.Car_mode=NO_FULL;
//			_dbg_printf("STOP_MOTOR\r\n");
		}
		else 
		if(AVG.Aoa_para_t.mode==1)//判断遥控模式
		{
//			AVG.Car_mode=AOA_Pattern_recognition(&AVG);//遥控模式判断
			AVG.Car_mode = Remote_mode;
//			_dbg_printf("Remote_mode\r\n");
		}
		else if(AVG.Aoa_para_t.mode==0)//其余标签 自动进入跟随模式
		{
			AVG.Car_mode=Follow_mode;
//			_dbg_printf("Follow_mode\r\n");
		}
//		_dbg_printf("MOD:%d  RC:%d   CM:%d   FB:%d   BS:%d\r\n",AVG.Aoa_para_t.mode,AVG.Remote_control,AVG.Car_mode,AVG.Basic_Directions,AVG.Inactive_BS);
//		_dbg_printf("1Valid_Bit:%d   Valid_count:%d\r\n",Valid_Bit,Valid_count);
	}
	else 
	{
		AVG.Car_mode=NO_FULL;//无数据
		Aoa_Pos_Check(0,0,1);//数据清空
//		_dbg_printf("2Valid_Bit:%d   Valid_count:%d\r\n",Valid_Bit,Valid_count);
	}
	return Valid_count >= Valid_Bit ? 1 : 0; // 100ms 内有3个以上的数据
}





/**************************************************************************
函数功能：读取AOA参数并根据参数进行控制
入口参数：无
返回  值：无
作    者：WHEELTEC
**************************************************************************/
void Read_AoA_Control(void)
{
	static uint8_t Dodge_mark;
	static uint64_t Move_Time=0,Motor_Time=0,LED_time;
	//数据处理 10ms 运行一次
	if(portGetTickCnt()-Motor_Time>=10000)//100ms 判断一次
	{
		Motor_Time=portGetTickCnt();//时间记录
		if(follow_car_task())
		{
			LED_time++;
			if(LED_time%50==0){LED2(ON);}else {LED2(OFF);}//正常运行时 LED2闪烁
		}
		else{LED2(OFF);}
	}
	//电机控制与OLED打印 50ms控制一次
	if(portGetTickCnt()-Move_Time>=50000)//40ms 控制一次电机
	{
		Move_Time=portGetTickCnt();//时间记录

#ifdef USE_RSSI_TAG //根据数据是否有信号强度，选择数据源
		AVG.Distance_filter=AVG.More_tag_tof_Ax[0].range;
		AVG.Angle_filter=AVG.More_tag_tof_Ax[0].angle;
#else  //不带信号强度
		AVG.Distance_filter=AVG.tag_tof_Ax[0].range;
		AVG.Angle_filter=AVG.tag_tof_Ax[0].angle;
#endif

		switch(AVG.Car_mode)//模式选择
		{
			case NO_FULL://标签无数据无数据
//				_dbg_printf("NO_FULL \r\n");
				CAR_STOP();
			break;
			
			case Lock_mode://锁模式// U1 AOA无锁功能 故去除
//				_dbg_printf("Lock_mode \r\n");
			break;
			
			case Follow_mode://跟随模式
				AVG.Car_Speed=Follow_speed;
				Car_Pwm_Direction(150,10,AVG.Car_Speed);
//				_dbg_printf("Follow_mode \r\n");
			break;
			
			case Recall_mode://召回模式
//				_dbg_printf("Recall_mode \r\n");
			break;
			
			case Remote_mode://遥控模式
				AVG.Car_Speed=Rcsf_speed;
				Preset_State_Of_Motor(AVG.Remote_control);
//				_dbg_printf("Remote_mode \r\n");
			break;
			
			default:break;
		}
		AVG.Tof_Directions=Oled_And_Tof_Control(AVG.Angle_filter,AVG.Distance_filter,AVG.Car_mode);
		if(AVG.Tof_Directions != 0){LED1(ON);}else {LED1(OFF);}//前方有障碍时LED1开启
	}
}








