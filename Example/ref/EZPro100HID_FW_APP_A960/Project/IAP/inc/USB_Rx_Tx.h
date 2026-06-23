
#ifndef USB_Rx_Tx_h
#define USB_Rx_Tx_h

//debug command
#define  RUN		0xE0
#define  STEPIN		0XE1
#define  STOP		0XE2
#define  RESET		0XE3
#define  STEPOVER	0XE4
#define  STEPOUT	0XE5
#define  SET_ADDRL	0xA0
#define  SET_ADDRH	0xA1
#define  READdata	0xD0 //addr inc when read opration after
#define  WRITEdata	0xD1 //addr inc when write opration before


extern u8 const ProgrammingFinished[5];
extern u8 const BlankOk[5];
extern u8 const NextData[5];
extern u8 const Reset[5];
extern u8 const MotherChipMade[5];
extern u8 const Reconfigured[5];
extern u8 const EraseFlash[5];
extern u8 const ErrorsCS[6];
extern u8 const ModelIn[5];

extern void USB_Rx_Tx();
extern void USB_SendData(u8 Length);

#endif