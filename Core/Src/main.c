#include "main.h"
#include "stm32f4xx_hal.h"

#define LED1_PIN   GPIO_PIN_5
#define LED2_PIN   GPIO_PIN_6
#define LED_PORT   GPIOA

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

volatile uint8_t led1_mode = 0;
volatile uint8_t led2_mode = 0;

volatile uint8_t led1_step = 0;
volatile uint8_t led2_step = 0;

uint32_t led1_fast[]  = {100, 100};
uint32_t led1_sos[]   = {100, 100, 100, 300, 300, 300, 100, 100, 100};
uint32_t led2_slow[]  = {1000, 1000};
uint32_t led2_custom[] = {300, 100, 50, 100, 50, 100, 200};

uint32_t led1_next = 100;
uint32_t led2_next = 1000;

uint8_t led1_state = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);

  while (1)
  {
    // idle loop
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_tick = 0;
  if (GPIO_Pin == BTN_PIN && HAL_GetTick() - last_tick > 200) {
    led1_mode = !led1_mode;
    led2_mode = !led2_mode;
    led1_step = 0;
    led2_step = 0;
    last_tick = HAL_GetTick();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    const uint32_t* pattern1 = (led1_mode == 0) ? led1_fast : led1_sos;
    uint8_t len1 = (led1_mode == 0) ? 2 : 9;

    led1_state = !led1_state;
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, led1_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    led1_next = pattern1[led1_step];
    led1_step = (led1_step + 1) % len1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, led1_next - 1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
  }

  if (htim->Instance == TIM3) {
    const uint32_t* pattern2 = (led2_mode == 0) ? led2_slow : led2_custom;
    uint8_t len2 = (led2_mode == 0) ? 2 : 7;

    if (led2_step % 2 == 1)
      HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);

    led2_next = pattern2[led2_step];
    led2_step = (led2_step + 1) % len2;

    __HAL_TIM_SET_AUTORELOAD(&htim3, led2_next - 1);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
  }
}

static void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 16000 - 1;  // 1ms tick
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100 - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_Base_Init(&htim2);

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_TIM3_Init(void)
{
  __HAL_RCC_TIM3_CLK_ENABLE();
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 16000 - 1;  // 1ms tick
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000 - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_Base_Init(&htim3);

  HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};

  // LED1 & LED2
  gpio.Pin = LED1_PIN | LED2_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &gpio);

  // Button
  gpio.Pin = BTN_PIN;
  gpio.Mode = GPIO_MODE_IT_RISING;
  gpio.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BTN_PORT, &gpio);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  __HAL_RCC_PWR_CLK_ENABLE();

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState = RCC_HSI_ON;
  osc.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&osc);

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
