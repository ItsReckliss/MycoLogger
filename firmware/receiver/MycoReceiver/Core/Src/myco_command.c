#include "myco_command.h"
#include "stm32f0xx.h"

#define COMMAND_LINE_SIZE 64U

static char command_line[COMMAND_LINE_SIZE];
static uint8_t command_line_length;
static MycoDownlinkCommand pending_command;
static volatile uint8_t command_ready;

static bool ParseU32(const char **cursor, uint32_t *value)
{
    uint32_t parsed = 0U;
    uint8_t digits = 0U;

    while ((**cursor >= '0') && (**cursor <= '9'))
    {
        uint32_t digit = (uint32_t)(**cursor - '0');
        if (parsed > ((0xFFFFFFFFUL - digit) / 10U))
        {
            return false;
        }
        parsed = (parsed * 10U) + digit;
        (*cursor)++;
        digits++;
    }

    if (digits == 0U)
    {
        return false;
    }
    *value = parsed;
    return true;
}

static bool ParseCommandLine(MycoDownlinkCommand *command)
{
    const char *cursor = command_line;

    if ((cursor[0] != 'C') || (cursor[1] != 'F') ||
        (cursor[2] != 'G') || (cursor[3] != ' '))
    {
        return false;
    }
    cursor += 4;

    if (!ParseU32(&cursor, &command->node_id) || (*cursor++ != ' ') ||
        !ParseU32(&cursor, &command->transaction_id) || (*cursor++ != ' ') ||
        !ParseU32(&cursor, &command->config_revision) || (*cursor++ != ' ') ||
        !ParseU32(&cursor, &command->report_interval_s) || (*cursor != '\0'))
    {
        return false;
    }

    return (command->node_id != 0U) &&
           (command->transaction_id != 0U) &&
           (command->config_revision != 0U) &&
           (command->report_interval_s >= 15U) &&
           (command->report_interval_s <= 604800U);
}

void MycoCommand_USBReceive(const uint8_t *data, uint32_t length)
{
    if (data == NULL)
    {
        return;
    }

    for (uint32_t index = 0U; index < length; index++)
    {
        char value = (char)data[index];
        if ((value == '\n') || (value == '\r'))
        {
            if (command_line_length != 0U)
            {
                MycoDownlinkCommand parsed;
                command_line[command_line_length] = '\0';
                if (ParseCommandLine(&parsed))
                {
                    pending_command = parsed;
                    __DMB();
                    command_ready = 1U;
                }
                command_line_length = 0U;
            }
        }
        else if (command_line_length < (COMMAND_LINE_SIZE - 1U))
        {
            command_line[command_line_length] = value;
            command_line_length++;
        }
        else
        {
            command_line_length = 0U;
        }
    }
}

bool MycoCommand_Take(MycoDownlinkCommand *command)
{
    uint32_t interrupt_state;

    if ((command == NULL) || (command_ready == 0U))
    {
        return false;
    }

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    if (command_ready == 0U)
    {
        if (interrupt_state == 0U)
        {
            __enable_irq();
        }
        return false;
    }
    *command = pending_command;
    command_ready = 0U;
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }
    return true;
}

bool MycoCommand_TakeForNode(uint32_t node_id,
                            MycoDownlinkCommand *command)
{
    uint32_t interrupt_state;

    if ((command == NULL) || (command_ready == 0U))
    {
        return false;
    }

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    if ((command_ready == 0U) || (pending_command.node_id != node_id))
    {
        if (interrupt_state == 0U)
        {
            __enable_irq();
        }
        return false;
    }
    *command = pending_command;
    command_ready = 0U;
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }
    return true;
}

static void WriteU32(uint8_t *destination, uint32_t value)
{
    destination[0] = (uint8_t)(value >> 24);
    destination[1] = (uint8_t)(value >> 16);
    destination[2] = (uint8_t)(value >> 8);
    destination[3] = (uint8_t)value;
}

void MycoCommand_BuildPacket(const MycoDownlinkCommand *command,
                             uint8_t packet[MYCO_CONFIG_PACKET_SIZE])
{
    if ((command == NULL) || (packet == NULL))
    {
        return;
    }
    packet[0] = 'M';
    packet[1] = 'Y';
    packet[2] = 'C';
    packet[3] = 'O';
    packet[4] = 1U;
    packet[5] = 0x80U;
    WriteU32(&packet[6], command->node_id);
    WriteU32(&packet[10], command->transaction_id);
    WriteU32(&packet[14], command->config_revision);
    WriteU32(&packet[18], command->report_interval_s);
}

void MycoCommand_BuildLinkAck(
    uint32_t node_id,
    uint32_t transmit_sequence,
    uint8_t packet[MYCO_LINK_ACK_PACKET_SIZE])
{
    if (packet == NULL)
    {
        return;
    }
    packet[0] = 'M';
    packet[1] = 'Y';
    packet[2] = 'C';
    packet[3] = 'O';
    packet[4] = 1U;
    packet[5] = 0x82U;
    WriteU32(&packet[6], node_id);
    WriteU32(&packet[10], transmit_sequence);
}
