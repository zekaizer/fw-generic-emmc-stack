#include "../include/emmc_config_presets.h"  /* Include static configuration first */
#include "emmc_protocol.h"
#include <string.h>

/* Protocol layer context */
static struct {
    emmc_protocol_config_t config;
    bool initialized;
    u32 optimal_transfer_size;
} g_protocol_ctx = {0};

emmc_result_t emmc_protocol_init(const emmc_protocol_config_t *config)
{
    emmc_result_t result;
    
    if (!config) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Initialize driver layer */
    result = emmc_driver_init(&config->driver_config);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Store configuration */
    g_protocol_ctx.config = *config;
    g_protocol_ctx.initialized = true;
#ifdef EMMC_STATIC_TRANSFER_SIZE
    g_protocol_ctx.optimal_transfer_size = EMMC_STATIC_TRANSFER_SIZE;
#else
    g_protocol_ctx.optimal_transfer_size = 256; /* Default: 128KB */
#endif
    
    return EMMC_OK;
}

emmc_result_t emmc_protocol_deinit(void)
{
    emmc_result_t result;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    result = emmc_driver_deinit();
    g_protocol_ctx.initialized = false;
    
    return result;
}

bool emmc_protocol_is_initialized(void)
{
    return g_protocol_ctx.initialized;
}

