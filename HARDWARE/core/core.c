#include "core.h" //功能函数头文件
#include "adc.h"
#include "pwm.h"
#include "delay.h"
#include "math.h"
#include "encoder.h"

struct _SETP Set_Para = {100, 120, MPPT_MODE, 100, 120, MPPT_MODE, FALSE};
struct _FLAG DF = {0, 0, 0, 0, 0}; // 控制标志位

#define sCnt 300   // ADC采样次数
#define VREF 3.340 // ADC参考电压

int32_t ADVal_SUM[4] = {0}; // ADC多次采样总和
float ADVal_Avg[4] = {0};	// ADC多次采样平均值

struct _MPPT MPPT_Con_Para = {0};
struct _IOP IO_Para = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // 输出参数

// OLED刷新计数 1mS计数一次，在1mS中断里累加
volatile uint16_t OLEDShowCnt = 0;
// 过流过压保护计数 1mS计数一次，在1mS中断里累加
volatile uint16_t Sys_Check_Cnt = 0;

/*
 *程序目的：输入输出参数采样计算
 *程序流程：对adc采集到的数据进行均值滤波、滤波后的数据经过比例系数  K  处理后得到真实的电流电压数据、然后存入结构体IO_Para当中
 */
void Cal_IO_Para(void)
{
	uint16_t num = 0;
	for (num = 0; num < sCnt; num++)
	{
		// 求ADC采样结果总和
		ADVal_SUM[0] += (int32_t)ADCConvertedValue[4 * num] - IO_Para.IoutOffset;	 // 输出电流
		ADVal_SUM[1] += (int32_t)ADCConvertedValue[4 * num + 1];					 // 输出电压
		ADVal_SUM[2] += (int32_t)ADCConvertedValue[4 * num + 2] - IO_Para.IinOffset; // 输入电流
		ADVal_SUM[3] += (int32_t)ADCConvertedValue[4 * num + 3];					 // 输入电压
	}
	for (num = 0; num < 4; num++)
	{
		ADVal_Avg[num] = ADVal_SUM[num] / sCnt;
	}
	ADVal_Avg[0] = ((ADVal_Avg[0] * VREF * 100 / 15) / 4096) * CAL_BUCK_IOUT_K + CAL_BUCK_IOUT_B; // 输出电流 单位A
	ADVal_Avg[1] = ((ADVal_Avg[1] * VREF * 75 / 3.3) / 4096) * CAL_BUCK_VOUT_K + CAL_BUCK_VOUT_B; // 输出电压 单位V

	ADVal_Avg[2] = ((ADVal_Avg[2] * VREF * 100 / 15) / 4096) * CAL_BUCK_IIN_K + CAL_BUCK_IOUT_B; // 输入电流 单位A
	ADVal_Avg[3] = ((ADVal_Avg[3] * VREF * 75 / 3.3) / 4096) * CAL_BUCK_VIN_K + CAL_BUCK_VIN_B;	 // 输入电压 单位V
	// 对采样值进行下限保护，防止负数
	if (ADVal_Avg[0] < 0)
		ADVal_Avg[0] = 0;
	if (ADVal_Avg[1] < 0)
		ADVal_Avg[1] = 0;
	if (ADVal_Avg[2] < 0)
		ADVal_Avg[2] = 0;
	if (ADVal_Avg[3] < 0)
		ADVal_Avg[3] = 0;
	// 将采样值存入IO_Para结构体
	IO_Para.Buck_Iout = ADVal_Avg[0] * 10; // 扩大10倍 单位0.01A
	IO_Para.Buck_Vout = ADVal_Avg[1] * 10;
	IO_Para.Buck_Iin = ADVal_Avg[2] * 10;
	IO_Para.Buck_Vin = ADVal_Avg[3] * 10;
	IO_Para.Buck_Pin = ADVal_Avg[2] * ADVal_Avg[3] * 10;
	IO_Para.Buck_Pout = ADVal_Avg[0] * ADVal_Avg[1] * 10;

	for (num = 0; num < 4; num++)
	{
		ADVal_SUM[num] = 0;
		ADVal_Avg[num] = 0;
	}
}

