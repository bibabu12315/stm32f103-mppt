#ifndef __CORE_H
#define __CORE_H

#include "sys.h"

// 滤波变量类型
typedef enum
{
	FALSE = 0,
	TRUE = 1
} bool;

// extern uint32_t PID_Adjust;	//PID调节参数
extern float Set_Start_Vout; // 设置启动时输出电压值
extern struct _FLAG DF;
extern struct _IOP IO_Para; // 输入输出参数
extern uint8_t ADC_SAMPLE_OVER_FLAG;
extern volatile uint16_t OLEDShowCnt;
extern volatile uint16_t Sys_Check_Cnt;
extern volatile uint16_t Sys_Mppt_Cnt;
// extern bool OUTPUT_FLAG;//输出使能标志位
extern struct _SETP Set_Para;
// 函数声明
void ADCSample(void);

void Buck_StateM(void);
void Buck_StateMInit(void);
void Buck_StateMWait(void);
void Buck_StateMRise(void);
void Buck_StateMRun(void);
void Buck_StateMErr(void);
void Buck_ValInit(void);

void VrefGet(void);
void ShortOff(void);
void SwOCP(void);
void VoutSwOVP(void);
void VinSwUVP(void);
void VinSwOVP(void);
void VoutOverpower(void);
void key_check(void);

void BAT_Vol_Check(void);
void OutPut_En_Control(void);
void Cal_IO_Para(void);
void PWM_Adjust(void);

/*****************************故障类型*****************/
#define F_NOERR 0x0000			   // 无故障
#define F_SW_VIN_UVP 0x0001		   // 输入欠压
#define F_SW_VIN_OVP 0x0002		   // 输入过压
#define F_SW_VOUT_UVP 0x0004	   // 输出欠压
#define F_SW_VOUT_OVP 0x0008	   // 输出过压
#define F_SW_IOUT_OCP 0x0010	   // 输出过流
#define F_SW_SHORT 0x0020		   // 输出短路
#define F_SW_VOUT_OVERPOWER 0x0040 // 输出过功率

#define MIN_BUCK_DUTY 50  // BUCK最小占空比
#define MAX_BUCK_DUTY 670 // BUCK最大占空比，93%

// #define MIN_BUCK_DUTY	25	//BUCK最小占空比
// #define MAX_BUCK_DUTY 335//BUCK最大占空比，93%

#define CAL_BUCK_VOUT_K 1 // Buck输出电压矫正K值
#define CAL_BUCK_VOUT_B 0 // Buck输出电压矫正B值

#define CAL_BUCK_IOUT_K 1 // 输出电流矫正K值
#define CAL_BUCK_IOUT_B 0 // 输出电流矫正B值

#define CAL_BUCK_VIN_K 1 // 输入电压矫正K值
#define CAL_BUCK_VIN_B 0 // 输入电压矫正B值

#define CAL_BUCK_IIN_K 1 // 输入电流矫正K值
#define CAL_BUCK_IIN_B 0 // 输入电流矫正B值

// #define VOLTAGE_STEP 0.1

// 状态机枚举量
typedef enum
{
	Init, // 初始化
	Wait, // 空闲等待
	Run,  // 正常运行
	Err	  // 故障
} STATE_M;

// 工作模式枚举变量
typedef enum
{
	MPPT_MODE = 0,	   // 固定步长PO——MPPT算法			 //0
	CVCC_MODE,		   // CV//CC模式				    //1
	MPPT_Vari_PO_MODE, // 变步长扰动PO——mppt算法		//2
	MPPT_INC_MODE,	   // 电导INC——mppt算法			    //3
	MODE_COUNT		   // 模式总数						//4
} SWork_M;

// 输出参数结构体
struct _IOP
{
	int32_t Buck_Vin;	// Buck 输入电压 单位0.1V
	int32_t Buck_Iin;	// Buck 输入电流 单位0.1A
	int32_t Buck_Vout;	// Buck 输出电压 位0.1V
	int32_t Buck_Iout;	// 输出电流 单位0.1A
	int32_t Buck_Pin;	// 输入功率 单位0.1W
	int32_t Buck_Pout;	// 输出功率 单位0.1W
	int32_t IoutOffset; // 输出电流采样偏置
	int32_t IinOffset;	// 输出电流采样偏置
	int16_t Duty_Cycle; // PWM占空比
};

// 屏幕设置参数结构体
struct _SETP
{
	int32_t Set_Io_Parameter; // 参数值 单位0.1A   要设定的参数
	int32_t Set_Vo_Parameter; // 参数值 单位0.1V
	SWork_M Set_Work_Mode;	  // 工作模式

	int32_t Real_Io_Parameter; // 参数值   确定要设置的参数（设置好，正在运行的参数）
	int32_t Real_Vo_Parameter; // 参数值
	SWork_M Real_Work_Mode;	   // 工作模式

	bool Switch; // 输出开关
};
// MPPT控制结构体
struct _MPPT
{
	int32_t powerInput;		  // 当前输入功率
	int32_t powerInputPrev;	  // 上一次输入功率
	int32_t voltageInput;	  // 当前输入电压
	int32_t voltageInputPrev; // 上一次输入电压
};
// 电池参数结构体
struct _BAT
{
	int32_t Bat_Vol;		  // 电池电压
	bool BAT_VOL_SAMPLE_FLAG; // 电池电压采样标志位
};
// 标志位定义
struct _FLAG
{
	uint16_t Buck_SMFlag;	// 状态机标志位
	uint16_t Buck_ErrFlag;	// 故障标志位
	uint8_t Buck_PWMENFlag; // 启动标志位
	uint8_t Cursor_GliFlag; // 光标闪烁标志位
	uint8_t Cursor_DelFlag; // 光标闪烁延时退出标志位
};

#define setRegBits(reg, mask) (reg |= (unsigned int)(mask))
#define clrRegBits(reg, mask) (reg &= (unsigned int)(~(unsigned int)(mask)))
#define getRegBits(reg, mask) (reg & (unsigned int)(mask))
#define getReg(reg) (reg)

#define CCMRAM __attribute__((section("ccmram")))

#endif
