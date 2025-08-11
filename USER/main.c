#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "core.h"
#include "adc.h"
#include "pwm.h"
#include "oled.h"
#include "display.h"
#include "encoder.h"

int main(void)
{
	delay_init(); // 延时函数初始化
	SystemInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	OLED_Init();	 // OLED液晶初始化
	Display_Heart(); // 液晶显示固定部分
	BUCK_SDIO_Init();
	ADCx_InitConfig();
	Encoder_Init();

	TIM1_PWM_Init(720, 1); // 72Mhz的计数频率，计数到720为100kHZ
	while (1)
	{
		if (OLEDShowCnt > 300) // 300m刷新一次
		{
			OLEDShowCnt = 0;
			Normal_Display();
		}
	}
}
