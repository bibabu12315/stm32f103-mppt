#include "pwm.h"

void BUCK_SDIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE); // 使能B端口时钟

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // 关闭jtag，使能SWD，可以用SWD模式调试
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 速度50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);			  // 初始化GPIOB15.11
	DISABLE_OUT;
}

// 输出PWM波
// arr：自动重装值。
// psc：时钟预分频数
void TIM1_PWM_Init(u16 arr, u16 psc)
{
	// 1、初始化结构体
	// 使用到GPIO、TIM基本配置、比较输出、死区配置；
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
	// 2、使能时钟，GPIO,TIM1，复用功能，
	// 时钟使能，管脚Rmap
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);
	// GPIO_PinRemapConfig(GPIO_PartialRemap_TIM1, ENABLE);
	TIM_DeInit(TIM1);
	TIM_InternalClockConfig(TIM1);
	// 3、配置引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// 4、 TIM基础设置；
	// 设置计数频率为72M/1；即每秒中计数值计数72M；
	// Period设置为719，即计数够720清零，对应PWM频率为72M/720=100Khz；
	TIM_TimeBaseStructure.TIM_Period = arr - 1;				// 周期为计数720次，计数频率为72M/720；
	TIM_TimeBaseStructure.TIM_Prescaler = psc - 1;			// 设置频率为72M/1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 不分频，时钟72M
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	// 5、设置输出比较
	// Pulse为跳变计数值，当计数值到达200时，PWM由高电平跳变到低电平，设置Pulse/Period即设置了占空比；
	// 在主程序中可以调用TIM_SetCompare1(TIM1,psc);动态设置Pulse的值；
	// 定时器比较输出配置
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;			  // PWM1模式
	TIM_OCInitStructure.TIM_Pulse = 50;							  // 占空比50/720=6.94%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	  // 设置主通道 CH1 输出极性为高_（CNT < CCR1）时输出为 高电平；
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;  // 空闲状态下输出为低电平
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出比较 CH1
	// 高级定时器才有的互补输出配置
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;		// 设置互补通道 CH1N 输出极性为高_（CNT < CCR1）时输出为 高电平；
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable; // 使能互补通道 CH1N 输出
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;	// 互补通道 CH1N 空闲状态下输出为低电平
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);

	/*6、设置死区
	本项目要驱动IR2101晶闸管，死区时间设置为500ns，此处TIM_DeadTime =36；
	死区时间设置要设置UTG[7：0]位；
	图中T_DTS为系统时钟周期：T_DTS=1/(时钟频率)=1/72M=13.9ns；[7：5]=0XX;
	当TIM_DeadTime 写入范围为[0，127]时，死区时间=[7：0] X T_DTS=[0，1764ns]

	当TIM_DeadTime 写入范围为[128，191]时，[7：5]=10X;
	死区时间=(64+[5:0]) x 2 x 13.89=（64+0)~(64+63)x2x13.89=[1777.9，3528.88]

	当TIM_DeadTime 写入范围为[192，223]时，[7:5]=110;
	死区时间=(32+[4:0]) X 8 X 13.89=(32+0)~(32+31) X 8X13.89=[3555.84，7000.56 ]

	TIM_DeadTime 写入范围为[224，255]时，[7:5]=111;
	死区时间=(32+[4:0])X 16 X13.89=(32+0)~(32+31) X 16X 13.89=[7111.68，140001.12]*/
	// 高级定时器死区配置

	/* 配置当 运行模式下（运行中但主输出关闭） 的输出状态；
	 * Disable 表示：主输出关闭时（MOE=0），通道输出立即变为空闲状态；
	 * Enable 表示：主输出关闭时，仍然保持最后的输出状态（直到停止）
	 * 一般设为 Disable 较为安全，防止“悬浮态”  */
	TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Disable;
	/* 配置当 空闲模式下（定时器未使能） 输出的状态；
	 * Disable 表示：输出为预设的空闲状态（见 OCIdleState、OCNIdleState）；
	 * Enable 表示：保持最后的输出状态直到定时器重新启动
	 * 常用设为 Disable，即 MCU复位后输出是低电平，更安全 */
	TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Disable;
	TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;		  // 用于锁定定时器配置，防止随意修改  OFF 表示不锁定，所有 BDTR 配置项可以随时修改
	TIM_BDTRInitStructure.TIM_DeadTime = 36;						  // 互补pwm输出之间的死区时间；单位是定时器的时钟周期  如上所述，36个时钟周期=500ns  36 / 72MHz = 0.5 ns（约）
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;			  // 关闭刹车功能使能 刹车保护功能（用于故障保护：如过流、过温） Enable：启用后外部信号可通过 BKIN 脚触发关闭 PWM 输出（MOE=0）。
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High; // 设置 BKIN 脚输入为高电平还是低电平触发刹车。High 表示高电平触发刹车
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
	// 7、 开启CCR和ARR，使能TIM1，
	// ccr1自动重装载
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM1, ENABLE);

	TIM_Cmd(TIM1, ENABLE);
	// 高级定时器才有的主输出使能，不开启不能用;

	TIM_CtrlPWMOutputs(TIM1, DISABLE);
}
