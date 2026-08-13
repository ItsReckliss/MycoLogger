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
#include "node_config.h"
#include "battery_adc.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BOOT_BLINK_TIME_MS          100U
#define DEFAULT_REPORT_INTERVAL_MS 60000U
#define DEFAULT_DOWNLINK_WINDOW_MS  1500U
#define BUTTON_DEBOUNCE_TIME_MS       30U
#define NODE_ID_BLINK_ON_TIME_MS      90U
#define NODE_ID_BLINK_OFF_TIME_MS    110U
#define CONFIG_BLINK_COUNT              6U
#define CONFIG_BLINK_ON_TIME_MS        45U
#define CONFIG_BLINK_OFF_TIME_MS       55U
#define NETWORK_CHECK_DURATION_MS    30000U
#define NETWORK_BLINK_ON_TIME_MS       100U
#define NETWORK_BLINK_OFF_TIME_MS      100U
#define NETWORK_BLINK_PAUSE_MS        1000U
#define NETWORK_FAIL_BLINK_COUNT         5U
#define RADIO_RETRY_INTERVAL_MS     5000U
#define RADIO_ERROR_PATTERN_MS      2000U
#define SENSOR_PACKET_SIZE            51U
#define CONFIG_PACKET_SIZE            26U
#define CONFIG_ACK_PACKET_SIZE        27U
#define TEST_PACKET_VERSION            1U
#define TEST_PACKET_SENSOR_READING     2U
#define TEST_PACKET_LINK_CHECK         3U
#define TEST_PACKET_SET_CONFIG      0x80U
#define TEST_PACKET_CONFIG_ACK      0x81U
#define TEST_PACKET_LINK_ACK        0x82U
#define LINK_ACK_PACKET_SIZE           14U
#define LINK_CHECK_PACKET_SIZE         14U
#define DEFAULT_NODE_ID                0U
#define FIRMWARE_VERSION_MAJOR         0U
#define FIRMWARE_VERSION_MINOR         8U
#define FIRMWARE_VERSION_PATCH         4U
#define IWDG_KEY_ENABLE             0xCCCCU
#define IWDG_KEY_WRITE_ACCESS       0x5555U
#define IWDG_KEY_REFRESH            0xAAAAU
#define IWDG_PRESCALER_DIV64              4U
#define IWDG_RELOAD_8_SECONDS          3999U
#define IWDG_WINDOW_DISABLED           4095U
#define IWDG_UPDATE_TIMEOUT_MS           10U
#define IWDG_REFRESH_INTERVAL_MS       1000U
#define RESET_CAUSE_FLAGS (RCC_CSR_OBLRSTF | RCC_CSR_PINRSTF | \
                           RCC_CSR_PWRRSTF | RCC_CSR_SFTRSTF | \
                           RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF | \
                           RCC_CSR_LPWRRSTF)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* Retained for ST-LINK inspection and future over-the-air diagnostics. */
volatile uint32_t g_boot_reset_flags = 0U;

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

static bool IndependentWatchdog_Start(void)
{
  uint32_t started;

  /* Keep source-level debugging usable; deployed operation is unaffected. */
  DBGMCU->APBFZ1 |= DBGMCU_APBFZ1_DBG_IWDG_STOP;

  /* Start the LSI-clocked watchdog, then program an approximately 8 s period. */
  IWDG->KR = IWDG_KEY_ENABLE;
  IWDG->KR = IWDG_KEY_WRITE_ACCESS;
  IWDG->PR = IWDG_PRESCALER_DIV64;
  IWDG->RLR = IWDG_RELOAD_8_SECONDS;
  IWDG->WINR = IWDG_WINDOW_DISABLED;

  started = HAL_GetTick();
  while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU)) != 0U)
  {
    if ((HAL_GetTick() - started) >= IWDG_UPDATE_TIMEOUT_MS)
    {
      return false;
    }
  }

  IWDG->KR = IWDG_KEY_REFRESH;
  return true;
}

static void IndependentWatchdog_Refresh(void)
{
  IWDG->KR = IWDG_KEY_REFRESH;
}

static void IncrementFailureCounter(uint16_t *counter)
{
  if ((counter != NULL) && (*counter != UINT16_MAX))
  {
    (*counter)++;
  }
}

static bool NetworkCheckIsStillAllowed(uint32_t now,
                                       uint32_t started)
{
  return (now - started) < NETWORK_CHECK_DURATION_MS;
}

