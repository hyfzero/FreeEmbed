
#include "stm32f10x.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "IRC.h"
#include "Config.h"
#include "ADC.h"
#include "LCD.h"





void OTPInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	

	//initial PD11 -> OTP_SCK,PC12->SDI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD,&GPIO_InitStructure);
        
        OTP_SCK_0;
        OTP_SDI_0; 

	//U5(74HC125D) OE1,2,3,4
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7 | GPIO_Pin_8 |GPIO_Pin_9 |GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	//initial PE4 -> OTP_SDO ,PE0->MTP_Busy
	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_4|GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(GPIOE,&GPIO_InitStructure);

	//MTP_DATA
	SDIO_Turnto_Input();

	//Initial CP, TIME_REF,FT
	OTP_SCK_0;
	OTP_SDI_0;
	OTP_DISA_0;
	OTP_DISB_0;
	OTP_DISC_0;
	OTP_DISD_0;
    
    // OTP_DISA_1;
    // OTP_DISB_1;
    // OTP_DISC_1;
    // OTP_DISD_1;

}

u8 OTP_Model_s(u16 OTPAddr)
{
	u8 i=12;
	u8 a=0;
	u8 redata=0;
	u16 temp=0;

	OTP_SDI_1;

	OTP_SCK_0;	//RW
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=13;i>0;i--)
	{
		OTP_SCK_0;  //A12

		temp =OTPAddr>>(i-1);
		if ((temp & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_1;
		NOP;
	}
	//NOP;
	OTP_SDI_0;

	OTP_SCK_0 ; //ACK1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//ACK2
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//SP
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=8;i>0;i--)
	{
		OTP_SCK_0;		
		a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
		redata |= a<<(i-1);
		NOP;
		OTP_SCK_1;//D7
		NOP;
	}
	//OTP_SCK_0; //D0
	//OTP_SCK_1;
	//NOP;
	OTP_SCK_0 ; //WT1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//PRG
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//WT2
	NOP;
	OTP_SCK_1;
	NOP;
	return redata;
}

u8 OTP_ReadByte(u16 OTPAddr)
{
	u8 i=12;
	u8 a=0;
	u8 redata=0;
	u16 temp=0;

	OTP_SDI_1;

	OTP_SCK_0;	//RW
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=13;i>0;i--)
	{
		OTP_SCK_0;  //A12
		
		temp =OTPAddr>>(i-1);
		if ((temp & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_1;
		NOP;
	}
	//NOP;
	OTP_SDI_0;

	OTP_SCK_0 ; //ACK1
	NOP;
	OTP_SCK_1;
	temp =GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
	if(temp==1)
	{
		return 0x00;
	}
	NOP;
	OTP_SCK_0;	//ACK2
	NOP;
	OTP_SCK_1;
	temp =GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
	if(temp==0)
	{
		return 0x00;
	}
	NOP;
	OTP_SCK_0;	//SP
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=8;i>0;i--)
	{
		OTP_SCK_0;		
		a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
		redata |= a<<(i-1);
		NOP;
		OTP_SCK_1;//D7
		NOP;
	}
	//OTP_SCK_0; //D0
	//OTP_SCK_1;
	//NOP;
	OTP_SCK_0 ; //WT1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//PRG
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//WT2
	NOP;
	OTP_SCK_1;
	NOP;
	return redata;
}


//--------------------------------------
//Ð´Ò»¸ö×Ö½Ú
//
//--------------------------------------
void OTP_WriteByte(u16 OTPAddr,u8 OTPData)
{
	u8 i;
	u16 cnt=0;
	u16 temp=0;
	OTP_SDI_1;
	NOP;
	OTP_SDI_0; 
	OTP_SCK_0; //RW
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=13;i>0;i--)
	{
		OTP_SCK_0;  //A12
		
        temp =OTPAddr>>(i-1);
		if ((temp & 0x0001)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_1;
		NOP;
	}
	OTP_SCK_0 ; //ACK1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//ACK2
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//SP
	NOP;
	OTP_SCK_1;
	NOP;
	for(i=8;i>0;i--)
	{
		OTP_SCK_0;  //D0
		temp = OTPData >>(i-1);
		if ((temp & 0x01)==1)
		{
			OTP_SDI_1;
		} 
		else
		{
			OTP_SDI_0;
		}
		NOP;
		OTP_SCK_1;//D7
		NOP;
	}
	//OTP_SCK_0; //D0
	//OTP_SCK_1;
	//OTP_SDI_1;
	OTP_SCK_0 ; //WT1
	NOP;
	OTP_SCK_1;
	NOP;
	OTP_SCK_0;	//PRG
	cnt=0;
	while(cnt<DeviceConfig_xx.ProTime) cnt++;
	OTP_SCK_1;
	cnt=0;
	while(cnt<DeviceConfig_xx.ProTime) cnt++;
	OTP_SCK_0;	//WT2
	NOP;
	OTP_SCK_1;
	NOP;
}


u8 OTP_Mode_IN(u16 OTPID,u8 vpp,u8 vdd)
{
	//u16 i;
	u8 a;
	POWER_ON(vpp,vdd);
	//Delay_1ms(3);//20
	displaWrite_Check();
	//OTP_ReadByte(OTPID); // Write OTP ID
	
	OTP_Model_s(OTPID);

	//test model in true false
	a=OTP_ReadByte(0x0003);
	if (a==0x00)
	{
		//ModelIn_result=0;
		return 0;
	}
    //ModelIn_result=1;    
	return 1;

}


