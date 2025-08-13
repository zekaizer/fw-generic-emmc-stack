/*
 * eMMC Advanced Features
 * 
 * This file contains optional advanced features that will be automatically
 * removed by the linker if not used (Dead Code Elimination).
 * 
 * Features included:
 * - Cache Management
 * - Background Operations (BKOPS)  
 * - Write Protection
 * - Sanitize/Secure Erase
 * - Health Monitoring
 * - Power Management
 */

#include "emmc_protocol.h"
#include "../driver/emmc_core.h"
#include <string.h>

/* External protocol context (defined in emmc_protocol.c) */
extern struct {
    emmc_protocol_config_t config;
    emmc_rpmb_crypto_interface_t rpmb_crypto;
    bool initialized;
    bool rpmb_initialized;
    u32 optimal_transfer_size;
} g_protocol_ctx;

/* ========================================================================= */
/* Cache Management Functions                                                */
/* ========================================================================= */

emmc_result_t emmc_set_cache_enable(bool enable)
{
    u8 cache_ctrl_value = enable ? 0x01 : 0x00;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_CACHE_CTRL, 
                           cache_ctrl_value, 
                           EMMC_SWITCH_TIMEOUT_MS);
}

emmc_result_t emmc_flush_cache(void)
{
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_FLUSH_CACHE, 
                           0x01, 
                           10000); /* 10 second timeout */
}

/* ========================================================================= */
/* Background Operations (BKOPS) Functions                                  */
/* ========================================================================= */

emmc_result_t emmc_set_bkops_enable(bool enable)
{
    u8 bkops_en_value = enable ? 0x01 : 0x00;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_BKOPS_EN, 
                           bkops_en_value, 
                           EMMC_SWITCH_TIMEOUT_MS);
}

emmc_result_t emmc_start_bkops(void)
{
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_BKOPS_START, 
                           0x01, 
                           EMMC_SWITCH_TIMEOUT_MS);
}

emmc_result_t emmc_check_bkops_status(u8 *level)
{
    emmc_result_t result;
    u32 status;
    
    if (!g_protocol_ctx.initialized || !level) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Get card status */
    result = emmc_get_card_status(&status);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Extract BKOPS level from status bits 7:6 */
    *level = (u8)((status >> 6) & 0x03);
    
    return EMMC_OK;
}

/* ========================================================================= */
/* Write Protection Functions                                               */
/* ========================================================================= */

emmc_result_t emmc_set_write_protect(emmc_partition_t partition, bool enable, bool permanent)
{
    emmc_result_t result;
    u32 start_addr = 0;
    u32 end_addr = 0;
    u64 partition_size;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Switch to target partition */
    result = emmc_select_partition(partition);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get partition size */
    result = emmc_get_partition_size(partition, &partition_size);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Calculate write protection range (entire partition) */
    start_addr = 0;
    end_addr = (u32)(partition_size / 512) - 1; /* Convert to sectors */
    
    if (enable) {
        /* Set write protection */
        result = emmc_send_command(EMMC_CMD28, start_addr, EMMC_RESP_R1B, NULL);
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Set end address */
        result = emmc_send_command(EMMC_CMD28, end_addr, EMMC_RESP_R1B, NULL);
        if (result != EMMC_OK) {
            return result;
        }
        
        /* Apply write protection */
        if (permanent) {
            /* Permanent write protection - irreversible */
            result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                                     EXT_CSD_USER_WP, 
                                     0x01, 
                                     EMMC_SWITCH_TIMEOUT_MS);
        } else {
            /* Temporary write protection */
            result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                                     EXT_CSD_USER_WP, 
                                     0x04, 
                                     EMMC_SWITCH_TIMEOUT_MS);
        }
    } else {
        /* Clear write protection (only if not permanent) */
        if (!permanent) {
            result = emmc_send_command(EMMC_CMD29, start_addr, EMMC_RESP_R1B, NULL);
            if (result != EMMC_OK) {
                return result;
            }
            
            result = emmc_send_command(EMMC_CMD29, end_addr, EMMC_RESP_R1B, NULL);
        } else {
            result = EMMC_NOT_SUPPORTED; /* Cannot clear permanent protection */
        }
    }
    
    return result;
}

