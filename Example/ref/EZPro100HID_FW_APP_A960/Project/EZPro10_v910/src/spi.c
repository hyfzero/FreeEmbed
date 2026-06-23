
#include "stm32f10x.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"
#include "IRC.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "Config.h"
#include "MC32P5232Pro.h"
#include "storage.h"
#include "spi_transport.h"

u8 Rxdata;

#define A960_FLASH_CAPACITY_BYTES   0x00020000UL
#define A960_FLASH_PAGE_SIZE_BYTES  1UL
#define A960_SPI_TIMEOUT_MS         1UL

static Storage a960_flash_storage;
static u8 a960_flash_constructed;

static StorageStatus a960_flash_init(Storage *storage);
static StorageStatus a960_flash_read(Storage *storage, uint64_t address, uint8_t *data, size_t length);
static StorageStatus a960_flash_program(Storage *storage, uint64_t address, const uint8_t *data, size_t length);
static StorageStatus a960_flash_erase(Storage *storage, uint64_t address, uint64_t length);
static HwStatus a960_spi_transfer(void *ctx, const SpiDevice *device, const SpiSegment *segments, size_t segment_count, uint32_t timeout_ms);

static const StorageOps a960_flash_ops =
{
	a960_flash_init,
	a960_flash_read,
	a960_flash_program,
	a960_flash_erase
};

static const SpiBusOps a960_spi_bus_ops =
{
	a960_spi_transfer
};

static SpiBus a960_spi_bus =
{
	&a960_spi_bus_ops,
	0
};

static SpiDevice a960_spi_device =
{
	&a960_spi_bus,
	0,
	0,
	3
};

static StorageStatus a960_flash_construct(void)
{
	if (a960_flash_constructed == 0)
	{
		storage_base_construct(&a960_flash_storage, "a960_sst25vf010a", &a960_flash_ops);
		a960_flash_constructed = 1;
	}

	if (a960_flash_storage.initialized == 0)
	{
		return storage_init(&a960_flash_storage);
	}

	return STORAGE_OK;
}

Storage *A960_FlashStorage(void)
{
	(void)a960_flash_construct();
	return &a960_flash_storage;
}

static HwStatus a960_spi_transfer_byte(u8 tx_data, u8 *rx_data)
{
	u16 count = 0;
	u16 read_data;

	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
	{
		if(++count > 60000) return HW_STATUS_TIMEOUT;
	}

	SPI_I2S_SendData(SPI2, tx_data);

	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
	{
		if(++count > 60000) return HW_STATUS_TIMEOUT;
	}

	read_data = SPI_I2S_ReceiveData(SPI2);
	if (rx_data != 0)
	{
		*rx_data = (u8)read_data;
	}

	return HW_STATUS_OK;
}

static HwStatus a960_spi_transfer(void *ctx, const SpiDevice *device, const SpiSegment *segments, size_t segment_count, uint32_t timeout_ms)
{
	size_t segment_index;
	size_t byte_index;
	HwStatus status;
	u8 tx_data;
	u8 rx_data;

	(void)ctx;
	(void)device;
	(void)timeout_ms;

	for (segment_index = 0; segment_index < segment_count; segment_index++)
	{
		for (byte_index = 0; byte_index < segments[segment_index].length; byte_index++)
		{
			if (segments[segment_index].tx_data != 0)
			{
				tx_data = segments[segment_index].tx_data[byte_index];
			}
			else
			{
				tx_data = segments[segment_index].fill_byte;
			}

			status = a960_spi_transfer_byte(tx_data, &rx_data);
			if (status != HW_STATUS_OK)
			{
				return status;
			}

			if (segments[segment_index].rx_data != 0)
			{
				segments[segment_index].rx_data[byte_index] = rx_data;
			}
		}
	}

	return HW_STATUS_OK;
}

