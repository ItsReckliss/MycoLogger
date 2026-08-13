#include "sx1262_tx.h"
#include "main.h"

/* SX1262 commands used by the transmitter. */
#define CMD_SET_STANDBY             0x80U
#define CMD_SET_RX                  0x82U
#define CMD_SET_TX                  0x83U
#define CMD_SET_SLEEP               0x84U
#define CMD_SET_RF_FREQUENCY        0x86U
#define CMD_CALIBRATE               0x89U
#define CMD_SET_PACKET_TYPE         0x8AU
#define CMD_SET_MODULATION_PARAMS   0x8BU
#define CMD_SET_PACKET_PARAMS       0x8CU
#define CMD_SET_TX_PARAMS           0x8EU
#define CMD_SET_BUFFER_BASE         0x8FU
#define CMD_SET_PA_CONFIG           0x95U
#define CMD_SET_REGULATOR_MODE      0x96U
#define CMD_SET_DIO3_TCXO_CTRL      0x97U
#define CMD_CALIBRATE_IMAGE         0x98U
#define CMD_SET_DIO2_RF_SWITCH      0x9DU
#define CMD_WRITE_REGISTER          0x0DU
#define CMD_WRITE_BUFFER            0x0EU
#define CMD_READ_BUFFER             0x1EU
#define CMD_SET_DIO_IRQ_PARAMS      0x08U
#define CMD_GET_IRQ_STATUS          0x12U
#define CMD_GET_RX_BUFFER_STATUS    0x13U
#define CMD_CLEAR_IRQ_STATUS        0x02U
#define CMD_GET_STATUS              0xC0U

#define IRQ_TX_DONE                 0x0001U
#define IRQ_RX_DONE                 0x0002U
#define IRQ_HEADER_ERROR            0x0020U
#define IRQ_CRC_ERROR               0x0040U
#define IRQ_TIMEOUT                 0x0200U
#define IRQ_TRANSMITTER_MASK        (IRQ_TX_DONE | IRQ_TIMEOUT)
#define IRQ_RECEIVER_MASK           (IRQ_RX_DONE | IRQ_HEADER_ERROR | \
                                     IRQ_CRC_ERROR | IRQ_TIMEOUT)

#define LORA_SYNC_WORD_MSB_ADDR     0x0740U
#define RADIO_BUSY_TIMEOUT_MS       100U
#define RADIO_TX_WATCHDOG_MS       3500U

static bool radio_active;
static bool radio_sleeping;
static uint32_t radio_tx_started_ms;
static uint8_t radio_mode;

static bool WaitWhileBusy(void);
static uint8_t SPI_Transfer(uint8_t output);
static bool WriteCommand(uint8_t opcode,
                         const uint8_t *parameters,
                         uint8_t length);
static bool ReadCommand(uint8_t opcode, uint8_t *data, uint8_t length);
static bool WriteRegister(uint16_t address,
                          const uint8_t *data,
                          uint8_t length);
static bool WriteBuffer(uint8_t offset,
                        const uint8_t *data,
                        uint8_t length);
static bool ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length);
static bool GetStatus(uint8_t *status);
static bool WakeRadio(void);
static void SelectRadio(void);
static void DeselectRadio(void);
static void SPI_Delay(void);

void SX1262_TX_BusInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* REST and NSS are active-low; SCK idles low for SPI mode 0. */
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port, Radio_NSS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Radio_SCK_GPIO_Port, Radio_SCK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Radio_MOSI_GPIO_Port, Radio_MOSI_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = Radio_NSS_Pin | Radio_SCK_Pin | Radio_REST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Radio_MOSI_Pin;
    HAL_GPIO_Init(Radio_MOSI_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Radio_BUSY_Pin | Radio_MISO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Radio_DIO1_Pin;
    HAL_GPIO_Init(Radio_DIO1_GPIO_Port, &GPIO_InitStruct);

    radio_active = false;
    radio_sleeping = false;
    radio_mode = 0U;
}

