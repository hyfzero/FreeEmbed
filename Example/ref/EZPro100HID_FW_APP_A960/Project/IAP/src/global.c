
#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "global.h"
#include "power.h"

#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "stm32_eval.h"
#include "IAP.h"


u16 USB_RxDataCS;
u8  USB_RxCommand;

u8 USB_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];

u8 USB_Rx_Flag;
u8 StateFlag=0xfe;

//用户程序Flash偏移
u32 FlashDestination = ApplicationAddress;
u32 PageSize = PAGE_SIZE;
u32 EraseCounter = 0x0;
u32 NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
u32 RamSource;


/*
typedef enum {

	LED1 = 0,
	LED2,
	LED3,
	LED4,
	LED5,

}LED_STATE;

LED_STATE led_status;

**/

/**

* @brief  Configures the different system clocks.

* @param  None

* @retval : None

*/

void RCC_Configuration(void)

{

	/* Setup the microcontroller system. Initialize the Embedded Flash 

	Interface, initialize the PLL and update the SystemFrequency variable. */

	SystemInit();

	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);

	//mco initial
	/*
		RCC_MCO_NoClock----无时钟输出
		RCC_MCO_SYSCLK-----输出系统时钟
		RCC_MCO_HSI--------输出内部高 8MHZ
		RCC_MCO_HSE--------输出高速外部时钟信号
		RCC_MCO_PLLCLK_DIV2--输出 PLL倍频后的二分频时钟
	*/
}

/**

* @brief  Configure the GPIOD Pins.

* @param  None

* @retval : None

*/

void GPIO_Configuration(void)

{

	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOF configuration: PC6 ->OSCO(CP),PC7 ->OSCI(TIME_REF), */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11 |GPIO_Pin_12;

	//GPIO_InitStructure.GPIO_Pin = ; //PC11->LEDNG,PC12 ->LEDBUSY

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_6|GPIO_Pin_13; //pc13 as CLN_VDDtoGND
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	
	//CLN_VDD2GND_Off;

	// select KEY
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU; // input push up
	GPIO_InitStructure.GPIO_Speed =GPIO_Speed_2MHz;

	GPIO_Init(GPIOD,&GPIO_InitStructure);


	//PB1 as USB pushup
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);


    //USB_REG_On;
}

void Freq(u8 Numb)
{
	if (Numb==1)
	{
		LEDNG_Off;
		LEDOK_On;
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(30);
		GPIO_ResetBits(GPIOA,GPIO_Pin_10);
	} 
	else
	{
		LEDOK_Off;
		LEDNG_On;
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(10);
		GPIO_ResetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(20);
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
		Delay_1ms(20);
		GPIO_ResetBits(GPIOA,GPIO_Pin_10);

	}
}

void Delay_100us(u16 cnt )
{
	u16 i,a;
	
	for (a=0 ;a<cnt;a++)
	{
		for(i=800;i>0;i--);
	}

}

void Delay_1ms(u16 cnt )
{
	u16 i,a;

	for (a=0 ;a<cnt;a++)
	{
		for(i=8500;i>0;i--);
	}

}