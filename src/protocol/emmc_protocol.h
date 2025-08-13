#ifndef EMMC_PROTOCOL_H
#define EMMC_PROTOCOL_H

#include "../include/emmc_types.h"
#include "../driver/emmc_core.h"

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

/* RPMB operation types */
typedef enum {
    EMMC_RPMB_WRITE_KEY     = 0x01,
    EMMC_RPMB_READ_WCOUNTER = 0x02,
    EMMC_RPMB_WRITE_DATA    = 0x03,
    EMMC_RPMB_READ_DATA     = 0x04,
    EMMC_RPMB_READ_RESULT   = 0x05
} emmc_rpmb_op_type_t;

/* RPMB request/response structure */
typedef struct {
    u8  stuff[196];         /* Stuff bytes */
    u8  key_mac[32];        /* Authentication key or MAC */
    u8  data[256];          /* Data payload */
    u8  nonce[16];          /* Nonce */
    u32 write_counter;      /* Write counter */
    u16 address;            /* Block address */
    u16 block_count;        /* Block count */
    u16 result;             /* Result code */
    u16 req_resp;           /* Request/Response type */
} __attribute__((packed)) emmc_rpmb_frame_t;

/* RPMB external crypto interface */
typedef struct {
    /* External key injection function */
    emmc_result_t (*inject_key)(const u8 *key, u32 key_len);
    
    /* External HMAC-SHA256 computation function */
    emmc_result_t (*compute_hmac)(const u8 *key, u32 key_len,
                                 const u8 *data, u32 data_len,
                                 u8 *mac, u32 mac_len);
    
    /* External MAC verification function */
    emmc_result_t (*verify_hmac)(const u8 *key, u32 key_len,
                                const u8 *data, u32 data_len,
                                const u8 *expected_mac, u32 mac_len);
} emmc_rpmb_crypto_interface_t;

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
 * @brief Erase sectors
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to erase
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_erase_sectors(u64 start_sector, u32 sector_count);

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

/* Write protection and other advanced functions moved to emmc_features.h */

/* Cache functions moved to emmc_features.h */

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

/* RPMB Functions */

/**
 * @brief Initialize RPMB with external crypto interface
 * @param crypto_interface External cryptographic functions
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface);

/**
 * @brief Program RPMB authentication key
 * @param key Authentication key (32 bytes)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_program_key(const u8 *key);

/**
 * @brief Read RPMB write counter
 * @param counter Pointer to store write counter value
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_get_write_counter(u32 *counter);

/**
 * @brief Write data to RPMB partition
 * @param address Block address (0-based)
 * @param data Data to write (256 bytes per block)
 * @param block_count Number of blocks to write
 * @param key Authentication key (32 bytes)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_write_data(u16 address, const u8 *data, u16 block_count, const u8 *key);

/**
 * @brief Read data from RPMB partition
 * @param address Block address (0-based)
 * @param data Buffer to store read data (256 bytes per block)
 * @param block_count Number of blocks to read
 * @param key Authentication key (32 bytes)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_read_data(u16 address, u8 *data, u16 block_count, const u8 *key);

/* Boot Functions */

/**
 * @brief Configure boot partition
 * @param boot_partition Boot partition selection (BOOT1 or BOOT2)
 * @param enable_boot_ack Enable boot acknowledge
 * @param boot_bus_width Boot bus width
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_configure_boot(emmc_partition_t boot_partition, bool enable_boot_ack, 
                                 emmc_bus_width_t boot_bus_width);

/**
 * @brief Write boot code to boot partition
 * @param boot_partition Target boot partition
 * @param boot_code Boot code data
 * @param code_size Size of boot code in bytes
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_write_boot_code(emmc_partition_t boot_partition, const u8 *boot_code, u32 code_size);

/* Advanced Features - Include optional advanced functions */
#include "emmc_features.h"

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
#define EMMC_RPMB_BLOCK_SIZE        256
#define EMMC_RPMB_KEY_SIZE          32
#define EMMC_RPMB_MAC_SIZE          32
#define EMMC_RPMB_NONCE_SIZE        16

/* Maximum transfer sizes */
#define EMMC_MAX_SINGLE_TRANSFER    65536   /* sectors */
#define EMMC_MAX_MULTI_TRANSFER     65535   /* sectors */

/* RPMB result codes */
#define EMMC_RPMB_RESULT_OK                 0x0000
#define EMMC_RPMB_RESULT_GENERAL_FAILURE    0x0001
#define EMMC_RPMB_RESULT_AUTH_FAILURE       0x0002
#define EMMC_RPMB_RESULT_COUNTER_FAILURE    0x0003
#define EMMC_RPMB_RESULT_ADDRESS_FAILURE    0x0004
#define EMMC_RPMB_RESULT_WRITE_FAILURE      0x0005
#define EMMC_RPMB_RESULT_READ_FAILURE       0x0006
#define EMMC_RPMB_RESULT_KEY_NOT_PROGRAMMED 0x0007

#endif /* EMMC_PROTOCOL_H */