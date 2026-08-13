/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sx1262.h"
#include "usbd_cdc_if.h"
#include "myco_command.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define NODE_BLINK_ON_TIME_MS       80U
#define NODE_BLINK_OFF_TIME_MS      90U
#define STATUS_MESSAGE_TIME_MS    5000U
#define RADIO_POLL_TIME_MS          10U
#define BUTTON_DEBOUNCE_TIME_MS    30U
#define USB_STARTUP_DELAY_MS      1500U
#define USB_MESSAGE_SIZE           320U
#define LINK_ACK_TURNAROUND_MS      350U

#define PROTOCOL_VERSION             1U
#define FIRMWARE_VERSION       "0.8.0"
#define IWDG_KEY_ENABLE          0xCCCCU
#define IWDG_KEY_WRITE_ACCESS    0x5555U
#define IWDG_KEY_REFRESH         0xAAAAU
#define IWDG_PRESCALER_DIV256          6U
#define IWDG_RELOAD_12_8_SECONDS    1999U
#define IWDG_WINDOW_DISABLED        4095U
#define IWDG_UPDATE_TIMEOUT_MS        10U
#define IWDG_REFRESH_INTERVAL_MS    1000U
#define RESET_CAUSE_FLAGS (RCC_CSR_OBLRSTF | RCC_CSR_PINRSTF | \
                           RCC_CSR_PORRSTF | RCC_CSR_SFTRSTF | \
                           RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF | \
                           RCC_CSR_LPWRRSTF)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* Retained for USB status reporting and debugger inspection. */
volatile uint32_t g_boot_reset_flags = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
static void USB_Print(const char *message);
static char *Append_U32(char *destination, uint32_t value);
static char *Append_I32(char *destination, int32_t value);
static char *Append_Text(char *destination, const char *text);
static void USB_ReportStatus(uint32_t uptime_ms,
                             GPIO_PinState button_state,
                             SX1262Status radio_init_status,
                             uint8_t radio_status);
static void USB_ReportRadioInit(SX1262Status init_status,
                                uint8_t radio_status);
static void USB_ReportIdentity(void);
static void USB_ReportPacket(const SX1262Packet *packet,
                             uint32_t sequence);
static uint8_t Packet_GetNodeId(const SX1262Packet *packet,
                                uint32_t *node_id);
static uint8_t Packet_GetNetworkCheck(const SX1262Packet *packet,
                                      uint32_t *node_id,
                                      uint32_t *transmit_sequence);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint8_t IndependentWatchdog_Start(void)
{
    uint32_t started;

    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    IWDG->KR = IWDG_KEY_ENABLE;
    IWDG->KR = IWDG_KEY_WRITE_ACCESS;
    IWDG->PR = IWDG_PRESCALER_DIV256;
    IWDG->RLR = IWDG_RELOAD_12_8_SECONDS;
    IWDG->WINR = IWDG_WINDOW_DISABLED;

    started = HAL_GetTick();
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU)) != 0U)
    {
        if ((HAL_GetTick() - started) >= IWDG_UPDATE_TIMEOUT_MS)
        {
            return 0U;
        }
    }

    IWDG->KR = IWDG_KEY_REFRESH;
    return 1U;
}

static void IndependentWatchdog_Refresh(void)
{
    IWDG->KR = IWDG_KEY_REFRESH;
}

/**
  * @brief Send a null-terminated string through USB CDC.
  *
  * This waits briefly if the USB transmit endpoint is busy.
  */
static void USB_Print(const char *message)
{
    uint32_t timeout_start;
    uint16_t length;

    if (message == NULL)
    {
        return;
    }

    length = (uint16_t)strlen(message);
    timeout_start = HAL_GetTick();

    while (CDC_Transmit_FS((uint8_t *)message, length) == USBD_BUSY)
    {
        if ((HAL_GetTick() - timeout_start) >= 100U)
        {
            /*
             * Give up after 100 ms rather than blocking
             * the receiver indefinitely.
             */
            return;
        }
    }
}

