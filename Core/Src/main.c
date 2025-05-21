#include "main.h"
#include "stm32f4xx_hal.h"

#define LED1_PIN   GPIO_PIN_5
#define LED2_PIN   GPIO_PIN_6
#define LED_PORT   GPIOA

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

TIM_HandleTypeDef htim2;

volatile uint8_t mode = 0;           // 0 = normal, 1 = custom cycle
volatile uint8_t step_custom = 0;

// Normal patterns
uint32_t led1_fast[]   = {100, 100};
uint32_t led1_sos[]    = {100, 100, 100, 300, 300, 300, 100, 100, 100};
uint32_t led2_slow[]   = {1000, 1000};
uint32_t led2_custom[] = {300, 100, 50, 100, 50, 100, 200};

// Custom cycle: LED1 on-off, LED2 blink twice, LED1 on-off, each 200ms
#define CUSTOM_STEPS 8
const uint32_t custom_dur[CUSTOM_STEPS] = {200,200, 200,200, 200,200, 200,200};
const uint8_t  custom_led1[CUSTOM_STEPS] = {1,0, 0,0, 0,0, 1,0};
const uint8_t  custom_led2[CUSTOM_STEPS] = {0,0, 1,0, 1,0, 0,0};

// Variables for normal mode
volatile uint8_t led1_mode = 0;
volatile uint8_t led2_mode = 0;
volatile uint8_t led1_step = 0;
volatile uint8_t led2_step = 0;
uint8_t led1_state = 0;
uint32_t led1_next = 100;
uint32_t led2_next = 1000;

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
    while (1) {}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_tick = 0;
    if (GPIO_Pin == BTN_PIN && HAL_GetTick() - last_tick > 200) {
        mode = !mode;              // toggle modes
        step_custom = 0;
        led1_step = led2_step = 0;
        last_tick = HAL_GetTick();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (mode == 1) {
            // Custom sequence
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, custom_led1[step_custom] ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, custom_led2[step_custom] ? GPIO_PIN_SET : GPIO_PIN_RESET);

            uint32_t next = custom_dur[step_custom];
            step_custom = (step_custom + 1) % CUSTOM_STEPS;
            __HAL_TIM_SET_AUTORELOAD(&htim2, next - 1);
        } else {
            // Normal mode: LED1 on TIM2, LED2 on TIM2 as well
            // LED1 blinking
            led1_state = !led1_state;
            HAL_GPIO_WritePin(LED_PORT, LED1_PIN, led1_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
            led1_next = ((led1_mode == 0) ? led1_fast[led1_step] : led1_sos[led1_step]);
            led1_step = (led1_step + 1) % ((led1_mode == 0) ? 2 : 9);

            // LED2 blinking
            uint8_t on = (led2_step % 2 == 1);
            HAL_GPIO_WritePin(LED_PORT, LED2_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
            led2_next = ((led2_mode == 0) ? led2_slow[led2_step] : led2_custom[led2_step]);
            led2_step = (led2_step + 1) % ((led2_mode == 0) ? 2 : 7);

            // choose shorter next period of two for consistency
            uint32_t min_next = (led1_next < led2_next) ? led1_next : led2_next;
            __HAL_TIM_SET_AUTORELOAD(&htim2, min_next - 1);
        }
        __HAL_TIM_SET_COUNTER(&htim2, 0);
    }
}

static void MX_TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;  // 1ms tick
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 200 - 1;       // initial 200ms
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED1_PIN | LED2_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);

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

    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