static StorageStatus a960_flash_init(Storage *storage)
{
	storage->info.capacity_bytes = A960_FLASH_CAPACITY_BYTES;
	storage->info.page_size_bytes = A960_FLASH_PAGE_SIZE_BYTES;
	storage->info.erase_size_bytes = A960_FLASH_CAPACITY_BYTES;
	storage->info.capabilities = STORAGE_CAP_READ | STORAGE_CAP_PROGRAM | STORAGE_CAP_ERASE | STORAGE_CAP_REQUIRES_ERASE;
	storage->info.erased_value = 0xff;

	return STORAGE_OK;
}

static u8 a960_flash_write_one_legacy(u32 FMAddr, u8 FMData)
{
	u16 i = 0;

	FM_CS_0;
	if (SPI_WriteByte(WRENCMD) == 0)
	{
		FM_CS_1;
		return 0;
	}
	FM_CS_1;
	while(i<200) i++;

	FM_CS_0;
	if (SPI_WriteByte(ByteProgramCMD) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr >> 16)) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr>>8)) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr & 0x0000ff)) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte(FMData) == 0)
	{
		FM_CS_1;
		return 0;
	}
	FM_CS_1;

	return 1;
}

static u8 a960_flash_read_legacy(u32 FMAddr, u8 *data, size_t length)
{
	size_t index;

	FM_CS_0;
	if (SPI_WriteByte(ReadCMD) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr >> 16)) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr>>8)) == 0)
	{
		FM_CS_1;
		return 0;
	}
	if (SPI_WriteByte((u8)(FMAddr & 0x0000ff)) == 0)
	{
		FM_CS_1;
		return 0;
	}

	for (index = 0; index < length; index++)
	{
		if (SPI_ReadByte() == 0)
		{
			FM_CS_1;
			return 0;
		}
		data[index] = Rxdata;
	}

	FM_CS_1;
	return 1;
}

static u8 a960_flash_chip_erase_legacy(void)
{
	u8 i = 0;

	FM_CS_0;
	if (SPI_WriteByte(WRENCMD) == 0)
	{
		FM_CS_1;
		return 0;
	}
	FM_CS_1;
	while(i<200) i++;
	i=0;

	FM_CS_0;
	if (SPI_WriteByte(ChipEraseCMD) == 0)
	{
		FM_CS_1;
		return 0;
	}
	while(i<200) i++;
	FM_CS_1;
	while((FMReadStatus()&& 0x01)==1) ;

	return 1;
}

static StorageStatus a960_flash_read(Storage *storage, uint64_t address, uint8_t *data, size_t length)
{
	(void)storage;

	if (length == 0)
	{
		return STORAGE_OK;
	}

	return (a960_flash_read_legacy((u32)address, data, length) == 1) ? STORAGE_OK : STORAGE_TIMEOUT;
}

static StorageStatus a960_flash_program(Storage *storage, uint64_t address, const uint8_t *data, size_t length)
{
	size_t index;

	(void)storage;

	for (index = 0; index < length; index++)
	{
		if (a960_flash_write_one_legacy((u32)(address + index), data[index]) == 0)
		{
			return STORAGE_TIMEOUT;
		}
	}

	return STORAGE_OK;
}

static StorageStatus a960_flash_erase(Storage *storage, uint64_t address, uint64_t length)
{
	(void)storage;

	if ((address != 0) || (length != A960_FLASH_CAPACITY_BYTES))
	{
		return STORAGE_UNSUPPORTED;
	}

	return (a960_flash_chip_erase_legacy() == 1) ? STORAGE_OK : STORAGE_TIMEOUT;
}
//u8 EEORE;

