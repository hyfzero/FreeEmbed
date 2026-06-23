/*
 * @Copyright: Shanghai Sinomcu Microelectronics Co.,Ltd.
 * @Author: Mike.Mo
 * @Emain: mgf@sinomcu.com
 * @Date: 2019-06-25 11:07:18
 * @Encoding: GB2312
 * @Description: 
 */

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "erorrNum.h"
#include "Config.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "stm32_eval.h"
#include "LCD.h"
#include "wx_i2c.h"
#include "IRC.h"

// v99.30 2019.06.25
const u8 FirmwareVersion[5]={73,61,19,06,25};

u16 IRC_VALUE;
u16 OTP_ADDR;
u32 OKcounter;
u16 NGcounter;
u8  WR_Command; // write ? read ? verify ? blank? 
u16 USB_RxDataCS;
u16 OptionCS;
u8  USB_RxCommand;
u8  OPTION_FRT;
u8  OPTION_FRT2;
u8  OPTION_FRT3;
u8  OPTION_TEMPADJ;
u16 OPTION_TEMP;
u8  IRC_MODEL;
u8  OTP_ReadData;
u8  ERORR_VALUE; //Return error number
u16 ERORR_ADDR;
u16 adc_value[3];
u8 USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
u8 USB_Tx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
u8 USB_Config_Buffer[MCU_CONFIG_DATA_SIZE];
u8 USB_Rx_Flag;
u8 StateFlag=0xfe;
u8 ModelIn_result=1;
u8 iic_data[IIC_DATA_BUFF_SIZE];
u8 OP_STATE;
u16 OSCCAL;//校准得到的频率校准值
u8  TADJ;//校准得到的温度校准值
u16 PFRC_OSCCAL;//校准得到的频率校准值

u8 LVRCAL_VALUE;

u8 ROM_OR_EEPROM;   //烧写ROM 和 EEPROM数据选项， bit0: ROM, bit1:EEPROM

u8 WriteFlag;//=1 mc32t8132在线烧录；=0 mc32t8132裸片烧录
u8 ChangePage;//
u16 flag_MCU_ID;
u8  NeedWriteSecondPage=0;
u8  HaveSecondPage=0;

u8  TestModeRegisterValue_H_L[OPTION_ByteSize_MAX];
/*
typedef enum {

	LED1 = 0,
	LED2,
	LED3,
	LED4,
	LED5,

}LED_STATE;

LED_STATE led_status;

**/

/**

* @brief  Configures the different system clocks.

* @param  None

* @retval : None

*/

void RCC_Configuration(void)

{

	/* Setup the microcontroller system. Initialize the Embedded Flash 

	Interface, initialize the PLL and update the SystemFrequency variable. */

	SystemInit();

	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);

	//mco initial
	/*
		RCC_MCO_NoClock----无时钟输出
		RCC_MCO_SYSCLK-----输出系统时钟
		RCC_MCO_HSI--------输出内部高 8MHZ
		RCC_MCO_HSE--------输出高速外部时钟信号
		RCC_MCO_PLLCLK_DIV2--输出 PLL倍频后的二分频时钟
	*/
	RCC_MCOConfig(RCC_MCO_NoClock);
	//RCC_MCOConfig(RCC_MCO_PLLCLK_Div2);

}

/**

* @brief  Configure the GPIOD Pins.

* @param  None

* @retval : None

*/

void GPIO_Configuration(void)

{

	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOF configuration: PC6 ->OSCO(CP),PC7 ->OSCI(TIME_REF), */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11 |GPIO_Pin_12;

	//GPIO_InitStructure.GPIO_Pin = ; //PC11->LEDNG,PC12 ->LEDBUSY

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_13; //pc13 as CLN_VDDtoGND
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	
        //CP_0;
        VDD_IO_Off;
        VPP_IO_Off;
	CLN_VDD2GND_Off;

	// select KEY
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU; // input push up
	GPIO_InitStructure.GPIO_Speed =GPIO_Speed_2MHz;

	GPIO_Init(GPIOD,&GPIO_InitStructure);

	//PA8 config as MCO CLK output
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	//PB1 as USB pushup
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	//PB5=IIC_SDA, PB6=IIC_SCL
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5|GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

    //USB_REG_On;

	//VDD_ADC= PA1/ADC1,VPP_ADC=PA2/ADC2,ADJ_ADC=PA3/ADC3

	

}

