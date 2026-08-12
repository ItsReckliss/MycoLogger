#include "sx1262.h"
#include "main.h"

/* SX1262 commands used by this receiver. */
#define CMD_SET_STANDBY             0x80U
#define CMD_SET_RX                  0x82U
#define CMD_SET_RF_FREQUENCY        0x86U
#define CMD_CALIBRATE               0x89U
#define CMD_SET_PACKET_TYPE         0x8AU
#define CMD_SET_MODULATION_PARAMS   0x8BU
#define CMD_SET_PACKET_PARAMS       0x8CU
#define CMD_SET_BUFFER_BASE         0x8FU
#define CMD_CALIBRATE_IMAGE         0x98U
#define CMD_SET_REGULATOR_MODE      0x96U
#define CMD_SET_DIO3_TCXO_CTRL      0x97U
#define CMD_SET_DIO2_RF_SWITCH      0x9DU
#define CMD_SET_DIO_IRQ_PARAMS      0x08U
#define CMD_GET_IRQ_STATUS          0x12U
#define CMD_GET_RX_BUFFER_STATUS    0x13U
#define CMD_GET_PACKET_STATUS       0x14U
#define CMD_CLEAR_IRQ_STATUS        0x02U
#define CMD_WRITE_REGISTER          0x0DU
#define CMD_READ_BUFFER             0x1EU
#define CMD_GET_STATUS              0xC0U

#define IRQ_RX_DONE                 0x0002U
#define IRQ_HEADER_ERROR            0x0020U
#define IRQ_CRC_ERROR               0x0040U
#define IRQ_TIMEOUT                 0x0200U
#define IRQ_RECEIVER_MASK           (IRQ_RX_DONE | IRQ_HEADER_ERROR | \
                                     IRQ_CRC_ERROR | IRQ_TIMEOUT)

#define LORA_SYNC_WORD_MSB_ADDR     0x0740U
#define LORA_SYNC_WORD_LSB_ADDR     0x0741U

#define RADIO_BUSY_TIMEOUT_MS       100U

static bool WaitWhileBusy(void);
static uint8_t SPI_Transfer(uint8_t output);
static bool WriteCommand(uint8_t opcode,
                         const uint8_t *parameters,
                         uint8_t length);
static bool ReadCommand(uint8_t opcode, uint8_t *data, uint8_t length);
static bool WriteRegister(uint16_t address,
                          const uint8_t *data,
                          uint8_t length);
static bool ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length);
static bool GetStatus(uint8_t *status);
static void SelectRadio(void);
static void DeselectRadio(void);

void SX1262_BusInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* REST and NSS are active-low; keep both deasserted initially. */
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port, Radio_NSS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = Radio_REST_Pin | Radio_NSS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Radio_SCK_Pin | Radio_MISO_Pin | Radio_MOSI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = Radio_BUSY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(Radio_BUSY_GPIO_Port, &GPIO_InitStruct);

    /* 48 MHz / 16 = 3 MHz, mode 0, 8-bit frames, software-controlled NSS. */
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_BR_1 |
                SPI_CR1_BR_0 |
                SPI_CR1_SSM |
                SPI_CR1_SSI;
    SPI1->CR2 = SPI_CR2_FRXTH |
                SPI_CR2_DS_2 |
                SPI_CR2_DS_1 |
                SPI_CR2_DS_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

SX1262Status SX1262_StartReceiver(uint8_t *radio_status)
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
    static const uint8_t packet_params[] = {
        0x00U, 0x0CU, /* 12-symbol preamble */
        0x00U,        /* explicit header */
        SX1262_MAX_PAYLOAD_SIZE,
        0x01U,        /* CRC enabled */
        0x00U         /* standard IQ */
    };
    static const uint8_t sync_word[] = {0x14U, 0x24U};
    static const uint8_t irq_params[] = {
        (uint8_t)(IRQ_RECEIVER_MASK >> 8),
        (uint8_t)IRQ_RECEIVER_MASK,
        0x00U, 0x00U, /* no DIO1 connection; IRQs are polled */
        0x00U, 0x00U,
        0x00U, 0x00U
    };
    static const uint8_t clear_all_irqs[] = {0xFFU, 0xFFU};
    static const uint8_t continuous_rx[] = {0xFFU, 0xFFU, 0xFFU};
    uint8_t status = 0U;

    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_RESET);
    HAL_Delay(2U);
    HAL_GPIO_WritePin(Radio_REST_GPIO_Port, Radio_REST_Pin, GPIO_PIN_SET);
    HAL_Delay(10U);

    if (!WaitWhileBusy())
    {
        return SX1262_STATUS_BUSY_TIMEOUT;
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
        !WriteCommand(CMD_SET_PACKET_PARAMS,
                      packet_params,
                      sizeof(packet_params)) ||
        !WriteRegister(LORA_SYNC_WORD_MSB_ADDR,
                       sync_word,
                       sizeof(sync_word)) ||
        !WriteCommand(CMD_SET_DIO_IRQ_PARAMS,
                      irq_params,
                      sizeof(irq_params)) ||
        !WriteCommand(CMD_CLEAR_IRQ_STATUS,
                      clear_all_irqs,
                      sizeof(clear_all_irqs)) ||
        !WriteCommand(CMD_SET_RX, continuous_rx, sizeof(continuous_rx)))
    {
        return SX1262_STATUS_BUSY_TIMEOUT;
    }

    if (!GetStatus(&status))
    {
        return SX1262_STATUS_BUSY_TIMEOUT;
    }

    if (radio_status != NULL)
    {
        *radio_status = status;
    }

    /* A floating or shorted MISO commonly reads all zeros or all ones. */
    if ((status == 0x00U) || (status == 0xFFU))
    {
        return SX1262_STATUS_BAD_RADIO_STATUS;
    }

    return SX1262_STATUS_OK;
}

