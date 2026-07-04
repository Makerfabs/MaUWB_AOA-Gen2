#ifndef __OLED_I2C_H
#define	__OLED_I2C_H

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include "stm32f10x.h"
#include "OSAL_Clock.h"
#include "stm32f10x_i2c.h"
#include "delay.h"
/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/		
#define OLED_ADDRESS	0x78 //通过调整0R电阻,屏可以0x78和0x7A两个地址 -- 默认0x78


/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/
extern const unsigned char F6x8[][6];
extern const unsigned char F8X16[];
extern unsigned char F16x16[];

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	
void OLED_Configuration(void);
void I2C_WriteByte(uint8_t addr,uint8_t data);
void WriteCmd(unsigned char I2C_Command);
void WriteDat(unsigned char I2C_Data);
void OLED_Init(void);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char fill_Data);
void OLED_CLS(void);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);

//OLED 屏幕打印
void OLED_Follow_Data_Display(int New_distance,int New_Angle);
void OLED_ALL_Display(uint8_t All_mode);
void OLED_Remote_Display(uint8_t OLED_N);
void OLED_display(int word);


#endif