emmc_result_t emmc_get_write_protect_status(emmc_partition_t partition, bool *enabled, bool *permanent)
{
    emmc_result_t result;
    u32 wp_status;
    
    if (!g_protocol_ctx.initialized || !enabled || !permanent) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to target partition */
    result = emmc_select_partition(partition);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Send write protect status command */
    result = emmc_send_command(EMMC_CMD30, 0, EMMC_RESP_R1, &wp_status);
    if (result != EMMC_OK) {
        return result;
    }
    
    *enabled = (wp_status & 0x80000000) != 0;    /* WP status bit */
    *permanent = (wp_status & 0x40000000) != 0;   /* Permanent WP bit */
    
    return EMMC_OK;
}

emmc_result_t emmc_set_boot_write_protect(bool enable)
{
    u8 boot_wp_value = enable ? 0x01 : 0x00;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_BOOT_WP, 
                           boot_wp_value, 
                           EMMC_SWITCH_TIMEOUT_MS);
}

emmc_result_t emmc_lock_unlock(const u8 *password, u8 password_len, bool lock, bool force_erase)
{
    emmc_result_t result;
    u8 lock_unlock_data[32] = {0}; /* Maximum password length + header */
    u8 cmd_data_len;
    
    if (!g_protocol_ctx.initialized || !password || password_len == 0 || password_len > 16) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Prepare lock/unlock data */
    lock_unlock_data[0] = lock ? (force_erase ? 0x08 : 0x04) : 0x00; /* Command */
    lock_unlock_data[1] = password_len; /* Password length */
    memcpy(&lock_unlock_data[2], password, password_len);
    
    cmd_data_len = 2 + password_len;
    
    /* Send CMD42 (LOCK_UNLOCK) */
    result = emmc_send_command_with_data(EMMC_CMD42, 0, EMMC_RESP_R1,
                                        lock_unlock_data, cmd_data_len, 
                                        1, false, NULL);
    
    return result;
}

/* ========================================================================= */
/* Sanitize and Secure Erase Functions                                     */
/* ========================================================================= */

emmc_result_t emmc_sanitize(void)
{
    emmc_result_t result;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Start sanitize operation */
    result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                             EXT_CSD_SANITIZE_START, 
                             0x01, 
                             EMMC_SWITCH_TIMEOUT_MS);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for sanitize completion - this can take a very long time */
    return emmc_wait_for_state(EMMC_STATE_TRAN, 300000); /* 5 minute timeout */
}

