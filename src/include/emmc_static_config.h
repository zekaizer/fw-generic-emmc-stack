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
    static const bool EMMC_COMPILE_HS400ES_ONLY = (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400_ES);
    static const bool EMMC_COMPILE_HS400_ONLY = (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400);
    static const bool EMMC_COMPILE_HS200_ONLY = (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS200);
    static const bool EMMC_COMPILE_SDR_ONLY = (!EMMC_COMPILE_HS400ES_ONLY && !EMMC_COMPILE_HS400_ONLY && !EMMC_COMPILE_HS200_ONLY);
#else
    /* Dynamic mode selection (default) */
    static const bool EMMC_COMPILE_HS400ES_ONLY = false;
    static const bool EMMC_COMPILE_HS400_ONLY = false;
    static const bool EMMC_COMPILE_HS200_ONLY = false;
    static const bool EMMC_COMPILE_SDR_ONLY = false;
#endif

/* Static bus width - eliminates width negotiation code */
#ifdef EMMC_STATIC_BUS_WIDTH
    static const bool EMMC_COMPILE_8BIT_ONLY = (EMMC_STATIC_BUS_WIDTH == 8);
    static const bool EMMC_COMPILE_4BIT_ONLY = (EMMC_STATIC_BUS_WIDTH == 4);
    static const bool EMMC_COMPILE_1BIT_ONLY = (EMMC_STATIC_BUS_WIDTH == 1);
#else
    /* Dynamic width selection (default) */
    static const bool EMMC_COMPILE_8BIT_ONLY = false;
    static const bool EMMC_COMPILE_4BIT_ONLY = false;
    static const bool EMMC_COMPILE_1BIT_ONLY = false;
#endif

/* ========================================================================= */
/* Partition Support Static Settings                                        */
/* ========================================================================= */

/* All partitions are always supported */
static const bool EMMC_COMPILE_BOOT_PARTITION = true;
static const bool EMMC_COMPILE_RPMB = true;
static const bool EMMC_COMPILE_GP_PARTITION = true;

/* ========================================================================= */
/* Command Set Static Settings                                              */
/* ========================================================================= */

/* Erase commands are not supported */

/* ========================================================================= */
/* Protocol Feature Static Settings                                         */
/* ========================================================================= */

/* Disable EXT_CSD parsing - use hardcoded defaults */
#ifdef EMMC_DISABLE_EXT_CSD
    static const bool EMMC_COMPILE_EXT_CSD_PARSING = false;
    /* Use static defaults */
    #define EMMC_STATIC_SECTOR_COUNT     (8ULL * 1024 * 1024 * 1024 / 512) /* 8GB default */
    #define EMMC_STATIC_BOOT_SIZE        (4 * 1024 * 1024)                  /* 4MB default */
    #define EMMC_STATIC_RPMB_SIZE        (4 * 1024 * 1024)                  /* 4MB default */
#else
    static const bool EMMC_COMPILE_EXT_CSD_PARSING = true;
#endif

/* Disable card status checking - always assume ready */
#ifdef EMMC_DISABLE_CARD_STATUS_CHECK
    static const bool EMMC_COMPILE_STATUS_CHECK = false;
#else
    static const bool EMMC_COMPILE_STATUS_CHECK = true;
#endif

/* Static sector size (always 512 bytes for eMMC) */
#ifndef EMMC_STATIC_SECTOR_SIZE
    #define EMMC_STATIC_SECTOR_SIZE       512
#endif

/* Disable CID/CSD parsing */
#ifdef EMMC_DISABLE_CARD_IDENTIFICATION
    static const bool EMMC_COMPILE_CID_PARSING = false;
    static const bool EMMC_COMPILE_CSD_PARSING = false;
#else
    static const bool EMMC_COMPILE_CID_PARSING = true;
    static const bool EMMC_COMPILE_CSD_PARSING = true;
#endif

/* ========================================================================= */
/* Transfer Optimization Static Settings                                    */
/* ========================================================================= */

/* Static transfer sizes - eliminate runtime calculation */
#ifdef EMMC_STATIC_TRANSFER_SIZE
    static const bool EMMC_COMPILE_DYNAMIC_TRANSFER_SIZE = false;
    #define EMMC_OPTIMAL_TRANSFER_SIZE          EMMC_STATIC_TRANSFER_SIZE
#else
    static const bool EMMC_COMPILE_DYNAMIC_TRANSFER_SIZE = true;
#endif

/* Disable CMD23 (Predefined block count) */
#ifdef EMMC_DISABLE_CMD23
    static const bool EMMC_COMPILE_CMD23_SUPPORT = false;
#else
    static const bool EMMC_COMPILE_CMD23_SUPPORT = true;
#endif

/* Always use multi-block transfers */
static const bool EMMC_COMPILE_MULTI_BLOCK = true;

/* ========================================================================= */
/* Error Handling Static Settings                                          */
/* ========================================================================= */

/* Minimal error handling - basic error codes only */
#ifdef EMMC_MINIMAL_ERROR_HANDLING
    static const bool EMMC_COMPILE_DETAILED_ERRORS = false;
    static const bool EMMC_COMPILE_ERROR_RECOVERY = false;
#else
    static const bool EMMC_COMPILE_DETAILED_ERRORS = true;
    static const bool EMMC_COMPILE_ERROR_RECOVERY = true;
#endif

/* Disable timeout handling - assume operations always succeed */
#ifdef EMMC_DISABLE_TIMEOUT_HANDLING
    static const bool EMMC_COMPILE_TIMEOUT_HANDLING = false;
#else
    static const bool EMMC_COMPILE_TIMEOUT_HANDLING = true;
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
    static const bool EMMC_COMPILE_DYNAMIC_BUS_MODE = false;
#else
    #define EMMC_STATIC_INLINE_BUS_CONFIG static inline
    static const bool EMMC_COMPILE_DYNAMIC_BUS_MODE = true;
#endif

/* Bus width optimization flags */
#ifdef EMMC_STATIC_BUS_WIDTH
    static const bool EMMC_COMPILE_DYNAMIC_BUS_WIDTH = false;
    static const bool EMMC_HAS_STATIC_BUS_WIDTH = true;
#else
    static const bool EMMC_COMPILE_DYNAMIC_BUS_WIDTH = true;
    static const bool EMMC_HAS_STATIC_BUS_WIDTH = false;
    #define EMMC_STATIC_BUS_WIDTH             0
#endif

/* Bus mode optimization flags */
#ifdef EMMC_STATIC_BUS_MODE
    static const bool EMMC_HAS_STATIC_BUS_MODE = true;
#else
    static const bool EMMC_HAS_STATIC_BUS_MODE = false;
    #define EMMC_STATIC_BUS_MODE              0
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