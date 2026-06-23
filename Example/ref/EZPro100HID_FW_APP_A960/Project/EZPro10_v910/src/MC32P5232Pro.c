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
#include "MC32P5232Pro.h"
#include "delay.h"
#include "MC32P21Pro.h"
#include "MC30F6910.h"
#include "MTPRW.h"

u16 CountCRC(u8 data1,u8 data2,u16 crc)
{
  u8 i;
  crc ^= (((uint16_t) data1) << 8);

  for (i = 0; i < 8; i++)
  {
        if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_16_POLYNOMIALS;
        else
                crc <<= 1;
  }
  crc ^= (((uint16_t) data2) << 8);

  for (i = 0; i < 8; i++)
  {
        if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_16_POLYNOMIALS;
        else
                crc <<= 1;
  }      
  return crc;
}


u8 MC32P5232_Program()
{
    u8  ReCnt=0,op_addr=0;
    u16  Fdata,ROMReadData,temp;
    u8	FdataL,FdataH;
    u8  ReFlag=0;

    u16   i,j,data[17];
    u16 option_value[OPTION_WordSize_MAX];
    u16 option_addr[OPTION_WordSize_MAX];


           
    
    struct
    {
        u16 addr;

        u16 loop1;         //循环计数器，第一层
        u16 loop2;         //循环计数器，第二层
        u16 loop3;         //循环计数器，第三层
        u16 loop4;         //循环计数器，第四层    
    }Cnt;     

    
    struct
    {
        u16 temp;           //临时存储区          
        u16 result;         //计算结果值
        u16 hardware;       //硬件结果
        u16 flag;           //标识
        u16 OTPsize;

    }Crc;        

      
      
     VDD30V_Off;
     POWER_OFF(vpp00,vdd00);
     return 1;  
}

/*******************************************************************************
* Function Name  :  MC32_MODEL_IN
* Description    : 
* Input          :  mode_in_config: 进模式相关配置
* Output         : None.
* Return         : flagModeInOK:    0-进模式失败
*                                   1-进模式成功
*******************************************************************************/
u8 A500_(void)
{
    u8  temp;
    u8  flagModeInOK=0;
    u16 otpReadBuf;    
    
    POWER_OFF(vpp00,vdd00);
    delay_ms(100);
    OTP_SCK_0;
    OTP_SDI_0; 

    if (MCU_Config_xx.ModeIn_CONFIG_xx.VddWrite == vdd65 )
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
    
    if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
    {
      VDD_IO_On;
    }
    
    delay_us(MCU_Config_xx.ModeIn_CONFIG_xx.Tdly);
    
    MCA330_Bus_Write(MCU_Config_xx.ModeIn_CONFIG_xx.UnlockCode,16,0);    /*解锁*/
    
    delay_us(MCU_Config_xx.ModeIn_CONFIG_xx.Twait);  
    
	if(MCA330_OTP_ReadByte(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr,&otpReadBuf))  /*读取chip id*/
	{
        if (otpReadBuf==MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID)                     /*判断chip id*/
        {
            flagModeInOK =1;    		
        }   
        else 
        {
            return 0;
        }
	}   
    

    
    if(MCA330_OTP_ReadByte(MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr,&otpReadBuf))/*读取 WriteTool_version*/
    {
        otpReadBuf &= 0x00ff;
        if (otpReadBuf==MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version)        /*判断 WriteTool_version*/
        {        
            flagModeInOK =1; 
        }   
        else 
        {
            return 0;
        }
    }       
    
    return flagModeInOK;
}


/*******************************************************************************
* Function Name  : A650_MODE_IN
* Description    : 
* Input          :                        
* Output         : None.
* Return         : None.
*******************************************************************************/
u8 A650_MODE_IN()
{
    u8 temp;
    u8  flagModeInOK=1;
    u16 otpReadBuf;     
    
    //power on & model in
    POWER_OFF(vpp00,vdd00);
    delay_ms(100);
    OTP_SCK_0;
    OTP_SDI_0; 

    //if(MCU_Config_xx.ModeIn_CONFIG_xx.ModeInTiming == ModeInTiming_MC32P21 )
    {
        //input wave
        if (MCU_Config_xx.ModeIn_CONFIG_xx.VddWrite == vdd65 )
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
        if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
        {
            VDD_IO_On;
        }  
        NOP;
        CLN_VDD2GND_Off;
        VPP12V_On;
        if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
        {
            VPP_IO_On;
        }               
        delay_us(MCU_Config_xx.ModeIn_CONFIG_xx.Tdly);        
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
        if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
        {
            VPP_IO_Off;
        }           
        delay_us(500);
        OTP_SDI_1;
        NOP;
        OTP_SDI_0;
        NOP;
        CLN_VDD2GND_Off;
        VPP12V_On;
        if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
        {
            VPP_IO_On;
        }   
        delay_us(500);
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
        if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
        {
            VPP_IO_Off;
        }                   
        delay_us(500);
        OTP_SDI_1;
        NOP;
        OTP_SDI_0;
     }

    CLN_VDD2GND_Off;
    //delay_us(MCU_Config_xx.ModeIn_CONFIG_xx.Twait);    
    delay_us(50000);  
    otpReadBuf = MC321_ReadByte(MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr);
    if (otpReadBuf != MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID)
    {
        ERORR_VALUE=OTP_ModeIn_false;
        flagModeInOK = 0;
        //ERORR_VALUE=OTP_ModeIn_false;// linmei add 
        return 0; //  linmei add 
    }  
    
   

    
    

    
    
    
    return flagModeInOK;
}
u8 MCA330_Bus_Write(u32 Data ,u8 BitSize ,u8 BitCheckEnable)
{
    u8 flagWriteOK  =1;
    u8 bitCnt;
    u32 DataBuf =Data;

    OTP_SCK_0;
    for(bitCnt = 0; bitCnt < BitSize; bitCnt++ )
    {
        if ((DataBuf & 0x00000001)==1)
        {
            OTP_SDI_1;
            OTP_SDI_1;
        } 
        else
        {
            OTP_SDI_0;
            OTP_SDI_0;
        }
        OTP_SCK_1;
        OTP_SCK_1;
        OTP_SCK_1;
        OTP_SCK_1;

        OTP_SCK_0; 
        if(BitCheckEnable)
        {
            if(DataBuf & 0x00000001)/*bit 判断*/
            {
                if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
                {
                }
                else
                {
                    flagWriteOK = 0;
                    break;
                }
            }
            else
            {
                if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
                {
                    flagWriteOK = 0;
                    break;
                }
            }
        }
        DataBuf = DataBuf>>1;
    }

    OTP_SDI_0;
    return flagWriteOK;
}