static bool NetworkIndicatorIsOn(uint32_t now, uint32_t started)
{
  const uint32_t cycle = NETWORK_BLINK_ON_TIME_MS +
                         NETWORK_BLINK_OFF_TIME_MS +
                         NETWORK_BLINK_ON_TIME_MS +
                         NETWORK_BLINK_PAUSE_MS;
  uint32_t phase = (now - started) % cycle;

  return (phase < NETWORK_BLINK_ON_TIME_MS) ||
         ((phase >= (NETWORK_BLINK_ON_TIME_MS + NETWORK_BLINK_OFF_TIME_MS)) &&
          (phase < (NETWORK_BLINK_ON_TIME_MS + NETWORK_BLINK_OFF_TIME_MS +
                    NETWORK_BLINK_ON_TIME_MS)));
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

static uint32_t ReadUint32BigEndian(const uint8_t *source)
{
  return ((uint32_t)source[0] << 24) |
         ((uint32_t)source[1] << 16) |
         ((uint32_t)source[2] << 8) |
         (uint32_t)source[3];
}

static void BuildSensorPacket(uint8_t *packet,
                              const MycoNodeConfig *config,
                              uint32_t sequence,
                              bool buttonPressed,
                              bool networkCheckRequested,
                              bool sensorValid,
                              bool batteryValid,
                              uint16_t batteryMillivolts,
                              const SCD41Measurement *measurement,
                              SCD41Error sensorError,
                              uint16_t sensorFailureCount,
                              uint16_t radioFailureCount)
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
               (sensorValid ? 0x02U : 0x00U) |
               (batteryValid ? 0x04U : 0x00U) |
               (networkCheckRequested ? 0x08U : 0x00U);
  WriteUint16BigEndian(&packet[19],
                       sensorValid ? measurement->co2_ppm : 0U);
  WriteUint16BigEndian(
      &packet[21],
      sensorValid ? (uint16_t)measurement->temperature_centi_c : 0U);
  WriteUint16BigEndian(
      &packet[23],
      sensorValid ? measurement->humidity_centi_percent : 0U);
  packet[25] = (uint8_t)sensorError;
  WriteUint32BigEndian(&packet[26], config->revision);
  WriteUint32BigEndian(&packet[30], config->report_interval_ms / 1000U);
  WriteUint16BigEndian(&packet[34],
                       batteryValid ? batteryMillivolts : 0U);
  /* A compact semantic version is included in every measurement. Repeating
     three bytes makes version discovery self-healing after server restarts or
     database restores without materially affecting LoRa airtime. */
  packet[36] = FIRMWARE_VERSION_MAJOR;
  packet[37] = FIRMWARE_VERSION_MINOR;
  packet[38] = FIRMWARE_VERSION_PATCH;
  /* These counters reset at boot and describe recoverable failures observed
     since this boot.  The reset flags identify why the current boot began. */
  WriteUint32BigEndian(&packet[39], g_boot_reset_flags);
  WriteUint16BigEndian(&packet[43], sensorFailureCount);
  WriteUint16BigEndian(&packet[45], radioFailureCount);
  WriteUint32BigEndian(&packet[47], config->downlink_window_ms);
}

static bool ApplyConfigPacket(const SX1262RxPacket *packet,
                              MycoNodeConfig *config,
                              uint32_t *transactionId,
                              uint8_t *status)
{
  uint32_t targetNodeId;
  uint32_t revision;
  uint32_t reportIntervalS;

  if ((packet == NULL) || (config == NULL) ||
      (transactionId == NULL) || (status == NULL) ||
      (packet->length != CONFIG_PACKET_SIZE) ||
      (packet->payload[0] != 'M') ||
      (packet->payload[1] != 'Y') ||
      (packet->payload[2] != 'C') ||
      (packet->payload[3] != 'O') ||
      (packet->payload[4] != TEST_PACKET_VERSION) ||
      (packet->payload[5] != TEST_PACKET_SET_CONFIG))
  {
    return false;
  }

  targetNodeId = ReadUint32BigEndian(&packet->payload[6]);
  *transactionId = ReadUint32BigEndian(&packet->payload[10]);
  revision = ReadUint32BigEndian(&packet->payload[14]);
  reportIntervalS = ReadUint32BigEndian(&packet->payload[18]);
  uint32_t downlinkWindowMs = ReadUint32BigEndian(&packet->payload[22]);

  return NodeConfig_Apply(config,
                          targetNodeId,
                          *transactionId,
                          revision,
                          reportIntervalS,
                          downlinkWindowMs,
                          status);
}

