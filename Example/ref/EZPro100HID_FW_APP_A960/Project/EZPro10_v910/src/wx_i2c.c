
#include "stm32f10x.h"
#include "wx_i2c.h"
#include "global.h"
#include "erorrNum.h"
#include "storage.h"
#include "i2c_transport.h"


#define IIC_Write_Address 0xa0 //Write Address
#define IIC_Read_Address 0xa1 //Read Address


#define IIC_SDA_DAT   GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) 

#define IIC_SDA_L   GPIO_ResetBits(GPIOB, GPIO_Pin_5)
#define IIC_SDA_H   GPIO_SetBits(GPIOB, GPIO_Pin_5)
#define IIC_SCL_L   GPIO_ResetBits(GPIOB, GPIO_Pin_6)
#define IIC_SCL_H   GPIO_SetBits(GPIOB, GPIO_Pin_6)

#define A960_EEPROM_CAPACITY_BYTES  0x00010000UL
#define A960_I2C_TIMEOUT_MS         1UL

static Storage a960_eeprom_storage;
static u8 a960_eeprom_constructed;

void IIC_Send_ack(void);

static StorageStatus a960_eeprom_init(Storage *storage);
static StorageStatus a960_eeprom_read(Storage *storage, uint64_t address, uint8_t *data, size_t length);
static StorageStatus a960_eeprom_program(Storage *storage, uint64_t address, const uint8_t *data, size_t length);
static HwStatus a960_i2c_bus_transfer(void *ctx, const I2cDevice *device, const I2cMessage *messages, size_t message_count, uint32_t timeout_ms);
static HwStatus a960_i2c_bus_probe(void *ctx, const I2cDevice *device, uint32_t timeout_ms);

static const StorageOps a960_eeprom_ops =
{
	a960_eeprom_init,
	a960_eeprom_read,
	a960_eeprom_program,
	0
};

static const I2cBusOps a960_i2c_bus_ops =
{
	a960_i2c_bus_transfer,
	a960_i2c_bus_probe
};

static I2cBus a960_i2c_bus =
{
	&a960_i2c_bus_ops,
	0
};

static I2cDevice a960_eeprom_device =
{
	&a960_i2c_bus,
	0x50,
	0
};

static StorageStatus a960_eeprom_construct(void)
{
	if (a960_eeprom_constructed == 0)
	{
		storage_base_construct(&a960_eeprom_storage, "a960_eeprom", &a960_eeprom_ops);
		a960_eeprom_constructed = 1;
	}

	if (a960_eeprom_storage.initialized == 0)
	{
		return storage_init(&a960_eeprom_storage);
	}

	return STORAGE_OK;
}

Storage *A960_EepromStorage(void)
{
	(void)a960_eeprom_construct();
	return &a960_eeprom_storage;
}

static StorageStatus a960_eeprom_init(Storage *storage)
{
	storage->info.capacity_bytes = A960_EEPROM_CAPACITY_BYTES;
	storage->info.page_size_bytes = 1;
	storage->info.erase_size_bytes = 0;
	storage->info.capabilities = STORAGE_CAP_READ | STORAGE_CAP_PROGRAM;
	storage->info.erased_value = 0xff;

	return STORAGE_OK;
}

static u8 a960_i2c_read_byte_is_last(const I2cMessage *message, size_t segment_index, size_t byte_index)
{
	return ((segment_index + 1) == message->segment_count) && ((byte_index + 1) == message->segments[segment_index].length);
}

static HwStatus a960_i2c_bus_transfer(void *ctx, const I2cDevice *device, const I2cMessage *messages, size_t message_count, uint32_t timeout_ms)
{
	size_t message_index;
	size_t segment_index;
	size_t byte_index;
	u8 address_byte;
	u8 has_read = 0;
	const I2cMessage *message;
	const I2cSegment *segment;

	(void)ctx;
	(void)timeout_ms;

	for (message_index = 0; message_index < message_count; message_index++)
	{
		message = &messages[message_index];
		if ((message->flags & I2C_MESSAGE_READ) != 0)
		{
			has_read = 1;
		}

		IIC_Start();
		address_byte = (u8)((device->address << 1) | (((message->flags & I2C_MESSAGE_READ) != 0) ? 1 : 0));
		IIC_Writebyte(address_byte);
		IIC_Wait_ack();

		for (segment_index = 0; segment_index < message->segment_count; segment_index++)
		{
			segment = &message->segments[segment_index];
			for (byte_index = 0; byte_index < segment->length; byte_index++)
			{
				if ((message->flags & I2C_MESSAGE_READ) != 0)
				{
					segment->rx_data[byte_index] = IIC_Readbyte();
					if (a960_i2c_read_byte_is_last(message, segment_index, byte_index) == 1)
					{
						IIC_Send_noack();
					}
					else
					{
						IIC_Send_ack();
					}
				}
				else
				{
					IIC_Writebyte(segment->tx_data[byte_index]);
					IIC_Wait_ack();
				}
			}
		}
	}

	IIC_Stop();
	if (has_read == 0)
	{
		delay_5us(10000);
	}

	return HW_STATUS_OK;
}

