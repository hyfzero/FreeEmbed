
#ifndef USB_Rx_Tx_h
#define USB_Rx_Tx_h


extern u8 const ProgrammingFinished[5];
extern u8 const BlankOk[5];
extern u8 const NextData[5];
extern u8 const Reset[5];
//extern u8 const MotherChipMade[5];
extern u8 const Reconfigured[5];
extern u8 const EraseFlash[5];
extern u8 const ErrorsCS[6];
extern u8 const ModelIn[5];

//extern u8 const ReadVison[7];

extern void USB_Rx_Tx();
extern void USB_SendData(u8 Length);

#endif