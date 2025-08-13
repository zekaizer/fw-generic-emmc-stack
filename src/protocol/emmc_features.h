#ifndef EMMC_FEATURES_H
#define EMMC_FEATURES_H

#include "../include/emmc_types.h"

/*
 * eMMC Advanced Features Support
 * 
 * This header declares all advanced eMMC functions.
 * Functions are implemented in emmc_advanced.c and will be automatically
 * removed by the linker if not used (Dead Code Elimination).
 * 
 * No conditional compilation (#ifdef) is used - the linker handles optimization.
 */

/* ========================================================================= */
/* Cache Management                                                          */
/* ========================================================================= */

/**
 * @brief Enable or disable cache
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 * 
 * Note: This function will be removed by linker if not called
 */
emmc_result_t emmc_set_cache_enable(bool enable);

/**
 * @brief Flush internal cache
 * @return EMMC_OK on success, error code otherwise
 * 
 * Note: This function will be removed by linker if not called
 */
emmc_result_t emmc_flush_cache(void);

/* ========================================================================= */
/* Background Operations (BKOPS)                                            */
/* ========================================================================= */

/**
 * @brief Enable or disable background operations
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_bkops_enable(bool enable);

/**
 * @brief Start background operations manually
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_start_bkops(void);

/**
 * @brief Check if background operations are needed
 * @param level Pointer to store BKOPS level
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_check_bkops_status(u8 *level);

/* ========================================================================= */
/* Write Protection                                                         */
/* ========================================================================= */

/**
 * @brief Set write protection on a partition
 * @param partition Target partition
 * @param enable true to enable, false to disable
 * @param permanent true for permanent protection, false for temporary
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_write_protect(emmc_partition_t partition, bool enable, bool permanent);

/**
 * @brief Get write protection status
 * @param partition Target partition  
 * @param enabled Pointer to store protection status
 * @param permanent Pointer to store permanent protection flag
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_write_protect_status(emmc_partition_t partition, bool *enabled, bool *permanent);

/**
 * @brief Set boot partition write protection
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_boot_write_protect(bool enable);

/**
 * @brief Lock/unlock card with password
 * @param password Password bytes
 * @param password_len Password length (1-16 bytes)
 * @param lock true to lock, false to unlock
 * @param force_erase true to force erase if password forgotten
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_lock_unlock(const u8 *password, u8 password_len, bool lock, bool force_erase);

/* ========================================================================= */
/* Sanitize and Secure Erase                                               */
/* ========================================================================= */

/**
 * @brief Sanitize the entire device
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_sanitize(void);

/**
 * @brief Secure erase sectors (cryptographically erase)
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to erase
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_secure_erase(u64 start_sector, u32 sector_count);

/**
 * @brief Secure trim sectors (for wear leveling)
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to trim
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_secure_trim(u64 start_sector, u32 sector_count);

/**
 * @brief TRIM sectors (hint for garbage collection)
 * @param start_sector Starting sector number
 * @param sector_count Number of sectors to trim
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_trim(u64 start_sector, u32 sector_count);

/* ========================================================================= */
/* Health Monitoring                                                        */
/* ========================================================================= */

/**
 * @brief Get device health information
 * @param pre_eol_info Pointer to store pre-EOL information
 * @param device_life_time_est_a Pointer to store device lifetime estimate type A
 * @param device_life_time_est_b Pointer to store device lifetime estimate type B
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_health_info(u8 *pre_eol_info, u8 *device_life_time_est_a, u8 *device_life_time_est_b);

/**
 * @brief Get card temperature (if supported)
 * @param temperature Pointer to store temperature in Celsius
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_get_temperature(s8 *temperature);

/* ========================================================================= */
/* Power Management                                                         */
/* ========================================================================= */

/**
 * @brief Enter sleep mode
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_enter_sleep_mode(void);

/**
 * @brief Exit sleep mode
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_exit_sleep_mode(void);

/**
 * @brief Set power-off notification
 * @param notify_type Notification type (0x00-0x03)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_power_off_notification(u8 notify_type);

/**
 * @brief Enable/disable High Priority Interrupt (HPI)
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_set_hpi_enable(bool enable);

/* ========================================================================= */
/* Feature Detection and Runtime Support                                    */
/* ========================================================================= */

/**
 * @brief Feature availability information
 */
typedef struct {
    bool cache_available;           /* Cache supported by card */
    bool bkops_available;          /* BKOPS supported by card */
    bool sanitize_available;       /* Sanitize supported by card */
    bool write_protect_available;  /* Write protection supported */
    bool secure_erase_available;   /* Secure erase supported */
    bool sleep_available;          /* Sleep mode supported */
    bool hpi_available;            /* HPI supported */
    bool trim_available;           /* TRIM supported */
} emmc_features_t;

/**
 * @brief Get available features based on card capabilities
 * @return Feature availability structure
 * 
 * Note: This function reads EXT_CSD to determine what features are supported
 */
emmc_features_t emmc_get_available_features(void);

/**
 * @brief Check if a specific feature is supported
 * @param feature_name Feature to check (for logging/debugging)
 * @return true if supported, false otherwise
 */
static inline bool emmc_feature_supported(const char *feature_name)
{
    /* This can be expanded for runtime feature checking */
    (void)feature_name; /* Suppress unused warning */
    return true; /* Default: assume all features are supported */
}

/* ========================================================================= */
/* Weak Function Overrides (for custom implementations)                    */
/* ========================================================================= */

/**
 * @brief Weak override for cache policy
 * Override this function to customize cache behavior
 */
__attribute__((weak))
static inline bool emmc_should_enable_cache(void)
{
    return false; /* Default: disable cache (manual enable required) */
}

/**
 * @brief Weak override for BKOPS policy  
 * Override this function to customize BKOPS behavior
 */
__attribute__((weak))
static inline bool emmc_should_enable_bkops(void)
{
    return false; /* Default: disable BKOPS (manual enable required) */
}

/**
 * @brief Weak override for write protection policy
 * Override this function to customize write protection behavior
 */
__attribute__((weak))
static inline bool emmc_should_enable_write_protect(emmc_partition_t partition)
{
    (void)partition;
    return false; /* Default: don't enable write protection */
}

/* ========================================================================= */
/* Advanced Feature Constants                                               */
/* ========================================================================= */

/* BKOPS levels */
#define EMMC_BKOPS_LEVEL_0          0   /* No BKOPS needed */
#define EMMC_BKOPS_LEVEL_1          1   /* Non-critical BKOPS */
#define EMMC_BKOPS_LEVEL_2          2   /* Performance impact */
#define EMMC_BKOPS_LEVEL_3          3   /* Critical BKOPS needed */

/* Power-off notification types */
#define EMMC_POWER_OFF_SHORT        0x01    /* Short power-off */
#define EMMC_POWER_OFF_LONG         0x02    /* Long power-off */
#define EMMC_POWER_OFF_SLEEP        0x03    /* Sleep power-off */

/* Health monitoring thresholds */
#define EMMC_HEALTH_NORMAL          0x01    /* Normal */
#define EMMC_HEALTH_WARNING         0x02    /* Warning - approaching EOL */
#define EMMC_HEALTH_URGENT          0x03    /* Urgent - device failure imminent */

/* Sanitize argument values */
#define EMMC_SANITIZE_OVERWRITE     0x01    /* Overwrite sanitize */
#define EMMC_SANITIZE_BLOCK_ERASE   0x02    /* Block erase sanitize */
#define EMMC_SANITIZE_CRYPTO_ERASE  0x03    /* Crypto erase sanitize */

#endif /* EMMC_FEATURES_H */