void Freq(u8 Numb)
{
	if (Numb==1)
	{
		LEDNG_Off;
		LEDOK_On;
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
		//Delay_1ms(30);
        //OKcounter=OKcounter+1;
		clear_line(4); //clear Write & Check
		display_OK_NG(OKcounter,1);

		GPIO_ResetBits(GPIOA,GPIO_Pin_10);

		display_OK_NG(NGcounter,0);
		clear_line(6); //clear Write & Check
		display_dynamicID();	
	} 
	else
	{
		LEDOK_Off;
		LEDNG_On;
	
        		
        clear_line(4); //clear Write & Check
		display_OK_NG(OKcounter,1);
		display_OK_NG(NGcounter,0);


		clear_line(6);
		// GPIO_ResetBits(GPIOA,GPIO_Pin_10);
		//Delay_1ms(20);
		displayErorr(ERORR_VALUE);
        GPIO_SetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(70);
        GPIO_ResetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(30);
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(50);
		// display_Addr();
		GPIO_ResetBits(GPIOA,GPIO_Pin_10);

	}
}


void Delay_100us(u16 cnt )
{
	u16 i,a;
	
	for (a=0 ;a<cnt;a++)
	{
		for(i=800;i>0;i--);
	}

}
void Delay_10us(u16 cnt )//徐明明加 根据Delay_100us改的  并没有用示波器测量 可能不准
{
	u16 i,a;
	
	for (a=0 ;a<cnt;a++)
	{
		for(i=100;i>0;i--);
	}

}

void Delay_5us(u16 cnt )//
{
	u16 i,a;
	
	for (a=0 ;a<cnt;a++)
	{
		for(i=40;i>0;i--);
	}

}

// void Delay_1us(u16 cnt )//徐明明加 根据Delay_100us改的  并没有用示波器测量 可能不准
// {
// 	u16 i,a;
	
// 	for (a=0 ;a<cnt;a++)
// 	{
// 		for(i=40;i>0;i--);
// 	}

// }

void Delay_1us(u32 count) //精准定时
{
	SysTick->VAL=0; //必须为0时，写Load才会立即重载
	SysTick->LOAD=72*count; //装载计数值，72MHZ时钟源，故72次为1us
	SysTick->CTRL=0x00000005; //时钟源设置HCKLK,使能定时器
	while(!(SysTick->CTRL&0x00010000)); //等待计数到0
	SysTick->CTRL=0x00000004; //closed basic-timer
}

void Delay_1ms(u16 cnt )
{
	u16 i,a;

	for (a=0 ;a<cnt;a++)
	{
		for(i=8500;i>0;i--);
	}

}



