
#ifndef CONFIG_H
#define CONFIG_H

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "global.h"

typedef struct DEVICE_CONFIG
{
	u16  MCU_ID;
	u8   ProgramID;
	u16  RomFirAddr;
	u16  RomEndAddr;
	u16  OptionAddr;
	u8   OptionSize;
	u8   OptionProCnt;
	u8   BlankCode;

	u8   IRC_OPTION; //irc adjust loadvalue
	u16	 IRC_FrMax;
	u16   IRC_FrMin;
	u16   IRC_FrType;
	
	u16 ProTime;
	u16 ProVDD;
	u16 ProVPP;
	u16 IrcVDD;
	u16 IrcVPP;
	u16 VerifyVDD;
	u16 VerifyVPP;

	u8  IRC_Select; // 0 =nul ,1=irc,2 = read and write
	u16 IRC_VALUE_ADDR; // m202,m202b:0x0004,,m101,m111:0x0002
	u8  IRC_Shift_Value ; // 校准值偏移量
    u8  WriteEn; // 低字节= F 时 写 ROM，高字节=F时写OPTION 0。
	u8	MCU_Type; //0x01:OTP ,0x02:MTP,0X03:flash
	u8  MCU_Name[10];

}DEVICE_CONFIG;// *DEVICE_CONFIG;

#define  MCU_Type_OTP   0X01
#define  MCU_Type_MTP   0x02
#define  MCU_Type_Flash 0x03

extern DEVICE_CONFIG DeviceConfig_xx;

typedef struct CRC_CONFIG
{
    u8      CrcCheckRomMode;//bit0=1 需要普通CRC校验；bit1=1需要Margin-1CRC校验；bit2=1需要off MarginCRC校验
    //u16     OptMapReg_FirAddr;//option映射寄存器地址 起始地址
    //u16     OptMapReg_ValidBitMask[OPTION_WordSize_MAX];//option映射寄存器地址有效位   
    u16     CrcNeedWriteBitMask[OPTION_WordSize_MAX];//bit = 1 需要写    
    u16     CrcNeedWriteBitValue[OPTION_WordSize_MAX];//CRC校准条件应该配置OPTION值
    u16     CRC_time_ms;

}CRC_CONFIG;// *CRC_CONFIG;
//extern CRC_CONFIG CrcConfig_xx;

typedef struct BadDot_CONFIG
{
    u8      BadDotNum;
    u16     BadDotAddr_MOTP0_ADDR[BadDotSize_MAX];//存放坏点地址的option地址（数组从高地址向地地址排列）
    u16     BadDotAddr_MOTP0_DATA[BadDotSize_MAX];//存放坏点数据的option地址（数组从高地址向地地址排列）
    u16     BadDotRomAddrShift;
    u16     BadDotEnableBitMask;//坏点使能位 
    u16     BadDotRegAddr_MOTP0_ADDR[BadDotSize_MAX];
    u16     BaddotRegDataMask[BadDotSize_MAX];

}BadDot_CONFIG;// *CRC_CONFIG;
//extern BadDot_CONFIG BadDotConfig_xx;

typedef struct WRITE_CONFIG
{ 
    u8      WriteEn;//0xf0--Option, 0x0f--ROM
    u8      Page;              //=0 不分page;=1有page 
    u8      OptionSize;   
    u16     OptionAddr[OPTION_WordSize_MAX];    
	u16     RomFirAddr;
	u16     RomEndAddr;
    u16     OtpValidBit;//otp数据有效位    
    u8      SecurityOption;    //加密位在option?
    u16     SecurityBitMask[OPTION_ByteSize_MAX];       //加密位     
    u8      HaveSoftCheckSumAddr;
    u16     SoftCheckSum_addr;//软件checksum地址    
    u8      HaveHardCheckSumAddr;
    u16     HardCheckSum_addr;//硬件checksum地址
    u8      ReadCheckRomMode;//bit0=1 需要普通读校验；bit1=1需要Margin-1读校验；bit2=1需要off Margin读校验
    u16     WriteProcess;//bit0=1 坏点修复后全读校验；bit1 坏点修复后CRC校验；bit2 进行坏点修复；bit3 烧完后全读找坏点；
                         //bit4=1 烧完后全读校验；bit5烧完后CRC校验
    
    u16     ProgModeOptMapRegFirAddr;//编程模式下opiton映射寄存器起始地址 
    u16     ProgModeOptMapReg_ValidBit[OPTION_WordSize_MAX];//   编程模式下opiton映射寄存器有效位  
    u16     EnginOptionMask[OPTION_ByteSize_MAX];        //工程测试校准位标记                 bitn=1 说明bitn是工程测试校准位
    u16     TestOptionMask[OPTION_ByteSize_MAX];         //验证测试位，最终应该是工程测试位（芯片有值，取芯片值；芯片无值，上位机填值非空，取上位机值；芯片无值，上位机填值空，报错）   
    u8      PassEnginTest;//=1 经过工程测试 =0未经过工程测试
    u16     EnginTestValue[OPTION_WordSize_MAX];//未经过工程测试的芯片烧写时模拟工程写值
}WRITE_CONFIG;// 烧录芯片时所需的一些参数
//extern WRITE_CONFIG WriteConfig_xx;

