#include "encoder.h"
#include "delay.h"
#include "core.h"
#include "display.h"
#include "pwm.h"

// 外部中断初始化函数
void EXTIX_Init(void)
{

	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 外部中断，需要使能AFIO时钟

	// GPIOB.1 中断线以及中断初始化配置
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 下降沿触发
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure); // 根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;		  // 使能按键所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		  // 子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // 使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);							  // 根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}
// 旋转编码器扫描
// arr：自动重装值。
// psc：时钟预分频数
void TIM3_Int_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); // 时钟使能

	// 定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr - 1;					// 设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc - 1;				// 设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);				// 根据指定的参数初始化TIMx的时间基数单位
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);					// 使能指定的TIM3中断,允许更新中断

	// 中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;			  // TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		  // 从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);							  // 初始化NVIC寄存器

	TIM_Cmd(TIM3, ENABLE); // 使能TIMx
}

// 旋转编码器初始化函数
// 初始化相关IO口
// 初始化按键检测外部中断
// 初始化编码器旋转检测定时器
void Encoder_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置成上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);		  // 初始化

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置成上拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);		  // 初始化

	EXTIX_Init();
	TIM3_Int_Init(10, 7200); // 10Khz的计数频率，10为1ms
}

// 外部中断响应函数，处理编码器按键
void EXTI1_IRQHandler(void)
{
	delay_ms(10); // 消抖
	if (Bmq_Button == 0)
	{
		while (Bmq_Button == 0)
			; // 等待按键松开

		if (DF.Cursor_GliFlag == 1) // 如果光标闪烁标志位为真
		{
			set_button++;
			if (set_button == TWO_PRESSURE) // 按下两次中间按键
			{
				// 模块处于非运行状态
				if (Set_Para.Switch == FALSE)
				{
					Set_Para.Real_Io_Parameter = Set_Para.Set_Io_Parameter;
					Set_Para.Real_Vo_Parameter = Set_Para.Set_Vo_Parameter;
					Set_Para.Real_Work_Mode = Set_Para.Set_Work_Mode;
				}

				DF.Cursor_GliFlag = 0;	  // 光标闪烁标志位清零
				set_button = NO_PRESSURE; // 设置按键按下次数清零
			}
		}
		else
		{
			// 更新标志位开启或则关闭输出
			// 输出电压小于输入电压的0.85倍
			if ((Set_Para.Switch == FALSE) && (Set_Para.Real_Vo_Parameter <= IO_Para.Buck_Vin * 0.85))
			{
				Set_Para.Switch = TRUE;
			}
			else
			{
				Set_Para.Switch = FALSE;
				// 当机器正常运行，或者启动过程中，按下按键后，关闭输出，进入待机状态
				if ((Set_Para.Switch == FALSE) && (DF.Buck_SMFlag == Run))
				{
					DF.Buck_SMFlag = Init;
					// 关闭PWM
					IO_Para.Duty_Cycle = 0;
					DF.Buck_PWMENFlag = 0;
					DISABLE_OUT;
					TIM_SetCompare1(TIM1, 0);
					TIM_CtrlPWMOutputs(TIM1, DISABLE);
				}
			}
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line1); // 清除EXTI1线路挂起位
}

/************************************************************************************************
	函数名称：Encoder
	函数功能：编码器旋转的扫描及处
		右转常见序列：0x00 → 0x02 → 0x03 → 0x01 → 0x00
		左转常见序列：0x00 → 0x01 → 0x03 → 0x02 → 0x00
	入口参数：无
	出口参数：char型    0-无旋转   'R'-正转(向右转)   'L'-反转(向左转)
************************************************************************************************/
unsigned char Encoder(void)
{
	static _Bool Enc0 = 0, Enc1 = 0;	   // Enc0：用于标记是否第一次调用函数；Enc1：用于一定位一脉冲模式下的“消抖”控制或降低灵敏度（需转两次才算一次）
	static unsigned char EncOld, EncX = 0; // EncOld：上一次采样的编码器状态；EncX：中间状态，用于辅助判断旋转方向（是否符合完整转动轨迹）
	unsigned char EncNow;				   // 当前编码器状态（00/01/10/11）
	_Bool EncType = 0;					   // 设置编码器类型: 0：一定位一脉冲（半步模式）→ 需要转动两次才算一次（低灵敏度）；1：两定位一脉冲（全步模式）→ 只要转到一个方向完整轨迹就响应一次（高灵敏度）

	if (Enc0 == 0) // 如果是第一次调用
	{
		EncOld = (PINA ? 0x02 : 0x00) + (PINB ? 0x01 : 0x00);
		Enc0 = 1; // 记住初次调用时编码器的状态
	}
	EncNow = (PINA ? 0x02 : 0x00) + (PINB ? 0x01 : 0x00); // 根据两个IO当前状态组合成16进制的0x00|0x01|0x02|0x03
	if (EncNow == EncOld)
		return (0); // 如果新数据和原来的数据一样(没有转动)就直接返回0
	// 可能是右转，待观察
	if (EncOld == 0x00 && EncNow == 0x02 || EncOld == 0x03 && EncNow == 0x01)
		EncX = EncNow; // 00-10|11-01
	// 确定是否是右转
	if (EncOld == 0x00 && EncX == 0x02 && EncNow == 0x03 || EncOld == 0x03 && EncX == 0x01 && EncNow == 0x00) // 00-10-11|11-01-00右转
	{
		EncOld = EncNow, EncX = 0; // 更新历史状态，并清空中间状态
		if (EncType == 1)
			return ('R'); // 两定位一脉冲
		else			  // 一定位一脉冲
		{
			if (Enc1 == 0)
				Enc1 = 1;
			else
			{
				// Delayms(60);       //延时降低旋转灵敏度(不能用在定时器中)
				Enc1 = 0;
				return ('R');
			}
		}
	}
	// 可能是左转，待观察
	if (EncOld == 0x00 && EncNow == 0x01 || EncOld == 0x03 && EncNow == 0x02)
		EncX = EncNow; // 00-01|11-10
	// 确定是否是左转
	if (EncOld == 0x00 && EncX == 0x01 && EncNow == 0x03 || EncOld == 0x03 && EncX == 0x02 && EncNow == 0x00) // 00-01-11|11-10-00左转
	{
		EncOld = EncNow, EncX = 0;
		if (EncType == 1)
			return ('L'); // 两定位一脉冲
		else			  // 一定位一脉冲
		{
			if (Enc1 == 0)
				Enc1 = 1;
			else
			{
				// Delayms(60);       //延时降低旋转灵敏度(不能用在定时器中)
				Enc1 = 0;
				return ('L');
			}
		}
	}
	return (0); // 没有正确解码返回0
}

unsigned char enc;
u16 count = 0;

u16 PWM_Time_Count = 0;
// struct _BMQP Bmq_Para = {0,0};
// 定时器3中断服务程序
void TIM3_IRQHandler(void) // TIM3中断  1ms
{
	static unsigned int num = 0;

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) // 检查TIM3更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除TIMx更新中断标志
		num++;
		OLEDShowCnt++;
		Sys_Check_Cnt++;

		// 翻转一次光标闪烁标志位
		if (num == 100) // 时间0.1s
		{
			num = 0;
			count++;
			if (count == 3) // 延时0.3s翻转该标志位 用于设置参数时 屏幕相应位置闪烁
			{
				count = 0;
				if (DF.Cursor_DelFlag == 0)
					DF.Cursor_DelFlag = 1;
				else if (DF.Cursor_DelFlag == 1)
					DF.Cursor_DelFlag = 0;
			}
			if (DF.Cursor_GliFlag == 1) // 如果光标闪烁标志位为真
			{
				// 光标闪烁标志位计数
				blink_flash_time_count++;
				if (blink_flash_time_count == 60) // 6s无操作光标消失
				{
					DF.Cursor_GliFlag = 0;
					set_button = NO_PRESSURE;
					blink_flash_time_count = 0;
				}
			}
			else
				blink_flash_time_count = 0;
		}
		enc = Encoder(); // 扫描编码器,取得返回值
		if (enc != 0)	 // 转动了编码器
		{
			blink_flash_time_count = 0;
			if (DF.Cursor_GliFlag == 0) // 如果光标闪烁标志位为假
			{
				DF.Cursor_GliFlag = 1;							  // 置位光标闪烁标志位
				Set_Para.Set_Work_Mode = Set_Para.Real_Work_Mode; // 实际模式幅值给设置模式
				// 实际参数幅值给设置参数
				Set_Para.Set_Io_Parameter = Set_Para.Real_Io_Parameter;
				Set_Para.Set_Vo_Parameter = Set_Para.Real_Vo_Parameter;
			}
			else
			{
				switch (set_button)
				{
				case NO_PRESSURE: // 没有按下中间按键
					if (enc == 'R')
					{
						cur_position++;
						if (cur_position >= 3) // 光标一共有3个位置
							cur_position = 0;
					}
					if (enc == 'L')
					{
						cur_position--;
						if (cur_position < 0) // 光标一共有3个位置
							cur_position = 2;
					}
					break;

				case ONE_PRESSURE: // 按下一次中间按键 进行对应参数设置
					switch (cur_position)
					{
					case 0: // 模式切换
						if (Set_Para.Set_Work_Mode != MPPT_MODE)
							Set_Para.Set_Work_Mode = MPPT_MODE;
						else if (Set_Para.Set_Work_Mode == MPPT_MODE)
							Set_Para.Set_Work_Mode = CVCC_MODE;

						break;
					case 1: // 电压设置

						if (Set_Para.Switch == FALSE) // 运行状态下不能进行模式切换tongdao
						{
							if (enc == 'R')
							{
								if (Set_Para.Set_Vo_Parameter < VO_MAX)
									Set_Para.Set_Vo_Parameter += 1;
							}
							if (enc == 'L')
							{
								if (Set_Para.Set_Vo_Parameter > VO_MIN)
									Set_Para.Set_Vo_Parameter -= 1;
							}
						}
						break;

					case 2:							  // 电流设置
						if (Set_Para.Switch == FALSE) // 运行状态下不能进行模式切换
						{
							if (enc == 'R')
							{
								if (Set_Para.Set_Io_Parameter < IO_MAX)
									Set_Para.Set_Io_Parameter += 1;
							}
							if (enc == 'L')
							{
								if (Set_Para.Set_Io_Parameter > IO_MIN)
									Set_Para.Set_Io_Parameter -= 1;
							}
						}
						break;
					}
					break;
				case TWO_PRESSURE:
					break;
				}
			}
		}

		if (Sys_Check_Cnt == 5) // 5ms
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

			PWM_Time_Count++;
			if (PWM_Time_Count >= 20) // 100ms 10Hz
			{
				PWM_Time_Count = 0;
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
		}
	}
}
