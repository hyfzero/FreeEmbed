
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "global.h"
#include "power.h"

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
		Delay_1ms(7);
		IO5V_Off;
		
	}
	else
	{
		VDD65V_Off;
		VDD30V_Off;
	}

	OTP_SCK_1;
	OTP_SDI_1;
	//TIM_REF_1;
	CP_1;
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
	}


	//TIM_REF_0;
	//Power18V_Off;
}