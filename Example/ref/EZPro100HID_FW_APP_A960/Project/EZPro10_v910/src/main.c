/******************** (C) COPYRIGHT SinoMCU ********************
* File Name          : main.c
* Author             : Mike Mo
* Version            : V2.2
* Date               : 21-March-2019
* Description        : MTP Program 
* Font               : GB2312
*/
/* Includes ------------------------------------------------------------------*/

#include "stm32f10x.h"

#include "stm32f10x_tim.h"  
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "erorrNum.h"
#include "spi.h"
#include "power.h"
#include "Config.h"
#include "ADC.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "USB_Rx_Tx.h"
#include "LCD.h"
#include "IRC.h"
#include "OTPRW.h"
#include "wx_i2c.h"
#include "delay.h"
#include "MC301Pro.h"
#include  "mc30f6910.h"
#include    "MTPRW.h"

#define SINOMCU_IAP 


/*******************************************************************************
* Function Name  : main. Writer only
* Description    : Main routine.
* Input          : None.
* Output         : None.
* Return         : None.c
*******************************************************************************/
int main(void)  
{
    //u16 i;     
    //----------------------------------- 
    u8 OKorNG; 
    u8 Retry=0;
    // u8 ProgramID;
    //u16 OKCnt=0;
    /* System Clocks Configuration */ 
    RCC_Configuration();
    /* Configure the GPIO ports */
    GPIO_Configuration(); 
    
    //  NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x4000);
    #ifdef SINOMCU_IAP
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x4000);
    #endif

    delay_init();  

    LEDOK_Off;
    LEDNG_Off;
    LEDBUSY_Off;

    //USB_REG_Off;
    IRCInitial();//OSCO use for a351/383 super model

    ConfigTimer();
    ConfigTIM4();
    /* spi2 configuration  */
    SPI2Init();
    //IIC initial
    IIC_Init();
    FLASH_Unlock();
    /* ADC initail  */
    ADC_Config();
    //DMA_Config();

    PowerInitial();

    OTPInit();
    VDD30V_Off;
    POWER_OFF(vpp00,vdd00);
    Power18V_On;

    GPIO_SetBits(GPIOA,GPIO_Pin_10);//蜂鸣器响
    Delay_1ms(300);
    GPIO_ResetBits(GPIOA,GPIO_Pin_10);

    OKorNG=1;
    OKcounter=0;
    NGcounter=0;
    ERORR_VALUE=0;

  //adj_vdd
    Retry=GPIO_ReadInputData(GPIOD);
    if(Retry==0xff) //进入测试模式
      Writer_Test();

    DeviceConfig(); //no use this funtion when key select type 

    LoadConfigToRAM();
    LoadCodeToRAM();
    LoadOptionToRAM();  

//    MCU_Config_xx.ModeIn_CONFIG_xx.WriteType=WriteFPGA;

    if(MCU_Config_xx.ModeIn_CONFIG_xx.WriteType == WriteFPGA)
    {
        MCP42050_ADJ(ADJ_VPP,25);
        MCP42050_ADJ(ADJ_VDD,25);
    }
    else
    {
        MCP42050_ADJ(ADJ_VPP,MCU_Config_xx.ModeIn_CONFIG_xx.ProVPP);//vpp value
        MCP42050_ADJ(ADJ_VDD,MCU_Config_xx.ModeIn_CONFIG_xx.ProVDD); 
    }
 
  
    
  //USB_RxDataCS=CalculateCheckSum(); 
  USB_RxDataCS = CalculateCRC(0xff);
  OptionCS = CalculateOptionCRC(0xff);
  //OptionCS =CalculateOptionCheckSum_7343();

  
  //----------LCD display initial --------------------------
  for(Retry=0;Retry<IIC_DATA_BUFF_SIZE;Retry++) // dynamic id
  {
    iic_data[Retry]=IIC_Read(Retry);
  }
  Initial_Lcd();

  displayfuntion();//LCD 
  //Power18V_Off;
  //-----------------------------------
    
  Set_System();
  Set_USBClock();
  USB_Init();
  USB_REG_On;
  USB_Interrupts_Config();

  //-------------------------------------  
  USB_Rx_Flag=0x00;
  USB_RxCommand=0x00;
  StateFlag = 0xfe;

