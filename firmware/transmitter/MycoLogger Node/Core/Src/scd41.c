#include "scd41.h"
#include "main.h"

#define SCD41_ADDRESS_7BIT               0x62U
#define SCD41_COMMAND_SINGLE_SHOT        0x219DU
#define SCD41_COMMAND_READ_MEASUREMENT   0xEC05U
#define SCD41_COMMAND_DATA_READY         0xE4B8U
#define SCD41_COMMAND_SERIAL_NUMBER      0x3682U
#define SCD41_POWER_UP_TIME_MS             100U
#define SCD41_MEASUREMENT_TIME_MS           5000U
#define SCD41_MEASUREMENT_TIMEOUT_MS        6000U
#define I2C_CLOCK_STRETCH_TIMEOUT_MS          2U

typedef enum
{
    SENSOR_STATE_OFF = 0,
    SENSOR_STATE_POWERING_UP,
    SENSOR_STATE_MEASURING
} SensorState;

static SensorState sensor_state;
static uint32_t sensor_state_started_ms;
static SCD41Error last_error;
static SCD41Error transaction_error;

static bool SendCommand(uint16_t command);
static bool ReadBytes(uint8_t *data, uint8_t length);
static bool I2C_Start(void);
static void I2C_Stop(void);
static bool I2C_WriteByte(uint8_t value);
static uint8_t I2C_ReadByte(bool acknowledge);
static bool ReleaseClock(void);
static void RecoverBus(void);
static void BusDelay(void);
static uint8_t CalculateCrc(const uint8_t *data, uint8_t length);
static bool CheckWordCrc(const uint8_t *word_and_crc);

void SCD41_BusInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(Sensor_Power_GPIO_Port,
                      Sensor_Power_Pin,
                      GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = Sensor_Power_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Sensor_Power_GPIO_Port, &GPIO_InitStruct);

    /* Open-drain high means released; the switched 10 kOhm resistors pull up. */
    HAL_GPIO_WritePin(GPIOA,
                      Sensor_SDA_Pin | Sensor_SCL_Pin,
                      GPIO_PIN_SET);
    GPIO_InitStruct.Pin = Sensor_SDA_Pin | Sensor_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    sensor_state = SENSOR_STATE_OFF;
    last_error = SCD41_ERROR_NONE;
}

SCD41Status SCD41_StartSingleShot(void)
{
    if (sensor_state != SENSOR_STATE_OFF)
    {
        return SCD41_STATUS_BUS_ERROR;
    }

    HAL_GPIO_WritePin(Sensor_Power_GPIO_Port,
                      Sensor_Power_Pin,
                      GPIO_PIN_SET);
    sensor_state = SENSOR_STATE_POWERING_UP;
    sensor_state_started_ms = HAL_GetTick();
    last_error = SCD41_ERROR_NONE;
    return SCD41_STATUS_OK;
}

