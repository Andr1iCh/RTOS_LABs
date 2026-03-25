/* USER CODE BEGIN Header */
/**
  * 3. Запрограмувати передачу рядка «Test3» через USART6 (PC6 — TX)
  * з інтервалом 2 сек. Реалізувати відображення прийнятого рядка у вигляді сигналів
  * за таблицею Морзе на світлодіоді PA5.
  * USART6 передає, USART1 (PB7 — RX) приймає.
  * Реалізація: CMSIS (регістри) + State Machine (автомат станів).
  */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN PV */
typedef enum {
    APP_STATE_IDLE,
    APP_STATE_TRANSMITTING,
    APP_STATE_MORSE_PROCESS,
    APP_STATE_MORSE_ON,
    APP_STATE_MORSE_OFF,
    APP_STATE_MORSE_SPACE
} MorseState;

// Змінні автомата станів
MorseState currentState = APP_STATE_IDLE;
uint32_t stateTimer = 0;
uint32_t txTimer = 0;

// Дані
const char txMsg[] = "Test3";
uint8_t txLen = 5;
uint8_t txIndex = 0;

char rxBuffer[20];
uint8_t rxIndex = 0;
uint8_t processIndex = 0;

// Налаштування Морзе
const char* currentMorse = "";
uint8_t symbolIndex = 0;
const uint32_t DOT_TIME = 25;
const uint32_t DASH_TIME = 75;
const uint32_t SYMBOL_SPACE = 30;
const uint32_t LETTER_SPACE = 100;
/* USER CODE END PV */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);

/* USER CODE BEGIN 0 */
const char* getMorseCode(char c) {
    switch(c) {
        case 'T': case 't': return "-";
        case 'E': case 'e': return ".";
        case 'S': case 's': return "...";
        case '3': return "...--";
        default: return "";
    }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();


  txTimer = HAL_GetTick();

  while (1)
  {
      //прийом
      if (USART1->SR & USART_SR_RXNE) {
          char c = USART1->DR;
          if (rxIndex < sizeof(rxBuffer) - 1) {
              rxBuffer[rxIndex++] = c;
          }
      }

      // Захист від ORE
      if (USART1->SR & USART_SR_ORE) {
          volatile uint32_t dummy = USART1->DR;
          (void)dummy;
      }

      switch (currentState) {

			case APP_STATE_IDLE:
				// Відправка кожні 2 секунди
				if (HAL_GetTick() - txTimer >= 2000) {
					txTimer = HAL_GetTick();
					txIndex = 0;
					rxIndex = 0;
					processIndex = 0;
					currentState = APP_STATE_TRANSMITTING;
				}
				// Перевірка чи зібрано слово повністю?
				else if (rxIndex >= txLen && processIndex == 0) {
					currentState = APP_STATE_MORSE_PROCESS;
				}
				break;

			case APP_STATE_TRANSMITTING:
				if (USART6->SR & USART_SR_TXE) {
					USART6->DR = txMsg[txIndex++];
					if (txIndex >= txLen) {
						currentState = APP_STATE_IDLE;
					}
				}
				break;

			case APP_STATE_MORSE_PROCESS:
				if (processIndex < rxIndex) {
					currentMorse = getMorseCode(rxBuffer[processIndex]);
					symbolIndex = 0;
					if (currentMorse[0] != '\0') {
						GPIOA->BSRR = GPIO_PIN_5;
						stateTimer = HAL_GetTick();
						currentState = APP_STATE_MORSE_ON;
					} else {
						processIndex++;
					}
				} else {
					currentState = APP_STATE_IDLE;
				}
				break;

			case APP_STATE_MORSE_ON:
				{
					uint32_t duration = (currentMorse[symbolIndex] == '.') ? DOT_TIME : DASH_TIME;
					if (HAL_GetTick() - stateTimer >= duration) {
						GPIOA->BSRR = (uint32_t)GPIO_PIN_5 << 16;
						stateTimer = HAL_GetTick();
						currentState = APP_STATE_MORSE_OFF;
					}
				}
				break;

			case APP_STATE_MORSE_OFF:
				if (HAL_GetTick() - stateTimer >= SYMBOL_SPACE) {
					symbolIndex++;
					if (currentMorse[symbolIndex] != '\0') {

						GPIOA->BSRR = GPIO_PIN_5;
						stateTimer = HAL_GetTick();
						currentState = APP_STATE_MORSE_ON;
					} else {
						processIndex++;
						stateTimer = HAL_GetTick();
						currentState = APP_STATE_MORSE_SPACE;
					}
				}
				break;

			case APP_STATE_MORSE_SPACE:
				if (HAL_GetTick() - stateTimer >= LETTER_SPACE) {
					currentState = APP_STATE_MORSE_PROCESS;
				}
				break;
		}

  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART6_UART_Init(void)
{
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
