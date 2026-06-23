#ifndef MC32P5232Pro_H
#define MC32P5232Pro_H


#define CRC_16_POLYNOMIALS      0x8005
#define ProgMTP_Tprog 2750

/*烧录模式指令�?*/
#define proCmd_BITSIZE          6
#define proCmd_functionMode	0x0a
#define proCmd_readProgAddr	0x12
#define proCmd_readProgData	0x13
#define proCmd_addrAdd  	0x14
#define proCmd_writeProgAddr	0x15
#define proCmd_writeProgData	0x16
#define proCmd_readRomData	0x17
#define proCmd_generateChecksum	0x18
#define proCmd_prog	        0x19
#define proCmd_PTM	        0x1A
#define proCmd_erase	        0x1b    /*mtp only*/
#define proCmd_changeOption     0x1c
#define proCmd_readRomDataAddrAdd       0x27
#define proCmd_progAddrAdd      0x29

/*烧录模式指令�?*/
#define proCmd_BITSIZE          6
#define proCmd_functionMode	0x0a
#define proCmd_readProgAddr	0x12
#define proCmd_readProgData	0x13
#define proCmd_addrAdd  	0x14
#define proCmd_writeProgAddr	0x15
#define proCmd_writeProgData	0x16
#define proCmd_readRomData	0x17
#define proCmd_generateChecksum	0x18
#define proCmd_prog	        0x19
#define proCmd_PTM	        0x1A
#define proCmd_erase	        0x1b    /*mtp only*/
#define proCmd_changeOption     0x1c
#define proCmd_readRomDataAddrAdd       0x27
#define proCmd_progAddrAdd      0x29

/* 测试模式指令�?*/
#define funCmd_BITSIZE          18
#define funCmd_activeInstruction      0x00000000
#define funCmd_quit                   0x00020000
#define funInstruction_MOVAI          0x3c00
#define funInstruction_MOVRA          0x5600
#define funInstruction_MOVAR          0x1600
#define funInstruction_NOP            0x0000
#define funAddr_TMODE                 0x70
#define funCmd_TMODE_NORMAL          0x00
#define funCmd_TMODE_HRC             0x01
#define funCmd_TMODE_LRC             0x02
#define funCmd_TMODE_LVR             0x03
#define funCmd_TMODE_USER            0xFF

#define WriteProcess_EnableCrcAfterWrite        MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<5)
#define WriteProcess_EnableReadCheckAfterWrite  MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<4)
#define WriteProcess_EnableReadBadDotAfterWrite MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<3)
#define WriteProcess_EnableRepairBadAfterWrite  MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<2)
#define WriteProcess_EnableCrcAfterRepair       MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<1)
#define WriteProcess_EnableReadCheckAfterRepair MCU_Config_xx.WriteConfig_xx.WriteProcess&(0x0001<<0)

#define Nomal      0
#define Margin1    1
#define OffMargin  2

extern u8 MC32P5232_Program();
extern u8 A500_MODEL_IN(void);
extern u8 A650_MODE_IN();
extern u8 MCA330_Bus_Write(u32 Data ,u8 BitSize ,u8 BitCheckEnable);
extern u8 MCA330_OTP_ReadByte(u16 OTPAddr,u16 *TargetRegister);
extern u8 MCA330_OTP_R(u16 *TargetRegister);
extern u8 MCA330_Bus_Read(u16 *TargetRegister ,u8 BitSize);
extern u8 CheckProductID(u16 addr,u16 data);
extern u8 MCA650_SendProgAddr(u16 Addr);
extern u8 MCA330_SendProgData(u16 Data);
extern u8 MCA330_ProgAddrAdd(u16 NextData);  
extern u8 MCA330_Bus_Program(u16 Data ,u16 GapTime ,u8 BitCheckEnable );
extern u8 MCA330_ProgByte(u16 Addr,u16 Data);
extern u8 MCA330_ReadByte_DiffMode(u8 ModeNum,u16 Addr,u16 *TargetRegister);
extern u8 MCA330_Engineer_prog(void);
extern u8 MCA330_ReadRomDataAddrAdd(u16 *TargetRegister);
extern u8 MC32P5232_MTPChipErase(u16 StaAddress);
extern u8 LoadMTPdata(u16 data);
extern u8 MC32P5232_ReadMTP_Nomal(u16 address);
extern u8 MC32P5232_ReadMTP_Margin(u16 address,u16 mode_Margin,u16 mode_MTP);
extern u8 MC32P5232_MTPWritePage(u16 StaAddress,u8 num,u8 wData);
extern u8 ProgramTiming(u32 Tprog);
extern void SendData_Bits(u8 BitNum,u16 Data);
extern void MC32P5232_Instruct(u16 OTPData);
extern void MC32P5232_InputData(u16 OTPData);
extern u16 MC32P5232_R(u16 read_mode);
extern void MC32P5232_SetAddress(u16 OTPAddr);
extern void MC32P5232_SetPTMmode(u16 mode);
u8  CheckLoadRegisterSuccess(u16 Reg_addr,u8 opt_size);
u8  LoadOptionToTestModeRegisterValue_H_L();
u8 MC32P5232IRC_TEST(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
u8 MC32P5232IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
void MC32P5232FT_11Bit(u16 osccal,u16 tadj);
void MC32P5232FT(u16 osccal,u16 tadj);
void MC32P5232FT1(u16 osccal,u16 tadj);
u8 MCA330_getHardwareCRC(u8 ModeNum,u16 otpsize,u16 crcinit, u16 *TargetRegister);
u8 MCA330_ChangeOption(u16 opAddr,u16 opData ,u16 opDataMask);
u8 MCA330_progData_R(u16 *TargetRegister);
u8 MCA330_ReadCheckRom(u8 ReadMode);
//u8 MCA330_ReadFindRomBad(u8 ReadMode);
u8 MCA650_ReadFindRomBad(u8 ReadMode);
u8 MC32P5232IRC_CHECK(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
u16 CountCRC(u8 data1,u8 data2,u16 crc);
u8 MCA650_ProgAndAddrAdd(u16 Wdata);
#endif	