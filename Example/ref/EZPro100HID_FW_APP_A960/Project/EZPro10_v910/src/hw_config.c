/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : hw_config.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Hardware Configuration & Setup
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#ifdef STM32L1XX_MD
 #include "stm32l1xx_it.h"
#else
 #include "stm32f10x_it.h"
#endif /* STM32L1XX_MD */

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "Config.h"
#include "global.h"

#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "stm32_eval.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ErrorStatus HSEStartUpStatus;
USART_InitTypeDef USART_InitStructure;

uint8_t  USART_Rx_Buffer [USART_RX_DATA_SIZE]; 
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;

uint8_t  USB_Tx_State = 0;

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
/* Extern variables ----------------------------------------------------------*/

//extern LINE_CODING linecoding;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_System(void)
{
//#if !defined(STM32F10X_CL) && !defined(STM32L1XX_MD)
//  GPIO_InitTypeDef GPIO_InitStructure;
//#endif /* STM32F10X_CL && STM32L1XX_MD */  
//
//#if defined(USB_USE_EXTERNAL_PULLUP)
//  GPIO_InitTypeDef  GPIO_InitStructure;
//#endif /* USB_USE_EXTERNAL_PULLUP */ 
//  
//  /*!< At this stage the microcontroller clock setting is already configured, 
//       this is done through SystemInit() function which is called from startup
//       file (startup_stm32f10x_xx.s) before to branch to application main.
//       To reconfigure the default setting of SystemInit() function, refer to
//       system_stm32f10x.c file
//     */   
//#ifdef STM32L1XX_MD
//  /* Enable the SYSCFG module clock */
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
//#endif /* STM32L1XX_MD */ 
//  
//  
//#if !defined(STM32F10X_CL) && !defined(STM32L1XX_MD) 
//  /* Enable USB_DISCONNECT GPIO clock */
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);
//
//  /* Configure USB pull-up pin */
//  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
//  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);
//#endif /* STM32F10X_CL && STM32L1XX_MD */
//   
//#if defined(USB_USE_EXTERNAL_PULLUP)
//  /* Enable the USB disconnect GPIO clock */
//  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIO_DISCONNECT, ENABLE);
//
//  /* USB_DISCONNECT used as USB pull-up */
//  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);  
//#endif /* USB_USE_EXTERNAL_PULLUP */  

  /* Enable USB_DISCONNECT GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);

  //把JTAG接口引脚设置成GPIO功能
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); 
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

  /* ADCCLK = PCLK2/8 */
  //RCC_ADCCLKConfig(RCC_PCLK2_Div8);      
  /* Configure the used GPIOs*/
  GPIO_Configuration_1();
  /* Additional EXTI configuration (configure both edges) */
  //EXTI_Configuration();

//  /* Configure the ADC*/
//  ADC_Configuration();  
  
}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_USBClock(void)
{
//#if defined(STM32L1XX_MD)
//  /* Enable USB clock */
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
//  
//#elif defined(STM32F10X_CL)
//  /* Select USBCLK source */
//  RCC_OTGFSCLKConfig(RCC_OTGFSCLKSource_PLLVCO_Div3);
//
//  /* Enable the USB clock */ 
//  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_OTG_FS, ENABLE) ;
//  
//#else 
//  /* Select USBCLK source */
//  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
//  
//  /* Enable the USB clock */
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
//#endif /* STM32F10X_CL */
  
  #if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  /* Enable USB clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
  
#else
  /* Select USBCLK source */
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  
  /* Enable the USB clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
#endif /* STM32L1XX_MD */
  
}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode
* Description    : Power-off system clocks and power while entering suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode
* Description    : Restores system clocks and power while exiting suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  /* Set the device state to the correct state */
  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
  else
  {
    bDeviceState = ATTACHED;
  }
  /*Enable SystemCoreClock*/
  SystemInit();
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  /* Enable The HSI (16Mhz) */
  RCC_HSICmd(ENABLE); 
#endif
#if defined(STM32F30X)
  ADC30x_Configuration();
#endif
}

/*******************************************************************************
* Function Name  : USB_Interrupts_Config
* Description    : Configures the USB interrupts
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Interrupts_Config(void)
{
//  NVIC_InitTypeDef NVIC_InitStructure;
//
//  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
//
//#ifdef STM32L1XX_MD
//  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//#elif defined(STM32F10X_CL) 
//  /* Enable the USB Interrupts */
//  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//#else
//  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//#endif /* STM32L1XX_MD */
//
//  /* Enable USART Interrupt */
//  NVIC_InitStructure.NVIC_IRQChannel = EVAL_COM1_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitTypeDef NVIC_InitStructure; 
  
  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
