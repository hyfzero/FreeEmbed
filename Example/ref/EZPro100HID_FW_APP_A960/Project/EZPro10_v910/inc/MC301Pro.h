
#ifndef MC301Pro_H
#define MC301Pro_H

//指令码
#define PDataRe 0x13
#define PAddrRe 0x12

#define SetAddr	0x15
#define DataWr	0x16
#define DataRe	0x17
#define AddrINC	0x14
#define ProStr	0x19
#define ProEnd	0x1a
#define TestMod	0x0a
#define Erase   0x1b
#define SetReadMode 0x1a
#define SetOpt  0x1c 
#define CheMode 0x18

//MTP模式
#define MTPwrite   0x1b//指令码：MTP烧写模式
#define MTPLoad    0x1d
#define ByteRead   0x00//Byte Read
#define ReadAndINC 0x27
#define ByteWrite  0x0a//Byte Write
#define PageWrite  0x02
#define ChipWrite  0x03
#define PageErase  0x12
#define ChipErase  0x13
#define ChipErase_A500  0x0b
#define ProMargin_PTM  0x01
#define EraMargin_PTM  0x04
#define ProMargin_PDIN  0x50
#define EraMargin_PDIN  0x04
#define ProMargin1_PDIN 0xb0
#define ProMargin2_PDIN 0x58
#define ProMargin3_PDIN 0x30
#define EraMargin1_PDIN 0x0a
#define EraMargin2_PDIN 0x02
#define EraMargin3_PDIN 0x04

extern u8 MC301_Program();
extern u16 MC301_R( );
extern void MC301_W(u16 OTPData);
extern u16 MC301_ReadByte(u16 OTPAddr);
extern void MC301_Instruct(u16 OTPDdata);
extern void MC301_WriteByte(u16 OTPAddr,u16 OTPData);
extern u8 M301IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 M301IRC_ModeInCheck(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern void MC301FTtest(u8 IRC_Opiton);
extern void MC301_IRC_INST(u16 OTPData);

//extern u16 MC301_Read();
//extern u16 MC301_ReadS();
//extern void MC301_WriteS(u16 OTPData);

#endif