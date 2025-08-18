#ifndef EMMC_RPMB_H
#define EMMC_RPMB_H

#include "../include/emmc_types.h"

/* RPMB operation types */
typedef enum {
	EMMC_RPMB_WRITE_KEY	 = 0x01,
	EMMC_RPMB_READ_WCOUNTER = 0x02,
	EMMC_RPMB_WRITE_DATA	= 0x03,
	EMMC_RPMB_READ_DATA	 = 0x04,
	EMMC_RPMB_READ_RESULT	 = 0x05
} emmc_rpmb_op_type_t;

/* RPMB request/response structure */
typedef struct {
	u8	stuff[196];		 /* Stuff bytes */
	u8	key_mac[32];		/* Authentication key or MAC */
	u8	data[256];			/* Data payload */
	u8	nonce[16];			/* Nonce */
	__be32 write_counter;	 /* Write counter (big-endian per JESD84-B51) */
	__be16 address;		 /* Half-sector address (0-based, big-endian per JESD84-B51) */
	__be16 block_count;	 /* Number of half-sectors (big-endian per JESD84-B51) */
	__be16 result;			/* Result code (big-endian per JESD84-B51) */
	__be16 req_resp;		/* Request/Response type (big-endian per JESD84-B51) */
} __attribute__((packed)) emmc_rpmb_frame_t;

/* RPMB external crypto interface */
typedef struct {
	/* External key retrieval function - gets currently active RPMB key */
	emmc_result_t (*get_key)(u8 *key, u32 key_len);

	/* External nonce generation function - generates cryptographically secure random nonce */
	emmc_result_t (*generate_nonce)(u8 *nonce, u32 nonce_len);


	/* External streaming HMAC initialization function (REQUIRED for multi-frame operations) */
	emmc_result_t (*hmac_init)(void **ctx, const u8 *key, u32 key_len);

	/* External streaming HMAC update function (REQUIRED for multi-frame operations) */
	emmc_result_t (*hmac_update)(void *ctx, const u8 *data, u32 data_len);

	/* External streaming HMAC finalization function (REQUIRED for multi-frame operations) */
	emmc_result_t (*hmac_final)(void *ctx, u8 *mac, u32 mac_len);
} emmc_rpmb_crypto_interface_t;

/* RPMB Functions */

/**
 * @brief Initialize RPMB with external crypto interface and frame buffer
 * @param crypto_interface External cryptographic functions (ALL functions REQUIRED)
 * @param frame_buffer External buffer for RPMB frames (REQUIRED)
 * @param max_frames Number of frames in buffer (minimum 1 frame required)
 * @return EMMC_OK on success, error code otherwise
 *
 * @note Required functions: get_key, generate_nonce, hmac_init, hmac_update, hmac_final.
 *		 All HMAC operations use streaming interface for memory efficiency.
 *		 Frame buffer must remain valid throughout RPMB operations.
 */
emmc_result_t emmc_rpmb_init(const emmc_rpmb_crypto_interface_t *crypto_interface,
							emmc_rpmb_frame_t *frame_buffer,
							u32 max_frames);

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
 * @brief Write data to RPMB partition (single/multi-frame optimized)
 * @param address Half-sector address (0-based, 256-byte units per JESD84-B51)
 * @param data Data to write (256 bytes per half-sector)
 * @param half_sector_count Number of half-sectors to write (limited by buffer size)
 * @return EMMC_OK on success, error code otherwise
 *
 * Note: Uses key from crypto interface. Optimized for both single and multi-frame
 * operations. Batches operations and uses streaming HMAC for efficiency.
 */
emmc_result_t emmc_rpmb_write_data(u16 address, const u8 *data, u16 half_sector_count);

/**
 * @brief Read data from RPMB partition (single/multi-frame optimized)
 * @param address Half-sector address (0-based, 256-byte units per JESD84-B51)
 * @param data Buffer to store read data (256 bytes per half-sector)
 * @param half_sector_count Number of half-sectors to read (limited by buffer size)
 * @return EMMC_OK on success, error code otherwise
 *
 * Note: Uses key from crypto interface. Optimized for both single and multi-frame
 * operations. Reads all half-sectors in single transaction for best performance.
 */
emmc_result_t emmc_rpmb_read_data(u16 address, u8 *data, u16 half_sector_count);


/* RPMB Constants per JESD84-B51 Section 6.6.22 */
#define EMMC_RPMB_DATA_SIZE			256	/* Half-sector data size per JESD84-B51 */
#define EMMC_RPMB_HALF_SECTOR_SIZE	256	/* Alias for data size clarity */
#define EMMC_RPMB_FRAME_SIZE		512	/* Full RPMB frame size (half sector) */
#define EMMC_RPMB_KEY_SIZE			32	/* Authentication key size */
#define EMMC_RPMB_METADATA_SIZE		28	/* Size of metadata from nonce to req_resp (16+4+2+2+2+2) per JESD84-B51 */
#define EMMC_RPMB_HMAC_DATA_SIZE	284	/* Size of data+metadata for HMAC (256+28) per JESD84-B51 */
#define EMMC_RPMB_MAC_SIZE			32	/* HMAC-SHA256 output size */
#define EMMC_RPMB_NONCE_SIZE		16	/* Random nonce size */

/* RPMB result codes */
#define EMMC_RPMB_RESULT_OK				 0x0000
#define EMMC_RPMB_RESULT_GENERAL_FAILURE	0x0001
#define EMMC_RPMB_RESULT_AUTH_FAILURE		 0x0002
#define EMMC_RPMB_RESULT_COUNTER_FAILURE	0x0003
#define EMMC_RPMB_RESULT_ADDRESS_FAILURE	0x0004
#define EMMC_RPMB_RESULT_WRITE_FAILURE		0x0005
#define EMMC_RPMB_RESULT_READ_FAILURE		 0x0006
#define EMMC_RPMB_RESULT_KEY_NOT_PROGRAMMED 0x0007

#endif /* EMMC_RPMB_H */