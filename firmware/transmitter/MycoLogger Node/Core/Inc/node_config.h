#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define NODE_CONFIG_PROVISIONING_MARKER 0x50524F56UL

typedef struct
{
    uint32_t node_id;
    uint32_t report_interval_ms;
    uint32_t downlink_window_ms;
    uint32_t revision;
    uint32_t last_transaction_id;
} MycoNodeConfig;

enum
{
    NODE_CONFIG_STATUS_APPLIED = 0,
    NODE_CONFIG_STATUS_STALE = 1,
    NODE_CONFIG_STATUS_INVALID_INTERVAL = 2,
    NODE_CONFIG_STATUS_FLASH_ERROR = 3
};

/** Load a valid saved configuration or populate the supplied defaults. */
void NodeConfig_Load(MycoNodeConfig *config,
                     uint32_t default_node_id,
                     uint32_t default_report_interval_ms,
                     uint32_t default_downlink_window_ms);

/**
 * Clear the one-shot marker written by the ST-LINK provisioner.
 * Returns true exactly once after a newly provisioned configuration boots.
 */
bool NodeConfig_ConsumeProvisioningMarker(MycoNodeConfig *config);

/**
 * Validate and persist one complete report-interval transaction.
 * Returns false only when the packet targets a different node.
 */
bool NodeConfig_Apply(MycoNodeConfig *config,
                      uint32_t target_node_id,
                      uint32_t transaction_id,
                      uint32_t revision,
                      uint32_t report_interval_s,
                      uint8_t *status);

#endif /* NODE_CONFIG_H */