static HwStatus a960_i2c_bus_probe(void *ctx, const I2cDevice *device, uint32_t timeout_ms)
{
	u8 address_byte;

	(void)ctx;
	(void)timeout_ms;

	IIC_Start();
	address_byte = (u8)(device->address << 1);
	IIC_Writebyte(address_byte);
	IIC_Wait_ack();
	IIC_Stop();

	return HW_STATUS_OK;
}

static StorageStatus a960_eeprom_read(Storage *storage, uint64_t address, uint8_t *data, size_t length)
{
	size_t index;
	uint8_t word_address;
	I2cSegment address_segment;
	I2cSegment data_segment;
	I2cMessage messages[2];

	(void)storage;

	for (index = 0; index < length; index++)
	{
		word_address = (uint8_t)((address + index) & 0xffU);
		address_segment.tx_data = &word_address;
		address_segment.rx_data = 0;
		address_segment.length = 1;
		data_segment.tx_data = 0;
		data_segment.rx_data = &data[index];
		data_segment.length = 1;
		messages[0].segments = &address_segment;
		messages[0].segment_count = 1;
		messages[0].flags = 0;
		messages[1].segments = &data_segment;
		messages[1].segment_count = 1;
		messages[1].flags = I2C_MESSAGE_READ;

		if (i2c_transfer(&a960_eeprom_device, messages, 2, A960_I2C_TIMEOUT_MS) != HW_STATUS_OK)
		{
			return STORAGE_IO_ERROR;
		}
	}

	return STORAGE_OK;
}

static StorageStatus a960_eeprom_program(Storage *storage, uint64_t address, const uint8_t *data, size_t length)
{
	size_t index;
	uint8_t word_address;
	I2cSegment segments[2];
	I2cMessage message;

	(void)storage;

	for (index = 0; index < length; index++)
	{
		word_address = (uint8_t)((address + index) & 0xffU);
		segments[0].tx_data = &word_address;
		segments[0].rx_data = 0;
		segments[0].length = 1;
		segments[1].tx_data = &data[index];
		segments[1].rx_data = 0;
		segments[1].length = 1;
		message.segments = segments;
		message.segment_count = 2;
		message.flags = 0;

		if (i2c_transfer(&a960_eeprom_device, &message, 1, A960_I2C_TIMEOUT_MS) != HW_STATUS_OK)
		{
			return STORAGE_IO_ERROR;
		}
	}

	return STORAGE_OK;
}

//S/N serial number
// on/off,length,AddrH,AddrL,IDstrat_3,2,1,0,IDend3,2,1,0,index
//    1/0,  1-4 , 0x18, 0x00,  0	  ,0,0,1,  1   ,0,0,0,  1 ; on 4,0x1800, 0001



