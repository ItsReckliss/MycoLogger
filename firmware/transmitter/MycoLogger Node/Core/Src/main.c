/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "sx1262_tx.h"
#include "scd41.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct
{
  uint32_t node_id;
  uint32_t report_interval_ms;
  uint32_t downlink_window_ms;
} MycoNodeConfig;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BOOT_BLINK_TIME_MS          100U
#define DEFAULT_REPORT_INTERVAL_MS 60000U
#define DEFAULT_DOWNLINK_WINDOW_MS  1500U
#define BUTTON_DEBOUNCE_TIME_MS       30U
#define HEARTBEAT_ON_TIME_MS         500U
#define RADIO_RETRY_INTERVAL_MS     5000U
#define RADIO_ERROR_PATTERN_MS      2000U
#define SENSOR_PACKET_SIZE            26U
#define TEST_PACKET_VERSION            1U
#define TEST_PACKET_SENSOR_READING     2U
#define DEFAULT_NODE_ID                1U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void BlinkDebugLedAtBoot(void)
{
  for (uint8_t blink = 0; blink < 3U; blink++)
  {
    HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_SET);
    HAL_Delay(BOOT_BLINK_TIME_MS);

    HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(BOOT_BLINK_TIME_MS);
  }
}

static void WriteUint32BigEndian(uint8_t *destination, uint32_t value)
{
  destination[0] = (uint8_t)(value >> 24);
  destination[1] = (uint8_t)(value >> 16);
  destination[2] = (uint8_t)(value >> 8);
  destination[3] = (uint8_t)value;
}

static void WriteUint16BigEndian(uint8_t *destination, uint16_t value)
{
  destination[0] = (uint8_t)(value >> 8);
  destination[1] = (uint8_t)value;
}