void SPI2Init(void)
{
	SPI_InitTypeDef   SPI_InitSructure;
	GPIO_InitTypeDef  GPIO_IniStructure;

	//initial SPI CLK
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);

	//define pin for spi2 as AF
	GPIO_IniStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 ;
	GPIO_IniStructure.GPIO_Speed =GPIO_Speed_50MHz;
	GPIO_IniStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_IniStructure);

	GPIO_IniStructure.GPIO_Pin = GPIO_Pin_12; // pin_11-->MCP42050_CS
	GPIO_IniStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_IniStructure.GPIO_Mode =GPIO_Mode_Out_OD; // FLASH CE PIN
	GPIO_Init(GPIOB,&GPIO_IniStructure);
    FM_CS_1;
	GPIO_IniStructure.GPIO_Pin = GPIO_Pin_11; // pin_11-->MCP42050_CS
	GPIO_IniStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_IniStructure.GPIO_Mode =GPIO_Mode_Out_PP; // FLASH CE PIN
	GPIO_Init(GPIOB,&GPIO_IniStructure);

	MCP42050_CS_1;
	//config spi2
	SPI_InitSructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_InitSructure.SPI_CPHA = SPI_CPHA_2Edge; //CLK = 1 when idel mode
	SPI_InitSructure.SPI_CPOL = SPI_CPOL_High;  // sample data when push
	SPI_InitSructure.SPI_CRCPolynomial = 7; //?
	SPI_InitSructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitSructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitSructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitSructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitSructure.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI2,&SPI_InitSructure);
	SPI_Cmd(SPI2,ENABLE);
	(void)a960_flash_construct();

}


// Transfer one byte through the SPI abstraction while preserving legacy semantics.
u8 SPI_WriteByte(u8 TxData)
{
	SpiSegment segment;

	segment.tx_data = &TxData;
	segment.rx_data = 0;
	segment.length = 1;
	segment.fill_byte = 0xaa;

	return (spi_transfer(&a960_spi_device, &segment, 1, A960_SPI_TIMEOUT_MS) == HW_STATUS_OK) ? 1 : 0;
}

// Read one byte with the legacy 0xaa fill byte and update Rxdata.
u8 SPI_ReadByte( )
{
	SpiSegment segment;
	u8 read_data = 0;

	segment.tx_data = 0;
	segment.rx_data = &read_data;
	segment.length = 1;
	segment.fill_byte = 0xaa;

	if (spi_transfer(&a960_spi_device, &segment, 1, A960_SPI_TIMEOUT_MS) != HW_STATUS_OK)
	{
		return 0;
	}

	Rxdata = read_data;
	return 1;
}

/************************************************
//MCP42050 ADJUST VDD
//
//
**************************************************/
void MCP42050_ADJ(u8 PotX,u8 Rvalue)
{
	MCP42050_CS_0;
	SPI_WriteByte(PotX); //Potentiometer 1=0x12; Potentiometer 2=0x11
	SPI_WriteByte(Rvalue);
	MCP42050_CS_1;
	Delay_100us(1);
}


/************************************************************
//FuntionName:
//Input		 :
//Output     :
//
***********************************************************/
void FMWriteOne(u32 FMAddr,u8 FMData)
{
	(void)storage_program(A960_FlashStorage(), FMAddr, &FMData, 1);
}

u8 FMReadOne(u32 FMAddr)
{
  u8 temp	=0;
  u8 data = 0;

  if (storage_read(A960_FlashStorage(), FMAddr, &data, 1) == STORAGE_OK)
  {
    Rxdata = data;
    temp =1;
  }
  else
  {
    temp= 0;
  }
  return temp;
  // data output stream is continuous through all addresses 
  // until CE# from low to high
}


void	FMChipErase()
{
	(void)storage_erase(A960_FlashStorage(), 0, A960_FLASH_CAPACITY_BYTES);
}

u8 FMReadStatus()
{	
	u8 xdata;

	FM_CS_0;
	SPI_WriteByte(RDSRCMD);
	if (SPI_ReadByte()==1)
	{
		xdata = Rxdata;
	}
	
	FM_CS_1;
	return xdata;

}

void FMWriteStatus(u8 BPdata )
{
	u8 i=0;
	FM_CS_0;
	SPI_WriteByte(EWSRCMD);
	FM_CS_1;
	while(i<200) i++;
	FM_CS_0;
	SPI_WriteByte(WRSRCMD);
	SPI_WriteByte( BPdata ); //clear BP0,BP1
	FM_CS_1;

}

