#include "crypto_test_vectors.h"
#include "sha256.h"
#include "hmac_sha256.h"
#include <string.h>
#include <stdio.h>

static const u8 sha256_empty_input[] = "";
static const u8 sha256_empty_expected[] = {
	0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
	0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
	0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
	0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};

static const u8 sha256_abc_input[] = "abc";
static const u8 sha256_abc_expected[] = {
	0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
	0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
	0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
	0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

static const u8 sha256_448bit_input[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const u8 sha256_448bit_expected[] = {
	0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
	0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
	0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
	0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
};

static const u8 sha256_896bit_input[] = 
	"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
static const u8 sha256_896bit_expected[] = {
	0xcf, 0x5b, 0x16, 0xa7, 0x78, 0xaf, 0x83, 0x80,
	0x03, 0x6c, 0xe5, 0x9e, 0x7b, 0x04, 0x92, 0x37,
	0x0b, 0x24, 0x9b, 0x11, 0xe8, 0xf0, 0x7a, 0x51,
	0xaf, 0xac, 0x45, 0x03, 0x7a, 0xfe, 0xe9, 0xd1
};

const crypto_test_vector_t sha256_test_vectors[] = {
	{
		.name = "SHA256 Empty String",
		.input = sha256_empty_input,
		.input_len = 0,
		.expected_output = sha256_empty_expected,
		.output_len = 32
	},
	{
		.name = "SHA256 'abc'",
		.input = sha256_abc_input,
		.input_len = 3,
		.expected_output = sha256_abc_expected,
		.output_len = 32
	},
	{
		.name = "SHA256 448-bit",
		.input = sha256_448bit_input,
		.input_len = 56,
		.expected_output = sha256_448bit_expected,
		.output_len = 32
	},
	{
		.name = "SHA256 896-bit",
		.input = sha256_896bit_input,
		.input_len = 112,
		.expected_output = sha256_896bit_expected,
		.output_len = 32
	}
};

const u32 sha256_test_vector_count = sizeof(sha256_test_vectors) / sizeof(sha256_test_vectors[0]);

static const u8 hmac_key1[] = {
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b
};
static const u8 hmac_data1[] = "Hi There";
static const u8 hmac_expected1[] = {
	0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
	0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
	0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
	0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
};

static const u8 hmac_key2[] = "Jefe";
static const u8 hmac_data2[] = "what do ya want for nothing?";
static const u8 hmac_expected2[] = {
	0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
	0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
	0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
	0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
};

static const u8 hmac_key3[] = {
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	0xaa, 0xaa, 0xaa, 0xaa
};
static const u8 hmac_data3[] = {
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	0xdd, 0xdd
};
static const u8 hmac_expected3[] = {
	0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46,
	0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7,
	0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22,
	0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe
};

const hmac_test_vector_t hmac_sha256_test_vectors[] = {
	{
		.name = "HMAC-SHA256 Test Case 1",
		.key = hmac_key1,
		.key_len = 20,
		.data = hmac_data1,
		.data_len = 8,
		.expected_mac = hmac_expected1,
		.mac_len = 32
	},
	{
		.name = "HMAC-SHA256 Test Case 2",
		.key = hmac_key2,
		.key_len = 4,
		.data = hmac_data2,
		.data_len = 28,
		.expected_mac = hmac_expected2,
		.mac_len = 32
	},
	{
		.name = "HMAC-SHA256 Test Case 3",
		.key = hmac_key3,
		.key_len = 20,
		.data = hmac_data3,
		.data_len = 50,
		.expected_mac = hmac_expected3,
		.mac_len = 32
	}
};

const u32 hmac_sha256_test_vector_count = sizeof(hmac_sha256_test_vectors) / sizeof(hmac_sha256_test_vectors[0]);

static int constant_time_memcmp(const volatile void *a, const volatile void *b, u32 len)
{
	const volatile u8 *pa = (const volatile u8 *)a;
	const volatile u8 *pb = (const volatile u8 *)b;
	u8 result = 0;
	
	for (u32 i = 0; i < len; i++) {
		result |= pa[i] ^ pb[i];
	}
	
	return result;
}

emmc_result_t crypto_run_sha256_tests(void)
{
	printf("Running SHA256 test vectors...\n");

	for (u32 i = 0; i < sha256_test_vector_count; i++) {
		const crypto_test_vector_t *test = &sha256_test_vectors[i];
		u8 digest[32];
		emmc_result_t result;

		printf("  Test %u: %s\n", i + 1, test->name);

		result = sha256_compute(test->input, test->input_len, digest);
		if (result != EMMC_OK) {
			printf("    FAILED: SHA256 computation error: %d\n", result);
			return result;
		}

		if (constant_time_memcmp(digest, test->expected_output, test->output_len) != 0) {
			printf("    FAILED: Output mismatch\n");
			printf("    Expected: ");
			for (u32 j = 0; j < test->output_len; j++) {
				printf("%02x", test->expected_output[j]);
			}
			printf("\n    Got:      ");
			for (u32 j = 0; j < test->output_len; j++) {
				printf("%02x", digest[j]);
			}
			printf("\n");
			return EMMC_ERROR;
		}

		printf("    PASSED\n");
	}

	printf("All SHA256 tests passed!\n");
	return EMMC_OK;
}

emmc_result_t crypto_run_hmac_sha256_tests(void)
{
	printf("Running HMAC-SHA256 test vectors...\n");

	for (u32 i = 0; i < hmac_sha256_test_vector_count; i++) {
		const hmac_test_vector_t *test = &hmac_sha256_test_vectors[i];
		u8 mac[32];
		emmc_result_t result;

		printf("  Test %u: %s\n", i + 1, test->name);

		result = hmac_sha256_compute(test->key, test->key_len, 
									 test->data, test->data_len, mac);
		if (result != EMMC_OK) {
			printf("    FAILED: HMAC computation error: %d\n", result);
			return result;
		}

		if (constant_time_memcmp(mac, test->expected_mac, test->mac_len) != 0) {
			printf("    FAILED: MAC mismatch\n");
			printf("    Expected: ");
			for (u32 j = 0; j < test->mac_len; j++) {
				printf("%02x", test->expected_mac[j]);
			}
			printf("\n    Got:      ");
			for (u32 j = 0; j < test->mac_len; j++) {
				printf("%02x", mac[j]);
			}
			printf("\n");
			return EMMC_ERROR;
		}

		printf("    PASSED\n");
	}

	printf("All HMAC-SHA256 tests passed!\n");
	return EMMC_OK;
}

emmc_result_t crypto_run_all_tests(void)
{
	emmc_result_t result;

	printf("=== Crypto Test Suite ===\n");
	printf("SHA256 hardware acceleration: Available\n");

	result = crypto_run_sha256_tests();
	if (result != EMMC_OK) {
		return result;
	}

	result = crypto_run_hmac_sha256_tests();
	if (result != EMMC_OK) {
		return result;
	}

	printf("=== All crypto tests passed! ===\n");
	return EMMC_OK;
}