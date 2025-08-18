#include "hmac_sha256.h"
#include <string.h>

#define IPAD_BYTE	0x36
#define OPAD_BYTE	0x5C

static void constant_time_memzero(volatile void *s, u32 len)
{
	volatile u8 *p = (volatile u8 *)s;
	while (len--) {
		*p++ = 0;
	}
}

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

emmc_result_t hmac_sha256_init(hmac_sha256_ctx_t *ctx, const u8 *key, u32 key_len)
{
	if (!ctx || !key || key_len == 0) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	emmc_result_t result;
	u8 hashed_key[SHA256_DIGEST_SIZE];
	const u8 *actual_key;
	u32 actual_key_len;

	memset(ctx, 0, sizeof(hmac_sha256_ctx_t));

	if (key_len > HMAC_BLOCK_SIZE) {
		result = sha256_compute(key, key_len, hashed_key);
		if (result != EMMC_OK) {
			return result;
		}
		actual_key = hashed_key;
		actual_key_len = SHA256_DIGEST_SIZE;
	} else {
		actual_key = key;
		actual_key_len = key_len;
	}

	memset(ctx->key_pad, 0, HMAC_BLOCK_SIZE);
	memcpy(ctx->key_pad, actual_key, actual_key_len);

	for (u32 i = 0; i < HMAC_BLOCK_SIZE; i++) {
		ctx->key_pad[i] ^= IPAD_BYTE;
	}

	result = sha256_init(&ctx->inner_ctx);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	result = sha256_update(&ctx->inner_ctx, ctx->key_pad, HMAC_BLOCK_SIZE);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	for (u32 i = 0; i < HMAC_BLOCK_SIZE; i++) {
		ctx->key_pad[i] ^= (IPAD_BYTE ^ OPAD_BYTE);
	}

	result = sha256_init(&ctx->outer_ctx);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	result = sha256_update(&ctx->outer_ctx, ctx->key_pad, HMAC_BLOCK_SIZE);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	ctx->initialized = true;

cleanup:
	constant_time_memzero(hashed_key, sizeof(hashed_key));
	if (result != EMMC_OK) {
		constant_time_memzero(ctx, sizeof(hmac_sha256_ctx_t));
	}
	
	return result;
}

emmc_result_t hmac_sha256_update(hmac_sha256_ctx_t *ctx, const u8 *data, u32 len)
{
	if (!ctx || !ctx->initialized || (!data && len > 0)) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	return sha256_update(&ctx->inner_ctx, data, len);
}

emmc_result_t hmac_sha256_final(hmac_sha256_ctx_t *ctx, u8 *mac)
{
	if (!ctx || !ctx->initialized || !mac) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	u8 inner_hash[SHA256_DIGEST_SIZE];
	emmc_result_t result;

	result = sha256_final(&ctx->inner_ctx, inner_hash);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	result = sha256_update(&ctx->outer_ctx, inner_hash, SHA256_DIGEST_SIZE);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	result = sha256_final(&ctx->outer_ctx, mac);

cleanup:
	constant_time_memzero(inner_hash, sizeof(inner_hash));
	constant_time_memzero(ctx, sizeof(hmac_sha256_ctx_t));
	
	return result;
}

emmc_result_t hmac_sha256_compute(const u8 *key, u32 key_len, 
								  const u8 *data, u32 data_len, 
								  u8 *mac)
{
	if (!key || key_len == 0 || (!data && data_len > 0) || !mac) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	hmac_sha256_ctx_t ctx;
	emmc_result_t result;

	result = hmac_sha256_init(&ctx, key, key_len);
	if (result != EMMC_OK) {
		return result;
	}

	result = hmac_sha256_update(&ctx, data, data_len);
	if (result != EMMC_OK) {
		goto cleanup;
	}

	result = hmac_sha256_final(&ctx, mac);

cleanup:
	constant_time_memzero(&ctx, sizeof(ctx));
	return result;
}