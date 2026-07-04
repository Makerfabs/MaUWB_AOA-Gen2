
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "OSAL.h"

/***********************************************
Function: Create a empty stack;
************************************************/
void osal_CreateQueue(PQUEUE Q,int maxsize)
{
#ifdef ST_CHIP
#else
	Q->pMsg=(Message *)malloc(sizeof(Message)*maxsize);
	if(NULL==Q->pMsg)
	{
		_dbg_printf("Memory allocation failure");
		while(1);
	}
#endif
	Q->front=0;         
	Q->rear=0;
	Q->maxsize=maxsize;
	Q->count = 0;  // 初始化
}


bool osal_FullQueue(PQUEUE Q)
{
	return Q->count == Q->maxsize;
}

bool osal_EmptyQueue(PQUEUE Q)
{
	return Q->count == 0;
}

bool osal_Enqueue(PQUEUE Q, uint8_t flag, uint8_t *buf, uint16_t len)
{
	if(osal_FullQueue(Q)){
		return false;
	}
	else
	{
		Q->pMsg[Q->rear].flag = flag;
		Q->pMsg[Q->rear].len = len;
		memcpy(Q->pMsg[Q->rear].buf, buf, len);
		Q->rear=(Q->rear+1)%Q->maxsize;
		Q->count++;  // 计数加1
		return true;
	}
}


bool osal_Dequeue_Ptr(PQUEUE Q, Message **val_ptr)
{
	if(osal_EmptyQueue(Q) || val_ptr == NULL) {
		if (val_ptr) *val_ptr = NULL;
		return false;
	}
	
	// 直接返回队列中元素的指针，不拷贝
	*val_ptr = &Q->pMsg[Q->front];
	
	Q->front = (Q->front + 1) % Q->maxsize;
	Q->count--;  // 计数减1
	
	// 长度检查
	if((*val_ptr)->len > QUEUE_MSG_LEN){
		_dbg_printf("error queue msg, len=%d\n", (*val_ptr)->len);
		*val_ptr = NULL;  // 清空指针
		return false;
	}
	
	return true;
}






void osal_itoa (unsigned int n,char s[])
{
	int i,j;
	char sw_data[32];
	//if((sign=n)<0)//
	//n=-n;//
	i=0;
	do{
		s[i++]=n%10+'0';//
	}
	while ((n/=10)>0);//

	s[i]='\0';
	for(j=i;j>=0;j--){//
		sw_data[i-j] = s[j];
	}
	for(j=1;j<=i;j++){//
		s[j-1] = sw_data[j];
	}	  
}


void osal_Str2Byte(const uint8_t* source, uint8_t* dest, int sourceLen)  
{
	short i;
	unsigned char highByte, lowByte;

	for (i = 0; i < sourceLen; i += 2)
	{
		highByte = toupper(source[i]);
		lowByte  = toupper(source[i + 1]);

		if (highByte > 0x39)
			highByte -= 0x37;
		else
			highByte -= 0x30;

		if (lowByte > 0x39)
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
}  


void osal_Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )  
{  
	int  i;  
	char szTmp[3];  

	for( i = 0; i < nSrcLen; i++ )  
	{  
		sprintf( szTmp, "%02X", (uint8_t) sSrc[i] );  
		memcpy( &sDest[i * 2], szTmp, 2 );  
	}  
}

int osal_strstr(char* str1, char str2, int same_cnt)
{
	int same_idx = 0;
	int idx = 0;
	char* p = str1;
	while (*p)
	{
		char* s1 = p;
		if(*s1 && *s1 == str2)
		{
//			s1++;
			same_idx++;
		}
		if (same_idx == same_cnt)
		{
			return idx;
		}
		p++;
		idx++;
	}
	return -1;
}

int osal_memstr(char* str1, int str1_len, char *str2, int str2_len)
{
	int src_idx = 0;
	int remain_len = str1_len - src_idx;
	while(src_idx < str1_len && remain_len >= str2_len)
	{
		if(memcmp(str1+src_idx, str2, str2_len) == 0)
		{
			return src_idx;
		}
	
		src_idx++;
		remain_len = str1_len - src_idx;
	}
	
	return -1;
}

int osal_memch(char* str1, int str1_len, int start_idx, char ch)
{
	int src_idx = start_idx;
	while(src_idx < str1_len)
	{
		if(*(str1+src_idx) == ch)
		{
			return src_idx;
		}
	
		src_idx++;
	}
	
	return -1;
}


int osal_muldata(uint8_t *buf, char str1, int str1_cnt, char str2, int str2_cnt)
{
	int str1_idx = osal_strstr(buf, str1, str1_cnt);
	int str2_idx = osal_strstr(buf, str2, str2_cnt);

	if(str1_idx != -1 && str2_idx != -1)
	{
		if((str2_idx - str1_idx) > 1)
		{
			return str1_idx;
		}
	}
	return -1;
}


int osal_finddata(uint8_t *buf, uint8_t *find_data, int buf_len, int find_len)
{
	if(buf == NULL || find_data == NULL || buf_len < find_len || find_len <= 0) 
		return -1;

	for(int i = 0; i <= buf_len - find_len; i++) {
		if(memcmp(&buf[i], find_data, find_len) == 0) {
			return i;  // 返回找到的位置
		}
	}
	return -1;  // 未找到
}