u8 FMReadID()
{
	u8 mfid=0;
	u8 dvid=0;

	FM_CS_0;
	SPI_WriteByte(ReadIDCMD);
	SPI_WriteByte(0x00);
	SPI_WriteByte(0x00);
	SPI_WriteByte(0x00);
	//mfid=SPI_ReadByte();
	//dvid=SPI_ReadByte();
	if (SPI_ReadByte() ==1)
	{
		mfid=Rxdata;
	}
	if (SPI_ReadByte()==1)
	{
		dvid=Rxdata;
	}
	FM_CS_1;
        if(mfid == 0xbf)
          if(dvid == 0x49)
            return 1;
          else
            return 0;
         else 
           return  0;
}



//Raed more bytes 
u8 FMReadMore(u32 FirstAddr)
{
	u8 temp	=0;

	SPI_WriteByte(ReadCMD);
	SPI_WriteByte((u8)(FirstAddr >> 16));
	SPI_WriteByte((u8)(FirstAddr>>8));
	SPI_WriteByte((u8)(FirstAddr & 0x0000ff));

	if (SPI_ReadByte()==1)
	{
		temp =1;
	}
	else
	{
		temp= 0;
	}
	return temp;
}

u16 CalculateCheckSum()
{
	u16 checksum=0;
//	u16 addr0;
//	u16 addr1;

	if (MCU_Config_xx.WriteConfig_xx.RomEndAddr==0x0000)
	{
		return 0;
	}

	FM_CS_0;
	FMReadMore(Addr_Flash_ROMStart+MCU_Config_xx.WriteConfig_xx.RomFirAddr-1);
	for(OTP_ADDR=MCU_Config_xx.WriteConfig_xx.RomFirAddr;OTP_ADDR<=MCU_Config_xx.WriteConfig_xx.RomEndAddr;OTP_ADDR++)
	{
		SPI_ReadByte();
		checksum +=Rxdata;			
	}
	FM_CS_1;

	return checksum;
}

//计算MTP的OPTION ，且只计算双数地址，奇数不计算
u16 CalculateOptionCheckSum_7343()
{
	u16 checksum=0; 
//	u16 addr0;
//	u16 addr1;

	if (MCU_Config_xx.WriteConfig_xx.OptionSize >100)
	{
		return 0;
	} 

	for(OTP_ADDR=0;OTP_ADDR<MCU_Config_xx.WriteConfig_xx.OptionSize;)
	{
		FMReadOne(Addr_Flash_OptionStart+OTP_ADDR);
		checksum +=Rxdata;
		OTP_ADDR +=2;		 	
	}
	return checksum;	
}

u16 CalculateCRCStruct(u8 mask)                                                       //mask =0xff or 0x3f or 0x1f for rom bit 16 or bit 14 or bit13
{
	u16 crc = 0xffff;
    u8      dataH,dataL;

    FM_CS_0;
    for(OTP_ADDR=MCU_Config_xx.WriteConfig_xx.RomFirAddr;OTP_ADDR<=MCU_Config_xx.WriteConfig_xx.RomEndAddr;OTP_ADDR++)
    {
        SPI_ReadByte();
        if((OTP_ADDR -MCU_Config_xx.WriteConfig_xx.RomFirAddr)%2 )
        {
            //dataH =(Rxdata & mask);
            dataH = WRITE_DATA_xx.rom_byte[OTP_ADDR];
            crc   = CountCRC(dataH ,dataL ,crc);
        }
        else
        {
            dataL =WRITE_DATA_xx.rom_byte[OTP_ADDR];
        }
    }
    FM_CS_1;
    
    crc &= MCU_Config_xx.WriteConfig_xx.OtpValidBit;

    return crc;
}
u16 CalculateOptionCRCStruct(u8 mask)                                                       //mask =0xff or 0x3f or 0x1f for rom bit 16 or bit 14 or bit13
{
    u16 crc = 0xffff;
    u8  dataH,dataL;

    FM_CS_0;
    for(OTP_ADDR=0;OTP_ADDR<=MCU_Config_xx.WriteConfig_xx.OptionSize*2;OTP_ADDR++)
    {
          SPI_ReadByte();
          if(OTP_ADDR%2 )
          {
            //dataH =(Rxdata & mask);
             dataH = WRITE_DATA_xx.option_byte[OTP_ADDR];
             crc   =CountCRC(dataH ,dataL ,crc);
          }
          else
          {
             dataL =WRITE_DATA_xx.option_byte[OTP_ADDR];
          }
      
    }
    FM_CS_1;
    
    //crc &= MCU_Config_xx.WriteConfig_xx.OtpValidBit;

    return crc;
}


