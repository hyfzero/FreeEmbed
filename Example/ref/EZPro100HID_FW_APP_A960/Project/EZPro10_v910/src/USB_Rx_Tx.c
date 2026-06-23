/*
 * @Copyright: Shanghai Sinomcu Microelectronics Co.,Ltd.
 * @Author: Mike.Mo
 * @Emain: mgf@sinomcu.com
 * @Date: 2019-06-25 11:07:18
 * @Encoding: GB2312
 * @Description: 
 */

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

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#include "USB_Rx_Tx.h"
#include "LCD.h"
#include "wx_i2c.h"
#include "MC301Pro.h"
#include "MC32P5232Pro.h"
#include "MC32P21Pro.h"
#include    "mc30f6910.h"
#include    "MTPRW.h"

u8 const ProgrammingFinished[5]={0x68,0x01,0x02,0x03,0x16};
u8 const BlankOk[5]={0x68,0x01,0x04,0x05,0x16};
u8 const NextData[5]={0x68,0x01,0x12,0x13,0x16};
u8 const NextData_E[5]={0x68,0x01,0x14,0x15,0x16};
u8 const Reset[5]={0x68,0x01,0xe2,0xe3,0x16};
//u8 const MotherChipMade[5]={0x68,0x01,0x06,0x07,0x16};
u8 const Reconfigured[5]={0x68,0x01,0x22,0x23,0x16};
u8 const ReNewConfigured[5]={0x68,0x01,0x0b,0x0c,0x16};
u8 const EraseFlash[5]={0x68,0x01,0x08,0x09,0x16};
u8 const ModelIn[5]={0x68,0x01,0x36,0x37,0x16};

//u8 const ReadVison[7]={0x68,0x03,0x46,0x01,0x01,0x4b,0x16};

u8 const ErrorsCS[6]={0x68,0x02,0xf2,0x05,0xf9,0x16};

