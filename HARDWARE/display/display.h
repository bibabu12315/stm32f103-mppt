#ifndef __DISPLAY_H
#define __DISPLAY_H	 
#include "sys.h"

////按键按下计数枚举量
typedef enum
{
    NO_PRESSURE,//没有按下
    ONE_PRESSURE,//按下一次
		TWO_PRESSURE//按下两次
}STATE_SET;

extern STATE_SET	set_button;//设置按键按下次数
extern u16 blink_flash_time_count;//光标闪烁时间计时
extern int cur_position;//设置参数时光标位置

void Display_Heart(void);
void Normal_Display(void);
#endif