u8 MCA330_OTP_ReadByte(u16 OTPAddr,u16 *TargetRegister)
{
    u8 readOK=0;	
    u16 OTPAddrBuf=OTPAddr;
    u16 result;

    if(MCA330_Bus_Write(proCmd_writeProgAddr,proCmd_BITSIZE,0))
    {
        if(MCA330_Bus_Write(OTPAddrBuf ,16,0))
        {
            if(MCA330_OTP_R(&result))
            {
                *TargetRegister = result;
                readOK = 1;
            }
        }
    }
    return readOK;
}


/*读取当前地址的OTP数据，并保存到u16目标寄存器中，返回标识*/
u8 MCA330_OTP_R(u16 *TargetRegister)
{
    u8 readOK=0;/*1=OK;0=fail*/
    u16 read_data;
    if(MCA330_Bus_Write(proCmd_readRomData, proCmd_BITSIZE , 0))
    {
        MCA330_Bus_Read(&read_data ,16);
        *TargetRegister = read_data;
        readOK =1; 
    }
    return readOK;
}

u8 MCA330_Bus_Read(u16 *TargetRegister ,u8 BitSize)
{
    u8 bitCnt;
    u16 DataBuf;
    u16 TargetDataBuf=0;
    for(bitCnt = 0; bitCnt < BitSize; bitCnt++ )
    {
        OTP_SCK_1; 
        OTP_SCK_1; 
        OTP_SCK_1; 
        OTP_SCK_1; 
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_0;
        OTP_SCK_0;
        DataBuf=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
        TargetDataBuf |= DataBuf << bitCnt;
    }

    *TargetRegister = TargetDataBuf;   
    return 1;
}

u8  CheckProductID(u16 addr,u16 data)
{
    u16 otpReadBuf ;
    if(MCA330_OTP_ReadByte(addr,&otpReadBuf))     /*读取chip version*/
    {
        otpReadBuf &= 0x00ff;
        if (otpReadBuf==data)                     /*判断chip version*/
        {        
            return 1;
        }   
    }
    return 0;
}

u8  CheckLoadRegisterSuccess(u16 Reg_addr,u8 opt_size)
{
    u8 i;
    u16 otpReadBuf,regReadBuf ;
    
    for(i=0;i<opt_size;i++)
    {
      
        if(MCA330_OTP_ReadByte(MCU_Config_xx.WriteConfig_xx.OptionAddr[i],&otpReadBuf))     
        {
            otpReadBuf &= MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[i];
            if(MCA330_OTP_ReadByte(Reg_addr++,&regReadBuf))     
            {      
                TestModeRegisterValue_H_L[2*i] = regReadBuf >> 8;//记录上电后寄存器的值，IRC校准需要用到
                TestModeRegisterValue_H_L[2*i+1] = regReadBuf;
                regReadBuf &= MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[i];
                if (otpReadBuf==regReadBuf)                     
                {        
                    //return 1;
                }  
                else 
                {
                   return 0;
                }
            }
        }
    }
    return 1;
}
u8  LoadOptionToTestModeRegisterValue_H_L()
{
    u8 i;
    u16 otpReadBuf ;
    
    for(i=0;i<MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
      
        if(MCA330_OTP_ReadByte(MCU_Config_xx.WriteConfig_xx.OptionAddr[i],&otpReadBuf))     
        {
            TestModeRegisterValue_H_L[2*i] = otpReadBuf >> 8;//记录上电后寄存器的值，IRC校准需要用到
            TestModeRegisterValue_H_L[2*i+1] = otpReadBuf;
        }
        else
        {
          return 0;
        }
    }
    return 1;
}
u8 MCA650_SendProgAddr(u16 Addr)
{
  u8 sendOK=0;
  u16 DataBuf = Addr;
  
  if(MCA330_Bus_Write(proCmd_writeProgAddr , proCmd_BITSIZE ,0))
  {
    if(MCA330_Bus_Write(DataBuf , 16 ,0))
    {
      sendOK =1;
    }
  }
  return sendOK;
}