void USB_Rx_Tx()
{
  u32 i,temp;
  u16 temp_data,tempcount,addr;
  u16 Read_otp_addr;
  u8 checksum=0;
  u8 reFlag=0;
  u16 Read_otp_data=0;
  

  //u32 
  //checksum
  for(i=1;i<USB_Rx_Buffer[1]+2;i++)
  {
    checksum +=USB_Rx_Buffer[i];
  }
  if (checksum != USB_Rx_Buffer[i])
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    //error
    for(temp=0;temp<6;temp++)
    {
      USB_Tx_Buffer[temp]=ErrorsCS[temp];
    }
    USB_SendData(64);
    LEDNG_On;
    LEDBUSY_Off;
    LEDOK_Off;
    Freq(0);
  }
  else if (USB_Rx_Buffer[2]==0x01)
  {
    //programming
    StateFlag =0xf7; //Writer otp
    USB_RxCommand=0x01;
    //for(temp=0;temp<5;temp++)
    //{
    //	USB_Tx_Buffer[temp]=ProgrammingFinished[temp];
    //}
    //USB_SendData(5);
  }
  else if (USB_Rx_Buffer[2]==0x45)//Read vision
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;

    
    USB_Tx_Buffer[0]=0x68;
    USB_Tx_Buffer[1]=0x06;
    USB_Tx_Buffer[2]=0x46;
    USB_Tx_Buffer[3]=73;//Vision: 
    USB_Tx_Buffer[4]=61;
    USB_Tx_Buffer[5]=26;//Date: 
    USB_Tx_Buffer[6]=6;
    USB_Tx_Buffer[7]=99; 
    USB_Tx_Buffer[8]=0x4b + USB_Tx_Buffer[3] +USB_Tx_Buffer[4]+USB_Tx_Buffer[5]+USB_Tx_Buffer[6]+USB_Tx_Buffer[7];
    USB_Tx_Buffer[9]=0x16;
    USB_SendData(64);

  }
  else if (USB_Rx_Buffer[2]==0x03) ////blank
  {
      reFlag=OTP_Blank(DeviceConfig_xx.RomFirAddr,DeviceConfig_xx.RomEndAddr,DeviceConfig_xx.OptionAddr);	
    LEDBUSY_Off;

    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    if(reFlag ==1)
    {
      for(temp=0;temp<5;temp++)
      {
        USB_Tx_Buffer[temp]=BlankOk[temp];
      }
      USB_SendData(64);
      LEDNG_Off;
      //LEDBUSY_Off;
      LEDOK_On;
      Freq(1);
    }
    else
    {
      USB_Tx_Buffer[0]=0x68;
      USB_Tx_Buffer[1]=0x06;
      USB_Tx_Buffer[2]=0xf2;
      USB_Tx_Buffer[3]=0x03;
      USB_Tx_Buffer[4]=OTP_ADDR/0x100;
      USB_Tx_Buffer[5]=OTP_ADDR%0x100;
      USB_Tx_Buffer[6]=0xaa; //Rxdata;  //Flash data
      USB_Tx_Buffer[7]=OTP_ReadData;
      USB_Tx_Buffer[8]=0xfb + USB_Tx_Buffer[4] +USB_Tx_Buffer[5]+USB_Tx_Buffer[6]+USB_Tx_Buffer[7];
      USB_Tx_Buffer[9]=0x16;
      USB_SendData(64);
      LEDNG_On;
      //LEDBUSY_Off;
      LEDOK_Off;
      Freq(0);
    }
  }
  else if (USB_Rx_Buffer[2]==0x04) //write data to eeprom 24c02
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    /*IIC_Write(0x01,0x55);
    OKorNG=IIC_Read(0x01);
    IIC_Write_Array(testdata,0x0001,5);    
    IIC_Read_Array(testdata,0x0001,5);*/
    //temp=USB_Rx_Buffer[4]; //eeprom addr 
    for (i=0;i<32;i++)
    {
      IIC_Write(i,USB_Rx_Buffer[5+i]);
    }
    //IIC_Write_Array(iic_data,0x0000,IIC_DATA_BUFF_SIZE);

    //verify
    USB_Tx_Buffer[2]=0x5a;
    for(temp=0;temp<32;temp++)
    {
      i=IIC_Read(temp);
      if (i!=USB_Rx_Buffer[temp+5])
      {
        USB_Tx_Buffer[2]=0x0a;
        break;
      }
    }
    USB_SendData(64);
  } 
  else if (USB_Rx_Buffer[2]==0x05) //make mother chip
  { 
    LoadConfigToRAM();
    LoadCodeToRAM();
    LoadOptionToRAM();      
    //USB_RxDataCS = CalculateCheckSum(); 
    USB_RxDataCS= CalculateCRC(0xff);
    OptionCS    = CalculateOptionCRC(0xff);
    //OptionCS  =CalculateOptionCheckSum_7343();
    
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    USB_Tx_Buffer[0]=0x68;
    USB_Tx_Buffer[1]=0x07;
    USB_Tx_Buffer[2]=0x06;
    USB_Tx_Buffer[3]=USB_RxDataCS/0x100; //check_sum
    USB_Tx_Buffer[4]=USB_RxDataCS%0x100;
    USB_Tx_Buffer[5]= OptionCS/0x100;
    USB_Tx_Buffer[6]= OptionCS%0x100;
    USB_Tx_Buffer[7]=00;
    USB_Tx_Buffer[8]=00;    
    USB_Tx_Buffer[9]=0x0a+USB_Tx_Buffer[3]+USB_Tx_Buffer[4]+USB_Tx_Buffer[5]+USB_Tx_Buffer[6]+USB_Tx_Buffer[7]+USB_Tx_Buffer[8];
    USB_Tx_Buffer[10]=0x16;
    USB_SendData(64);
    //DeviceConfig();   
    
    //reConfig vpp,vdd value
    if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
    {
        MCP42050_ADJ(ADJ_VPP,25);
        MCP42050_ADJ(ADJ_VDD,25);
    }
    else
    {
        MCP42050_ADJ(ADJ_VPP,MCU_Config_xx.ModeIn_CONFIG_xx.ProVPP);//vpp value
        MCP42050_ADJ(ADJ_VDD,MCU_Config_xx.ModeIn_CONFIG_xx.ProVDD); 
    }
    
    for(temp=0;temp<IIC_DATA_BUFF_SIZE;temp++) // dynamic id
    {
      iic_data[temp]=IIC_Read(temp);
    }
    Initial_Lcd();
    displayfuntion();//LCD 
    LEDNG_Off;
    LEDBUSY_Off;
    LEDOK_On;   
    Freq(1);
  }
  else if (USB_Rx_Buffer[2]==0x06) //read eeprom data  24c02
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    for(temp=0;temp<32;temp++)
    {
      USB_Tx_Buffer[temp]=IIC_Read(temp);
    }
    USB_SendData(64);
  }
  else if (USB_Rx_Buffer[2]==0x07) //erase flash
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    FMWriteStatus(BP_MemoryArray0); //clear protect of all
    FMChipErase();
    for(temp=0;temp<5;temp++)
    {
       USB_Tx_Buffer[temp]=EraseFlash[temp];
    }
    USB_SendData(64);         
  }
  else if (USB_Rx_Buffer[2]==0x08) //Update Firmware
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
     FLASH_ProgramWord(0x0801FC00,0x12345678); //4byte one time   
     USB_Tx_Buffer[0]=0x68;
     USB_Tx_Buffer[1]=0x08;
     USB_Tx_Buffer[2]=0xaa;     
     USB_SendData(64);
     //��λ֮ǰʹ��USB����
     USB_Rx_Flag=0x00;
     //#ifndef STM32F10X_CL
     /* Enable the receive of data on EP3 */
     SetEPRxValid(ENDP1);
     //#endif /* STM32F10X_CL */     
     Delay_1ms(300);
     *((u32 *)0xE000ED0C) = 0x05fa0004; //reset system       
  }    
  else if (USB_Rx_Buffer[2]==0x11) //download data 
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    temp=USB_Rx_Buffer[3]*256+USB_Rx_Buffer[4];
    Read_otp_addr=temp;
    for( i=0; i<(USB_Rx_Buffer[1]-3) ; i++)
    {
      //ReadData =OTP_ReadByte(i);
      FMWriteOne(Addr_Flash_ROMStart+temp+i,USB_Rx_Buffer[i+5]);
    }
    for(temp=0;temp<5;temp++)
    {
      USB_Tx_Buffer[temp]=NextData[temp];
    }
    USB_SendData(64);

  }	
  else if (USB_Rx_Buffer[2]==0x21)  //MUC_config_xx
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    WR_Command = USB_Rx_Buffer[5];
    addr = USB_Rx_Buffer[3]*256 + USB_Rx_Buffer[4];
    for(i=0;i<57;i++)
    {
        USB_Config_Buffer[addr-0XF000+i] = USB_Rx_Buffer[5+i];
    }      
    // if(addr == 0xF039)
    // {
    //     i = 1;
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin           = temp_data*256+USB_Config_Buffer[i++];
        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType          = temp_data*256+USB_Config_Buffer[i++];
        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax           = temp_data*256+USB_Config_Buffer[i++];
        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.WriteConfig_xx.RomFirAddr         = temp_data*256+USB_Config_Buffer[i++];
        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.WriteConfig_xx.RomEndAddr         = temp_data*256+USB_Config_Buffer[i++];
        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.WriteConfig_xx.SoftCheckSum_addr      = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.WriteConfig_xx.HardCheckSum_addr      = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.BadDotConfig_xx.BadDotRomAddrShift    = temp_data*256+USB_Config_Buffer[i++];        
    //     MCU_Config_xx.MTP_CONFIG_xx.EnableWrite    = USB_Config_Buffer[i++];  
        
    //     MCU_Config_xx.WriteConfig_xx.OptionSize         = USB_Config_Buffer[i++];
        
    //     for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
    //     {
    //         temp_data = USB_Config_Buffer[i++];
    //         MCU_Config_xx.WriteConfig_xx.OptionAddr[tempcount]     = temp_data*256+USB_Config_Buffer[i++];  
    //     }       
    //     MCU_Config_xx.IRC_CONFIG_xx.TadjNum = USB_Config_Buffer[i++];
    //     for(tempcount=0;tempcount<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;tempcount++)
    //     {
    //         temp_data = USB_Config_Buffer[i++];
    //         MCU_Config_xx.IRC_CONFIG_xx.TadjValue[tempcount] = temp*256+USB_Config_Buffer[i++]; 
    //     }         
        
    //     //进模式相关配�??
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID           = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr      = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version      = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version_addr = temp_data*256+USB_Config_Buffer[i++];        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H       = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr       = temp_data*256+USB_Config_Buffer[i++];        
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L  = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L_addr  = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version  = temp_data*256+USB_Config_Buffer[i++];   
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr  = temp_data*256+USB_Config_Buffer[i++];
    //     //temp_data = USB_Config_Buffer[i++];
    //     //MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr_ValidBit  = temp_data*256+USB_Config_Buffer[i++];
        
    //     MCU_Config_xx.ModeIn_CONFIG_xx.WriteType        = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.VddWrite         = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.ProVDD           = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.ProVPP           = USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Tdly             = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.Twait            = temp_data*256+USB_Config_Buffer[i++];
    //     temp_data = USB_Config_Buffer[i++];
    //     MCU_Config_xx.ModeIn_CONFIG_xx.UnlockCode       = temp_data*256+USB_Config_Buffer[i++];
        
    //     //reConfig vpp,vdd value ---- 20t01 or otp or flash
    //     MCP42050_ADJ(ADJ_VPP,MCU_Config_xx.ModeIn_CONFIG_xx.ProVPP);//vpp value
    //     MCP42050_ADJ(ADJ_VDD,MCU_Config_xx.ModeIn_CONFIG_xx.ProVDD);         
    // }
        
    
    for(temp=0;temp<5;temp++)
    {
      USB_Tx_Buffer[temp]=Reconfigured[temp];
    }
    USB_SendData(64);
  }
  else if (USB_Rx_Buffer[2]==0x37) //model in
  {   
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    OTP_DISA_0;
    OTP_DISB_0;
    OTP_DISC_0;
    OTP_DISD_0;  
    if( MTP_MODE_IN())
    {
        reFlag=1;      
    }  
    else
    {
        reFlag=0;
    }
//===================== check product id =======================================
    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr,RW_TYPE_ADDR);            
    Read_otp_data=MTP_Read_Word(RW_TYPE_DATA);    //read product id
            
    if(((Read_otp_data & 0x00ff)==MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H) || ((Read_otp_data & 0x00ff)==MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L))
    {
    }
    else
    {
      reFlag=0;
    }

    MTP_Write_Word(MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr,RW_TYPE_ADDR);           
    Read_otp_data=MTP_Read_Word(RW_TYPE_DATA);    //read product id
    
    if((Read_otp_data & 0x0f00)!=MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version)
    {
      reFlag=0;
    }  
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);    //�˳�progģʽ    
    
    //check product id
