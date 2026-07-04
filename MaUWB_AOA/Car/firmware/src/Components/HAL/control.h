#include "stm32f10x.h"
#include "OSAL_Clock.h"


/***************************************************************************************************
 * 跟随宏定义
 ***************************************************************************************************/
#define BUFFER_SIZE 100      //缓冲数组大小
//由于 TOF测距 与 OLED硬件IIC冲突 二者只能选其一
#define OLED_OR_TOF    0//0 只是用 OLED    1 只使用 TOF

typedef enum {
	
    FULL_ON      = 0,  // 空未有状态
	//方向标志位判断
    FRONT        = 1,  // 正前方    1
    BACK,              // 正后方    2
	SIDE,              // 侧方      3
    LEFT_SIDE,         // 左侧边    4
    RIGHT_SIDE,        // 右侧边    5
    FRONT_HALF,        // 前半部    6
    REAR_HALF,         // 后半部    7
	
	//标签位置判断
    TAG_FRONT,         // 正前方                                          8
    TAG_BACK,          // 正后方                                          9
    TAG_LEFT_FRONT,    // 左前方（ LEFT_SIDE 与 FRONT_HALF 的结合方向）   10
    TAG_LEFT_BACK,     // 左后方（ LEFT_SIDE 与 REAR_HALF 的结合方向）    11
    TAG_RIGHT_FRONT,   // 右前方（ RIGHT_SIDE 与 FRONT_HALF 的结合方向）  12
    TAG_RIGHT_BACK,    // 右后方（ RIGHT_SIDE 与 REAR_HALF 的结合方向）   13
} Tag_Position;//标签位置判断 标志位定义

typedef enum {
	NO_ACTION = 0,          // 不执行
    RCSF = 1,               //遥控直行        1
    FOLLOW_STRAIGHT,        //跟随直行        2
    STOP_MOTOR,             //停止            3
    BACKWARD,               // 后退           4
    SPOT_LEFT_TURN,         // 原地左转       5
    SPOT_RIGHT_TURN,        // 原地右转       6
    LEFT_TURN,              //左转            7
    RIGHT_TURN,             //右转            8
    FOLLOW_CAR,             //跟随            9
    LEFT_BACKWARD,          //左倒车          10
    RIGHT_BACKWARD,         //右倒车          11
} TurnDirection;//定义转向指令的枚举

typedef enum {
    NO_FULL = 0,          //空              0
    Lock_mode,            //锁模式          1
    Follow_mode,          //跟随模式        2
    Recall_mode,          //召回模式        3
    Remote_mode,          //遥控模式        4
} CarMode;//车辆模式 根据遥控设置，如果是其它标签模式只为跟随模式

typedef struct {
    float x;  // 状态估计值
    float p;  // 估计误差协方差
    float q;  // 过程噪声协方差（建议值0.01-0.1）
    float r;  // 测量噪声协方差（建议值0.5-2.0）
} KalmanState;
/***************************************************************************************************
 * 跟随数据
 ***************************************************************************************************/
#pragma pack(push, 1)


typedef struct
{
	uint32_t is_lowbattery:1;			//是否低电量报警
	uint32_t is_alarm:1;				//是否报警
	uint32_t is_chrg:1;					//是否充电
	uint32_t is_tdby:1;					//是否充满
	uint32_t battery_val:10;			//电池电压350=3.50V
	uint32_t is_offset_range_zero_bit:1;//距离校正位
	uint32_t is_offset_pdoa_zero_bit:1;	//角度校正位

	uint32_t turn_up:1;					//(遥控专用)向前
	uint32_t turn_down:1;				//(遥控专用)向后
	uint32_t turn_left:1;				//(遥控专用)向左
	uint32_t turn_right:1;			    //(遥控专用)向右
	uint32_t mode:3;					//(遥控专用)模式
	uint32_t recal:1;					//(遥控专用)召回
	uint32_t lock:1;					//(遥控专用)上锁
	
	uint32_t dev_type:3;				//设备类型(0:学习板 1:手环 2:遥控器)
	uint32_t reserve:4;					//预留
}Aoa_Detail_Para_t;//遥控命令结构体

typedef struct
{
	int      Angle_filter;    //最终角度
	int      Distance_filter; //最终距离

	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
	}tag_tof_Ax[4];//AOA带信号强度的数据
	
	struct{
		int16_t angle;					//角度(°)
		uint16_t range;					//距离(cm)
		int16_t  rssi;					//信号强度
	}More_tag_tof_Ax[4];//AOA 不带信号强度的数据
	
	Aoa_Detail_Para_t  Aoa_para_t;//遥控参数
	
	uint8_t Remote_control;//遥控按键控制 表示哪一个按键按下
	uint8_t Car_mode;      //遥控模式
	uint8_t Inactive_BS;   //基站是否有效标志位(多基站时使用)
	
	uint8_t Tag_LR;//多基站标签方向判断  在左还是在右，判断车辆需要向右还是是向左转
	uint8_t Basic_Directions; //标签位置
	uint8_t Tof_Directions;   //避障位置
	
	uint16_t Car_Speed;
	
}AOA_DATA;
#pragma pack(pop)

extern AOA_DATA AVG;

/***************************************************************************************************
 * 跟随控制函数
 ***************************************************************************************************/
//void follow_car_task(void);
void Read_AoA_Control(void);
void car_motor_speed (int distance);

int AOA_Angle_difference(int Angle);
int AOA_final_angle (int Angle_difference,int Angle );
int car_distance (int AOA_distance);