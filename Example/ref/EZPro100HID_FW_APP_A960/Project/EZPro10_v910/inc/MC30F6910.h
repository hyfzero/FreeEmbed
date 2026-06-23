/*
 * @Copyright: Shanghai Sinomcu Microelectronics Co.,Ltd.
 * @Author: Mike.Mo
 * @Emain: mgf@sinomcu.com
 * @Date: 2019-06-25 11:07:18
 * @Encoding: GB2312
 * @Description: 
 */


#ifndef _mc30f6910_h
#define _mc30f6910_h

#define     EXIT_TO_WAIT          0

//idel-clt reg
#define     RELOAD_OPTION_MODE      1
#define     PROG_MODE               2
#define     DEBUG_MODE              3
#define     TEST_MODE               4
#define     FLASH_BIST_MODE         5
#define     EEPROM_BIST_MODE        6


#define     FSR0     0X184
#define     FSR1     0X185
#define     INDF2    0X182
#define     PFRCCR   0X1C0

//PROG-CLT REG
#define     CHIP_EO         (0x0001<<10)    //奇偶片选
#define     SECTOR          (0X0001<<9)     //块选择
#define     CHIP_SELECT     (0X0001<<8)     //片选择
#define     PROG_CMD_EXIT   0X0000      //退出编程模式
#define     PROG_CMD_CLRPL  0X0001      //清HVPL
#define     PROG_CMD_ERASE  0X0002      //擦除模式
#define     PROG_CMD_PROG   0X0003      //烧写模式
#define     PROG_CMD_LPRD   0X0004      //低功耗读模式

#define     EN_UNLOCK       (0x0001<<11)    //加密页可擦
#define     LOCKED          (0X0001<<10)    //加密
#define     CFG_EN          (0X0001<<9)     //配置字寄存器选中
#define     PINFO_EN        (0X0001<<8)     //coptbit页选中
#define     DINFO_EN        (0X0001<<7)     //doptbit选中
#define     UINFO_EN        (0X0001<<6)     //uoptbit选中
#define     EEPROM_EN       (0X0001<<5)     //eeprom选中
#define     MAIN_EN         (0X0001<<4)     //main 选中
#define     LPRD_EN         (0X0001<<3)     //LPRD有效
#define     CLPL_EN         (0X0001<<2)     //CLRPL有效
#define     PROG_EN         (0X0001<<1)     //PROG有效
#define     ERASE_EN        (0X0001<<0)     //ERASE有效

//debug0-ctl reg
#define     DEBUG_EXIT      0
#define     DEBUG_RESET     1
#define     DEBUG_ABORT     2
#define     DEBUG_RUN       3
#define     DEBUG_STEP_IN   4
#define     DEBUG_STEP_OVER 5
#define     DEBUG_STEP_OUT  6
#define     DEBUG_CRC       7   //校验
#define     DEBUG_CRC_ENCR  8   //加密校验


#define     PROG_TIME0      15
#define     PROG_TIME1      200
#define     PROG_TIME2      150
#define     ERASE_TIME      400


extern u8 mc30f6910_program();
extern u8 erase_chip();
extern u8 erase_eeprom();
extern u8 erase_user_option();
extern u8 erase_rom();
extern u8 write_eeprom();
extern u8 hirc_adj(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 hirc_adj_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 PFRC_adj(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);
extern u8 PFRC_adj_check(u16 IRC_FreqMin,u16 IRC_FreqMax,u16 IRC_FreqType);

u8 CheckOptionCRC(void);
u8 CheckRomCRC(void);
#endif