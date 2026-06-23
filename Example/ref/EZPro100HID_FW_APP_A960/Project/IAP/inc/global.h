
#ifndef global_h
#define golbal_h

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#define VIRTUAL_COM_PORT_DATA_SIZE              64
#define IIC_DATA_BUFF_SIZE			23

#define CP_0    GPIO_ResetBits(GPIOC,GPIO_Pin_6)
#define CP_1    GPIO_SetBits(GPIOC,GPIO_Pin_6) 
#define OTP_SCK_0    GPIO_ResetBits(GPIOD,GPIO_Pin_11)
#define OTP_SCK_1    GPIO_SetBits(GPIOD,GPIO_Pin_11)

#define OTP_SDI_0    GPIO_ResetBits(GPIOD,GPIO_Pin_12)
#define OTP_SDI_1    GPIO_SetBits(GPIOD,GPIO_Pin_12)

#define OTP_DISA_0    GPIO_ResetBits(GPIOB,GPIO_Pin_7)
#define OTP_DISA_1    GPIO_SetBits(GPIOB,GPIO_Pin_7)
#define OTP_DISB_0    GPIO_ResetBits(GPIOB,GPIO_Pin_8)
#define OTP_DISB_1    GPIO_SetBits(GPIOB,GPIO_Pin_8)
#define OTP_DISC_0    GPIO_ResetBits(GPIOB,GPIO_Pin_9)
#define OTP_DISC_1    GPIO_SetBits(GPIOB,GPIO_Pin_9)
#define OTP_DISD_0    GPIO_ResetBits(GPIOB,GPIO_Pin_10)
#define OTP_DISD_1    GPIO_SetBits(GPIOB,GPIO_Pin_10)


#define OTP_SDO_PORT	GPIOE	
#define OTP_SDO_PIN		GPIO_Pin_4

//#define OTP_BUSY_PORT	GPIOE	
//#define OTP_BUSY_PIN	GPIO_Pin_1

#define OTP_FT_PORT		GPIOE
#define OTP_FT_PIN		GPIO_Pin_0

#define OTP_CP_PORT		GPIOC
#define OTP_CP_PIN		GPIO_Pin_6

#define OTP_GIME_REF_PORT	GPIOC
#define OTP_TIME_REF_PIN	GPIO_Pin_7


extern u16 USB_RxDataCS;
extern u8  WR_Command; 
extern u8  USB_RxCommand;


extern u8 USB_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
extern u8 USB_Rx_Flag;
extern u8 StateFlag;


extern u32 FlashDestination ;
extern u32 PageSize ;
extern u32 EraseCounter;
extern u32 NbrOfPage;
extern FLASH_Status FLASHStatus;
extern u32 RamSource;

#define LEDOK_Off GPIO_SetBits(GPIOC,GPIO_Pin_10)
#define LEDOK_On  GPIO_ResetBits(GPIOC,GPIO_Pin_10)

#define LEDNG_Off GPIO_SetBits(GPIOC,GPIO_Pin_11)
#define LEDNG_On  GPIO_ResetBits(GPIOC,GPIO_Pin_11)

#define LEDBUSY_Off GPIO_SetBits(GPIOC,GPIO_Pin_12)
#define LEDBUSY_On  GPIO_ResetBits(GPIOC,GPIO_Pin_12)

#define USB_REG_On   GPIO_SetBits(GPIOB,GPIO_Pin_1)
#define USB_REG_Off  GPIO_ResetBits(GPIOB,GPIO_Pin_1)

#define FlagRead  GPIOD,GPIO_Pin_7
#define FlagProtect  GPIOD,GPIO_Pin_6
#define FlagBlank  GPIOD,GPIO_Pin_5
#define FlagVerify GPIOD,GPIO_Pin_4
#define FlagWrite  GPIOD,GPIO_Pin_3

#define KEY  GPIOD,GPIO_Pin_10


extern void Writer_Test(); //test.c

extern void RCC_Configuration(void);
extern void GPIO_Configuration(void);
extern void Freq(u8 Numb);
extern void Delay_100us(u16 cnt );
extern void Delay_1ms(u16 cnt );
extern void DeviceConfig();

#endif