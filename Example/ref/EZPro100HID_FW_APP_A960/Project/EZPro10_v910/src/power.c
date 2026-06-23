
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "global.h"
#include "Config.h"
#include "spi.h"
#include "OTPRW.h"
#include "MTPRW.h"
#include "power.h"
#include "IRC.h"

void PowerInitial()
{
	GPIO_InitTypeDef GPIO_InitStructure;



	//initial PA9 -> VPP, PA0 ->VDD65,PA5 -> Power18V,PA8--CLKoutput
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_9 |GPIO_Pin_0| GPIO_Pin_5 | GPIO_Pin_10 ;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	//gpio PE15 ->VDD30V
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE,&GPIO_InitStructure);

	//control 164245A VCC
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE,&GPIO_InitStructure);

	Power18V_Off;
	VPP12V_Off;
	VDD30V_Off;
	VDD65V_Off;
	IO5V_Off;
	

}

//power on vpp ,vdd
void POWER_ON(u8 vpp,u8 vdd	)
{
	//u16 i;
	/*Power18V_On;
	Delay_1ms(100);*/
	CLN_VDD2GND_Off;
	Delay_1us(10);
	SDIO_Turnto_Output();
	OTP_SDIO_0;
	OTP_SCK_0;
	if (vpp==vpp12)
	{
		VPP12V_On;
	}
	else
	{
		VPP12V_Off;
	}

	if (vdd== vdd65)
	{
		VDD65V_On;
        Delay_1ms(1);
        VDD30V_Off;
        //Delay_1ms(2);
		IO5V_On;
                
		
	}
	else if (vdd == vdd30)
	{
		VDD30V_On;
		VDD65V_Off;
		//Delay_1ms(1);
		Delay_1us(100);
		//IO5V_Off;
		
		if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType== WriteFPGA)
		{
			VDD_IO_On;
		}
	}
	else
	{
		VDD65V_Off;
		VDD30V_Off;
	}

	SDIO_Turnto_Output();
	//OTP_SDIO_1;
	//OTP_SCK_1;
	OTP_SDI_1;
	//TIM_REF_1;
//	CP_1;
}

//power off
void POWER_OFF(u8 vpp,u8 vdd)
{
	if (vpp==vpp00)
	{
		VPP12V_Off;
	}
	if(vdd==vdd00)
	{
		VDD30V_Off;
		VDD65V_Off;
		IO5V_Off;
		CLN_VDD2GND_On;
	}

	SDIO_Turnto_Input();
	OTP_SDIO_0;
	OTP_SCK_0;
	OTP_SDI_0;
    VDD_IO_Off;
    VPP_IO_Off;
	
}

void testPower()
{
	POWER_ON(vpp12,vdd65);
	//POWER_ON(vpp00,vdd30);
	Delay_1ms(500);
	POWER_OFF(vpp00,vdd00);
	Delay_1ms(500);
}