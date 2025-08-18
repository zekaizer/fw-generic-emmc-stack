/*
 * eMMC Stack Basic Usage Example
 *
 * This example demonstrates how to initialize and use the eMMC stack
 * for basic read/write operations in a bare-metal environment.
 */

#include "../src/protocol/emmc_protocol.h"
#include <string.h>
#include <stdio.h>

/* Example platform-specific HAL configuration */
static hal_emmc_config_t example_hal_config = {
	.base_address = 0x40000000,	 /* Platform-specific eMMC controller base address */
	.max_clock_freq = 200000000, /* 200 MHz maximum */
	.timeout_ms = 5000,			 /* 5 second timeout */
	.dma_enabled = true,		 /* Enable DMA if available */
	.max_bus_width = 8			 /* 8-bit bus width support */
};

/* Example driver configuration */
static emmc_driver_config_t example_driver_config = {
	.init_timeout_ms = 1000,
	.enable_cache = true,
	.enable_bkops = true};

/* Example protocol configuration */
static emmc_protocol_config_t example_protocol_config = {
	.auto_optimize = true,
	.enable_advanced_features = true};

/* Example buffer for testing */
static u8 test_buffer[4096] __attribute__((aligned(32))); /* 8 sectors */

/*
 * Example initialization function
 */
emmc_result_t example_init(void)
{
	emmc_result_t result;

	/* Set up driver configuration */
	example_protocol_config.driver_config = example_driver_config;
	example_protocol_config.driver_config.hal_config = example_hal_config;

	/* Initialize protocol stack */
	result = emmc_protocol_init(&example_protocol_config);
	if (result != EMMC_OK) {
		return result;
	}

	/* Initialize eMMC card */
	result = emmc_initialize();
	if (result != EMMC_OK) {
		return result;
	}

	return EMMC_OK;
}

/*
 * Example read/write test function
 */
emmc_result_t example_read_write_test(void)
{
	emmc_result_t result;
	const emmc_card_info_t *card_info;
	u64 test_sector = 0x1000; /* Test at sector 4096 */
	u32 sector_count = 8;	  /* Test 8 sectors (4KB) */

	/* Get card information */
	card_info = emmc_get_card_info();
	if (!card_info) {
		return EMMC_ERROR;
	}

	/* Prepare test data */
	for (u32 i = 0; i < sizeof(test_buffer); i++) {
		test_buffer[i] = (u8)(i & 0xFF);
	}

	/* Write test data */
	result = emmc_write_sectors(test_sector, sector_count, test_buffer);
	if (result != EMMC_OK) {
		return result;
	}

	/* Clear buffer */
	memset(test_buffer, 0, sizeof(test_buffer));

	/* Read back data */
	result = emmc_read_sectors(test_sector, sector_count, test_buffer);
	if (result != EMMC_OK) {
		return result;
	}

	/* Verify data */
	for (u32 i = 0; i < sizeof(test_buffer); i++) {
		if (test_buffer[i] != (u8)(i & 0xFF)) {
			return EMMC_ERROR; /* Data mismatch */
		}
	}

	return EMMC_OK;
}

/*
 * Example partition switching test
 */
emmc_result_t example_partition_test(void)
{
	emmc_result_t result;
	u64 boot_partition_size;

	/* Get boot partition size */
	result = emmc_get_partition_size(EMMC_PART_BOOT1, &boot_partition_size);
	if (result != EMMC_OK) {
		return result;
	}

	if (boot_partition_size > 0) {
		/* Switch to boot partition 1 */
		result = emmc_select_partition(EMMC_PART_BOOT1);
		if (result != EMMC_OK) {
			return result;
		}

		/* Write some test data to boot partition */
		memset(test_buffer, 0xAA, 512);
		result = emmc_write_sectors(0, 1, test_buffer);
		if (result != EMMC_OK) {
			return result;
		}

		/* Read back and verify */
		memset(test_buffer, 0, 512);
		result = emmc_read_sectors(0, 1, test_buffer);
		if (result != EMMC_OK) {
			return result;
		}

		if (test_buffer[0] != 0xAA) {
			return EMMC_ERROR;
		}

		/* Switch back to user partition */
		result = emmc_select_partition(EMMC_PART_USER);
		if (result != EMMC_OK) {
			return result;
		}
	}

	return EMMC_OK;
}

/*
 * Example performance optimization test
 */
emmc_result_t example_performance_test(void)
{
	emmc_result_t result;
	emmc_bus_width_t width;
	emmc_bus_mode_t mode;
	u32 frequency;

	/* Get current bus configuration */
	result = emmc_get_bus_config(&width, &mode, &frequency);
	if (result != EMMC_OK) {
		return result;
	}

	/* Test different bus configurations */
	if (width < EMMC_BUS_WIDTH_8) {
		result = emmc_set_bus_config(EMMC_BUS_WIDTH_8, mode);
		/* Continue even if this fails - not all platforms support 8-bit */
	}

	/* Test large transfer performance */
	u32 large_sector_count = emmc_get_optimal_transfer_size();
	if (large_sector_count > 8) {
		/* Allocate larger buffer for testing (not shown - would need platform-specific allocation) */
		/* For this example, we'll just use the existing buffer */
		result = emmc_read_sectors(0x2000, 8, test_buffer);
		if (result != EMMC_OK) {
			return result;
		}
	}

	return EMMC_OK;
}

/*
 * Helper function to generate device-unique key (placeholder)
 */
static void generate_device_unique_key(u8 *key)
{
	/* In real implementation, this would derive a key from:
	 * - Device serial number
	 * - Hardware security module
	 * - OTP (One-Time Programmable) memory
	 * - Secure boot chain
	 */
	for (int i = 0; i < 32; i++) {
		key[i] = (u8)(0xA5 ^ i); /* Simple pattern for example */
	}
}