u8 dynamic_id_add()
{
	u32 id_a_current=0;
	//u32 id_a_end=0;
	u8 index;
        u8 rew;

	index=iic_data[14];
	//id_length=iic_data[1];
	/*id_addr0=(iic_data[2]<<8)+iic_data[3];
	id_addr1=(iic_data[4]<<8)+iic_data[5];
	id_addr2=(iic_data[6]<<8)+iic_data[7];
	id_addr3=(iic_data[8]<<8)+iic_data[9];*/

		if ((iic_data[0]&0x0f)==0x0a)
		{//滚动码开
			id_a_current=(iic_data[10]<<24)+(iic_data[11]<<16)+(iic_data[12]<<8)+iic_data[13];
			//id_a_end=(iic_data[15]<<24)+(iic_data[16]<<16)+(iic_data[17]<<8)+iic_data[18];

			id_a_current+=index;

			if (iic_data[1]==0x01)
			{
				id_a_current &=0xff;
			}
			else if (iic_data[1]==0x02)
			{
				id_a_current &=0xffff;
			}
			else if (iic_data[1]==0x03)
			{
				id_a_current &=0xffffff;
			}
			else
			{
				id_a_current &=0xffffffff;
			}
                        
                        rew=0;
			while(rew<4)
			{
				IIC_Write(13,(id_a_current& 0xff));
				IIC_Write(12,((id_a_current>>8)&0xff));
				IIC_Write(11,((id_a_current>>16)&0xff));
				IIC_Write(10,((id_a_current>>24)&0xff));

				//OKcounter=OKcounter+1;
				IIC_Write(22,(OKcounter& 0xff));
				IIC_Write(21,((OKcounter>>8)&0xff));
				IIC_Write(20,((OKcounter>>16)&0xff));
				IIC_Write(19,((OKcounter>>24)&0xff));
				
				index=IIC_Read(22);

				if (index ==(OKcounter & 0xff))
				{				
					break;
				}
				else if (rew==3)
				{
					ERORR_VALUE=EEPROM_Write_ERROR;
					return 0;
				}				
			}

		}
                else if(iic_data[0]==0xa0)	//烧写数量限制
		{
            rew=0;
			while(rew<4)
			{
				// IIC_Write(13,(id_a_current& 0xff));
				// IIC_Write(12,((id_a_current>>8)&0xff));
				// IIC_Write(11,((id_a_current>>16)&0xff));
				// IIC_Write(10,((id_a_current>>24)&0xff));

				//OKcounter=OKcounter+1;
				IIC_Write(22,(OKcounter& 0xff));
				IIC_Write(21,((OKcounter>>8)&0xff));
				IIC_Write(20,((OKcounter>>16)&0xff));
				IIC_Write(19,((OKcounter>>24)&0xff));
				
				index=IIC_Read(22);

				if (index ==(OKcounter & 0xff))
				{				
					break;
				}
				else if (rew==3)
				{
					ERORR_VALUE=EEPROM_Write_ERROR;
					return 0;
				}				
			}
		}                


		return 1;
}

void delay_5us(u16 num)//满足IIC时序的延时
{
	u16 i,j;
	for(i=num;i>0;i--)
	{
		for(j=5;j>0;j--);
	}
}

void IIC_Init(void)//IIC初始化程序
{
IIC_SDA_H;
IIC_SCL_H;
delay_5us(1);
(void)a960_eeprom_construct();
}

void IIC_Start(void)//IIC开始信号
{
IIC_SDA_H;
IIC_SCL_H;
delay_5us(1);
IIC_SDA_L;
delay_5us(1);
IIC_SCL_L;
}

void IIC_Stop(void)//IIC停止信号
{
IIC_SDA_L;
IIC_SCL_H;
delay_5us(1);
IIC_SDA_H;
}

void IIC_Wait_ack(void)//IIC主器件等待应答  ?????
{
     u16 ErrTime=500;
    IIC_SCL_H;
//while(IIC_SDA&&(ErrTime>0))
	while(IIC_SDA_DAT)
	{
		ErrTime--;
                if(ErrTime<2)
                  break;
	}
delay_5us(1);
IIC_SCL_L;
}


void IIC_Send_ack(void)//IIC主器件发送应答
{
IIC_SDA_L;
delay_5us(1);
IIC_SCL_H;
delay_5us(1);
IIC_SCL_L;
}


void IIC_Send_noack(void)//IIC主器件发送非应答
{
IIC_SDA_H;
IIC_SCL_H;
delay_5us(1);
IIC_SCL_L;
}


void IIC_Writebyte(u8 Data)//IIC写一个字节
{
u8 i;
for(i=8;i>0;i--)
{
   IIC_SCL_L;
   if((Data&0x80)==0x80)
   {
    IIC_SDA_H;
   }
   else
   {
    IIC_SDA_L;
   }
   Data<<=1;
   delay_5us(1);
   IIC_SCL_H;
   delay_5us(1);
}
IIC_SCL_L;
IIC_SDA_H;
delay_5us(10);
}


