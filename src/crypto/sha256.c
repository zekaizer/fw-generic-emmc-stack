#include "sha256.h"
#include "sha256_internal.h"
#include <string.h>

static bool use_hw_accel = false;
static bool hw_detect_done = false;

static void sha256_detect_hw(void)
{
	if (!hw_detect_done) {
		use_hw_accel = sha256_arm_ce_is_supported();
		hw_detect_done = true;
	}
}

static void sha256_block(u32 *state, const u8 *data)
{
	sha256_detect_hw();
	
	if (use_hw_accel) {
		sha256_arm_ce_block(state, data);
	} else {
		sha256_block_software(state, data);
	}
}

emmc_result_t sha256_init(sha256_ctx_t *ctx)
{
	if (!ctx) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
	
	ctx->count = 0;
	ctx->buffer_len = 0;
	
	return EMMC_OK;
}

emmc_result_t sha256_update(sha256_ctx_t *ctx, const u8 *data, u32 len)
{
	if (!ctx || (!data && len > 0)) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	ctx->count += len;

	if (ctx->buffer_len > 0) {
		u32 copy_len = SHA256_BLOCK_SIZE - ctx->buffer_len;
		if (copy_len > len) {
			copy_len = len;
		}

		memcpy(ctx->buffer + ctx->buffer_len, data, copy_len);
		ctx->buffer_len += copy_len;
		data += copy_len;
		len -= copy_len;

		if (ctx->buffer_len == SHA256_BLOCK_SIZE) {
			sha256_block(ctx->state, ctx->buffer);
			ctx->buffer_len = 0;
		}
	}

	while (len >= SHA256_BLOCK_SIZE) {
		sha256_block(ctx->state, data);
		data += SHA256_BLOCK_SIZE;
		len -= SHA256_BLOCK_SIZE;
	}

	if (len > 0) {
		memcpy(ctx->buffer, data, len);
		ctx->buffer_len = len;
	}

	return EMMC_OK;
}

emmc_result_t sha256_final(sha256_ctx_t *ctx, u8 *digest)
{
	if (!ctx || !digest) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	u64 bit_count = ctx->count * 8;

	ctx->buffer[ctx->buffer_len] = 0x80;
	ctx->buffer_len++;

	if (ctx->buffer_len > 56) {
		memset(ctx->buffer + ctx->buffer_len, 0, SHA256_BLOCK_SIZE - ctx->buffer_len);
		sha256_block(ctx->state, ctx->buffer);
		ctx->buffer_len = 0;
	}

	memset(ctx->buffer + ctx->buffer_len, 0, 56 - ctx->buffer_len);

	((u32 *)ctx->buffer)[14] = __builtin_bswap32((u32)(bit_count >> 32));
	((u32 *)ctx->buffer)[15] = __builtin_bswap32((u32)bit_count);

	sha256_block(ctx->state, ctx->buffer);

	for (int i = 0; i < 8; i++) {
		((u32 *)digest)[i] = __builtin_bswap32(ctx->state[i]);
	}

	memset(ctx, 0, sizeof(sha256_ctx_t));

	return EMMC_OK;
}

emmc_result_t sha256_compute(const u8 *data, u32 len, u8 *digest)
{
	sha256_ctx_t ctx;
	emmc_result_t result;

	result = sha256_init(&ctx);
	if (result != EMMC_OK) {
		return result;
	}

	result = sha256_update(&ctx, data, len);
	if (result != EMMC_OK) {
		return result;
	}

	return sha256_final(&ctx, digest);
}