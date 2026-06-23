
#ifndef power_h
#define power_h

//power value
#define vpp12  1
#define vpp00  0

#define vdd65  1
#define vdd30  2
#define vdd_adjust  2
#define vdd00  0

#define Power18V_On	  GPIO_SetBits(GPIOA,GPIO_Pin_5)
#define Power18V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_5)

#define VPP12V_On	GPIO_SetBits(GPIOA,GPIO_Pin_9)//可调的VPP
#define VPP12V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_9)

#define VDD30V_On	GPIO_SetBits(GPIOA,GPIO_Pin_0)//可调的VDD
#define VDD30V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_0)

#define VDD65V_On	GPIO_SetBits(GPIOE,GPIO_Pin_15)//固定VDD
#define VDD65V_Off  GPIO_ResetBits(GPIOE,GPIO_Pin_15)

#define IO5V_On	GPIO_SetBits(GPIOE,GPIO_Pin_14)         //5V
#define IO5V_Off  GPIO_ResetBits(GPIOE,GPIO_Pin_14)     //3.3V

#define VDD_IO_On   GPIO_SetBits(GPIOC,GPIO_Pin_6)      //OSCO
#define VDD_IO_Off   GPIO_ResetBits(GPIOC,GPIO_Pin_6)

#define VPP_IO_On   GPIO_SetBits(GPIOC,GPIO_Pin_7)      //OSCI
#define VPP_IO_Off   GPIO_ResetBits(GPIOC,GPIO_Pin_7)


//power value
#define vpp85   1       //8.5V
#define vpp00   0

#define vdd50   1       //5.0V
#define vdd00   0

#define VPP_On  GPIO_SetBits(GPIOA,GPIO_Pin_9)
#define VPP_Off GPIO_ResetBits(GPIOA,GPIO_Pin_9)

#define VDD_On  GPIO_SetBits(GPIOA,GPIO_Pin_0)
#define VDD_Off GPIO_ResetBits(GPIOA,GPIO_Pin_0)

extern void PowerInitial();
extern void POWER_ON(u8 vpp,u8 vdd);
extern void POWER_OFF(u8 vpp,u8 vdd);

extern void testPower();



#endif