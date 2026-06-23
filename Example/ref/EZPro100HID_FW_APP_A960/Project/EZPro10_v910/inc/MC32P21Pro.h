
#ifndef MC32P21Pro_H
#define MC32P21Pro_H

//commands were define in mc30p01pro.h file

extern u8 MC32P7311_Program();
//extern u8 MC32P7511_Program();
extern u8 MC321_Program();
extern u16 MC321_PreRead(u8 command);
extern u16 MC321_R( );
extern void MC321_W(u16 OTPData);
extern u16 MC321_ReadByte(u16 OTPAddr);
extern void MC321_Instruct(u16 OTPDdata);
extern void MC321_WriteByte(u16 OTPAddr,u16 OTPData);
extern u8 M321IRC(u16 IRC_FreMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 M321IRC_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 osccal);
extern u8 M321IRC_11Bit(u16 IRC_FreMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 M321IRC_11Bit_last(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 M321IRC_11Bit_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 osccal);
extern void MC321_IRC_INST(u16 OTPData);
extern void MC321FTtest(u8 irc_adj_value);
extern void MC321FTtest_11Bit(u16 irc_adj_value);
extern void MC321_MODEL_IN(u8 VddFlag);
extern void MC32_MODEL_IN(u8 VddFlag,u8 mode,u16 Tdly);

extern u8 MC32P21_Program();
extern u8 MC32P21_Program_New();
extern u8 MC3081IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 MC3081IRC_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u8 osccal,u8 tempadj);

//extern u8 M9029IRC(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
//extern u8 M9029IRC_last(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
//extern void	  M9029FTtest(u16 irc_adj_value,u8 tempadj);

extern u8 M31P11IRC_11Bit(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
//extern void MC31P11FTtest_11Bit(u16 irc_adj_value);
extern void MC31P11FTtest_11Bit(u16 irc_adj_value,u8 TempADJ);
extern u8 MC32P8132_Program();
extern u8 MC32P5312IRC_11Bit(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern void MC32P5312FTtest_11Bit(u16 irc_adj_value,u8 TempADJ);
extern u8 MC32P5312_Program();

extern u16 MC321_R_DiffMode(u8 ModeNum);
extern void MC321_SetRmode(u8 ReadMode);
extern u16 MC321_ReadByte_DiffMode(u8 ModeNum,u16 OTPAddr);
extern void SetOption(u16 OTPAddr,u16 OTPData);
extern void InputData(u16 OTPData);
extern void SetAddress(u16 OTPAddr);

#endif	