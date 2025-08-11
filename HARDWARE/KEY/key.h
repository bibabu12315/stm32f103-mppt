#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"
	 

#define KEY_CUR  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)//读取按键1
#define KEY_ON_OFF  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)//读取按键2
#define KEY_MODE  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3)//读取按键3
#define KEY_BAT_V  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)//读取按键4

#define KEY_CUR_PRES	1		
#define KEY_ON_OFF_PRES	2	 
#define KEY_MODE_PRES	3
#define KEY_BAT_V_PRES	4		


void KEY_Init(void);//IO初始化
u8 KEY_Scan(u8 mode);  	//按键扫描函数					    
#endif