void DeviceConfig()
{
	u8 configdata[50];
	u8 tempcount;
    u8  buffer0,buffer1,buffer2;
    u32 config_addr;

    FMReadOne(0x01f000); //config data
    buffer0=Rxdata;
    FMReadOne(0x018d00);
    buffer1=Rxdata;
    FMReadOne(0x017d00);
    buffer2=Rxdata;

    if(buffer0==0xf7)
    {
        config_addr=0x01f000;
    }
    else if(buffer1==0xf7)
    {
        config_addr=0x018d00;
    }
    else
    {
        config_addr=0x017d00;
    }
    

	FM_CS_0;
    FMReadMore(config_addr-1);    //0x017d00

	for (tempcount=0;tempcount<=53;tempcount++)
	{
		SPI_ReadByte();
		configdata[tempcount]=Rxdata;
	}
	FM_CS_1;
	WR_Command = configdata[0];
	DeviceConfig_xx.MCU_ID= configdata[1]*256+configdata[2]; //[4]
	DeviceConfig_xx.ProgramID=configdata[3];
	DeviceConfig_xx.RomFirAddr=configdata[4]*256 + configdata[5];
	DeviceConfig_xx.RomEndAddr=configdata[6]*256 + configdata[7]; 
	DeviceConfig_xx.OptionAddr=configdata[8]*256 +configdata[9];
	DeviceConfig_xx.OptionSize=configdata[10];
	DeviceConfig_xx.OptionProCnt=configdata[11];
	DeviceConfig_xx.BlankCode =configdata[12];

	DeviceConfig_xx.IRC_OPTION=configdata[13];
	DeviceConfig_xx.IRC_FrMax=configdata[14]*256 + configdata[15];
	DeviceConfig_xx.IRC_FrMin=configdata[16]*256 + configdata[17];
	DeviceConfig_xx.IRC_FrType=configdata[18]*256 + configdata[19];

	DeviceConfig_xx.ProTime = configdata[20]*256 + configdata[21];
	DeviceConfig_xx.ProVDD =configdata[22]*256 + configdata[23]; //6.5V
	DeviceConfig_xx.ProVPP =configdata[24]*256 + configdata[25]; // 150
	DeviceConfig_xx.IrcVDD =configdata[26]*256 + configdata[27]; // 5.0=51 3.0=25
	DeviceConfig_xx.IrcVPP =configdata[28]*256 + configdata[29];
	DeviceConfig_xx.VerifyVDD=configdata[30]*256 + configdata[31];
	DeviceConfig_xx.VerifyVPP=configdata[32]*256 + configdata[33];

	DeviceConfig_xx.IRC_Select =configdata[34];
	DeviceConfig_xx.IRC_VALUE_ADDR =configdata[35]*256+configdata[36];
	DeviceConfig_xx.IRC_Shift_Value =configdata[37];
	DeviceConfig_xx.WriteEn =configdata[38];
	DeviceConfig_xx.MCU_Type=configdata[39];

	DeviceConfig_xx.MCU_Name[0]=configdata[40];  
	DeviceConfig_xx.MCU_Name[1]=configdata[41];
	DeviceConfig_xx.MCU_Name[2]=configdata[42];
	DeviceConfig_xx.MCU_Name[3]=configdata[43];
	DeviceConfig_xx.MCU_Name[4]=configdata[44];
	DeviceConfig_xx.MCU_Name[5]=configdata[45];
	DeviceConfig_xx.MCU_Name[6]=configdata[46];
	DeviceConfig_xx.MCU_Name[7]=configdata[47];
	DeviceConfig_xx.MCU_Name[8]=configdata[48];
	DeviceConfig_xx.MCU_Name[9]=configdata[49];

    // if (FMReadOne(0x018d70)==1) 
    // {
    //     tempcount =Rxdata;
    // }
    // if (tempcount==0x39)
    // {
    //     return;
    // }
	// if (FMReadOne(0x017d70)==1)
	// {
	// 	tempcount =Rxdata;
	// }
	
	// if (tempcount!=0x39)
	// {
	// 	ERORR_VALUE=VS_Low;
	// 	IIC_Write(0,0xff);
	// 	IIC_Write(0,0xff);
	// }

}
void LoadCodeToRAM()
{
    u16   tempcount;
    u16   temp_rom_size;
    
    FM_CS_0;
    FMReadMore(Addr_Flash_ROMStart+MCU_Config_xx.WriteConfig_xx.RomFirAddr-1);

    if(MCU_Config_xx.WriteConfig_xx.RomEndAddr>0x3fff)
        temp_rom_size=0x4000;
    else
        temp_rom_size=MCU_Config_xx.WriteConfig_xx.RomEndAddr+1;
    
    for (tempcount=MCU_Config_xx.WriteConfig_xx.RomFirAddr;tempcount<temp_rom_size;tempcount++)             /* 加载code 到sram 中*/
    {
        SPI_ReadByte();
        WRITE_DATA_xx.rom_byte[tempcount] = Rxdata;  //max buf 4k*16bit
    }
    
    FM_CS_1;
}