u8 MCA330_SendProgData(u16 Data)
{
  u8 sendOK=0;
  u16 DataBuf = Data;
  
  if(MCA330_Bus_Write(proCmd_writeProgData , proCmd_BITSIZE ,1))
  {
    if(MCA330_Bus_Write(DataBuf , 16 ,1))
    {
      sendOK =1;
    }
  }
  return sendOK;
}
u8 MCA330_ProgAddrAdd(u16 NextData)
{
  u8 flagProgOK=0;
  u16 DataBuf = NextData;

    if(MCA330_Bus_Write(proCmd_progAddrAdd , proCmd_BITSIZE ,1 ))
    {
      if(MCA330_Bus_Program(DataBuf ,100,1))/*间隔100us用于烧录*/ //if(MCA330_Bus_Program(0x0000 ,700,0 ))/*间隔100us用于烧录*/
      
      {
        flagProgOK = 1;
      }
    }
    
    return flagProgOK;
}
u8 MCA650_ProgAndAddrAdd(u16 Wdata)
{
    u8 flagProgOK=0;

    if(MCA330_Bus_Write(proCmd_writeProgData , proCmd_BITSIZE ,0 ))/*送写数据命令*/
    {
        if(MCA330_Bus_Write(Wdata , 16 ,0))/*送待烧写数据*/
        {
            if(MCA330_Bus_Write(proCmd_prog , proCmd_BITSIZE ,0 ))/*送编程命令*/
            {
                if(MCA330_Bus_Program(0xffff ,100,0))/*送编程时钟*/ 
                {
                    if(MCA330_Bus_Write(proCmd_addrAdd , proCmd_BITSIZE ,0 ))/*地址自加*/
                    {
                        flagProgOK = 1;
                    }
                }
            }
        }
    }
    
    return flagProgOK;
}
u8 MCA650_ReadRomDataAndAddrAdd(u16 *TargetRegister)
{
    u8 readOK=0;
    u16 result;
    
    if(MCA330_OTP_R(&result))
    {
        *TargetRegister = result;
        if(MCA330_Bus_Write(proCmd_addrAdd , proCmd_BITSIZE ,0 ))
        {
            readOK = 1;
        }
    }  

    return readOK;
}
u8 MCA330_Bus_Program(u16 Data ,u16 GapTime ,u8 BitCheckEnable )
{
    u8  flagProgOK      = 1;
    u8  bitCnt;
    u16 DataBuf         = Data;

    OTP_SCK_0;
    for(bitCnt=0;bitCnt<8;bitCnt++)     /* 数据传入mcu*/
    {         
        if ((DataBuf & 0x0001)==1)
        {
            OTP_SDI_1;
            OTP_SDI_1;
        } 
        else
        {
            OTP_SDI_0;
            OTP_SDI_0;
        }
        OTP_SCK_1;  
        OTP_SCK_1;
        OTP_SCK_1;
        OTP_SCK_1;

        OTP_SCK_0;

        if(BitCheckEnable)
        {
            if(DataBuf & 0x0001)/*bit 判断*/
            {
                if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
                {}
                else
                {
                    flagProgOK = 0;
                    break;
                }
            }
            else
            {
                if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
                {
                    flagProgOK = 0;
                    break;
                }
            }
        }      

        DataBuf = DataBuf>>1;
    }

    delay_us(GapTime-15);/*100us 时间间隔=bit5~bit8所占总时间*/    
    OTP_SCK_0;
    for(bitCnt=0;bitCnt<8;bitCnt++)     /* 数据传入mcu*/
    {         
        if ((DataBuf & 0x0001)==1)
        {
            OTP_SDI_1;
            OTP_SDI_1;
        } 
        else
        {
            OTP_SDI_0;
            OTP_SDI_0;
        }
        OTP_SCK_1;  
        OTP_SCK_1;
        OTP_SCK_1;
        OTP_SCK_1;

        OTP_SCK_0;

        if(BitCheckEnable)
        {
            if(DataBuf & 0x0001)/*bit 判断*/
            {
            if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
            {}
            else
            {
            flagProgOK = 0;
            break;
            }
            }
            else
            {
            if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
            {
            flagProgOK = 0;
            break;
            }
        }
    }      

    DataBuf = DataBuf>>1;
    }
    OTP_SDI_0;


    return flagProgOK;
  
}
u8 MCA330_ProgByte(u16 Addr,u16 Data)
{
  u8 flagProgOK=0;
  u16 DataBuf = Data;


    if(MCA330_Bus_Write(proCmd_writeProgAddr , proCmd_BITSIZE ,0))/*写地址*/
    {
      if(MCA330_Bus_Write(Addr , 16 ,0))
      {
        if(MCA330_Bus_Write(proCmd_writeProgData , proCmd_BITSIZE ,0 ))/*写待烧写的数据*/
        {
          if(MCA330_Bus_Write(DataBuf , 16 ,0))
          {
            if(MCA330_Bus_Write(proCmd_prog , proCmd_BITSIZE ,0 ))
            {
              if(MCA330_Bus_Program(0xffff ,100,0))/*间隔100us用于烧录*/ //if(MCA330_Bus_Program(0x0000 ,700,0 ))/*间隔100us用于烧录*/
              
              {
                flagProgOK = 1;
              }
            }
          }
        }
      }
    }
    return flagProgOK;
}

/*******************************************************************************
* Function Name  : MCA330 ReadByte_DiffMode.
* Description    : 不同模式读数据（设置读取地址）
* Input          : ModeNum 0：普通模式；1：Margin-1模式；2：Off State Margin模式
*                  OTPAddr：读取地址
* Output         : a读取到的数据
*******************************************************************************/

