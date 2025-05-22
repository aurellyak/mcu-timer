#include "gpio.h"

// setup awal GPIOA: PA5 & PA6 buat LED, PA0 buat tombol mode
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // nyalain clock buat GPIOA
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // setup LED1 (PA5) & LED2 (PA6) sebagai output push-pull
  GPIO_InitStruct.Pin   = LED1_Pin | LED2_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  // setup tombol mode (PA0) buat interrupt rising edge
  GPIO_InitStruct.Pin   = MODE_BTN_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(MODE_BTN_Port, &GPIO_InitStruct);

  // aktifin interrupt EXTI0 (buat PA0)
  HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
