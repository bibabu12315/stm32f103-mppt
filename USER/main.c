#include "main.h"

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
		if (Sys_Check_Cnt >= 5) // 5ms调用一次
		{
			Sys_Check_Cnt = 0;

			// 输出短路保护
			ShortOff();
			// 输出软件过流函数
			SwOCP();
			// 输出软件过压保护函数
			VoutSwOVP();
			// 输入欠压保护
			VinSwUVP();
			// 输入过压保护
			VinSwOVP();
			// 输出过功率保护
			VoutOverpower();

			// 电源状态机函数
			Buck_StateM();
		}
			// 执行mppt算法
		if (Sys_Mppt_Cnt >= 100) // 100ms调用一次
		{
			Sys_Mppt_Cnt = 0;
			// 如果关闭了LM74700的PWM通道，则只采集数据，不做算法
			if (DF.Buck_PWMENFlag == 0)
			{
				// 输入输出的adc采样及滤波
				Cal_IO_Para();
			}
			// 否则要占空比调节算法
			else
			{
				// 输入输出的adc采样及滤波
				Cal_IO_Para();
				// 占空比调节算法
				PWM_Adjust(); // 100ms调用一次算法
			}
		}

		if (OLEDShowCnt > 300) // 300m刷新一次
		{
			OLEDShowCnt = 0;
			Normal_Display();
		}
	}
}
