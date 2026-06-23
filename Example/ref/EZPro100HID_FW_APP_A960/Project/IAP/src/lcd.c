
#include "stm32f10x.h"

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#include "stm32f10x_tim.h"
#include "global.h"
#include "LCD.h"

#define DELAY_14US  for (i=0;i<141;i++)


/*初始化子程序*/
void Initial_Lcd()
{
        u8 i;
	GPIO_InitTypeDef GPIO_InitStructure;

	//PIN39_RES ：复位，L：LCD复位，回到高电平后，液晶模块开始工作
	//PIN40_SDA
	//PIN41_SCK
	//PIN42_RS:寄存器选择信号，H：数据，L:指令

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_8|GPIO_Pin_10|GPIO_Pin_11;

	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;// GPIO_Mode_Out_OD; // 开漏输出 
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(GPIOE, &GPIO_InitStructure);

	LCD_RST_1;
	LCD_SCK_1;
	Delay_100us(1);
        //reset=0;                 //Reset the chip when reset=0 
		LCD_RST_0;
        //delay(20);
		Delay_1ms(1);
        //reset=1;
		LCD_RST_1;
		DELAY_14US;
	transfer_command(0xe2);		/*软复位*/
	transfer_command(0x2c);		/*升压步聚1*/
	DELAY_14US;//Delay_100us(1); 
	transfer_command(0x2e);		/*升压步聚2*/
	DELAY_14US;//Delay_100us(1);
	transfer_command(0x2f);		/*升压步聚3*/
	DELAY_14US;//Delay_100us(1);
	transfer_command(0x23);		/*粗调对比度，可设置范围20～27*/
	transfer_command(0x81);		/*微调对比度*/
	transfer_command(0x27);		/*微调对比度的值，可设置范围0～63*/
	transfer_command(0xa2);		/*1/9偏压比（bias）*/
	transfer_command(0xc8);		/*行扫描顺序：从上到下*/
	transfer_command(0xa0);		/*列扫描顺序：从左到右*/
	transfer_command(0x60);		/*起始行：从第一行开始*/
	transfer_command(0xaf);		/*开显示*/


}



//===============clear all dot martrics=============
void clear_screen()
{
   u8 i,j;
       
 	for(i=0;i<9;i++)
        {
        	//cs1=0;
		
		transfer_command(0xb0+i);
		transfer_command(0x10);
		transfer_command(0x01);
		for(j=0;j<128;j++)
		{
	        	transfer_data(0x00);
		}
         }
}

/*=========写指令===============*/
void transfer_command( u8 data1)   
{
	u8 i;
	//cs1=0;
	//rs=0;
	LCD_RS_0;
	DELAY_14US;
	for(i=0;i<8;i++)
	{
		//sclk=0;
		LCD_SCK_0;
	    if(data1&0x80) LCD_SDA_1;//sid=1;
	    else LCD_SDA_0;//sid=0;
		//delay1(1);
		Delay_100us(1);
		//sclk=1;
		LCD_SCK_1;
		Delay_100us(1);
	 	data1=data1<<1;
 		
	  }
	  LCD_SCK_1;

}

/*-------写数据---------------*/
void transfer_data(u8 data1)
{
	u8 i;
	//cs1=0;
	//rs=1;
	LCD_RS_1;
	DELAY_14US;
	for(i=0;i<8;i++)
	       {
		//sclk=0;
		LCD_SCK_0;
		if(data1&0x80) LCD_SDA_1;//sid=1;
		else LCD_SDA_0;//sid=0;
		LCD_SCK_1;
		//sclk=1;		
	 	data1=data1<<1; 		
	        }
	LCD_SCK_1;

}


