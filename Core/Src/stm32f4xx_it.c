#include "main.h"
#include "stm32f4xx_it.h"
#include "tim.h"
#include "gpio.h"

extern uint8_t mode; // variabel mode, didefinisiin di main.c

// counter milidetik
static volatile uint32_t ms_tick = 0;

// buat nyimpen waktu LED di MODE 0
static uint32_t tick_led1_m0 = 0;
static uint32_t tick_led2_m0 = 0;

// buat nyimpen status LED di MODE 1
static uint32_t tick_led1_m1   = 0;
static uint32_t tick_led2_m1   = 0;
static uint8_t  led1_on_state  = 1;
static uint8_t  led2_phase     = 0;

// handler buat SysTick
void SysTick_Handler(void)
{
  HAL_IncTick();
}

// handler buat interrupt TIM2
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

// handler buat interrupt eksternal dari pin PA0
void EXTI0_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(MODE_BTN_Pin);
}

// callback pas tombol ditekan (EXTI): ganti mode
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == MODE_BTN_Pin)
  {
    mode ^= 1; // toggle mode 0 ↔ 1

    // reset semua timer & state tergantung mode baru
    ms_tick = ms_tick; // biar timing tetap konsisten

    if (mode == 0)
    {
      // reset timer buat MODE 0
      tick_led1_m0 = ms_tick;
      tick_led2_m0 = ms_tick;
    }
    else
    {
      // reset state awal buat MODE 1
      led1_on_state = 1;
      tick_led1_m1  = ms_tick;
      // pastiin LED2 dalam kondisi mati saat awal
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    }

    // matiin dua-duanya pas pergantian mode
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
  }
}

// callback dari timer TIM2 (dipanggil tiap 1ms)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM2) return;

  ms_tick++;

  if (mode == 0)
  {
    // MODE 0: LED1 nyala-mati tiap 200ms
    if (ms_tick - tick_led1_m0 >= 200)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
      tick_led1_m0 = ms_tick;
    }

    // MODE 0: LED2 nyala-mati tiap 1000ms
    if (ms_tick - tick_led2_m0 >= 1000)
    {
      HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
      tick_led2_m0 = ms_tick;
    }
  }
  else
  {
    // MODE 1: LED1 nyala 200ms, terus LED2 blinking 2x (fase 0–4, 200ms per fase)
    if (led1_on_state)
    {
      // LED1 nyala
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);

      if (ms_tick - tick_led1_m1 >= 200)
      {
        // setelah 200ms, matiin LED1 dan mulai blinking LED2
        led1_on_state = 0;
        tick_led2_m1  = ms_tick;
        led2_phase    = 0;
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
      }
    }
    else
    {
      // LED1 mati, masuk ke fase blinking LED2
      if (ms_tick - tick_led2_m1 >= 200)
      {
        tick_led2_m1 = ms_tick;

        // nyalain atau matiin LED2 tiap 200ms tergantung fase
        if (led2_phase % 2 == 1)
          HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        else
          HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

        led2_phase++;

        if (led2_phase >= 5)
        {
          // selesai 2x blink, balik ke LED1 nyala
          led1_on_state = 1;
          tick_led1_m1  = ms_tick;
          HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        }
      }
    }
  }
}