//    Read_otp_data=MTP_Program_ReadByte(MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr);
//    if(Read_otp_data!=MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H)
//    {
//      reFlag=0;
//    }
//
//    Read_otp_data=MTP_Program_ReadByte(MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L_addr);
    // if(Read_otp_data!=MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L)
    // {
    //     reFlag=0;
    // }

    if(reFlag==0)	
    {
      for(temp=0;temp<5;temp++)
      {
        USB_Tx_Buffer[temp]=Reset[temp];
      }
      ModelIn_result=0;
    }
    else
    {
      for(temp=0;temp<5;temp++)
      {
        USB_Tx_Buffer[temp]=ModelIn[temp];
      }
      ModelIn_result=1;
    }
    USB_SendData(64);
  }
  else if (USB_Rx_Buffer[2]==0x38)  //read OTP; 
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    Read_otp_addr=USB_Rx_Buffer[3]*256 +USB_Rx_Buffer[4];
    // if(Read_otp_addr==0xc000)
    // {
    //   USB_Rx_Buffer[4]=Read_otp_addr;
    // }		
    checksum=0x39+33;
    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    //set the first addr of option
    MTP_Write_Word(Read_otp_addr,RW_TYPE_ADDR);
    for(temp=0;temp<32;temp++)
    {
        Read_otp_data=MTP_Read_Word(RW_TYPE_DATA); //addr++ auto complete
        
        USB_Tx_Buffer[3+temp]=Read_otp_data; //L data
        checksum = checksum +USB_Tx_Buffer[3+temp];
          temp=temp+1;
          USB_Tx_Buffer[3+temp]=Read_otp_data>>8;		
          checksum = checksum +USB_Tx_Buffer[3+temp];
    }

    //exit program mode
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);

    USB_Tx_Buffer[0]=0x68;
    USB_Tx_Buffer[1]=33;
    USB_Tx_Buffer[2]=0x39;
    USB_Tx_Buffer[35]=checksum;
    USB_Tx_Buffer[36]=0x16;
    USB_SendData(64);
  }
  else if(USB_Rx_Buffer[2]==0x48) //read option data
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
      Read_otp_addr=USB_Rx_Buffer[3]*256 +USB_Rx_Buffer[4];
    // if(Read_otp_addr==0xc000)
    // {
    //   USB_Rx_Buffer[4]=Read_otp_addr;
    // }		
    checksum=0x49+33;
    //get in prog mode
    MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
    //set the first addr of option
    MTP_Write_Word(Read_otp_addr,RW_TYPE_ADDR);
   for(temp=0;temp<32;temp++)
   {
       Read_otp_data=MTP_Read_Word(RW_TYPE_DATA); //addr++ auto complete
       
       USB_Tx_Buffer[3+temp]=Read_otp_data; //L data
       checksum = checksum +USB_Tx_Buffer[3+temp];
 
      temp=temp+1;
      USB_Tx_Buffer[3+temp]=Read_otp_data>>8;		
      checksum = checksum +USB_Tx_Buffer[3+temp];
       
   }

    //exit program mode
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);

    USB_Tx_Buffer[0]=0x68;
    USB_Tx_Buffer[1]=33;
    USB_Tx_Buffer[2]=0x49;
    USB_Tx_Buffer[35]=checksum;
    USB_Tx_Buffer[36]=0x16;
    USB_SendData(64);  
  }
  else if (USB_Rx_Buffer[2]==0x40) //PowerOff
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    //�˳�waitģʽ
    MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);      //��ȡ�������˳�waitģʽ    
    
    POWER_OFF(vpp00,vdd00);
    CLN_VDD2GND_On;  //power on ʱ��
    
    for(temp=0;temp<5;temp++)
    {
      USB_Tx_Buffer[temp]=Reset[temp];
    }
    USB_SendData(64);
    LEDNG_Off;
    LEDBUSY_Off;
    LEDOK_On;
    if (ModelIn_result==1)
    {
      Freq(1);
    }
    else
    {
      Freq(0);
    }
    
    //��д������ SCK SDI��д������
    OTP_DISA_1;
    OTP_DISB_1;
    OTP_DISC_1;
    OTP_DISD_1;     
    
    LoadConfigToRAM();    
  }
  else if(USB_Rx_Buffer[2]==0x13) //write mcu-chip eeprom data to flash-chip
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    temp=USB_Rx_Buffer[3]*256+USB_Rx_Buffer[4];
    Read_otp_addr=temp;
    for( i=0; i<(USB_Rx_Buffer[1]-3) ; i++)
    {
      //ReadData =OTP_ReadByte(i);
      FMWriteOne(Addr_Flash_MTPStart+temp+i,USB_Rx_Buffer[i+5]);
    }
    for(temp=0;temp<5;temp++)
    {
      USB_Tx_Buffer[temp]=NextData_E[temp];
    }
    USB_SendData(64);
  }
  else if(USB_Rx_Buffer[2]==0x42) //read mcu-chip eeprom
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
      Read_otp_addr=USB_Rx_Buffer[3]*256 +USB_Rx_Buffer[4];
      // if(Read_otp_addr==0xc000)
      // {
      //   USB_Rx_Buffer[4]=Read_otp_addr;
      // }		
      checksum=0x43+33;
      //get in prog mode
      MTP_Write_Word(PROG_MODE,RW_TYPE_CTL);
      //set the first addr of option
      MTP_Write_Word(0xc000+Read_otp_addr,RW_TYPE_ADDR);
      for(temp=0;temp<32;temp++)
      {
        Read_otp_data=MTP_Read_Word(RW_TYPE_DATA); //addr++ auto complete

        USB_Tx_Buffer[3+temp]=Read_otp_data; //L data  just 8bit data
        checksum = checksum +USB_Tx_Buffer[3+temp];
             
         temp=temp+1;
         USB_Tx_Buffer[3+temp]=Read_otp_data>>8;		
         checksum = checksum +USB_Tx_Buffer[3+temp];
        
      }

      //exit program mode
      MTP_Write_Word(PROG_CMD_EXIT,RW_TYPE_CTL);

      USB_Tx_Buffer[0]=0x68;
      USB_Tx_Buffer[1]=33;
      USB_Tx_Buffer[2]=0x43;
      USB_Tx_Buffer[35]=checksum;
      USB_Tx_Buffer[36]=0x16;
      USB_SendData(64);   
  }
  else if (USB_Rx_Buffer[2]==0xe1)
  {
    ////----zqq 2021.5.20----////
    for(temp=0;temp<64;temp++)
      USB_Tx_Buffer[temp]=0x00;
    
    for(temp=0;temp<5;temp++)
    {
      USB_Tx_Buffer[temp]=Reset[temp];
    }
    USB_SendData(64);
  }
  USB_Rx_Flag=0x00;
  //#ifndef STM32F10X_CL
  /* Enable the receive of data on EP3 */
     SetEPRxValid(ENDP1);
  //#endif /* STM32F10X_CL */
}