static void BuildSensorPacket(uint8_t *packet,
                              const MycoNodeConfig *config,
                              uint32_t sequence,
                              bool buttonPressed,
                              bool sensorValid,
                              const SCD41Measurement *measurement,
                              SCD41Error sensorError)
{
  packet[0] = 'M';
  packet[1] = 'Y';
  packet[2] = 'C';
  packet[3] = 'O';
  packet[4] = TEST_PACKET_VERSION;
  packet[5] = TEST_PACKET_SENSOR_READING;
  WriteUint32BigEndian(&packet[6], config->node_id);
  WriteUint32BigEndian(&packet[10], sequence);
  WriteUint32BigEndian(&packet[14], HAL_GetTick() / 1000U);
  packet[18] = (buttonPressed ? 0x01U : 0x00U) |
               (sensorValid ? 0x02U : 0x00U);
  WriteUint16BigEndian(&packet[19],
                       sensorValid ? measurement->co2_ppm : 0U);
  WriteUint16BigEndian(
      &packet[21],
      sensorValid ? (uint16_t)measurement->temperature_centi_c : 0U);
  WriteUint16BigEndian(
      &packet[23],
      sensorValid ? measurement->humidity_centi_percent : 0U);
  packet[25] = (uint8_t)sensorError;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  BlinkDebugLedAtBoot();
  SX1262_TX_BusInit();
  SCD41_BusInit();

  /*
   * Keep changeable settings together so a future authenticated downlink can
   * validate and apply a complete configuration update atomically.
   */
  MycoNodeConfig nodeConfig = {
      .node_id = DEFAULT_NODE_ID,
      .report_interval_ms = DEFAULT_REPORT_INTERVAL_MS,
      .downlink_window_ms = DEFAULT_DOWNLINK_WINDOW_MS
  };

  uint8_t radioStatus = 0U;
  bool radioReady =
      SX1262_TX_Init(&radioStatus) == SX1262_TX_STATUS_OK;
  bool sensorCycleFailed = false;
  bool sensorMeasurementValid = false;
  bool sensorPacketPending = false;
  SCD41Measurement sensorMeasurement = {0};
  uint32_t lastRadioInitTick = HAL_GetTick();
  uint32_t lastSensorCycleTick = HAL_GetTick();
  uint32_t ledFlashStartTick = 0U;
  bool ledFlashActive = false;
  uint32_t transmitSequence = 0U;
  uint8_t sensorPacket[SENSOR_PACKET_SIZE];
  GPIO_PinState rawButtonState =
      HAL_GPIO_ReadPin(Debug_Button_GPIO_Port, Debug_Button_Pin);
  GPIO_PinState previousRawButtonState = rawButtonState;
  GPIO_PinState debouncedButtonState = rawButtonState;
  uint32_t buttonDebounceTick = HAL_GetTick();

  /* Take the first sample immediately; subsequent starts are one minute apart. */
  if (SCD41_StartSingleShot() != SCD41_STATUS_OK)
  {
    sensorCycleFailed = true;
    sensorPacketPending = true;
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    rawButtonState = HAL_GPIO_ReadPin(Debug_Button_GPIO_Port,
                                     Debug_Button_Pin);
    if (rawButtonState != previousRawButtonState)
    {
      previousRawButtonState = rawButtonState;
      buttonDebounceTick = now;
    }

    if (((now - buttonDebounceTick) >= BUTTON_DEBOUNCE_TIME_MS) &&
        (rawButtonState != debouncedButtonState))
    {
      debouncedButtonState = rawButtonState;
      if ((debouncedButtonState == GPIO_PIN_RESET) &&
          !SCD41_IsActive())
      {
        /* A press always requests a new sensor conversion before transmit. */
        lastSensorCycleTick = now;
        sensorMeasurementValid = false;
        if (SCD41_StartSingleShot() != SCD41_STATUS_OK)
        {
          sensorCycleFailed = true;
          sensorPacketPending = true;
        }
      }
    }

    bool buttonPressed = debouncedButtonState == GPIO_PIN_RESET;

    if (SCD41_IsActive())
    {
      SCD41Event sensorEvent = SCD41_Poll(&sensorMeasurement);
      if (sensorEvent == SCD41_EVENT_MEASUREMENT)
      {
        sensorMeasurementValid = true;
        sensorCycleFailed = false;
        sensorPacketPending = true;
      }
      else if ((sensorEvent == SCD41_EVENT_BUS_ERROR) ||
               (sensorEvent == SCD41_EVENT_CRC_ERROR))
      {
        sensorMeasurementValid = false;
        sensorCycleFailed = true;
        sensorPacketPending = true;
      }
    }
    else if ((now - lastSensorCycleTick) >= nodeConfig.report_interval_ms)
    {
      HAL_GPIO_WritePin(Sensor_Power_GPIO_Port,
                        Sensor_Power_Pin,
                        GPIO_PIN_RESET);
      lastSensorCycleTick = now;
      sensorMeasurementValid = false;
      if (SCD41_StartSingleShot() != SCD41_STATUS_OK)
      {
        sensorCycleFailed = true;
        sensorPacketPending = true;
      }
    }

    if (!radioReady &&
        ((now - lastRadioInitTick) >= RADIO_RETRY_INTERVAL_MS))
    {
      lastRadioInitTick = now;
      radioReady =
          SX1262_TX_Init(&radioStatus) == SX1262_TX_STATUS_OK;
    }

    if (radioReady && sensorPacketPending && !SX1262_TX_IsActive())
    {
      uint32_t nextSequence = transmitSequence + 1U;
      BuildSensorPacket(sensorPacket,
                        &nodeConfig,
                        nextSequence,
                        buttonPressed,
                        sensorMeasurementValid,
                        &sensorMeasurement,
                        SCD41_GetLastError());

      if (SX1262_TX_Start(sensorPacket, sizeof(sensorPacket)) ==
          SX1262_TX_STATUS_OK)
      {
        transmitSequence = nextSequence;
        sensorPacketPending = false;
      }
      else
      {
        radioReady = false;
        lastRadioInitTick = now;
      }
    }

    if (radioReady && SX1262_TX_IsActive())
    {
      SX1262TxEvent event = SX1262_TX_Poll();
      if (event == SX1262_TX_EVENT_DONE)
      {
        ledFlashStartTick = now;
        ledFlashActive = true;
      }
      else if ((event == SX1262_TX_EVENT_TIMEOUT) ||
               (event == SX1262_TX_EVENT_BUS_ERROR))
      {
        radioReady = false;
        lastRadioInitTick = now;
      }
    }

    if (ledFlashActive &&
        ((now - ledFlashStartTick) >= HEARTBEAT_ON_TIME_MS))
    {
      ledFlashActive = false;
    }

    if (buttonPressed)
    {
      /* The button shorts PC14 to ground, so a low input means pressed. */
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_SET);
    }
    else if (ledFlashActive)
    {
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_SET);
    }
    else if (radioReady && sensorCycleFailed &&
             (((now % RADIO_ERROR_PATTERN_MS) < 100U) ||
              (((now % RADIO_ERROR_PATTERN_MS) >= 200U) &&
               ((now % RADIO_ERROR_PATTERN_MS) < 300U)) ||
              (((now % RADIO_ERROR_PATTERN_MS) >= 400U) &&
               ((now % RADIO_ERROR_PATTERN_MS) < 500U))))
    {
      /* Three short flashes mean the SCD41 did not initialize/read. */
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_SET);
    }
    else if (!radioReady &&
             (((now % RADIO_ERROR_PATTERN_MS) < 100U) ||
              (((now % RADIO_ERROR_PATTERN_MS) >= 200U) &&
               ((now % RADIO_ERROR_PATTERN_MS) < 300U))))
    {
      /* Two short flashes mean radio initialization/transmission failed. */
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_RESET);
    }

    /* Sleep the CPU until the next SysTick/interrupt while state machines wait. */
    __WFI();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Debug_Button_Pin */
  GPIO_InitStruct.Pin = Debug_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Debug_Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Debug_LED_Pin */
  GPIO_InitStruct.Pin = Debug_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Debug_LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