void LoadOptionToRAM()
{
    u16   tempcount;

    FM_CS_0;
    FMReadMore(Addr_Flash_OptionStart-1);

    for (tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)            
    {
        SPI_ReadByte();
        WRITE_DATA_xx.option_byte[tempcount] = Rxdata;
    }
    
    FM_CS_1;
    
    for (tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)            
    {
        if(tempcount%2)
        {
          WRITE_DATA_xx.option_byte_H_L[tempcount] = WRITE_DATA_xx.option_byte[tempcount-1];          
        }
        else
        {
          WRITE_DATA_xx.option_byte_H_L[tempcount] = WRITE_DATA_xx.option_byte[tempcount+1];                    
        }
    }   
}
void LoadConfigToRAM()
{
    u16   i,tempcount,temp;
    u8    configdata[Config_ByteSize_MAX];
    
    FM_CS_0;
    FMReadMore(Addr_Flash_CfgStart-1);

    for (tempcount=0;tempcount<Config_ByteSize_MAX;tempcount++)            
    {
        SPI_ReadByte();
        configdata[tempcount] = Rxdata;
    }
    
    if(configdata[0] == 0xf7)
    {
        i = 1;
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMin           = temp*256+configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.IRC_FrType          = temp*256+configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.IRC_FrMax           = temp*256+configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.RomFirAddr         = temp*256+configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.RomEndAddr         = temp*256+configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.SoftCheckSum_addr      = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.HardCheckSum_addr      = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.BadDotConfig_xx.BadDotRomAddrShift    = temp*256+configdata[i++];        
        MCU_Config_xx.MTP_CONFIG_xx.EnableWrite    = configdata[i++];  
        
        MCU_Config_xx.WriteConfig_xx.OptionSize         = configdata[i++];
        
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.OptionAddr[tempcount]     = temp*256+configdata[i++];  
        }       
        
        MCU_Config_xx.IRC_CONFIG_xx.TadjNum                                 = configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.IRC_CONFIG_xx.TadjNum;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.IRC_CONFIG_xx.TadjValue[tempcount]                = temp*256+configdata[i++]; 
        }           
        //进模式相关配置
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID           = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr      = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version      = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version_addr = temp*256+configdata[i++];     
        
        
//        temp = configdata[i++];
//        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H       = temp*256+configdata[i++];
//        temp = configdata[i++];
//        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr  = temp*256+configdata[i++];        
//        temp = configdata[i++];
//        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L  = temp*256+configdata[i++];
//        temp = configdata[i++];
//        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L_addr   = temp*256+configdata[i++];
//        
//        temp = configdata[i++];       
//        MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version  = temp*256+configdata[i++];   
//        temp = configdata[i++];
//        MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr  = temp*256+configdata[i++];  
//        temp = configdata[i++];
//          temp = configdata[i++];
        
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H       = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L  = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr  = temp*256+configdata[i++];        
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L_addr   = temp*256+configdata[i++];
        
        temp = configdata[i++];       
        MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_version  = temp*256+configdata[i++];   
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr  = temp*256+configdata[i++];  
        temp = configdata[i++];
        temp = configdata[i++];
        
        //MCU_Config_xx.ModeIn_CONFIG_xx.WriteTool_addr_ValidBit  = temp*256+configdata[i++]; 
        
        MCU_Config_xx.ModeIn_CONFIG_xx.WriteType        = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.VddWrite         = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.ProVDD           = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.ProVPP           = configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Tdly             = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.Twait            = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.ModeIn_CONFIG_xx.UnlockCode       = temp*256+configdata[i++];
        for(tempcount=0;tempcount<10;tempcount++)
        {
            MCU_Config_xx.ModeIn_CONFIG_xx.MCU_Name[tempcount]     = configdata[i++];
        }              
        
        //ROM烧写相关配置
        MCU_Config_xx.WriteConfig_xx.WriteEn            = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.Page               = configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.OtpValidBit        = temp*256+configdata[i++]; 
        
        MCU_Config_xx.WriteConfig_xx.SecurityOption     = configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.SecurityBitMask[tempcount]     = temp*256+configdata[i++]; 
        }        
        
        MCU_Config_xx.WriteConfig_xx.HaveSoftCheckSumAddr   = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.HaveHardCheckSumAddr   = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.ReadCheckRomMode       = configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.WriteProcess           = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.WriteConfig_xx.ProgModeOptMapRegFirAddr  = temp*256+configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[tempcount]     = temp*256+configdata[i++];  
        }               
        
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.EnginOptionMask[tempcount]     = temp*256+configdata[i++];  
        }          
        
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.TestOptionMask[tempcount]     = temp*256+configdata[i++];  
        }         
        MCU_Config_xx.WriteConfig_xx.PassEnginTest   = configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.WriteConfig_xx.EnginTestValue[tempcount]     = temp*256+configdata[i++];  
        }         
        
        
        //CRC校验相关配置
        MCU_Config_xx.CrcConfig_xx.CrcCheckRomMode                          = configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[tempcount]       = temp*256+configdata[i++];  
        }         
        
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[tempcount]      = temp*256+configdata[i++];  
        }         
        
        temp = configdata[i++];
        MCU_Config_xx.CrcConfig_xx.CRC_time_ms                              = temp*256+configdata[i++];
                                  
        //IRC校准相关配置
        MCU_Config_xx.IRC_CONFIG_xx.IRC_Select                              =configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.TadjChoose                              = configdata[i++];
        
        temp = configdata[i++];
        temp = configdata[i++];
        
        temp = configdata[i++];        
        MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI                           = temp*256+configdata[i++]; 
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA                           = temp*256+configdata[i++]; 
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[tempcount]    = configdata[i++];  
        }                     
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.IRC_CONFIG_xx.OsccalOptionBitMask[tempcount]    = temp*256+configdata[i++];  
        } 
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[tempcount]    = configdata[i++];  
        }         
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.IRC_CONFIG_xx.TadjOptionBitMask[tempcount]    = temp*256+configdata[i++];  
        }    
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit_num    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr    = temp*256+configdata[i++];
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.TMODE_value    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegInvalidAddr    = temp*256+configdata[i++]; 
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[tempcount]    = temp*256+configdata[i++];   
        }    
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[tempcount]    = configdata[i++];  
        }
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[tempcount]    = configdata[i++];  
        }        
        for(tempcount=0;tempcount<MCU_Config_xx.WriteConfig_xx.OptionSize*2;tempcount++)
        {
            MCU_Config_xx.IRC_CONFIG_xx.LoadUserChooseBitMask_H_L[tempcount]    = configdata[i++];  
        }         
        MCU_Config_xx.IRC_CONFIG_xx.IRC_CheckAfterWrite    = configdata[i++];  
        
        //MTP烧写相关配置
        MCU_Config_xx.MTP_CONFIG_xx.HaveMTP    = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.EnableWrite    = configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.MtpFirAddr    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.MtpEndAddr    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.ByteSizeForChip    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.ByteSizeForPage    = temp*256+configdata[i++];  
        temp = configdata[i++];
        MCU_Config_xx.MTP_CONFIG_xx.PageSizeForChip    = temp*256+configdata[i++];  
        MCU_Config_xx.MTP_CONFIG_xx.WriteMode    = configdata[i++];  
        
        //Bad Dot相关配置
        MCU_Config_xx.BadDotConfig_xx.BadDotNum    = configdata[i++];
        for(tempcount=0;tempcount<MCU_Config_xx.BadDotConfig_xx.BadDotNum;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_ADDR[tempcount]    = temp*256+configdata[i++];   
        }        
        for(tempcount=0;tempcount<MCU_Config_xx.BadDotConfig_xx.BadDotNum;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_DATA[tempcount]    = temp*256+configdata[i++];   
        }         
