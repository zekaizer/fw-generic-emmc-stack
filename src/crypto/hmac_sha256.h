#ifndef HMAC_SHA256_H
#define HMAC_SHA256_H

#include "sha256.h"

#define HMAC_SHA256_KEY_SIZE	32
#define HMAC_SHA256_MAC_SIZE	32
#define HMAC_BLOCK_SIZE			64

typedef struct {
	sha256_ctx_t inner_ctx;
	sha256_ctx_t outer_ctx;
	u8 key_pad[HMAC_BLOCK_SIZE];
	bool initialized;
} hmac_sha256_ctx_t;

emmc_result_t hmac_sha256_init(hmac_sha256_ctx_t *ctx, const u8 *key, u32 key_len);

emmc_result_t hmac_sha256_update(hmac_sha256_ctx_t *ctx, const u8 *data, u32 len);

emmc_result_t hmac_sha256_final(hmac_sha256_ctx_t *ctx, u8 *mac);

emmc_result_t hmac_sha256_compute(const u8 *key, u32 key_len, 
								  const u8 *data, u32 data_len, 
								  u8 *mac);

#endif /* HMAC_SHA256_H */