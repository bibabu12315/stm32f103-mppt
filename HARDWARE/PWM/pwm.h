#ifndef __PWM_H
#define __PWM_H
#include "sys.h"

#define ENABLE_OUT GPIO_SetBits(GPIOB, GPIO_Pin_11)    // 控制放倒灌LM74700使能，开启pwm通道
#define DISABLE_OUT GPIO_ResetBits(GPIOB, GPIO_Pin_11) // 控制放倒灌LM74700使能，关闭pwm通道

void BUCK_SDIO_Init(void);
void TIM1_PWM_Init(u16 arr, u16 psc);

#endif