SX1262Event SX1262_Poll(SX1262Packet *packet)
{
    uint8_t irq_bytes[2];
    uint8_t clear_bytes[2];
    uint8_t buffer_status[2];
    uint8_t packet_status[3];
    uint16_t irq_status;

    if ((packet == NULL) ||
        !ReadCommand(CMD_GET_IRQ_STATUS, irq_bytes, sizeof(irq_bytes)))
    {
        return SX1262_EVENT_BUS_ERROR;
    }

    irq_status = ((uint16_t)irq_bytes[0] << 8) | irq_bytes[1];
    if (irq_status == 0U)
    {
        return SX1262_EVENT_NONE;
    }

    clear_bytes[0] = irq_bytes[0];
    clear_bytes[1] = irq_bytes[1];

    if ((irq_status & IRQ_CRC_ERROR) != 0U)
    {
        (void)WriteCommand(CMD_CLEAR_IRQ_STATUS, clear_bytes, sizeof(clear_bytes));
        return SX1262_EVENT_CRC_ERROR;
    }

    if ((irq_status & IRQ_HEADER_ERROR) != 0U)
    {
        (void)WriteCommand(CMD_CLEAR_IRQ_STATUS, clear_bytes, sizeof(clear_bytes));
        return SX1262_EVENT_HEADER_ERROR;
    }

    if ((irq_status & IRQ_TIMEOUT) != 0U)
    {
        (void)WriteCommand(CMD_CLEAR_IRQ_STATUS, clear_bytes, sizeof(clear_bytes));
        return SX1262_EVENT_TIMEOUT;
    }

    if ((irq_status & IRQ_RX_DONE) == 0U)
    {
        (void)WriteCommand(CMD_CLEAR_IRQ_STATUS, clear_bytes, sizeof(clear_bytes));
        return SX1262_EVENT_NONE;
    }

    if (!ReadCommand(CMD_GET_RX_BUFFER_STATUS,
                     buffer_status,
                     sizeof(buffer_status)))
    {
        return SX1262_EVENT_BUS_ERROR;
    }

    packet->length = buffer_status[0];
    if (packet->length > SX1262_MAX_PAYLOAD_SIZE)
    {
        packet->length = SX1262_MAX_PAYLOAD_SIZE;
    }

    if (!ReadBuffer(buffer_status[1], packet->payload, packet->length) ||
        !ReadCommand(CMD_GET_PACKET_STATUS,
                     packet_status,
                     sizeof(packet_status)))
    {
        return SX1262_EVENT_BUS_ERROR;
    }

    /* Datasheet units: RSSI is -raw/2 dBm and SNR is signed raw/4 dB. */
    packet->rssi_dbm_x2 = -(int16_t)packet_status[0];
    packet->snr_db_quarters = (int8_t)packet_status[1];

    if (!WriteCommand(CMD_CLEAR_IRQ_STATUS, clear_bytes, sizeof(clear_bytes)))
    {
        return SX1262_EVENT_BUS_ERROR;
    }

    return SX1262_EVENT_PACKET;
}

bool SX1262_IsBusy(void)
{
    return HAL_GPIO_ReadPin(Radio_BUSY_GPIO_Port, Radio_BUSY_Pin) == GPIO_PIN_SET;
}

static bool WaitWhileBusy(void)
{
    uint32_t start = HAL_GetTick();

    while (SX1262_IsBusy())
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
    while ((SPI1->SR & SPI_SR_TXE) == 0U)
    {
    }

    *(__IO uint8_t *)&SPI1->DR = output;

    while ((SPI1->SR & SPI_SR_RXNE) == 0U)
    {
    }

    return *(__IO uint8_t *)&SPI1->DR;
}

static bool WriteCommand(uint8_t opcode,
                         const uint8_t *parameters,
                         uint8_t length)
{
    uint8_t index;

    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(opcode);
    for (index = 0U; index < length; index++)
    {
        (void)SPI_Transfer(parameters[index]);
    }
    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
    }
    DeselectRadio();

    return WaitWhileBusy();
}

static bool ReadCommand(uint8_t opcode, uint8_t *data, uint8_t length)
{
    uint8_t index;

    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(opcode);
    (void)SPI_Transfer(0x00U); /* command status byte */
    for (index = 0U; index < length; index++)
    {
        data[index] = SPI_Transfer(0x00U);
    }
    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
    }
    DeselectRadio();

    return true;
}

static bool WriteRegister(uint16_t address,
                          const uint8_t *data,
                          uint8_t length)
{
    uint8_t index;

    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_WRITE_REGISTER);
    (void)SPI_Transfer((uint8_t)(address >> 8));
    (void)SPI_Transfer((uint8_t)address);
    for (index = 0U; index < length; index++)
    {
        (void)SPI_Transfer(data[index]);
    }
    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
    }
    DeselectRadio();

    return WaitWhileBusy();
}

static bool ReadBuffer(uint8_t offset, uint8_t *data, uint8_t length)
{
    uint8_t index;

    if (!WaitWhileBusy())
    {
        return false;
    }

    SelectRadio();
    (void)SPI_Transfer(CMD_READ_BUFFER);
    (void)SPI_Transfer(offset);
    (void)SPI_Transfer(0x00U);
    for (index = 0U; index < length; index++)
    {
        data[index] = SPI_Transfer(0x00U);
    }
    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
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
    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
    }
    DeselectRadio();

    return true;
}

static void SelectRadio(void)
{
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port, Radio_NSS_Pin, GPIO_PIN_RESET);
}

static void DeselectRadio(void)
{
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port, Radio_NSS_Pin, GPIO_PIN_SET);
}