bool FIRST_IN_FLAG = TRUE;

/**
 * @brief 调节PWM占空比
 * 根据工作模式（CCCV或MPPT）调整PWM占空比。
 * 在CCCV模式下，根据输出电流和电压与设定值的比较
 * 在MPPT模式下，根据输入电压和功率的变化来调整占空比。
 * @param None
 * @return None
 */
void PWM_Adjust(void)
{
	switch (Set_Para.Real_Work_Mode)
	{
	case CVCC_MODE:
		if (IO_Para.Buck_Iout > Set_Para.Real_Io_Parameter)
		{
			IO_Para.Duty_Cycle--;
		}
		else if (IO_Para.Buck_Vout > Set_Para.Real_Vo_Parameter)
		{
			IO_Para.Duty_Cycle--;
		}
		else if (IO_Para.Buck_Vout < Set_Para.Real_Vo_Parameter)
		{
			IO_Para.Duty_Cycle++;
		}
		else
		{
		}

		// 环路输出最大最小占空比限制
		if (IO_Para.Duty_Cycle > MAX_BUCK_DUTY)
			IO_Para.Duty_Cycle = MAX_BUCK_DUTY;
		if (IO_Para.Duty_Cycle < MIN_BUCK_DUTY)
			IO_Para.Duty_Cycle = MIN_BUCK_DUTY;
		TIM_SetCompare1(TIM1, IO_Para.Duty_Cycle);

		break;
	case MPPT_MODE:

		MPPT_Con_Para.voltageInput = IO_Para.Buck_Vin;
		MPPT_Con_Para.powerInput = IO_Para.Buck_Pin;

		// 输入电压不能小于8V，否则模块无法正常工作
		if (IO_Para.Buck_Vin < 80)
		{
			IO_Para.Duty_Cycle--;
		}
		else
		{
			// 输出电流电压超过设定值时，降低占空比
			if (IO_Para.Buck_Iout > Set_Para.Real_Io_Parameter)
			{
				IO_Para.Duty_Cycle--;
			}
			else if (IO_Para.Buck_Vout > Set_Para.Real_Vo_Parameter)
			{
				IO_Para.Duty_Cycle--;
			}
			else
			{
				//  ↑P ↑V ; →MPP  //D--
				if ((MPPT_Con_Para.powerInput > MPPT_Con_Para.powerInputPrev) && (MPPT_Con_Para.voltageInput > MPPT_Con_Para.voltageInputPrev))
				{
					IO_Para.Duty_Cycle -= 4;
				}
				//  ↑P ↓V ; MPP←  //D++
				else if ((MPPT_Con_Para.powerInput > MPPT_Con_Para.powerInputPrev) && (MPPT_Con_Para.voltageInput < MPPT_Con_Para.voltageInputPrev))
				{
					IO_Para.Duty_Cycle += 4;
				}
				//  ↓P ↑V ; MPP→  //D++
				else if ((MPPT_Con_Para.powerInput < MPPT_Con_Para.powerInputPrev) && (MPPT_Con_Para.voltageInput > MPPT_Con_Para.voltageInputPrev))
				{
					IO_Para.Duty_Cycle += 4;
				}
				//  ↓P ↓V ; ←MPP  //D--
				else if ((MPPT_Con_Para.powerInput < MPPT_Con_Para.powerInputPrev) && (MPPT_Con_Para.voltageInput < MPPT_Con_Para.voltageInputPrev))
				{
					IO_Para.Duty_Cycle -= 4;
				}

				// 输出电压小于设定值时，增加占空比
				else if (IO_Para.Buck_Vout < Set_Para.Real_Vo_Parameter)
				{
					IO_Para.Duty_Cycle++;
				}

				// 更新状态——上次输入功率和电压 = 当前输入功率和电压
				MPPT_Con_Para.voltageInputPrev = MPPT_Con_Para.voltageInput;
				MPPT_Con_Para.powerInputPrev = MPPT_Con_Para.powerInput;
			}
		}

		// 环路输出最大最小占空比限制
		if (IO_Para.Duty_Cycle > MAX_BUCK_DUTY)
			IO_Para.Duty_Cycle = MAX_BUCK_DUTY;
		if (IO_Para.Duty_Cycle < MIN_BUCK_DUTY)
			IO_Para.Duty_Cycle = MIN_BUCK_DUTY;
		// 设置PWM占空比
		TIM_SetCompare1(TIM1, IO_Para.Duty_Cycle);

		break;
	}
}

