#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"

// definisi pin yang dipake
#define LED1_Pin        GPIO_PIN_5
#define LED1_GPIO_Port  GPIOA
#define LED2_Pin        GPIO_PIN_6
#define LED2_GPIO_Port  GPIOA
#define MODE_BTN_Pin    GPIO_PIN_0
#define MODE_BTN_Port   GPIOA

// variabel global buat nyimpen mode aktif sekarang
extern uint8_t mode;

// deklarasi fungsi
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* __MAIN_H */