typedef struct ModeIn_CONFIG
{
    u16     MCU_ID;             //芯片ID号 
    u16     MCU_ID_addr;        //芯片ID存放地址
    u16     MCU_version;        //芯片版本号
    u16     MCU_version_addr;   //芯片版本号存放地址     
    u16     Product_id_H;       //产品主ID高字节
    u16     Product_id_H_addr;  //产品次ID高字节地址
    u16     Product_id_L;       //产品ID低字节
    u16     Product_id_L_addr;  //产品ID低字节地址
    u16     WriteTool_version;        //芯片版本号
    u16     WriteTool_addr;   //芯片版本号存放地址 
    //u16     WriteTool_addr_ValidBit; //烧录工具有效位
    u16     WriteType;          //烧写类型 FPGA/实际MCU    
//    u8      ModeInTiming;       //进模式时序 0-MC32P31;1-MC30P6060;2-MC32P6230
    u8      VddWrite;           //VDD电压
    u8      ProVDD;
    u8      ProVPP;
    u16     Tdly;               //VDD上电之后延时Tdly时间，再给进模式时序
    u16     Twait;              //给完进模式时序后，延时Twait时间，再读MCU_ID
    u16     UnlockCode;         //进模式解锁码
	u8      MCU_Name[10];
}ModeIn_CONFIG;// 烧录芯片时所需的一些参数
//extern ModeIn_CONFIG ModeIn_CONFIG_xx;

typedef struct WRITE_DATA
{
    union
    {
        u8  rom_byte[RomByteSize_MAX];//支持4K*16bit code烧写                 
        u16 rom_word[RomWordSize_MAX];                
    };
    union
    {
        u8  option_byte[OPTION_ByteSize_MAX]; 
        u16 option_word[OPTION_WordSize_MAX];   
    };
    u8 option_byte_H_L[OPTION_ByteSize_MAX]; 
}WRITE_DATA;// 烧录芯片时所需的一些参数
extern WRITE_DATA WRITE_DATA_xx;


typedef struct IRC_CONFIG
{
    u8      IRC_Select;//IRC_Selsct IRC_Selsct  00=NULL , 01=IRC ,02 = Read and Writ
	u16	    IRC_FrMin;
	u16     IRC_FrType;
	u16     IRC_FrMax;  
    u8      TadjChoose;//=0 测试模式上位机填温度较准值 ；=1正式版本工具 特定温度校准值
    u8      TadjNum;//温度校准点个数
    u8      TadjValue[TadjNumMax];//温度校准值
    u16     InstructMOVAI;
    u16     InstructMOVRA;

    u8      OsccalRegisterBitMask_H_L[OPTION_ByteSize_MAX];//先高位 后低位
    u16     OsccalOptionBitMask[OPTION_WordSize_MAX];
    u8      TadjRegisterBitMask_H_L[OPTION_ByteSize_MAX];
    u16     TadjOptionBitMask[OPTION_WordSize_MAX];
    u16     OSCCAL_bit_num;              //osccal 位数
    u16     OSCCAL_bit;              //osccal 位
    u16     TADJ_bit_num;                //TADJ位数
    u16     TADJ_bit;                //TADJ位
    //u8    register_size;
    u16     TMODE_addr;
    u16     TMODE_value;
    u16     FunTestModeOptMapRegInvalidAddr;//功能测试模式下option映射寄存器无效地址
    u16     FunTestModeOptMapRegAddr_H_L[OPTION_ByteSize_MAX];//先高位寄存器地址、再低位寄存器地址
    u8      FunTestModeOptMapRegMask_H_L[OPTION_ByteSize_MAX];
    u8      FunTestModeOptMapRegValue_H_L[OPTION_ByteSize_MAX]; 
    u8      LoadUserChooseBitMask_H_L[OPTION_ByteSize_MAX];
    u8      IRC_CheckAfterWrite;//=1 需要再验    
}IRC_CONFIG;
//extern IRC_CONFIG IRC_CONFIG_xx;

typedef struct MTP_CONFIG
{
    u8   HaveMTP;  
    u8   EnableWrite;
	u16  MtpFirAddr;
	u16  MtpEndAddr;    
    u16  ByteSizeForChip;///一个chip 有多少Byte
    u16  ByteSizeForPage;//一个page 有多少byte
    u16  PageSizeForChip;//一个chip 有多少page
    u8   WriteMode;//=0 ByteWrite;=1 PageWrite; =2 ChipWrite 
}MTP_CONFIG;
//extern MTP_CONFIG MTP_CONFIG_xx;

typedef struct MCU_CONFIG
{
    ModeIn_CONFIG   ModeIn_CONFIG_xx;
    WRITE_CONFIG    WriteConfig_xx;
    CRC_CONFIG      CrcConfig_xx;
    IRC_CONFIG      IRC_CONFIG_xx;
    MTP_CONFIG      MTP_CONFIG_xx;
    BadDot_CONFIG   BadDotConfig_xx;
}MCU_CONFIG;
extern MCU_CONFIG MCU_Config_xx;

#endif