u8 MCA330_ReadByte_DiffMode(u8 ModeNum,u16 Addr,u16 *TargetRegister)
{
	u8 readOK=0;
        u8 PTMMode;
	u16 AddrBuf=Addr;
        switch(ModeNum)
        {
        case 0:
          PTMMode = 0;
          break;
          
        case 1:
          PTMMode = 6;
          break;
          
        case 2:
          PTMMode = 5;
          break;
          
        default:
          PTMMode = 0;
          break;
        }
        
       if( MCA330_Bus_Write(proCmd_PTM , proCmd_BITSIZE ,0))
       {
          if(MCA330_Bus_Write(PTMMode , 16 ,0))
          {
            
            if(MCA330_OTP_ReadByte(AddrBuf,TargetRegister))
            {
              readOK =1;
            }
            
          }
       }
        

	return readOK;
}
u8 MCA330_ReadRomDataAddrAdd(u16 *TargetRegister)
{
  u8 readOK=0;

  if(MCA330_Bus_Write(proCmd_readRomDataAddrAdd , proCmd_BITSIZE ,0))
  {
    MCA330_Bus_Read(TargetRegister , 16);
    readOK =1;
  }
  return readOK;
}

u8 MC32P5232_ReadMTP_Nomal(u16 address)
{
  u8 a;
  MC32P5232_SetAddress(address);           //1．通过0x15命令设置读取地址 
  MC32P5232_SetPTMmode(ByteRead);          //2. 设PTM模式（指令码：0x1a ） Byte Read       
  MC32P5232_InputData(0x0000);  
  a=MC32P5232_R(0x0000);                   
  return a;
}
/*******************************************************************************
* Function Name  : MC32P5232_MTP_Margin
* Description    : MTP Margin Read
* Input          : address:读取地址；   
* Output         : None
* Return         : 
*******************************************************************************/
u8 MC32P5232_ReadMTP_Margin(u16 address,u16 mode_Margin,u16 mode_MTP)
{
  u8 a;
  MC32P5232_SetAddress(address);          //1．通过0x15命令设置读取地址 
  MC32P5232_InputData(mode_Margin);              //2.	数据输入（指令码：0x16）Program Margin1-3/Erase Margin1-3（见MTP模式切换操作）
  MC32P5232_SetPTMmode(mode_MTP);              //3.	设PTM模式（指令码：0x1a ）Program Margin1-3/Erase Margin1-3（见MTP模式切换操作）      
  a=MC32P5232_R(mode_Margin);
  return a;
}


/*******************************************************************************
* Function Name  : MC32P5232_MTPWritePage
* Description    : MTP Page Write
* Input          : PageStaAddress:页首地址；num:加载第几个数据；wData:加载的数据
* Output         : None
* Return         : 
*******************************************************************************/
u8 MC32P5232_MTPWritePage(u16 StaAddress,u8 num,u8 wData)
{
  if(num==0)                      //加载第一个数据之前设置地址及PTM模式
  {
    MC32P5232_SetAddress(StaAddress);       //1．通过0x15命令设置页首地址 
    MC32P5232_SetPTMmode(PageWrite);        //2. 设PTM模式（指令码：0x1a ） Page Write     
    MC32P5232_InputData(wData);
  }  
  else
  {
    LoadMTPdata(wData);
  }
  if(num==16)
  {
    if(!ProgramTiming(ProgMTP_Tprog))      //编程时间最长5ms    
    {
      return 0;
    }    
  }
  return 1;
}

/*******************************************************************************
* Function Name  : MC32P5232_MTPWriteChip
* Description    : MTP Chip Write
* Input          : num:加载第几个数据；wData:加载的数据
* Output         : None
* Return         : 
*******************************************************************************/

void MC32P5232_MTPWriteChip(u16 StaAddress,u8 num,u8 wData)
{
  u8 i,temp;
  if(num==0)//第一个数据
  {
    MC32P5232_SetAddress(StaAddress);    //1．通过0x15命令设置页首地址 
    MC32P5232_SetPTMmode(ChipWrite);        //2. 设PTM模式（指令码：0x1a ） Page Write       
  }
  MC32P5232_InputData(wData);               //3. 数据输入（指令码：0x16）
  MC32P5232_Instruct(MTPLoad);        //4. MTP加载	
  for(i=0;i<16;i++)
  {
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
    NOP;
  }
  if(num<15)//不是最后一次
  {
    MC301_Instruct(AddrINC);      //5. 地址加一
  }
  else if(num==15)
  {
    MC301_Instruct(MTPwrite);     //6.MTP烧写
    for(i=0;i<8;i++)
    {
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;
      NOP;
    }  
    Delay_1ms(5);
    for(i=0;i<8;i++)
    {
      OTP_SCK_1;
      NOP;
      OTP_SCK_0;
      NOP;
    }    
  }
}

/*******************************************************************************
* Function Name  : MC32P5232_MTPPageErase
* Description    : MTP Page Erase
* Input          : PageStaAddress:页首地址
* Output         : None
* Return         : 
*******************************************************************************/
/*
void MC32P5232_MTPPageErase(u16 StaAddress)
{
  u8 i,temp;
  MC32P5232_SetAddress(StaAddress);    //1．通过0x15命令设置页首地址 
  SetPTMmode(PageErase);         //2. 设PTM模式（指令码：0x1a ） Page Write       
  MC301_Instruct(MTPwrite);      //3. MTP烧写
  for(i=0;i<8;i++)
  {
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
    NOP;
  }  
  Delay_1ms(10);
  for(i=0;i<8;i++)
  {
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
    NOP;
  }
}
*/
/*******************************************************************************
* Function Name  : MC32P5232_MTPChipErase
* Description    : MTP Page Erase
* Input          : PageStaAddress:页首地址
* Output         : None
* Return         : 
*******************************************************************************/
u8 MC32P5232_MTPChipErase(u16 StaAddress)
{
  MC32P5232_SetAddress(StaAddress);    
  MC32P5232_SetPTMmode(ChipErase_A500);               
  //MC32P5232_Instruct(MTPwrite);      
  if(!ProgramTiming(ProgMTP_Tprog))      //编程时间最长5ms    
  {
    return 0;
  }
  return 1;
}

