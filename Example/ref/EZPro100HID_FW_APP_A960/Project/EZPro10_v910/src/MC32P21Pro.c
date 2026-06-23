
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
#include "MC32P21Pro.h"


void MC321_MODEL_IN(u8 VddFlag)
{
  u8 temp;
  //power on & model in
  POWER_OFF(vpp00,vdd00);
  Delay_1ms(100);
  OTP_SCK_0;
  OTP_SDI_0; 

  //input wave
  if (VddFlag==vdd65)
  {
    VDD65V_On;
    NOP;
    IO5V_On;
  } 
  else
  {
    VDD30V_On;
    IO5V_Off;
  }

#ifdef debug 
  VDD_IO_On;
#endif

  NOP;
  CLN_VDD2GND_Off;
  VPP12V_On;

#ifdef debug 
  VPP_IO_On;
#endif

  //Delay_100us(2);
  Delay_100us(5);
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;

  VPP12V_Off;
  CLN_VDD2GND_On;

#ifdef debug  
  VPP_IO_Off;
#endif

  Delay_100us(5);
  //Delay_100us(2);
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;

  CLN_VDD2GND_Off;
  VPP12V_On;
#ifdef debug  
  VPP_IO_On;
#endif

  Delay_100us(5);

  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  NOP;
  VPP12V_Off;
  CLN_VDD2GND_On;
#ifdef debug  
  VPP_IO_Off;
#endif
  Delay_100us(5);
  OTP_SDI_1;
  NOP;
  OTP_SDI_0;
  Delay_1ms(1);
}

/*******************************************************************************
* Function Name  : MC32_MODEL_IN
* Description    : 
* Input          : VddFlag vdd65：VDD=6.5V；vdd30：VDD=3V.
                   mode    mode32p21：MC32P21进模式时序；mode30p6060：MC30P6060进模式时序.
                   Tdly    VDD上电后延时一定时间再给进模式时序.
* Output         : None.
* Return         : None.
*******************************************************************************/
void MC32_MODEL_IN(u8 VddFlag,u8 mode,u16 Tdly)
{
     u8 temp;
     u16 i;
     //power on & model in
     POWER_OFF(vpp00,vdd00);
     Delay_1ms(100);
     OTP_SCK_0;
     OTP_SDI_0; 
  
     if(mode==mode32p21)
     {
          //input wave
          if (VddFlag==vdd65)
          {
               VDD65V_On;
               NOP;
               IO5V_On;
          } 
          else
          {
               VDD30V_On;
               IO5V_Off;
          }

          #ifdef debug 
               VDD_IO_On;
          #endif

          NOP;
          CLN_VDD2GND_Off;
          VPP12V_On;

          #ifdef debug 
               VPP_IO_On;
          #endif

          //Delay_100us(5);

          for (i=0 ;i<Tdly;i++)
          {
               NOP_5us;
          }  

          OTP_SDI_1;
          NOP;
          //NOP_5us;
          OTP_SDI_0;
          NOP;
          //NOP_5us;
          OTP_SDI_1;
          NOP;
          //NOP_5us;
          OTP_SDI_0;
          NOP;
          //NOP_5us;
          OTP_SDI_1;
          NOP;
          //NOP_5us;
          OTP_SDI_0;
          NOP;
          OTP_SDI_1;
          NOP;
          OTP_SDI_0;
          NOP;

          VPP12V_Off;
          CLN_VDD2GND_On;

          #ifdef debug  
               VPP_IO_Off;
          #endif

          Delay_100us(5);
          //Delay_100us(2);
          OTP_SDI_1;
          NOP;
          OTP_SDI_0;
          NOP;

          CLN_VDD2GND_Off;
          VPP12V_On;
          #ifdef debug  
               VPP_IO_On;
          #endif

          Delay_100us(5);

          OTP_SDI_1;
          NOP;
          OTP_SDI_0;
          NOP;
          OTP_SDI_1;
          NOP;
          OTP_SDI_0;
          NOP;
          VPP12V_Off;
          CLN_VDD2GND_On;
          #ifdef debug  
               VPP_IO_Off;
          #endif
          Delay_100us(5);
          OTP_SDI_1;
          NOP;
          OTP_SDI_0;
          Delay_1ms(1);
     }
     else if(mode==mode30p6060)
     {
          //input wave
          if (VddFlag==vdd65)
          {
               VDD65V_On;
               NOP;
               IO5V_On;
          } 
          else
          {
               VDD30V_On;
               IO5V_Off;
          }
//          NOP;
//          NOP;
//          NOP;NOP;NOP;NOP;NOP;NOP;
//          NOP;NOP;NOP;NOP;NOP;NOP;
//          Delay_1us(5);
//          NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
          for (i=0 ;i<Tdly;i++)
          {
               NOP_5us;
          } 

          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_0;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;//0b1011          
          OTP_SDI_0;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;//0b0111

          OTP_SDI_0;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;//0b0111

          OTP_SDI_0;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_0;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;
          OTP_SDI_1;
          OTP_SCK_1;
          NOP;
          OTP_SCK_0;
          NOP;//0b0101  
          OTP_SDI_0;
          Delay_1ms(1);
          NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;       
     }
}