SX1262TxStatus SX1262_TX_Init(uint8_t *radio_status)
{
    static const uint8_t standby_rc[] = {0x00U};
    static const uint8_t regulator_ldo[] = {0x00U};
    static const uint8_t tcxo_1v8_5ms[] = {0x02U, 0x00U, 0x01U, 0x40U};
    static const uint8_t rf_switch_dio2[] = {0x01U};
    static const uint8_t calibrate_all[] = {0x7FU};
    static const uint8_t calibrate_902_928[] = {0xE1U, 0xE9U};
    static const uint8_t packet_type_lora[] = {0x01U};
    static const uint8_t frequency_915mhz[] = {0x39U, 0x30U, 0x00U, 0x00U};
    static const uint8_t buffer_base[] = {0x00U, 0x00U};
    static const uint8_t modulation[] = {
        0x07U, /* spreading factor 7 */
        0x04U, /* 125 kHz bandwidth */
        0x01U, /* coding rate 4/5 */
        0x00U  /* low-data-rate optimization off */
    };
    static const uint8_t pa_config_sx1262[] = {
        0x04U, /* PA duty cycle */
        0x07U, /* high-power PA maximum */
        0x00U, /* SX1262 device selection */
        0x01U  /* PA lookup-table setting */
    };
    static const uint8_t tx_params[] = {
        0x00U, /* 0 dBm is ample for a close-range bench test */
        0x04U  /* 200 us PA ramp */
    };
    static const uint8_t sync_word[] = {0x14U, 0x24U};
    static const uint8_t irq_params[] = {
        (uint8_t)(IRQ_TRANSMITTER_MASK >> 8),
        (uint8_t)IRQ_TRANSMITTER_MASK,
        (uint8_t)(IRQ_TRANSMITTER_MASK >> 8),
        (uint8_t)IRQ_TRANSMITTER_MASK, /* TX done/timeout on DIO1 */
        0x00U, 0x00U,
        0x00U, 0x00U
    };
    static const uint8_t clear_all_irqs[] = {0xFFU, 0xFFU};
    uint8_t status = 0U;

    radio_active = false;
    radio_sleeping = false;
    radio_mode = 0U;

    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_RESET);
    HAL_Delay(2U);
    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_SET);
    HAL_Delay(10U);

    if (!WaitWhileBusy())
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }

    if (!WriteCommand(CMD_SET_STANDBY, standby_rc, sizeof(standby_rc)) ||
        !WriteCommand(CMD_SET_REGULATOR_MODE,
                      regulator_ldo,
                      sizeof(regulator_ldo)) ||
        !WriteCommand(CMD_SET_DIO3_TCXO_CTRL,
                      tcxo_1v8_5ms,
                      sizeof(tcxo_1v8_5ms)) ||
        !WriteCommand(CMD_SET_DIO2_RF_SWITCH,
                      rf_switch_dio2,
                      sizeof(rf_switch_dio2)) ||
        !WriteCommand(CMD_CALIBRATE,
                      calibrate_all,
                      sizeof(calibrate_all)) ||
        !WriteCommand(CMD_CALIBRATE_IMAGE,
                      calibrate_902_928,
                      sizeof(calibrate_902_928)) ||
        !WriteCommand(CMD_SET_PACKET_TYPE,
                      packet_type_lora,
                      sizeof(packet_type_lora)) ||
        !WriteCommand(CMD_SET_RF_FREQUENCY,
                      frequency_915mhz,
                      sizeof(frequency_915mhz)) ||
        !WriteCommand(CMD_SET_BUFFER_BASE,
                      buffer_base,
                      sizeof(buffer_base)) ||
        !WriteCommand(CMD_SET_MODULATION_PARAMS,
                      modulation,
                      sizeof(modulation)) ||
        !WriteCommand(CMD_SET_PA_CONFIG,
                      pa_config_sx1262,
                      sizeof(pa_config_sx1262)) ||
        !WriteCommand(CMD_SET_TX_PARAMS,
                      tx_params,
                      sizeof(tx_params)) ||
        !WriteRegister(LORA_SYNC_WORD_MSB_ADDR,
                       sync_word,
                       sizeof(sync_word)) ||
        !WriteCommand(CMD_SET_DIO_IRQ_PARAMS,
                      irq_params,
                      sizeof(irq_params)) ||
        !WriteCommand(CMD_CLEAR_IRQ_STATUS,
                      clear_all_irqs,
                      sizeof(clear_all_irqs)))
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }

    if (!GetStatus(&status))
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }

    if (radio_status != NULL)
    {
        *radio_status = status;
    }

    /* A floating or shorted MISO commonly reads all zeros or all ones. */
    if ((status == 0x00U) || (status == 0xFFU))
    {
        return SX1262_TX_STATUS_BAD_RADIO_STATUS;
    }

    return SX1262_TX_STATUS_OK;
}

