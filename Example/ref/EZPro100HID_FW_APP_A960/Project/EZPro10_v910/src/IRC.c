
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "IRC.h"
#include "Config.h"

//TIM3_CH1,TIM3_CH2(pc6,pc7)
void  ConfigTimer(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
//  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO,ENABLE); //AFIO时钟必须开启。
  RCC_APB1PeriphClockCmd(  RCC_APB1Periph_TIM3,ENABLE);

  /*  STM32 默认也 TIM3是在 PA6,PA7,PB0,PB1 脚，需要Remap到新的管脚上去。 */
  GPIO_PinRemapConfig(GPIO_FullRemap_TIM3,ENABLE);

  //PC6 ->OSCO(CP),PC7 ->OSCI(TIME_REF),
  /*
  GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  TIM_REF_0;

  CP_0;
  */
  ////TIM_DeInit(TIM3);
  TIM_TimeBaseStructure.TIM_Prescaler = 0; //不再对时钟分频
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned3;//trig down
  //TIM_TimeBaseStructure.TIM_Period =3600+9216/Fsys;//用来计数，控制输出频率.Fsys是要校准的单片机主频率
  TIM_TimeBaseStructure.TIM_Period =3600+DeviceConfig_xx.IRC_FrType; //3600+9216/Fsys
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 9;//9
  TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
  //
  //Channel1 configuration
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性设置
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
  TIM_OCInitStructure.TIM_Pulse =Tcp_ref;
  TIM_OC2Init(TIM3,&TIM_OCInitStructure);

  //TIM_CtrlPWMOutputs(TIM3, ENABLE);
  //TIM_Cmd(TIM3,ENABLE);

}





void ConfigTIM4()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  //TIM_OCInitTypeDef TIM_OCInitStructure;

  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //GPIO_Mode_IN_FLOATING;// not AIN modul
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);


   RCC_APB1PeriphClockCmd(  RCC_APB1Periph_TIM4,ENABLE);	

   TIM_TimeBaseStructure.TIM_Period=0xffff; //定时时间
   TIM_TimeBaseStructure.TIM_Prescaler=0x00;   //时钟源分频数 F=72000/(x+1)
   TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
   TIM_TimeBaseStructure.TIM_ClockDivision=0x00;// input clock division


   TIM_TimeBaseInit(TIM4,&TIM_TimeBaseStructure);
   TIM_ETRClockMode2Config(TIM4,TIM_ExtTRGPSC_OFF,TIM_ExtTRGPolarity_NonInverted,0x00);
   TIM_SetCounter(TIM4,0);

   // timer=4ms
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE); //initial timer3
   TIM_TimeBaseStructure.TIM_Period=36000;
   TIM_TimeBaseStructure.TIM_Prescaler=19; // t=36000(19+1)/72000;
   TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
   TIM_TimeBaseStructure.TIM_ClockDivision=0;

   TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
   //---------------------------------------------------------------------------
   TIM_OCInitTypeDef  TIM_OCInitStructure;
   TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
   TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
   TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable;
   TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性设置
   TIM_OCInitStructure.TIM_Pulse =Tcp_ref;
   TIM_OC2Init(TIM3,&TIM_OCInitStructure);
   TIM_CtrlPWMOutputs(TIM3,ENABLE);
 //  CP_0;

   //-----------------------------------------------------------------------------

   TIM_SetCounter(TIM3,0);


   //CH4 output
   TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
   TIM_Cmd(TIM4,DISABLE);
   TIM_Cmd(TIM3,DISABLE);	
}

/*** vave generating by delay ****/

void IRCInitial()
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin= GPIO_Pin_6 |GPIO_Pin_7|GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC,&GPIO_InitStructure);

  GPIO_ResetBits(GPIOC,GPIO_Pin_8);
  CP_0;
  TIM_REF_0;
}

/*
u8 IRC_wave()
{
	u8 cnt;
	//u8 RCvalue;
	//change initial time3
	TIM_CtrlPWMOutputs(TIM3, ENABLE);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM3,ENABLE);
	cnt=0;
	while(cnt<9)
	{
		CP_0;
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		while(TIM_GetITStatus(TIM3,TIM_IT_Update) == RESET) ;
		CP_1;
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		while(TIM_GetITStatus(TIM3,TIM_IT_Update) == RESET) ;
		cnt++;
	}

	TIM_Cmd(TIM3,DISABLE);
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性设置
	TIM_OCInitStructure.TIM_Pulse =Tcp_ref;
	TIM_OC2Init(TIM3,&TIM_OCInitStructure);
	TIM_CtrlPWMOutputs(TIM3,DISABLE);
    
	IRC_VALUE=OTP_ReadByte(0x0200);

	return 1;


}

//test RC

u8 IRC_FTest(u16 MAXFRE,u16 MINFRE )
{
	u16 cnt[3];
	u16 cnt_a;
	u8 i;

	//TIM3 Initial
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE); //initial timer3
	TIM_TimeBaseStructure.TIM_Period=36000;
	TIM_TimeBaseStructure.TIM_Prescaler=0x01;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision=0;

	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
	TIM_SetCounter(TIM3,0);


	//CH4 output
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);

	//POWER_ON(vpp12,vdd65);
        //VDD65V_On;
        //VDD30V_Off;
	//IO5V_On;
        //--------------
        VDD30V_On; 
	VDD65V_Off;
	IO5V_Off;
		CLN_VDD2GND_Off;
        VPP12V_On;
	Delay_1ms(100);

	OTP_SCK_1;
	Delay_100us(1);
	OTP_SCK_0;
	Delay_1ms(100);

    for (i=0;i<3;i++)
	{
		TIM_SetCounter(TIM3,0);
		TIM_SetCounter(TIM4,0);
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		TIM_Cmd(TIM4,ENABLE);
		TIM_Cmd(TIM3,ENABLE);
		while(TIM_GetITStatus(TIM3,TIM_IT_Update) == RESET) ;
		cnt[i] = TIM_GetCounter(TIM4);
    }
	TIM_Cmd(TIM3,DISABLE);
	TIM_Cmd(TIM4,DISABLE);

	cnt_a= (u16)((cnt[0]+cnt[1]+cnt[2])/3 );

	if ((MAXFRE>=cnt_a) && (cnt_a >=MINFRE)) // 逻辑与
	{
		i=1;	
        //        LEDOK_On;
	}
	else
	{
		i=0;
        //        LEDOK_Off;
	}
	//test
	//assert_param(IS_GPIO_ALL_PERIPH(GPIOC));
	//assert_param(IS_GPIO_PIN(GPIO_Pin_10));
	//GPIOC->ODR ^=  GPIO_Pin_10;

	POWER_OFF(vpp00,vdd00);

	return i;


}*/