/**
  * @brief Append an unsigned 32-bit decimal value without pulling printf into
  *        this 32 KB target.
  * @retval Pointer to the byte immediately after the appended digits.
  */
static char *Append_U32(char *destination, uint32_t value)
{
    char reversed_digits[10];
    uint8_t digit_count = 0U;

    do
    {
        reversed_digits[digit_count] =
            (char)('0' + (value % 10U));
        digit_count++;
        value /= 10U;
    }
    while (value != 0U);

    while (digit_count != 0U)
    {
        digit_count--;
        *destination = reversed_digits[digit_count];
        destination++;
    }

    return destination;
}

static char *Append_I32(char *destination, int32_t value)
{
    uint32_t magnitude;

    if (value < 0)
    {
        *destination = '-';
        destination++;
        magnitude = (uint32_t)(-value);
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    return Append_U32(destination, magnitude);
}

static char *Append_Text(char *destination, const char *text)
{
    size_t length = strlen(text);
    memcpy(destination, text, length);
    return destination + length;
}

static void USB_ReportIdentity(void)
{
    USB_Print(
        "{\"v\":1,\"type\":\"hello\","
        "\"device\":\"mycologger-receiver\","
        "\"fw\":\"" FIRMWARE_VERSION "\","
        "\"transport\":\"usb-cdc-acm\"}\n");
}

/**
  * @brief Emit one machine-readable receiver status record.
  *
  * Each USB record is one UTF-8 JSON object terminated by LF. This is
  * intentionally simple to consume through a CDC/ACM serial port on Windows,
  * Linux, macOS, or a Raspberry Pi.
  */
static void USB_ReportStatus(uint32_t uptime_ms,
                             GPIO_PinState button_state,
                             SX1262Status radio_init_status,
                             uint8_t radio_status)
{
    char message[USB_MESSAGE_SIZE];
    char *cursor = message;
    static const char prefix[] =
        "{\"v\":1,\"type\":\"status\",\"uptime_ms\":";
    static const char firmware_field[] =
        ",\"fw\":\"" FIRMWARE_VERSION "\"";
    static const char busy_field[] = ",\"radio_busy\":";
    static const char button_field[] = ",\"button_pressed\":";
    static const char radio_state_field[] = ",\"radio_state\":\"";
    const char *boolean_text;
    size_t text_length;

    memcpy(cursor, prefix, sizeof(prefix) - 1U);
    cursor += sizeof(prefix) - 1U;
    cursor = Append_U32(cursor, uptime_ms);

    memcpy(cursor, firmware_field, sizeof(firmware_field) - 1U);
    cursor += sizeof(firmware_field) - 1U;

    memcpy(cursor, busy_field, sizeof(busy_field) - 1U);
    cursor += sizeof(busy_field) - 1U;
    boolean_text =
        SX1262_IsBusy() ? "true" : "false";
    text_length = strlen(boolean_text);
    memcpy(cursor, boolean_text, text_length);
    cursor += text_length;

    memcpy(cursor, button_field, sizeof(button_field) - 1U);
    cursor += sizeof(button_field) - 1U;
    boolean_text =
        (button_state == GPIO_PIN_RESET) ? "true" : "false";
    text_length = strlen(boolean_text);
    memcpy(cursor, boolean_text, text_length);
    cursor += text_length;

    memcpy(cursor, radio_state_field, sizeof(radio_state_field) - 1U);
    cursor += sizeof(radio_state_field) - 1U;

    if (radio_init_status == SX1262_STATUS_OK)
    {
        cursor = Append_Text(cursor, "rx\"");
    }
    else
    {
        cursor = Append_Text(cursor, "error\",\"radio_error_code\":");
        cursor = Append_U32(cursor, (uint32_t)radio_init_status);
        cursor = Append_Text(cursor, ",\"radio_status_raw\":");
        cursor = Append_U32(cursor, radio_status);
    }

    cursor = Append_Text(cursor, ",\"reset_flags\":");
    cursor = Append_U32(cursor, g_boot_reset_flags);

    *cursor = '}';
    cursor++;
    *cursor = '\n';
    cursor++;
    *cursor = '\0';

    USB_Print(message);
}

static void USB_ReportRadioInit(SX1262Status init_status,
                                uint8_t radio_status)
{
    char message[USB_MESSAGE_SIZE];
    char *cursor = message;

    if (init_status == SX1262_STATUS_OK)
    {
        USB_Print(
            "{\"v\":1,\"type\":\"radio\",\"state\":\"rx\","
            "\"model\":\"sx1262\",\"frequency_hz\":915000000,"
            "\"modulation\":\"lora\",\"sf\":7,\"bandwidth_hz\":125000,"
            "\"coding_rate\":\"4/5\",\"sync_word\":\"private\","
            "\"tcxo_v\":\"1.8\"}\n");
        return;
    }

    cursor = Append_Text(
        cursor,
        "{\"v\":1,\"type\":\"radio\",\"state\":\"error\",\"code\":");
    cursor = Append_U32(cursor, (uint32_t)init_status);
    cursor = Append_Text(cursor, ",\"status_raw\":");
    cursor = Append_U32(cursor, radio_status);
    cursor = Append_Text(cursor, "}\n");
    *cursor = '\0';
    USB_Print(message);
}

static void USB_ReportPacket(const SX1262Packet *packet,
                             uint32_t sequence)
{
    static const char hex_digits[] = "0123456789abcdef";
    char message[USB_MESSAGE_SIZE];
    char *cursor = message;
    uint8_t index;

    cursor = Append_Text(cursor, "{\"v\":1,\"type\":\"packet\",\"seq\":");
    cursor = Append_U32(cursor, sequence);
    cursor = Append_Text(cursor, ",\"length\":");
    cursor = Append_U32(cursor, packet->length);
    cursor = Append_Text(cursor, ",\"rssi_dbm_x2\":");
    cursor = Append_I32(cursor, packet->rssi_dbm_x2);
    cursor = Append_Text(cursor, ",\"snr_db_quarters\":");
    cursor = Append_I32(cursor, packet->snr_db_quarters);
    cursor = Append_Text(cursor, ",\"payload_hex\":\"");

    for (index = 0U; index < packet->length; index++)
    {
        *cursor = hex_digits[packet->payload[index] >> 4];
        cursor++;
        *cursor = hex_digits[packet->payload[index] & 0x0FU];
        cursor++;
    }

    cursor = Append_Text(cursor, "\"}\n");
    *cursor = '\0';
    USB_Print(message);
}

static uint8_t Packet_GetNodeId(const SX1262Packet *packet,
                                uint32_t *node_id)
{
    if ((packet == NULL) || (node_id == NULL) ||
        (packet->length < 10U) ||
        (packet->payload[0] != 'M') ||
        (packet->payload[1] != 'Y') ||
        (packet->payload[2] != 'C') ||
        (packet->payload[3] != 'O') ||
        (packet->payload[4] != PROTOCOL_VERSION))
    {
        return 0U;
    }

    *node_id = ((uint32_t)packet->payload[6] << 24) |
               ((uint32_t)packet->payload[7] << 16) |
               ((uint32_t)packet->payload[8] << 8) |
               (uint32_t)packet->payload[9];
    return 1U;
}

static uint8_t Packet_GetNetworkCheck(const SX1262Packet *packet,
                                      uint32_t *node_id,
                                      uint32_t *transmit_sequence)
{
    if ((packet == NULL) || (node_id == NULL) ||
        (transmit_sequence == NULL) || (packet->length < 14U) ||
        (Packet_GetNodeId(packet, node_id) == 0U))
    {
        return 0U;
    }

    if (packet->payload[5] == 3U)
    {
        if (packet->length != MYCO_LINK_ACK_PACKET_SIZE)
        {
            return 0U;
        }
    }
    else if ((packet->payload[5] != 2U) ||
             (packet->length < 19U) ||
             ((packet->payload[18] & 0x08U) == 0U))
    {
        return 0U;
    }

    *transmit_sequence =
        ((uint32_t)packet->payload[10] << 24) |
        ((uint32_t)packet->payload[11] << 16) |
        ((uint32_t)packet->payload[12] << 8) |
        (uint32_t)packet->payload[13];
    return 1U;
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

    /* MCU Configuration------------------------------------------------------*/

    /*
     * Reset all peripherals, initialize the Flash interface,
     * and initialize SysTick.
     */
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
    if (IndependentWatchdog_Start() == 0U)
    {
        Error_Handler();
    }
    SX1262_BusInit();
    MX_USB_DEVICE_Init();

    /* USER CODE BEGIN 2 */

    uint32_t now;

    uint32_t led_timer = HAL_GetTick();
    uint32_t led_blinks_remaining = 0U;
    uint8_t led_on = 0U;
    uint32_t packet_node_id = 0U;
    uint32_t watchdog_refresh_timer = HAL_GetTick();

    /*
     * USB heartbeat timing.
     */
    uint32_t status_timer = HAL_GetTick();
    uint32_t radio_poll_timer = HAL_GetTick();
    uint32_t packet_sequence = 0U;
    uint8_t radio_status = 0U;
    SX1262Packet radio_packet;
    SX1262Event radio_event;
    SX1262TxEvent radio_tx_event;
    MycoDownlinkCommand downlink_command;
    uint8_t downlink_packet[MYCO_CONFIG_PACKET_SIZE];
    uint8_t link_ack_packet[MYCO_LINK_ACK_PACKET_SIZE];
    uint8_t link_ack_pending = 0U;
    uint32_t link_ack_node_id = 0U;
    uint32_t link_ack_sequence = 0U;
    uint32_t link_ack_queued_tick = 0U;
    SX1262Status radio_init_status =
        SX1262_StartReceiver(&radio_status);

    /*
     * Button debounce state.
     *
     * The button connects PA2 to ground:
     * released = GPIO_PIN_SET
     * pressed  = GPIO_PIN_RESET
     */
    GPIO_PinState raw_button_state =
        HAL_GPIO_ReadPin(Debug_GPIO_Port, Debug_Pin);

    GPIO_PinState previous_raw_button_state = raw_button_state;
    GPIO_PinState debounced_button_state = raw_button_state;

    uint32_t button_debounce_timer = HAL_GetTick();

    /*
     * Give Windows or Linux time to enumerate the CDC device
     * before transmitting the first message.
     */
    HAL_Delay(USB_STARTUP_DELAY_MS);

    USB_ReportIdentity();

    USB_ReportRadioInit(radio_init_status, radio_status);

    /*
     * Start the five-second status interval after the
     * startup message is transmitted.
     */
    status_timer = HAL_GetTick();
    radio_poll_timer = HAL_GetTick();
    led_timer = HAL_GetTick();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        now = HAL_GetTick();

        if ((now - watchdog_refresh_timer) >= IWDG_REFRESH_INTERVAL_MS)
        {
            IndependentWatchdog_Refresh();
            watchdog_refresh_timer = now;
        }

        if (MycoCommand_TakeInfoRequest())
        {
            USB_ReportIdentity();
        }

        if ((radio_init_status == SX1262_STATUS_OK) &&
            ((now - radio_poll_timer) >= RADIO_POLL_TIME_MS))
        {
            radio_poll_timer = now;
            if (SX1262_IsTransmitting())
            {
                radio_tx_event = SX1262_PollTransmit();
                if (radio_tx_event == SX1262_TX_EVENT_BUS_ERROR)
                {
                    USB_Print(
                        "{\"v\":1,\"type\":\"radio_error\","
                        "\"error\":\"downlink_bus\"}\n");
                }
                else if (radio_tx_event == SX1262_TX_EVENT_TIMEOUT)
                {
                    USB_Print(
                        "{\"v\":1,\"type\":\"radio_error\","
                        "\"error\":\"downlink_timeout\"}\n");
                }
                radio_event = SX1262_EVENT_NONE;
            }
            else
            {
                radio_event = SX1262_Poll(&radio_packet);
            }

            if (radio_event == SX1262_EVENT_PACKET)
            {
                uint8_t is_network_check;
                packet_sequence++;
                is_network_check =
                    Packet_GetNetworkCheck(&radio_packet,
                                           &link_ack_node_id,
                                           &link_ack_sequence);
                /* Forward link checks too. The host can return an already
                   queued config command before the 350 ms radio turnaround. */
                USB_ReportPacket(&radio_packet, packet_sequence);
                if (Packet_GetNodeId(&radio_packet, &packet_node_id) != 0U)
                {
                    /* Restart the indication with the newest packet's node. */
                    led_blinks_remaining = packet_node_id;
                    led_on = (led_blinks_remaining != 0U) ? 1U : 0U;
                    led_timer = now;
                    HAL_GPIO_WritePin(
                        LED_GPIO_Port,
                        LED_Pin,
                        (led_on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
                }
                if (is_network_check != 0U)
                {
                    link_ack_pending = 1U;
                    link_ack_queued_tick = now;
                }
            }
            else if (radio_event == SX1262_EVENT_CRC_ERROR)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"radio_error\","
                    "\"error\":\"crc\"}\n");
            }
            else if (radio_event == SX1262_EVENT_HEADER_ERROR)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"radio_error\","
                    "\"error\":\"header\"}\n");
            }
            else if (radio_event == SX1262_EVENT_TIMEOUT)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"radio_error\","
                    "\"error\":\"timeout\"}\n");
            }
            else if (radio_event == SX1262_EVENT_BUS_ERROR)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"radio_error\","
                    "\"error\":\"bus\"}\n");
            }
        }

        if ((radio_init_status == SX1262_STATUS_OK) &&
            !SX1262_IsTransmitting() && (link_ack_pending != 0U) &&
            ((now - link_ack_queued_tick) >= LINK_ACK_TURNAROUND_MS))
        {
            if (MycoCommand_TakeForNode(link_ack_node_id,
                                       &downlink_command))
            {
                /* A real queued configuration also confirms the network. */
                MycoCommand_BuildPacket(&downlink_command, downlink_packet);
                if (SX1262_StartTransmit(downlink_packet,
                                         sizeof(downlink_packet)) ==
                    SX1262_STATUS_OK)
                {
                    link_ack_pending = 0U;
                }
            }
            else
            {
                MycoCommand_BuildLinkAck(link_ack_node_id,
                                         link_ack_sequence,
                                         link_ack_packet);
                if (SX1262_StartTransmit(link_ack_packet,
                                         sizeof(link_ack_packet)) ==
                    SX1262_STATUS_OK)
                {
                    link_ack_pending = 0U;
                }
            }
        }
        else if ((radio_init_status == SX1262_STATUS_OK) &&
                 !SX1262_IsTransmitting() &&
                 (link_ack_pending == 0U) &&
                 MycoCommand_Take(&downlink_command))
        {
            MycoCommand_BuildPacket(&downlink_command, downlink_packet);
            if (SX1262_StartTransmit(downlink_packet,
                                     sizeof(downlink_packet)) !=
                SX1262_STATUS_OK)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"radio_error\","
                    "\"error\":\"downlink_start\"}\n");
            }
        }

        /*
         * Read the button's current raw electrical state.
         */
        raw_button_state =
            HAL_GPIO_ReadPin(Debug_GPIO_Port, Debug_Pin);

        /*
         * Restart the debounce timer whenever the raw
         * button reading changes.
         */
        if (raw_button_state != previous_raw_button_state)
        {
            previous_raw_button_state = raw_button_state;
            button_debounce_timer = now;
        }

        /*
         * Accept a new button state only after it has
         * remained unchanged for the debounce interval.
         */
        if (((now - button_debounce_timer) >=
             BUTTON_DEBOUNCE_TIME_MS) &&
            (raw_button_state != debounced_button_state))
        {
            debounced_button_state = raw_button_state;

            /*
             * The button is active-low, so RESET means pressed.
             * This runs once when a new press is detected.
             */
            if (debounced_button_state == GPIO_PIN_RESET)
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"button\","
                    "\"pressed\":true}\n");
            }
            else
            {
                USB_Print(
                    "{\"v\":1,\"type\":\"button\","
                    "\"pressed\":false}\n");
            }
        }

        /*
         * Send a status record over USB every five seconds. This also means a
         * host which opens the port after startup will quickly receive state.
         */
        if ((now - status_timer) >= STATUS_MESSAGE_TIME_MS)
        {
            status_timer = now;
            USB_ReportStatus(now,
                             debounced_button_state,
                             radio_init_status,
                             radio_status);
        }

        /* Quick, nonblocking blink count equals the received packet's node. */
        if ((led_on != 0U) &&
            ((now - led_timer) >= NODE_BLINK_ON_TIME_MS))
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            led_on = 0U;
            led_timer = now;
            if (led_blinks_remaining != 0U)
            {
                led_blinks_remaining--;
            }
        }
        else if ((led_on == 0U) && (led_blinks_remaining != 0U) &&
                 ((now - led_timer) >= NODE_BLINK_OFF_TIME_MS))
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            led_on = 1U;
            led_timer = now;
        }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

    /*
     * Use the internal 48 MHz oscillator.
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI48;

    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Run the CPU, AHB, and APB1 at 48 MHz.
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI48;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Use HSI48 as the USB clock source.
     */
    PeriphClkInit.PeriphClockSelection =
        RCC_PERIPHCLK_USB;

    PeriphClkInit.UsbClockSelection =
        RCC_USBCLKSOURCE_HSI48;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Enable the Clock Recovery System.
     */
    __HAL_RCC_CRS_CLK_ENABLE();

    RCC_CRSInitStruct.Prescaler =
        RCC_CRS_SYNC_DIV1;

    RCC_CRSInitStruct.Source =
        RCC_CRS_SYNC_SOURCE_USB;

    RCC_CRSInitStruct.Polarity =
        RCC_CRS_SYNC_POLARITY_RISING;

    RCC_CRSInitStruct.ReloadValue =
        __HAL_RCC_CRS_RELOADVALUE_CALCULATE(
            48000000U,
            1000U);

    RCC_CRSInitStruct.ErrorLimitValue = 34U;
    RCC_CRSInitStruct.HSI48CalibrationValue = 32U;

    HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
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
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*
     * Start with the LED off.
     */
    HAL_GPIO_WritePin(
        LED_GPIO_Port,
        LED_Pin,
        GPIO_PIN_RESET);

    /*
     * PA1: debug LED output.
     * HIGH turns the LED on.
     */
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        LED_GPIO_Port,
        &GPIO_InitStruct);

    /*
     * PA2: debug button input.
     *
     * The button connects PA2 to ground.
     * Use the internal pull-up so the pin does not float.
     */
    GPIO_InitStruct.Pin = Debug_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(
        Debug_GPIO_Port,
        &GPIO_InitStruct);

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

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
  * @brief Reports the source file and line number where
  *        an assert_param error occurred.
  * @param file Pointer to the source file name
  * @param line Source line number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */

    (void)file;
    (void)line;

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
