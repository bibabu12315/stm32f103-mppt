#include "timer.h"
#include "core.h"

// arr：自动重装值。
// psc：时钟预分频数
void TIM2_Int_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // 时钟使能

	// 定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr - 1;					// 设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc - 1;				// 设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);				// 根据指定的参数初始化TIMx的时间基数单位
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					// 使能指定的TIM3中断,允许更新中断

	// 中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;			  // TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		  // 从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);							  // 初始化NVIC寄存器

	TIM_Cmd(TIM2, DISABLE); // 失能TIM2
}

// 定时器2中断服务程序
void TIM2_IRQHandler(void) // TIM3中断25KHZ 40us
{
	//	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //检查TIM2更新中断发生与否
	//		{
	//			TIM_ClearITPendingBit(TIM2, TIM_IT_Update  );  //清除TIMx更新中断标志
	//			IO_Para.ADC_SAMPLE_OVER_FLAG=TRUE;
	//			ADCSample();
	//		}
}

/*******************************非阻塞式延时函数Delay*************************************/
volatile uint32_t sysTickMillis = 0; // 系统毫秒计数
// SysTick 中断服务函数（1ms触发一次）
void SysTick_Handler(void)
{
	sysTickMillis++;
}

// 初始化 SysTick 定时器，1ms中断
void SysTick_Init(void)
{
	// SystemCoreClock 是系统时钟频率，72MHz一般情况下
	if (SysTick_Config(SystemCoreClock / 1000))
	{
		// 配置失败，进入死循环
		while (1)
			;
	}

	// 配置 SysTick 优先级，数值越小优先级越高
	NVIC_SetPriority(SysTick_IRQn, 1); // 优先级1，适中较高
}

// 获取当前毫秒计数
uint32_t GetSysTick(void)
{
	return sysTickMillis;
}
