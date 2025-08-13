#ifndef EMMC_CORE_H
#define EMMC_CORE_H

#include "../include/emmc_types.h"
#include "../include/emmc_cmd.h"
#include "../hal/hal_emmc.h"

/* eMMC driver initialization parameters */
typedef struct {
    hal_emmc_config_t hal_config;   /* HAL configuration */
    u32 init_timeout_ms;            /* Initialization timeout */
    bool enable_cache;              /* Enable card cache */
    bool enable_bkops;              /* Enable background operations */
} emmc_driver_config_t;

/* eMMC driver context */
typedef struct {
    emmc_card_info_t    card_info;      /* Card information */
    hal_emmc_config_t   hal_config;     /* HAL configuration */
    emmc_driver_config_t driver_config; /* Driver configuration */
    bool                initialized;    /* Driver initialization status */
    u32                 current_clock;  /* Current clock frequency */
    emmc_partition_t    active_part;    /* Currently active partition */
} emmc_driver_context_t;

/* Driver function prototypes */

/**
 * @brief Initialize the eMMC driver
 * @param config Driver configuration
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_driver_init(const emmc_driver_config_t *config);

/**
 * @brief Deinitialize the eMMC driver
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_driver_deinit(void);

/**
 * @brief Get driver context (read-only)
 * @return Pointer to driver context
 */
const emmc_driver_context_t* emmc_driver_get_context(void);

/**
 * @brief Perform card identification and initialization
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_card_initialize(void);

/**
 * @brief Send a command to the eMMC card
 * @param cmd_index Command index
 * @param argument Command argument
 * @param resp_type Expected response type
 * @param response Buffer for response (can be NULL for no response)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_send_command(u8 cmd_index, u32 argument, 
                               emmc_resp_type_t resp_type, u32 *response);

/**
 * @brief Send a command with data transfer
 * @param cmd_index Command index
 * @param argument Command argument
 * @param resp_type Expected response type
 * @param buffer Data buffer
 * @param block_size Block size in bytes
 * @param block_count Number of blocks
 * @param read_operation true for read, false for write
 * @param response Buffer for response (can be NULL)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_send_command_with_data(u8 cmd_index, u32 argument,
                                         emmc_resp_type_t resp_type,
                                         u8 *buffer, u32 block_size, 
                                         u32 block_count, bool read_operation,
                                         u32 *response);

/**
 * @brief Get card status
 * @param status Pointer to store card status
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_card_status(u32 *status);

/**
 * @brief Check if card is ready for data transfer
 * @return true if ready, false otherwise
 */
bool emmc_is_card_ready(void);

/**
 * @brief Read the Card Identification (CID) register
 * @param cid Pointer to store CID data
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_read_cid(emmc_cid_t *cid);

/**
 * @brief Read the Card Specific Data (CSD) register
 * @param csd Pointer to store CSD data
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_read_csd(emmc_csd_t *csd);

/**
 * @brief Read the Extended CSD register
 * @param ext_csd Pointer to store EXT_CSD data
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_read_ext_csd(emmc_ext_csd_t *ext_csd);

/**
 * @brief Switch to a different partition
 * @param partition Target partition
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_switch_partition(emmc_partition_t partition);

/**
 * @brief Switch card mode (timing, bus width, etc.)
 * @param access_mode Access mode for SWITCH command
 * @param index EXT_CSD register index
 * @param value New value to set
 * @param timeout_ms Timeout in milliseconds
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_switch_mode(u8 access_mode, u8 index, u8 value, u32 timeout_ms);

/**
 * @brief Set bus width
 * @param width Desired bus width
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_bus_width(emmc_bus_width_t width);

/**
 * @brief Set high speed mode
 * @param mode High speed mode
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_timing_mode(emmc_bus_mode_t mode);

/**
 * @brief Set block length
 * @param block_len Block length in bytes
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_block_length(u32 block_len);

/**
 * @brief Perform bus test
 * @param width Bus width to test
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_bus_test(emmc_bus_width_t width);

/**
 * @brief Calculate card capacity from CSD
 * @param csd CSD register data
 * @param ext_csd EXT_CSD register data
 * @return Card capacity in bytes
 */
u64 emmc_calculate_capacity(const emmc_csd_t *csd, const emmc_ext_csd_t *ext_csd);

/**
 * @brief Parse CID register response
 * @param response Raw CID response (4 x 32-bit words)
 * @param cid Parsed CID structure
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_parse_cid(const u32 *response, emmc_cid_t *cid);

/**
 * @brief Parse CSD register response
 * @param response Raw CSD response (4 x 32-bit words)
 * @param csd Parsed CSD structure
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_parse_csd(const u32 *response, emmc_csd_t *csd);

/**
 * @brief Parse EXT_CSD register data
 * @param buffer Raw EXT_CSD data (512 bytes)
 * @param ext_csd Parsed EXT_CSD structure
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_parse_ext_csd(const u8 *buffer, emmc_ext_csd_t *ext_csd);

/**
 * @brief Check and handle card status errors
 * @param status Card status value
 * @return EMMC_OK if no errors, error code otherwise
 */
emmc_result_t emmc_check_card_status(u32 status);

/**
 * @brief Wait for card to be in specified state
 * @param target_state Target card state
 * @param timeout_ms Timeout in milliseconds
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_wait_for_state(emmc_state_t target_state, u32 timeout_ms);

/**
 * @brief Enable or disable card cache
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_cache_enable(bool enable);

/**
 * @brief Flush card cache
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_flush_cache(void);

/**
 * @brief Enable or disable background operations
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_bkops_enable(bool enable);

/**
 * @brief Get card temperature (if supported)
 * @param temperature Pointer to store temperature in Celsius
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_temperature(s8 *temperature);

/* Helper macros */
#define EMMC_BLOCK_SIZE         512
#define EMMC_MAX_BLOCK_COUNT    65535

/* Default timeouts */
#define EMMC_CMD_TIMEOUT_MS     5000
#define EMMC_DATA_TIMEOUT_MS    10000
#define EMMC_INIT_TIMEOUT_MS    1000
#define EMMC_SWITCH_TIMEOUT_MS  500

/* Card state extraction from status */
#define EMMC_GET_CARD_STATE(status) \
    ((emmc_state_t)(((status) & EMMC_R1_STATE_MASK) >> EMMC_R1_STATE_SHIFT))

#endif /* EMMC_CORE_H */