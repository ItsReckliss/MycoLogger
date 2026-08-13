#include "node_config.h"
#include "stm32u0xx_hal.h"
#include "stm32u0xx_hal_flash_ex.h"
#include <string.h>

#define NODE_CONFIG_PAGE_SIZE          0x800UL
#define NODE_CONFIG_PAGE_A_ADDRESS  0x08007000UL
#define NODE_CONFIG_PAGE_B_ADDRESS  0x08007800UL
#define NODE_CONFIG_PAGE_A                14U
#define NODE_CONFIG_PAGE_B                15U
#define NODE_CONFIG_MAGIC        0x4D594346UL
#define NODE_CONFIG_LAYOUT_VERSION       2U
#define NODE_CONFIG_LEGACY_VERSION       1U
#define NODE_CONFIG_MIN_INTERVAL_S      15U
#define NODE_CONFIG_MAX_INTERVAL_S  604800U

typedef struct
{
    uint32_t magic;
    uint32_t layout_version;
    uint32_t generation;
    uint32_t node_id;
    uint32_t report_interval_ms;
    uint32_t downlink_window_ms;
    uint32_t revision;
    uint32_t last_transaction_id;
    uint32_t reserved;
    uint32_t checksum;
} StoredNodeConfig;

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
} LegacyNodeConfig;

static uint32_t CalculateChecksum(const void *record, uint32_t size)
{
    const uint8_t *bytes = (const uint8_t *)record;
    uint32_t hash = 2166136261UL;

    for (uint32_t index = 0U; index < (size - sizeof(uint32_t)); index++)
    {
        hash ^= bytes[index];
        hash *= 16777619UL;
    }
    return hash;
}

static bool ConfigFieldsAreValid(uint32_t node_id,
                                 uint32_t report_interval_ms,
                                 uint32_t downlink_window_ms)
{
    return (node_id != 0U) &&
           ((report_interval_ms % 1000U) == 0U) &&
           ((report_interval_ms / 1000U) >= NODE_CONFIG_MIN_INTERVAL_S) &&
           ((report_interval_ms / 1000U) <= NODE_CONFIG_MAX_INTERVAL_S) &&
           (downlink_window_ms >= 100U) && (downlink_window_ms <= 60000U);
}

static bool StoredConfigIsValid(const StoredNodeConfig *stored)
{
    return (stored->magic == NODE_CONFIG_MAGIC) &&
           (stored->layout_version == NODE_CONFIG_LAYOUT_VERSION) &&
           ConfigFieldsAreValid(stored->node_id, stored->report_interval_ms,
                                stored->downlink_window_ms) &&
           (stored->checksum == CalculateChecksum(stored, sizeof(*stored)));
}

static bool LegacyConfigIsValid(const LegacyNodeConfig *stored)
{
    return (stored->magic == NODE_CONFIG_MAGIC) &&
           (stored->layout_version == NODE_CONFIG_LEGACY_VERSION) &&
           ConfigFieldsAreValid(stored->node_id, stored->report_interval_ms,
                                stored->downlink_window_ms) &&
           (stored->checksum == CalculateChecksum(stored, sizeof(*stored)));
}

static const StoredNodeConfig *NewestStoredConfig(void)
{
    const StoredNodeConfig *a = (const StoredNodeConfig *)NODE_CONFIG_PAGE_A_ADDRESS;
    const StoredNodeConfig *b = (const StoredNodeConfig *)NODE_CONFIG_PAGE_B_ADDRESS;
    bool a_valid = StoredConfigIsValid(a);
    bool b_valid = StoredConfigIsValid(b);

    if (!a_valid)
    {
        return b_valid ? b : NULL;
    }
    if (!b_valid)
    {
        return a;
    }
    return ((int32_t)(a->generation - b->generation) > 0) ? a : b;
}

static bool WriteRecord(uint32_t address, uint32_t page,
                        const StoredNodeConfig *stored)
{
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .Page = page,
        .NbPages = 1U
    };
    uint32_t page_error = 0U;
    bool success = HAL_FLASH_Unlock() == HAL_OK;

    if (success && (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK))
    {
        success = false;
    }
    for (uint32_t index = 0U;
         success && (index < (sizeof(*stored) / sizeof(uint64_t))); index++)
    {
        uint64_t double_word;
        memcpy(&double_word, ((const uint8_t *)stored) +
               (index * sizeof(uint64_t)), sizeof(double_word));
        success = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                    address + (index * sizeof(uint64_t)),
                                    double_word) == HAL_OK;
    }
    (void)HAL_FLASH_Lock();
    return success && StoredConfigIsValid((const StoredNodeConfig *)address) &&
           (((const StoredNodeConfig *)address)->generation == stored->generation);
}

