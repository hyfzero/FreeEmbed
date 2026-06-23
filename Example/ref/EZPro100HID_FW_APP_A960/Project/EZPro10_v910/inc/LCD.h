
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

extern const u8 *pp[];
extern const u8 *pNum[];
extern const u8 graphic1[];
//extern const u8  graphic2[];

//extern const u8 lcd_gong[]; //工具 型号
//extern const u8 lcd_ju[];
extern const u8 lcd_xing[];
extern const u8 lcd_hao[];

//错误代码
extern const u8 lcd_cuo[];
extern const u8 lcd_wu[];
extern const u8 lcd_dai[];
extern const u8 lcd_ma[];

extern const u8 lcd_maohao[];
extern const u8 lcd_henggan[];
extern const u8 lcd_a[];
extern const u8 lcd_b[];
extern const u8 lcd_c[];
extern const u8 lcd_d[];
extern const u8 lcd_e[];
extern const u8 lcd_f[];
extern const u8 lcd_g[];
extern const u8 lcd_h1[];
extern const u8 lcd_i[];
extern const u8 lcd_j[];
extern const u8 lcd_k[];
extern const u8 lcd_l[];
extern const u8 lcd_m[];
extern const u8 lcd_n[];
extern const u8 lcd_o[];
extern const u8 lcd_p[];
extern const u8 lcd_q[];
extern const u8 lcd_r[];
extern const u8 lcd_s[];
extern const u8 lcd_t[];
extern const u8 lcd_u[];
extern const u8 lcd_v[];
extern const u8 lcd_w[];
extern const u8 lcd_x[];
extern const u8 lcd_y[];
extern const u8 lcd_z[];


extern const u8 lcd_A[];
extern const u8 lcd_B[];
extern const u8 lcd_C[];
extern const u8 lcd_D[];
extern const u8 lcd_E[];
extern const u8 lcd_F[];
extern const u8 lcd_G[];
extern const u8 lcd_H1[];
extern const u8 lcd_I[];
extern const u8 lcd_J[];
extern const u8 lcd_K[];
extern const u8 lcd_L[];
extern const u8 lcd_M[];
extern const u8 lcd_N[];
extern const u8 lcd_O[];
extern const u8 lcd_P[];
extern const u8 lcd_Q[];
extern const u8 lcd_R[];
extern const u8 lcd_S[];
extern const u8 lcd_T[];
extern const u8 lcd_U[];
extern const u8 lcd_V[];
extern const u8 lcd_W[];
extern const u8 lcd_X[];
extern const u8 lcd_Y[];
extern const u8 lcd_Z[];

extern const u8 lcd_0[];
extern const u8 lcd_1[];
extern const u8 lcd_2[];
extern const u8 lcd_3[];
extern const u8 lcd_4[];
extern const u8 lcd_5[];
extern const u8 lcd_6[];
extern const u8 lcd_7[];
extern const u8 lcd_8[];
extern const u8 lcd_9[];

extern const u8 lcd_point[];
extern const u8 lcd_blank[];
//char const graphic1[];
//char const graphic2[];
//char const graphic3[];
//char const graphic4[];
//char const graphic5[];
//char const graphic6[];
//char const graphic7[];
//char const graphic8[];
//char const graphic9[];
//char const graphic10[];

extern void displaygraphic(const u8 *dp);


extern void Initial_Lcd();
extern void clear_screen();
extern void clear_line(u8 page0);
extern void displayfuntion();
extern void displayTestMode();
extern void displayVDD_VPP(u32 vdd,u32 vpp);
extern void displayTestState(u8 stateNum);
extern void display_CheckSum_Option();
extern void displaWrite_Check();
extern void display_OK_NG(u32 okcounter,u8 ok_ng_flag);
extern void displayErorr(u8 erorrNum);
extern void display_dynamicID();
extern void display_Addr();
extern void displayTestResult(int FreqValue,u8 page0,u8 page1,u8 lineNumb);
extern void displayAZ(const u8 *dp,u8 page0,u8 page1,u8 lineNum);
extern void displayCHAR(const u8 *dp,u8 page0,u8 page1,u8 lineNum);
void display_CRC();

#endif