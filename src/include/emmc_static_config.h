#ifndef EMMC_STATIC_CONFIG_H
#define EMMC_STATIC_CONFIG_H

/*
 * eMMC Static Configuration for Code Size Optimization
 * 
 * This header allows compile-time configuration to eliminate unused code paths
 * and reduce binary size for resource-constrained embedded systems.
 * 
 * Usage:
 *   1. Define specific configuration macros
 *   2. Use predefined presets from emmc_config_presets.h
 *   3. Let the build system handle the configuration
 * 
 * Note: Static configuration disables runtime flexibility but significantly 
 *       reduces code size through dead code elimination.
 */

#include "emmc_types.h"

/* ========================================================================= */
/* Bus Configuration Static Settings                                        */
/* ========================================================================= */

/* Static bus mode - eliminates mode negotiation code */
#ifdef EMMC_STATIC_BUS_MODE
    #if (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400_ES)
        #define EMMC_COMPILE_HS400ES_ONLY    1
        #define EMMC_COMPILE_HS400_ONLY      0
        #define EMMC_COMPILE_HS200_ONLY      0
        #define EMMC_COMPILE_SDR_ONLY        0
    #elif (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400)
        #define EMMC_COMPILE_HS400ES_ONLY    0
        #define EMMC_COMPILE_HS400_ONLY      1
        #define EMMC_COMPILE_HS200_ONLY      0
        #define EMMC_COMPILE_SDR_ONLY        0
    #elif (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS200)
        #define EMMC_COMPILE_HS400ES_ONLY    0
        #define EMMC_COMPILE_HS400_ONLY      0
        #define EMMC_COMPILE_HS200_ONLY      1
        #define EMMC_COMPILE_SDR_ONLY        0
    #else
        #define EMMC_COMPILE_HS400ES_ONLY    0
        #define EMMC_COMPILE_HS400_ONLY      0
        #define EMMC_COMPILE_HS200_ONLY      0
        #define EMMC_COMPILE_SDR_ONLY        1
    #endif
#else
    /* Dynamic mode selection (default) */
    #define EMMC_COMPILE_HS400ES_ONLY    0
    #define EMMC_COMPILE_HS400_ONLY      0
    #define EMMC_COMPILE_HS200_ONLY      0
    #define EMMC_COMPILE_SDR_ONLY        0
#endif

/* Static bus width - eliminates width negotiation code */
#ifdef EMMC_STATIC_BUS_WIDTH
    #if (EMMC_STATIC_BUS_WIDTH == 8)
        #define EMMC_COMPILE_8BIT_ONLY       1
        #define EMMC_COMPILE_4BIT_ONLY       0
        #define EMMC_COMPILE_1BIT_ONLY       0
    #elif (EMMC_STATIC_BUS_WIDTH == 4)
        #define EMMC_COMPILE_8BIT_ONLY       0
        #define EMMC_COMPILE_4BIT_ONLY       1
        #define EMMC_COMPILE_1BIT_ONLY       0
    #else
        #define EMMC_COMPILE_8BIT_ONLY       0
        #define EMMC_COMPILE_4BIT_ONLY       0
        #define EMMC_COMPILE_1BIT_ONLY       1
    #endif
#else
    /* Dynamic width selection (default) */
    #define EMMC_COMPILE_8BIT_ONLY       0
    #define EMMC_COMPILE_4BIT_ONLY       0
    #define EMMC_COMPILE_1BIT_ONLY       0
#endif

/* ========================================================================= */
/* Partition Support Static Settings                                        */
/* ========================================================================= */

/* All partitions are always supported */
#define EMMC_COMPILE_BOOT_PARTITION  1
#define EMMC_COMPILE_RPMB            1
#define EMMC_COMPILE_GP_PARTITION    1

/* ========================================================================= */
/* Command Set Static Settings                                              */
/* ========================================================================= */

/* Erase commands are not supported */

/* ========================================================================= */
/* Protocol Feature Static Settings                                         */
/* ========================================================================= */

/* Disable EXT_CSD parsing - use hardcoded defaults */
#ifdef EMMC_DISABLE_EXT_CSD
    #define EMMC_COMPILE_EXT_CSD_PARSING  0
    /* Use static defaults */
    #define EMMC_STATIC_SECTOR_COUNT     (8ULL * 1024 * 1024 * 1024 / 512) /* 8GB default */
    #define EMMC_STATIC_BOOT_SIZE        (4 * 1024 * 1024)                  /* 4MB default */
    #define EMMC_STATIC_RPMB_SIZE        (4 * 1024 * 1024)                  /* 4MB default */
