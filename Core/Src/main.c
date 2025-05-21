#include "main.h"
#include "stm32f4xx_hal.h"

#define LED1_PIN   GPIO_PIN_5
#define LED2_PIN   GPIO_PIN_6
#define LED_PORT   GPIOA

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

TIM_HandleTypeDef htim2;

volatile uint8_t pattern_mode = 0;
volatile uint8_t pattern_step = 0;

// Mode 0 sequences (normal)
uint32_t sequence1_mode0[] = {100, 100};
uint32_t sequence1_mode1[] = {100, 100, 100, 300, 300, 300, 100, 100, 100};
uint32_t sequence2_mode0[] = {1000, 1000};
uint32_t sequence2_mode1[] = {300, 100, 50, 100, 50, 100, 200};

// Mode 1 combined cycle: LED1 and LED2 integrated
#define CYCLE_LEN 10
const uint32_t cycle_duration[CYCLE_LEN] = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200};
const uint8_t  cycle_led1[CYCLE_LEN]    = {1,   0,   0,   0,   0,   1,   0,   0,   0,   0};
const uint8_t  cycle_led2[CYCLE_LEN]    = {0,   0,   1,   0,   1,   0,   0,   1,   0,   1};

// State for normal mode
volatile uint8_t seq1_idx = 0;
volatile uint8_t seq2_idx = 0;
uint8_t seq1_state = 0;

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
        pattern_mode = !pattern_mode;
        pattern_step = 0;
        seq1_idx = 0;
        seq2_idx = 0;
        last_tick = HAL_GetTick();
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    if (pattern_mode == 1) {
        // Integrated cycle mode
        HAL_GPIO_WritePin(LED_PORT, LED1_PIN, cycle_led1[pattern_step] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN, cycle_led2[pattern_step] ? GPIO_PIN_SET : GPIO_PIN_RESET);

        uint32_t next = cycle_duration[pattern_step];
        pattern_step = (pattern_step + 1) % CYCLE_LEN;
        __HAL_TIM_SET_AUTORELOAD(&htim2, next - 1);

    } else {
        // Normal independent sequences
        // LED1 sequence
        seq1_state = !seq1_state;
        HAL_GPIO_WritePin(LED_PORT, LED1_PIN, seq1_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        uint32_t dur1 = (seq1_state 
                         ? sequence1_mode0[seq1_idx] 
                         : sequence1_mode1[seq1_idx]);
        seq1_idx = (seq1_idx + 1) % (sequence1_mode0[seq1_idx] ? 2 : 9);

        // LED2 sequence
        uint8_t led2_on = (seq2_idx % 2 == 1);
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN, led2_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        uint32_t dur2 = (seq2_idx % 2
                         ? sequence2_mode0[seq2_idx]
                         : sequence2_mode1[seq2_idx]);
        seq2_idx = (seq2_idx + 1) % (sequence2_mode0[seq2_idx] ? 2 : 7);

        uint32_t next = (dur1 < dur2) ? dur1 : dur2;
        __HAL_TIM_SET_AUTORELOAD(&htim2, next - 1);
    }
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

static void MX_TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 200 - 1;
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

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
