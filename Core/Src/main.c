#include "main.h"
#include "tim.h"
#include "gpio.h"

// mode global (buat switch pattern)
uint8_t mode = 0;

int main(void)
{
  HAL_Init();
  SystemClock_Config(); // buat setup clock internal
  MX_GPIO_Init();
  MX_TIM2_Init();
  HAL_TIM_Base_Start_IT(&htim2); // mulai timer dengan interrupt

  while (1)
  {
    // handling via interrupt/EXTI
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM       = 16;
  RCC_OscInitStruct.PLL.PLLN       = 336;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ       = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();  // error handling pas setup clock
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK
                                    | RCC_CLOCKTYPE_HCLK
                                    | RCC_CLOCKTYPE_PCLK1
                                    | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1; // hasil akhir 84MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // 42MHz buat APB1
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // 84MHz buat APB2
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    HAL_Delay(100);
  }
}
