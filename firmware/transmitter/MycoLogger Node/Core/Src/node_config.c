#include "node_config.h"
#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_flash_ex.h"
#include <string.h>

#define NODE_CONFIG_FLASH_ADDRESS       0x08007800UL
#define NODE_CONFIG_FLASH_PAGE          15U
#define NODE_CONFIG_MAGIC               0x4D594346UL
#define NODE_CONFIG_LAYOUT_VERSION      1U
#define NODE_CONFIG_MIN_INTERVAL_S      15U
#define NODE_CONFIG_MAX_INTERVAL_S      604800U

typedef struct
{
    uint32_t magic;
    uint32_t layout_version;
    uint32_t node_id;
    uint32_t report_interval_ms;
    uint32_t downlink_window_ms;
    uint32_t revision;
    uint32_t last_transaction_id;
    uint32_t checksum;
} StoredNodeConfig;

static uint32_t CalculateChecksum(const StoredNodeConfig *stored)
{
    const uint8_t *bytes = (const uint8_t *)stored;
    uint32_t hash = 2166136261UL;

    for (uint32_t index = 0U;
         index < (sizeof(StoredNodeConfig) - sizeof(uint32_t));
         index++)
    {
        hash ^= bytes[index];
        hash *= 16777619UL;
    }
    return hash;
}

static bool StoredConfigIsValid(const StoredNodeConfig *stored)
{
    return (stored->magic == NODE_CONFIG_MAGIC) &&
           (stored->layout_version == NODE_CONFIG_LAYOUT_VERSION) &&
           (stored->node_id != 0U) &&
           ((stored->report_interval_ms / 1000U) >=
            NODE_CONFIG_MIN_INTERVAL_S) &&
           ((stored->report_interval_ms / 1000U) <=
            NODE_CONFIG_MAX_INTERVAL_S) &&
           (stored->checksum == CalculateChecksum(stored));
}

static bool SaveConfig(const MycoNodeConfig *config)
{
    StoredNodeConfig stored = {
        .magic = NODE_CONFIG_MAGIC,
        .layout_version = NODE_CONFIG_LAYOUT_VERSION,
        .node_id = config->node_id,
        .report_interval_ms = config->report_interval_ms,
        .downlink_window_ms = config->downlink_window_ms,
        .revision = config->revision,
        .last_transaction_id = config->last_transaction_id,
        .checksum = 0U
    };
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .Page = NODE_CONFIG_FLASH_PAGE,
        .NbPages = 1U
    };
    uint32_t page_error = 0U;
    bool success = true;

    stored.checksum = CalculateChecksum(&stored);

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    {
        success = false;
    }

    for (uint32_t index = 0U;
         success && (index < (sizeof(stored) / sizeof(uint64_t)));
         index++)
    {
        uint64_t double_word;
        memcpy(&double_word,
               ((const uint8_t *)&stored) + (index * sizeof(uint64_t)),
               sizeof(double_word));
        if (HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_DOUBLEWORD,
                NODE_CONFIG_FLASH_ADDRESS + (index * sizeof(uint64_t)),
                double_word) != HAL_OK)
        {
            success = false;
        }
    }

    (void)HAL_FLASH_Lock();

    return success &&
           StoredConfigIsValid(
               (const StoredNodeConfig *)NODE_CONFIG_FLASH_ADDRESS);
}

void NodeConfig_Load(MycoNodeConfig *config,
                     uint32_t default_node_id,
                     uint32_t default_report_interval_ms,
                     uint32_t default_downlink_window_ms)
{
    const StoredNodeConfig *stored =
        (const StoredNodeConfig *)NODE_CONFIG_FLASH_ADDRESS;

    if (config == NULL)
    {
        return;
    }

    if (StoredConfigIsValid(stored))
    {
        config->node_id = stored->node_id;
        config->report_interval_ms = stored->report_interval_ms;
        config->downlink_window_ms = stored->downlink_window_ms;
        config->revision = stored->revision;
        config->last_transaction_id = stored->last_transaction_id;
        return;
    }

    config->node_id = default_node_id;
    config->report_interval_ms = default_report_interval_ms;
    config->downlink_window_ms = default_downlink_window_ms;
    config->revision = 0U;
    config->last_transaction_id = 0U;
}

bool NodeConfig_ConsumeProvisioningMarker(MycoNodeConfig *config)
{
    MycoNodeConfig acknowledged;

    if ((config == NULL) ||
        (config->last_transaction_id != NODE_CONFIG_PROVISIONING_MARKER))
    {
        return false;
    }

    acknowledged = *config;
    acknowledged.last_transaction_id = 0U;
    if (!SaveConfig(&acknowledged))
    {
        return false;
    }

    *config = acknowledged;
    return true;
}

bool NodeConfig_Apply(MycoNodeConfig *config,
                      uint32_t target_node_id,
                      uint32_t transaction_id,
                      uint32_t revision,
                      uint32_t report_interval_s,
                      uint8_t *status)
{
    MycoNodeConfig candidate;

    if ((config == NULL) || (status == NULL) ||
        (target_node_id != config->node_id))
    {
        return false;
    }

    if ((transaction_id == config->last_transaction_id) &&
        (revision == config->revision))
    {
        *status = NODE_CONFIG_STATUS_APPLIED;
        return true;
    }

    if (revision <= config->revision)
    {
        *status = NODE_CONFIG_STATUS_STALE;
        return true;
    }

    if ((report_interval_s < NODE_CONFIG_MIN_INTERVAL_S) ||
        (report_interval_s > NODE_CONFIG_MAX_INTERVAL_S))
    {
        *status = NODE_CONFIG_STATUS_INVALID_INTERVAL;
        return true;
    }

    candidate = *config;
    candidate.report_interval_ms = report_interval_s * 1000U;
    candidate.revision = revision;
    candidate.last_transaction_id = transaction_id;

    if (!SaveConfig(&candidate))
    {
        *status = NODE_CONFIG_STATUS_FLASH_ERROR;
        return true;
    }

    *config = candidate;
    *status = NODE_CONFIG_STATUS_APPLIED;
    return true;
}
