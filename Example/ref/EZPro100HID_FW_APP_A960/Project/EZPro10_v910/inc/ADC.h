
#ifndef ADC_H
#define ADC_H

#define ADC1_DR_Address    ((u32)0x4001244C)


extern void ADC_Config();
//extern void DMA_Config(void);
extern u8 TestAdc(void);

#endif