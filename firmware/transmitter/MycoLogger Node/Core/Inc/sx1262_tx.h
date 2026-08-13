#ifndef SX1262_TX_H
#define SX1262_TX_H

#include <stdbool.h>
#include <stdint.h>

#define SX1262_TX_MAX_PAYLOAD_SIZE 64U

typedef enum
{
    SX1262_TX_STATUS_OK = 0,
    SX1262_TX_STATUS_BUSY_TIMEOUT,
    SX1262_TX_STATUS_BAD_RADIO_STATUS,
    SX1262_TX_STATUS_INVALID_ARGUMENT
} SX1262TxStatus;

typedef enum
{
    SX1262_TX_EVENT_NONE = 0,
    SX1262_TX_EVENT_DONE,
    SX1262_TX_EVENT_TIMEOUT,
    SX1262_TX_EVENT_BUS_ERROR
} SX1262TxEvent;

typedef enum
{
    SX1262_RX_EVENT_NONE = 0,
    SX1262_RX_EVENT_PACKET,
    SX1262_RX_EVENT_TIMEOUT,
    SX1262_RX_EVENT_CRC_ERROR,
    SX1262_RX_EVENT_HEADER_ERROR,
    SX1262_RX_EVENT_BUS_ERROR
} SX1262RxEvent;

typedef struct
{
    uint8_t payload[SX1262_TX_MAX_PAYLOAD_SIZE];
    uint8_t length;
} SX1262RxPacket;

/** Configure the software-SPI pins used by the transmitter PCB. */
void SX1262_TX_BusInit(void);

/** Reset and configure the SX1262 for the MycoLogger 915 MHz LoRa link. */
SX1262TxStatus SX1262_TX_Init(uint8_t *radio_status);

/** Begin one nonblocking LoRa transmission. */
SX1262TxStatus SX1262_TX_Start(const uint8_t *payload, uint8_t length);

/** Poll DIO1/IRQ status for completion of the active transmission. */
SX1262TxEvent SX1262_TX_Poll(void);

/** Open a nonblocking LoRa receive window after an uplink. */
SX1262TxStatus SX1262_TX_StartReceive(uint32_t window_ms);

/** Poll the active receive window and copy one packet when available. */
SX1262RxEvent SX1262_TX_PollReceive(SX1262RxPacket *packet);

/** Enter warm-start sleep while no transmit or receive operation is active. */
SX1262TxStatus SX1262_TX_Sleep(void);

/** Return true while a transmission is awaiting completion. */
bool SX1262_TX_IsActive(void);

#endif /* SX1262_TX_H */