SX1262TxStatus SX1262_TX_Start(const uint8_t *payload, uint8_t length)
{
    uint8_t packet_params[] = {
        0x00U, 0x0CU, /* 12-symbol preamble */
        0x00U,        /* explicit header */
        length,
        0x01U,        /* CRC enabled */
        0x00U         /* standard IQ */
    };
    static const uint8_t clear_all_irqs[] = {0xFFU, 0xFFU};
    static const uint8_t tx_timeout_3s[] = {0x02U, 0xEEU, 0x00U};
    static const uint8_t irq_params[] = {
        (uint8_t)(IRQ_TRANSMITTER_MASK >> 8),
        (uint8_t)IRQ_TRANSMITTER_MASK,
        (uint8_t)(IRQ_TRANSMITTER_MASK >> 8),
        (uint8_t)IRQ_TRANSMITTER_MASK,
        0x00U, 0x00U, 0x00U, 0x00U
    };

    if ((payload == NULL) || (length == 0U) ||
        (length > SX1262_TX_MAX_PAYLOAD_SIZE) || radio_active)
    {
        return SX1262_TX_STATUS_INVALID_ARGUMENT;
    }

    if (!WakeRadio() ||
        !WriteCommand(CMD_SET_PACKET_PARAMS,
                      packet_params,
                      sizeof(packet_params)) ||
        !WriteBuffer(0x00U, payload, length) ||
        !WriteCommand(CMD_SET_DIO_IRQ_PARAMS,
                      irq_params,
                      sizeof(irq_params)) ||
        !WriteCommand(CMD_CLEAR_IRQ_STATUS,
                      clear_all_irqs,
                      sizeof(clear_all_irqs)) ||
        !WriteCommand(CMD_SET_TX, tx_timeout_3s, sizeof(tx_timeout_3s)))
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }

    radio_tx_started_ms = HAL_GetTick();
    radio_active = true;
    radio_mode = 1U;
    return SX1262_TX_STATUS_OK;
}

SX1262TxEvent SX1262_TX_Poll(void)
{
    uint8_t irq_bytes[2];
    uint16_t irq_status;
    bool watchdog_expired;

    if (!radio_active || (radio_mode != 1U))
    {
        return SX1262_TX_EVENT_NONE;
    }

    watchdog_expired =
        (HAL_GetTick() - radio_tx_started_ms) >= RADIO_TX_WATCHDOG_MS;

    if ((HAL_GPIO_ReadPin(Radio_DIO1_GPIO_Port, Radio_DIO1_Pin) ==
         GPIO_PIN_RESET) && !watchdog_expired)
    {
        return SX1262_TX_EVENT_NONE;
    }

    if (!ReadCommand(CMD_GET_IRQ_STATUS, irq_bytes, sizeof(irq_bytes)))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_TX_EVENT_BUS_ERROR;
    }

    irq_status = ((uint16_t)irq_bytes[0] << 8) | irq_bytes[1];

    if ((irq_status != 0U) &&
        !WriteCommand(CMD_CLEAR_IRQ_STATUS, irq_bytes, sizeof(irq_bytes)))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_TX_EVENT_BUS_ERROR;
    }

    if ((irq_status & IRQ_TX_DONE) != 0U)
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_TX_EVENT_DONE;
    }

    if (((irq_status & IRQ_TIMEOUT) != 0U) || watchdog_expired)
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_TX_EVENT_TIMEOUT;
    }

    return SX1262_TX_EVENT_NONE;
}

bool SX1262_TX_IsActive(void)
{
    return radio_active;
}

SX1262TxStatus SX1262_TX_Sleep(void)
{
    /* Warm-start preserves the radio configuration and packet buffer. */
    static const uint8_t warm_start[] = {0x04U};

    if (radio_active)
    {
        return SX1262_TX_STATUS_INVALID_ARGUMENT;
    }
    if (radio_sleeping)
    {
        return SX1262_TX_STATUS_OK;
    }
    /* SetSleep is exceptional: BUSY is not waited after NSS rises because the
       part has already stopped its command interface. */
    if (!WaitWhileBusy())
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }
    SelectRadio();
    (void)SPI_Transfer(CMD_SET_SLEEP);
    (void)SPI_Transfer(warm_start[0]);
    DeselectRadio();

    radio_mode = 0U;
    radio_sleeping = true;
    return SX1262_TX_STATUS_OK;
}

