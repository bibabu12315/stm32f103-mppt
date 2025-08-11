#include "key.h"
#include "delay.h"

//按键初始化函数 
//PA15和PC5 设置成输入
void KEY_Init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;

 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO,ENABLE);//使能PORTB时钟

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//关闭jtag，使能SWD，可以用SWD模式调试 
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
} 

//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//返回值：
//0，没有任何按键按下
//KEY0_PRES，KEY0按下
//KEY1_PRES，KEY1按下
//WKUP_PRES，WK_UP按下 
//注意此函数有响应优先级,KEY0>KEY1>WK_UP!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	if(key_up&&(KEY_CUR==0||KEY_ON_OFF==0||KEY_MODE==0||KEY_BAT_V==0))
	{
		delay_ms(10);//去抖动  
		key_up=0;
		if(KEY_CUR==0)return KEY_CUR_PRES;
		else if(KEY_ON_OFF==0)return KEY_ON_OFF_PRES;
		else if(KEY_MODE==0)return KEY_MODE_PRES; 
		else if(KEY_BAT_V==0)return KEY_BAT_V_PRES; 
	}else if(KEY_CUR==1&&KEY_ON_OFF==1&&KEY_MODE==1&KEY_BAT_V==1)key_up=1; 	     
	return 0;// 无按键按下
}