//  MC32P8132_Program();
  while (1) 
  {  	
    //RCC_MCOConfig(RCC_MCO_NoClock);
    //RCC_MCOConfig(RCC_MCO_PLLCLK_Div2);
         
    if(USB_Rx_Flag==0xaa && USB_Rx_Buffer[0]==0x68)
    {
      USB_Rx_Tx();
      //Retry=0;
    }
    else if(USB_Rx_Flag ==0xaa && USB_Rx_Buffer[0]!=0x68)
    {
      USB_Rx_Flag=0x00;
      //#ifndef STM32F10X_CL
      /* Enable the receive of data on EP3 */
         SetEPRxValid(ENDP1);
      //#endif /* STM32F10X_CL */
    }   
    //----------------------------------
    //---------------- key scan ---------------------------------
    if (GPIO_ReadInputDataBit(KEY)==0)
    {
      Delay_1ms(5);
      if (GPIO_ReadInputDataBit(KEY)==0)
      {
        StateFlag = GPIO_ReadInputData(GPIOD) & 0x00fe;
        //StateFlag = WR_Command; //
        LEDNG_Off;
        LEDOK_Off;

        for(Retry=0;Retry<IIC_DATA_BUFF_SIZE;Retry++) // dynamic id
        {
           iic_data[Retry]=IIC_Read(Retry);
        }
      }
      while( GPIO_ReadInputDataBit(KEY)==0 );  
    } 
    //----------------------------------------------
    if (StateFlag != 0xfe)
    {
      LEDBUSY_On;
      LEDNG_Off;
      LEDOK_Off;
      Retry=0;     
      if ((StateFlag & 0x80)==0) //加载母片程序到FLASH，当前没用到
      {
        //Read command -------------------------				
        StateFlag |= 0x08 ;//IF Read active,don't Write mcu

        //OKorNG=OTP_DownLoad();
        while(++Retry <10)  
        {
          if (OTP_DownLoad(DeviceConfig_xx.MCU_ID,DeviceConfig_xx.RomFirAddr))
          {
            OKorNG=1;
            break;
          }
          else if(Retry ==8)
          {
            OKorNG =0;
            break;
          }
        }
        //POWER_OFF(vpp00,vdd00);
      }

      //blank
      if ((StateFlag & 0x20)==0) 
      {
        //Check Blank command
        LEDBUSY_On;
        LEDBUSY_Off;
      }
      //Write ---------------------------------
      if ((StateFlag & 0x08)==0) 
      {
        //烧写开始 SCK SDI信号使能
        OTP_DISA_0;
        OTP_DISB_0;
        OTP_DISC_0;
        OTP_DISD_0;  
        
        while(++Retry <5)
        {
          if(mc30f6910_program())
          {
              OKorNG=1;
              break;            
          }        
          //if(Retry ==3)
          if(Retry ==1)//测试用
          {
            OKorNG =0;
            break;
          }
          FM_CS_1;
          POWER_OFF(vpp00,vdd00);
          //ConfigTimer();
          ConfigTIM4();
          Delay_1ms(100);
          Delay_1ms(200);
        }
        // ok counter & dynamic id process 
        if (OKorNG)
        {
          OKcounter=OKcounter+1;
          OKorNG=dynamic_id_add(); 
        } 
        else
        {
          NGcounter=NGcounter+1;
        }

        if(USB_RxCommand ==0x01)
        {
          for(Retry=0;Retry<64;Retry++)
            USB_Tx_Buffer[Retry]=0x00; 
          
          if (OKorNG==1)
          {
            for(Retry=0;Retry<5;Retry++)
            {
              USB_Tx_Buffer[Retry]=ProgrammingFinished[Retry];
            }
            USB_SendData(64);
          } 
          else
          {
            USB_Tx_Buffer[0]=0x68;
            USB_Tx_Buffer[1]=0x06;
            USB_Tx_Buffer[2]=0xf2;
            USB_Tx_Buffer[3]=0x03;
            USB_Tx_Buffer[4]=OTP_ADDR/0x100;
            USB_Tx_Buffer[5]=OTP_ADDR%0x100;
            USB_Tx_Buffer[6]=Rxdata;
            USB_Tx_Buffer[7]=OTP_ReadData;
            USB_Tx_Buffer[8]=0xfb + USB_Tx_Buffer[4] +USB_Tx_Buffer[5]+USB_Tx_Buffer[6]+USB_Tx_Buffer[7];
            USB_Tx_Buffer[9]=0x16;
            USB_SendData(64);
          }
        }
        //POWER_OFF(vpp00,vdd00);
      }
      //----------------------------------------
      if ((StateFlag & 0x10)==0) //Verify
      {
        OKorNG=OTP_Verify( );

        POWER_OFF(vpp00,vdd00);
      }

      FM_CS_1;
      POWER_OFF(vpp00,vdd00);
      
        //烧写结束后 SCK SDI烧写脚悬空
        OTP_DISA_1;
        OTP_DISB_1;
        OTP_DISC_1;
        OTP_DISD_1;        
        
      ConfigTIM4();
      LEDBUSY_Off;

      Freq(OKorNG);

      StateFlag=0xfe;
      USB_RxCommand=0x00;
      //Delay_1ms(200);
      //Delay_1ms(200);
      //while(GPIO_ReadInputDataBit(KEY)==0); //waite key off
    } 
  }//whil(1)
}
#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