//#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
//  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//  /* Enable the USB Wake-up interrupt */
//  NVIC_InitStructure.NVIC_IRQChannel = USB_FS_WKUP_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//#elif defined(STM32F37X)
//  /* Enable the USB interrupt */
//  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//  /* Enable the USB Wake-up interrupt */
//  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//  
//#else
  /* Enable the USB interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the USB Wake-up interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);  
  
}

void USB_Connect_On()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //GPIOX_BSRR 高16位置1 清零对应位，低16位置1 置位对应位。
    //GPIOA->BSRR=1<<(10); //CLR PA10
}

void USB_Connect_Off()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //GPIOX_BSRR 高16位置1 清零对应位，低16位置1 置位对应位。
    //GPIOA->BSRR=1<<(10+16); //CLR PA10

}

/*******************************************************************************
* Function Name  : USB_Cable_Config
* Description    : Software Connection/Disconnection of USB Cable
* Input          : None.
* Return         : Status
*******************************************************************************/
void USB_Cable_Config (FunctionalState NewState)
{
//#ifdef STM32L1XX_MD
//  if (NewState != DISABLE)
//  {
//    STM32L15_USB_CONNECT;
//  }
//  else
//  {
//    STM32L15_USB_DISCONNECT;
//  }  
//
//#elif defined(USE_STM3210C_EVAL)  
//  if (NewState != DISABLE)
//  {
//    USB_DevConnect();
//  }
//  else
//  {
//    USB_DevDisconnect();
//  }
//  
//#else /* USE_STM3210B_EVAL or USE_STM3210E_EVAL */
//  if (NewState != DISABLE)
//  {
//    GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
//  }
//  else
//  {
//    GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
//  }
//#endif /* USE_STM3210C_EVAL */
  
#if defined(STM32L1XX_MD) || defined (STM32L1XX_HD)|| (STM32L1XX_MD_PLUS)
  if (NewState != DISABLE)
  {
    STM32L15_USB_CONNECT;
  }
  else
  {
    STM32L15_USB_DISCONNECT;
  }  
  
#else /* USE_STM3210B_EVAL or USE_STM3210E_EVAL */
	if (NewState != DISABLE)
	{
		//GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
		//GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
		USB_Connect_On();
	}
	else
	{
		//GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
		//GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
		USB_Connect_Off();

	}
#endif /* STM32L1XX_MD */  
  
}

