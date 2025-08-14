#ifndef EMMC_CONFIG_PRESETS_H
#define EMMC_CONFIG_PRESETS_H

/*
 * eMMC Configuration Presets for Common Use Cases
 * 
 * This header provides predefined configurations for typical embedded scenarios.
 * Each preset optimizes code size and functionality for specific use cases.
 * 
 * Usage:
 *   #define EMMC_PRESET_TINY     // Before including any eMMC headers
 *   #include "emmc_config_presets.h"
 * 
 * Or use build system:
 *   make EMMC_PRESET=tiny
 *   cmake -DEMMC_PRESET=embedded
 */

/* ========================================================================= */
/* Preset: TINY - Absolute minimum code size                               */
/* ========================================================================= */
#ifdef EMMC_PRESET_TINY
    /* Bus configuration: SDR mode, 1-bit only for maximum compatibility */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_SDR
    #define EMMC_STATIC_BUS_WIDTH        1
    
    /* Partition support: All partitions supported */
    
    /* Command set: Basic read/write only */
    #define EMMC_MINIMAL_CMD_SET
    #define EMMC_DISABLE_CMD23
    
    /* Protocol simplification */
    #define EMMC_DISABLE_EXT_CSD
    #define EMMC_DISABLE_CARD_STATUS_CHECK
    #define EMMC_DISABLE_CARD_IDENTIFICATION
    
    /* Error handling: Minimal */
    #define EMMC_MINIMAL_ERROR_HANDLING
    #define EMMC_DISABLE_TIMEOUT_HANDLING
    
    /* Static transfer size */
    #define EMMC_STATIC_TRANSFER_SIZE    1  /* Single sector only */
    
    /* Estimated code size: ~6-8KB */
#endif

/* ========================================================================= */
/* Preset: EMBEDDED - Balanced performance and size for embedded systems   */
/* ========================================================================= */
#ifdef EMMC_PRESET_EMBEDDED
    /* Bus configuration: HS400ES for best performance, 8-bit bus */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_HS400_ES
    #define EMMC_STATIC_BUS_WIDTH        8
    
    /* Partition support: All partitions supported */
    
    /* Command set: Basic operations + boot support */
    
    /* Keep essential features */
    /* EMMC_COMPILE_CMD23_SUPPORT = 1 (enabled) */
    
    /* Protocol features: Keep card identification */
    /* EMMC_COMPILE_EXT_CSD_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_STATUS_CHECK = 1 (enabled) */
    
    /* Error handling: Full error checking */
    /* EMMC_COMPILE_DETAILED_ERRORS = 1 (enabled) */
    /* EMMC_COMPILE_ERROR_RECOVERY = 1 (enabled) */
    
    /* Optimized transfer size for HS400ES + 8-bit */
    #define EMMC_STATIC_TRANSFER_SIZE    1024  /* 512KB transfers */
    
    /* Estimated code size: ~12-14KB */
#endif

/* ========================================================================= */
/* Preset: SECURE - Security-focused with RPMB support                     */
/* ========================================================================= */
#ifdef EMMC_PRESET_SECURE
    /* Bus configuration: HS200 for good performance with security */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_HS200
    #define EMMC_STATIC_BUS_WIDTH        8
    
    /* Partition support: All partitions supported */
    /* EMMC_COMPILE_RPMB = 1 (enabled) */
    /* EMMC_COMPILE_BOOT_PARTITION = 1 (enabled) */
    /* EMMC_COMPILE_GP_PARTITION = 1 (enabled) */
    
    /* Command set: Basic commands only */
    #define EMMC_DISABLE_SLEEP           /* No power management */
    
    /* Keep essential features */
    /* EMMC_COMPILE_CMD23_SUPPORT = 1 (enabled) */
    
    /* Protocol features: Full card identification for security */
    /* EMMC_COMPILE_EXT_CSD_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_CID_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_STATUS_CHECK = 1 (enabled) */
    
    /* Error handling: Full error checking for security */
    /* EMMC_COMPILE_DETAILED_ERRORS = 1 (enabled) */
    /* EMMC_COMPILE_ERROR_RECOVERY = 1 (enabled) */
    
    /* Transfer size optimized for secure operations */
    #define EMMC_STATIC_TRANSFER_SIZE    256   /* 128KB transfers */
    
    /* Estimated code size: ~16-18KB */
