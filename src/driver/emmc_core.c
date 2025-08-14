#include "../include/emmc_config_presets.h"  /* Include static configuration first */
#include "emmc_core.h"

/* Driver context (global state) */
static emmc_driver_context_t g_emmc_ctx = {0};

emmc_result_t emmc_driver_init(const emmc_driver_config_t *config)
{
    emmc_result_t result;
    
    if (!config) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Initialize driver context */
    g_emmc_ctx.driver_config = *config;
    g_emmc_ctx.hal_config = config->hal_config;
    g_emmc_ctx.initialized = false;
    g_emmc_ctx.current_clock = 0;
    g_emmc_ctx.active_part = EMMC_PART_USER;
    
    /* Initialize HAL */
    result = hal_emmc_init(&config->hal_config);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Reset controller */
    result = hal_emmc_reset(EMMC_RESET_ALL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set initial power and clock */
    result = hal_emmc_set_power(true, HAL_EMMC_VOLTAGE_33V);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set initial clock frequency */
    g_emmc_ctx.current_clock = hal_emmc_set_clock(HAL_EMMC_INIT_CLOCK_FREQ);
    if (g_emmc_ctx.current_clock == 0) {
        return EMMC_ERROR;
    }
    
    /* Enable clock */
    result = hal_emmc_clock_enable(true);
    if (result != EMMC_OK) {
        return result;
    }
    
    g_emmc_ctx.initialized = true;
    return EMMC_OK;
}

emmc_result_t emmc_driver_deinit(void)
{
    /* Disable clock */
    hal_emmc_clock_enable(false);
    
    /* Power down */
    hal_emmc_set_power(false, 0);
    
    /* Reset context */
    g_emmc_ctx.initialized = false;
    
    return EMMC_OK;
}

const emmc_driver_context_t* emmc_driver_get_context(void)
{
    return &g_emmc_ctx;
}

emmc_result_t emmc_send_command(u8 cmd_index, u32 argument, 
                               emmc_resp_type_t resp_type, u32 *response)
{
    hal_emmc_cmd_t cmd = {0};
    emmc_result_t result;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Wait for controller to be ready */
    if (!hal_emmc_is_command_ready()) {
        result = hal_emmc_wait_for_bit_clear(EMMC_PRNTS_STATE, 
                                            EMMC_STATE_CMD_INHIBIT, 
                                            EMMC_CMD_TIMEOUT_MS);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Setup command structure */
    cmd.index = cmd_index;
    cmd.argument = argument;
    cmd.resp_type = resp_type;
    cmd.data_present = false;
    cmd.crc_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R3);
    cmd.index_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R2);
    
    /* Send command */
    result = hal_emmc_send_command(&cmd, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get response if requested */
    if (response && resp_type != EMMC_RESP_NONE) {
        result = hal_emmc_get_response(response, resp_type);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_send_command_with_data(u8 cmd_index, u32 argument,
                                         emmc_resp_type_t resp_type,
                                         u8 *buffer, u32 block_size, 
                                         u32 block_count, bool read_operation,
                                         u32 *response)
{
    hal_emmc_cmd_t cmd = {0};
    hal_emmc_data_t data = {0};
    emmc_result_t result;
    
    if (!g_emmc_ctx.initialized || !buffer) {
        return EMMC_INVALID_PARAM;
    }
    
    if (block_count > EMMC_MAX_BLOCK_COUNT) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Wait for controller to be ready */
    if (!hal_emmc_is_command_ready() || hal_emmc_is_data_active()) {
        result = hal_emmc_wait_for_bit_clear(EMMC_PRNTS_STATE, 
                                            EMMC_STATE_CMD_INHIBIT | EMMC_STATE_DATA_INHIBIT,
                                            EMMC_CMD_TIMEOUT_MS);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Setup command structure */
    cmd.index = cmd_index;
    cmd.argument = argument;
    cmd.resp_type = resp_type;
    cmd.data_present = true;
    cmd.crc_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R3);
    cmd.index_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R2);
    
    /* Setup data structure */
    data.buffer = buffer;
    data.block_size = block_size;
    data.block_count = block_count;
    data.read_operation = read_operation;
    data.use_dma = g_emmc_ctx.hal_config.dma_enabled;
    
    /* Send command with data */
    result = hal_emmc_send_command(&cmd, &data);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for data transfer completion */
    result = hal_emmc_wait_data_complete(EMMC_DATA_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get response if requested */
    if (response && resp_type != EMMC_RESP_NONE) {
        result = hal_emmc_get_response(response, resp_type);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_get_card_status(u32 *status)
{
    if (!status || !g_emmc_ctx.initialized) {
        return EMMC_INVALID_PARAM;
    }
    
    return emmc_send_command(EMMC_CMD13, 
                            (u32)g_emmc_ctx.card_info.rca << 16,
                            EMMC_RESP_R1, status);
}

bool emmc_is_card_ready(void)
{
    u32 status;
    emmc_result_t result;
    
    result = emmc_get_card_status(&status);
    if (result != EMMC_OK) {
        return false;
    }
    
    return (status & EMMC_R1_READY_FOR_DATA) != 0;
}

emmc_result_t emmc_card_initialize(void)
{
    emmc_result_t result;
    u32 response[4];
    u32 ocr = 0;
    u16 rca = 1;
    int retry_count;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Reset card to idle state */
    result = emmc_send_command(EMMC_CMD0, 0, EMMC_RESP_NONE, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    hal_emmc_delay_ms(10);
    
    /* Send operating conditions command with retry */
    for (retry_count = 0; retry_count < 100; retry_count++) {
        /* Check operating conditions */
        result = emmc_send_command(EMMC_CMD1, EMMC_OCR_VDD_33_34 | EMMC_OCR_SECTOR_MODE,
                                  EMMC_RESP_R3, response);
        if (result != EMMC_OK) {
            return result;
        }
        
        ocr = response[0];
        
        /* Check if card is ready */
        if (ocr & EMMC_OCR_CARD_POWER_UP) {
            break;
        }
        
        hal_emmc_delay_ms(10);
    }
    
    if (!(ocr & EMMC_OCR_CARD_POWER_UP)) {
        return EMMC_TIMEOUT;
    }
    
    g_emmc_ctx.card_info.ocr = ocr;
    
    /* Get CID */
#if EMMC_COMPILE_CID_PARSING
    result = emmc_send_command(EMMC_CMD2, 0, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    result = emmc_parse_cid(response, &g_emmc_ctx.card_info.cid);
    if (result != EMMC_OK) {
        return result;
    }
#else
    /* Skip CID parsing in minimal configuration */
    result = emmc_send_command(EMMC_CMD2, 0, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    /* Set minimal CID information */
    g_emmc_ctx.card_info.cid.manufacturer_id = 0x00;
    g_emmc_ctx.card_info.cid.product_serial = 0x00000000;
#endif
    
    /* Set RCA */
    result = emmc_send_command(EMMC_CMD3, (u32)rca << 16, EMMC_RESP_R1, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    g_emmc_ctx.card_info.rca = rca;
    
    /* Get CSD */
#if EMMC_COMPILE_CSD_PARSING
    result = emmc_send_command(EMMC_CMD9, (u32)rca << 16, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    result = emmc_parse_csd(response, &g_emmc_ctx.card_info.csd);
    if (result != EMMC_OK) {
        return result;
    }
#else
    /* Skip CSD parsing in minimal configuration */
    result = emmc_send_command(EMMC_CMD9, (u32)rca << 16, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    /* Set minimal CSD information */
    g_emmc_ctx.card_info.csd.sector_size = EMMC_STATIC_SECTOR_SIZE;
    g_emmc_ctx.card_info.csd.capacity_sectors = 0; /* Will be set from EXT_CSD or defaults */
#endif
    
    /* Select card */
    result = emmc_send_command(EMMC_CMD7, (u32)rca << 16, EMMC_RESP_R1B, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for card to be in transfer state */
    result = emmc_wait_for_state(EMMC_STATE_TRAN, 1000);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Read EXT_CSD */
#if EMMC_COMPILE_EXT_CSD_PARSING
    u8 ext_csd_buffer[512];
    result = emmc_send_command_with_data(EMMC_CMD8, 0, EMMC_RESP_R1,
                                        ext_csd_buffer, 512, 1, true, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    result = emmc_parse_ext_csd(ext_csd_buffer, &g_emmc_ctx.card_info.ext_csd);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Calculate capacity */
    g_emmc_ctx.card_info.capacity = emmc_calculate_capacity(&g_emmc_ctx.card_info.csd,
                                                           &g_emmc_ctx.card_info.ext_csd);
#else
    /* Skip EXT_CSD parsing - use static defaults */
    /* Initialize EXT_CSD with default values */
    memset(&g_emmc_ctx.card_info.ext_csd, 0, sizeof(emmc_ext_csd_t));
    
    /* Set static configuration values */
    #ifdef EMMC_STATIC_SECTOR_COUNT
        g_emmc_ctx.card_info.capacity = EMMC_STATIC_SECTOR_COUNT * EMMC_STATIC_SECTOR_SIZE;
    #else
        g_emmc_ctx.card_info.capacity = 8ULL * 1024 * 1024 * 1024; /* Default: 8GB */
    #endif
    
    #ifdef EMMC_STATIC_BUS_MODE
        /* Set static bus mode support */
        #if (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400_ES)
            g_emmc_ctx.card_info.ext_csd.card_type = 0x07; /* HS200 + HS400 + Enhanced Strobe */
            g_emmc_ctx.card_info.ext_csd.strobe_support = 0x01;
        #elif (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400)
            g_emmc_ctx.card_info.ext_csd.card_type = 0x03; /* HS200 + HS400 */
        #elif (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS200)
            g_emmc_ctx.card_info.ext_csd.card_type = 0x02; /* HS200 */
        #else
            g_emmc_ctx.card_info.ext_csd.card_type = 0x01; /* SDR/HS */
        #endif
    #else
        g_emmc_ctx.card_info.ext_csd.card_type = 0x07; /* Support all modes */
        g_emmc_ctx.card_info.ext_csd.strobe_support = 0x01;
    #endif
    
    /* Set partition configuration */
    g_emmc_ctx.card_info.ext_csd.partition_config = 0x00; /* User partition active */
    
    #if EMMC_COMPILE_BOOT_PARTITION
        #ifdef EMMC_STATIC_BOOT_SIZE
            g_emmc_ctx.card_info.ext_csd.boot_size_mult = EMMC_STATIC_BOOT_SIZE / (128 * 1024);
        #else
            g_emmc_ctx.card_info.ext_csd.boot_size_mult = 32; /* Default: 4MB */
        #endif
    #endif
    
    #if EMMC_COMPILE_RPMB
        #ifdef EMMC_STATIC_RPMB_SIZE
            g_emmc_ctx.card_info.ext_csd.rpmb_size_mult = EMMC_STATIC_RPMB_SIZE / (128 * 1024);
        #else
            g_emmc_ctx.card_info.ext_csd.rpmb_size_mult = 32; /* Default: 4MB */
        #endif
    #endif
#endif
    
    /* Set initial state */
    g_emmc_ctx.card_info.state = EMMC_STATE_TRAN;
    g_emmc_ctx.card_info.active_partition = EMMC_PART_USER;
    g_emmc_ctx.card_info.bus_mode = EMMC_MODE_SDR;
    g_emmc_ctx.card_info.bus_width = EMMC_BUS_WIDTH_1;
    g_emmc_ctx.card_info.clock_freq = g_emmc_ctx.current_clock;
    g_emmc_ctx.card_info.enhanced_strobe = false;
    g_emmc_ctx.card_info.initialized = true;
    
    return EMMC_OK;
}

emmc_result_t emmc_parse_cid(const u32 *response, emmc_cid_t *cid)
{
    if (!response || !cid) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Parse CID register fields */
    cid->mid = (u8)((response[0] >> 24) & 0xFF);
    cid->oid = (u16)((response[0] >> 8) & 0xFFFF);
    
    cid->pnm[0] = (u8)(response[0] & 0xFF);
    cid->pnm[1] = (u8)((response[1] >> 24) & 0xFF);
    cid->pnm[2] = (u8)((response[1] >> 16) & 0xFF);
    cid->pnm[3] = (u8)((response[1] >> 8) & 0xFF);
    cid->pnm[4] = (u8)(response[1] & 0xFF);
    cid->pnm[5] = (u8)((response[2] >> 24) & 0xFF);
    
    cid->prv = (u8)((response[2] >> 16) & 0xFF);
    cid->psn = ((response[2] & 0xFFFF) << 16) | ((response[3] >> 16) & 0xFFFF);
    cid->mdt = (u8)((response[3] >> 8) & 0xFF);
    cid->crc = (u8)((response[3] >> 1) & 0x7F);
    
    return EMMC_OK;
}

emmc_result_t emmc_parse_csd(const u32 *response, emmc_csd_t *csd)
{
    if (!response || !csd) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Parse CSD register fields */
    csd->csd_structure = (u8)((response[0] >> 30) & 0x3);
    csd->mmc_prot = (u8)((response[0] >> 26) & 0xF);
    csd->taac = (u8)((response[0] >> 16) & 0xFF);
    csd->nsac = (u8)((response[0] >> 8) & 0xFF);
    csd->tran_speed = (u8)(response[0] & 0xFF);
    
    csd->ccc = (u16)((response[1] >> 20) & 0xFFF);
    csd->read_bl_len = (u8)((response[1] >> 16) & 0xF);
    csd->read_bl_partial = ((response[1] >> 15) & 0x1) != 0;
    csd->write_blk_misalign = ((response[1] >> 14) & 0x1) != 0;
    csd->read_blk_misalign = ((response[1] >> 13) & 0x1) != 0;
    csd->dsr_imp = ((response[1] >> 12) & 0x1) != 0;
    
    csd->c_size = ((response[1] & 0x3FF) << 2) | ((response[2] >> 30) & 0x3);
    csd->vdd_r_curr_min = (u8)((response[2] >> 27) & 0x7);
    csd->vdd_r_curr_max = (u8)((response[2] >> 24) & 0x7);
    csd->vdd_w_curr_min = (u8)((response[2] >> 21) & 0x7);
    csd->vdd_w_curr_max = (u8)((response[2] >> 18) & 0x7);
    csd->c_size_mult = (u8)((response[2] >> 15) & 0x7);
    csd->erase_grp_size = (u8)((response[2] >> 10) & 0x1F);
    csd->erase_grp_mult = (u8)((response[2] >> 5) & 0x1F);
    csd->wp_grp_size = (u8)(response[2] & 0x1F);
    
    csd->wp_grp_enable = ((response[3] >> 31) & 0x1) != 0;
    csd->default_ecc = (u8)((response[3] >> 29) & 0x3);
    csd->r2w_factor = (u8)((response[3] >> 26) & 0x7);
    csd->write_bl_len = (u8)((response[3] >> 22) & 0xF);
    csd->write_bl_partial = ((response[3] >> 21) & 0x1) != 0;
    csd->content_prot_app = ((response[3] >> 16) & 0x1) != 0;
    csd->file_format_grp = ((response[3] >> 15) & 0x1) != 0;
    csd->copy = ((response[3] >> 14) & 0x1) != 0;
    csd->perm_write_protect = ((response[3] >> 13) & 0x1) != 0;
    csd->tmp_write_protect = ((response[3] >> 12) & 0x1) != 0;
    csd->file_format = (u8)((response[3] >> 10) & 0x3);
    csd->ecc = (u8)((response[3] >> 8) & 0x3);
    csd->crc = (u8)((response[3] >> 1) & 0x7F);
    
    return EMMC_OK;
}

emmc_result_t emmc_parse_ext_csd(const u8 *buffer, emmc_ext_csd_t *ext_csd)
{
    if (!buffer || !ext_csd) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Parse key EXT_CSD fields */
    ext_csd->ext_csd_rev = buffer[192];
    ext_csd->csd_structure = buffer[194];
    ext_csd->card_type = buffer[196];
    ext_csd->pwr_cl_52_195 = buffer[200];
    ext_csd->pwr_cl_26_195 = buffer[201];
    ext_csd->pwr_cl_52_360 = buffer[202];
    ext_csd->pwr_cl_26_360 = buffer[203];
    ext_csd->min_perf_r_4_26 = buffer[205];
    ext_csd->min_perf_w_4_26 = buffer[206];
    ext_csd->min_perf_r_8_26_4_52 = buffer[207];
    ext_csd->min_perf_w_8_26_4_52 = buffer[208];
    ext_csd->min_perf_r_8_52 = buffer[209];
    ext_csd->min_perf_w_8_52 = buffer[210];
    
    /* Sector count (4 bytes, little endian) */
    ext_csd->sec_count = (u32)buffer[212] | 
                        ((u32)buffer[213] << 8) |
                        ((u32)buffer[214] << 16) |
                        ((u32)buffer[215] << 24);
                        
    ext_csd->sleep_current_vcc = buffer[220];
    ext_csd->sleep_current_vccq = buffer[221];
    ext_csd->sleep_awake_timeout = buffer[217];
    ext_csd->hc_wp_grp_size = (u32)buffer[221];
    ext_csd->rel_wr_sec_c = buffer[222];
    ext_csd->erase_timeout_mult = buffer[223];
    ext_csd->hc_erase_grp_size = buffer[224];
    ext_csd->acc_size = buffer[225];
    ext_csd->boot_mult = buffer[226];
    ext_csd->boot_info = buffer[228];
    ext_csd->sec_trim_mult = buffer[229];
    ext_csd->sec_erase_mult = buffer[230];
    ext_csd->sec_feature_support = buffer[231];
    ext_csd->trim_mult = buffer[232];
    
    ext_csd->pwr_cl_200_195 = buffer[236];
    ext_csd->pwr_cl_200_360 = buffer[237];
    ext_csd->pwr_cl_ddr_52_195 = buffer[238];
    ext_csd->pwr_cl_ddr_52_360 = buffer[239];
    ext_csd->cache_flush_policy = buffer[240];
    ext_csd->ini_timeout_ap = buffer[241];
    
    ext_csd->correct_prg_sectors_num = (u32)buffer[242] |
                                      ((u32)buffer[243] << 8) |
                                      ((u32)buffer[244] << 16) |
                                      ((u32)buffer[245] << 24);
                                      
    ext_csd->bkops_en = buffer[163];
    ext_csd->bkops_start = buffer[164];
    ext_csd->sanitize_start = buffer[165];
    ext_csd->wr_rel_param = buffer[166];
    ext_csd->wr_rel_set = buffer[167];
    ext_csd->rpmb_size_mult = buffer[168];
    ext_csd->fw_config = buffer[169];
    ext_csd->user_wp = buffer[171];
    ext_csd->boot_wp = buffer[173];
    ext_csd->boot_wp_status = buffer[174];
    ext_csd->erase_group_def = buffer[175];
    ext_csd->boot_bus_conditions = buffer[177];
    ext_csd->boot_config_prot = buffer[178];
    ext_csd->partition_config = buffer[179];
    ext_csd->erased_mem_cont = buffer[181];
    ext_csd->bus_width = buffer[183];
    ext_csd->strobe_support = buffer[184];
    ext_csd->hs_timing = buffer[185];
    ext_csd->power_class = buffer[187];
    ext_csd->cmd_set_rev = buffer[189];
    ext_csd->cmd_set = buffer[191];
    
    return EMMC_OK;
}

u64 emmc_calculate_capacity(const emmc_csd_t *csd, const emmc_ext_csd_t *ext_csd)
{
    if (!csd || !ext_csd) {
        return 0;
    }
    
    /* For eMMC 4.4+, use SEC_COUNT from EXT_CSD if available */
    if (ext_csd->sec_count > 0) {
        return (u64)ext_csd->sec_count * 512;
    }
    
    /* Fallback to CSD calculation for older cards */
    u32 mult = 1 << (csd->c_size_mult + 2);
    u32 blocknr = (csd->c_size + 1) * mult;
    u32 block_len = 1 << csd->read_bl_len;
    
    return (u64)blocknr * block_len;
}

emmc_result_t emmc_check_card_status(u32 status)
{
    if (status & EMMC_R1_ERROR_MASK) {
        if (status & EMMC_R1_OUT_OF_RANGE) return EMMC_ERROR;
        if (status & EMMC_R1_ADDRESS_ERROR) return EMMC_ERROR;
        if (status & EMMC_R1_BLOCK_LEN_ERROR) return EMMC_ERROR;
        if (status & EMMC_R1_ERASE_SEQ_ERROR) return EMMC_ERROR;
        if (status & EMMC_R1_ERASE_PARAM) return EMMC_ERROR;
        if (status & EMMC_R1_WP_VIOLATION) return EMMC_ERROR;
        if (status & EMMC_R1_LOCK_UNLOCK_FAILED) return EMMC_ERROR;
        if (status & EMMC_R1_COM_CRC_ERROR) return EMMC_CRC_ERROR;
        if (status & EMMC_R1_ILLEGAL_COMMAND) return EMMC_ERROR;
        if (status & EMMC_R1_CARD_ECC_FAILED) return EMMC_ERROR;
        if (status & EMMC_R1_CC_ERROR) return EMMC_ERROR;
        if (status & EMMC_R1_ERROR) return EMMC_ERROR;
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_wait_for_state(emmc_state_t target_state, u32 timeout_ms)
{
    u32 status;
    emmc_result_t result;
    u32 elapsed = 0;
    
    while (elapsed < timeout_ms) {
        result = emmc_get_card_status(&status);
        if (result != EMMC_OK) {
            return result;
        }
        
        if (EMMC_GET_CARD_STATE(status) == target_state) {
            return EMMC_OK;
        }
        
        hal_emmc_delay_ms(10);
        elapsed += 10;
    }
    
    return EMMC_TIMEOUT;
}

emmc_result_t emmc_read_cid(emmc_cid_t *cid)
{
    u32 response[4];
    emmc_result_t result;
    
    if (!cid || !g_emmc_ctx.initialized) {
        return EMMC_INVALID_PARAM;
    }
    
    result = emmc_send_command(EMMC_CMD2, 0, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    return emmc_parse_cid(response, cid);
}

emmc_result_t emmc_read_csd(emmc_csd_t *csd)
{
    u32 response[4];
    emmc_result_t result;
    
    if (!csd || !g_emmc_ctx.initialized) {
        return EMMC_INVALID_PARAM;
    }
    
    result = emmc_send_command(EMMC_CMD9, g_emmc_ctx.card_info.rca << 16, EMMC_RESP_R2, response);
    if (result != EMMC_OK) {
        return result;
    }
    
    return emmc_parse_csd(response, csd);
}

emmc_result_t emmc_read_ext_csd(emmc_ext_csd_t *ext_csd)
{
    u8 buffer[512];
    emmc_result_t result;
    
    if (!ext_csd || !g_emmc_ctx.initialized) {
        return EMMC_INVALID_PARAM;
    }
    
    result = emmc_send_command_with_data(EMMC_CMD8, 0, EMMC_RESP_R1, 
                                        buffer, 512, 1, true, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    return emmc_parse_ext_csd(buffer, ext_csd);
}

emmc_result_t emmc_switch_partition(emmc_partition_t partition)
{
    emmc_result_t result;
    u8 partition_config;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    if (partition > EMMC_PART_GP4) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Read current partition configuration */
    partition_config = (partition & 0x07) << 3;
    
    /* Switch partition using SWITCH command */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_PARTITION_CONFIG, 
                             partition_config, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    g_emmc_ctx.active_part = partition;
    return EMMC_OK;
}

emmc_result_t emmc_switch_mode(u8 access_mode, u8 index, u8 value, u32 timeout_ms)
{
    u32 argument;
    emmc_result_t result;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Build SWITCH command argument */
    argument = (access_mode << 24) | (index << 16) | (value << 8);
    
    result = emmc_send_command(EMMC_CMD6, argument, EMMC_RESP_R1B, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for card to be ready after switch */
    result = emmc_wait_for_state(EMMC_STATE_TRAN, timeout_ms);
    if (result != EMMC_OK) {
        return result;
    }
    
    return EMMC_OK;
}

emmc_result_t emmc_set_bus_width(emmc_bus_width_t width)
{
    emmc_result_t result;
    u8 bus_width_value;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Map bus width enum to EXT_CSD value */
    switch (width) {
        case EMMC_BUS_WIDTH_1:
            bus_width_value = 0;
            break;
        case EMMC_BUS_WIDTH_4:
            bus_width_value = 1;
            break;
        case EMMC_BUS_WIDTH_8:
            bus_width_value = 2;
            break;
        default:
            return EMMC_INVALID_PARAM;
    }
    
    /* Set bus width in card */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_BUS_WIDTH, 
                             bus_width_value, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set bus width in controller */
    result = hal_emmc_set_bus_width(width);
    if (result != EMMC_OK) {
        return result;
    }
    
    g_emmc_ctx.card_info.bus_width = width;
    return EMMC_OK;
}

emmc_result_t emmc_set_timing_mode(emmc_bus_mode_t mode)
{
    emmc_result_t result;
    u8 hs_timing_value;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Map timing mode to EXT_CSD HS_TIMING value */
    switch (mode) {
        case EMMC_MODE_SDR:
            hs_timing_value = 0;
            break;
        case EMMC_MODE_HS200:
            hs_timing_value = 2;
            break;
        case EMMC_MODE_HS400:
        case EMMC_MODE_HS400_ES:
            hs_timing_value = 3;
            break;
        default:
            hs_timing_value = 1; /* High Speed */
            break;
    }
    
    /* Set timing mode in card */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_HS_TIMING, 
                             hs_timing_value, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set timing mode in controller */
    result = hal_emmc_set_timing(mode);
    if (result != EMMC_OK) {
        return result;
    }
    
    g_emmc_ctx.card_info.bus_mode = mode;
    return EMMC_OK;
}

emmc_result_t emmc_set_block_length(u32 block_len)
{
    emmc_result_t result;
    
    if (!g_emmc_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    if (block_len == 0 || block_len > 512) {
        return EMMC_INVALID_PARAM;
    }
    
    result = emmc_send_command(EMMC_CMD16, block_len, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    return EMMC_OK;
}

