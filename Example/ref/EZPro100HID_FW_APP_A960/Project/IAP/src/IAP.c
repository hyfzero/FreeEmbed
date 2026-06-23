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
#include "ymodem.c"


pFunction Jump_To_Application;
u32 JumpAddress;
u32 BlockNbr=0;
u32 UserMemoryMask=0;
u8  FlashProtection=0;

 void IAP_funtion()
 {
	 
           
	 BlockNbr = (FlashDestination - DefaultStartAddress) >> 12;

#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
	 UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
#else /* USE_STM3210E_EVAL */
	 if (BlockNbr < 62)
	 {
		 UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
	 }
	 else
	 {
		 UserMemoryMask = ((uint32_t)DefaultStartAddress);
	 }
#endif /* (STM32F10X_MD) || (STM32F10X_MD_VL) */

	 if ((FLASH_GetWriteProtectionOptionByte() & UserMemoryMask) != UserMemoryMask)
	 {
		 FlashProtection = 1;
	 }
	 else
	 {
		 FlashProtection = 0;
	 }
     
//     USB_Buffer[0]=0x68;
//     USB_Buffer[1]=0x00;
//     USB_Buffer[2]=0x08;
//     USB_Buffer[3]=0x16;
//     USB_SendData(4); 

	 while (1)
	 {
		 if(USB_Rx_Flag==0xaa && USB_Buffer[0]==0x69)
		 {
			 USB_Rx_Tx();
			 //Retry=0;
		 }
		 else if (USB_Rx_Flag==0xaa && USB_Buffer[0]!=0x69)
		 {
			 USB_Rx_Flag=0x00;
			 /* Enable the receive of data on EP3 */
			 SetEPRxValid(ENDP3);
		 }

		// StateFlag=GPIO_ReadInputDataBit(iapKEY);
	 }//whil(1)

	 //while (1) 
	 //{  	//RCC_MCOConfig(RCC_MCO_NoClock);
		// //RCC_MCOConfig(RCC_MCO_PLLCLK_Div2);

		// if(USB_Rx_Flag==0xaa && USB_Rx_Buffer[0]==0x68)
		// {
		//	 USB_Rx_Tx();
		//	 //Retry=0;
		// }
		// else if(USB_Rx_Flag ==0xaa && USB_Rx_Buffer[0]!=0x68)
		// {
		//	 USB_Rx_Flag=0x00;
		//	 //#ifndef STM32F10X_CL
		//	 /* Enable the receive of data on EP3 */
		//	 SetEPRxValid(ENDP3);
		//	 //#endif /* STM32F10X_CL */
		// }

	 //}//whil(1)
 }



 /*******************************************************************************
  * @函数名称	FLASH_DisableWriteProtectionPages
  * @函数说明   接触Flash写保护
  * @输入参数   无
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void FLASH_DisableWriteProtectionPages(void)
{
    u32 useroptionbyte = 0, WRPR = 0;
    u32 var1 = OB_IWDG_SW, var2 = OB_STOP_NoRST, var3 = OB_STDBY_NoRST;
    FLASH_Status status = FLASH_BUSY;

    WRPR = FLASH_GetWriteProtectionOptionByte();

    //测试是否写保护
    if ((WRPR & UserMemoryMask) != UserMemoryMask)
    {
        useroptionbyte = FLASH_GetUserOptionByte();

        UserMemoryMask |= WRPR;

        status = FLASH_EraseOptionBytes();

        if (UserMemoryMask != 0xFFFFFFFF)
        {
            status = FLASH_EnableWriteProtection((uint32_t)~UserMemoryMask);
        }
        //用处选项字是否有编程
        if ((useroptionbyte & 0x07) != 0x07)
        {
            //重新保存选项字
            if ((useroptionbyte & 0x01) == 0x0)
            {
                var1 = OB_IWDG_HW;
            }
            if ((useroptionbyte & 0x02) == 0x0)
            {
                var2 = OB_STOP_RST;
            }
            if ((useroptionbyte & 0x04) == 0x0)
            {
                var3 = OB_STDBY_RST;
            }

            FLASH_UserOptionByteConfig(var1, var2, var3);
        }

        if (status == FLASH_COMPLETE)
        {
            USB_Buffer[1]=0x01;
            //系统复位重新加载选项字
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG,ENABLE);
			WWDG_Enable(0x4F);
        }
        else
        {
            USB_Buffer[1]=0x02;
        }
    }
    else
    {
        USB_Buffer[1]=0x03;
    }
}


/*******************************************************************************
  * @函数名称	FLASH_PagesMask
  * @函数说明   计算Falsh页
  * @输入参数   Size:文件长度
  * @输出参数   无
  * @返回参数   页的数量
*******************************************************************************/
uint32_t FLASH_PagesMask( u32 Size)
{
    uint32_t pagenumber = 0x0;
    uint32_t size = Size;

    if ((size % PAGE_SIZE) != 0)
    {
        pagenumber = (size / PAGE_SIZE) + 1;
    }
    else
    {
        pagenumber = size / PAGE_SIZE;
    }
    return pagenumber;

}