//        temp = configdata[i++];
//        MCU_Config_xx.BadDotConfig_xx.BadDotRomAddrShift    = temp*256+configdata[i++];   
        temp = configdata[i++];
        MCU_Config_xx.BadDotConfig_xx.BadDotEnableBitMask    = temp*256+configdata[i++];   
        for(tempcount=0;tempcount<MCU_Config_xx.BadDotConfig_xx.BadDotNum;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.BadDotConfig_xx.BadDotRegAddr_MOTP0_ADDR[tempcount]    = temp*256+configdata[i++];   
        }         
        for(tempcount=0;tempcount<MCU_Config_xx.BadDotConfig_xx.BadDotNum;tempcount++)
        {
            temp = configdata[i++];
            MCU_Config_xx.BadDotConfig_xx.BaddotRegDataMask[tempcount]    = temp*256+configdata[i++];   
        }         
          
    }
        
    FM_CS_1;
}

void ConfigMCU(void)
{
//------------------------------配置烧写相关参数----------------------------     
     MCU_Config_xx.WriteConfig_xx.Page=0;//是否分页 需要写page
     MCU_Config_xx.WriteConfig_xx.SecurityOption=0;//加密位所在option索引 
     MCU_Config_xx.WriteConfig_xx.SoftCheckSum_addr=0x800C;
     MCU_Config_xx.WriteConfig_xx.HaveSoftCheckSumAddr=0x01;
     MCU_Config_xx.WriteConfig_xx.ReadCheckRomMode=0x03;
     MCU_Config_xx.WriteConfig_xx.OptionSize=7;
     MCU_Config_xx.CrcConfig_xx.CrcCheckRomMode=0x03;
     
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[0]=0x0;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[1]=0x0;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[2]=0x0;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitValue[3]=0x0;
     
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[0]=0x0000;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[1]=0x00e8;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[2]=0x0000;
     MCU_Config_xx.CrcConfig_xx.CrcNeedWriteBitMask[3]=0x0000;
     MCU_Config_xx.CrcConfig_xx.CRC_time_ms=50;//CRC校验等待时间 ms
     MCU_Config_xx.BadDotConfig_xx.BadDotNum=2;
     MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_ADDR[0]=0x8009;//存放坏点地址的option地址（数组从高地址向地地址排列）
     MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_ADDR[1]=0x8007;//存放坏点地址的option地址（数组从高地址向地地址排列）
     MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_DATA[0]=0x800A;//存放坏点数据的option地址（数组从高地址向地地址排列）
     MCU_Config_xx.BadDotConfig_xx.BadDotAddr_MOTP0_DATA[1]=0x8008;//存放坏点数据的option地址（数组从高地址向地地址排列）
     
     //1K模式或0.5K模式第二次烧录 坏点修复rom偏移地址
     MCU_Config_xx.BadDotConfig_xx.BadDotRomAddrShift=0x0000;//不同分页 rom偏移地址 上位机根据页选 下传rom偏移地址 
     
     //0.5K模式第一次烧录 坏点修复rom偏移地址
     //MCU_Config_xx.WriteConfig_xx.BadDotAddrShift=0x0200;//不同分页 rom偏移地址 上位机根据页选 下传rom偏移地址 
     
     MCU_Config_xx.BadDotConfig_xx.BadDotEnableBitMask=0x8000;//坏点使能位
     MCU_Config_xx.BadDotConfig_xx.BadDotRegAddr_MOTP0_ADDR[0]=0xFF09;//坏点修复配置字寄存器（先高地址 后低地址）
     MCU_Config_xx.BadDotConfig_xx.BadDotRegAddr_MOTP0_ADDR[1]=0XFF07;//坏点修复配置字寄存器
     MCU_Config_xx.BadDotConfig_xx.BaddotRegDataMask[0]=0x27FF;//坏点1地址有效位
     MCU_Config_xx.BadDotConfig_xx.BaddotRegDataMask[1]=0x27FF;//坏点2地址有效位
     
     MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x3f;//校验流程 123456  CRC校验1、全读校验2、全读找坏点3、坏点修复4、修复后CRC校验5、修复后全读校验6
     //MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x2E;//1345    CRC校验、全读找坏点、坏点修复、修复后CRC校验
     //MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x1D;//2346    全读校验、全读找坏点、坏点修复、修复后全读校验
     //MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x20;//1       CRC校验
     //MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x10;//2       全读校验
     //MCU_Config_xx.WriteConfig_xx.WriteProcess = 0x30;//12      CRC校验、全读校验

     
//测试版本配置如下      
//     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[0]=0x0000;
//     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[1]=0x0000;
//     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[2]=0x0000;
//     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[3]=0x003f;
         
//     MCU_Config_xx.WriteConfig_xx.TestOptionMask[0]=0x0000;
//     MCU_Config_xx.WriteConfig_xx.TestOptionMask[1]=0x0000;
//     MCU_Config_xx.WriteConfig_xx.TestOptionMask[2]=0x001f;
//     MCU_Config_xx.WriteConfig_xx.TestOptionMask[3]=0x0000;
     
//正式版本配置如下     
     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[0]=0x0000;//工程测试值标志位
     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[1]=0x0000;//工程测试值标志位
     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[2]=0x001f;//LVRCAL
     MCU_Config_xx.WriteConfig_xx.EnginOptionMask[3]=0x003f;//VDCAL option3 bit5:0 工程测试值
     
     MCU_Config_xx.WriteConfig_xx.TestOptionMask[0]=0x0000;//验证测试填值标志位
     MCU_Config_xx.WriteConfig_xx.TestOptionMask[1]=0x0000;
     MCU_Config_xx.WriteConfig_xx.TestOptionMask[2]=0x0000;
     MCU_Config_xx.WriteConfig_xx.TestOptionMask[3]=0x0000;
     
     MCU_Config_xx.WriteConfig_xx.SecurityBitMask[0]=0x0010;//加密位标志
     MCU_Config_xx.WriteConfig_xx.SecurityBitMask[1]=0x0000;
     MCU_Config_xx.WriteConfig_xx.SecurityBitMask[2]=0x0000;
     MCU_Config_xx.WriteConfig_xx.SecurityBitMask[3]=0x0000;   
     
     MCU_Config_xx.IRC_CONFIG_xx.IRC_CheckAfterWrite = 1;//烧写结束IRC再次验证
     
     
//------------------------------配置进模式相关参数----------------------------  
     MCU_Config_xx.ModeIn_CONFIG_xx.WriteType = WriteFPGA;
     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID=0xA383;//项目ID 
     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_ID_addr=0xFFFF;//项目ID存放地址
     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H=0x0000;//测试 
     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_H_addr=0x8028;//测试 
     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L=0x0000;
     MCU_Config_xx.ModeIn_CONFIG_xx.Product_id_L_addr=0x8026;
     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version=0X10;
     MCU_Config_xx.ModeIn_CONFIG_xx.MCU_version_addr=0XFFFE;
     MCU_Config_xx.ModeIn_CONFIG_xx.VddWrite=vdd_adjust;//可调电压
     MCU_Config_xx.ModeIn_CONFIG_xx.Tdly=50;//us
     MCU_Config_xx.ModeIn_CONFIG_xx.Twait=4000;//us
     MCU_Config_xx.ModeIn_CONFIG_xx.UnlockCode=0xaeed;
     
//------------------------------IRC校准相关配置----------------------------   
    MCU_Config_xx.IRC_CONFIG_xx.TadjChoose = 0;
    MCU_Config_xx.IRC_CONFIG_xx.TadjNum = 3;
    MCU_Config_xx.IRC_CONFIG_xx.TadjValue[0] = 0x0E;
    MCU_Config_xx.IRC_CONFIG_xx.TadjValue[1] = 0x0D;
    MCU_Config_xx.IRC_CONFIG_xx.TadjValue[2] = 0x0F;
    MCU_Config_xx.IRC_CONFIG_xx.InstructMOVAI=0x0b00;
    MCU_Config_xx.IRC_CONFIG_xx.InstructMOVRA=0x1580;
    
    
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[0]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[1]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[2]=0x00;//option1 bit10:0   
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[3]=0x00;  
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[4]=0x00; 
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[5]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[6]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalRegisterBitMask_H_L[7]=0x00;    
    

    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[0]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[1]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[2]=0x00;   
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[3]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[4]=0x0f;//option2 bit12:8 
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[5]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[6]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjRegisterBitMask_H_L[7]=0x00;    
    
    MCU_Config_xx.IRC_CONFIG_xx.OsccalOptionBitMask[0]=0x0000;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalOptionBitMask[1]=0x0000;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalOptionBitMask[2]=0x00ff;
    MCU_Config_xx.IRC_CONFIG_xx.OsccalOptionBitMask[3]=0x0000;

    MCU_Config_xx.IRC_CONFIG_xx.TadjOptionBitMask[0]=0x0000;
    MCU_Config_xx.IRC_CONFIG_xx.TadjOptionBitMask[1]=0x0000;
    MCU_Config_xx.IRC_CONFIG_xx.TadjOptionBitMask[2]=0x0f00;
    MCU_Config_xx.IRC_CONFIG_xx.TadjOptionBitMask[3]=0x0000;      
    
    MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit_num=8;
    MCU_Config_xx.IRC_CONFIG_xx.OSCCAL_bit=0x00ff;

    MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit_num=4;
    MCU_Config_xx.IRC_CONFIG_xx.TADJ_bit=0x000f;
    
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegInvalidAddr = 0x00;//定义无效地址
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[0]=0x00;//无对应寄存器地址 不写
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[1]=0x77;//
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[2]=0x79;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[3]=0x78;  
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[4]=0x7B;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[5]=0x7A;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[6]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegAddr_H_L[7]=0x7C; 
    
    MCU_Config_xx.WriteConfig_xx.ProgModeOptMapRegFirAddr=0xfff0;
    //MCU_Config_xx.IRC_CONFIG_xx.register_size=0x08;
 
    MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[0]=0x001f;
    MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[1]=0x03ff;
    MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[2]=0x07ff;
    MCU_Config_xx.WriteConfig_xx.ProgModeOptMapReg_ValidBit[3]=0x000f;

    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[0]=0xff;//
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[1]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[2]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[3]=0xf7;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[4]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[5]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[6]=0xff;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegMask_H_L[7]=0xff;
    
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[0]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[1]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[2]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[3]=0x00;   
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[4]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[5]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[6]=0x00;
    MCU_Config_xx.IRC_CONFIG_xx.FunTestModeOptMapRegValue_H_L[7]=0x00;       
    MCU_Config_xx.IRC_CONFIG_xx.TMODE_addr=0x70;//测试功能寄存器TMODE地址
    MCU_Config_xx.IRC_CONFIG_xx.TMODE_value=0x01;//高频模式
   
//------------------------------MTP烧写相关配置---------------------------- 
    MCU_Config_xx.MTP_CONFIG_xx.HaveMTP = 0;//芯片是否需要烧写MTP数据区
    MCU_Config_xx.MTP_CONFIG_xx.EnableWrite = 0;
    MCU_Config_xx.MTP_CONFIG_xx.MtpFirAddr = 0xc000;
    MCU_Config_xx.MTP_CONFIG_xx.MtpEndAddr = 0xc0ff;
    MCU_Config_xx.MTP_CONFIG_xx.ByteSizeForChip = 256;
    MCU_Config_xx.MTP_CONFIG_xx.PageSizeForChip = 16;
    MCU_Config_xx.MTP_CONFIG_xx.ByteSizeForPage = 16;
    MCU_Config_xx.MTP_CONFIG_xx.WriteMode = 1;//Page Write
    
//------------------------------OPTION烧写相关配置----------------------------   
    //1K模式或0.5K模式第二次烧录时对应配置字地址
    MCU_Config_xx.WriteConfig_xx.OptionAddr[0] = 0x8000;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[1] = 0x8001;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[2] = 0x8002;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[3] = 0x8003;
    
    //0.5K模式第一次烧录时对应配置字
    /*
    MCU_Config_xx.WriteConfig_xx.OptionAddr[0] = 0x8000;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[1] = 0x8004;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[2] = 0x8005;
    MCU_Config_xx.WriteConfig_xx.OptionAddr[3] = 0x8006;    
    */
}