///*******************************************************************************
//* Function Name  :  USART_Config_Default.
//* Description    :  configure the EVAL_COM1 with default values.
//* Input          :  None.
//* Return         :  None.
//*******************************************************************************/
//void USART_Config_Default(void)
//{
//  /* EVAL_COM1 default configuration */
//  /* EVAL_COM1 configured as follow:
//        - BaudRate = 9600 baud  
//        - Word Length = 8 Bits
//        - One Stop Bit
//        - Parity Odd
//        - Hardware flow control disabled
//        - Receive and transmit enabled
//  */
//  //USART_InitStructure.USART_BaudRate = 9600;
//  //USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//  //USART_InitStructure.USART_StopBits = USART_StopBits_1;
//  //USART_InitStructure.USART_Parity = USART_Parity_Odd;
//  //USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//  //USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
//
//  /* Configure and enable the USART */
//  //STM_EVAL_COMInit(COM1, &USART_InitStructure);
//
//  /* Enable the USART Receive interrupt */
//  //USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
//}
//
///*******************************************************************************
//* Function Name  :  USART_Config.
//* Description    :  Configure the EVAL_COM1 according to the line coding structure.
//* Input          :  None.
//* Return         :  Configuration status
//                    TRUE : configuration done with success
//                    FALSE : configuration aborted.
//*******************************************************************************/
//bool USART_Config(void)
//{
//
//  /* set the Stop bit*/
//  //switch (linecoding.format)
//  //{
//  //  case 0:
//  //    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//  //    break;
//  //  case 1:
//  //    USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
//  //    break;
//  //  case 2:
//  //    USART_InitStructure.USART_StopBits = USART_StopBits_2;
//  //    break;
//  //  default :
//  //  {
//  //    USART_Config_Default();
//  //    return (FALSE);
//  //  }
//  //}
//
//  ///* set the parity bit*/
//  //switch (linecoding.paritytype)
//  //{
//  //  case 0:
//  //    USART_InitStructure.USART_Parity = USART_Parity_No;
//  //    break;
//  //  case 1:
//  //    USART_InitStructure.USART_Parity = USART_Parity_Even;
//  //    break;
//  //  case 2:
//  //    USART_InitStructure.USART_Parity = USART_Parity_Odd;
//  //    break;
//  //  default :
//  //  {
//  //    USART_Config_Default();
//  //    return (FALSE);
//  //  }
//  //}
//
//  ///*set the data type : only 8bits and 9bits is supported */
//  //switch (linecoding.datatype)
//  //{
//  //  case 0x07:
//  //    /* With this configuration a parity (Even or Odd) should be set */
//  //    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//  //    break;
//  //  case 0x08:
//  //    if (USART_InitStructure.USART_Parity == USART_Parity_No)
//  //    {
//  //      USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//  //    }
//  //    else 
//  //    {
//  //      USART_InitStructure.USART_WordLength = USART_WordLength_9b;
//  //    }
//  //    
//  //    break;
//  //  default :
//  //  {
//  //    USART_Config_Default();
//  //    return (FALSE);
//  //  }
//  //}
//
//  //USART_InitStructure.USART_BaudRate = linecoding.bitrate;
//  //USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
// // USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
// 
//  /* Configure and enable the USART */
//  //STM_EVAL_COMInit(COM1, &USART_InitStructure);
//
//  return (TRUE);
//}
//
///*******************************************************************************
//* Function Name  : USB_To_USART_Send_Data.
//* Description    : send the received data from USB to the UART 0.
//* Input          : data_buffer: data address.
//                   Nb_bytes: number of bytes to send.
//* Return         : none.
//*******************************************************************************/
//void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes)
//{
//  
// /* uint32_t i;
//  
//  for (i = 0; i < Nb_bytes; i++)
//  {
//    USART_SendData(EVAL_COM1, *(data_buffer + i));
//    while(USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TXE) == RESET); 
//  }  */
//}
//
///*******************************************************************************
//* Function Name  : Handle_USBAsynchXfer.
//* Description    : send data to USB.
//* Input          : None.
//* Return         : none.
//*******************************************************************************/
//void Handle_USBAsynchXfer (void)
//{
//  
//  uint16_t USB_Tx_ptr;
//  uint16_t USB_Tx_length;
//  
//  if(USB_Tx_State != 1)
//  {
//    if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
//    {
//      USART_Rx_ptr_out = 0;
//    }
//    
//    if(USART_Rx_ptr_out == USART_Rx_ptr_in) 
//    {
//      USB_Tx_State = 0; 
//      return;
//    }
//    
//    if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
//    { 
//      USART_Rx_length = USART_RX_DATA_SIZE - USART_Rx_ptr_out;
//    }
//    else 
//    {
//      USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
//    }
//    
//    if (USART_Rx_length > VIRTUAL_COM_PORT_DATA_SIZE)
//    {
//      USB_Tx_ptr = USART_Rx_ptr_out;
//      USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;
//      
//      USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;	
//      USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;	
//    }
//    else
//    {
//      USB_Tx_ptr = USART_Rx_ptr_out;
//      USB_Tx_length = USART_Rx_length;
//      
//      USART_Rx_ptr_out += USART_Rx_length;
//      USART_Rx_length = 0;
//    }
//    USB_Tx_State = 1; 
//    
//#ifdef USE_STM3210C_EVAL
//    USB_SIL_Write(EP1_IN, &USART_Rx_Buffer[USB_Tx_ptr], USB_Tx_length);  
//#else
//    UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
//    SetEPTxCount(ENDP1, USB_Tx_length);
//    SetEPTxValid(ENDP1); 
//#endif /* USE_STM3210C_EVAL */
//  }  
//  
//}
///*******************************************************************************
//* Function Name  : UART_To_USB_Send_Data.
//* Description    : send the received data from UART 0 to USB.
//* Input          : None.
//* Return         : none.
//*******************************************************************************/
//void USART_To_USB_Send_Data(void)
//{
//  
//  if (linecoding.datatype == 7)
//  {
//    USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(EVAL_COM1) & 0x7F;
//  }
//  else if (linecoding.datatype == 8)
//  {
//    USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(EVAL_COM1);
//  }
//  
//  USART_Rx_ptr_in++;
//  
//  /* To avoid buffer overflow */
//  if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
//  {
//    USART_Rx_ptr_in = 0;
//  }
//}
//
///*******************************************************************************
//* Function Name  : Get_SerialNum.
//* Description    : Create the serial number string descriptor.
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//void Get_SerialNum(void)
//{
//  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;
//
//#ifdef STM32L1XX_MD
//  Device_Serial0 = *(uint32_t*)(0x1FF80050);
//  Device_Serial1 = *(uint32_t*)(0x1FF80054);
//  Device_Serial2 = *(uint32_t*)(0x1FF80064);
//#else  
//  Device_Serial0 = *(__IO uint32_t*)(0x1FFFF7E8);
//  Device_Serial1 = *(__IO uint32_t*)(0x1FFFF7EC);
//  Device_Serial2 = *(__IO uint32_t*)(0x1FFFF7F0);
//#endif /* STM32L1XX_MD */  
//
//  Device_Serial0 += Device_Serial2;
//
//  if (Device_Serial0 != 0)
//  {
//    IntToUnicode (Device_Serial0, &Virtual_Com_Port_StringSerial[2] , 8);
//    IntToUnicode (Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
//  }
//}
//
///*******************************************************************************
//* Function Name  : HexToChar.
//* Description    : Convert Hex 32Bits value into char.
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
//{
//  uint8_t idx = 0;
//  
//  for( idx = 0 ; idx < len ; idx ++)
//  {
//    if( ((value >> 28)) < 0xA )
//    {
//      pbuf[ 2* idx] = (value >> 28) + '0';
//    }
//    else
//    {
//      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
//    }
//    
//    value = value << 4;
//    
//    pbuf[ 2* idx + 1] = 0;
//  }
//}
//#ifdef STM32F10X_CL
///*******************************************************************************
//* Function Name  : USB_OTG_BSP_uDelay.
//* Description    : provide delay (usec).
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//void USB_OTG_BSP_uDelay (const uint32_t usec)
//{
//  RCC_ClocksTypeDef  RCC_Clocks;  
//
//  /* Configure HCLK clock as SysTick clock source */
//  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
//  
//  RCC_GetClocksFreq(&RCC_Clocks);
//  
//  SysTick_Config(usec * (RCC_Clocks.HCLK_Frequency / 1000000));  
//  
//  SysTick->CTRL  &= ~SysTick_CTRL_TICKINT_Msk ;
//  
//  while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
//}
//#endif /* STM32F10X_CL */
///******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/


