#ifndef __ENCODER_H
#define __ENCODER_H	 
#include "sys.h"

#define PINA  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0)//旋转编码器A
#define PINB  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)//旋转编码器B
#define Bmq_Button  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)//旋转编码器按键

//限制最大最小值 保护
#define VO_MAX 500 //输出电压最大值 50V
#define VO_MIN 30 //输出电压最小值 3V
#define IO_MAX 100 //输出电流最大值 10A
#define IO_MIN 1 //输出电流最小值 0.1A


void Encoder_Init(void);	

#endif