/*******************************************************************************
* Function Name  : LoadMTPdata
* Description    : Load MTP data
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 LoadMTPdata(u16 data)
{
  u8 i,temp;
  u16 a;
  
  MC32P5232_Instruct(MTPLoad);        //4. MTP加载	
  
  OTP_SCK_0;
  a = data;
  if ((a & 0x0001)==1)
  {
    OTP_SDI_1;
  } 
  else
  {
    OTP_SDI_0;
  }   
  OTP_SCK_1;
  NOP;    
  OTP_SCK_0;  
  a =data>>1;
  if ((a & 0x0001)==1)
  {
    OTP_SDI_1;
  } 
  else
  {
    OTP_SDI_0;
  }   
  NOP;  
  NOP; 
  NOP;
  NOP;  
  OTP_SCK_1;
  NOP;
  OTP_SCK_0;
    
  
  for(i=2;i<16;i++)
  {
    a =data>>i;
    if ((a & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    NOP;
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
    //NOP;
  }   
  
  return 1;  
}
/*******************************************************************************
* Function Name  : ProgramtTiming
* Description    : MTP Program timing
* Input          : Tprog MTP编程等待SDO输出低电平时间
* Output         : None
* Return         : None
*******************************************************************************/
u8 ProgramTiming(u32 Tprog)
{ 
  u8 i,temp;
  MC32P5232_Instruct(MTPwrite);
  for(i=0;i<8;i++)
  {
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
    NOP;
  }  
  while(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
  //while(1)//测试用
  {
    NOP;
    Tprog--;
    if(Tprog==0)
    {
      return 0;
      //break;//测试用
    }
  }
  for(i=0;i<8;i++)
  {
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
    NOP;
  }
  return 1;
}

void SendData_Bits(u8 BitNum,u16 Data)
{
  u8 i,temp; 
  u16 a;
  OTP_SCK_0;
  for(i=0;i<BitNum;i++)
  {
    a =Data>>i;
    if ((a & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    NOP;
    OTP_SCK_1;  //1
    NOP;
    OTP_SCK_0;
  }
}

void MC32P5232_Instruct(u16 OTPData)
{
  u8 i,temp,a;

  OTP_SCK_0;
  for(i=0;i<6;i++)
  {
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
    OTP_SCK_1;
    NOP;
    OTP_SCK_0;
  }
}

void MC32P5232_InputData(u16 OTPData)
{
  MC32P5232_Instruct(DataWr);
  SendData_Bits(16,OTPData); 
}

u16 MC32P5232_R(u16 read_mode)
{
	u8 i,temp; //temp use for Nop instruce
	u16 a,read_data;
	//read 
	MC32P5232_Instruct(ReadAndINC);
 
	read_data=0;
  for(i=0;i<16;i++)
  {
    a =read_mode>>i;
    if ((a & 0x0001)==1)
    {
      OTP_SDI_1;
    } 
    else
    {
      OTP_SDI_0;
    }
    NOP;
    OTP_SCK_1;  //1
    NOP;   
    OTP_SCK_0;
    NOP;
		a=GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN);
		read_data |= a<<i;     
  } 

	return read_data;
}

void MC32P5232_SetAddress(u16 OTPAddr)
{
  MC32P5232_Instruct(SetAddr);
  SendData_Bits(16,OTPAddr);   
}    
void MC32P5232_SetPTMmode(u16 mode)
{
  MC32P5232_Instruct(SetReadMode);
  SendData_Bits(16,mode);  
}


u8 MC32P5232IRC_TEST(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{
    u16 i,j=0;
    u8  bit_shift;
    u16 osccal_mid,osccal;
    u16 temp0_FT,temp1_FT,dif0,dif1;
    u8 tadj;

    if(MCA330_Bus_Write(proCmd_functionMode, proCmd_BITSIZE ,0)==0)       /*进入功能测试模式*/
    {
        ERORR_VALUE=IRC_Value_false;
        return 0;
    }

    for(i=0;i<3;i++)
    {
        MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                   /*无效运行指令*/
    }   

    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
        //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
        MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
        //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
        TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
        TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
        if(MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] != MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegInvalidAddr)
        {//有效寄存器地址
            MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],funCmd_BITSIZE,0);      
            MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i],funCmd_BITSIZE,0);               
        }
    }
    
    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,funCmd_BITSIZE,0);      
    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr,funCmd_BITSIZE,0);            


    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                     /*无效运行指令*/
    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0); 
           
    //从上位机取温度校准值
    tadj = 0;
    bit_shift = MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit_num;
    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
        if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i])   
        {              
            for(j=0;j<8;j++)
            {
              if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i]&(0x80>>j))
              {
                if(bit_shift>0)
                {
                    bit_shift--;
                    if(WRITE_DATA_xx.option_byte_H_L[i]&(0x80>>j))
                    {
                        tadj |= (0x01<<bit_shift);
                    }
                    else
                    {
                        tadj &= ~(0x01<<bit_shift);
                    }   
                }
              }
            }  
        }
    }    
    osccal_mid=(MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit+1)/2;
    osccal=osccal_mid;
    
    for(i=0;i<MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num;i++)    
    {
        MC32P5232FT(osccal,tadj);
        if (IRC_VALUE == IRC_FreqType)
        {
            break;
        }
        if (IRC_VALUE<IRC_FreqType)
        {
            osccal |=osccal_mid >>i;
        } 
        else
        {
            osccal &=~(osccal_mid>>i);
        }
        osccal |=(osccal_mid>>1)>>i;
    } 

    MC32P5232FT(osccal+1,tadj);
    temp1_FT=IRC_VALUE;
    ; 
    MC32P5232FT(osccal,tadj);
    temp0_FT=IRC_VALUE;

    if(temp1_FT>IRC_FreqType)
    {
        dif1=temp1_FT-IRC_FreqType;
    }
    else
    {
        dif1=IRC_FreqType-temp1_FT;
    }

    if(temp0_FT>IRC_FreqType)
    {
        dif0=temp0_FT-IRC_FreqType;
    }
    else
    {
        dif0=IRC_FreqType-temp0_FT;
    }    

    if(dif1<dif0)
    {
        osccal=osccal+1;
        IRC_VALUE=temp1_FT;
    }

    if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
    {
        OSCCAL=osccal;
        TADJ=tadj;
        MC32P5232FT(osccal,tadj);//通过调用这个函数，可以处理、记录 频率校准值 和 温度校准值   
        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            return 1;
        }
    }     
    OSCCAL=MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit;
    TADJ=MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit;
    return 0;
}



