
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "erorrNum.h"
#include "spi.h"
#include "power.h"
#include "OTPRW.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"


void ADC_Config(void)
{

	ADC_InitTypeDef  ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_ADCCLKConfig(RCC_PCLK2_Div2);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA,ENABLE);


	GPIO_InitStructure.GPIO_Pin  =GPIO_Pin_1 |GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_AIN;
	GPIO_Init(GPIOA,&GPIO_InitStructure); //默认速度为两兆


	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_InitStructure);


	ADC_InitStructure.ADC_Mode              = ADC_Mode_Independent;  //独立模式
	ADC_InitStructure.ADC_ScanConvMode      =DISABLE; //ENABLE;      //连续多通道模式
	ADC_InitStructure.ADC_ContinuousConvMode =ENABLE;      //连续转换
	ADC_InitStructure.ADC_ExternalTrigConv  = ADC_ExternalTrigConv_None; //转换不受外界决定
	ADC_InitStructure.ADC_DataAlign         =ADC_DataAlign_Right;   //右对齐
	ADC_InitStructure.ADC_NbrOfChannel      =1;   //3    //扫描通道数

	ADC_Init(ADC1, &ADC_InitStructure);

	//转换序列1，2，3
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 1,ADC_SampleTime_1Cycles5); //通道X,采样时间为1.5周期,1代表规则通道第1个这个1是啥意思我不太清楚只有是1的时候我的ADC才正常。
    //    ADC_RegularChannelConfig(ADC1,ADC_Channel_2,2,ADC_SampleTime_1Cycles5);
	//ADC_RegularChannelConfig(ADC1,ADC_Channel_3, 3, ADC_SampleTime_1Cycles5);

	//ADC_DMACmd(ADC1,ENABLE);

	ADC_Cmd  (ADC1,ENABLE);             //使能或者失能指定的ADC
	//ADC 自动校准，开机后需要执行一次，保证精度
	// Enable ADC1 reset calibaration register
	ADC_ResetCalibration(ADC1);
	// Check the end of ADC1 reset calibration register
	while(ADC_GetResetCalibrationStatus(ADC1));
	// Start ADC1 calibaration
	ADC_StartCalibration(ADC1);
	// Check the end of ADC1 calibration
	while(ADC_GetCalibrationStatus(ADC1));
	// ADC自动校准结束
    
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);//使能或者失能指定的ADC的软件转换启动功能
}

//void DMA_Config(void)
//{
//	DMA_InitTypeDef DMA_InitStructure;
//
//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//使能DMA时钟
//
//	DMA_DeInit(DMA1_Channel1);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;//adc1的数据寄存器设备地址，数据手册上有
//	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&adc_value;//一个有2个元素数组的首地址
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//	//BufferSize=2，因为ADC转换序列有2个通道
//	//如此设置，使序列1结果放在adc_value[0]，序列2结果放在adc_value[1]
//	DMA_InitStructure.DMA_BufferSize = 3;
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
//	//循环模式开启，Buffer写满后，自动回到初始地址开始传输
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
//	//配置完成后，启动DMA通道
//	DMA_Cmd(DMA1_Channel1, ENABLE);
//}



u8 TestAdc(void)
{

	u16 adc;
	//POWER_ON(vpp12,vdd30);
    // MCP42050_ADJ(ADJ_VPP,150);//vpp value
     //MCP42050_ADJ(ADJ_VDD,51); //vdd value
	if (DeviceConfig_xx.MCU_Type==0x01)
	{
		
		ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 1,ADC_SampleTime_1Cycles5);
		while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
		adc=ADC_GetConversionValue(ADC1);
		if ((adc>1522) || (adc<1346)) //7.0V~6.2V  ADJ=6.74V
		{
		   ERORR_VALUE=VDD_Test_false;
		   return 0;
		}
		ADC_RegularChannelConfig(ADC1,ADC_Channel_2, 1,ADC_SampleTime_1Cycles5);
		Delay_1ms(1);
		while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
		adc=ADC_GetConversionValue(ADC1);

		if ((adc >2775)||(adc<2450))  //13v~11.5v  ADJ=12.27V
		{
			ERORR_VALUE=VPP_Test_false;
			return 0;
		}
	}
	return 1;

}
