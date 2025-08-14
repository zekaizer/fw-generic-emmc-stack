#ifndef EMMC_PROTOCOL_H
#define EMMC_PROTOCOL_H

#include "../include/emmc_types.h"
#include "../driver/emmc_core.h"

/* Include RPMB interface - always included, conditionally used */
#include "emmc_rpmb.h"

/* Protocol layer initialization configuration */
typedef struct {
    emmc_driver_config_t driver_config; /* Driver configuration */
    bool auto_optimize;                  /* Automatically optimize bus settings */
    bool enable_advanced_features;      /* Enable HS200/HS400, cache, etc. */
} emmc_protocol_config_t;

/* Block I/O request structure */
typedef struct {
    u64  start_sector;      /* Starting sector number */
    u32  sector_count;      /* Number of sectors to transfer */
    u8   *buffer;          /* Data buffer */
    bool read_operation;   /* true for read, false for write */
    bool reliable_write;   /* Use reliable write (if supported) */
} emmc_block_request_t;


/* Protocol layer function prototypes */

/**
 * @brief Initialize the eMMC protocol stack
 * @param config Protocol configuration
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_protocol_init(const emmc_protocol_config_t *config);

/**
 * @brief Deinitialize the protocol stack
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_protocol_deinit(void);

/**
 * @brief Get protocol stack status
 * @return true if initialized, false otherwise
 */
bool emmc_protocol_is_initialized(void);

/**
 * @brief Perform full eMMC initialization and optimization
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_initialize(void);

/**
 * @brief Read sectors from eMMC
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to read
 * @param buffer Buffer to store read data
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_read_sectors(u64 start_sector, u32 sector_count, u8 *buffer);

/**
 * @brief Write sectors to eMMC
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to write
 * @param buffer Buffer containing data to write
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_write_sectors(u64 start_sector, u32 sector_count, const u8 *buffer);

/**
 * @brief Perform block I/O request
 * @param request Block I/O request structure
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_block_io(const emmc_block_request_t *request);


/**
 * @brief Switch to a different partition
 * @param partition Target partition
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_select_partition(emmc_partition_t partition);

/**
 * @brief Get current active partition
 * @return Current active partition
 */
emmc_partition_t emmc_get_active_partition(void);

/**
 * @brief Get partition size
 * @param partition Partition to query
 * @param size_bytes Pointer to store partition size in bytes
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_partition_size(emmc_partition_t partition, u64 *size_bytes);



/**
 * @brief Get card information
 * @return Pointer to card information structure (read-only)
 */
const emmc_card_info_t* emmc_get_card_info(void);

/**
 * @brief Optimize bus settings for best performance
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_optimize_performance(void);

/**
 * @brief Set bus width and timing mode
 * @param width Desired bus width
 * @param mode Desired timing mode
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_bus_config(emmc_bus_width_t width, emmc_bus_mode_t mode);

/**
 * @brief Get current bus configuration
 * @param width Pointer to store current bus width
 * @param mode Pointer to store current timing mode
 * @param frequency Pointer to store current frequency
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_bus_config(emmc_bus_width_t *width, emmc_bus_mode_t *mode, u32 *frequency);




/* Utility Functions */

/**
 * @brief Convert sector number to LBA
 * @param sector_num Sector number (512-byte sectors)
 * @return LBA value for eMMC commands
 */
static inline u32 emmc_sector_to_lba(u64 sector_num)
{
    return (u32)(sector_num & 0xFFFFFFFF);
}

/**
 * @brief Check if sector number requires extended addressing
 * @param sector_num Sector number to check
 * @return true if extended addressing needed, false otherwise
 */
static inline bool emmc_needs_extended_lba(u64 sector_num)
{
    return sector_num > 0xFFFFFFFF;
}

/**
 * @brief Get optimal transfer size for current bus configuration
 * @return Optimal transfer size in sectors
 */
u32 emmc_get_optimal_transfer_size(void);

/* Constants */
#define EMMC_SECTOR_SIZE            512

/* Maximum transfer sizes */
#define EMMC_MAX_SINGLE_TRANSFER    65536   /* sectors */
#define EMMC_MAX_MULTI_TRANSFER     65535   /* sectors */

#endif /* EMMC_PROTOCOL_H */