emmc_result_t emmc_initialize(void)
{
    emmc_result_t result;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Initialize card */
    result = emmc_card_initialize();
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Optimize performance if enabled and not statically configured */
    if (g_protocol_ctx.config.auto_optimize && 
        (!EMMC_HAS_STATIC_BUS_MODE || !EMMC_HAS_STATIC_BUS_WIDTH)) {
        result = emmc_optimize_performance();
        if (result != EMMC_OK) {
            /* Continue even if optimization fails */
        }
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_read_sectors(u64 start_sector, u32 sector_count, u8 *buffer)
{
    emmc_block_request_t request = {
        .start_sector = start_sector,
        .sector_count = sector_count,
        .buffer = buffer,
        .read_operation = true,
        .reliable_write = false
    };
    
    return emmc_block_io(&request);
}

emmc_result_t emmc_write_sectors(u64 start_sector, u32 sector_count, const u8 *buffer)
{
    emmc_block_request_t request = {
        .start_sector = start_sector,
        .sector_count = sector_count,
        .buffer = (u8*)buffer,
        .read_operation = false,
        .reliable_write = false
    };
    
    return emmc_block_io(&request);
}

emmc_result_t emmc_block_io(const emmc_block_request_t *request)
{
    emmc_result_t result;
    u32 remaining_sectors;
    u32 transfer_sectors;
    u64 current_sector;
    u8 *current_buffer;
    u32 max_transfer;
    bool use_predefined_count = false;
    
    if (!g_protocol_ctx.initialized || !request || !request->buffer) {
        return EMMC_INVALID_PARAM;
    }
    
    if (request->sector_count == 0) {
        return EMMC_OK;
    }
    
    /* Check if card is ready */
    if (!emmc_is_card_ready()) {
        return EMMC_BUSY;
    }
    
max_transfer = (request->sector_count > 1) ? EMMC_MAX_MULTI_TRANSFER : EMMC_MAX_SINGLE_TRANSFER;
    
    if (EMMC_COMPILE_CMD23_SUPPORT) {
        /* For large transfers, use predefined block count (CMD23) for better performance */
        if (remaining_sectors > g_protocol_ctx.optimal_transfer_size / 4) {
            use_predefined_count = true;
        }
    }
    
    remaining_sectors = request->sector_count;
    current_sector = request->start_sector;
    current_buffer = request->buffer;
    
    while (remaining_sectors > 0) {
        /* Determine transfer size */
        transfer_sectors = (remaining_sectors > max_transfer) ? max_transfer : remaining_sectors;
        
        if (transfer_sectors > g_protocol_ctx.optimal_transfer_size) {
            transfer_sectors = g_protocol_ctx.optimal_transfer_size;
        }
        
        /* Perform transfer */
        if (transfer_sectors == 1) {
            /* Single block transfer */
            u8 cmd_index = request->read_operation ? EMMC_CMD17 : EMMC_CMD24;
            u32 lba = emmc_sector_to_lba(current_sector);
            
            result = emmc_send_command_with_data(cmd_index, lba, EMMC_RESP_R1,
                                               current_buffer, EMMC_SECTOR_SIZE, 
                                               1, request->read_operation, NULL);
        } else {
            /* Multi-block transfer */
            u8 cmd_index = request->read_operation ? EMMC_CMD18 : EMMC_CMD25;
            u32 lba = emmc_sector_to_lba(current_sector);
            
            if (EMMC_COMPILE_CMD23_SUPPORT) {
                u32 cmd23_arg = transfer_sectors;
                
                /* Set predefined block count (CMD23) for better performance and reliability */
                if (use_predefined_count || request->reliable_write) {
                    /* For reliable write, set bit 31 to indicate reliable write */
                    if (request->reliable_write) {
                        cmd23_arg |= (1U << 31);
                    }
                    
                    result = emmc_send_command(EMMC_CMD23, cmd23_arg, EMMC_RESP_R1, NULL);
                    if (result != EMMC_OK) {
                        return result;
                    }
                }
            }
            
            result = emmc_send_command_with_data(cmd_index, lba, EMMC_RESP_R1,
                                               current_buffer, EMMC_SECTOR_SIZE,
                                               transfer_sectors, request->read_operation, NULL);
            
            if (EMMC_COMPILE_CMD23_SUPPORT) {
                /* Send stop command only if CMD23 was not used (no predefined count) */
                if (result == EMMC_OK && !use_predefined_count && !request->reliable_write) {
                    u32 stop_response;
                    emmc_send_command(EMMC_CMD12, 0, EMMC_RESP_R1B, &stop_response);
                }
            } else {
                /* Always send stop command if CMD23 is not supported */
                if (result == EMMC_OK) {
                    u32 stop_response;
                    emmc_send_command(EMMC_CMD12, 0, EMMC_RESP_R1B, &stop_response);
                }
            }
        }
        
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Update counters */
        remaining_sectors -= transfer_sectors;
        current_sector += transfer_sectors;
        current_buffer += transfer_sectors * EMMC_SECTOR_SIZE;
    }
    
    return EMMC_OK;
}


emmc_result_t emmc_select_partition(emmc_partition_t partition)
{
    /* All partitions are supported */
    emmc_result_t result;
    const emmc_driver_context_t *ctx;
    u8 partition_config;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Get current partition configuration */
    partition_config = ctx->card_info.ext_csd.partition_config;
    
    /* Clear partition access bits and set new partition */
    partition_config &= ~EXT_CSD_PARTITION_ACCESS_MASK;
    partition_config |= (u8)partition;
    
    /* Switch partition */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_PARTITION_CONFIG, 
                             partition_config, 
                             EMMC_SWITCH_TIMEOUT_MS);
    
    return result;
}

emmc_partition_t emmc_get_active_partition(void)
{
    const emmc_driver_context_t *ctx;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_PART_USER;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_PART_USER;
    }
    
    return (emmc_partition_t)(ctx->card_info.ext_csd.partition_config & 
                             EXT_CSD_PARTITION_ACCESS_MASK);
}

emmc_result_t emmc_get_partition_size(emmc_partition_t partition, u64 *size_bytes)
{
    const emmc_driver_context_t *ctx;
    const emmc_ext_csd_t *ext_csd;
    
    if (!g_protocol_ctx.initialized || !size_bytes) {
        return EMMC_INVALID_PARAM;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_NOT_READY;
    }
    
    ext_csd = &ctx->card_info.ext_csd;
    
    switch (partition) {
        case EMMC_PART_USER:
            *size_bytes = ctx->card_info.capacity;
            break;
            
        case EMMC_PART_BOOT1:
        case EMMC_PART_BOOT2:
            *size_bytes = (u64)ext_csd->boot_mult * 128 * 1024; /* boot_mult * 128KB */
            break;
            
        case EMMC_PART_RPMB:
            *size_bytes = (u64)ext_csd->rpmb_size_mult * 128 * 1024; /* rpmb_size_mult * 128KB */
            break;
            
        case EMMC_PART_GP1:
        case EMMC_PART_GP2:
        case EMMC_PART_GP3:
        case EMMC_PART_GP4:
            /* General purpose partitions - would need more complex calculation */
            *size_bytes = 0;
            return EMMC_NOT_SUPPORTED;
            
        default:
            return EMMC_INVALID_PARAM;
    }
    
    return EMMC_OK;
}

