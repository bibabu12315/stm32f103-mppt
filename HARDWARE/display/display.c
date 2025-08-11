#include "display.h"
#include "oled.h"
#include "core.h"

STATE_SET set_button = NO_PRESSURE; // 设置按键按下次数
u16 blink_flash_time_count = 0;		// 光标闪烁时间计时
int cur_position = 0;				// 设置参数时光标位置

void Display_Heart(void) // 校正模式显示
{
	OLED_Clear(); // 清屏
	//"123456789ABCDEFG"
	OLED_ShowString(0, 0, "Mode:CC/CV  OFF", 16);
	OLED_ShowString(0, 2, "Vin:**.*V/**.*A", 16);
	OLED_ShowString(0, 4, "Vou:**.*V/**.*A", 16);
	OLED_ShowString(0, 6, "Set:**.*V/**.*A", 16);
}
// 将参数拆分成每一位，便于显示
void Para_Split(u8 *table, int32_t var)
{
	table[0] = (u8)(var / 100) + '0';
	table[1] = (u8)(var % 100 / 10) + '0';
	table[2] = '.';
	table[3] = (u8)(var % 10) + '0';
	table[4] = '\0';
}

// 显示光标闪烁状态下的动态数据
void Display_Blink_Dynamic_Vars(void)
{
	u8 Middle_temp[6] = {0};
	if (Set_Para.Set_Work_Mode != MPPT_MODE)
		OLED_ShowString(40, 0, "CC/CV", 16); // 如果工作模式不是MPPT模式，则展示CC/CV模式
	else if (Set_Para.Set_Work_Mode == MPPT_MODE)
		OLED_ShowString(40, 0, "MPPT ", 16); // 如果工作模式是MPPT模式，则展示MPPT模式

	// 保存设置电压的每一位，小数点后一位保留
	Para_Split(Middle_temp, Set_Para.Set_Vo_Parameter);
	OLED_ShowString(32, 6, Middle_temp, 16);
	OLED_ShowString(64, 6, "V", 16);

	// 保存设置电流的每一位，小数点后一位保留
	Para_Split(Middle_temp, Set_Para.Set_Io_Parameter);
	OLED_ShowString(80, 6, Middle_temp, 16);
	OLED_ShowString(112, 6, "A", 16);
}

/**
 * @brief
 * 显示光标不闪烁状态下的动态数据
 *
 */
void Display_Normal_Dynamic_Vars(void)
{
	u8 Middle_temp[6] = {0};
	if (Set_Para.Real_Work_Mode != MPPT_MODE)
		OLED_ShowString(40, 0, "CC/CV", 16);
	else if (Set_Para.Real_Work_Mode == MPPT_MODE)
		OLED_ShowString(40, 0, "MPPT ", 16);

	// 保存实际电压的每一位，小数点后一位保留
	Para_Split(Middle_temp, Set_Para.Real_Vo_Parameter);
	OLED_ShowString(32, 6, Middle_temp, 16);
	OLED_ShowString(64, 6, "V", 16);

	// 保存实际电流的每一位，小数点后一位保留
	Para_Split(Middle_temp, Set_Para.Real_Io_Parameter);
	OLED_ShowString(80, 6, Middle_temp, 16);
	OLED_ShowString(112, 6, "A", 16);
}
// 无故障，正常显示各个参数
void Normal_Display(void)
{
	//	u16 temp;
	u8 Middle_temp[6] = {0};
	if (DF.Cursor_GliFlag == 1) // 如果光标闪烁标志位为真 参数设置状态
	{

		if (DF.Cursor_DelFlag == 1) // 延时标志位置1
		{
			// 显示全部动态数据
			Display_Blink_Dynamic_Vars();
		}
		else
		{
			// 光标位置
			switch (cur_position)
			{
			case 0: // 光标在Mode:CC/CV位置
				OLED_ShowString(40, 0, "     ", 16);
				break;
			case 1: // 光标在Set:**.*V
				OLED_ShowString(32, 6, "     ", 16);
				break;
			case 2: // 光标在**.*A
				OLED_ShowString(80, 6, "     ", 16);
				break;
			}
		}
	}
	else // 光标不闪烁 正常显示状态
	{
		// 显示通常状态下的动态数据
		Display_Normal_Dynamic_Vars();

		// 显示实际输入输出电压电流值
		Para_Split(Middle_temp, (int32_t)IO_Para.Buck_Vin);
		OLED_ShowString(32, 2, Middle_temp, 16);
		OLED_ShowString(64, 2, "V", 16);

		Para_Split(Middle_temp, (int32_t)IO_Para.Buck_Iin);
		OLED_ShowString(80, 2, Middle_temp, 16);
		OLED_ShowString(112, 2, "A", 16);

		Para_Split(Middle_temp, (int32_t)IO_Para.Buck_Vout);
		OLED_ShowString(32, 4, Middle_temp, 16);
		OLED_ShowString(64, 4, "V", 16);

		Para_Split(Middle_temp, (int32_t)IO_Para.Buck_Iout);
		OLED_ShowString(80, 4, Middle_temp, 16);
		OLED_ShowString(112, 4, "A", 16);

		// 显示开始停止
		if (Set_Para.Switch == TRUE)
			OLED_ShowString(96, 0, "ON ", 16);
		else
			OLED_ShowString(96, 0, "OFF", 16);
	}
}
