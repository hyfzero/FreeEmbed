
#ifndef lcd_h
#define lcd_h

//PIN39_RES ：复位，L：LCD复位，回到高电平后，液晶模块开始工作
//PIN41_SDA
//PIN42_SCK
//PIN5_RS:寄存器选择信号，H：数据，L:指令

#define LCD_RST_0    GPIO_ResetBits(GPIOE,GPIO_Pin_8)
#define LCD_RST_1    GPIO_SetBits(GPIOE,GPIO_Pin_8) 
#define LCD_SDA_0	 GPIO_ResetBits(GPIOE,GPIO_Pin_10)
#define LCD_SDA_1	 GPIO_SetBits(GPIOE,GPIO_Pin_10)
#define LCD_SCK_0	 GPIO_ResetBits(GPIOE,GPIO_Pin_11)
#define LCD_SCK_1	 GPIO_SetBits(GPIOE,GPIO_Pin_11)
#define LCD_RS_0	 GPIO_ResetBits(GPIOE,GPIO_Pin_6)
#define LCD_RS_1	 GPIO_SetBits(GPIOE,GPIO_Pin_6)


extern void transfer_data(u8 data1);
extern void transfer_command(u8 data1);
extern void Initial_Lcd();
extern void clear_screen();

#endif