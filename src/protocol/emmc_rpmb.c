#include "../include/emmc_config_presets.h"  /* Include static configuration first */
#include "emmc_rpmb.h"
#include "emmc_protocol.h"
#include <string.h>

/* Forward declarations for protocol layer access */
extern emmc_result_t emmc_select_partition(emmc_partition_t partition);
extern emmc_partition_t emmc_get_active_partition(void);
extern emmc_result_t emmc_send_command(u8 cmd_index, u32 arg, emmc_resp_type_t resp_type, u32 *response);
extern emmc_result_t emmc_send_command_with_data(u8 cmd_index, u32 arg, emmc_resp_type_t resp_type,
                                                u8 *buffer, u32 block_size, u32 block_count, 
                                                bool read_operation, u32 *response);

/* RPMB context */
static struct {
    emmc_rpmb_crypto_interface_t crypto;
    emmc_rpmb_frame_t *frame_buffer;
    u32 max_frames;
    bool initialized;
} g_rpmb_ctx = {0};

/* Endian conversion utilities for RPMB frame fields (JESD84-B51 big-endian format) */
static inline u16 be16_to_cpu(__be16 val)
{
#ifdef __LITTLE_ENDIAN__
    return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
#else
    return val;
#endif
}

static inline u32 be32_to_cpu(__be32 val)
{
#ifdef __LITTLE_ENDIAN__
    return ((val >> 24) & 0xFF) | 
           ((val >> 8) & 0xFF00) | 
           ((val & 0xFF00) << 8) | 
           ((val & 0xFF) << 24);
#else
    return val;
#endif
}

static inline __be16 cpu_to_be16(u16 val)
{
#ifdef __LITTLE_ENDIAN__
    return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
#else
    return val;
#endif
}

