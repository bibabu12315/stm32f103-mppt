#include "adc.h"
#include "delay.h"

// DMA缓存，用于存放ADC采集结果，长度为ARRSIZE
u16 ADCConvertedValue[ARRAYSIZE];

// 配置RCC时钟
void RCC_Configuration(void)
{
  // 设置ADC时钟为PCLK2的1/6，72MHz/6 = 12MHz，低于14MHz上限
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  // 开启DMA1时钟
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // 开启ADC1和GPIOA的时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
}

// 配置GPIO模拟输入引脚
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // 配置PA2~PA5为模拟输入模式（对应ADC通道2~5）
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// ADC和DMA初始化配置
void ADCx_InitConfig(void)
{
  ADC_InitTypeDef ADC_InitStructure;
  DMA_InitTypeDef DMA_InitStructure;

  // 初始化RCC时钟
  RCC_Configuration();

  // 初始化GPIO
  GPIO_Configuration();

  // 配置ADC工作在独立模式
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;

  // 使能扫描模式（用于多通道采样）
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;

  // 使能连续转换模式
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;

  // 使用软件触发启动转换
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;

  // 设置数据右对齐（12位数据右对齐）
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;

  // 设置参与转换的通道数为4
  ADC_InitStructure.ADC_NbrOfChannel = 4;

  // 初始化ADC1
  ADC_Init(ADC1, &ADC_InitStructure);

  // 配置ADC通道2，转换顺序为第1，采样时间239.5个周期
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_239Cycles5);

  // 配置ADC通道3，转换顺序为第2，采样时间239.5个周期
  ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 2, ADC_SampleTime_239Cycles5);

  // 配置ADC通道4，转换顺序为第3，采样时间239.5个周期
  ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 3, ADC_SampleTime_239Cycles5);

  // 配置ADC通道5，转换顺序为第4，采样时间239.5个周期
  ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 4, ADC_SampleTime_239Cycles5);

  // 使能ADC1的DMA请求功能
  ADC_DMACmd(ADC1, ENABLE);

  // 启用ADC1
  ADC_Cmd(ADC1, ENABLE);

  // 重置ADC校准寄存器
  ADC_ResetCalibration(ADC1);

  // 等待校准寄存器重置完成
  while (ADC_GetResetCalibrationStatus(ADC1))
    ;

  // 启动ADC校准
  ADC_StartCalibration(ADC1);

  // 等待校准完成
  while (ADC_GetCalibrationStatus(ADC1))
    ;

  // 启动ADC软件转换（开始采样）
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  // DMA配置
  DMA_DeInit(DMA1_Channel1); // 复位DMA1通道1配置

  // 设置DMA外设地址为ADC1的数据寄存器地址
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32) & (ADC1->DR);

  // 设置DMA的内存地址为ADC结果缓冲区
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCConvertedValue;

  // 设置传输方向为外设到内存
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;

  // 设置DMA缓冲区大小（单位为16位数据）
  DMA_InitStructure.DMA_BufferSize = ARRAYSIZE;

  // 禁止外设地址自增（始终是ADC1->DR）
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;

  // 启用内存地址自增
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

  // 设置外设数据宽度为16位（半字）
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;

  // 设置内存数据宽度为16位
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;

  // 设置DMA为循环模式，采样完成自动回到首地址
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;

  // 设置DMA通道优先级为高
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;

  // 禁止内存到内存模式（非M2M）
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

  // 初始化DMA1通道1
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);

  // 启用DMA1通道1
  DMA_Cmd(DMA1_Channel1, ENABLE);
}

// ADC采样中断处理函数（本例中未启用ADC中断）
void ADC1_IRQHandler(void)
{
  // 清除ADC中断标志
  ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);

  // 软件触发ADC转换（不建议在中断中重复触发）
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