#endif

/* ========================================================================= */
/* Preset: PERFORMANCE - Maximum performance, larger code size acceptable  */
/* ========================================================================= */
#ifdef EMMC_PRESET_PERFORMANCE
    /* Bus configuration: HS400ES for maximum speed */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_HS400_ES
    #define EMMC_STATIC_BUS_WIDTH        8
    
    /* Partition support: All partitions supported */
    /* EMMC_COMPILE_BOOT_PARTITION = 1 (enabled) */
    /* EMMC_COMPILE_RPMB = 1 (enabled) */
    /* EMMC_COMPILE_GP_PARTITION = 1 (enabled) */
    
    /* Command set: All commands for maximum functionality */
    /* All commands enabled by default */
    
    /* Protocol features: Full feature set */
    /* EMMC_COMPILE_EXT_CSD_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_CID_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_STATUS_CHECK = 1 (enabled) */
    
    /* Error handling: Full error handling */
    /* EMMC_COMPILE_DETAILED_ERRORS = 1 (enabled) */
    /* EMMC_COMPILE_ERROR_RECOVERY = 1 (enabled) */
    
    /* Large transfer sizes for maximum throughput */
    #define EMMC_STATIC_TRANSFER_SIZE    2048  /* 1MB transfers */
    
    /* Estimated code size: ~20-22KB (full size) */
#endif

/* ========================================================================= */
/* Preset: BOOTLOADER - Optimized for bootloader use case                  */
/* ========================================================================= */
#ifdef EMMC_PRESET_BOOTLOADER
    /* Bus configuration: HS200 for reliability */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_HS200
    #define EMMC_STATIC_BUS_WIDTH        8
    
    /* Partition support: All partitions supported */
    /* EMMC_COMPILE_BOOT_PARTITION = 1 (enabled) */
    /* EMMC_COMPILE_RPMB = 1 (enabled) */
    /* EMMC_COMPILE_GP_PARTITION = 1 (enabled) */
    
    /* Command set: Basic + boot commands */
    
    /* Keep essential features for emergency recovery */
    /* EMMC_COMPILE_CMD23_SUPPORT = 1 (enabled) */
    
    /* Protocol features: Minimal parsing */
    #define EMMC_DISABLE_CARD_IDENTIFICATION  /* Don't need card details */
    /* EMMC_COMPILE_EXT_CSD_PARSING = 1 (for boot config) */
    /* EMMC_COMPILE_STATUS_CHECK = 1 (for reliability) */
    
    /* Error handling: Essential only */
    /* EMMC_COMPILE_ERROR_RECOVERY = 1 (for reliability) */
    
    /* Medium transfer sizes for boot code loading */
    #define EMMC_STATIC_TRANSFER_SIZE    512   /* 256KB transfers */
    
    /* Estimated code size: ~10-12KB */
#endif

