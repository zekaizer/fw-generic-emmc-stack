#ifndef EMMC_RPMB_H
#define EMMC_RPMB_H

#include "../include/emmc_types.h"

/* RPMB operation types */
typedef enum {
    EMMC_RPMB_WRITE_KEY     = 0x01,
    EMMC_RPMB_READ_WCOUNTER = 0x02,
    EMMC_RPMB_WRITE_DATA    = 0x03,
    EMMC_RPMB_READ_DATA     = 0x04,
    EMMC_RPMB_READ_RESULT   = 0x05
} emmc_rpmb_op_type_t;

/* RPMB request/response structure */
typedef struct {
    u8  stuff[196];         /* Stuff bytes */
    u8  key_mac[32];        /* Authentication key or MAC */
    u8  data[256];          /* Data payload */
    u8  nonce[16];          /* Nonce */
    u32 write_counter;      /* Write counter */
    u16 address;            /* Block address */
    u16 block_count;        /* Block count */
    u16 result;             /* Result code */
    u16 req_resp;           /* Request/Response type */
} __attribute__((packed)) emmc_rpmb_frame_t;

/* RPMB external crypto interface */
typedef struct {
    /* External key retrieval function - gets currently active RPMB key */
    emmc_result_t (*get_key)(u8 *key, u32 key_len);
    
    /* External nonce generation function - generates cryptographically secure random nonce */
    emmc_result_t (*generate_nonce)(u8 *nonce, u32 nonce_len);
    
    /* External HMAC-SHA256 computation function */
    emmc_result_t (*compute_hmac)(const u8 *key, u32 key_len,
                                 const u8 *data, u32 data_len,
                                 u8 *mac, u32 mac_len);
    
    /* External MAC verification function */
    emmc_result_t (*verify_hmac)(const u8 *key, u32 key_len,
                                const u8 *data, u32 data_len,
                                const u8 *expected_mac, u32 mac_len);
    
    /* External streaming HMAC initialization function (REQUIRED for multi-frame operations) */
    emmc_result_t (*hmac_init)(void **ctx, const u8 *key, u32 key_len);
    
    /* External streaming HMAC update function (REQUIRED for multi-frame operations) */
    emmc_result_t (*hmac_update)(void *ctx, const u8 *data, u32 data_len);
    
    /* External streaming HMAC finalization function (REQUIRED for multi-frame operations) */
    emmc_result_t (*hmac_final)(void *ctx, u8 *mac, u32 mac_len);
} emmc_rpmb_crypto_interface_t;

/* RPMB Functions */

/**
 * @brief Initialize RPMB with external crypto interface
 * @param crypto_interface External cryptographic functions (ALL functions REQUIRED)
 * @return EMMC_OK on success, error code otherwise
 * 
 * @note All crypto interface functions are mandatory, including streaming HMAC.
 *       Crypto implementations should handle internal buffering/streaming as needed.
 */
emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface);

/**
 * @brief Program RPMB authentication key
 * @param key Authentication key (32 bytes)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_program_key(const u8 *key);

/**
 * @brief Read RPMB write counter
 * @param counter Pointer to store write counter value
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t emmc_rpmb_get_write_counter(u32 *counter);

/**
 * @brief Write data to RPMB partition (single/multi-block optimized)
 * @param address Block address (0-based)
 * @param data Data to write (256 bytes per block)
 * @param block_count Number of blocks to write (max: EMMC_RPMB_MAX_BLOCKS)
 * @return EMMC_OK on success, error code otherwise
 * 
 * Note: Uses key from crypto interface. Optimized for both single and multi-block
 * operations. Batches operations and uses streaming HMAC for efficiency.
 */
emmc_result_t emmc_rpmb_write_data(u16 address, const u8 *data, u16 block_count);

/**
 * @brief Read data from RPMB partition (single/multi-block optimized)
 * @param address Block address (0-based)
 * @param data Buffer to store read data (256 bytes per block)
 * @param block_count Number of blocks to read (max: EMMC_RPMB_MAX_BLOCKS)
 * @return EMMC_OK on success, error code otherwise
 * 
 * Note: Uses key from crypto interface. Optimized for both single and multi-block
 * operations. Reads all blocks in single transaction for best performance.
 */
emmc_result_t emmc_rpmb_read_data(u16 address, u8 *data, u16 block_count);


/* RPMB Constants */
#define EMMC_RPMB_BLOCK_SIZE        256
#define EMMC_RPMB_KEY_SIZE          32
#define EMMC_RPMB_MAC_SIZE          32
#define EMMC_RPMB_NONCE_SIZE        16
#define EMMC_RPMB_MAX_BLOCKS        32     /* Maximum blocks per RPMB transaction */

/* RPMB result codes */
#define EMMC_RPMB_RESULT_OK                 0x0000
#define EMMC_RPMB_RESULT_GENERAL_FAILURE    0x0001
#define EMMC_RPMB_RESULT_AUTH_FAILURE       0x0002
#define EMMC_RPMB_RESULT_COUNTER_FAILURE    0x0003
#define EMMC_RPMB_RESULT_ADDRESS_FAILURE    0x0004
#define EMMC_RPMB_RESULT_WRITE_FAILURE      0x0005
#define EMMC_RPMB_RESULT_READ_FAILURE       0x0006
#define EMMC_RPMB_RESULT_KEY_NOT_PROGRAMMED 0x0007

#endif /* EMMC_RPMB_H */