SCD41Event SCD41_Poll(SCD41Measurement *measurement)
{
    uint8_t serial_number[9];
    uint8_t ready_data[3];
    uint8_t measurement_data[9];
    uint16_t ready_status;
    uint16_t raw_temperature;
    uint16_t raw_humidity;

    if ((measurement == NULL) || (sensor_state == SENSOR_STATE_OFF))
    {
        return SCD41_EVENT_NONE;
    }

    if (sensor_state == SENSOR_STATE_POWERING_UP)
    {
        if ((HAL_GetTick() - sensor_state_started_ms) <
            SCD41_POWER_UP_TIME_MS)
        {
            return SCD41_EVENT_NONE;
        }

        RecoverBus();

        /* Verify that the powered device is an intact, responding SCD41. */
        if (!SendCommand(SCD41_COMMAND_SERIAL_NUMBER))
        {
            last_error = (transaction_error != SCD41_ERROR_NONE) ?
                         transaction_error :
                         SCD41_ERROR_SERIAL_COMMAND_NACK;
            SCD41_PowerOff();
            return SCD41_EVENT_BUS_ERROR;
        }
        HAL_Delay(1U);
        if (!ReadBytes(serial_number, sizeof(serial_number)))
        {
            last_error = SCD41_ERROR_SERIAL_READ_NACK;
            SCD41_PowerOff();
            return SCD41_EVENT_BUS_ERROR;
        }
        if (!CheckWordCrc(&serial_number[0]) ||
            !CheckWordCrc(&serial_number[3]) ||
            !CheckWordCrc(&serial_number[6]))
        {
            last_error = SCD41_ERROR_SERIAL_CRC;
            SCD41_PowerOff();
            return SCD41_EVENT_CRC_ERROR;
        }

        if (!SendCommand(SCD41_COMMAND_SINGLE_SHOT))
        {
            last_error = SCD41_ERROR_SINGLE_SHOT_NACK;
            SCD41_PowerOff();
            return SCD41_EVENT_BUS_ERROR;
        }

        sensor_state = SENSOR_STATE_MEASURING;
        sensor_state_started_ms = HAL_GetTick();
        return SCD41_EVENT_NONE;
    }

    if ((HAL_GetTick() - sensor_state_started_ms) <
        SCD41_MEASUREMENT_TIME_MS)
    {
        return SCD41_EVENT_NONE;
    }

    if (!SendCommand(SCD41_COMMAND_DATA_READY))
    {
        last_error = SCD41_ERROR_READY_COMMAND_NACK;
        SCD41_PowerOff();
        return SCD41_EVENT_BUS_ERROR;
    }
    HAL_Delay(1U);
    if (!ReadBytes(ready_data, sizeof(ready_data)))
    {
        last_error = SCD41_ERROR_READY_READ_NACK;
        SCD41_PowerOff();
        return SCD41_EVENT_BUS_ERROR;
    }
    if (!CheckWordCrc(ready_data))
    {
        last_error = SCD41_ERROR_READY_CRC;
        SCD41_PowerOff();
        return SCD41_EVENT_CRC_ERROR;
    }

    ready_status = ((uint16_t)ready_data[0] << 8) | ready_data[1];
    if ((ready_status & 0x07FFU) == 0U)
    {
        if ((HAL_GetTick() - sensor_state_started_ms) >=
            SCD41_MEASUREMENT_TIMEOUT_MS)
        {
            last_error = SCD41_ERROR_MEASUREMENT_TIMEOUT;
            SCD41_PowerOff();
            return SCD41_EVENT_BUS_ERROR;
        }
        return SCD41_EVENT_NONE;
    }

    if (!SendCommand(SCD41_COMMAND_READ_MEASUREMENT))
    {
        last_error = SCD41_ERROR_MEASUREMENT_COMMAND_NACK;
        SCD41_PowerOff();
        return SCD41_EVENT_BUS_ERROR;
    }
    HAL_Delay(1U);
    if (!ReadBytes(measurement_data, sizeof(measurement_data)))
    {
        last_error = SCD41_ERROR_MEASUREMENT_READ_NACK;
        SCD41_PowerOff();
        return SCD41_EVENT_BUS_ERROR;
    }
    if (!CheckWordCrc(&measurement_data[0]) ||
        !CheckWordCrc(&measurement_data[3]) ||
        !CheckWordCrc(&measurement_data[6]))
    {
        last_error = SCD41_ERROR_MEASUREMENT_CRC;
        SCD41_PowerOff();
        return SCD41_EVENT_CRC_ERROR;
    }

    measurement->co2_ppm =
        ((uint16_t)measurement_data[0] << 8) | measurement_data[1];
    raw_temperature =
        ((uint16_t)measurement_data[3] << 8) | measurement_data[4];
    raw_humidity =
        ((uint16_t)measurement_data[6] << 8) | measurement_data[7];

    /* Datasheet conversions, represented as fixed-point hundredths. */
    measurement->temperature_centi_c =
        (int16_t)(-4500L +
                  (((int32_t)17500L * raw_temperature + 32767L) / 65535L));
    measurement->humidity_centi_percent =
        (uint16_t)(((uint32_t)10000UL * raw_humidity + 32767UL) / 65535UL);

    SCD41_PowerOff();
    last_error = SCD41_ERROR_NONE;
    return SCD41_EVENT_MEASUREMENT;
}

bool SCD41_IsActive(void)
{
    return sensor_state != SENSOR_STATE_OFF;
}

SCD41Error SCD41_GetLastError(void)
{
    return last_error;
}

void SCD41_PowerOff(void)
{
    HAL_GPIO_WritePin(Sensor_Power_GPIO_Port,
                      Sensor_Power_Pin,
                      GPIO_PIN_RESET);
    sensor_state = SENSOR_STATE_OFF;
}

static bool SendCommand(uint16_t command)
{
    transaction_error = SCD41_ERROR_NONE;

    if (!I2C_Start())
    {
        if ((Sensor_SCL_GPIO_Port->IDR & Sensor_SCL_Pin) == 0U)
        {
            transaction_error = SCD41_ERROR_SCL_HELD_LOW;
        }
        else if ((Sensor_SDA_GPIO_Port->IDR & Sensor_SDA_Pin) == 0U)
        {
            transaction_error = SCD41_ERROR_SDA_HELD_LOW;
        }
        I2C_Stop();
        return false;
    }

    if (!I2C_WriteByte((uint8_t)(SCD41_ADDRESS_7BIT << 1)))
    {
        transaction_error = SCD41_ERROR_ADDRESS_NACK;
        I2C_Stop();
        return false;
    }
    if (!I2C_WriteByte((uint8_t)(command >> 8)))
    {
        transaction_error = SCD41_ERROR_COMMAND_MSB_NACK;
        I2C_Stop();
        return false;
    }
    if (!I2C_WriteByte((uint8_t)command))
    {
        transaction_error = SCD41_ERROR_COMMAND_LSB_NACK;
        I2C_Stop();
        return false;
    }

    I2C_Stop();
    return true;
}

