/*
 ********************************************************************************
 *
 *                                 osal.h
 *
 * File          : osal.h
 * Version       : V1.0
 * Author        : tony
 * Mode          : Thumb2
 * Toolchain     : 
 * Description   : 
 *                
 * History       :
 * Date          : 2013.07.22
 *******************************************************************************/
 
#ifndef __OSAL_H_
#define __OSAL_H_

#ifdef __cplusplus
extern "C"
}
#endif

/**************************************************************************************************
 * 																				INCLUDES
 **************************************************************************************************/
#include <stdbool.h>
#include "OSAL_Comdef.h"



/**************************************************************************************************
 * 																				CONSTANTS
 **************************************************************************************************/
#define ST_CHIP

#define QUEUE_MSG_LEN  (1024)
#define QUEUE_MSG_MAX  (3)

typedef struct{
	uint8_t  flag;
	uint16_t len;
	uint8_t  buf[QUEUE_MSG_LEN];
}Message;


typedef struct
{
	Message pMsg[QUEUE_MSG_MAX];
	int front;    
	int rear;    
	int maxsize; 
	int count;    // 添加：当前元素个数
}QUEUE,*PQUEUE;


/***************************************************************************************************
 * 																				TYPEDEF
 ***************************************************************************************************/

/***************************************************************************************************
 * 																				GLOBAL VARIABLES
 ***************************************************************************************************/


/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/	
extern void osal_CreateQueue(PQUEUE Q,int maxsize);
extern void osal_TraverseQueue(PQUEUE Q);
extern bool osal_FullQueue(PQUEUE Q);
extern bool osal_EmptyQueue(PQUEUE Q);
extern bool osal_Enqueue(PQUEUE Q, uint8_t flag, uint8_t *buf, uint16_t len);
extern bool osal_Dequeue_Ptr(PQUEUE Q, Message **val_ptr);

extern void osal_itoa (unsigned int n,char s[]);
extern void osal_Str2Byte(const uint8_t* source, uint8_t* dest, int sourceLen);  
extern void osal_Hex2Str( const char *sSrc,  char *sDest, int nSrcLen );
extern int  osal_strstr(char* str1, char str2, int same_cnt);
extern int  osal_memstr(char* str1, int str1_len, char *str2, int str2_len);
extern int  osal_memch(char* str1, int str1_len, int start_idx, char ch);
extern int  osal_muldata(uint8_t *buf, char str1, int str1_cnt, char str2, int str2_cnt);
extern int osal_finddata(uint8_t *buf, uint8_t *find_data, int buf_len, int find_len);


#ifdef __cplusplus
}
#endif
 
#endif//__OSAL_H_