/******************************************************************************
函数名称：读取芯片唯一ID码
创建时间：2019-06-27
修改时间：
备    注：
******************************************************************************/
void Get_ChipID(void)
{
//     u32 temp0,temp1,temp2;
//     temp0 = *(__IO u32*)(0x1FFFF7E8);    //产品唯一身份标识寄存器（96位）
//     temp1 = *(__IO u32*)(0x1FFFF7EC);
//     temp2 = *(__IO u32*)(0x1FFFF7F0);
                                  
// //ID码地址： 0x1FFFF7E8   0x1FFFF7EC  0x1FFFF7F0 ，只需要读取这个地址中的数据就可以了。

//     temp[0] = (u8)(temp0 & 0x000000FF);
//     temp[1] = (u8)((temp0 & 0x0000FF00)>>8);
//     temp[2] = (u8)((temp0 & 0x00FF0000)>>16);
//     temp[3] = (u8)((temp0 & 0xFF000000)>>24);
//     temp[4] = (u8)(temp1 & 0x000000FF);
//     temp[5] = (u8)((temp1 & 0x0000FF00)>>8);
//     temp[6] = (u8)((temp1 & 0x00FF0000)>>16);
//     temp[7] = (u8)((temp1 & 0xFF000000)>>24);
//     temp[8] = (u8)(temp2 & 0x000000FF);
//     temp[9] = (u8)((temp2 & 0x0000FF00)>>8);
//     temp[10] = (u8)((temp2 & 0x00FF0000)>>16);
//     temp[11] = (u8)((temp2 & 0xFF000000)>>24);         
}

