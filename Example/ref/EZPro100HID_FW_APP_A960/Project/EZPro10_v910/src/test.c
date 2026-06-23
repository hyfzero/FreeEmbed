
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "spi.h"
#include "power.h"
#include "OTPRW.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"
#include "LCD.h"
#include "wx_i2c.h"

void Writer_Test()
{
	u8 fun_state;
	u8 temp;
	u8 keyflag;
	u16 adc;
	u32 vddValue,vppValue;
	//u16 keycount;

	fun_state=0;
	keyflag=0;
	MCP42050_ADJ(ADJ_VPP,150);//vpp value
	MCP42050_ADJ(ADJ_VDD,51); //vdd value
	Initial_Lcd();
	displayTestMode();
	displayVDD_VPP(0,0);
	while(1)
	{

		if (GPIO_ReadInputDataBit(KEY)==0)
		{
			Delay_1ms(10);
			//keycount ++;
			if (GPIO_ReadInputDataBit(KEY)==0)
			{
				fun_state ++;
				keyflag=1;
				displayTestState(fun_state);
                while(GPIO_ReadInputDataBit(KEY)==0);
			}		
		}

		if ((fun_state==1) && (keyflag==1) )
		{			
			if (FMReadID()==1)
			{
                // Freq(1); //ok
				LEDBUSY_On;
				LEDNG_On;
				LEDOK_On;
				
			}

			MCP42050_ADJ(ADJ_VPP,110);//vpp value=9.5V MTP's VPP value 
			Delay_1ms(1);
			POWER_ON(vpp12,vdd30);
			Delay_1ms(10);

			//adc test
			ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 1,ADC_SampleTime_1Cycles5);
			while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
			adc=ADC_GetConversionValue(ADC1);
			vddValue=adc*46; //0.004679
			
			ADC_RegularChannelConfig(ADC1,ADC_Channel_2, 1,ADC_SampleTime_1Cycles5);
			while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
			adc=ADC_GetConversionValue(ADC1);
			vppValue=adc*47;

			displayVDD_VPP(vddValue,vppValue);
			keyflag=0;

		}
		else if ((fun_state==2) && (keyflag==1))
		{

			MCP42050_ADJ(ADJ_VPP,150);//vpp value
			Delay_1ms(1);
			POWER_ON(vpp12,vdd65);
			Delay_1ms(10);

			LEDOK_Off;
			//LEDBUSY_Off;
			//LEDNG_Off;
			//adc test
			ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 1,ADC_SampleTime_1Cycles5);
			while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
			adc=ADC_GetConversionValue(ADC1);
			vddValue=adc*47; //0.004679

			ADC_RegularChannelConfig(ADC1,ADC_Channel_2, 1,ADC_SampleTime_1Cycles5);
			while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET); //检查制定ADC标志位置1与否 ADC_FLAG_EOC 转换结束标志位
			adc=ADC_GetConversionValue(ADC1);
			vppValue=adc*47;

			displayVDD_VPP(vddValue,vppValue);
			keyflag=0;

		}
		else if ((fun_state==3)&& (keyflag==1))
		{
			IIC_Write(0x30,0xaa);
			IIC_Write(0x31,0x55);
			temp=IIC_Read(0x30);
			if (temp==0xaa)
			{
				temp=IIC_Read(0x31);
				if (temp==0x55)
				{
					LEDNG_Off;
				}
			}
			keyflag=0;
		}
		else if ((fun_state==4)&& (keyflag==1))
		{
			LEDBUSY_Off;
			ConfigTimer();
			keyflag=0;
		}
		else if (fun_state==5)
		{
			LEDNG_On;
			OTP_SCK_0;
			OTP_SDI_1;
			NOP;
			OTP_SCK_1;
			OTP_SDI_0;
			
		}
		else if (fun_state==6)
		{
			LEDNG_Off;
			LEDOK_On;
            //IRC_wave();  //mc10p02r IRC
		}
		else if (fun_state==7) //test OTP_SDO
		{
			//LEDOK_Off;
			//LEDBUSY_On;
			OTP_SCK_1;
			OTP_SDI_1;
			if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN)==1)
			{
				LEDOK_On;
				Delay_1ms(200);
			}
			OTP_SCK_0;
			OTP_SDI_0;
			if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN)==0)
			{
				LEDOK_Off;
				Delay_1ms(200);
			}
			
		}
		else if (fun_state==8)
		{
			LEDOK_Off;
			LEDNG_Off;
			LEDBUSY_Off;
			fun_state=0;
		}
		
	}
}