SX1262TxStatus SX1262_TX_StartReceive(uint32_t window_ms)
{
    uint8_t packet_params[] = {
        0x00U, 0x0CU,
        0x00U,
        SX1262_TX_MAX_PAYLOAD_SIZE,
        0x01U,
        0x00U
    };
    uint8_t rx_timeout[3];
    uint32_t timeout_ticks;
    static const uint8_t clear_all_irqs[] = {0xFFU, 0xFFU};
    static const uint8_t irq_params[] = {
        (uint8_t)(IRQ_RECEIVER_MASK >> 8),
        (uint8_t)IRQ_RECEIVER_MASK,
        (uint8_t)(IRQ_RECEIVER_MASK >> 8),
        (uint8_t)IRQ_RECEIVER_MASK,
        0x00U, 0x00U, 0x00U, 0x00U
    };

    if ((window_ms == 0U) || (window_ms > 60000U) || radio_active)
    {
        return SX1262_TX_STATUS_INVALID_ARGUMENT;
    }

    /* SX1262 RX timeout units are 15.625 us, or 64 ticks per ms. */
    timeout_ticks = window_ms * 64U;
    rx_timeout[0] = (uint8_t)(timeout_ticks >> 16);
    rx_timeout[1] = (uint8_t)(timeout_ticks >> 8);
    rx_timeout[2] = (uint8_t)timeout_ticks;

    if (!WakeRadio() ||
        !WriteCommand(CMD_SET_PACKET_PARAMS,
                      packet_params,
                      sizeof(packet_params)) ||
        !WriteCommand(CMD_SET_DIO_IRQ_PARAMS,
                      irq_params,
                      sizeof(irq_params)) ||
        !WriteCommand(CMD_CLEAR_IRQ_STATUS,
                      clear_all_irqs,
                      sizeof(clear_all_irqs)) ||
        !WriteCommand(CMD_SET_RX, rx_timeout, sizeof(rx_timeout)))
    {
        return SX1262_TX_STATUS_BUSY_TIMEOUT;
    }

    radio_active = true;
    radio_mode = 2U;
    return SX1262_TX_STATUS_OK;
}

SX1262RxEvent SX1262_TX_PollReceive(SX1262RxPacket *packet)
{
    uint8_t irq_bytes[2];
    uint8_t buffer_status[2];
    uint16_t irq_status;

    if (!radio_active || (radio_mode != 2U))
    {
        return SX1262_RX_EVENT_NONE;
    }
    if ((packet == NULL) ||
        !ReadCommand(CMD_GET_IRQ_STATUS, irq_bytes, sizeof(irq_bytes)))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_BUS_ERROR;
    }

    irq_status = ((uint16_t)irq_bytes[0] << 8) | irq_bytes[1];
    if (irq_status == 0U)
    {
        return SX1262_RX_EVENT_NONE;
    }
    if (!WriteCommand(CMD_CLEAR_IRQ_STATUS, irq_bytes, sizeof(irq_bytes)))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_BUS_ERROR;
    }

    if ((irq_status & IRQ_CRC_ERROR) != 0U)
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_CRC_ERROR;
    }
    if ((irq_status & IRQ_HEADER_ERROR) != 0U)
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_HEADER_ERROR;
    }
    if ((irq_status & IRQ_TIMEOUT) != 0U)
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_TIMEOUT;
    }
    if ((irq_status & IRQ_RX_DONE) == 0U)
    {
        return SX1262_RX_EVENT_NONE;
    }

    if (!ReadCommand(CMD_GET_RX_BUFFER_STATUS,
                     buffer_status,
                     sizeof(buffer_status)))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_BUS_ERROR;
    }
    packet->length = buffer_status[0];
    if (packet->length > SX1262_TX_MAX_PAYLOAD_SIZE)
    {
        packet->length = SX1262_TX_MAX_PAYLOAD_SIZE;
    }
    if (!ReadBuffer(buffer_status[1], packet->payload, packet->length))
    {
        radio_active = false;
        radio_mode = 0U;
        return SX1262_RX_EVENT_BUS_ERROR;
    }

    radio_active = false;
    radio_mode = 0U;
    return SX1262_RX_EVENT_PACKET;
}