u8 MC32P5232IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{
    u16 i,j=0,k;
    u16 osccal_mid,osccal;
    u16 temp0_FT,temp1_FT,dif0,dif1;

    if(MCA330_Bus_Write(proCmd_functionMode, proCmd_BITSIZE ,0)==0)       /*进入功能测试模式*/
    {
        ERORR_VALUE=IRC_Value_false;
        return 0;
    }

    for(i=0;i<3;i++)
    {
        MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                   /*无效运行指令*/
    }

    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
        //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
        MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];
        
        //取上位机取界面配置的IRC校准相关的option值 (内部RC振荡器频率选择位FAS(16M/8M/4M/2M/1M/455K);VDSEL
        TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
        TestModeRegisterValue_H_L[i] |= WRITE_DATA_xx.option_byte_H_L[i] & MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[i];
        
        MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],funCmd_BITSIZE,0);      
        MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i],funCmd_BITSIZE,0);               
    }

    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,funCmd_BITSIZE,0);      
    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr,funCmd_BITSIZE,0);            


    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                     /*无效运行指令*/
    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0); 
           
    
    osccal_mid=(MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit+1)/2;   
    
    for(i=0;i<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;i++)
    {   
        osccal=osccal_mid;
        for(j=0;j<MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num;j++)   
        {
            MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[k]);
            if (IRC_VALUE == IRC_FreqType)
            {
                break;
            }
            if (IRC_VALUE<IRC_FreqType)
            {
               // osccal |=osccal_mid >>i;
                osccal |=osccal_mid >>j;//linmei 修改，以前的版本有误，变量是错的 2018.12.18
            } 
            else
            {
               // osccal &=~(osccal_mid>>i);
                osccal &=~(osccal_mid>>j);//linmei 修改，以前的版本有误，变量是错的2018.12.18
            }
            //osccal |=(osccal_mid>>1)>>i;
            osccal |=(osccal_mid>>1)>>j;//linmei 修改，以前的版本有误，变量是错的2018.12.18
        } 

        MC32P5232FT(osccal+1,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[k]);
        temp1_FT=IRC_VALUE;
        ; 
        MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[k]);
        temp0_FT=IRC_VALUE;

        if(temp1_FT>IRC_FreqType)
        {
            dif1=temp1_FT-IRC_FreqType;
        }
        else
        {
            dif1=IRC_FreqType-temp1_FT;
        }

        if(temp0_FT>IRC_FreqType)
        {
            dif0=temp0_FT-IRC_FreqType;
        }
        else
        {
            dif0=IRC_FreqType-temp0_FT;
        }    

        if(dif1<dif0)
        {
            osccal=osccal+1;
            IRC_VALUE=temp1_FT;
        }

        if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
        {
            OSCCAL=osccal;
            TADJ=MCU_Config_xx.IRC_CONFIG_xx.TadjValue[k];
            MC32P5232FT(osccal,MCU_Config_xx.IRC_CONFIG_xx.TadjValue[k]);//通过调用这个函数，可以处理、记录 频率校准值 和 温度校准值   
            if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
            {
                return 1;
            }
        }     
    }
    OSCCAL=MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit;
    TADJ=MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit;
    return 0;
}



