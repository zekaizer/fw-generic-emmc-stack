#ifndef CRYPTO_TEST_VECTORS_H
#define CRYPTO_TEST_VECTORS_H

#include "../include/emmc_types.h"

typedef struct {
	const char *name;
	const u8 *input;
	u32 input_len;
	const u8 *expected_output;
	u32 output_len;
} crypto_test_vector_t;

typedef struct {
	const char *name;
	const u8 *key;
	u32 key_len;
	const u8 *data;
	u32 data_len;
	const u8 *expected_mac;
	u32 mac_len;
} hmac_test_vector_t;

extern const crypto_test_vector_t sha256_test_vectors[];
extern const u32 sha256_test_vector_count;

extern const hmac_test_vector_t hmac_sha256_test_vectors[];
extern const u32 hmac_sha256_test_vector_count;

emmc_result_t crypto_run_sha256_tests(void);
emmc_result_t crypto_run_hmac_sha256_tests(void);
emmc_result_t crypto_run_all_tests(void);

#endif /* CRYPTO_TEST_VECTORS_H */