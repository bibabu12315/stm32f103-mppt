#ifndef __ADC_H
#define __ADC_H	
#include "sys.h"

//4个通道, 每个通道300个采样
#define ARRAYSIZE 4*300
extern u16 ADCConvertedValue[ARRAYSIZE];
void ADCx_InitConfig(void);
#endif 
