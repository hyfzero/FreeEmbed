
#ifndef IRC_H
#define IRC_H

#define   Fsys    4    //  system freq 72MHz
#define   Tcp_ref 3600 // cp & Tref pwm


#define CP_0    GPIO_ResetBits(GPIOC,GPIO_Pin_6)
#define CP_1    GPIO_SetBits(GPIOC,GPIO_Pin_6) 

#define TIM_REF_0 GPIO_ResetBits(GPIOC,GPIO_Pin_7)
#define TIM_REF_1 GPIO_SetBits(GPIOC,GPIO_Pin_7) 

//#define DELAY_50US  for(i=0;i<396;i++)
//#define DELAY_49US  for(i=0;i<390;i++)
//#define TimeRef	    for(i=0;i<150;i++)
//#define DELAY_14US  for (i=0;i<141;i++)

#define DELAY_50US  for(i=0;i<325;i++)
#define DELAY_49US  for(i=0;i<390;i++)
#define TimeRef	    for(i=0;i<150;i++)
#define DELAY_14US  for (i=0;i<141;i++)

extern void ConfigTimer(void);
extern void ConfigTIM4();

extern void IRCInitial();
//extern u8 IRC_wave();
//extern u8 IRC_FTest(u16 MAXFRE,u16 MINFRE );


#endif