/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration_1(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
   
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC|
						 RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOD, ENABLE);

  
  
  /* USB_DISCONNECT used as USB pull-up  PB1 */ 
  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);

//  //--------------------------------------------------
//  //copy from iap project
//	//output PA0->DP_DIR,
//	//GPIOA->BSRR=1<<(10); 
//	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_0;
//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP; //direct control ,READ FROM EV-CHIP
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//	GPIO_Init(GPIOA,&GPIO_InitStructure);
// 
//	//GPIOX_BSRR 高16位置1 清零对应位，低16位置1 置位对应位。
//	GPIOA->BSRR=1<<(16); //PA0=0       
//
//
//	//PB15->DI_DIR
//	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_15;
//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStructure);
//	GPIOB->BSRR=1<<(15); //set DI_DIR=1
//
//	//PB13,14 (IO5,IO6)
//	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_13|GPIO_Pin_14;
//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;	//push down,外部10K上拉与STM32内部下拉进行分压比
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStructure);
//
//	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==1)
//	{
//		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)==1)
//		{
//			//hardware v0.7
//			GPIO_InitStructure.GPIO_Pin= GPIO_Pin_13|GPIO_Pin_14;
//			GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;  //ex-push up
//			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//			GPIO_Init(GPIOB,&GPIO_InitStructure);
//
//			GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3|GPIO_Pin_4;
//			GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD; //ex-push up
//			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
//			GPIO_Init(GPIOA,&GPIO_InitStructure);	
//
//			//power_clr
//			GPIO_InitStructure.GPIO_Pin= GPIO_Pin_15;
//			GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP; //
//			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
//			GPIO_Init(GPIOC,&GPIO_InitStructure);
//
//			//GPIOX_BSRR 高16位置1 清零对应位，低16位置1 置位对应位。
//			GPIOC->BRR=1<<15;	//power_clr=0; BRR???
//
//			GPIOA->BSRR=1<<3;		//P03=1
//			GPIOA->BSRR=1<<4; 		//P04=1
//			GPIOB->BSRR=1<<(13) ;	//IO5=1
//			GPIOB->BSRR=1<<(14) ; 	//IO6=1;
//			vcc_3v();		
//                        //LEDgreen_ON;
//		}                                    
//                
//	}
/* 	else
	{
			//hardware v0.5
			//IO5,IO6 output 
			GPIO_InitStructure.GPIO_Pin= GPIO_Pin_13|GPIO_Pin_14;
			GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_Init(GPIOB,&GPIO_InitStructure);

			//P03,P04 input
			GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3|GPIO_Pin_4;
			GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD; //PUSH UP
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
			GPIO_Init(GPIOA,&GPIO_InitStructure);	
	} */


}