emmc_result_t emmc_secure_erase(u64 start_sector, u32 sector_count)
{
    emmc_result_t result;
    u32 start_lba, end_lba;
    
    if (!g_protocol_ctx.initialized || sector_count == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    start_lba = emmc_sector_to_lba(start_sector);
    end_lba = emmc_sector_to_lba(start_sector + sector_count - 1);
    
    /* Set secure erase start */
    result = emmc_send_command(EMMC_CMD32, start_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set secure erase end */
    result = emmc_send_command(EMMC_CMD33, end_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Execute secure erase */
    result = emmc_send_command(EMMC_CMD38, 0x80000000, EMMC_RESP_R1B, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for completion */
    return emmc_wait_for_state(EMMC_STATE_TRAN, 60000); /* 1 minute timeout */
}

emmc_result_t emmc_secure_trim(u64 start_sector, u32 sector_count)
{
    emmc_result_t result;
    u32 start_lba, end_lba;
    
    if (!g_protocol_ctx.initialized || sector_count == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    start_lba = emmc_sector_to_lba(start_sector);
    end_lba = emmc_sector_to_lba(start_sector + sector_count - 1);
    
    /* Set secure trim start */
    result = emmc_send_command(EMMC_CMD32, start_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set secure trim end */
    result = emmc_send_command(EMMC_CMD33, end_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Execute secure trim */
    result = emmc_send_command(EMMC_CMD38, 0x80008000, EMMC_RESP_R1B, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Wait for completion */
    return emmc_wait_for_state(EMMC_STATE_TRAN, 30000); /* 30 second timeout */
}

emmc_result_t emmc_trim(u64 start_sector, u32 sector_count)
{
    emmc_result_t result;
    u32 start_lba, end_lba;
    
    if (!g_protocol_ctx.initialized || sector_count == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    start_lba = emmc_sector_to_lba(start_sector);
    end_lba = emmc_sector_to_lba(start_sector + sector_count - 1);
    
    /* Set trim start */
    result = emmc_send_command(EMMC_CMD32, start_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Set trim end */
    result = emmc_send_command(EMMC_CMD33, end_lba, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Execute trim */
    result = emmc_send_command(EMMC_CMD38, 0x00000001, EMMC_RESP_R1B, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    return emmc_wait_for_state(EMMC_STATE_TRAN, 10000); /* 10 second timeout */
}

/* ========================================================================= */
/* Health Monitoring Functions                                              */
/* ========================================================================= */

emmc_result_t emmc_get_health_info(u8 *pre_eol_info, u8 *device_life_time_est_a, u8 *device_life_time_est_b)
{
    const emmc_card_info_t *card_info;
    
    if (!g_protocol_ctx.initialized || !pre_eol_info || !device_life_time_est_a || !device_life_time_est_b) {
        return EMMC_INVALID_PARAM;
    }
    
    card_info = emmc_get_card_info();
    if (!card_info) {
        return EMMC_NOT_READY;
    }
    
    /* Extract health information from EXT_CSD */
    /* Note: These fields are typically at specific EXT_CSD indices */
    *pre_eol_info = 0;        /* EXT_CSD[267] - Pre EOL information */
    *device_life_time_est_a = 0; /* EXT_CSD[268] - Device life time estimation type A */
    *device_life_time_est_b = 0; /* EXT_CSD[269] - Device life time estimation type B */
    
    /* This would require reading current EXT_CSD or storing these values during init */
    return EMMC_NOT_SUPPORTED; /* Placeholder - requires EXT_CSD re-read */
}

emmc_result_t emmc_get_temperature(s8 *temperature)
{
    if (!g_protocol_ctx.initialized || !temperature) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Temperature monitoring would require specific eMMC controller support */
    *temperature = 25; /* Default 25°C - placeholder */
    
    return EMMC_NOT_SUPPORTED; /* Platform-specific implementation required */
}

/* ========================================================================= */
/* Power Management Functions                                               */
/* ========================================================================= */

emmc_result_t emmc_enter_sleep_mode(void)
{
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Send CMD5 (SLEEP_AWAKE) with sleep bit set */
    return emmc_send_command(EMMC_CMD5, 0x80000000, EMMC_RESP_R1B, NULL);
}

emmc_result_t emmc_exit_sleep_mode(void)
{
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Send CMD5 (SLEEP_AWAKE) with awake bit set */
    return emmc_send_command(EMMC_CMD5, 0x00000000, EMMC_RESP_R1B, NULL);
}

emmc_result_t emmc_power_off_notification(u8 notify_type)
{
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    if (notify_type > 0x03) { /* Valid values: 0x00, 0x01, 0x02, 0x03 */
        return EMMC_INVALID_PARAM;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_POWER_OFF_NOTIFICATION, 
                           notify_type, 
                           EMMC_SWITCH_TIMEOUT_MS);
}

emmc_result_t emmc_set_hpi_enable(bool enable)
{
    u8 hpi_mgmt_value = enable ? 0x01 : 0x00;
    
    if (!g_protocol_ctx.initialized) {
        return EMMC_NOT_READY;
    }
    
    return emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE, 
                           EXT_CSD_HPI_MGMT, 
                           hpi_mgmt_value, 
                           EMMC_SWITCH_TIMEOUT_MS);
}