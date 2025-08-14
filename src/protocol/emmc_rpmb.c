#include "../include/emmc_config_presets.h"  /* Include static configuration first */
#include "emmc_rpmb.h"
#include "emmc_protocol.h"
#include <string.h>

/* Forward declarations for protocol layer access */
extern emmc_result_t emmc_select_partition(emmc_partition_t partition);
extern emmc_partition_t emmc_get_active_partition(void);
extern emmc_result_t emmc_send_command(u8 cmd_index, u32 arg, emmc_response_type_t resp_type, u32 *response);
extern emmc_result_t emmc_send_command_with_data(u8 cmd_index, u32 arg, emmc_response_type_t resp_type,
                                                u8 *buffer, u32 block_size, u32 block_count, 
                                                bool read_operation, u32 *response);

/* RPMB context */
static struct {
    emmc_rpmb_crypto_interface_t crypto;
    bool initialized;
} g_rpmb_ctx = {0};

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
 * @brief Send RPMB request frames (request transmission only)
 */
static emmc_result_t emmc_rpmb_send_request_frames(const emmc_rpmb_frame_t *req_frames,
                                                   u32 frame_count,
                                                   bool reliable_write)
{
    emmc_result_t result;
    
    if (!req_frames || frame_count == 0 || frame_count > EMMC_RPMB_MAX_BLOCKS) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition if not already there */
    if (emmc_get_active_partition() != EMMC_PART_RPMB) {
        result = emmc_select_partition(EMMC_PART_RPMB);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Set block count with reliable write flag if needed */
    u32 cmd23_arg = frame_count;
    if (reliable_write) {
        cmd23_arg |= (1U << 31);  /* Set reliable write flag */
    }
    
    result = emmc_send_command(EMMC_CMD23, cmd23_arg, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Send request frames */
    result = emmc_send_command_with_data(EMMC_CMD25, 0, EMMC_RESP_R1,
                                        (u8*)req_frames, sizeof(emmc_rpmb_frame_t),
                                        frame_count, false, NULL);
    
    return result;
}

/**
 * @brief Get RPMB response frames (response reception only)
 */
static emmc_result_t emmc_rpmb_get_response_frames(emmc_rpmb_frame_t *resp_frames,
                                                   u32 frame_count)
{
    emmc_result_t result;
    
    if (!resp_frames || frame_count == 0 || frame_count > EMMC_RPMB_MAX_BLOCKS) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Set block count for response read */
    result = emmc_send_command(EMMC_CMD23, frame_count, EMMC_RESP_R1, NULL);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Read response frames */
    result = emmc_send_command_with_data(EMMC_CMD18, 0, EMMC_RESP_R1,
                                        (u8*)resp_frames, sizeof(emmc_rpmb_frame_t),
                                        frame_count, true, NULL);
    
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

/* RPMB Public Functions */

emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface)
{
    if (!crypto_interface) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Validate all required crypto functions including streaming HMAC */
    if (!crypto_interface->get_key || !crypto_interface->generate_nonce ||
        !crypto_interface->compute_hmac || !crypto_interface->verify_hmac ||
        !crypto_interface->hmac_init || !crypto_interface->hmac_update || 
        !crypto_interface->hmac_final) {
        return EMMC_INVALID_PARAM;
    }
    
    g_rpmb_ctx.crypto = *crypto_interface;
    g_rpmb_ctx.initialized = true;
    
    return EMMC_OK;
}

emmc_result_t emmc_rpmb_program_key(const u8 *key)
{
    emmc_rpmb_frame_t frame = {0};
    emmc_result_t result;
    
    if (!g_rpmb_ctx.initialized || !key) {
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
    
    if (!g_rpmb_ctx.initialized || !counter) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition */
    result = emmc_select_partition(EMMC_PART_RPMB);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare request frame */
    request_frame.req_resp = EMMC_RPMB_READ_WCOUNTER;
    
    /* Send write counter read request (JESD84-B51: 1 frame request) */
    result = emmc_rpmb_send_request_frames(&request_frame, 1, false);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get response (JESD84-B51: 1 frame response) */
    result = emmc_rpmb_get_response_frames(&response_frame, 1);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Extract write counter */
    *counter = response_frame.write_counter;
    
    return (response_frame.result == EMMC_RPMB_RESULT_OK) ? EMMC_OK : EMMC_ERROR;
}