static emmc_result_t emmc_optimize_performance(void)
{
    emmc_result_t result = EMMC_OK;
    const emmc_driver_context_t *ctx;
    const emmc_ext_csd_t *ext_csd;
    emmc_bus_width_t target_width;
    emmc_bus_mode_t target_mode;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.config.enable_advanced_features) {
        return EMMC_NOT_READY;
    }
    
    /* Static configuration - compiler will eliminate dead code */
    if (EMMC_HAS_STATIC_BUS_MODE && EMMC_HAS_STATIC_BUS_WIDTH) {
        /* Already statically configured - no optimization needed */
        return EMMC_OK;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_NOT_READY;
    }
    
    ext_csd = &ctx->card_info.ext_csd;
    
    /* Determine optimal bus width - only if not statically configured */
    if (!EMMC_HAS_STATIC_BUS_WIDTH) {
        if (ctx->hal_config.max_bus_width >= 8) {
            target_width = EMMC_BUS_WIDTH_8;
        } else if (ctx->hal_config.max_bus_width >= 4) {
            target_width = EMMC_BUS_WIDTH_4;
        } else {
            target_width = EMMC_BUS_WIDTH_1;
        }
    } else {
        target_width = (emmc_bus_width_t)EMMC_STATIC_BUS_WIDTH;
    }
    
    /* Determine optimal timing mode - only if not statically configured */
    if (!EMMC_HAS_STATIC_BUS_MODE) {
        target_mode = EMMC_MODE_SDR; /* Start with legacy */
        
        /* Check for Enhanced Strobe support (highest priority) */
        if ((ext_csd->card_type & 0x04) && (ext_csd->strobe_support & 0x01)) {
            target_mode = EMMC_MODE_HS400_ES;
        } else if (ext_csd->card_type & 0x04) {
            target_mode = EMMC_MODE_HS400;
        } else if (ext_csd->card_type & 0x02) {
            target_mode = EMMC_MODE_SDR;
        }
    } else {
        target_mode = (emmc_bus_mode_t)EMMC_STATIC_BUS_MODE;
    }
    
    /* Set bus configuration - only if dynamic negotiation needed */
    if (!EMMC_HAS_STATIC_BUS_MODE || !EMMC_HAS_STATIC_BUS_WIDTH) {
        result = emmc_set_bus_config(target_width, target_mode);
        
        /* Fallback logic - only for dynamic configuration */
        if (result != EMMC_OK && !EMMC_HAS_STATIC_BUS_MODE) {
            if (target_mode == EMMC_MODE_HS400_ES) {
                result = emmc_set_bus_config(target_width, EMMC_MODE_HS400);
            }
            if (result != EMMC_OK && (target_mode == EMMC_MODE_HS400 || target_mode == EMMC_MODE_HS400_ES)) {
                result = emmc_set_bus_config(target_width, EMMC_MODE_HS200);
            }
            if (result != EMMC_OK) {
                result = emmc_set_bus_config(target_width, EMMC_MODE_SDR);
            }
        }
        
        if (result != EMMC_OK && !EMMC_HAS_STATIC_BUS_WIDTH && target_width == EMMC_BUS_WIDTH_8) {
            result = emmc_set_bus_config(EMMC_BUS_WIDTH_4, target_mode);
        }
        
        if (result != EMMC_OK && !EMMC_HAS_STATIC_BUS_MODE && !EMMC_HAS_STATIC_BUS_WIDTH) {
            result = emmc_set_bus_config(EMMC_BUS_WIDTH_1, EMMC_MODE_SDR);
        }
    }
    
    /* Calculate optimal transfer size - only if not statically configured */
    if (!__builtin_constant_p(g_protocol_ctx.optimal_transfer_size) || g_protocol_ctx.optimal_transfer_size == 256) {
        switch (target_width) {
            case EMMC_BUS_WIDTH_8:
                g_protocol_ctx.optimal_transfer_size = 1024; /* 512KB */
                break;
            case EMMC_BUS_WIDTH_4:
                g_protocol_ctx.optimal_transfer_size = 512; /* 256KB */
                break;
            default:
                g_protocol_ctx.optimal_transfer_size = 256; /* 128KB */
                break;
        }
    }
    
    return result;
}