/*
 * Example RPMB initialization (requires external crypto implementation)
 */
static emmc_result_t example_crypto_get_key(u8 *key, u32 key_len)
{
	/* Platform-specific key retrieval implementation would go here */
	/* This is just a placeholder - return stored RPMB key */
	memset(key, 0x42, key_len); /* Dummy key for example */
	return EMMC_OK;
}

static emmc_result_t example_crypto_generate_nonce(u8 *nonce, u32 nonce_len)
{
	/* Platform-specific secure random nonce generation would go here */
	/* This is just a placeholder */
	for (u32 i = 0; i < nonce_len; i++) {
		nonce[i] = (u8)(i * 0x5A); /* Dummy nonce for example */
	}
	return EMMC_OK;
}

static emmc_result_t example_crypto_compute_hmac(const u8 *key, u32 key_len,
												 const u8 *data, u32 data_len,
												 u8 *mac, u32 mac_len)
{
	/* Platform-specific HMAC-SHA256 implementation would go here */
	/* This is just a placeholder */
	memset(mac, 0x5A, mac_len); /* Dummy MAC for example */
	return EMMC_OK;
}

static emmc_result_t example_crypto_verify_hmac(const u8 *key, u32 key_len,
												const u8 *data, u32 data_len,
												const u8 *expected_mac, u32 mac_len)
{
	/* Platform-specific HMAC verification would go here */
	/* This is just a placeholder */
	return EMMC_OK;
}

emmc_result_t example_rpmb_init(void)
{
	emmc_rpmb_crypto_interface_t crypto_interface = {
		.get_key = example_crypto_get_key,
		.generate_nonce = example_crypto_generate_nonce,
		.compute_hmac = example_crypto_compute_hmac,
		.verify_hmac = example_crypto_verify_hmac};

	/* Allocate RPMB frame buffer for operations */
	static emmc_rpmb_frame_t rpmb_frame_buffer[16]; /* 16 frames = 8KB */

	return emmc_rpmb_init(&crypto_interface, rpmb_frame_buffer, 16);
}

/*
 * Example RPMB secure storage operations
 */
emmc_result_t example_rpmb_secure_storage_test(void)
{
	emmc_result_t result;
	u8 authentication_key[32];
	u8 secure_data[256];
	u8 read_buffer[256];
	u32 write_counter;

	/* Generate device-specific authentication key */
	generate_device_unique_key(authentication_key);

	/* Initialize RPMB */
	result = example_rpmb_init();
	if (result != EMMC_OK) {
		return result;
	}

	/* Program authentication key (one-time operation) */
	result = emmc_rpmb_program_key(authentication_key);
	if (result != EMMC_OK) {
		/* Key might already be programmed - continue */
	}

	/* Check write counter */
	result = emmc_rpmb_get_write_counter(&write_counter);
	if (result != EMMC_OK) {
		return result;
	}

	/* Prepare secure data */
	memset(secure_data, 0, sizeof(secure_data));
	snprintf((char *)secure_data, sizeof(secure_data),
			 "Confidential data - Write Counter: %u", write_counter);

	/* Write secure data to RPMB */
	result = emmc_rpmb_write_data(0, secure_data, 1);
	if (result != EMMC_OK) {
		return result;
	}

	/* Read and verify secure data */
	memset(read_buffer, 0, sizeof(read_buffer));
	result = emmc_rpmb_read_data(0, read_buffer, 1);
	if (result != EMMC_OK) {
		return result;
	}

	/* Verify data integrity */
	if (memcmp(secure_data, read_buffer, sizeof(secure_data)) != 0) {
		return EMMC_ERROR; /* Data corruption detected */
	}

	return EMMC_OK;
}

/*
 * Example main function demonstrating complete usage
 */
int example_main(void)
{
	emmc_result_t result;
	const emmc_card_info_t *card_info;

	/* Initialize eMMC stack */
	result = example_init();
	if (result != EMMC_OK) {
		/* Handle initialization error */
		return -1;
	}

	/* Get and display card information */
	card_info = emmc_get_card_info();
	if (card_info) {
		/* Card information is available */
		/* In a real application, you might log this information */
	}

	/* Run basic read/write test */
	result = example_read_write_test();
	if (result != EMMC_OK) {
		/* Handle test error */
		return -2;
	}

	/* Test partition switching */
	result = example_partition_test();
	if (result != EMMC_OK) {
		/* Handle partition test error */
		return -3;
	}

	/* Test performance features */
	result = example_performance_test();
	if (result != EMMC_OK) {
		/* Handle performance test error */
		return -4;
	}

	/* Initialize RPMB (optional) */
	result = example_rpmb_init();
	if (result != EMMC_OK) {
		/* RPMB initialization failed - continue without RPMB */
	} else {
		/* Test RPMB secure storage */
		result = example_rpmb_secure_storage_test();
		if (result != EMMC_OK) {
			/* Handle RPMB test error */
			return -5;
		}
	}

	/* Cleanup */
	emmc_protocol_deinit();

	return 0; /* Success */
}

/*
 * Example error handling function
 */
const char *emmc_error_to_string(emmc_result_t error)
{
	switch (error) {
	case EMMC_OK:
		return "Success";
	case EMMC_ERROR:
		return "General error";
	case EMMC_TIMEOUT:
		return "Timeout";
	case EMMC_CRC_ERROR:
		return "CRC error";
	case EMMC_BUSY:
		return "Device busy";
	case EMMC_NOT_READY:
		return "Not ready";
	case EMMC_INVALID_PARAM:
		return "Invalid parameter";
	case EMMC_NOT_SUPPORTED:
		return "Not supported";
	default:
		return "Unknown error";
	}
}
