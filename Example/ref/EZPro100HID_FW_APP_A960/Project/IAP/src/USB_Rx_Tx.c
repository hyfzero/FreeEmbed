
/* Includes ------------------------------------------------------------------*/

#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "USB_Rx_Tx.h"
#include "IAP.h"

//u8 const ProgrammingFinished[5]={0x69,0x01,0x02,0x03,0x16};
//u8 const BlankOk[5]={0x69,0x01,0x04,0x05,0x16};
//u8 const NextData[5]={0x69,0x01,0x12,0x13,0x16};
//u8 const Reset[5]={0x69,0x01,0xe2,0xe3,0x16};
//u8 const MotherChipMade[5]={0x69,0x01,0x06,0x07,0x16};
//u8 const Reconfigured[5]={0x69,0x01,0x22,0x23,0x16};
//u8 const EraseFlash[5]={0x69,0x01,0x08,0x09,0x16};
//u8 const ModelIn[5]={0x69,0x01,0x36,0x37,0x16};

//u8 const ReadVison[7]={0x69,0x03,0x46,0x01,0x01,0x4b,0x16};

u8 const ErrorsCS[6]={0x69,0x02,0xf2,0x05,0xf9,0x16};

void USB_Rx_Tx()
{

	
	u32 i;
	u8 checksum=0;
	u32 rom_data;
	u32 rom_addr;
	//checksum
    for(i=2;i<USB_Buffer[2]+2;i++)
	{
		checksum +=USB_Buffer[i];
	}
	checksum= 0xff-checksum;
	if (checksum != USB_Buffer[i])
	{
		//error
		USB_Buffer[0]=0x69;
                USB_Buffer[1]=0xf2;
                USB_Buffer[2]=0xaa;
		USB_SendData(3);
	}
	if (USB_Buffer[1]==0x05) //test com port 
	{ 
               USB_Buffer[0]=0x69;
               USB_Buffer[1]=3;
               USB_Buffer[2]=4;
               USB_Buffer[3]=5;
               USB_Buffer[4]=12;
               USB_Buffer[5]=0x16;
               USB_SendData(6);
	}
	else if (USB_Buffer[1]==0x45)//Read vision
	{
		USB_Buffer[0]=0x69;
		USB_Buffer[1]=0x06;
		USB_Buffer[2]=0x46;
		USB_Buffer[3]=0x01;//Vision: v01.01
		USB_Buffer[4]=0x03;
		USB_Buffer[5]=14;//Date: 12.03.15
        USB_Buffer[6]=7;
		USB_Buffer[7]=14; 
		USB_Buffer[8]=0x4b + USB_Buffer[3] +USB_Buffer[4]+USB_Buffer[5]+USB_Buffer[6]+USB_Buffer[7];
		USB_Buffer[9]=0x16;
		USB_SendData(10);

	}
	else if (USB_Buffer[1]==0x06) //get flashprotection 
	{
		USB_Buffer[0]=0x69;
		if (FlashProtection==0x01)
		{
			//USB_Buffer[1]=0x01;
			//解除写保护
			FLASH_DisableWriteProtectionPages();
		} 
		else
		{
			USB_Buffer[1]=0x00;
		}
		USB_Buffer[2]=0xaa;
		USB_SendData(3);
	}
	else if (USB_Buffer[1]==0x07) //erase flash pages
	{
		//擦除Flash
		NbrOfPage=FLASH_PagesMask(FLASH_SIZE-0x4000);
		for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
		{
			FLASHStatus = FLASH_ErasePage(FlashDestination + (PageSize * EraseCounter));
		}
		
		USB_Buffer[0]=0x69;
		USB_Buffer[1]=0x07;
		USB_Buffer[2]=0xAA;
		USB_SendData(3);
	}
	else if (USB_Buffer[1]==0x08)
	{
		//判断用户是否已经下载程序，因为正常情况下此地址是栈地址。
		//若没有这一句的话，即使没有下载程序也会进入而导致跑飞。
          /*
		if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
		{
			//SerialPutString("Execute user Program\r\n\n");
			//跳转至用户代码
			JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
			Jump_To_Application = (pFunction) JumpAddress;

			//初始化用户程序的堆栈指针
			__set_MSP(*(__IO uint32_t*) ApplicationAddress);
			Jump_To_Application();
		}*/
          USB_Buffer[0]=0x69;
          USB_Buffer[1]=0x08;
          USB_Buffer[2]=0xaa;     
          USB_SendData(3);      
          //复位之前使能USB接收
          USB_Rx_Flag=0x00;
          //#ifndef STM32F10X_CL
          /* Enable the receive of data on EP3 */
          SetEPRxValid(ENDP3);
          //#endif /* STM32F10X_CL */       
          Delay_1ms(300);
          *((u32 *)0xE000ED0C) = 0x05fa0004; //reset system

	}
	else if(USB_Buffer[1]==0X09) //LOAD s19 FILE
	{
		//0x0800 0000
		//S315_0800_0000_980D002031270008D1260008D3260008_BD
                rom_addr=USB_Buffer[3];
		rom_addr=rom_addr<<8;
		rom_addr |=USB_Buffer[4];
		rom_addr=rom_addr<<8;
		rom_addr |=USB_Buffer[5];
		rom_addr=rom_addr<<8;
		rom_addr |=USB_Buffer[6]; //起始地址数据

		for (i=7;i<USB_Buffer[2]+2;)
		{
			rom_data=USB_Buffer[i+3];
			rom_data=rom_data<<8;
			rom_data |=USB_Buffer[i+2];
			rom_data=rom_data<<8;
			rom_data |=USB_Buffer[i+1];
			rom_data=rom_data<<8;
			rom_data |=USB_Buffer[i]; //起始地址数据

			FLASHStatus=FLASH_ProgramWord(rom_addr,rom_data); //4byte one time
			if (*(uint32_t*)rom_addr != rom_data)
			{
				//error
				USB_Buffer[0]=0x69;
				USB_Buffer[1]=0xf2;
				USB_SendData(2);
				break;
			}

			i=i+4;
			rom_addr=rom_addr+4;
		}
		
		    USB_Buffer[0]=0x69;
				USB_Buffer[1]=0xaa;
				USB_SendData(2);
		
	}
	
	USB_Rx_Flag=0x00;
	//#ifndef STM32F10X_CL
	/* Enable the receive of data on EP3 */
	   SetEPRxValid(ENDP3);
	//#endif /* STM32F10X_CL */
}

//-----------------------------------
void USB_SendData(u8 Length)
{
	 u8 index=0;

	  while (Length >= 64)  //一次最多发送64字节
	  {
	    while(GetEPTxStatus(ENDP1) != EP_TX_NAK);  
	    UserToPMABufferCopy(&USB_Buffer[index], ENDP1_TXADDR, 64);
	    SetEPTxCount(ENDP1, 64);
	    SetEPTxValid(ENDP1);
	    index  += 64;  //计算偏移和剩余字节数
	    Length -= 64;
	  }
	//发送最后<=64字节的数据
	while(GetEPTxStatus(ENDP1) != EP_TX_NAK); 
	UserToPMABufferCopy(&USB_Buffer[index], ENDP1_TXADDR, Length);
	SetEPTxCount(ENDP1, Length);
	SetEPTxValid(ENDP1);
	////clear se_flag
	//USB_Rx_Flag=0x00;
	//remain=63;
	//index=0;
}