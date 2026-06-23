
#ifndef MTPRW_H
#define MTPRW_H

#define CRC_4_POLYNOMIALS       0x03

#define BIT_DELAY_1US  1

#define CMD_DEBUG_SET_ADDRHH    0x22
#define CMD_DEBUG_SET_ADDRMM    0x21
#define CMD_DEBUG_SET_ADDRLL    0x20
#define CMD_DEBUG_READ_DATA     0x10
#define CMD_DEBUG_WRITE_DATA    0x11
#define CMD_DEBGU_READ_STATUS   0x23

#define CMD_PROG_READ_CURENT_ADDR   0x12
#define CMD_PROG_READ_CURENT_DATA   0x13
#define CMD_PROG_SET_ADDR           0x15
#define CMD_PROG_WRITE_DATA         0x16
#define CMD_PROG_READ_DATA          0X17
#define CMD_PROG_CAL_CRC            0X1b
#define CMD_PROG_TEST_MODE          0x1a
#define CMD_PROG_ENABLE             0x19

#define RW_TYPE_STATUS          0x00
#define RW_TYPE_ADDR            0X01
#define RW_TYPE_DATA            0X02
#define RW_TYPE_CTL             0X03

#define CHIP_WAIT_MODE               8
#define CHIP_PROG_MODE               4
#define CHIP_DEBUG_MODE              2
#define CHIP_TEST_MODE               1

#define READ_SDIO_DATA      GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_0)
#define OTP_SDIO_0          GPIO_ResetBits(GPIOC,GPIO_Pin_0)
#define OTP_SDIO_1          GPIO_SetBits(GPIOC,GPIO_Pin_0)

#define DP_DIR_1_A_to_B GPIO_WriteBit(GPIOC, GPIO_Pin_8,Bit_SET)
#define DP_DIR_1_B_to_A GPIO_WriteBit(GPIOC, GPIO_Pin_8,Bit_RESET)



extern void SDIO_Turnto_Input(void);
extern void SDIO_Turnto_Output(void);
extern u8 MTP_MODE_IN();
extern void SendByte(u8 sdata);
extern void SendWord(u16 sdata);
extern u16  ReadWord();
extern void LineReset(void);
extern u8 MTP_Start();
extern u8 MTP_Start_mode();//panqian新增20230720
extern void MTP_ACK();
extern void MTP_NACK();
extern void MTP_TYPE_RW(u8 type_data,u8 rw_flag);
extern u8 MTP_STOP();
extern void TEST_STATUS_dig_signal();

extern void MTP_Write_Word(u16 buf_data,u8 type_data);
extern u16 MTP_Read_Word(u8 type_data);
extern u8 MTP_Read_Word_mode(u8 type_data);//panqian新增20230720
extern void MTP_Write_Word_16Byte(u16 addr_start,u16 *data_buf);
extern void SendWord_Erase(u8 Terase0,u16 Terase1,u8 Terase2);
extern void MTP_Erase_Prog(u8 delay0,u16 delay1,u8 delay2);


#endif