static bool WaitWhileBusy(void)
{
    uint32_t start = HAL_GetTick();

    while (HAL_GPIO_ReadPin(Radio_BUSY_GPIO_Port, Radio_BUSY_Pin) ==
           GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - start) >= RADIO_BUSY_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

static uint8_t SPI_Transfer(uint8_t output)
{
    uint8_t input = 0U;

    for (uint8_t mask = 0x80U; mask != 0U; mask >>= 1)
    {
        if ((output & mask) != 0U)
        {
            Radio_MOSI_GPIO_Port->BSRR = Radio_MOSI_Pin;
        }
        else
        {
            Radio_MOSI_GPIO_Port->BSRR = (uint32_t)Radio_MOSI_Pin << 16U;
        }

        SPI_Delay();
        Radio_SCK_GPIO_Port->BSRR = Radio_SCK_Pin;
        SPI_Delay();

        if ((Radio_MISO_GPIO_Port->IDR & Radio_MISO_Pin) != 0U)
        {
            input |= mask;
        }

        Radio_SCK_GPIO_Port->BSRR = (uint32_t)Radio_SCK_Pin << 16U;
        SPI_Delay();
    }

    return input;
}

static bool WriteCommand(uint8_t opcode,
                         const uint8_t *parameters,
                         uint8_t length)
{
    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(opcode);
    for (uint8_t index = 0U; index < length; index++)
    {
        (void)SPI_Transfer(parameters[index]);
    }
    DeselectRadio();

    return WaitWhileBusy();
}

static bool ReadCommand(uint8_t opcode, uint8_t *data, uint8_t length)
{
    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(opcode);
    (void)SPI_Transfer(0x00U); /* command status byte */
    for (uint8_t index = 0U; index < length; index++)
    {
        data[index] = SPI_Transfer(0x00U);
    }
    DeselectRadio();

    return true;
}

static bool WriteRegister(uint16_t address,
                          const uint8_t *data,
                          uint8_t length)
{
    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_WRITE_REGISTER);
    (void)SPI_Transfer((uint8_t)(address >> 8));
    (void)SPI_Transfer((uint8_t)address);
    for (uint8_t index = 0U; index < length; index++)
    {
        (void)SPI_Transfer(data[index]);
    }
    DeselectRadio();

    return WaitWhileBusy();
}

static bool WriteBuffer(uint8_t offset,
                        const uint8_t *data,
                        uint8_t length)
{
    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_WRITE_BUFFER);
    (void)SPI_Transfer(offset);
    for (uint8_t index = 0U; index < length; index++)
    {
        (void)SPI_Transfer(data[index]);
    }
    DeselectRadio();

    return WaitWhileBusy();
}

static bool ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length)
{
    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_READ_BUFFER);
    (void)SPI_Transfer(offset);
    (void)SPI_Transfer(0x00U);
    for (uint8_t index = 0U; index < length; index++)
    {
        data[index] = SPI_Transfer(0x00U);
    }
    DeselectRadio();

    return true;
}

static bool GetStatus(uint8_t *status)
{
    if ((status == NULL) || !WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_GET_STATUS);
    *status = SPI_Transfer(0x00U);
    DeselectRadio();

    return true;
}

static bool WakeRadio(void)
{
    if (!radio_sleeping)
    {
        return true;
    }

    /* The SX1262 leaves sleep when NSS is asserted. No command is needed. */
    SelectRadio();
    SPI_Delay();
    DeselectRadio();
    HAL_Delay(1U);
    if (!WaitWhileBusy())
    {
        return false;
    }

    radio_sleeping = false;
    return true;
}

static void SelectRadio(void)
{
    Radio_NSS_GPIO_Port->BSRR = (uint32_t)Radio_NSS_Pin << 16U;
}

static void DeselectRadio(void)
{
    Radio_NSS_GPIO_Port->BSRR = Radio_NSS_Pin;
}

static void SPI_Delay(void)
{
    /* Keep software SPI comfortably below the SX1262 maximum clock rate. */
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}