emmc_result_t emmc_set_bus_config(emmc_bus_width_t width, emmc_bus_mode_t mode)
{
    emmc_result_t result;
    u8 bus_width_value;
    u8 timing_value;
    u32 target_freq;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Map bus width to EXT_CSD value */
    switch (width) {
        case EMMC_BUS_WIDTH_1:
            bus_width_value = EXT_CSD_BUS_WIDTH_1;
            break;
        case EMMC_BUS_WIDTH_4:
            bus_width_value = EXT_CSD_BUS_WIDTH_4;
            break;
        case EMMC_BUS_WIDTH_8:
            bus_width_value = EXT_CSD_BUS_WIDTH_8;
            break;
        default:
            return EMMC_INVALID_PARAM;
    }
    
    /* Map timing mode to EXT_CSD value and target frequency */
    switch (mode) {
        case EMMC_MODE_SDR:
            timing_value = EXT_CSD_TIMING_HS;
            target_freq = HAL_EMMC_HS_CLOCK_FREQ;
            break;
        case EMMC_MODE_HS200:
            timing_value = EXT_CSD_TIMING_HS200;
            target_freq = HAL_EMMC_HS200_CLOCK_FREQ;
            break;
        case EMMC_MODE_HS400:
            timing_value = EXT_CSD_TIMING_HS400;
            target_freq = HAL_EMMC_HS400_CLOCK_FREQ;
            bus_width_value = EXT_CSD_DDR_BUS_WIDTH_8; /* HS400 requires 8-bit DDR */
            break;
        case EMMC_MODE_HS400_ES:
            timing_value = EXT_CSD_TIMING_HS400;
            target_freq = HAL_EMMC_HS400_CLOCK_FREQ;
            bus_width_value = EXT_CSD_DDR_BUS_WIDTH_8; /* HS400ES requires 8-bit DDR */
            break;
        default:
            return EMMC_INVALID_PARAM;
    }
    
    /* Set timing mode first */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_HS_TIMING, 
                             timing_value, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set bus width */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_BUS_WIDTH, 
                             bus_width_value, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Configure HAL for new bus settings */
    result = hal_emmc_set_bus_width(width);
    if (result != EMMC_OK) {
        return result;
    }
    
    result = hal_emmc_set_timing(mode);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set new clock frequency */
    hal_emmc_set_clock(target_freq);
    
    /* Enable Enhanced Strobe if HS400ES mode */
    if (mode == EMMC_MODE_HS400_ES) {
        /* Enable Enhanced Strobe in EXT_CSD */
        result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE,
                                 EXT_CSD_STROBE_SUPPORT,
                                 EXT_CSD_ENHANCED_STROBE,
                                 EMMC_SWITCH_TIMEOUT_MS);
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Configure HAL for Enhanced Strobe */
        result = hal_emmc_set_timing(mode);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_get_bus_config(emmc_bus_width_t *width, emmc_bus_mode_t *mode, u32 *frequency)
{
    const emmc_driver_context_t *ctx;
    
    if (!g_protocol_ctx.initialized || !width || !mode || !frequency) {
        return EMMC_INVALID_PARAM;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_NOT_READY;
    }
    
    *width = ctx->card_info.bus_width;
    *mode = ctx->card_info.bus_mode;
    *frequency = ctx->card_info.clock_freq;
    
    return EMMC_OK;
}

const emmc_card_info_t* emmc_get_card_info(void)
{
    const emmc_driver_context_t *ctx;
    
    if (!g_protocol_ctx.initialized) {
        return NULL;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return NULL;
    }
    
    return &ctx->card_info;
}


u32 emmc_get_optimal_transfer_size(void)
{
    return g_protocol_ctx.optimal_transfer_size;
}