u8 MC32P5232IRC_CHECK(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType)
{
    u8 i;
    u16 cnt[3];
    
    if(MCA330_Bus_Write(proCmd_functionMode, proCmd_BITSIZE ,0)==0)       /*进入功能测试模式*/
    {
        ERORR_VALUE=IRC_Value_false;
        return 0;
    }

    for(i=0;i<3;i++)
    {
        MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                   /*无效运行指令*/
    }

    //关闭看门狗
    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
        //处理固定校准条件相关值 关闭看门狗；关闭外部复位；振荡器模式OSCM选择内部高频HIRC振荡器； 
        MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i] &= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] &= ~MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[i];
        TestModeRegisterValue_H_L[i] |= MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[i];        
        
        MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],funCmd_BITSIZE,0);      
        MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i],funCmd_BITSIZE,0);               
    }

    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + MCU_Config_xx.IRC_CONFIG_xx.TMODE_value ,funCmd_BITSIZE,0);      
    MCA330_Bus_Write(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr,funCmd_BITSIZE,0);            


    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0);                     /*无效运行指令*/
    MCA330_Bus_Write(funCmd_activeInstruction + funInstruction_NOP,funCmd_BITSIZE,0); 
           
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
    IRC_VALUE = (u16)((cnt[0]+cnt[1]+cnt[2])/3 );

    if ((IRC_VALUE<=IRC_FreqMax) && (IRC_VALUE>=IRC_FreqMin))
    {
        return 1;
    }     
    return 0;
}


void MC32P5232FT(u16 osccal,u16 tadj)
{
    u16 i,j;
    u16 cnt[3];
    u16 osccal_shift,tadj_shift;
    
    osccal_shift = MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num ;
    tadj_shift = MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit_num;
    
    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
       //处理频率校准值
      if((i==6)||(i==7))        //只将HIRC的初始值填入，屏蔽PFRC的值写入
      {
      }
      else
      {      
        //处理频率校准值
        if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i])
        {         
            for(j=0;j<8;j++)
            {
              if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i]&(0x80>>j))
              {
                  if(osccal_shift>0)
                  {
                      osccal_shift--;
                      if(osccal&(0x01<<osccal_shift))
                      {
                            TestModeRegisterValue_H_L[i] |= (0x80>>j);
                      }      
                      else
                      {
                            TestModeRegisterValue_H_L[i] &= ~(0x80>>j);
                      }
                  }
              }
            }
        }
      }      
        //处理温度校准值
        if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i])
        {         
            for(j=0;j<8;j++)
            {
              if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i]&(0x80>>j))
              {
                  if(tadj_shift>0)
                  {
                      tadj_shift--;
                      if(tadj&(0x01<<tadj_shift))
                      {
                            TestModeRegisterValue_H_L[i] |= (0x80>>j);
                      }      
                      else
                      {
                            TestModeRegisterValue_H_L[i] &= ~(0x80>>j);
                      }
                  }
              }
            }
        }        
        if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i] | MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i])
        {
//            MC321_IRC_INST(MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i]);      
//            MC321_IRC_INST(MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i]);
         if((i==6)||(i==7))
         {
         }
         else
         {            
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);      
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
        }
        }
        
    }              

        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP     /*无效运行指令*/
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
   
        TEST_STATUS_dig_signal();
        
        SDIO_Turnto_Input();  //PDT设置成输入
    
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
    

    
    Delay_1ms(1);
    
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    Delay_1us(BIT_DELAY_1US);    
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    Delay_1us(BIT_DELAY_1US);
    SDIO_Turnto_Output();   //PDT设置成输出
    MTP_ACK();
    MTP_TYPE_RW(0x00,0); //write
    SendWord(0x00);

    MTP_NACK();
    MTP_STOP();
} 

void MC32P5232FT1(u16 osccal,u16 tadj)
{
    u16 i,j;
    u16 cnt[3];
    u16 osccal_shift,tadj_shift;
    
    osccal_shift = MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num ;
    tadj_shift = MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit_num;
    
    for(i=0;i<2*MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
    {
      //处理频率校准值
      if((i==4)||(i==5))        //只将PFRC的初始值填入，屏蔽HIRC的值写入
      {
      } 
      else
      {
        //处理频率校准值
        if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i])
        {         
            for(j=0;j<8;j++)
            {
              if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i]&(0x80>>j))
              {
                  if(osccal_shift>0)
                  {
                      osccal_shift--;
                      if(osccal&(0x01<<osccal_shift))
                      {
                            TestModeRegisterValue_H_L[i] |= (0x80>>j);
                      }      
                      else
                      {
                            TestModeRegisterValue_H_L[i] &= ~(0x80>>j);
                      }
                  }
              }
            }
        }
      }      
        //处理温度校准值
        if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i])
        {         
            for(j=0;j<8;j++)
            {
              if(MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i]&(0x80>>j))
              {
                  if(tadj_shift>0)
                  {
                      tadj_shift--;
                      if(tadj&(0x01<<tadj_shift))
                      {
                            TestModeRegisterValue_H_L[i] |= (0x80>>j);
                      }      
                      else
                      {
                            TestModeRegisterValue_H_L[i] &= ~(0x80>>j);
                      }
                  }
              }
            }
        }        
        if(MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[i] | MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[i])
        {
//            MC321_IRC_INST(MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i]);      
//            MC321_IRC_INST(MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i]);
       if((i==4)||(i==5))
       {
        }
        else
        {            
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + ((MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0XFF00)>>8) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR1,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + (MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[i] & 0X00FF) ,RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + FSR0,RW_TYPE_DATA);      
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI + TestModeRegisterValue_H_L[i],RW_TYPE_DATA);
        MTP_Write_Word(funCmd_activeInstruction + MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA + INDF2,RW_TYPE_DATA);
        }
        }
    }              

        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP     /*无效运行指令*/
        MTP_Write_Word(0x0000,RW_TYPE_DATA); //1 NOP
   
        TEST_STATUS_dig_signal();
        
        SDIO_Turnto_Input();  //PDT设置成输入
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
    

    Delay_1ms(1);
    
    OTP_SCK_0;
    Delay_1us(BIT_DELAY_1US);
    Delay_1us(BIT_DELAY_1US);    
    OTP_SCK_1;
    Delay_1us(BIT_DELAY_1US);
    Delay_1us(BIT_DELAY_1US);
    SDIO_Turnto_Output();   //PDT设置成输出
    MTP_ACK();
    MTP_TYPE_RW(0x00,0); //write
    SendWord(0x00);

    MTP_NACK();
    MTP_STOP();
} 
/*******************************************************************************
* Function Name  : MCA330_getHardwareCRC
* Description    : 配置寄存器，并获取芯片硬件CRC值
* Input          : ModeNum：0-普通CRC;1-Marign1 CRC;2-Off Margin CRC
*                  optsize：CRC校验地址空间
*                  crcinit：CRC初始值
* Output         : TargetRegister：读取到的CRC值
*******************************************************************************/

