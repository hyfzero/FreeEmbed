/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_endp.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Endpoint routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "Config.h"
#include "global.h"
#include "spi.h"
#include "OTPRW.h"
#include "power.h"

#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"

uint8_t Receive_Buffer[2];

///* Private typedef -----------------------------------------------------------*/
///* Private define ------------------------------------------------------------*/
//
///* Interval between sending IN packets in frame number (1 frame = 1ms) */
//#define VCOMPORT_IN_FRAME_INTERVAL             5
//
///* Private macro -------------------------------------------------------------*/
///* Private variables ---------------------------------------------------------*/
//
//extern  uint8_t USART_Rx_Buffer[];
//extern uint32_t USART_Rx_ptr_out;
//extern uint32_t USART_Rx_length;
//extern uint8_t  USB_Tx_State;
//
///* Private function prototypes -----------------------------------------------*/
///* Private functions ---------------------------------------------------------*/
//
///*******************************************************************************
//* Function Name  : EP1_IN_Callback
//* Description    :
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//void EP1_IN_Callback (void)
//{
//
//}
//
///*******************************************************************************
//* Function Name  : EP3_OUT_Callback
//* Description    :
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//void EP3_OUT_Callback(void)
//{
// // uint16_t USB_Rx_Cnt;
//  
//  /* Get the received data buffer and update the counter */
//  //USB_Rx_Cnt = USB_SIL_Read(EP3_OUT, USB_Rx_Buffer);
//  USB_SIL_Read(EP3_OUT, USB_Rx_Buffer);
//  /* USB data will be immediately processed, this allow next USB traffic being 
//  NAKed till the end of the USART Xfer */
//  
//  //USB_To_USART_Send_Data(USB_Rx_Buffer, USB_Rx_Cnt);
//   USB_Rx_Flag=0xaa;
//   
////#ifndef STM32F10X_CL
//  /* Enable the receive of data on EP3 */
////   SetEPRxValid(ENDP3);
////#endif /* STM32F10X_CL */
//}
//
//
///*******************************************************************************
//* Function Name  : SOF_Callback / INTR_SOFINTR_Callback
//* Description    :
//* Input          : None.
//* Output         : None.
//* Return         : None.
//*******************************************************************************/
//#ifdef STM32F10X_CL
//void INTR_SOFINTR_Callback(void)
//#else
//void SOF_Callback(void)
//#endif /* STM32F10X_CL */
//{
//  static uint32_t FrameCount = 0;
//  
//  if(bDeviceState == CONFIGURED)
//  {
//    if (FrameCount++ == VCOMPORT_IN_FRAME_INTERVAL)
//    {
//      /* Reset the frame counter */
//      FrameCount = 0;
//      
//      /* Check the data to be sent through IN pipe */
//      Handle_USBAsynchXfer();
//    }
//  }  
//}
///******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : EP1_OUT_Callback.
* Description    : EP1 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_OUT_Callback(void)
{
  

  /* Read received data (64 bytes) */  
  USB_SIL_Read(EP1_OUT, USB_Rx_Buffer);
 
  if (USB_Rx_Buffer[0] == 0x68)
  {
    USB_Rx_Flag=0xaa;
  }
  else
  {
    USB_Rx_Flag=0x00;
  }

  SetEPRxStatus(ENDP1, EP_RX_VALID);
 
}

/*******************************************************************************
* Function Name  : EP1_IN_Callback.
* Description    : EP1 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback(void)
{
//  PrevXferComplete = 1;

  /* Write the descriptor through the endpoint */
  //USB_SIL_Write(EP1_IN, (uint8_t*)Send_Buffer, 2);

  //SetEPTxValid(ENDP1);

  //PrevXferComplete = 0;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

