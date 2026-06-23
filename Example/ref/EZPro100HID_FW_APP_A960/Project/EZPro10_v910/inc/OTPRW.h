
#ifndef OTPRW_H
#define OTPRW_H

#define ID_M10P02R	0x0353
#define ID_M20P02	0x0202
#define ID_M224		0x0224
#define ID_M208		0x0228

#define M10P02R_ProtectBit 0x7f;

#define Addr_OTP_Option 0x0000
#define Addr_OTP_ROMStart 0x1000
#define Addr_OTP_ROMEnd   0x1fff

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

#define DP_DIR_1_A_to_B GPIO_WriteBit(GPIOC, GPIO_Pin_8,Bit_SET)
#define DP_DIR_1_B_to_A GPIO_WriteBit(GPIOC, GPIO_Pin_8,Bit_RESET)

#define OTP_SDO_PORT	GPIOE	
#define OTP_SDO_PIN		GPIO_Pin_4

//#define OTP_BUSY_PORT	GPIOE	
//#define OTP_BUSY_PIN	GPIO_Pin_1

#define OTP_FT_PORT		GPIOE
#define OTP_FT_PIN		GPIO_Pin_0

#define OTP_CP_PORT		GPIOC
#define OTP_CP_PIN		GPIO_Pin_6

#define OTP_TIME_REF_PORT	GPIOC
#define OTP_TIME_REF_PIN	GPIO_Pin_7


#define NOP	 for (temp=0;temp<12;temp++) //value=6
#define NOP_5us	 for (temp=0;temp<42;temp++)

extern const u16 ProgTime; 

extern void SDIO_Turnto_Input(void);
extern void SDIO_Turnto_Ooutput(void);
extern void OTPInit(void);
extern u8 OTP_ReadByte(u16 OTPAddr);
extern u8 OTP_Model_s(u16 OTPAddr);
extern void OTP_WriteByte(u16 OTPAddr,u8 OTPData);
extern u8 OTP_Mode_IN(u16 OTPID,u8 vpp,u8 vdd);

extern u8 OTP_Verify( );
extern u8 OTP_Blank(u16 ROMFirstAddr,u16 ROMEndAddr,u16 OPTIONFirstAddr);
extern u8 OTP_DownLoad(u16 MCU_ID,u16 MCU_FirAdd);

extern void OTP_Test();

#endif