/* ========================================================================= */
/* Preset: RECOVERY - For recovery/update scenarios                        */
/* ========================================================================= */
#ifdef EMMC_PRESET_RECOVERY
    /* Bus configuration: Conservative for reliability */
    #define EMMC_STATIC_BUS_MODE         EMMC_MODE_HS200
    #define EMMC_STATIC_BUS_WIDTH        8
    
    /* Partition support: All partitions supported */
    /* EMMC_COMPILE_BOOT_PARTITION = 1 (enabled) */
    /* EMMC_COMPILE_RPMB = 1 (enabled) */
    /* EMMC_COMPILE_GP_PARTITION = 1 (enabled) */
    
    /* Command set: Basic commands for recovery */
    #define EMMC_DISABLE_BKOPS           /* No background ops during recovery */
    
    /* Keep advanced transfers */
    /* EMMC_COMPILE_CMD23_SUPPORT = 1 (enabled) */
    
    /* Protocol features: Full identification for recovery validation */
    /* EMMC_COMPILE_EXT_CSD_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_CID_PARSING = 1 (enabled) */
    /* EMMC_COMPILE_STATUS_CHECK = 1 (enabled) */
    
    /* Error handling: Full error handling for recovery robustness */
    /* EMMC_COMPILE_DETAILED_ERRORS = 1 (enabled) */
    /* EMMC_COMPILE_ERROR_RECOVERY = 1 (enabled) */
    
    /* Medium transfer sizes for balanced performance */
    #define EMMC_STATIC_TRANSFER_SIZE    512   /* 256KB transfers */
    
    /* Estimated code size: ~16-18KB */
#endif

/* ========================================================================= */
/* Preset Validation and Conflict Resolution                               */
/* ========================================================================= */

/* Check for multiple presets */
#if defined(EMMC_PRESET_TINY) + defined(EMMC_PRESET_EMBEDDED) + \
    defined(EMMC_PRESET_SECURE) + defined(EMMC_PRESET_PERFORMANCE) + \
    defined(EMMC_PRESET_BOOTLOADER) + defined(EMMC_PRESET_RECOVERY) > 1
    #error "Only one eMMC preset can be defined at a time"
#endif

/* Provide default if no preset is selected */
#if !defined(EMMC_PRESET_TINY) && !defined(EMMC_PRESET_EMBEDDED) && \
    !defined(EMMC_PRESET_SECURE) && !defined(EMMC_PRESET_PERFORMANCE) && \
    !defined(EMMC_PRESET_BOOTLOADER) && !defined(EMMC_PRESET_RECOVERY)
    /* No preset selected - use dynamic configuration (full features) */
    #warning "No eMMC preset selected, using dynamic configuration (largest size)"
#endif

/* ========================================================================= */
/* Preset Information for Build System                                     */
/* ========================================================================= */

/* Macro to get current preset name as string */
#ifdef EMMC_PRESET_TINY
    #define EMMC_CURRENT_PRESET_NAME     "tiny"
    #define EMMC_ESTIMATED_CODE_SIZE     "6-8KB"
#elif defined(EMMC_PRESET_EMBEDDED)
    #define EMMC_CURRENT_PRESET_NAME     "embedded"  
    #define EMMC_ESTIMATED_CODE_SIZE     "12-14KB"
#elif defined(EMMC_PRESET_SECURE)
    #define EMMC_CURRENT_PRESET_NAME     "secure"
    #define EMMC_ESTIMATED_CODE_SIZE     "16-18KB"
#elif defined(EMMC_PRESET_PERFORMANCE)
    #define EMMC_CURRENT_PRESET_NAME     "performance"
    #define EMMC_ESTIMATED_CODE_SIZE     "20-22KB"
#elif defined(EMMC_PRESET_BOOTLOADER)
    #define EMMC_CURRENT_PRESET_NAME     "bootloader"
    #define EMMC_ESTIMATED_CODE_SIZE     "10-12KB"
#elif defined(EMMC_PRESET_RECOVERY)
    #define EMMC_CURRENT_PRESET_NAME     "recovery"
    #define EMMC_ESTIMATED_CODE_SIZE     "16-18KB"
#else
    #define EMMC_CURRENT_PRESET_NAME     "dynamic"
    #define EMMC_ESTIMATED_CODE_SIZE     "20-25KB"
#endif

/* Include the static configuration after preset is applied */
#include "emmc_static_config.h"

#endif /* EMMC_CONFIG_PRESETS_H */