u8 IIC_Readbyte(void)//IIC读一个字节
{
u8 i,Data=0;
IIC_SDA_H;
for(i=8;i>0;i--)
{
   Data<<=1;
   IIC_SCL_L;
   delay_5us(1);
   IIC_SCL_H;
   delay_5us(1);
//   Data|=GPIO_ReadInputDataBit(GPIOB, GPIO_Pin7) 
   Data|=IIC_SDA_DAT; //读管脚的状态
}
IIC_SCL_L;
delay_5us(1);
return(Data);
}


void IIC_Write(u16 Address,u8 Data)//IIC write one byte
{
	(void)storage_program(A960_EepromStorage(), Address, &Data, 1);
}


void IIC_Write_Array(u8 *Data,u16 Address,u16 Num)//IIC 往起始地址Address里写Data[]数组
{
u8 i;
u8 *p;
u8 page1,page2,yushu;
u8 page,addr,addr1;
addr = Address&0xff;
page1 = Address>>5;
page2 =(Address+Num)>>5;
addr1 = (Address+Num)&0xff;
p=Data;
for(page=page1;page<=page2;page++)
{
if((page==page1)||(page==page2))
{
   if(page==page1)
   	{
   	 if(page1==page2)
   	 	{
                yushu= Num;
   	           }
   	
    else
    	{
		yushu=(32-addr&0x1f)%32;
    	}
   	}
   else 
   	yushu=addr1%32;
}
else
{
	yushu=0;
}
IIC_Start();
IIC_Writebyte(IIC_Write_Address);
IIC_Wait_ack();
IIC_Writebyte(page>>3);
IIC_Wait_ack();
IIC_Writebyte(addr);
IIC_Wait_ack();
for(i=0;i<32;i++)
{
addr++;
IIC_Writebyte(*p);
IIC_Wait_ack();
p++;
if(yushu!=0)
{
	if(i==yushu-1)
	break;
}
}
IIC_Stop();
delay_5us(10000);
}  
}


u8 IIC_Read(u16 Address)//IIC read one byte
{
	u8 Data = 0;

	(void)storage_read(A960_EepromStorage(), Address, &Data, 1);
	return(Data);
}


void IIC_Read_Array(u8 *Data,u16 Address,u16 Num)//IIC 读起始地址Address里的Data[]
{
u16 i;
u8 *p;
u8 page,addr;
addr = Address&0xff;
page = Address>>8;
p=Data;   
IIC_Start();
IIC_Writebyte(IIC_Write_Address);
IIC_Wait_ack();
IIC_Writebyte(page);
IIC_Wait_ack();
IIC_Writebyte(addr);
IIC_Wait_ack();
IIC_Start();
IIC_Writebyte(IIC_Read_Address);
IIC_Wait_ack();
for(i=0;i<Num;i++)
{
*(p+i)=IIC_Readbyte();
if(i==Num-1)
	IIC_Send_noack();
else
	IIC_Send_ack();
}
IIC_Stop();
}


void IIC_Clear(u16 Address,u16 Num)
{
u8 i,j;
u8 page1,page2,yushu;
u8 page,addr,n,addr1;
addr = Address&0xff;
page1 = (Address>>8)&0xf;
page2 =((Address+Num)>>8)&0xf;
addr1 = (Address+Num)&0xff;
for(page=page1;page<=page2;page++)
{
if((page==page1)||(page==page2))
{
   if(page==page1)
   	{
   	 if(page1==page2)
   	 	{
	 	n=Num/32;
	 yushu=Num%32;
   	 	}
	  else
	  	{
   	n=(256-addr)/32;
	 yushu=(256-addr)%32;
	  	}
	 if(yushu)
	 	n+=1;
   	}
    else
    	{
    	n=addr1/32;
		yushu=addr1%32;
		if(yushu)
			n+=1;
    	}
}
else
{
	n=8;
	yushu=0;
}
for(j=0;j<n;j++)
{
IIC_Start();
IIC_Writebyte(IIC_Write_Address);
IIC_Wait_ack();
IIC_Writebyte(page);
IIC_Wait_ack();
IIC_Writebyte(addr+32*j);
IIC_Wait_ack();
for(i=0;i<32;i++)
{
IIC_Writebyte(0xff);
IIC_Wait_ack();
}
IIC_Stop();
delay_5us(10000);
}
}
}

/*******************************************************************************************************
                                 end  file!!!
********************************************************************************************************/

