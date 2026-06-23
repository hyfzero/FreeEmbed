

#include "stm32f10x.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"
#include "MC301Pro.h"
#include "erorrNum.h"

u8 MC301_Program()
{

}

void MC301_Instruct(u16 OTPData)
{
  u8 i,temp,a;

  OTP_SCK_0;
  for(i=0;i<6;i++)
  {
    OTP_SCK_1;
    a = OTPData >>i;
    if ((a & 0x01)==1)
    {
            OTP_SDI_1;
    } 
    else
    {
            OTP_SDI_0;
    }
    NOP;
    OTP_SCK_0;
    NOP;
  }
}

u16 MC301_R( )
{
        u8 i,temp; //temp use for Nop instruce
		u16 a,read_data;
	//read 
	MC301_Instruct(DataRe);
	NOP;
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	read_data=0;
	for(i=0;i<14;i++)
	{
		OTP_SCK_1;  //1
		NOP;
		a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
		read_data |= a<<i;

		OTP_SCK_0;//D7
		NOP;
	}
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;

	return read_data;

}
void MC301_W(u16 OTPData)
{
	  u8 i,temp;
      u16 wData; 
	MC301_Instruct(DataWr);

	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	for(i=0;i<14;i++)
	{
		OTP_SCK_1;  //1

		wData =OTPData>>i;
		if ((wData & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_0;
		NOP;
	}
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;

	MC301_Instruct(ProStr);
	Delay_100us(DeviceConfig_xx.ProTime);
	MC301_Instruct(ProEnd);

}

void MC301_WriteByte(u16 OTPAddr,u16 OTPData)
{
	u8 i,temp;
	u16 a;
	MC301_Instruct(SetAddr);

	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	for(i=0;i<14;i++)
	{
		OTP_SCK_1;  //1

		a =OTPAddr>>i;
		if ((a & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_0;
		NOP;
	}
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	
	MC301_W(OTPData);
	//MC301_Instruct(ProStr);
	//Delay_100us(1);
	//MC301_Instruct(ProEnd);
}

u16 MC301_ReadByte(u16 OTPAddr)
{
	u8 i,temp;
	u16 a;

	MC301_Instruct(SetAddr);
	NOP;
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	for(i=0;i<14;i++)
	{
		OTP_SCK_1;  //1

		a =OTPAddr>>i;
		if ((a & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_0;
		NOP;
	}
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	
	a=MC301_R();

	return a;
}


//mc30p01 irc
////IRC mainfuntion
//u8 M301IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
//{
//
//	u8 i;
//	u8 mm=0x80;
//       // u16 id;
//
//	POWER_OFF(vpp00,vdd00);
////vdd 5.0v
//	//MCP42050_ADJ(ADJ_VDD,51);
//	OTP_SCK_0;
//	OTP_SDI_0;
//
//	VDD30V_On;
//	Delay_1ms(1); 
//	IO5V_Off;
//	Delay_1ms(10);
//	CLN_VDD2GND_Off;
//	VPP12V_On;
//
//	Delay_1ms(5);
//	
//	//id=MC301_ReadByte(0X3FFF); //read MCU ID
//
//        MC301_Instruct(TestMod);
//	for(i=0;i<5;i++)
//	{
//		MC301_IRC_INST(0X0000); //5 NOP
//	}
//	
//	MC301_IRC_INST(0x0b10); //	movai 0x00 ; Firc/128
//	MC301_IRC_INST(0x15f2); //	movra 0x72
//	
//	MC301_IRC_INST(0x0b60); //	movra 0x60 ; p62 output Firc/128
//	MC301_IRC_INST(0x15f0); //	movra 0x70 ;
//	
//
//
//	OPTION_FRT=0; //initial 
//	for(i=0;i<8;i++)
//	{
//		//mm=0xff;
//		//M201FTSendData(IRC_Option,IRC_Addr,mm);
//		MC301FTtest(mm);
//		if (IRC_VALUE == IRC_FreqType)
//		{
//			break;
//		}
//		if (IRC_VALUE<IRC_FreqType)
//		{
//			mm |=0x80 >>i;
//		} 
//		else
//		{
//			mm &=~(0x80>>i);
//		}
//		mm |=0x40>>i;
//	}
//
//	MC301FTtest(mm);
//
//	if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
//	{
//		OPTION_FRT=mm;
//		return 1;
//	} 
//	else
//	{
//		OPTION_FRT=0XFF;
//		return 0;
//	}
//}
//

u8 M301IRC_ModeInCheck(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{

  u8 i;
  u8 mm=0x80;

  MC301_Instruct(TestMod);
  for(i=0;i<5;i++)
  {
    MC301_IRC_INST(0X0000); //5 NOP
  }

  MC301_IRC_INST(0x0b10); //	movai 0x00 ; Firc/128
  MC301_IRC_INST(0x15f2); //	movra 0x72

  MC301_IRC_INST(0x0b60); //	movra 0x60 ; p62 output Firc/128
  MC301_IRC_INST(0x15f0); //	movra 0x70 ;
	
  mm=OPTION_FRT;
  MC301FTtest(mm);

  if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
  {
    OPTION_FRT=mm;
    return 1;
  } 
  else
  {
    OPTION_FRT=0XFF;
    return 0;
  }
}


void MC301FTtest(u8 IRC_Opiton)
{
	u16 temp=0x0b00;
	u16 i;
	u16 cnt[3];

	temp=temp+IRC_Opiton;
	MC301_IRC_INST(temp); //	movra 0x60 ; p62 output Firc/128
	MC301_IRC_INST(0x15f1); //	movra 0x70 ;

	MC301_IRC_INST(0X0000); //NOP

	//-------------------------
	Delay_1ms(10); //wait same time

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

	IRC_VALUE= (u16)((cnt[0]+cnt[1]+cnt[2])/3 );

	//-----------------------------
}

void MC301_IRC_INST(u16 OTPData)
{
	u8 i,temp;
	u16 wData; 
	//MC301_Instruct(DataWr);

	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;
	for(i=0;i<14;i++)
	{
		OTP_SCK_1;  //1

		wData =OTPData>>i;
		if ((wData & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_0;
		NOP;
	}
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;
	NOP;

	//4 CLK Q1,Q2,Q3,Q4
	OTP_SCK_1;
	NOP;
	OTP_SCK_0; //Q1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;//Q2
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;//Q3
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;//Q4
	NOP;

}

//u16 MC301_ReadS()
//{
//	u16 a;
//	MC301_Instruct(AddrINC);
//	//read 
//	a=MC301_R();
//	return a;
//
//}
//
//void MC301_WriteS(u16 OTPData)
//{
//
//	MC301_Instruct(AddrINC);
//	MC301_W(OTPData);
//
//}