u16 MC321_PreRead(u8 command)
{
    u8 i,temp; //temp use for Nop instruce
    u16 a,read_data;
    //read 
    MC301_Instruct(command);
    NOP;
    NOP;

    read_data=0;
    for(i=0;i<16;i++)
    {
        OTP_SCK_1;  //1
        NOP;
        a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
        read_data |= a<<i;

        OTP_SCK_0;//D7
        NOP;
    }

    return read_data;
}


void SetAddress(u16 OTPAddr)
{
  u8 i,temp;
  u16 a;
  MC301_Instruct(SetAddr);
  for(i=0;i<16;i++)
  {
    a =OTPAddr>>i;
    if ((a & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }  
}

void InputData(u16 OTPData)
{
  u8 i,temp;
  u16 a;
  
  MC301_Instruct(DataWr);
  for(i=0;i<16;i++)
  {
    a =OTPData>>i;
    if ((a & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }  
}
void SetOption(u16 OTPAddr,u16 OTPData)
{
//  u16 a;  
//  a=MC321_ReadByte(OTPAddr); 
//  a=MC321_ReadByte(0x8000+(OTPAddr&0x000F)); 
  SetAddress(OTPAddr);           //1．	通过0x15命令设置OPTION的映射地址
  InputData(OTPData);            //2．	通过0x16命令输入数据  
  MC301_Instruct(SetOpt);        //3．  通过0x1b命令完成OPTION修改 
  Delay_1ms(1);
//  a=MC321_ReadByte(OTPAddr); 
//  a=MC321_ReadByte(0x8000+(OTPAddr&0x000F)); 
}

u16 MC321_R()
{
	u8 i,temp; //temp use for Nop instruce
	u16 a,read_data;
	//read 
	MC301_Instruct(DataRe);
	NOP;
	NOP;

	read_data=0;
	for(i=0;i<16;i++)
	{
		OTP_SCK_1;  //1
		NOP;
       //NOP;//徐明明屏蔽
		a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
		read_data |= a<<i;

		OTP_SCK_0;//D7
		NOP;
        //NOP;//徐明明屏蔽
	}

	return read_data;
}
/*******************************************************************************
* Function Name  : MC321_R_Margin.
* Description    : 不同模式读数据（不需要设置读取地址，之前设置过初始地址，读过数据，并且发了地址加1命令）
* Input          : ModeNum 0：普通模式；1：Margin-1模式；2：Margin-2模式
* Output         : read_data:读取到的数据
*******************************************************************************/
u16 MC321_R_DiffMode(u8 ModeNum)
{
  u8 i,temp; //temp use for Nop instruce
  u16 a,read_data;
  //set read mode
  MC321_SetRmode(ModeNum);

  //read 
  MC301_Instruct(DataRe);
  NOP;
  NOP;

  read_data=0;
  for(i=0;i<16;i++)
  {
    OTP_SCK_1;  //1
    NOP;
    //NOP;//徐明明屏蔽
    a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
    read_data |= a<<i;

    OTP_SCK_0;//D7
    NOP;
    //NOP;//徐明明屏蔽
  }
  return read_data;
}
/*******************************************************************************
* Function Name  : MC321_SetRmode.
* Description    : 设置不同读取模式
* Input          : ModeNum 0：普通模式；1：Margin-1模式；2：Margin-2模式
* Output         : none
*******************************************************************************/
void MC321_SetRmode(u8 ReadMode)
{
  u8 i,temp; //temp use for Nop instruce
  u16 a,wData;
  if(ReadMode == 0)//普通读取
  {
    a = 0x0000;
  }
  else if(ReadMode == 1)//Margin-1模式读取
  {
    a = 0x0006;
  }
  else if(ReadMode == 2)//Off State Margin模式读取
  {
    a = 0x0005;
  }
  MC301_Instruct(SetReadMode);
  NOP;NOP;
  for(i=0;i<16;i++)
  {         
    wData =a>>i;
    if ((wData & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }  
}

void MC321_W(u16 OTPData)
{
  u8 i,temp;
  u16 wData; 

  MC301_Instruct(DataWr);
  NOP;NOP;
  for(i=0;i<16;i++)
  {         
    wData =OTPData>>i;
    if ((wData & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }
  //program time
    MC301_Instruct(ProStr);
    for(i=0;i<8;i++)
    {
        NOP;
        OTP_SCK_1;
        NOP;
        OTP_SCK_0;	
    }
    Delay_10us(7);//102us
    for(i=0;i<8;i++)
    {
        NOP;
        OTP_SCK_1;
        NOP;
        OTP_SCK_0;	
    } 

}

/*
void MC321_W(u16 OTPData)
{
  u8 i,temp;
  u16 wData; 
  u32 count;

  MC301_Instruct(DataWr);
  NOP;NOP;
  for(i=0;i<16;i++)
  {         
    wData =OTPData>>i;
    if ((wData & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }
  if ((DeviceConfig_xx.MCU_ID==0x3401)||(DeviceConfig_xx.MCU_ID==0x3081)
  ||(DeviceConfig_xx.MCU_ID==0x3316)||(DeviceConfig_xx.MCU_ID==0x7022)||(DeviceConfig_xx.MCU_ID==0x5222)
    ||(DeviceConfig_xx.MCU_ID==0x9033)||(DeviceConfig_xx.MCU_ID==0x5312))
  {
    wData=MC321_PreRead(PDataRe);
    if (wData!=OTPData)
    {
        return ;
    }
  }
  //program time
  if(DeviceConfig_xx.MCU_ID==0x3316||DeviceConfig_xx.MCU_ID==0x3378||DeviceConfig_xx.MCU_ID==0x6060
    ||DeviceConfig_xx.MCU_ID==0x3111||DeviceConfig_xx.MCU_ID==0x3401||DeviceConfig_xx.MCU_ID==0x5312
    ||DeviceConfig_xx.MCU_ID==0x7022||DeviceConfig_xx.MCU_ID==0x7011||DeviceConfig_xx.MCU_ID==0x7212
    ||DeviceConfig_xx.MCU_ID==0x7511||DeviceConfig_xx.MCU_ID==0x5222||DeviceConfig_xx.MCU_ID==0x7311)//编程时间 90us - 110us
  {
    MC301_Instruct(ProStr);
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    }
    Delay_10us(7);//102us
//    Delay_1us(10);
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    } 
  }
  else if(DeviceConfig_xx.MCU_ID==0x9029||DeviceConfig_xx.MCU_ID==0x9033)//编程时间 300us
  {
    MC301_Instruct(ProStr);
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    }
    Delay_10us(21);
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    } 
  }
  else
  {
    MC301_Instruct(ProStr);
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    }
    if (DeviceConfig_xx.MCU_Type==0x02)
    {
      count=0;
      while(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
      {
        count++;
      }
    } 
    else
    {
      Delay_100us(DeviceConfig_xx.ProTime);
      Delay_100us(DeviceConfig_xx.ProTime);
      Delay_100us(DeviceConfig_xx.ProTime);    
    }
    for(i=0;i<8;i++)
    {
      NOP;
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;	
    }
  }
}

*/
/*******************************************************************************
* Function Name  : MC321_W_100us
* Description    : 有Margin功能的芯片烧写 时间100us
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void MC321_W_100us(u16 OTPData)
{
  u8 i,temp;
  u16 wData; 

  MC301_Instruct(DataWr);
  NOP;NOP;
  for(i=0;i<16;i++)
  {         
    wData =OTPData>>i;
    if ((wData & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }

  //确认编程数据正确
  wData=MC321_PreRead(PDataRe);
  if (wData!=OTPData)
  {
      return ;
  }

  //program time
  MC301_Instruct(ProStr);
  for(i=0;i<8;i++)
  {
    NOP;
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;	
  }
  Delay_10us(7);//102us
  for(i=0;i<8;i++)
  {
    NOP;
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;	
  } 
}
void MC321_WriteByte(u16 OTPAddr,u16 OTPData)
{
	u8 i,temp;
	u16 a;
	MC301_Instruct(SetAddr);
	NOP;
	NOP;
	for(i=0;i<16;i++)
	{
		a =OTPAddr>>i;
		if ((a & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		OTP_SCK_1;  //1
		NOP;
		OTP_SCK_0;
		NOP;
	}

	MC321_W(OTPData);
}

u16 MC321_ReadByte(u16 OTPAddr)
{
	u8 i,temp;
	u16 a;
	MC301_Instruct(SetAddr);
	for(i=0;i<16;i++)
	{
		a =OTPAddr>>i;
		if ((a & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		OTP_SCK_1;  //1
		NOP;
		OTP_SCK_0;
		NOP;
	}

	a=MC321_R();

	return a;
}
//不同读取模式读数据
/*******************************************************************************
* Function Name  : MC321_ReadByte_DiffMode.
* Description    : 不同模式读数据（设置读取地址）
* Input          : ModeNum 0：普通模式；1：Margin-1模式；2：Margin-2模式
*                  OTPAddr：读取地址
* Output         : a读取到的数据
*******************************************************************************/

u16 MC321_ReadByte_DiffMode(u8 ModeNum,u16 OTPAddr)
{
	u8 i,temp;
	u16 a;
        
        //set read mode 
        MC321_SetRmode(ModeNum);
        //read
	MC301_Instruct(SetAddr);
	for(i=0;i<16;i++)
	{
		a =OTPAddr>>i;
		if ((a & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		OTP_SCK_1;  //1
		NOP;
		OTP_SCK_0;
		NOP;
	}

	a=MC321_R();

	return a;
}

//--------------------------------------------------------
u8 M321IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{
  u8 i,vdsel;
  u8 mm=0x80;
  //u8 temp;
   u16 temp0_FT,temp1_FT ;
  //power on & model in
  //MC321_MODEL_IN(vdd30);
  MC32_MODEL_IN(vdd30,mode32p21,delay_1ms);
  Delay_1ms(1);
  MC301_Instruct(TestMod);

  for(i=0;i<5;i++)
  {
    MC321_IRC_INST(0X0000); //5 NOP
  }
  MC321_IRC_INST(0x3c74) ; //movai	0x80 ; //Fosc/256
  MC321_IRC_INST(0x57f9) ; //movra	0x1f9
  MC321_IRC_INST(0x3c00) ; //movai	0x00
  MC321_IRC_INST(0x57fa) ; //movra	0x1fa
  if(DeviceConfig_xx.ProVDD==0x2821) 
  {
    if (FMReadOne(Addr_Flash_Option+3)==1)
    {
      vdsel =Rxdata;
    }
    else
    {
      ERORR_VALUE=FM_Read_false;
      return 0;
    }
    MC321_IRC_INST(0x3cff) ; //movai	0xff ; //
    MC321_IRC_INST(0x57fb) ; //movra	0x1fb
    MC321_IRC_INST(0x3c00+vdsel) ; //movai	0x11
    MC321_IRC_INST(0x57fc) ; //movra	0x1fc

    MC321_IRC_INST(0x3c01) ;  //movai	0x01
    MC321_IRC_INST(0x57f8) ; 
    MC321_IRC_INST(0x0000) ;
    Delay_1ms(10);

    //temp value adj
    vdsel=0x10;
    for (i=0;i<4;i++)
    {
      MC321_IRC_INST(0x3c00+vdsel) ; //movai	0x11
      MC321_IRC_INST(0x57fc) ; //movra	0x1fc

      MC321FTtest(0xff);
      temp0_FT=IRC_VALUE;
      MC321FTtest(0x00);
      temp1_FT=IRC_VALUE;

      if((vdsel==0x1e)||(vdsel==0x02))
      {
        if ((temp0_FT>2109)&&(temp1_FT<2109))
          break;
        else if (vdsel==0x02)
        {
            vdsel=0x04;
        }
        else
            return 0;
      }

      if ((temp0_FT>2109)&&(temp1_FT<2109))
      {
         vdsel &= ~(0x10>>i);
      }
      else 
      {
         vdsel |= 0x10>>i;
      }

      vdsel |=0x08>>i;

    }
    if (vdsel!=0x04)
    {
        vdsel=vdsel+1;
    }

    MC321_IRC_INST(0x3c00+vdsel) ; //movai	0x11
    MC321_IRC_INST(0x57fc) ; //movra	0x1fc

  }  
  MC321_IRC_INST(0x3c01) ;  //movai	0x01
  MC321_IRC_INST(0x57f8) ; 
  MC321_IRC_INST(0x0000) ;
  Delay_1ms(10);
  mm=0x80;
  OPTION_FRT=0; //initial 
  for(i=0;i<8;i++)
  {
    //mm=0xff;
    //M201FTSendData(IRC_Option,IRC_Addr,mm);
    MC321FTtest(mm);
    if (IRC_VALUE == IRC_FreqType)
    {
      break;
    }
    if (IRC_VALUE<IRC_FreqType)
    {
      mm |=0x80 >>i;
    } 
    else
    {
      mm &=~(0x80>>i);
    }
    mm |=0x40>>i;
  }
  MC321FTtest(mm+1);
  temp0_FT=IRC_VALUE;

  MC321FTtest(mm);
  temp1_FT=IRC_VALUE;

  if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
  {
    mm=mm+1;
    IRC_VALUE=temp0_FT;
  }       
  if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
  {
    if (DeviceConfig_xx.ProVDD==0x2821)
    {
        OPTION_FRT=vdsel;
    }
    else
    {
        OPTION_FRT=0xff;
    }          
    OPTION_FRT2=mm;
    return 1;
  } 
  else
  {
    OPTION_FRT=0XFF;
    OPTION_FRT2=0xff;
    ERORR_VALUE=IRC_Value_false;
    return 0;
  }
}

//--------------------------------------------------------
u8 M321IRC_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 osccal)
{
  u8 i;
  u8 mm=0x80;
  
//  MC32_MODEL_IN(vdd30,mode32p21,delay_1ms);
//  Delay_1ms(1);
  MC301_Instruct(TestMod);

  for(i=0;i<5;i++)
  {
    MC321_IRC_INST(0X0000); //5 NOP
  }
  MC321_IRC_INST(0x3c74) ; //movai	0x80 ; //Fosc/256
  MC321_IRC_INST(0x57f9) ; //movra	0x1f9
  MC321_IRC_INST(0x3c00) ; //movai	0x00
  MC321_IRC_INST(0x57fa) ; //movra	0x1fa
   
  MC321_IRC_INST(0x3c01) ;  //movai	0x01
  MC321_IRC_INST(0x57f8) ; 
  MC321_IRC_INST(0x0000) ;
  Delay_1ms(10);
  
  mm=osccal;
  MC321FTtest(mm);
     
  if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
  {
    return 1;
  } 
  else
  {
    return 0;
  }
}



void MC321FTtest(u8 IRC_Option)
{
	u16 temp=0x3c00;
	u16 i;
	u16 cnt[3];

    if (DeviceConfig_xx.MCU_ID==0x3081)
    {
        temp=0x0b00+IRC_Option;
        MC321_IRC_INST(temp);
        MC321_IRC_INST(0x15fb);
    }
    else if (DeviceConfig_xx.MCU_ID==0x3401)
    {
        temp=0x0400+IRC_Option;
        MC321_IRC_INST(temp);
        MC321_IRC_INST(0x00bb);
    }
    else
    {
        temp=temp+IRC_Option;
        MC321_IRC_INST(temp);
        MC321_IRC_INST(0x57fb);
    }
    MC321_IRC_INST(0x0000);

	//----------------------------
	Delay_1ms(5);
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

}
//---------------------------------------------------------
//---------------------------------------------------------
u8 MC3081IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{
    u8 i,opbit_L,opbit_H;
    u8 mm=0x80;
    //u8 temp;
    u16 temp0_FT,temp1_FT ;
    //power on & model in
    //MC321_MODEL_IN(vdd30);
    MC32_MODEL_IN(vdd30,mode32p21,delay_150us);//150us

    Delay_1ms(1);

    temp1_FT=MC321_ReadByte(0XFFFF);

    MC301_Instruct(TestMod);
    
    if (DeviceConfig_xx.MCU_ID==0x3401)
    {
        CLN_VDD2GND_Off;
        VPP12V_On;  // W/R ADDR:0x38--0x39 when VPP=7.5v 
       // Delay_100us(5); 
    }
    Delay_1ms(1);

    for(i=0;i<10;i++)
    {
        MC321_IRC_INST(0X0000); //5 NOP
    }
    

    if (DeviceConfig_xx.MCU_ID==0x3401)
    {       
        MC321_IRC_INST(0x0405);              //movai           0x00    ;OPITON[12:8]=0x10   
        MC321_IRC_INST(0x00ba);              //movra           0x3a 
    
        if (FMReadOne(0x012120)==1)          //opbit addr 0x2120---0x2125, 0x2126: page index, 0x2127:timer value
        {
            opbit_L =Rxdata;
        }
        else
        {
            ERORR_VALUE=FM_Read_false;
            return 0;
        }

        MC321_IRC_INST(0x0400+opbit_L);              //movai           0x00    ;OPTION0[7:0]=0x00    
        MC321_IRC_INST(0x00b9);              //movra           0x39

        MC321_IRC_INST(0x040f);              //movai           0x00    ;addr8001[12:8]=0x00
        MC321_IRC_INST(0x00bc);              //movra           0x3c                         
        MC321_IRC_INST(0x0480);              //movai           0x80    ;addr8001[7:0]=0x80  
        MC321_IRC_INST(0x00bb);              //movra           0x3b

        if (FMReadOne(0x012120+4)==1)
        {
            opbit_L =Rxdata;
        }
        else
        {
            ERORR_VALUE=FM_Read_false;
            return 0;
        }
        if (FMReadOne(0x012120+5)==1)
        {
            opbit_H =Rxdata;
        }
        else
        {
            return 0;
        }
        
        OPTION_FRT2= opbit_L; //save return tempadj value

        MC321_IRC_INST(0x0400+opbit_H);             //movai           0x18    ;addr8002[12:8]=0x18 
        MC321_IRC_INST(0x00be);                     //movra           0x3e                         
        MC321_IRC_INST(0x0400+opbit_L);             //movai           0x0A    ;addr8002[7:0]=0x05 
        MC321_IRC_INST(0x00bd);                     //movra           0x3d 

        
        if (FMReadOne(0x012120+7)==1)  //0x2127:timer value
        {
            opbit_L =Rxdata;
        }
        else
        {
            ERORR_VALUE=FM_Read_false;
            return 0;
        }
        MC321_IRC_INST(0x0400+opbit_L);             //movai         0x01
        MC321_IRC_INST(0x0031);                     //iosw          t0cr

        MC321_IRC_INST(0x0401);                     //LHR    output from DSO 
        MC321_IRC_INST(0x00b8) ; 
        MC321_IRC_INST(0x0000) ;

    }
    else
    {   //mc30p081

        if (FMReadOne(0x012120)==1)
        {
            opbit_L =Rxdata;
        }
        else
        {
            ERORR_VALUE=FM_Read_false;
            return 0;
        }

        //OPBIT0
        MC321_IRC_INST(0X0b00+opbit_L);     //movai   0x07
        //MC321_IRC_INST(0X0b07);     //movai   0x07
        MC321_IRC_INST(0x15f9);     //movra   0x79
        MC321_IRC_INST(0X0b35);     //0x35
        MC321_IRC_INST(0x15fa);
        //OPBIT1 
        MC321_IRC_INST(0X0b80);    
        MC321_IRC_INST(0x15fb);
        MC321_IRC_INST(0X0b36);     //0x36
        MC321_IRC_INST(0x15fc);

        if (FMReadOne(0x012120+4)==1)
        {
            opbit_L =Rxdata;
        }
        else
        {
            ERORR_VALUE=FM_Read_false;
            return 0;
        }
        if (FMReadOne(0x012120+5)==1)
        {
            opbit_H =Rxdata;
        }
        else
        {
            return 0;
        }

        OPTION_FRT2=opbit_L;

        MC321_IRC_INST(0X0b00+opbit_L);     //option2
        //MC321_IRC_INST(0X0bc5);     //movai   0x07
        MC321_IRC_INST(0x15fd);
        MC321_IRC_INST(0X0b00+opbit_H);
        //MC321_IRC_INST(0X0b03);     //movai   0x07
        MC321_IRC_INST(0x15fe);

        if (FMReadOne(0x012120+7)==1)
        {
            opbit_H =Rxdata;
        }
        else
        {
            return 0;
        }
        MC321_IRC_INST(0x0b00+opbit_H);
        //MC321_IRC_INST(0X0b06);     //movai   0x07
        MC321_IRC_INST(0x15c1);   //movra   0x41 FOSC/64

        MC321_IRC_INST(0x0b01); //movai 0x01
        MC321_IRC_INST(0x15f8); //movra 0x78 //HIRC OUTPUT FROM PSDO

    }



    MC321_IRC_INST(0x0000);
    MC321_IRC_INST(0x0000) ;

    Delay_1ms(10);
    OPTION_FRT=0; //initial 
    for(i=0;i<8;i++)
    {
        //mm=0xff;
        //M201FTSendData(IRC_Option,IRC_Addr,mm);
        MC321FTtest(mm);
        if (IRC_VALUE == IRC_FreqType)
        {
            break;
        }
        if (IRC_VALUE<IRC_FreqType)
        {
            mm |=0x80 >>i;
        } 
        else
        {
            mm &=~(0x80>>i);
        }
        mm |=0x40>>i;
    }

    MC321FTtest(mm);

    if (DeviceConfig_xx.MCU_ID==0x3401)
    {
        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }

        //---------tempadj+/-1------------------------------------
        if (FMReadOne(0x012120+4)==1) //read option2 tempadj
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        OPTION_FRT2=opbit_L; //save temp value

        if (FMReadOne(0x012120)==1) //read option0 ,flag 16MHZ/8MHZ/4MHZ/2MHZ/1MHZ/455K
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        opbit_L =opbit_L>>4;
        opbit_L =opbit_L &0x07;
        if (opbit_L==2) //adj+1
        {
            OPTION_FRT2 =OPTION_FRT2+1;
        } 
        else //adj-1;
        {
            OPTION_FRT2 =OPTION_FRT2-1;
        }

        MC321_IRC_INST(0X0400+OPTION_FRT2);     //option3 L
        MC321_IRC_INST(0x00bd);

        MC321_IRC_INST(0x0000);
        MC321_IRC_INST(0x0000) ;

        Delay_1ms(10);
        mm=0x80; 
        for(i=0;i<8;i++)
        {
            //mm=0xff;
            //M201FTSendData(IRC_Option,IRC_Addr,mm);
            MC321FTtest(mm);
            if (IRC_VALUE == IRC_FreqType)
            {
                break;
            }
            if (IRC_VALUE<IRC_FreqType)
            {
                mm |=0x80 >>i;
            } 
            else
            {
                mm &=~(0x80>>i);
            }
            mm |=0x40>>i;
        }


        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }
        //---------tempadj_3------------------------------------
        if (FMReadOne(0x012120+4)==1) //read option2 tempadj
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        OPTION_FRT2=opbit_L; //save temp value

        if (FMReadOne(0x012120)==1) //read option0 ,flag 16MHZ/8MHZ/4MHZ/2MHZ/1MHZ/455K
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        opbit_L =opbit_L>>4;
        opbit_L =opbit_L &0x07;
        
        if(opbit_L==0||opbit_L==1)
        {
          OPTION_FRT2 =OPTION_FRT2+1;
        }
        else if (opbit_L==2)
        {
          OPTION_FRT2 =OPTION_FRT2-1;
        }
        else
        {
          OPTION_FRT2 =OPTION_FRT2-2;
        }

        MC321_IRC_INST(0X0400+OPTION_FRT2);     //option3 L
        MC321_IRC_INST(0x00bd);

        MC321_IRC_INST(0x0000);
        MC321_IRC_INST(0x0000) ;

        Delay_1ms(10);
        mm=0x80; 
        for(i=0;i<8;i++)
        {
            //mm=0xff;
            //M201FTSendData(IRC_Option,IRC_Addr,mm);
            MC321FTtest(mm);
            if (IRC_VALUE == IRC_FreqType)
            {
                break;
            }
            if (IRC_VALUE<IRC_FreqType)
            {
                mm |=0x80 >>i;
            } 
            else
            {
                mm &=~(0x80>>i);
            }
            mm |=0x40>>i;
        }


        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }
        else
        {
            OPTION_FRT=0XFF;
            OPTION_FRT2=0xff;
            return 0;
        }

    }
    else if (DeviceConfig_xx.MCU_ID==0x3081)
    {
        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }

        //---------tempadj+/-1------------------------------------
        if (FMReadOne(0x012120+4)==1) //read option2 tempadj
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        OPTION_FRT2=opbit_L; //save temp value

        if (FMReadOne(0x012120)==1) //read option0 ,flag 16MHZ/8MHZ/4MHZ/2MHZ/1MHZ/455K
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        opbit_L =opbit_L>>4;
        opbit_L =opbit_L &0x07;
        if ((opbit_L==5)||(opbit_L==2)) //adj+1
        {
            OPTION_FRT2 =OPTION_FRT2+1;
        } 
        else //adj-1;
        {
            OPTION_FRT2 =OPTION_FRT2-1;
        }
        
        MC321_IRC_INST(0X0b00+OPTION_FRT2);     //option2
        //MC321_IRC_INST(0X0bc5);     //movai   0x07
        MC321_IRC_INST(0x15fd);

        MC321_IRC_INST(0x0000);
        MC321_IRC_INST(0x0000) ;

        Delay_1ms(10);
        mm=0x80; 
        for(i=0;i<8;i++)
        {
            //mm=0xff;
            //M201FTSendData(IRC_Option,IRC_Addr,mm);
            MC321FTtest(mm);
            if (IRC_VALUE == IRC_FreqType)
            {
                break;
            }
            if (IRC_VALUE<IRC_FreqType)
            {
                mm |=0x80 >>i;
            } 
            else
            {
                mm &=~(0x80>>i);
            }
            mm |=0x40>>i;
        }

       
        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }
        //---------tempadj_3------------------------------------
        if (FMReadOne(0x012120+4)==1) //read option2 tempadj
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        OPTION_FRT2=opbit_L; //save temp value

        if (FMReadOne(0x012120)==1) //read option0 ,flag 16MHZ/8MHZ/4MHZ/2MHZ/1MHZ/455K
        {
            opbit_L =Rxdata;
        }
        else
        {
            return 0;
        }

        opbit_L =opbit_L>>4;
        opbit_L =opbit_L &0x07;
//        if ((opbit_L==5)||(opbit_L==2)) //adj+1
//        {
//            OPTION_FRT2 =OPTION_FRT2+1;
//        } 
//        else //adj-1;
//        {
//            OPTION_FRT2 =OPTION_FRT2-1;
//        }
        if ((opbit_L==0)||(opbit_L==1)||(opbit_L==4)) //adj+1
        {
            OPTION_FRT2 =OPTION_FRT2+1;
        } 
        else if((opbit_L==2)||(opbit_L==5)) //adj-1;
        {
            OPTION_FRT2 =OPTION_FRT2-1;
        }        
        else
        {
          OPTION_FRT2 =OPTION_FRT2-2;
        }
        MC321_IRC_INST(0X0b00+OPTION_FRT2);     //option2
        //MC321_IRC_INST(0X0bc5);     //movai   0x07
        MC321_IRC_INST(0x15fd);

        MC321_IRC_INST(0x0000);
        MC321_IRC_INST(0x0000) ;

        Delay_1ms(10);
        mm=0x80; 
        for(i=0;i<8;i++)
        {
            //mm=0xff;
            //M201FTSendData(IRC_Option,IRC_Addr,mm);
            MC321FTtest(mm);
            if (IRC_VALUE == IRC_FreqType)
            {
                break;
            }
            if (IRC_VALUE<IRC_FreqType)
            {
                mm |=0x80 >>i;
            } 
            else
            {
                mm &=~(0x80>>i);
            }
            mm |=0x40>>i;
        }

       
        MC321FTtest(mm+1);
        temp0_FT=IRC_VALUE;

        MC321FTtest(mm);
        temp1_FT=IRC_VALUE;

        if((temp0_FT-IRC_FreqType)<(IRC_FreqType-temp1_FT))
        {
            mm=mm+1;
            IRC_VALUE=temp0_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OPTION_FRT=mm;
            //OPTION_FRT2--return the temp adj
            return 1;
        }
        else
        {
            OPTION_FRT=0XFF;
            OPTION_FRT2=0xff;
            return 0;
        }

    }

    OPTION_FRT=0XFF;
    OPTION_FRT2=0xff;
    return 0;


}


void MC321_IRC_INST(u16 OTPData)
{
  u8 i,temp;
  u16 a; 

  for(i=0;i<16;i++)
  {
    a =OTPData>>i;
    if ((a & 0x0001)==1)
    {
            OTP_SDI_1;
    } 
    else
    {
            OTP_SDI_0;
    }
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    NOP;
  }

  //2 CLK Q1,Q2
  OTP_SCK_1;
  NOP;
  OTP_SCK_0; //Q1
  NOP;
  OTP_SCK_1;
  NOP;
  OTP_SCK_0;//Q2
  NOP;
}