u16 CalculateCRC(u8 mask)                                                       //mask =0xff or 0x3f or 0x1f for rom bit 16 or bit 14 or bit13
{
	u16 crc = 0xffff;
    u8      dataH,dataL;

    FM_CS_0;
    FMReadMore(Addr_Flash_ROMStart+MCU_Config_xx.WriteConfig_xx.RomFirAddr-1);
    for(OTP_ADDR=MCU_Config_xx.WriteConfig_xx.RomFirAddr;OTP_ADDR<=MCU_Config_xx.WriteConfig_xx.RomEndAddr;OTP_ADDR++)
    {
        SPI_ReadByte();
        if((OTP_ADDR -MCU_Config_xx.WriteConfig_xx.RomFirAddr)%2 )
        {
            //dataH =(Rxdata & mask);
            dataH = Rxdata;
            crc   = CountCRC(dataH ,dataL ,crc);
        }
        else
        {
            dataL =Rxdata;
        }
    }
    FM_CS_1;
    
    crc &= MCU_Config_xx.WriteConfig_xx.OtpValidBit;

    return crc;
}


u16 CalculateOptionCRC(u8 mask)                                                       //mask =0xff or 0x3f or 0x1f for rom bit 16 or bit 14 or bit13
{
    u16 crc = 0xffff;
    u8  dataH,dataL;

    FM_CS_0;
    FMReadMore(Addr_Flash_OptionStart-1);
    for(OTP_ADDR=0;OTP_ADDR<=MCU_Config_xx.WriteConfig_xx.OptionSize*2;OTP_ADDR++)
    {
          SPI_ReadByte();
          if(OTP_ADDR%2 )
          {
            //dataH =(Rxdata & mask);
             dataH = Rxdata;
             crc   =CountCRC(dataH ,dataL ,crc);
          }
          else
          {
             dataL =Rxdata;
          }
      
    }
    FM_CS_1;
    
    //crc &= MCU_Config_xx.WriteConfig_xx.OtpValidBit;

    return crc;
}
/* test 
//funtion test 
void SPI_Test()
{
	
	FMReadID();
	FMReadStatus();
	FMWriteStatus(BP_MemoryArray0); 
	FMReadStatus();
	FMChipErase();    
	FMWriteOne(0x1800,0xaa);
	FMWriteOne(0x1801,0x55)
	FMWriteOne(0x1802,0x01);
	FMWriteOne(0x1803,0x02);

	u8 data[4]={0,0,0,0};
	u16 cnt=0;


	FM_CS_0;
	FMReadMore(0x1800);
	data[0]=Rxdata;
	SPI_ReadByte();
	data[1]=Rxdata;
	SPI_ReadByte();
	data[2]=Rxdata;
	SPI_ReadByte();
	data[3]=Rxdata;
	FM_CS_1;
	
	FMReadOne(0x1800);
	data[0]=Rxdata;
	FMReadOne(0x1801);
	data[1]=Rxdata;
	FMReadOne(0x1802);
	data[2]=Rxdata;
	FMReadOne(0x1803);
	data[3]=Rxdata;
	
	
	if((data[0]==0xaa)&&(data[1]==0x55)&&(data[2]==0x01)&&(data[3]==0x02))
	{ 
		GPIO_ResetBits(LEDOK) ; //LEDD1=0
	}
	else
	{
		GPIO_SetBits(LEDOK);
	}
}

*/

