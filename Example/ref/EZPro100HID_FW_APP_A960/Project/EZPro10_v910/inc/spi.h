/*
 * @Copyright: Shanghai Sinomcu Microelectronics Co.,Ltd.
 * @Author: Mike.Mo
 * @Emain: mgf@sinomcu.com
 * @Date: 2019-06-25 11:07:18
 * @Encoding: GB2312
 * @Description: 
 */


#ifndef spi_h
#define spi_h

typedef struct Storage Storage;


//sst25vf010
#define DeviceID 0x49
#define ManuID   0xbf

#define Addr_Flash_ROMStart         0x010000
#define Addr_Flash_MTPStart         0x018000    //Buffer for eeprom
#define Addr_Flash_OptionStart      0x01e000
#define Addr_Flash_CfgStart         0x01f000
#define Addr_Flash_RollStart        0x01F800
#define Addr_Flash_CRC_value        0x01F900 //PC ´«¸øÉÕÐ´Æ÷
#define Addr_Flash_CRC_OPTION       0x01f904    //option crc-value 0xf904,05
#define Addr_Flash_ROM_OR_EEPROM    0x01F906    //f906,07
#define Addr_Flash_DATA_SIZE_EEPROM 0x01F908
#define Addr_Flash_CRC_EEPROM       0x01F90a    //eeprom data crc value


#define Addr_Flash_Option   0x012100
#define Addr_Flash_OptionProtec 0x012120
#define Addr_Flash_Option8k   0x014100 //2012.02.27
#define Addr_Flash_OptionProtec8k 0x014120
#define addr_Flash_OptionProtec16K 0x018120
#define Addr_Flash_MTPStart 0x018000//

#define Addr_Flash_Config	0x017d00
#define Addr_Flash_MCU_ID	0x017d01
#define Addr_Flash_CfgProgramID 0x017d03
#define Addr_Flash_CfgFirADD0  0x017d04
#define Addr_Flash_CfgFirADD1	0x017d05
#define Addr_Flash_CfgFirADD2  0x017d06
#define Addr_Flash_CfgFirADD3	0x017d07
#define Addr_Flash_MaxFreH   0x017d0e
#define Addr_Flash_MaxFreL   0x017d0f
#define Addr_Flash_MiniFreH  0x017d10
#define Addr_Flash_MiniFreL  0x017d11

#define Addr_Flash_CfgFirADD0_16k   0x018d04
#define Addr_Flash_CfgFirADD1_16k	0x018d05
#define Addr_Flash_CfgFirADD2_16k   0x018d06
#define Addr_Flash_CfgFirADD3_16k	0x018d07


#define ReadCMD  0x03
#define SectorEraseCMD 0x20
#define BlockEraseCMD  0x52
#define ChipEraseCMD   0x60
#define ByteProgramCMD 0x02  
#define AAIProgramCMD  0xaf  //Auto Address Increment program
#define RDSRCMD        0x05 //Read-Status-Register
#define EWSRCMD        0x50 //Enable Write Status Register
#define WRSRCMD        0x01 //Write Status Register
#define WRENCMD        0x06 //Write-Enable
#define WRDICMD        0x04 //Write disable
#define ReadIDCMD      0x90  //read ID

#define BP_MemoryArray0   0x00 //Protected Memory Area None
#define BP_MemoryArray1   0x01 // 1/4 Protected 0x018000-0x01ffff
#define BP_MemoryArray2   0x02 // 1/2 0x010000-0x01ffff
#define BP_MemoryArray3   0x03 // full 0x000000-0x01ffff


#define MCP42050_CS_0  GPIO_ResetBits(GPIOB,GPIO_Pin_11)
#define MCP42050_CS_1  GPIO_SetBits(GPIOB,GPIO_Pin_11)
#define FM_CS_0        GPIO_ResetBits(GPIOB,GPIO_Pin_12)
#define FM_CS_1        GPIO_SetBits(GPIOB,GPIO_Pin_12)

//MCP42050
#define ADJ_VDD  0x12 //PW1
#define ADJ_VPP  0x11 //PW0

extern u8 Rxdata;
Storage *A960_FlashStorage(void);

extern void SPI2Init( );
extern u8 SPI_WriteByte(u8 TxData);
extern u8 SPI_ReadByte();

extern void MCP42050_ADJ(u8 P0tX,u8 Rvalue);

extern void FMWriteOne(u32 FMAddr,u8 FMData);
extern u8 FMReadOne(u32 FMAddr);
extern void	FMChipErase();
extern u8 FMReadStatus();
extern void  FMWriteStatus(u8 BPdata);
extern u8 FMReadID();
extern u8 FMReadMore(u32 FirstAddr);
extern u16 CalculateCheckSum();
extern u16 CalculateOptionCheckSum_7343();
u16 CalculateCRC(u8 mask);
u16 CalculateOptionCRC(u8 mask);

u16 CalculateOptionCRCStruct(u8 mask);
u16 CalculateCRCStruct(u8 mask);

#endif