static bool ReadBytes(uint8_t *data, uint8_t length)
{
    if ((data == NULL) || (length == 0U) || !I2C_Start())
    {
        I2C_Stop();
        return false;
    }

    if (!I2C_WriteByte((uint8_t)((SCD41_ADDRESS_7BIT << 1) | 0x01U)))
    {
        I2C_Stop();
        return false;
    }

    for (uint8_t index = 0U; index < length; index++)
    {
        data[index] = I2C_ReadByte(index < (uint8_t)(length - 1U));
    }

    I2C_Stop();
    return true;
}

static bool I2C_Start(void)
{
    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    if (!ReleaseClock())
    {
        return false;
    }
    if ((Sensor_SDA_GPIO_Port->IDR & Sensor_SDA_Pin) == 0U)
    {
        return false;
    }
    BusDelay();
    Sensor_SDA_GPIO_Port->BSRR = (uint32_t)Sensor_SDA_Pin << 16U;
    BusDelay();
    Sensor_SCL_GPIO_Port->BSRR = (uint32_t)Sensor_SCL_Pin << 16U;
    BusDelay();
    return true;
}

static void I2C_Stop(void)
{
    Sensor_SDA_GPIO_Port->BSRR = (uint32_t)Sensor_SDA_Pin << 16U;
    BusDelay();
    (void)ReleaseClock();
    BusDelay();
    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    BusDelay();
}

static bool I2C_WriteByte(uint8_t value)
{
    for (uint8_t mask = 0x80U; mask != 0U; mask >>= 1)
    {
        if ((value & mask) != 0U)
        {
            Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
        }
        else
        {
            Sensor_SDA_GPIO_Port->BSRR =
                (uint32_t)Sensor_SDA_Pin << 16U;
        }

        BusDelay();
        if (!ReleaseClock())
        {
            return false;
        }
        BusDelay();
        Sensor_SCL_GPIO_Port->BSRR = (uint32_t)Sensor_SCL_Pin << 16U;
        BusDelay();
    }

    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    BusDelay();
    if (!ReleaseClock())
    {
        return false;
    }
    BusDelay();
    bool acknowledged =
        (Sensor_SDA_GPIO_Port->IDR & Sensor_SDA_Pin) == 0U;
    Sensor_SCL_GPIO_Port->BSRR = (uint32_t)Sensor_SCL_Pin << 16U;
    BusDelay();
    return acknowledged;
}

static uint8_t I2C_ReadByte(bool acknowledge)
{
    uint8_t value = 0U;

    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
        value <<= 1;
        BusDelay();
        (void)ReleaseClock();
        BusDelay();
        if ((Sensor_SDA_GPIO_Port->IDR & Sensor_SDA_Pin) != 0U)
        {
            value |= 0x01U;
        }
        Sensor_SCL_GPIO_Port->BSRR = (uint32_t)Sensor_SCL_Pin << 16U;
        BusDelay();
    }

    if (acknowledge)
    {
        Sensor_SDA_GPIO_Port->BSRR = (uint32_t)Sensor_SDA_Pin << 16U;
    }
    else
    {
        Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    }
    BusDelay();
    (void)ReleaseClock();
    BusDelay();
    Sensor_SCL_GPIO_Port->BSRR = (uint32_t)Sensor_SCL_Pin << 16U;
    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    BusDelay();

    return value;
}

static bool ReleaseClock(void)
{
    uint32_t start = HAL_GetTick();

    Sensor_SCL_GPIO_Port->BSRR = Sensor_SCL_Pin;
    while ((Sensor_SCL_GPIO_Port->IDR & Sensor_SCL_Pin) == 0U)
    {
        if ((HAL_GetTick() - start) >= I2C_CLOCK_STRETCH_TIMEOUT_MS)
        {
            return false;
        }
    }
    return true;
}

static void RecoverBus(void)
{
    Sensor_SDA_GPIO_Port->BSRR = Sensor_SDA_Pin;
    for (uint8_t pulse = 0U; pulse < 9U; pulse++)
    {
        Sensor_SCL_GPIO_Port->BSRR =
            (uint32_t)Sensor_SCL_Pin << 16U;
        BusDelay();
        (void)ReleaseClock();
        BusDelay();
    }
    I2C_Stop();
}

static void BusDelay(void)
{
    /* Approximately standard-mode I2C speed with the 16 MHz system clock. */
    for (volatile uint8_t delay = 0U; delay < 20U; delay++)
    {
        __NOP();
    }
}

static uint8_t CalculateCrc(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFFU;

    for (uint8_t index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static bool CheckWordCrc(const uint8_t *word_and_crc)
{
    return CalculateCrc(word_and_crc, 2U) == word_and_crc[2];
}
