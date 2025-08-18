/*
 * RPMB (Replay Protected Memory Block) Usage Example
 *
 * This example demonstrates how to use the RPMB functionality with
 * ARM Crypto Extension accelerated HMAC-SHA256 implementation.
 */

#include "../src/protocol/emmc_protocol.h"
#include "../src/crypto/rpmb_crypto.h"
#include <string.h>
#include <stdio.h>

static u8 test_rpmb_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

static emmc_rpmb_frame_t rpmb_frame_buffer[8] __attribute__((aligned(32)));

static u8 test_data[256] = {
	"This is test data for RPMB write/read operations. "
	"RPMB provides authenticated and replay-protected storage "
	"using HMAC-SHA256 authentication with external crypto keys."
};

static u8 read_buffer[256];

emmc_result_t rpmb_example_init(void)
{
	emmc_result_t result;
	const emmc_rpmb_crypto_interface_t *crypto_interface;

	printf("RPMB Example: Initializing crypto interface...\n");

	result = rpmb_crypto_init(test_rpmb_key);
	if (result != EMMC_OK) {
		printf("Error: Failed to initialize RPMB crypto: %d\n", result);
		return result;
	}

	crypto_interface = rpmb_crypto_get_interface();
	if (!crypto_interface) {
		printf("Error: Failed to get crypto interface\n");
		return EMMC_ERROR;
	}

	result = emmc_rpmb_init(crypto_interface, rpmb_frame_buffer, 8);
	if (result != EMMC_OK) {
		printf("Error: Failed to initialize RPMB: %d\n", result);
		return result;
	}

	printf("RPMB Example: Crypto interface initialized successfully\n");
	printf("RPMB Example: SHA256 hardware acceleration available\n");

	return EMMC_OK;
}

emmc_result_t rpmb_example_program_key(void)
{
	emmc_result_t result;

	printf("RPMB Example: Programming authentication key...\n");

	result = emmc_rpmb_program_key(test_rpmb_key);
	if (result != EMMC_OK) {
		printf("Error: Failed to program RPMB key: %d\n", result);
		return result;
	}

	printf("RPMB Example: Key programmed successfully\n");
	return EMMC_OK;
}

emmc_result_t rpmb_example_write_read_test(void)
{
	emmc_result_t result;
	u16 test_address = 0x100;
	u32 write_counter;

	printf("RPMB Example: Testing write/read operations...\n");

	result = emmc_rpmb_get_write_counter(&write_counter);
	if (result != EMMC_OK) {
		printf("Error: Failed to get write counter: %d\n", result);
		return result;
	}
	printf("RPMB Example: Current write counter: %u\n", write_counter);

	printf("RPMB Example: Writing test data to address 0x%04x...\n", test_address);
	result = emmc_rpmb_write_data(test_address, test_data, 1);
	if (result != EMMC_OK) {
		printf("Error: Failed to write RPMB data: %d\n", result);
		return result;
	}
	printf("RPMB Example: Write completed successfully\n");

	result = emmc_rpmb_get_write_counter(&write_counter);
	if (result != EMMC_OK) {
		printf("Error: Failed to get write counter after write: %d\n", result);
		return result;
	}
	printf("RPMB Example: Write counter after write: %u\n", write_counter);

	memset(read_buffer, 0, sizeof(read_buffer));

	printf("RPMB Example: Reading data from address 0x%04x...\n", test_address);
	result = emmc_rpmb_read_data(test_address, read_buffer, 1);
	if (result != EMMC_OK) {
		printf("Error: Failed to read RPMB data: %d\n", result);
		return result;
	}
	printf("RPMB Example: Read completed successfully\n");

	if (memcmp(test_data, read_buffer, 256) == 0) {
		printf("RPMB Example: Data verification PASSED\n");
	} else {
		printf("RPMB Example: Data verification FAILED\n");
		return EMMC_ERROR;
	}

	return EMMC_OK;
}

emmc_result_t rpmb_example_multi_block_test(void)
{
	emmc_result_t result;
	u16 test_address = 0x200;
	u8 multi_block_data[512];
	u8 multi_read_buffer[512];

	printf("RPMB Example: Testing multi-block operations...\n");

	for (u32 i = 0; i < 512; i++) {
		multi_block_data[i] = (u8)(i & 0xFF);
	}

	printf("RPMB Example: Writing 2 blocks to address 0x%04x...\n", test_address);
	result = emmc_rpmb_write_data(test_address, multi_block_data, 2);
	if (result != EMMC_OK) {
		printf("Error: Failed to write multi-block RPMB data: %d\n", result);
		return result;
	}
	printf("RPMB Example: Multi-block write completed successfully\n");

	memset(multi_read_buffer, 0, sizeof(multi_read_buffer));

	printf("RPMB Example: Reading 2 blocks from address 0x%04x...\n", test_address);
	result = emmc_rpmb_read_data(test_address, multi_read_buffer, 2);
	if (result != EMMC_OK) {
		printf("Error: Failed to read multi-block RPMB data: %d\n", result);
		return result;
	}
	printf("RPMB Example: Multi-block read completed successfully\n");

	if (memcmp(multi_block_data, multi_read_buffer, 512) == 0) {
		printf("RPMB Example: Multi-block data verification PASSED\n");
	} else {
		printf("RPMB Example: Multi-block data verification FAILED\n");
		return EMMC_ERROR;
	}

	return EMMC_OK;
}

emmc_result_t rpmb_example_run_all_tests(void)
{
	emmc_result_t result;

	printf("=== RPMB Example: Starting all tests ===\n");

	result = rpmb_example_init();
	if (result != EMMC_OK) {
		return result;
	}

	result = rpmb_example_program_key();
	if (result != EMMC_OK) {
		return result;
	}

	result = rpmb_example_write_read_test();
	if (result != EMMC_OK) {
		return result;
	}

	result = rpmb_example_multi_block_test();
	if (result != EMMC_OK) {
		return result;
	}

	printf("=== RPMB Example: All tests completed successfully ===\n");

	return EMMC_OK;
}

#ifdef RPMB_EXAMPLE_STANDALONE
int main(void)
{
	emmc_result_t result = rpmb_example_run_all_tests();
	return (result == EMMC_OK) ? 0 : 1;
}
#endif