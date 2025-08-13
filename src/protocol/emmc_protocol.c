#include "../include/emmc_config_presets.h"  /* Include static configuration first */
#include "emmc_protocol.h"
#include <string.h>

/* Protocol layer context */
static struct {
    emmc_protocol_config_t config;
    emmc_rpmb_crypto_interface_t rpmb_crypto;
    bool initialized;
    bool rpmb_initialized;
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
    g_protocol_ctx.rpmb_initialized = false;
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
    g_protocol_ctx.rpmb_initialized = false;
    
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
    
    /* Optimize performance if enabled */
    if (g_protocol_ctx.config.auto_optimize) {
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
    
#if EMMC_COMPILE_SINGLE_BLOCK_ONLY
    max_transfer = 1; /* Force single block transfers */
    use_predefined_count = false;
#else
    max_transfer = (request->sector_count > 1) ? EMMC_MAX_MULTI_TRANSFER : EMMC_MAX_SINGLE_TRANSFER;
    
    #if EMMC_COMPILE_CMD23_SUPPORT
    /* For large transfers, use predefined block count (CMD23) for better performance */
    if (remaining_sectors > g_protocol_ctx.optimal_transfer_size / 4) {
        use_predefined_count = true;
    }
    #endif
#endif
    
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
#if EMMC_COMPILE_SINGLE_BLOCK_ONLY
        /* Force single block transfer only */
        {
            u8 cmd_index = request->read_operation ? EMMC_CMD17 : EMMC_CMD24;
            u32 lba = emmc_sector_to_lba(current_sector);
            
            result = emmc_send_command_with_data(cmd_index, lba, EMMC_RESP_R1,
                                               current_buffer, EMMC_SECTOR_SIZE, 
                                               1, request->read_operation, NULL);
        }
#else
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
            
#if EMMC_COMPILE_CMD23_SUPPORT
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
#endif
            
            result = emmc_send_command_with_data(cmd_index, lba, EMMC_RESP_R1,
                                               current_buffer, EMMC_SECTOR_SIZE,
                                               transfer_sectors, request->read_operation, NULL);
            
#if EMMC_COMPILE_CMD23_SUPPORT
            /* Send stop command only if CMD23 was not used (no predefined count) */
            if (result == EMMC_OK && !use_predefined_count && !request->reliable_write) {
                u32 stop_response;
                emmc_send_command(EMMC_CMD12, 0, EMMC_RESP_R1B, &stop_response);
            }
#else
            /* Always send stop command if CMD23 is not supported */
            if (result == EMMC_OK) {
                u32 stop_response;
                emmc_send_command(EMMC_CMD12, 0, EMMC_RESP_R1B, &stop_response);
            }
#endif
        }
#endif
        
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Update counters */
#if EMMC_COMPILE_SINGLE_BLOCK_ONLY
        remaining_sectors -= 1;
        current_sector += 1;
        current_buffer += EMMC_SECTOR_SIZE;
#else
        remaining_sectors -= transfer_sectors;
        current_sector += transfer_sectors;
        current_buffer += transfer_sectors * EMMC_SECTOR_SIZE;
#endif
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_erase_sectors(u64 start_sector, u32 sector_count)
{
#if EMMC_COMPILE_ERASE_CMD
    emmc_result_t result;
    u32 start_lba, end_lba;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    if (sector_count == 0) {
        return EMMC_OK;
    }
    
    start_lba = emmc_sector_to_lba(start_sector);
    end_lba = emmc_sector_to_lba(start_sector + sector_count - 1);
    
    /* Set erase start address */
    result = emmc_send_command(EMMC_CMD35, start_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set erase end address */
    result = emmc_send_command(EMMC_CMD36, end_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Execute erase */
    result = emmc_send_command(EMMC_CMD38, 0, EMMC_RESP_R1B, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for erase completion */
    return emmc_wait_for_state(EMMC_STATE_TRAN, 30000); /* 30 second timeout */
#else
    /* Erase command disabled in static configuration */
    (void)start_sector;
    (void)sector_count;
    return EMMC_NOT_SUPPORTED;
#endif
}

emmc_result_t emmc_select_partition(emmc_partition_t partition)
{
    /* Check if partition switching is enabled */
    if (partition == EMMC_PART_BOOT1 && !EMMC_COMPILE_BOOT_PARTITION) {
        return EMMC_NOT_SUPPORTED;
    }
    if (partition == EMMC_PART_BOOT2 && !EMMC_COMPILE_BOOT_PARTITION) {
        return EMMC_NOT_SUPPORTED;
    }
    if (partition == EMMC_PART_RPMB && !EMMC_COMPILE_RPMB) {
        return EMMC_NOT_SUPPORTED;
    }
    if ((partition >= EMMC_PART_GP1 && partition <= EMMC_PART_GP4) && !EMMC_COMPILE_GP_PARTITION) {
        return EMMC_NOT_SUPPORTED;
    }

#ifdef EMMC_USER_PARTITION_ONLY
    /* User partition only mode - reject all other partitions */
    if (partition != EMMC_PART_USER) {
        return EMMC_NOT_SUPPORTED;
    }
    return EMMC_OK;  /* No actual switching needed */
#else
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
#endif
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

emmc_result_t emmc_optimize_performance(void)
{
    emmc_result_t result;
    const emmc_driver_context_t *ctx;
    const emmc_ext_csd_t *ext_csd;
    emmc_bus_width_t target_width;
    emmc_bus_mode_t target_mode;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.config.enable_advanced_features) {
        return EMMC_NOT_READY;
    }
    
    ctx = emmc_driver_get_context();
    if (!ctx || !ctx->card_info.initialized) {
        return EMMC_NOT_READY;
    }
    
    ext_csd = &ctx->card_info.ext_csd;
    
    /* Determine optimal bus width */
    if (ctx->hal_config.max_bus_width >= 8) {
        target_width = EMMC_BUS_WIDTH_8;
    } else if (ctx->hal_config.max_bus_width >= 4) {
        target_width = EMMC_BUS_WIDTH_4;
    } else {
        target_width = EMMC_BUS_WIDTH_1;
    }
    
    /* Determine optimal timing mode based on card capabilities */
    target_mode = EMMC_MODE_SDR; /* Start with legacy */
    
    /* Check for Enhanced Strobe support (highest priority) */
    if ((ext_csd->card_type & 0x04) && (ext_csd->strobe_support & 0x01)) {
        /* Supports HS400 Enhanced Strobe - best performance */
        target_mode = EMMC_MODE_HS400_ES;
    } else if (ext_csd->card_type & 0x04) {
        /* Supports HS200/HS400 */
        target_mode = EMMC_MODE_HS400;
    } else if (ext_csd->card_type & 0x02) {
        /* Supports 52MHz */
        target_mode = EMMC_MODE_SDR;
    }
    
    /* Set bus configuration */
    result = emmc_set_bus_config(target_width, target_mode);
    if (result != EMMC_OK) {
        /* Try fallback configurations in order of preference */
        if (target_mode == EMMC_MODE_HS400_ES) {
            result = emmc_set_bus_config(target_width, EMMC_MODE_HS400);
        }
        if (result != EMMC_OK && (target_mode == EMMC_MODE_HS400 || target_mode == EMMC_MODE_HS400_ES)) {
            result = emmc_set_bus_config(target_width, EMMC_MODE_HS200);
        }
        if (result != EMMC_OK && target_mode == EMMC_MODE_HS200) {
            result = emmc_set_bus_config(target_width, EMMC_MODE_SDR);
        }
        
        if (result != EMMC_OK && target_width == EMMC_BUS_WIDTH_8) {
            result = emmc_set_bus_config(EMMC_BUS_WIDTH_4, target_mode);
        }
        
        if (result != EMMC_OK) {
            result = emmc_set_bus_config(EMMC_BUS_WIDTH_1, EMMC_MODE_SDR);
        }
    }
    
    /* Enable cache if supported */
    if (result == EMMC_OK && (ext_csd->cache_flush_policy & 0x01)) {
        /* Cache function is in emmc_advanced.c - will be linked if used */
        extern emmc_result_t emmc_set_cache_enable(bool enable);
        emmc_set_cache_enable(true);
    }
    
    /* Calculate optimal transfer size based on bus configuration */
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

/* Advanced cache functions moved to emmc_advanced.c */

u32 emmc_get_optimal_transfer_size(void)
{
    return g_protocol_ctx.optimal_transfer_size;
}

/* RPMB Functions */
emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface)
{
    if (!g_protocol_ctx.initialized || !crypto_interface) {
        return EMMC_INVALID_PARAM;
    }
    
    if (!crypto_interface->get_key || !crypto_interface->generate_nonce ||
        !crypto_interface->compute_hmac || !crypto_interface->verify_hmac) {
        return EMMC_INVALID_PARAM;
    }
    
    g_protocol_ctx.rpmb_crypto = *crypto_interface;
    g_protocol_ctx.rpmb_initialized = true;
    
    return EMMC_OK;
}

emmc_result_t emmc_rpmb_program_key(const u8 *key)
{
    emmc_rpmb_frame_t frame = {0};
    emmc_result_t result;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.rpmb_initialized || !key) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition */
    result = emmc_select_partition(EMMC_PART_RPMB);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare RPMB frame for key programming */
    frame.req_resp = EMMC_RPMB_WRITE_KEY;
    
    /* Copy key to frame */
    for (int i = 0; i < EMMC_RPMB_KEY_SIZE; i++) {
        frame.key_mac[i] = key[i];
    }
    
    /* Key will be programmed directly to the eMMC, no injection needed */
    
    /* Send RPMB write key command */
    result = emmc_send_command_with_data(EMMC_CMD25, 0, EMMC_RESP_R1,
                                        (u8*)&frame, sizeof(emmc_rpmb_frame_t),
                                        1, false, NULL);
    
    return result;
}

emmc_result_t emmc_rpmb_get_write_counter(u32 *counter)
{
    emmc_rpmb_frame_t request_frame = {0};
    emmc_rpmb_frame_t response_frame = {0};
    emmc_result_t result;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.rpmb_initialized || !counter) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition */
    result = emmc_select_partition(EMMC_PART_RPMB);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare request frame */
    request_frame.req_resp = EMMC_RPMB_READ_WCOUNTER;
    
    /* Send write counter read request */
    result = emmc_send_command_with_data(EMMC_CMD25, 0, EMMC_RESP_R1,
                                        (u8*)&request_frame, sizeof(emmc_rpmb_frame_t),
                                        1, false, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Read response */
    result = emmc_send_command_with_data(EMMC_CMD18, 0, EMMC_RESP_R1,
                                        (u8*)&response_frame, sizeof(emmc_rpmb_frame_t),
                                        1, true, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Extract write counter */
    *counter = response_frame.write_counter;
    
    return (response_frame.result == EMMC_RPMB_RESULT_OK) ? EMMC_OK : EMMC_ERROR;
}

/* RPMB Helper Functions */

/**
 * @brief Prepare RPMB frame with common fields
 */
static void emmc_rpmb_prepare_frame(emmc_rpmb_frame_t *frame, 
                                   u16 req_resp, u16 address, 
                                   u16 block_count, const u8 *nonce)
{
    if (!frame) return;
    
    /* Clear frame */
    memset(frame, 0, sizeof(emmc_rpmb_frame_t));
    
    /* Set common fields */
    frame->req_resp = req_resp;
    frame->address = address;
    frame->block_count = block_count;
    
    /* Copy nonce if provided */
    if (nonce) {
        memcpy(frame->nonce, nonce, EMMC_RPMB_NONCE_SIZE);
    }
}

/**
 * @brief Send RPMB request and read response
 */
static emmc_result_t emmc_rpmb_send_request(const emmc_rpmb_frame_t *req_frame,
                                           emmc_rpmb_frame_t *resp_frame,
                                           u32 frame_count)
{
    emmc_result_t result;
    
    if (!req_frame || (!resp_frame && frame_count > 0)) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition if not already there */
    if (emmc_get_active_partition() != EMMC_PART_RPMB) {
        result = emmc_select_partition(EMMC_PART_RPMB);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Set block count for reliable write */
    if (frame_count > 0) {
        u32 reliable_write_flag = (1U << 31);
        result = emmc_send_command(EMMC_CMD23, frame_count | reliable_write_flag, 
                                  EMMC_RESP_R1, NULL);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Send request frame */
    result = emmc_send_command_with_data(EMMC_CMD25, 0, EMMC_RESP_R1,
                                        (u8*)req_frame, sizeof(emmc_rpmb_frame_t),
                                        1, false, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Read response if expected */
    if (resp_frame && frame_count > 0) {
        /* Set block count for response read */
        result = emmc_send_command(EMMC_CMD23, frame_count, EMMC_RESP_R1, NULL);
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Read response frames */
        result = emmc_send_command_with_data(EMMC_CMD18, 0, EMMC_RESP_R1,
                                            (u8*)resp_frame, sizeof(emmc_rpmb_frame_t),
                                            frame_count, true, NULL);
    }
    
    return result;
}

/**
 * @brief Verify write counter increment
 */
static emmc_result_t emmc_rpmb_verify_write_counter(u32 expected_counter, u32 actual_counter)
{
    /* Write counter should increment by 1 */
    if (actual_counter != expected_counter + 1) {
        return EMMC_ERROR;
    }
    
    return EMMC_OK;
}

/**
 * @brief Generate random nonce (platform implementation needed)
 */
static void emmc_rpmb_generate_nonce(u8 *nonce)
{
    /* Platform-specific random number generation */
    /* This is a simple implementation - real systems should use hardware RNG */
    static u32 nonce_counter = 0x12345678;
    
    for (int i = 0; i < EMMC_RPMB_NONCE_SIZE; i += 4) {
        nonce_counter = nonce_counter * 1103515245 + 12345; /* Simple PRNG */
        nonce[i] = (u8)(nonce_counter >> 24);
        nonce[i+1] = (u8)(nonce_counter >> 16);
        nonce[i+2] = (u8)(nonce_counter >> 8);
        nonce[i+3] = (u8)(nonce_counter);
    }
}

emmc_result_t emmc_rpmb_write_data(u16 address, const u8 *data, u16 block_count, const u8 *key)
{
    emmc_result_t result;
    u8 current_key[EMMC_RPMB_KEY_SIZE];
    const u8 *auth_key = key;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.rpmb_initialized || 
        !data || block_count == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Check RPMB block count limits */
    if (block_count > EMMC_RPMB_MAX_BLOCKS) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Get key from crypto interface if not provided */
    if (!key) {
        if (!g_protocol_ctx.rpmb_crypto.get_key) {
            return EMMC_INVALID_PARAM;
        }
        result = g_protocol_ctx.rpmb_crypto.get_key(current_key, EMMC_RPMB_KEY_SIZE);
        if (result != EMMC_OK) {
            return result;
        }
        auth_key = current_key;
    }
    
    /* RPMB authenticated writes must be done one block at a time */
    /* Each write increments the write counter, so we do sequential writes */
    for (u16 i = 0; i < block_count; i++) {
        result = emmc_rpmb_write_single_block(address + i, 
                                            data + i * EMMC_RPMB_BLOCK_SIZE, 
                                            auth_key);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_rpmb_read_data(u16 address, u8 *data, u16 block_count, const u8 *key)
{
    emmc_result_t result;
    emmc_rpmb_frame_t read_frame = {0};
    emmc_rpmb_frame_t *response_frames;
    u8 nonce[EMMC_RPMB_NONCE_SIZE];
    u8 current_key[EMMC_RPMB_KEY_SIZE];
    const u8 *auth_key = key;
    
    if (!g_protocol_ctx.initialized || !g_protocol_ctx.rpmb_initialized || 
        !data || block_count == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Check RPMB block count limits */
    if (block_count > EMMC_RPMB_MAX_BLOCKS) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Get key from crypto interface if not provided */
    if (!key) {
        if (!g_protocol_ctx.rpmb_crypto.get_key) {
            return EMMC_INVALID_PARAM;
        }
        result = g_protocol_ctx.rpmb_crypto.get_key(current_key, EMMC_RPMB_KEY_SIZE);
        if (result != EMMC_OK) {
            return result;
        }
        auth_key = current_key;
    }
    
    /* Allocate response frames buffer (static allocation for bare-metal) */
    static emmc_rpmb_frame_t static_response_frames[EMMC_RPMB_MAX_BLOCKS];
    response_frames = static_response_frames;
    
    if (block_count > EMMC_RPMB_MAX_BLOCKS) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Generate secure nonce using crypto interface */
    result = g_protocol_ctx.rpmb_crypto.generate_nonce(nonce, EMMC_RPMB_NONCE_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare read request frame */
    emmc_rpmb_prepare_frame(&read_frame, EMMC_RPMB_READ_DATA, address, block_count, nonce);
    
    /* Send read request and receive multiple frames */
    result = emmc_rpmb_send_request(&read_frame, response_frames, block_count);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Process each response frame */
    for (u16 i = 0; i < block_count; i++) {
        emmc_rpmb_frame_t *frame = &response_frames[i];
        
        /* Check result */
        if (frame->result != EMMC_RPMB_RESULT_OK) {
            return EMMC_ERROR;
        }
        
        /* Verify nonce matches (only for first frame) */
        if (i == 0 && memcmp(frame->nonce, nonce, EMMC_RPMB_NONCE_SIZE) != 0) {
            return EMMC_ERROR; /* Nonce mismatch - possible replay attack */
        }
        
        /* Verify MAC for each frame */
        result = emmc_rpmb_verify_frame_mac(frame, auth_key);
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Copy data to output buffer */
        memcpy(data + i * EMMC_RPMB_BLOCK_SIZE, frame->data, EMMC_RPMB_BLOCK_SIZE);
    }
    
    return EMMC_OK;
}

/* RPMB Helper Functions for Multi-Frame Support */

/**
 * @brief Verify MAC for a single RPMB frame
 */
static emmc_result_t emmc_rpmb_verify_frame_mac(const emmc_rpmb_frame_t *frame, const u8 *key)
{
    if (!frame || !key) {
        return EMMC_INVALID_PARAM;
    }
    
    /* HMAC input: data || nonce || write_counter || address || block_count || result || req_resp */
    u8 hmac_data[256 + 16 + 4 + 2 + 2 + 2 + 2]; /* Total: 284 bytes */
    u32 hmac_offset = 0;
    
    /* Build HMAC input for verification */
    memcpy(hmac_data + hmac_offset, frame->data, 256);
    hmac_offset += 256;
    memcpy(hmac_data + hmac_offset, frame->nonce, 16);
    hmac_offset += 16;
    memcpy(hmac_data + hmac_offset, &frame->write_counter, 4);
    hmac_offset += 4;
    memcpy(hmac_data + hmac_offset, &frame->address, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &frame->block_count, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &frame->result, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &frame->req_resp, 2);
    hmac_offset += 2;
    
    return g_protocol_ctx.rpmb_crypto.verify_hmac(key, EMMC_RPMB_KEY_SIZE,
                                                  hmac_data, hmac_offset,
                                                  frame->key_mac, EMMC_RPMB_MAC_SIZE);
}

/**
 * @brief Write a single RPMB block (for multi-block write operations)
 */
static emmc_result_t emmc_rpmb_write_single_block(u16 address, const u8 *data, const u8 *key)
{
    emmc_result_t result;
    emmc_rpmb_frame_t write_frame = {0};
    emmc_rpmb_frame_t result_frame = {0};
    u32 write_counter;
    u8 mac[EMMC_RPMB_MAC_SIZE];
    
    /* Get current write counter */
    result = emmc_rpmb_get_write_counter(&write_counter);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare write frame */
    emmc_rpmb_prepare_frame(&write_frame, EMMC_RPMB_WRITE_DATA, address, 1, NULL);
    write_frame.write_counter = write_counter;
    
    /* Copy data to frame */
    memcpy(write_frame.data, data, EMMC_RPMB_BLOCK_SIZE);
    
    /* Compute HMAC-SHA256 */
    u8 hmac_data[256 + 16 + 4 + 2 + 2 + 2 + 2]; /* Total: 284 bytes */
    u32 hmac_offset = 0;
    
    /* Build HMAC input */
    memcpy(hmac_data + hmac_offset, write_frame.data, 256);
    hmac_offset += 256;
    memcpy(hmac_data + hmac_offset, write_frame.nonce, 16);
    hmac_offset += 16;
    memcpy(hmac_data + hmac_offset, &write_frame.write_counter, 4);
    hmac_offset += 4;
    memcpy(hmac_data + hmac_offset, &write_frame.address, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &write_frame.block_count, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &write_frame.result, 2);
    hmac_offset += 2;
    memcpy(hmac_data + hmac_offset, &write_frame.req_resp, 2);
    hmac_offset += 2;
    
    result = g_protocol_ctx.rpmb_crypto.compute_hmac(key, EMMC_RPMB_KEY_SIZE,
                                                     hmac_data, hmac_offset,
                                                     mac, EMMC_RPMB_MAC_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Copy MAC to frame */
    memcpy(write_frame.key_mac, mac, EMMC_RPMB_MAC_SIZE);
    
    /* Send authenticated write request */
    result = emmc_rpmb_send_request(&write_frame, &result_frame, 1);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Check result */
    if (result_frame.result != EMMC_RPMB_RESULT_OK) {
        return EMMC_ERROR;
    }
    
    /* Verify write counter incremented */
    return emmc_rpmb_verify_write_counter(write_counter, result_frame.write_counter);
}

/* New optimized multi-frame functions */

emmc_result_t emmc_rpmb_read_multi(u16 address, u8 *data, u16 block_count)
{
    /* Use read_data with NULL key to use stored key */
    return emmc_rpmb_read_data(address, data, block_count, NULL);
}

emmc_result_t emmc_rpmb_write_multi(u16 address, const u8 *data, u16 block_count)
{
    /* Use write_data with NULL key to use stored key */
    return emmc_rpmb_write_data(address, data, block_count, NULL);
}