/*******************************************************************************
* Function Name : EXTI_Configuration.
* Description   : Configure the EXTI lines for Key and Tamper push buttons.
* Input         : None.
* Output        : None.
* Return value  : The direction value.
*******************************************************************************/
void EXTI_Configuration(void)
{
  EXTI_InitTypeDef EXTI_InitStructure;
  
#if defined (USE_STM32L152_EVAL)
  ///* Configure RIGHT EXTI line to generate an interrupt on rising & falling edges */  
  //EXTI_InitStructure.EXTI_Line = RIGHT_BUTTON_EXTI_LINE;
  //EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  //EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  //EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  //EXTI_Init(&EXTI_InitStructure);
  //
  ///* Clear the RIGHT EXTI line pending bit */
  //EXTI_ClearITPendingBit(RIGHT_BUTTON_EXTI_LINE);
  //
  ///* Configure LEFT EXTI Line to generate an interrupt rising & falling edges */  
  //EXTI_InitStructure.EXTI_Line = LEFT_BUTTON_EXTI_LINE;
  //EXTI_Init(&EXTI_InitStructure);
  //
  ///* Clear the LEFT EXTI line pending bit */
  //EXTI_ClearITPendingBit(LEFT_BUTTON_EXTI_LINE);
  
#else   

  //此段注释掉会影响USB的枚举结果，？？
  /* Configure Key EXTI line to generate an interrupt on rising & falling edges */  
  EXTI_InitStructure.EXTI_Line = KEY_BUTTON_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  /* Clear the Key EXTI line pending bit */
  EXTI_ClearITPendingBit(KEY_BUTTON_EXTI_LINE);
  
#endif /* USE_STM32L152_EVAL */  
  
  /* Configure the EXTI line 18 connected internally to the USB IP */
  EXTI_ClearITPendingBit(EXTI_Line18);
  EXTI_InitStructure.EXTI_Line = EXTI_Line18; 
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;
  
  Device_Serial0 = *(uint32_t*)(0x1FFFF7E8);
  Device_Serial1 = *(uint32_t*)(0x1FFFF7EC);
  Device_Serial2 = *(uint32_t*)(0x1FFFF7F0);
  
//  if(Device_Serial0 != 0)
//  {
//     CustomHID_StringSerial[2] = (uint8_t)(Device_Serial0 & 0x000000FF);  
//     CustomHID_StringSerial[4] = (uint8_t)((Device_Serial0 & 0x0000FF00) >> 8);
//     CustomHID_StringSerial[6] = (uint8_t)((Device_Serial0 & 0x00FF0000) >> 16);
//     CustomHID_StringSerial[8] = (uint8_t)((Device_Serial0 & 0xFF000000) >> 24);  
//     
//     CustomHID_StringSerial[10] = (uint8_t)(Device_Serial1 & 0x000000FF);  
//     CustomHID_StringSerial[12] = (uint8_t)((Device_Serial1 & 0x0000FF00) >> 8);
//     CustomHID_StringSerial[14] = (uint8_t)((Device_Serial1 & 0x00FF0000) >> 16);
//     CustomHID_StringSerial[16] = (uint8_t)((Device_Serial1 & 0xFF000000) >> 24); 
//     
//     CustomHID_StringSerial[18] = (uint8_t)(Device_Serial2 & 0x000000FF);  
//     CustomHID_StringSerial[20] = (uint8_t)((Device_Serial2 & 0x0000FF00) >> 8);
//     CustomHID_StringSerial[22] = (uint8_t)((Device_Serial2 & 0x00FF0000) >> 16);
//     CustomHID_StringSerial[24] = (uint8_t)((Device_Serial2 & 0xFF000000) >> 24); 
//  }   
  
//  Device_Serial0 = *(uint32_t*)ID1;
//  Device_Serial1 = *(uint32_t*)ID2;
//  Device_Serial2 = *(uint32_t*)ID3;
  
  Device_Serial0 += Device_Serial2;
  
  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &CustomHID_StringSerial[2] , 8);
    IntToUnicode (Device_Serial1, &CustomHID_StringSerial[18], 4);
  }
}

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/