static void BuildConfigAckPacket(uint8_t *packet,
                                 const MycoNodeConfig *config,
                                 uint32_t transactionId,
                                 uint8_t status)
{
  packet[0] = 'M';
  packet[1] = 'Y';
  packet[2] = 'C';
  packet[3] = 'O';
  packet[4] = TEST_PACKET_VERSION;
  packet[5] = TEST_PACKET_CONFIG_ACK;
  WriteUint32BigEndian(&packet[6], config->node_id);
  WriteUint32BigEndian(&packet[10], transactionId);
  WriteUint32BigEndian(&packet[14], config->revision);
  packet[18] = status;
  WriteUint32BigEndian(&packet[19], config->report_interval_ms / 1000U);
  WriteUint32BigEndian(&packet[23], config->downlink_window_ms);
}

static bool IsLinkAckPacket(const SX1262RxPacket *packet,
                            const MycoNodeConfig *config,
                            uint32_t expectedSequence)
{
  return (packet != NULL) && (config != NULL) &&
         (packet->length == LINK_ACK_PACKET_SIZE) &&
         (packet->payload[0] == 'M') &&
         (packet->payload[1] == 'Y') &&
         (packet->payload[2] == 'C') &&
         (packet->payload[3] == 'O') &&
         (packet->payload[4] == TEST_PACKET_VERSION) &&
         (packet->payload[5] == TEST_PACKET_LINK_ACK) &&
         (ReadUint32BigEndian(&packet->payload[6]) == config->node_id) &&
         (ReadUint32BigEndian(&packet->payload[10]) == expectedSequence);
}

