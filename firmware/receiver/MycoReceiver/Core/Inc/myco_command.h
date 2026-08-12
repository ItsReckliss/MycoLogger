#ifndef MYCO_COMMAND_H
#define MYCO_COMMAND_H

#include <stdbool.h>
#include <stdint.h>

#define MYCO_CONFIG_PACKET_SIZE 22U
#define MYCO_LINK_ACK_PACKET_SIZE 14U

typedef struct
{
    uint32_t node_id;
    uint32_t transaction_id;
    uint32_t config_revision;
    uint32_t report_interval_s;
} MycoDownlinkCommand;

/** Append bytes received from the USB CDC OUT endpoint. */
void MycoCommand_USBReceive(const uint8_t *data, uint32_t length);

/** Atomically take the newest complete command received from the host. */
bool MycoCommand_Take(MycoDownlinkCommand *command);

/** Atomically take a pending command only when it targets this node. */
bool MycoCommand_TakeForNode(uint32_t node_id,
                            MycoDownlinkCommand *command);

/** Return true once for each INFO or VERSION line received over USB. */
bool MycoCommand_TakeInfoRequest(void);

/** Build the compact LoRa configuration payload. */
void MycoCommand_BuildPacket(const MycoDownlinkCommand *command,
                             uint8_t packet[MYCO_CONFIG_PACKET_SIZE]);

/** Build the receiver response to a boot network-check request. */
void MycoCommand_BuildLinkAck(
    uint32_t node_id,
    uint32_t transmit_sequence,
    uint8_t packet[MYCO_LINK_ACK_PACKET_SIZE]);

#endif /* MYCO_COMMAND_H */