/** ===================================================================
**     Funtion Name :void Buck_StateM(void)
**     Description :   状态机函数，在5ms中断中运行，5ms运行一次
**     初始化状态
**     等外启动状态
**     启动状态
**     运行状态
**     故障状态
**     Parameters  :
**     Returns     :
** ===================================================================*/
void Buck_StateM(void)
{
	// 判断状态类型
	switch (DF.Buck_SMFlag)
	{
	// 初始化状态
	case Init:
		Buck_StateMInit();
		break;
	// 等待状态
	case Wait:
		Buck_StateMWait();
		break;
	// 运行状态
	case Run:
		Buck_StateMRun();
		break;
	// 故障状态
	case Err:
		Buck_StateMErr();
		break;
	}
}
/** ===================================================================
**     Funtion Name :void Buck_StateMInit(void)
**     Description :   初始化状态函数，参数初始化
**     Parameters  :
**     Returns     :
** ===================================================================*/
void Buck_StateMInit(void)
{
	// 相关参数初始化
	Buck_ValInit();
	// 状态机跳转至等待软启状态
	DF.Buck_SMFlag = Wait;
}

/** ===================================================================
**     Funtion Name :void Buck_ValInit(void)
**     Description :   相关参数初始化函数
**     Parameters  :
**     Returns     :
** ===================================================================*/
void Buck_ValInit(void)
{
	// 关闭PWM
	DF.Buck_PWMENFlag = 0;
	DISABLE_OUT;
	TIM_SetCompare1(TIM1, 0);
	TIM_CtrlPWMOutputs(TIM1, DISABLE);
	IO_Para.Duty_Cycle = 0;
	// 清除故障标志位
	DF.Buck_ErrFlag = 0;
}

/** ===================================================================
**     Funtion Name :void Buck_StateMWait(void)
**     Description :   等待状态机，等待1S后无故障则软启
**     Parameters  :
**     Returns     :
** ===================================================================*/
void Buck_StateMWait(void)
{
	// 计数器定义
	static uint16_t Buck_CntS = 0;
	static uint32_t IoutSum = 0;
	static uint32_t IinSum = 0;
	static float middle_val;

	// 关PWM
	DF.Buck_PWMENFlag = 0;
	// 计数器累加
	Buck_CntS++;
	// 等待*S，采样输出电流偏置好后， 且无故障情况,进入启动状态
	if (Buck_CntS > 50)
	{
		Buck_CntS = 50;
		// 启动条件：1、系统无故障 2、启动开关开启
		if ((DF.Buck_ErrFlag == F_NOERR) && (Set_Para.Switch == TRUE))
		{
			// 计数器清0  是否需要放出if判断外
			Buck_CntS = 0;

			// 标志这打开pwm，但不算实际开启pwm波，实际开启是——“ENABLE_OUT”
			DF.Buck_PWMENFlag = 1;
			// 设定初始占空比
			// middle_val = ((float)Set_Para.Real_Vo_Parameter/(float)IO_Para.Buck_Vin)*360.0-36.0;
			middle_val = ((float)Set_Para.Real_Vo_Parameter / (float)IO_Para.Buck_Vin) * 720.0 - 72.0;
			IO_Para.Duty_Cycle = middle_val;

			// 环路输出最大最小占空比限制
			if (IO_Para.Duty_Cycle > MAX_BUCK_DUTY)
				IO_Para.Duty_Cycle = MAX_BUCK_DUTY;
			if (IO_Para.Duty_Cycle < MIN_BUCK_DUTY)
				IO_Para.Duty_Cycle = MIN_BUCK_DUTY;

			// 打开器件LM74700的PWM通道
			ENABLE_OUT;
			TIM_CtrlPWMOutputs(TIM1, ENABLE); // MOE 主输出使能
			delay_ms(10);
			TIM_SetCompare1(TIM1, IO_Para.Duty_Cycle);

			// 状态机跳转至运行状态
			DF.Buck_SMFlag = Run;
		}
	}
	else // 进行输入和输出电流1.65V偏置求平均
	{
		// 输入输出电流偏置求和
		IoutSum += ADCConvertedValue[0];
		IinSum += ADCConvertedValue[2];
		// 50次数
		if (Buck_CntS == 50)
		{
			// 求平均
			IO_Para.IoutOffset = IoutSum / 50;
			IO_Para.IinOffset = IinSum / 50;
			IoutSum = 0;
			IinSum = 0;
		}
	}
}