static inline __be32 cpu_to_be32(u32 val)
{
#ifdef __LITTLE_ENDIAN__
    return ((val >> 24) & 0xFF) | 
           ((val >> 8) & 0xFF00) | 
           ((val & 0xFF00) << 8) | 
           ((val & 0xFF) << 24);
#else
    return val;
#endif
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
    
    /* Set common fields in big-endian format (JESD84-B51) */
    frame->req_resp = cpu_to_be16(req_resp);
    frame->address = cpu_to_be16(address);
    frame->block_count = cpu_to_be16(block_count);
    
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
    
    if (!req_frames || frame_count == 0 || frame_count > g_rpmb_ctx.max_frames) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition if not already there */
    if (emmc_get_active_partition() != EMMC_PART_RPMB) {
        result = emmc_select_partition(EMMC_PART_RPMB);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Set frame count with reliable write flag if needed */
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
    
    if (!resp_frames || frame_count == 0 || frame_count > g_rpmb_ctx.max_frames) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Set frame count for response read */
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
 * @brief Map RPMB result codes to emmc_result_t (JESD84-B51 Section 6.6.22)
 */
static emmc_result_t emmc_rpmb_map_result_code(u16 rpmb_result)
{
    switch (rpmb_result) {
        case EMMC_RPMB_RESULT_OK:
            return EMMC_OK;
        case EMMC_RPMB_RESULT_GENERAL_FAILURE:
            return EMMC_ERROR;
        case EMMC_RPMB_RESULT_AUTH_FAILURE:
            return EMMC_MAC_ERROR;
        case EMMC_RPMB_RESULT_COUNTER_FAILURE:
            return EMMC_COUNTER_ERROR;
        case EMMC_RPMB_RESULT_ADDRESS_FAILURE:
            return EMMC_ADDRESS_ERROR;
        case EMMC_RPMB_RESULT_WRITE_FAILURE:
        case EMMC_RPMB_RESULT_READ_FAILURE:
            return EMMC_ERROR;
        case EMMC_RPMB_RESULT_KEY_NOT_PROGRAMMED:
            return EMMC_AUTH_ERROR;
        default:
            return EMMC_ERROR;
    }
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
 * @brief Constant-time memory comparison to prevent timing attacks
 */
static int emmc_rpmb_constant_time_memcmp(const u8 *a, const u8 *b, size_t len)
{
    u8 result = 0;
    
    /* Always compare all bytes regardless of differences found */
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    
    /* Return 0 if equal, non-zero if different */
    return result;
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

emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface,
							   emmc_rpmb_frame_t *frame_buffer,
							   u32 max_frames)
{
    if (!crypto_interface || !frame_buffer || max_frames == 0) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Validate all required crypto functions */
    if (!crypto_interface->get_key || !crypto_interface->generate_nonce ||
        !crypto_interface->hmac_init || !crypto_interface->hmac_update || 
        !crypto_interface->hmac_final) {
        return EMMC_INVALID_PARAM;
    }
    
    g_rpmb_ctx.crypto = *crypto_interface;
    g_rpmb_ctx.frame_buffer = frame_buffer;
    g_rpmb_ctx.max_frames = max_frames;
    g_rpmb_ctx.initialized = true;
    
    return EMMC_OK;
}

emmc_result_t emmc_rpmb_program_key(const u8 *key)
{
    emmc_rpmb_frame_t *frame;
    emmc_result_t result;
    
    if (!g_rpmb_ctx.initialized || !key) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition */
    result = emmc_select_partition(EMMC_PART_RPMB);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Use first frame from buffer */
    frame = &g_rpmb_ctx.frame_buffer[0];
    memset(frame, 0, sizeof(emmc_rpmb_frame_t));
    
    /* Prepare RPMB frame for key programming */
    frame->req_resp = cpu_to_be16(EMMC_RPMB_WRITE_KEY);
    
    /* Copy key to frame */
    for (int i = 0; i < EMMC_RPMB_KEY_SIZE; i++) {
        frame->key_mac[i] = key[i];
    }
    
    /* Key will be programmed directly to the eMMC, no injection needed */
    
    /* Send RPMB write key command */
    result = emmc_send_command_with_data(EMMC_CMD25, 0, EMMC_RESP_R1,
                                        (u8*)frame, sizeof(emmc_rpmb_frame_t),
                                        1, false, NULL);
    
    return result;
}

emmc_result_t emmc_rpmb_get_write_counter(u32 *counter)
{
    emmc_rpmb_frame_t *request_frame;
    emmc_rpmb_frame_t *response_frame;
    emmc_result_t result;
    
    if (!g_rpmb_ctx.initialized || !counter) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Switch to RPMB partition */
    result = emmc_select_partition(EMMC_PART_RPMB);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Use frames from buffer */
    request_frame = &g_rpmb_ctx.frame_buffer[0];
    response_frame = &g_rpmb_ctx.frame_buffer[0];  /* Reuse same frame after request */
    
    /* Prepare request frame */
    memset(request_frame, 0, sizeof(emmc_rpmb_frame_t));
    request_frame->req_resp = cpu_to_be16(EMMC_RPMB_READ_WCOUNTER);
    
    /* Send write counter read request (JESD84-B51: 1 frame request) */
    result = emmc_rpmb_send_request_frames(request_frame, 1, false);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get response (JESD84-B51: 1 frame response) */
    result = emmc_rpmb_get_response_frames(response_frame, 1);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Extract write counter from big-endian format */
    *counter = be32_to_cpu(response_frame->write_counter);
    
    return emmc_rpmb_map_result_code(be16_to_cpu(response_frame->result));
}

/**
 * @brief Write data to RPMB partition (JESD84-B51 compliant)
 */
emmc_result_t emmc_rpmb_write_data(u16 address, const u8 *data, u16 half_sector_count)
{
    emmc_result_t result;
    u32 write_counter;
    u8 nonce[EMMC_RPMB_NONCE_SIZE];
    
    if (!g_rpmb_ctx.initialized || !data || half_sector_count == 0 || half_sector_count > g_rpmb_ctx.max_frames) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Get current write counter */
    result = emmc_rpmb_get_write_counter(&write_counter);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Generate nonce for this operation */
    result = g_rpmb_ctx.crypto.generate_nonce(nonce, EMMC_RPMB_NONCE_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Use frames from buffer */
    emmc_rpmb_frame_t *request_frames = g_rpmb_ctx.frame_buffer;
    
    for (u16 i = 0; i < half_sector_count; i++) {
        emmc_rpmb_prepare_frame(&request_frames[i], EMMC_RPMB_WRITE_DATA, 
                               address + i, half_sector_count, nonce);
        
        /* Copy data */
        memcpy(request_frames[i].data, data + (i * EMMC_RPMB_DATA_SIZE), 
               EMMC_RPMB_DATA_SIZE);
        
        /* Set write counter in big-endian format (JESD84-B51: same counter for all frames) */
        request_frames[i].write_counter = cpu_to_be32(write_counter);
    }
    
    /* Calculate HMAC over all frames (JESD84-B51: concatenated data) */
    void *hmac_ctx = NULL;
    result = g_rpmb_ctx.crypto.hmac_init(&hmac_ctx, NULL, 0);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Stream HMAC calculation over all frame data (optimized: data+metadata as single frame) */
    for (u16 i = 0; i < half_sector_count; i++) {
        /* Add frame data+metadata to HMAC as contiguous frame (JESD84-B51 big-endian format) */
        result = g_rpmb_ctx.crypto.hmac_update(hmac_ctx, request_frames[i].data, 
                                              EMMC_RPMB_HMAC_DATA_SIZE);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Finalize HMAC and place in last frame only (JESD84-B51) */
    result = g_rpmb_ctx.crypto.hmac_final(hmac_ctx, request_frames[half_sector_count - 1].key_mac, 
                                         EMMC_RPMB_MAC_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Send request frames with reliable write */
    result = emmc_rpmb_send_request_frames(request_frames, half_sector_count, true);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Get result frame - reuse first frame in buffer */
    emmc_rpmb_frame_t *response_frame = &g_rpmb_ctx.frame_buffer[0];
    result = emmc_rpmb_get_response_frames(response_frame, 1);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Check RPMB result */
    result = emmc_rpmb_map_result_code(be16_to_cpu(response_frame->result));
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Verify write counter incremented correctly (JESD84-B51) */
    result = emmc_rpmb_verify_write_counter(write_counter, be32_to_cpu(response_frame->write_counter));
    if (result != EMMC_OK) {
        return result;
    }
    
    return EMMC_OK;
}

/**
 * @brief Read data from RPMB partition (JESD84-B51 compliant)
 */
emmc_result_t emmc_rpmb_read_data(u16 address, u8 *data, u16 half_sector_count)
{
    emmc_result_t result;
    u8 nonce[EMMC_RPMB_NONCE_SIZE];
    
    if (!g_rpmb_ctx.initialized || !data || half_sector_count == 0 || half_sector_count > g_rpmb_ctx.max_frames) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Generate nonce for this operation */
    result = g_rpmb_ctx.crypto.generate_nonce(nonce, EMMC_RPMB_NONCE_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Prepare read request frame using first frame in buffer */
    emmc_rpmb_frame_t *request_frame = &g_rpmb_ctx.frame_buffer[0];
    emmc_rpmb_prepare_frame(request_frame, EMMC_RPMB_READ_DATA, 
                           address, half_sector_count, nonce);
    
    /* Send read request */
    result = emmc_rpmb_send_request_frames(request_frame, 1, false);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Use frames from buffer */
    emmc_rpmb_frame_t *response_frames = g_rpmb_ctx.frame_buffer;
    result = emmc_rpmb_get_response_frames(response_frames, half_sector_count);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Verify nonce in first response frame */
    if (emmc_rpmb_constant_time_memcmp(response_frames[0].nonce, nonce, EMMC_RPMB_NONCE_SIZE) != 0) {
        return EMMC_ERROR;
    }
    
    /* Check RPMB result in first frame */
    result = emmc_rpmb_map_result_code(be16_to_cpu(response_frames[0].result));
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Verify MAC using streaming HMAC */
    void *hmac_ctx = NULL;
    result = g_rpmb_ctx.crypto.hmac_init(&hmac_ctx, NULL, 0);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Stream HMAC calculation over all response frame data (optimized: data+metadata as single frame) */
    for (u16 i = 0; i < half_sector_count; i++) {
        /* Add frame data+metadata to HMAC as contiguous frame (JESD84-B51 big-endian format) */
        result = g_rpmb_ctx.crypto.hmac_update(hmac_ctx, response_frames[i].data, 
                                              EMMC_RPMB_HMAC_DATA_SIZE);
        if (result != EMMC_OK) {
            return result;
        }
    }
    
    /* Calculate expected MAC */
    u8 expected_mac[EMMC_RPMB_MAC_SIZE];
    result = g_rpmb_ctx.crypto.hmac_final(hmac_ctx, expected_mac, EMMC_RPMB_MAC_SIZE);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Verify MAC from last response frame (JESD84-B51) */
    if (emmc_rpmb_constant_time_memcmp(expected_mac, response_frames[half_sector_count - 1].key_mac, 
               EMMC_RPMB_MAC_SIZE) != 0) {
        return EMMC_ERROR;
    }
    
    /* Copy data to output buffer */
    for (u16 i = 0; i < half_sector_count; i++) {
        memcpy(data + (i * EMMC_RPMB_DATA_SIZE), response_frames[i].data, 
               EMMC_RPMB_DATA_SIZE);
    }
    
    return EMMC_OK;
}