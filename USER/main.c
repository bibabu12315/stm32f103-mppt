#include "main.h"


void dwt_init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	//DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t dwt_cycle(void)
{
	return DWT->CYCCNT;
}

void GPIO_TestPin_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启 GPIOA 时钟（PA15 属于 GPIOA）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 PA15 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 默认置低
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
}


int32_t test_count = 0; // 测试计时

int main(void)
{
	
	GPIO_TestPin_Config();
	delay_init(); // 延时函数初始化
	dwt_init();	  // DWT计数器初始化
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
				//GPIO_SetBits(GPIOA, GPIO_Pin_15);
				//DWT->CYCCNT = 0;  
				//DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
				Cal_IO_Para();
				//test_count = dwt_cycle(); // 读取DWT计数器
				//GPIO_ResetBits(GPIOA, GPIO_Pin_15);
			}
			// 否则要占空比调节算法
			else
			{
				// 输入输出的adc采样及滤波
				Cal_IO_Para();
				// 占空比调节算法
				//GPIO_SetBits(GPIOA, GPIO_Pin_15);
				//delay_ms(1);  // 延时1ms，用于示波器测算法时长
				DWT->CYCCNT = 0;  
				DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
				PWM_Adjust(); // 100ms调用一次算法
				test_count = dwt_cycle(); // 读取DWT计数器
				//GPIO_ResetBits(GPIOA, GPIO_Pin_15);
			}
		}

		if (OLEDShowCnt > 300) // 300m刷新一次
		{
			OLEDShowCnt = 0;
			Normal_Display();
		}
	}
}