static bool SaveConfig(MycoNodeConfig *config)
{
    const StoredNodeConfig *current = NewestStoredConfig();
    const LegacyNodeConfig *legacy = (const LegacyNodeConfig *)NODE_CONFIG_PAGE_B_ADDRESS;
    uint32_t target_address = NODE_CONFIG_PAGE_A_ADDRESS;
    uint32_t target_page = NODE_CONFIG_PAGE_A;
    StoredNodeConfig stored = {0};

    if (current != NULL)
    {
        target_address = ((uint32_t)current == NODE_CONFIG_PAGE_A_ADDRESS)
            ? NODE_CONFIG_PAGE_B_ADDRESS : NODE_CONFIG_PAGE_A_ADDRESS;
        target_page = (target_address == NODE_CONFIG_PAGE_A_ADDRESS)
            ? NODE_CONFIG_PAGE_A : NODE_CONFIG_PAGE_B;
        stored.generation = current->generation + 1U;
    }
    else if (LegacyConfigIsValid(legacy))
    {
        /* A v1 record remains intact while its first v2 successor is verified. */
        stored.generation = 1U;
    }
    else
    {
        stored.generation = 1U;
    }

    stored.magic = NODE_CONFIG_MAGIC;
    stored.layout_version = NODE_CONFIG_LAYOUT_VERSION;
    stored.node_id = config->node_id;
    stored.report_interval_ms = config->report_interval_ms;
    stored.downlink_window_ms = config->downlink_window_ms;
    stored.revision = config->revision;
    stored.last_transaction_id = config->last_transaction_id;
    stored.checksum = CalculateChecksum(&stored, sizeof(stored));

    if (!WriteRecord(target_address, target_page, &stored))
    {
        return false;
    }
    config->generation = stored.generation;
    return true;
}

static void LoadFields(MycoNodeConfig *config, uint32_t node_id,
                       uint32_t interval, uint32_t window, uint32_t revision,
                       uint32_t transaction, uint32_t generation)
{
    config->node_id = node_id;
    config->report_interval_ms = interval;
    config->downlink_window_ms = window;
    config->revision = revision;
    config->last_transaction_id = transaction;
    config->generation = generation;
}

void NodeConfig_Load(MycoNodeConfig *config, uint32_t default_node_id,
                     uint32_t default_report_interval_ms,
                     uint32_t default_downlink_window_ms)
{
    const StoredNodeConfig *stored;
    const LegacyNodeConfig *legacy;
    if (config == NULL) return;
    stored = NewestStoredConfig();
    if (stored != NULL)
    {
        LoadFields(config, stored->node_id, stored->report_interval_ms,
                   stored->downlink_window_ms, stored->revision,
                   stored->last_transaction_id, stored->generation);
        return;
    }
    legacy = (const LegacyNodeConfig *)NODE_CONFIG_PAGE_B_ADDRESS;
    if (LegacyConfigIsValid(legacy))
    {
        LoadFields(config, legacy->node_id, legacy->report_interval_ms,
                   legacy->downlink_window_ms, legacy->revision,
                   legacy->last_transaction_id, 0U);
        return;
    }
    LoadFields(config, default_node_id, default_report_interval_ms,
               default_downlink_window_ms, 0U, 0U, 0U);
}

bool NodeConfig_ConsumeProvisioningMarker(MycoNodeConfig *config)
{
    MycoNodeConfig acknowledged;
    if ((config == NULL) ||
        (config->last_transaction_id != NODE_CONFIG_PROVISIONING_MARKER)) return false;
    acknowledged = *config;
    acknowledged.last_transaction_id = 0U;
    if (!SaveConfig(&acknowledged)) return false;
    *config = acknowledged;
    return true;
}

bool NodeConfig_Apply(MycoNodeConfig *config, uint32_t target_node_id,
                      uint32_t transaction_id, uint32_t revision,
                      uint32_t report_interval_s, uint32_t downlink_window_ms,
                      uint8_t *status)
{
    MycoNodeConfig candidate;
    if ((config == NULL) || (status == NULL) || (target_node_id != config->node_id)) return false;
    if ((transaction_id == config->last_transaction_id) && (revision == config->revision))
    { *status = NODE_CONFIG_STATUS_APPLIED; return true; }
    if (revision <= config->revision)
    { *status = NODE_CONFIG_STATUS_STALE; return true; }
    if ((report_interval_s < NODE_CONFIG_MIN_INTERVAL_S) ||
        (report_interval_s > NODE_CONFIG_MAX_INTERVAL_S))
    { *status = NODE_CONFIG_STATUS_INVALID_INTERVAL; return true; }
    if ((downlink_window_ms < 100U) || (downlink_window_ms > 60000U))
    { *status = NODE_CONFIG_STATUS_INVALID_INTERVAL; return true; }
    candidate = *config;
    candidate.report_interval_ms = report_interval_s * 1000U;
    candidate.downlink_window_ms = downlink_window_ms;
    candidate.revision = revision;
    candidate.last_transaction_id = transaction_id;
    if (!SaveConfig(&candidate)) { *status = NODE_CONFIG_STATUS_FLASH_ERROR; return true; }
    *config = candidate;
    *status = NODE_CONFIG_STATUS_APPLIED;
    return true;
}