/*
** ===================================================================
**     Funtion Name :void Buck_StateMRun(void)
**     Description :正常运行，主处理函数在中断中运行
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void Buck_StateMRun(void)
{
}

/*
** ===================================================================
**     Funtion Name :void StateMErr(void)
**     Description :故障状态
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void Buck_StateMErr(void)
{
	// 关闭PWM
	DF.Buck_PWMENFlag = 0;
	DISABLE_OUT;
	TIM_SetCompare1(TIM1, 0);
	TIM_CtrlPWMOutputs(TIM1, DISABLE);
	IO_Para.Duty_Cycle = 0;
	// 若故障消除跳转至等待重新软启
	if (DF.Buck_ErrFlag == F_NOERR)
		DF.Buck_SMFlag = Wait;
}

/*
** ===================================================================
**     Funtion Name :void ShortOff(void)
**     Description :短路保护，可以重启10次
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_SHORT_I 120 // 短路电流判据12A
#define MIN_SHORT_V 30	// 短路电压判据3V

void ShortOff(void)
{
	static int32_t RSCnt = 0;
	static uint8_t RSNum = 0;

	// 当输出电流大于 *A，且电压小于*V时，可判定为发生短路保护
	if ((IO_Para.Buck_Iout > MAX_SHORT_I) && (IO_Para.Buck_Vout < MIN_SHORT_V))
	{
		// 关闭PWM
		DF.Buck_PWMENFlag = 0;
		DISABLE_OUT;
		TIM_SetCompare1(TIM1, 0);
		TIM_CtrlPWMOutputs(TIM1, DISABLE);
		TIM_Cmd(TIM2, DISABLE); // 失能TIM2
		// 故障标志位
		setRegBits(DF.Buck_ErrFlag, F_SW_SHORT);
		// 跳转至故障状态
		DF.Buck_SMFlag = Err;
	}
	// 输出短路保护恢复
	// 当发生输出短路保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
	if (getRegBits(DF.Buck_ErrFlag, F_SW_SHORT))
	{
		// 等待故障清楚计数器累加
		RSCnt++;
		// 等待2S
		if (RSCnt > 400)
		{
			// 计数器清零
			RSCnt = 0;
			// 短路重启只重启10次，10次后不重启
			if (RSNum > 10)
			{
				// 确保不清除故障，不重启
				RSNum = 11;
				// 关闭PWM
				DF.Buck_PWMENFlag = 0;
				DISABLE_OUT;
				TIM_SetCompare1(TIM1, 0);
				TIM_CtrlPWMOutputs(TIM1, DISABLE);
				TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			}
			else
			{
				// 短路重启计数器累加
				RSNum++;
				// 清除过流保护故障标志位
				clrRegBits(DF.Buck_ErrFlag, F_SW_SHORT);
			}
		}
	}
}
/*
** ===================================================================
**     Funtion Name :void SwOCP(void)
**     Description :软件过流保护，可重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_OCP_VAL 105 // 10.5A过流保护点
void SwOCP(void)
{
	// 过流保护判据保持计数器定义
	static uint16_t OCPCnt = 0;
	// 故障清楚保持计数器定义
	static uint16_t RSCnt = 0;
	// 保留保护重启计数器
	static uint16_t RSNum = 0;

	// 当输出电流大于*A，且保持500ms
	if ((IO_Para.Buck_Iout > MAX_OCP_VAL) && (DF.Buck_PWMENFlag == 1))
	{
		// 条件保持计时
		OCPCnt++;
		// 条件保持50ms，则认为过流发生
		if (OCPCnt > 10)
		{
			// 计数器清0
			OCPCnt = 0;
			// 关闭PWM
			DF.Buck_PWMENFlag = 0;
			DISABLE_OUT;
			TIM_SetCompare1(TIM1, 0);
			TIM_CtrlPWMOutputs(TIM1, DISABLE);
			TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			// 故障标志位
			setRegBits(DF.Buck_ErrFlag, F_SW_IOUT_OCP);
			// 跳转至故障状态
			DF.Buck_SMFlag = Err;
		}
	}
	else
		// 计数器清0
		OCPCnt = 0;

	// 输出过流后恢复
	// 当发生输出软件过流保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
	if (getRegBits(DF.Buck_ErrFlag, F_SW_IOUT_OCP))
	{
		// 等待故障清楚计数器累加
		RSCnt++;
		// 等待2S
		if (RSCnt > 400)
		{
			// 计数器清零
			RSCnt = 0;
			// 过流重启计数器累加
			RSNum++;
			// 过流重启只重启10次，10次后不重启（严重故障）
			if (RSNum > 10)
			{
				// 确保不清除故障，不重启
				RSNum = 11;
				// 关闭PWM
				DF.Buck_PWMENFlag = 0;
				DISABLE_OUT;
				TIM_SetCompare1(TIM1, 0);
				TIM_CtrlPWMOutputs(TIM1, DISABLE);
				TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			}
			else
			{
				// 清除过流保护故障标志位
				clrRegBits(DF.Buck_ErrFlag, F_SW_IOUT_OCP);
			}
		}
	}
}

/*
** ===================================================================
**     Funtion Name :void SwOVP(void)
**     Description :软件输出过压保护，不重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/

#define MAX_VOUT_OVP_VAL 550 // 过压保护55V

void VoutSwOVP(void)
{
	// 过压保护判据保持计数器定义
	static uint16_t OVPCnt = 0;

	// 当输出电压大于40V，且保持100ms
	if (IO_Para.Buck_Vout > MAX_VOUT_OVP_VAL)
	{
		// 条件保持计时
		OVPCnt++;
		// 条件保持10ms
		if (OVPCnt > 2)
		{
			// 计时器清零
			OVPCnt = 0;
			// 关闭PWM
			DF.Buck_PWMENFlag = 0;
			DISABLE_OUT;
			TIM_SetCompare1(TIM1, 0);
			TIM_CtrlPWMOutputs(TIM1, DISABLE);
			TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			// 故障标志位
			setRegBits(DF.Buck_ErrFlag, F_SW_VOUT_OVP);
			// 跳转至故障状态
			DF.Buck_SMFlag = Err;
		}
	}
	else
		OVPCnt = 0;
}
/*
** ===================================================================
**     Funtion Name :void VinSwUVP(void)
**     Description :输入软件欠压保护，低压输入保护,可恢复
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MIN_UVP_VAL 68	  // 6.8V欠压保护
#define MIN_UVP_VAL_RE 74 // 7.4V欠压保护恢复
void VinSwUVP(void)
{
	// 过压保护判据保持计数器定义
	static uint16_t UVPCnt = 0;
	static uint16_t RSCnt = 0;

	// 当输入电压小于10V，且保持200ms
	if ((IO_Para.Buck_Vin < MIN_UVP_VAL) && (DF.Buck_SMFlag != Init))
	{
		// 条件保持计时
		UVPCnt++;
		// 条件保持10ms
		if (UVPCnt > 2)
		{
			// 计时器清零
			UVPCnt = 0;
			RSCnt = 0;
			// 关闭PWM
			DF.Buck_PWMENFlag = 0;
			DISABLE_OUT;
			TIM_SetCompare1(TIM1, 0);
			TIM_CtrlPWMOutputs(TIM1, DISABLE);
			TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			// 故障标志位
			setRegBits(DF.Buck_ErrFlag, F_SW_VIN_UVP);
			// 跳转至故障状态
			DF.Buck_SMFlag = Err;
		}
	}
	else
		UVPCnt = 0;

	// 输入欠压保护恢复
	// 当发生输入欠压保护，等待输入电压恢复至正常水平后清楚故障标志位，重启
	if (getRegBits(DF.Buck_ErrFlag, F_SW_VIN_UVP))
	{
		if (IO_Para.Buck_Vin > MIN_UVP_VAL_RE)
		{
			// 等待故障清楚计数器累加
			RSCnt++;
			// 等待1S
			if (RSCnt > 200)
			{
				RSCnt = 0;
				UVPCnt = 0;
				// 清楚故障标志位
				clrRegBits(DF.Buck_ErrFlag, F_SW_VIN_UVP);
			}
		}
		else
			RSCnt = 0;
	}
	else
		RSCnt = 0;
}

/*
** ===================================================================
**     Funtion Name :void VinSwOVP(void)
**     Description :软件输入过压保护，不重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VIN_OVP_VAL 550 // 55V过压保护
void VinSwOVP(void)
{
	// 过压保护判据保持计数器定义
	static uint16_t OVPCnt = 0;

	// 输入电压大于15V，且保持100ms
	if (IO_Para.Buck_Vin > MAX_VIN_OVP_VAL)
	{
		// 条件保持计时
		OVPCnt++;
		// 条件保持10ms
		if (OVPCnt > 2)
		{
			// 计时器清零
			OVPCnt = 0;
			// 关闭PWM
			DF.Buck_PWMENFlag = 0;
			DISABLE_OUT;
			TIM_SetCompare1(TIM1, 0);
			TIM_CtrlPWMOutputs(TIM1, DISABLE);
			TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			// 故障标志位
			setRegBits(DF.Buck_ErrFlag, F_SW_VIN_OVP);
			// 跳转至故障状态
			DF.Buck_SMFlag = Err;
		}
	}
	else
		OVPCnt = 0;
}
/*
** ===================================================================
**     Funtion Name :void VoutOverpower(void)
**     Description :软件输出过功率保护，不重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VOUT_OVERPOWER_VAL 1000 // 100W过压保护
void VoutOverpower(void)
{
	// 过功率保护判据保持计数器定义
	static uint16_t OVerPCnt = 0;

	// 输出功率大于100W，且保持100ms
	if (IO_Para.Buck_Pout > MAX_VOUT_OVERPOWER_VAL)
	{
		// 条件保持计时
		OVerPCnt++;
		// 条件保持10ms
		if (OVerPCnt > 2)
		{
			// 计时器清零
			OVerPCnt = 0;
			// 关闭PWM
			DF.Buck_PWMENFlag = 0;
			DISABLE_OUT;
			TIM_SetCompare1(TIM1, 0);
			TIM_CtrlPWMOutputs(TIM1, DISABLE);
			TIM_Cmd(TIM2, DISABLE); // 失能TIM2
			// 故障标志位
			setRegBits(DF.Buck_ErrFlag, F_SW_VOUT_OVERPOWER);
			// 跳转至故障状态
			DF.Buck_SMFlag = Err;
		}
	}
	else
		OVerPCnt = 0;
}