u8 MCA330_getHardwareCRC(u8 ModeNum,u16 otpsize,u16 crcinit, u16 *TargetRegister) 
{
    u8 getOK=0;
    u8 PTMMode;
    u8 i;
    u16 addr;
    //u8 error_num_test=0;

    addr = MCU_Config_xx.WriteConfig_xx.ProgModeOptMapRegFirAddr;
    switch(ModeNum)
    {
        case 0:
        PTMMode = 0;
        break;

        case 1:
        PTMMode = 6;
        break;

        case 2:
        PTMMode = 5;
        //          PTMMode = 7;
        break;

        case 3:
        PTMMode = 5;
        break;

        default:
        PTMMode = 0;
        break;
    }

    if( MCA330_Bus_Write(proCmd_PTM , proCmd_BITSIZE ,1))
    {
        if(MCA330_Bus_Write(PTMMode , 16 ,1))
        {  
            for(i=0;i<MCU_Config_xx.WriteConfig_xx.OptionSize;i++)
            {
                if(MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[i])
                {//需要写值
                    if(MCA330_ChangeOption(addr+i,MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[i],MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[i]))
                    {                 
                    }
                    else
                    {
                        getOK=0;
                        return getOK;
                    }
                }
            }

            if(MCA650_SendProgAddr(otpsize))
            {
                if(MCA330_SendProgData(crcinit))
                {
                    if(MCA330_Bus_Write(proCmd_generateChecksum , proCmd_BITSIZE ,0))
                    {
                        //Delay_1ms(10);
                        delay_ms(MCU_Config_xx.CrcConfig_xx.CRC_time_ms);

                        if(GPIO_ReadInputDataBit(OTP_SDO_PORT,OTP_SDO_PIN))
                        {
                            if(MCA330_progData_R(TargetRegister))
                            {
                                getOK =1;
                            }
                            else
                            {
                                //error_num_test=6;
                            }                              
                        }
                        else
                        {
                            //error_num_test=5;
                        }                          
                    }
                    else
                    {
                        //error_num_test=4;
                    }                     
                } 
                else
                {
                    //error_num_test=3;
                } 
            }
            else
            {
                //error_num_test=2;
            }       
        }
        else
        {
            //error_num_test=1;
        }   
    }
    else
    {
        //error_num_test=0;
    }
    return getOK;

}

/*******************************************************************************
* Function Name  : MCA330_ChangeOption
* Description    : 通过改变option映射地址的数据，从而改变option的值
* Input          : addr ,data
*                  
* Output         : flag
*******************************************************************************/
u8 MCA330_ChangeOption(u16 opAddr,u16 opData ,u16 opDataMask)
{
    u8 changeOK=0;
    u16 opDataBuf;
  
    if(MCA650_SendProgAddr(opAddr))
    {
        if(MCA330_SendProgData(opData ))
        {
            if(MCA330_Bus_Write(proCmd_changeOption , proCmd_BITSIZE ,1))
            {
                Delay_1ms(1);
                if(MCA330_OTP_R(&opDataBuf)) 
                {
                    if((opDataBuf & opDataMask) == (opData & opDataMask) )
                    {
                        changeOK =1;
                    }
    
                }

            }
        }
    }
    return changeOK;
}

/*******************************************************************************
* Function Name  : MCA330_progData_R
* Description    : 读取当前地址写入的数据，保存到目标寄存器中
* Input          : 
*                  
* Output         : flag
*******************************************************************************/

u8 MCA330_progData_R(u16 *TargetRegister)
{
        u8 readOK=0;/*1=OK;0=fail*/
	u16 read_data;
	
        
        if(MCA330_Bus_Write(proCmd_readProgData, proCmd_BITSIZE , 1))
        {
          MCA330_Bus_Read(&read_data ,16);
          
          *TargetRegister = read_data;
          readOK =1; 
        }


	return readOK;
}

/*******************************************************************************
* Function Name  : MCA330_ReadCheckRomBad
* Description    : 不同读模式校验rom数据是否正确，并查找坏点
* Input          :  
*                  
* Output         : 0-坏点个数超出可用坏点数 1-坏点个数不超出可用坏点数
*******************************************************************************/
u8 MCA650_ReadFindRomBad(u8 ReadMode)
{
     

    return ReadMode;
}  

/*******************************************************************************
* Function Name  : MCA330_ReadCheckRom
* Description    : 不同读模式校验rom数据是否正确
* Input          :  
*                  
* Output         : 0-有坏点 1-无坏点
*******************************************************************************/
u8 MCA330_ReadCheckRom(u8 ReadMode)
{  
    return 1;
} 


