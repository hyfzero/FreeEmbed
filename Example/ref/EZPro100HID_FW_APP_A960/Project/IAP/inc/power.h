
#ifndef power_h
#define power_h

//power value
#define vpp12  1
#define vpp00  0

#define vdd65  1
#define vdd30  2
#define vdd00  0

#define Power18V_On	  GPIO_SetBits(GPIOA,GPIO_Pin_5)
#define Power18V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_5)

#define VPP12V_On	GPIO_SetBits(GPIOA,GPIO_Pin_9)
#define VPP12V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_9)

#define VDD30V_On	GPIO_SetBits(GPIOA,GPIO_Pin_0)
#define VDD30V_Off  GPIO_ResetBits(GPIOA,GPIO_Pin_0)

#define VDD65V_On	GPIO_SetBits(GPIOE,GPIO_Pin_15)
#define VDD65V_Off  GPIO_ResetBits(GPIOE,GPIO_Pin_15)

#define IO5V_On	GPIO_SetBits(GPIOE,GPIO_Pin_14)
#define IO5V_Off  GPIO_ResetBits(GPIOE,GPIO_Pin_14)

extern void PowerInitial();
extern void POWER_ON(u8 vpp,u8 vdd);
extern void POWER_OFF(u8 vpp,u8 vdd);

extern void testPower();



#endif