//Verify
u8 OTP_Verify( )
{
	u8 VerifyData;
	u8 ReadData;
	//u32 FMAddr=0x010000;
	//u32 FMOptionAddr =0x012000;
	u16 i;
	u8 Wcounter=0;


	//power on
	if(OTP_Mode_IN(DeviceConfig_xx.MCU_ID,vpp12,vdd65)==0)	
	{
		return 0;
	}
	if (TestAdc()==0)
	{
		return 0;
	}


    for (Wcounter=0;Wcounter<3;Wcounter++)
    {
   
		// Option value verify
       if(DeviceConfig_xx.WriteEn & 0x0f)
       {
			ReadData=OTP_ReadByte(DeviceConfig_xx.OptionAddr);

			if (FMReadOne(Addr_Flash_Option)==1)
			{
				VerifyData=Rxdata;
			}
			else
			{ 
				return 0;
			}
			if (ReadData != VerifyData)
			{
				return 0;
			} 
         }
           if(DeviceConfig_xx.WriteEn & 0xf0)
           {
			// Select flash
			FM_CS_0;
			FMReadMore(Addr_Flash_ROMStart+DeviceConfig_xx.RomFirAddr-1);
			VerifyData=Rxdata;
			//ROM value verify	
			for(i=DeviceConfig_xx.RomFirAddr;i<=DeviceConfig_xx.RomEndAddr;i++)
			{
				ReadData = OTP_ReadByte(i);

				//read flash data
				SPI_ReadByte();
				VerifyData=Rxdata;

				// compare rom & flash data
				if (ReadData != VerifyData)
				{
					FM_CS_1;
					OTP_ADDR=i;
					return 0;
				}

			}
			FM_CS_1;
           }

	 } //3 times verify
     if(DeviceConfig_xx.WriteEn & 0xf0)
     {
	    if (FMReadOne(Addr_Flash_OptionProtec)==1)
	    {
		VerifyData =Rxdata;
	    }
	    else
	    {
		  return 0;
	    }
         Wcounter=0;
	    while(++Wcounter<5)
	   {		
		 OTP_WriteByte(DeviceConfig_xx.OptionAddr,VerifyData);
		 if (OTP_ReadByte(DeviceConfig_xx.OptionAddr) ==VerifyData)
		 {
			Wcounter=0;
			break;
		  }
		  else if (Wcounter==4)
		  {
			//POWER_OFF(vpp00,vdd00);
			OTP_ADDR=DeviceConfig_xx.OptionAddr;
			return 0;
		  }
	    }
       }
	POWER_OFF(vpp00,vdd00);
	//Delay_1ms(200);
	return 1;

}

//OTP BLANK
u8 OTP_Blank(u16 ROMFirstAddr,u16 ROMEndAddr,u16 OPTIONFirstAddr)
{

	//u32 FMAddr=0x010000;
	//u32 FMOptionAddr =0x012000;
	//u16 i;
	
	//MODEL IN 
	OTP_Mode_IN(DeviceConfig_xx.MCU_ID,vpp12,vdd65);

	OTP_ADDR=OPTIONFirstAddr;
	// Option value verify
	OTP_ReadData=OTP_ReadByte(OTP_ADDR);

	if (OTP_ReadData !=DeviceConfig_xx.BlankCode)
	{
		return 0;
	}
	//ROM value verify	
	for(OTP_ADDR=ROMFirstAddr;OTP_ADDR<=ROMEndAddr;OTP_ADDR++)
	{
		OTP_ReadData = OTP_ReadByte(OTP_ADDR);

		// compare rom & Blank data
		if (OTP_ReadData != DeviceConfig_xx.BlankCode)
		{
			return 0;
		}

	}

	//power off
	POWER_OFF(vpp00,vdd00);
	Delay_1ms(50);
	return 1;

}

//------------------------------------------
u8 OTP_DownLoad(u16 MCU_ID,u16 MCU_FirAdd)
{
	u8 ResultFlag;
	u8 ReadData;
	u32 i;

	if (FMReadID() !=1)
	{
		return 0;
	}
	FMWriteStatus(BP_MemoryArray0); //clear protect of all
	FMChipErase();

	//otp 
	if (OTP_Mode_IN(MCU_ID,vpp12,vdd65)==0)
	{
		return 0;
	}

    //Read Option value
	ReadData = OTP_ReadByte(DeviceConfig_xx.OptionAddr);
	
	//Write it to Flash 
    FMWriteOne(Addr_Flash_Option,ReadData);

	//Read ROM value
	//for(i=Addr_Flash_ROMStart;i<Addr_OTP_ROMEnd;i++)
        for(i=MCU_FirAdd;i<=DeviceConfig_xx.RomEndAddr;i++)
	{
		ReadData =OTP_ReadByte(i);
		FMWriteOne(Addr_Flash_ROMStart+i,ReadData);
	}

	//Verify data
	ResultFlag = OTP_Verify();
	//power off
	POWER_OFF(vpp00,vdd00);
	Delay_1ms(50);

    return ResultFlag;
}
