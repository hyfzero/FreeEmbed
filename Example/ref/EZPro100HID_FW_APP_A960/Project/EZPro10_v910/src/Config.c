#include "stm32f10x.h"

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#include "stm32f10x_tim.h"
#include "global.h"
#include "spi.h"
#include "power.h"
#include "OTPRW.h"
#include "IRC.h"
#include "Config.h"



DEVICE_CONFIG DeviceConfig_xx=
{
	0x0353, //MCU_ID
	0x00,	//ProgramID
	0x1800,	//RomFirAddr
	0x1fff,  //RomEndAddr
	0x0000,	//OptionAddr
	0x02,	//OptionSize
	0x02,	//OptionProCnt  //现在基本不用
	0xff,	//BlankCode     //现在基本不用

	0x00,	//IRC_OPTION    //现在基本不用，以前只有一个OPTION字节时，上位机直接发配置信息给烧写器
	0x65,  //IRC_FrMax
	0x2e,   //ICR_FrMin
	0x40,	//IRC_FrType

	2000,	//ProTime       //如9033开发手册5.4节OTP编程时序图，有个大于100us，现在一般设置4
	0x0605,	//ProVDD        //现在基本不用
	150,	//ProVPP        
	25,	    //IrcVDD
	150,  //IrcVPP          //现在基本不用
	0x0300,	//VerifyVDD     //现在基本不用
	0x0b00, //VerifyVPP     //现在基本不用

	0x00,   //IRC_Selsct    //IRC_Selsct 00=NULL , 01=IRC ,02 = Read and Write
	0x0002, //IRC_VALUE_ADDR 现在不用，以前是个地址数据，该地址存放IRC校准得到的数据
	0x06,   //IRC_Shift_Value 现在基本不用 
        0xff, // u8  WriteEn; 0xf0--Option, 0x0f--ROM
	0x01,	//mcu type: 0x01:OTP ,0x02 :MTP ,0X03 :flash
	//7D27 4D433130        419     db 'MC10P02R';
	//7D2B 50303252
	{0x4d,	//mcu Name
	0x43,	//mcu Name
	0x31,	//mcu Name
	0x30,	//mcu Name
	0x50,	//mcu Name
	0x30,	//mcu Name
	0x32,	//mcu Name
	0x52,	//mcu Name
	0x20,	//mcu Name
	0x20}	//mcu Name
};

//CRC_CONFIG      CrcConfig_xx;
//WRITE_CONFIG    WriteConfig_xx;
//ModeIn_CONFIG   ModeIn_CONFIG_xx;
//IRC_CONFIG      IRC_CONFIG_xx;
//MTP_CONFIG      MTP_CONFIG_xx;
//BadDot_CONFIG   BadDotConfig_xx;

MCU_CONFIG      MCU_Config_xx = {0};
WRITE_DATA      WRITE_DATA_xx;