//-----------------------------------
void USB_SendData(u8 Length)
{
//  u8 index=0;
//  while (Length >= 64)  //һ�����?��64�ֽ�
//  {
//    while(GetEPTxStatus(ENDP1) != EP_TX_NAK);  
//    UserToPMABufferCopy(&USB_Tx_Buffer[index], ENDP1_TXADDR, 64);
//    SetEPTxCount(ENDP1, 64);
//    SetEPTxValid(ENDP1);
//    index  += 64;  //����ƫ�ƺ�ʣ���ֽ���
//    Length -= 64;
//  }
//  //�������?<=64�ֽڵ�����
//  while(GetEPTxStatus(ENDP1) != EP_TX_NAK); 
//  UserToPMABufferCopy(&USB_Tx_Buffer[index], ENDP1_TXADDR, Length);
//  SetEPTxCount(ENDP1, Length);
//  SetEPTxValid(ENDP1);
//  ////clear se_flag
//  //USB_Rx_Flag=0x00;
//  //remain=63;
//  //index=0;
  
	//length=0;
	USB_SIL_Write(EP1_IN, (uint8_t*)USB_Tx_Buffer, USB_DATA_BUF_LENGHT);
	//USB_SIL_Write(EP1_IN, (uint8_t*)USB_Send_Buffer, length);
	
	SetEPTxValid(ENDP1);
	//PrevXferComplete = 0; 
}