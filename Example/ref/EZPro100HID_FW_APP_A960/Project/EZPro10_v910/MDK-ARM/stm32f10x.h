#ifndef KEIL_STM32F10X_HD_WRAPPER_H
#define KEIL_STM32F10X_HD_WRAPPER_H

/*
 * Keil's STM32F103ZG device selection injects the XL-density macro.
 * The original IAR APP project is built with STM32F10X_HD, and the old
 * StdPeriph header cannot compile if HD and XL IRQ lists are both enabled.
 */
#ifdef STM32F10X_XL
#undef STM32F10X_XL
#endif

#include "..\..\..\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\stm32f10x.h"

#endif
