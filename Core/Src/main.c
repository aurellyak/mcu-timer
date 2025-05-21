#include "main.h"
#include "stm32f4xx_hal.h"

#define LED1_PIN   GPIO_PIN_5
#define LED2_PIN   GPIO_PIN_6
#define LED_PORT   GPIOA

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

TIM_HandleTypeDef htim2;

volatile uint8_t mode = 0;        // 0 or 1
volatile uint8_t step = 0;

// Define a 4-step cycle for each mode and each LED
#define STEPS      4

// durations[mode][step] in ms
const uint32_t durations[2][STEPS] = {
    {200, 800, 200, 800},  // mode 0: LED1 on 200/off800, LED2 same
    {400, 400, 400, 400}   // mode 1: equal on/off for both
};

// led1_pattern[mode][step]
const uint8_t led1_pattern[2][STEPS] = {
    {1, 0, 1, 0},          // mode 0: on, off, on, off
    {1, 1, 0, 0}           // mode 1: two on, two off
};

// led2_pattern[mode][step]
const uint8_t led2_pattern[2][STEPS] = {
    {0, 1, 0, 1},          // mode 0: off,on,off,on
    {0, 0, 1, 1}           // mode 1: two off, two on
};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start_IT(&htim2);
    while (1) {
        // Idle
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last = 0;
    if (GPIO_Pin == BTN_PIN && HAL_GetTick() - last > 200) {
        mode ^= 1;       // toggle mode
        step = 0;
        last = HAL_GetTick();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    // Set LEDs based on current mode and step
    HAL_GPIO_WritePin(LED_PORT, LED1_PIN, led1_pattern[mode][step] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED2_PIN, led2_pattern[mode][step] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Schedule next step
    uint32_t next = durations[mode][step];
    step = (step + 1) % STEPS;
    __HAL_TIM_SET_AUTORELOAD(&htim2, next - 1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

static void MX_TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;  // 1ms tick
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = durations[0][0] - 1;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    // LEDs
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

    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
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

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
