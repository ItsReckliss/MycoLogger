#ifndef SCD41_H
#define SCD41_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SCD41_STATUS_OK = 0,
    SCD41_STATUS_BUS_ERROR,
    SCD41_STATUS_CRC_ERROR
} SCD41Status;

typedef enum
{
    SCD41_EVENT_NONE = 0,
    SCD41_EVENT_MEASUREMENT,
    SCD41_EVENT_BUS_ERROR,
    SCD41_EVENT_CRC_ERROR
} SCD41Event;

typedef struct
{
    uint16_t co2_ppm;
    int16_t temperature_centi_c;
    uint16_t humidity_centi_percent;
} SCD41Measurement;

typedef enum
{
    SCD41_ERROR_NONE = 0,
    SCD41_ERROR_SERIAL_COMMAND_NACK = 1,
    SCD41_ERROR_SERIAL_READ_NACK = 2,
    SCD41_ERROR_SERIAL_CRC = 3,
    SCD41_ERROR_SINGLE_SHOT_NACK = 4,
    SCD41_ERROR_READY_COMMAND_NACK = 5,
    SCD41_ERROR_READY_READ_NACK = 6,
    SCD41_ERROR_READY_CRC = 7,
    SCD41_ERROR_MEASUREMENT_TIMEOUT = 8,
    SCD41_ERROR_MEASUREMENT_COMMAND_NACK = 9,
    SCD41_ERROR_MEASUREMENT_READ_NACK = 10,
    SCD41_ERROR_MEASUREMENT_CRC = 11,
    SCD41_ERROR_SCL_HELD_LOW = 12,
    SCD41_ERROR_SDA_HELD_LOW = 13,
    SCD41_ERROR_ADDRESS_NACK = 14,
    SCD41_ERROR_COMMAND_MSB_NACK = 15,
    SCD41_ERROR_COMMAND_LSB_NACK = 16
} SCD41Error;

/** Configure PB7 and the open-drain software-I2C pins, initially powered off. */
void SCD41_BusInit(void);

/** Begin a nonblocking power-cycled SCD41 single-shot measurement. */
SCD41Status SCD41_StartSingleShot(void);

/** Poll data-ready status and return a CRC-checked measurement when available. */
SCD41Event SCD41_Poll(SCD41Measurement *measurement);

/** Return true between sensor power-on and completion/error. */
bool SCD41_IsActive(void);

/** Return the most recent single-shot cycle's diagnostic result. */
SCD41Error SCD41_GetLastError(void);

/** Disable the PB7-controlled sensor load switch. */
void SCD41_PowerOff(void);

#endif /* SCD41_H */
