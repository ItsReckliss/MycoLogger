#ifndef SX1262_H
#define SX1262_H

#include "stm32f0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define SX1262_MAX_PAYLOAD_SIZE 64U

typedef enum
{
    SX1262_STATUS_OK = 0,
    SX1262_STATUS_BUSY_TIMEOUT = 1,
    SX1262_STATUS_BAD_RADIO_STATUS = 2,
    SX1262_STATUS_INVALID_ARGUMENT = 3
} SX1262Status;

typedef enum
{
    SX1262_EVENT_NONE = 0,
    SX1262_EVENT_PACKET,
    SX1262_EVENT_CRC_ERROR,
    SX1262_EVENT_HEADER_ERROR,
    SX1262_EVENT_TIMEOUT,
    SX1262_EVENT_BUS_ERROR
} SX1262Event;

typedef enum
{
    SX1262_TX_EVENT_NONE = 0,
    SX1262_TX_EVENT_DONE,
    SX1262_TX_EVENT_TIMEOUT,
    SX1262_TX_EVENT_BUS_ERROR
} SX1262TxEvent;

typedef struct
{
    uint8_t payload[SX1262_MAX_PAYLOAD_SIZE];
    uint8_t length;
    int16_t rssi_dbm_x2;
    int8_t snr_db_quarters;
} SX1262Packet;

/** Configure the STM32 SPI and GPIO interface used by the radio. */
void SX1262_BusInit(void);

/** Configure the SX1262 and enter continuous LoRa receive mode. */
SX1262Status SX1262_StartReceiver(uint8_t *radio_status);

/** Poll the radio IRQ status and return one event, if available. */
SX1262Event SX1262_Poll(SX1262Packet *packet);

/** Temporarily leave continuous RX and start one downlink transmission. */
SX1262Status SX1262_StartTransmit(const uint8_t *payload, uint8_t length);

/** Poll a downlink transmission; continuous RX resumes on completion. */
SX1262TxEvent SX1262_PollTransmit(void);

bool SX1262_IsTransmitting(void);

/** Return the current electrical level of the SX1262 BUSY signal. */
bool SX1262_IsBusy(void);

#endif /* SX1262_H */
