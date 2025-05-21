#include "main.h"
#include "stm32f4xx_hal.h"

#define LED1_PIN   GPIO_PIN_5
#define LED2_PIN   GPIO_PIN_6
#define LED_PORT   GPIOA

#define BTN_PIN    GPIO_PIN_0
#define BTN_PORT   GPIOA

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

volatile uint8_t mode = 0;
volatile uint8_t step_custom = 0;

// Original patterns
uint32_t led1_fast[]   = {100, 100};
uint32_t led1_sos[]    = {100,100,100,300,300,300,100,100,100};
uint32_t led2_slow[]   = {1000, 1000};
uint32_t led2_custom[] = {300,100,50,100,50,100,200};

// Custom durations (200ms each)
#define CUSTOM_LEN1 2  // LED1 on/off
#define CUSTOM_LEN2 4  // LED2 on/off twice

// Variables for original mode
volatile uint8_t led1_mode=0, led2_mode=0;
volatile uint8_t led1_step=0, led2_step=0;
uint8_t led1_state=0;

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
  while (1) {}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last=0;
  if(GPIO_Pin==BTN_PIN && HAL_GetTick()-last>200) {
    mode = !mode;
    step_custom = 0;
    led1_step = led2_step = 0;
    last = HAL_GetTick();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(mode==1) {
    // Custom mode: both timers run at 200ms
    if(htim->Instance==TIM2) {
      // LED1 on/off
      uint8_t on = (step_custom % CUSTOM_LEN1 == 0);
      HAL_GPIO_WritePin(LED_PORT, LED1_PIN, on?GPIO_PIN_SET:GPIO_PIN_RESET);
      step_custom = (step_custom + 1) % (CUSTOM_LEN1 + CUSTOM_LEN2);
      __HAL_TIM_SET_AUTORELOAD(&htim2, 200-1);
      __HAL_TIM_SET_COUNTER(&htim2,0);
    }
    if(htim->Instance==TIM3) {
      // LED2 blink twice: steps 2,3,4,5
      uint8_t idx = (step_custom - CUSTOM_LEN1);
      if(step_custom>=CUSTOM_LEN1) {
        uint8_t on2 = (idx%2==0 && idx<CUSTOM_LEN2);
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN, on2?GPIO_PIN_SET:GPIO_PIN_RESET);
      } else {
        HAL_GPIO_WritePin(LED_PORT, LED2_PIN, GPIO_PIN_RESET);
      }
      __HAL_TIM_SET_AUTORELOAD(&htim3, 200-1);
      __HAL_TIM_SET_COUNTER(&htim3,0);
    }
  } else {
    // Normal mode
    if(htim->Instance==TIM2) {
      led1_state = !led1_state;
      HAL_GPIO_WritePin(LED_PORT, LED1_PIN, led1_state?GPIO_PIN_SET:GPIO_PIN_RESET);
      uint32_t next1 = (led1_mode==0? led1_fast[led1_step]:led1_sos[led1_step]);
      led1_step=(led1_step+1)% (led1_mode?9:2);
      __HAL_TIM_SET_AUTORELOAD(&htim2,next1-1);
      __HAL_TIM_SET_COUNTER(&htim2,0);
    }
    if(htim->Instance==TIM3) {
      uint8_t on = (led2_step%2);
      HAL_GPIO_WritePin(LED_PORT, LED2_PIN, on?GPIO_PIN_SET:GPIO_PIN_RESET);
      uint32_t next2 = (led2_mode==0?led2_slow[led2_step]:led2_custom[led2_step]);
      led2_step=(led2_step+1)% (led2_mode?7:2);
      __HAL_TIM_SET_AUTORELOAD(&htim3,next2-1);
      __HAL_TIM_SET_COUNTER(&htim3,0);
    }
  }
}

static void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance=TIM2;
  htim2.Init.Prescaler=16000-1;
  htim2.Init.CounterMode=TIM_COUNTERMODE_UP;
  htim2.Init.Period=200-1;
  HAL_TIM_Base_Init(&htim2);
  HAL_NVIC_SetPriority(TIM2_IRQn,0,0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_TIM3_Init(void)
{
  __HAL_RCC_TIM3_CLK_ENABLE();
  htim3.Instance=TIM3;
  htim3.Init.Prescaler=16000-1;
  htim3.Init.CounterMode=TIM_COUNTERMODE_UP;
  htim3.Init.Period=200-1;
  HAL_TIM_Base_Init(&htim3);
  HAL_NVIC_SetPriority(TIM3_IRQn,1,0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  GPIO_InitTypeDef g={0};
  g.Pin=LED1_PIN|LED2_PIN; g.Mode=GPIO_MODE_OUTPUT_PP;
  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT,&g);
  g.Pin=BTN_PIN; g.Mode=GPIO_MODE_IT_RISING; g.Pull=GPIO_PULLDOWN;
  HAL_GPIO_Init(BTN_PORT,&g);
  HAL_NVIC_SetPriority(EXTI0_IRQn,2,0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef o={0}; RCC_ClkInitTypeDef c={0};
  __HAL_RCC_PWR_CLK_ENABLE();
  o.OscillatorType=RCC_OSCILLATORTYPE_HSI; o.HSIState=RCC_HSI_ON; o.PLL.PLLState=RCC_PLL_NONE;
  HAL_RCC_OscConfig(&o);
  c.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  c.SYSCLKSource=RCC_SYSCLKSOURCE_HSI; c.AHBCLKDivider=RCC_SYSCLK_DIV1;
  c.APB1CLKDivider=RCC_HCLK_DIV1; c.APB2CLKDivider=RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&c,FLASH_LATENCY_0);
}