static void BuildLinkCheckPacket(uint8_t *packet,
                                 const MycoNodeConfig *config,
                                 uint32_t checkSequence)
{
  packet[0] = 'M';
  packet[1] = 'Y';
  packet[2] = 'C';
  packet[3] = 'O';
  packet[4] = TEST_PACKET_VERSION;
  packet[5] = TEST_PACKET_LINK_CHECK;
  WriteUint32BigEndian(&packet[6], config->node_id);
  WriteUint32BigEndian(&packet[10], checkSequence);
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

  g_boot_reset_flags = RCC->CSR & RESET_CAUSE_FLAGS;
  RCC->CSR |= RCC_CSR_RMVF;

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  if (!IndependentWatchdog_Start())
  {
    Error_Handler();
  }

  BlinkDebugLedAtBoot();
  SX1262_TX_BusInit();
  SCD41_BusInit();
  BatteryADC_InitPin();

  MycoNodeConfig nodeConfig;
  NodeConfig_Load(&nodeConfig,
                  DEFAULT_NODE_ID,
                  DEFAULT_REPORT_INTERVAL_MS,
                  DEFAULT_DOWNLINK_WINDOW_MS);
  bool provisioningConfirmation =
      NodeConfig_ConsumeProvisioningMarker(&nodeConfig);

  /* Node zero is deliberately silent until an ST-LINK provisioner writes a
     valid identity into the reserved configuration flash page. */
  if (nodeConfig.node_id == 0U)
  {
    while (1)
    {
      for (uint32_t flash = 0U; flash < 4U; flash++)
      {
        HAL_GPIO_WritePin(Debug_LED_GPIO_Port,
                          Debug_LED_Pin,
                          GPIO_PIN_SET);
        HAL_Delay(90U);
        HAL_GPIO_WritePin(Debug_LED_GPIO_Port,
                          Debug_LED_Pin,
                          GPIO_PIN_RESET);
        HAL_Delay(110U);
      }
      IndependentWatchdog_Refresh();
      HAL_Delay(1800U);
    }
  }

  uint8_t radioStatus = 0U;
  bool radioReady =
      SX1262_TX_Init(&radioStatus) == SX1262_TX_STATUS_OK;
  bool sensorCycleFailed = false;
  uint16_t sensorFailureCount = 0U;
  uint16_t radioFailureCount = 0U;
  bool sensorMeasurementValid = false;
  bool sensorPacketPending = false;
  bool sensorTransmitActive = false;
  bool downlinkReceiveActive = false;
  bool configAckTransmitActive = false;
  bool networkLinkPacketPending = radioReady;
  bool networkLinkTransmitActive = false;
  bool networkConfirmationPending = true;
  bool networkStartupCheckActive = radioReady;
  uint32_t networkCheckSequence = 0U;
  uint32_t networkCheckStartedTick = HAL_GetTick();
  SCD41Measurement sensorMeasurement = {0};
  uint32_t lastRadioInitTick = HAL_GetTick();
  uint32_t lastSensorCycleTick = HAL_GetTick();
  uint32_t ledSequenceTick = HAL_GetTick();
  uint32_t ledSequenceBlinksRemaining =
      provisioningConfirmation ? CONFIG_BLINK_COUNT : 0U;
  uint32_t ledSequenceOnTimeMs = CONFIG_BLINK_ON_TIME_MS;
  uint32_t ledSequenceOffTimeMs = CONFIG_BLINK_OFF_TIME_MS;
  bool ledSequenceActive = provisioningConfirmation;
  bool ledSequenceLedOn = provisioningConfirmation;
  uint32_t transmitSequence = 0U;
  uint8_t sensorPacket[SENSOR_PACKET_SIZE];
  uint8_t linkCheckPacket[LINK_CHECK_PACKET_SIZE];
  uint8_t configAckPacket[CONFIG_ACK_PACKET_SIZE];
  SX1262RxPacket downlinkPacket;
  uint32_t configTransactionId = 0U;
  uint8_t configStatus = 0U;
  uint16_t batteryMillivolts = 0U;
  GPIO_PinState rawButtonState =
      HAL_GPIO_ReadPin(Debug_Button_GPIO_Port, Debug_Button_Pin);
  GPIO_PinState previousRawButtonState = rawButtonState;
  GPIO_PinState debouncedButtonState = rawButtonState;
  uint32_t buttonDebounceTick = HAL_GetTick();
  uint32_t lastWatchdogRefreshTick = HAL_GetTick();

  /* Take the first sample immediately; subsequent starts are one minute apart. */
  if (SCD41_StartSingleShot() != SCD41_STATUS_OK)
  {
    sensorCycleFailed = true;
    IncrementFailureCounter(&sensorFailureCount);
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
      if (debouncedButtonState == GPIO_PIN_RESET)
      {
        /* Identify this physical transmitter without blocking sensing/radio. */
        ledSequenceBlinksRemaining = nodeConfig.node_id;
        ledSequenceOnTimeMs = NODE_ID_BLINK_ON_TIME_MS;
        ledSequenceOffTimeMs = NODE_ID_BLINK_OFF_TIME_MS;
        ledSequenceActive = ledSequenceBlinksRemaining > 0U;
        ledSequenceLedOn = ledSequenceActive;
        ledSequenceTick = now;

        if (!SCD41_IsActive())
        {
          /* A press always requests a new sensor conversion before transmit. */
          lastSensorCycleTick = now;
          sensorMeasurementValid = false;
          if (SCD41_StartSingleShot() != SCD41_STATUS_OK)
          {
            sensorCycleFailed = true;
            IncrementFailureCounter(&sensorFailureCount);
            sensorPacketPending = true;
          }
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
        IncrementFailureCounter(&sensorFailureCount);
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
        IncrementFailureCounter(&sensorFailureCount);
        sensorPacketPending = true;
      }
    }

    if (!radioReady &&
        ((now - lastRadioInitTick) >= RADIO_RETRY_INTERVAL_MS))
    {
      lastRadioInitTick = now;
      radioReady =
          SX1262_TX_Init(&radioStatus) == SX1262_TX_STATUS_OK;
      if (!radioReady)
      {
        IncrementFailureCounter(&radioFailureCount);
      }
      if (radioReady && networkConfirmationPending &&
          NetworkCheckIsStillAllowed(now, networkCheckStartedTick))
      {
        networkStartupCheckActive = true;
        networkLinkPacketPending = true;
      }
    }

    if (radioReady && networkLinkPacketPending &&
        !SX1262_TX_IsActive())
    {
      networkCheckSequence++;
      BuildLinkCheckPacket(linkCheckPacket,
                           &nodeConfig,
                           networkCheckSequence);
      if (SX1262_TX_Start(linkCheckPacket, sizeof(linkCheckPacket)) ==
          SX1262_TX_STATUS_OK)
      {
        networkLinkPacketPending = false;
        networkLinkTransmitActive = true;
      }
      else
      {
        IncrementFailureCounter(&radioFailureCount);
        networkStartupCheckActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }
    else if (radioReady && sensorPacketPending && !SX1262_TX_IsActive())
    {
      uint32_t nextSequence = transmitSequence + 1U;
      bool batteryValid =
          BatteryADC_ReadMillivolts(&batteryMillivolts);
      BuildSensorPacket(sensorPacket,
                        &nodeConfig,
                        nextSequence,
                        buttonPressed,
                        networkConfirmationPending,
                        sensorMeasurementValid,
                        batteryValid,
                        batteryMillivolts,
                        &sensorMeasurement,
                        SCD41_GetLastError(),
                        sensorFailureCount,
                        radioFailureCount);

      if (SX1262_TX_Start(sensorPacket, sizeof(sensorPacket)) ==
          SX1262_TX_STATUS_OK)
      {
        transmitSequence = nextSequence;
        if (networkConfirmationPending && !networkStartupCheckActive)
        {
          /* Later reports continue checking after a bounded boot failure. */
          networkCheckSequence = nextSequence;
        }
        sensorPacketPending = false;
        sensorTransmitActive = true;
      }
      else
      {
        IncrementFailureCounter(&radioFailureCount);
        networkStartupCheckActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }

    if (radioReady && networkLinkTransmitActive)
    {
      SX1262TxEvent event = SX1262_TX_Poll();
      if (event == SX1262_TX_EVENT_DONE)
      {
        networkLinkTransmitActive = false;
        if (SX1262_TX_StartReceive(nodeConfig.downlink_window_ms) ==
            SX1262_TX_STATUS_OK)
        {
          downlinkReceiveActive = true;
        }
        else
        {
          IncrementFailureCounter(&radioFailureCount);
          networkStartupCheckActive = false;
          radioReady = false;
          lastRadioInitTick = now;
        }
      }
      else if ((event == SX1262_TX_EVENT_TIMEOUT) ||
               (event == SX1262_TX_EVENT_BUS_ERROR))
      {
        IncrementFailureCounter(&radioFailureCount);
        networkLinkTransmitActive = false;
        networkStartupCheckActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }
    else if (radioReady && sensorTransmitActive)
    {
      SX1262TxEvent event = SX1262_TX_Poll();
      if (event == SX1262_TX_EVENT_DONE)
      {
        sensorTransmitActive = false;
        if (SX1262_TX_StartReceive(nodeConfig.downlink_window_ms) ==
            SX1262_TX_STATUS_OK)
        {
          downlinkReceiveActive = true;
        }
        else
        {
          IncrementFailureCounter(&radioFailureCount);
          networkStartupCheckActive = false;
          radioReady = false;
          lastRadioInitTick = now;
        }
      }
      else if ((event == SX1262_TX_EVENT_TIMEOUT) ||
               (event == SX1262_TX_EVENT_BUS_ERROR))
      {
        IncrementFailureCounter(&radioFailureCount);
        sensorTransmitActive = false;
        networkStartupCheckActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }
    else if (radioReady && downlinkReceiveActive)
    {
      SX1262RxEvent event = SX1262_TX_PollReceive(&downlinkPacket);
      if (event == SX1262_RX_EVENT_PACKET)
      {
        uint32_t previousConfigRevision = nodeConfig.revision;
        downlinkReceiveActive = false;
        bool configPacketAccepted =
            ApplyConfigPacket(&downlinkPacket,
                              &nodeConfig,
                              &configTransactionId,
                              &configStatus);
        bool linkAcknowledged =
            IsLinkAckPacket(&downlinkPacket,
                            &nodeConfig,
                            networkCheckSequence);

        if (configPacketAccepted || linkAcknowledged)
        {
          bool wasStartupCheck = networkStartupCheckActive;
          networkConfirmationPending = false;
          networkStartupCheckActive = false;

          if (linkAcknowledged && !wasStartupCheck)
          {
            /* Confirm recovery if the receiver was unavailable at startup. */
            ledSequenceBlinksRemaining = CONFIG_BLINK_COUNT;
            ledSequenceOnTimeMs = CONFIG_BLINK_ON_TIME_MS;
            ledSequenceOffTimeMs = CONFIG_BLINK_OFF_TIME_MS;
            ledSequenceActive = true;
            ledSequenceLedOn = true;
            ledSequenceTick = now;
          }
        }
        else if (networkStartupCheckActive)
        {
          /* An unrelated downlink is not proof that this node was reached. */
          if (NetworkCheckIsStillAllowed(now, networkCheckStartedTick))
          {
            networkLinkPacketPending = true;
          }
          else
          {
            networkStartupCheckActive = false;
            ledSequenceBlinksRemaining = NETWORK_FAIL_BLINK_COUNT;
            ledSequenceOnTimeMs = NETWORK_BLINK_ON_TIME_MS;
            ledSequenceOffTimeMs = NETWORK_BLINK_OFF_TIME_MS;
            ledSequenceActive = true;
            ledSequenceLedOn = true;
            ledSequenceTick = now;
          }
        }

        if (configPacketAccepted)
        {
          if ((configStatus == NODE_CONFIG_STATUS_APPLIED) &&
              (nodeConfig.revision != previousConfigRevision))
          {
            /* A short flurry confirms a newly accepted receiver command. */
            ledSequenceBlinksRemaining = CONFIG_BLINK_COUNT;
            ledSequenceOnTimeMs = CONFIG_BLINK_ON_TIME_MS;
            ledSequenceOffTimeMs = CONFIG_BLINK_OFF_TIME_MS;
            ledSequenceActive = true;
            ledSequenceLedOn = true;
            ledSequenceTick = now;
          }
          BuildConfigAckPacket(configAckPacket,
                               &nodeConfig,
                               configTransactionId,
                               configStatus);
          /* Give the receiver time to return from TX to continuous RX. */
          HAL_Delay(30U);
          if (SX1262_TX_Start(configAckPacket,
                              sizeof(configAckPacket)) ==
              SX1262_TX_STATUS_OK)
          {
            configAckTransmitActive = true;
          }
          else
          {
            IncrementFailureCounter(&radioFailureCount);
            radioReady = false;
            lastRadioInitTick = now;
          }
        }
      }
      else if ((event == SX1262_RX_EVENT_TIMEOUT) ||
               (event == SX1262_RX_EVENT_CRC_ERROR) ||
               (event == SX1262_RX_EVENT_HEADER_ERROR))
      {
        downlinkReceiveActive = false;
        if (networkStartupCheckActive)
        {
          if (NetworkCheckIsStillAllowed(now, networkCheckStartedTick))
          {
            networkLinkPacketPending = true;
          }
          else
          {
            networkStartupCheckActive = false;
            ledSequenceBlinksRemaining = NETWORK_FAIL_BLINK_COUNT;
            ledSequenceOnTimeMs = NETWORK_BLINK_ON_TIME_MS;
            ledSequenceOffTimeMs = NETWORK_BLINK_OFF_TIME_MS;
            ledSequenceActive = true;
            ledSequenceLedOn = true;
            ledSequenceTick = now;
          }
        }
      }
      else if (event == SX1262_RX_EVENT_BUS_ERROR)
      {
        IncrementFailureCounter(&radioFailureCount);
        downlinkReceiveActive = false;
        networkStartupCheckActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }
    else if (radioReady && configAckTransmitActive)
    {
      SX1262TxEvent event = SX1262_TX_Poll();
      if (event == SX1262_TX_EVENT_DONE)
      {
        configAckTransmitActive = false;
      }
      else if ((event == SX1262_TX_EVENT_TIMEOUT) ||
               (event == SX1262_TX_EVENT_BUS_ERROR))
      {
        IncrementFailureCounter(&radioFailureCount);
        configAckTransmitActive = false;
        radioReady = false;
        lastRadioInitTick = now;
      }
    }

    if (ledSequenceActive)
    {
      uint32_t phaseDuration = ledSequenceLedOn
          ? ledSequenceOnTimeMs
          : ledSequenceOffTimeMs;
      if ((now - ledSequenceTick) >= phaseDuration)
      {
        ledSequenceTick = now;
        if (ledSequenceLedOn)
        {
          ledSequenceLedOn = false;
          ledSequenceBlinksRemaining--;
          if (ledSequenceBlinksRemaining == 0U)
          {
            ledSequenceActive = false;
          }
        }
        else
        {
          ledSequenceLedOn = true;
        }
      }
    }

    if (ledSequenceActive)
    {
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port,
                        Debug_LED_Pin,
                        ledSequenceLedOn ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else if (networkStartupCheckActive)
    {
      HAL_GPIO_WritePin(Debug_LED_GPIO_Port,
                        Debug_LED_Pin,
                        NetworkIndicatorIsOn(now, networkCheckStartedTick)
                            ? GPIO_PIN_SET
                            : GPIO_PIN_RESET);
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

    /* Refresh only after a full state-machine pass completed. A stuck sensor,
       radio, or fault handler therefore cannot keep the watchdog alive. */
    if ((now - lastWatchdogRefreshTick) >= IWDG_REFRESH_INTERVAL_MS)
    {
      IndependentWatchdog_Refresh();
      lastWatchdogRefreshTick = now;
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