#else
    #define EMMC_COMPILE_EXT_CSD_PARSING  1
#endif

/* Disable card status checking - always assume ready */
#ifdef EMMC_DISABLE_CARD_STATUS_CHECK
    #define EMMC_COMPILE_STATUS_CHECK     0
#else
    #define EMMC_COMPILE_STATUS_CHECK     1
#endif

/* Static sector size (always 512 bytes for eMMC) */
#ifndef EMMC_STATIC_SECTOR_SIZE
    #define EMMC_STATIC_SECTOR_SIZE       512
#endif

/* Disable CID/CSD parsing */
#ifdef EMMC_DISABLE_CARD_IDENTIFICATION
    #define EMMC_COMPILE_CID_PARSING      0
    #define EMMC_COMPILE_CSD_PARSING      0
#else
    #define EMMC_COMPILE_CID_PARSING      1
    #define EMMC_COMPILE_CSD_PARSING      1
#endif

/* ========================================================================= */
/* Transfer Optimization Static Settings                                    */
/* ========================================================================= */

/* Static transfer sizes - eliminate runtime calculation */
#ifdef EMMC_STATIC_TRANSFER_SIZE
    #define EMMC_COMPILE_DYNAMIC_TRANSFER_SIZE  0
    #define EMMC_OPTIMAL_TRANSFER_SIZE          EMMC_STATIC_TRANSFER_SIZE
#else
    #define EMMC_COMPILE_DYNAMIC_TRANSFER_SIZE  1
#endif

/* Disable CMD23 (Predefined block count) */
#ifdef EMMC_DISABLE_CMD23
    #define EMMC_COMPILE_CMD23_SUPPORT    0
#else
    #define EMMC_COMPILE_CMD23_SUPPORT    1
#endif

/* Always use multi-block transfers */
#define EMMC_COMPILE_MULTI_BLOCK      1

/* ========================================================================= */
/* Error Handling Static Settings                                          */
/* ========================================================================= */

/* Minimal error handling - basic error codes only */
#ifdef EMMC_MINIMAL_ERROR_HANDLING
    #define EMMC_COMPILE_DETAILED_ERRORS  0
    #define EMMC_COMPILE_ERROR_RECOVERY   0
#else
    #define EMMC_COMPILE_DETAILED_ERRORS  1
    #define EMMC_COMPILE_ERROR_RECOVERY   1
#endif

/* Disable timeout handling - assume operations always succeed */
#ifdef EMMC_DISABLE_TIMEOUT_HANDLING
    #define EMMC_COMPILE_TIMEOUT_HANDLING 0
#else
    #define EMMC_COMPILE_TIMEOUT_HANDLING 1
#endif

/* ========================================================================= */
/* Utility Macros for Conditional Compilation                              */
/* ========================================================================= */

/* Compile-time branch elimination */
#define EMMC_STATIC_IF(condition) if (__builtin_constant_p(condition) && (condition))
#define EMMC_STATIC_IF_NOT(condition) if (__builtin_constant_p(condition) && !(condition))

/* Function inlining hints for static configurations */
#ifdef EMMC_STATIC_BUS_MODE
    #define EMMC_STATIC_INLINE_BUS_CONFIG __attribute__((always_inline)) static inline
#else
    #define EMMC_STATIC_INLINE_BUS_CONFIG static inline
#endif

/* Dead code elimination helpers */
#define EMMC_COMPILE_TIME_ASSERT(condition) \
    do { if (!(condition)) __builtin_unreachable(); } while(0)

/* ========================================================================= */
/* Static Configuration Validation                                          */
/* ========================================================================= */

/* Validate HS400ES configuration */
#if EMMC_COMPILE_HS400ES_ONLY && !EMMC_COMPILE_8BIT_ONLY
    #error "HS400ES mode requires 8-bit bus width"
#endif

/* Validate minimal configuration */
#ifdef EMMC_MINIMAL_CMD_SET
    #if EMMC_COMPILE_BOOT_PARTITION
        #error "Minimal command set is incompatible with boot partition support"
    #endif
    #if EMMC_COMPILE_RPMB
        #error "Minimal command set is incompatible with RPMB support"
    #endif
#endif


#endif /* EMMC_STATIC_CONFIG_H */