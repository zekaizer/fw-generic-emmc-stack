#include "rpmb_crypto.h"
#include <string.h>

static rpmb_crypto_context_t g_rpmb_crypto_ctx = {0};

static u32 g_nonce_counter = 0;

static void constant_time_memzero(volatile void *s, u32 len)
{
	volatile u8 *p = (volatile u8 *)s;
	while (len--) {
		*p++ = 0;
	}
}

static u32 get_hardware_random_seed(void)
{
#if defined(__ARM_ARCH)
	u32 pmccntr, pmcr;
	
	__asm__ volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(pmcr));
	pmcr |= 1;
	__asm__ volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(pmcr));
	
	__asm__ volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcr));
	pmcr |= 0x80000000;
	__asm__ volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(pmcr));
	
	__asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
	return pmccntr;
#else
	return 0x12345678;
#endif
}

static u32 simple_prng(u32 *state)
{
	*state = (*state * 1103515245U) + 12345U;
	return *state;
}

emmc_result_t rpmb_crypto_init(const u8 *rpmb_key)
{
	memset(&g_rpmb_crypto_ctx, 0, sizeof(g_rpmb_crypto_ctx));
	
	if (rpmb_key) {
		return rpmb_crypto_set_key(rpmb_key, RPMB_CRYPTO_KEY_SIZE);
	}
	
	g_nonce_counter = get_hardware_random_seed();
	
	return EMMC_OK;
}

static emmc_result_t rpmb_crypto_set_key(const u8 *key, u32 key_len)
{
	if (!key || key_len != RPMB_CRYPTO_KEY_SIZE) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	memcpy(g_rpmb_crypto_ctx.rpmb_key, key, RPMB_CRYPTO_KEY_SIZE);
	g_rpmb_crypto_ctx.key_set = true;
	
	return EMMC_OK;
}

static emmc_result_t rpmb_crypto_get_key(u8 *key, u32 key_len)
{
	if (!key || key_len != RPMB_CRYPTO_KEY_SIZE) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	if (!g_rpmb_crypto_ctx.key_set) {
		return EMMC_ERROR_NOT_INITIALIZED;
	}

	memcpy(key, g_rpmb_crypto_ctx.rpmb_key, RPMB_CRYPTO_KEY_SIZE);
	
	return EMMC_OK;
}

static emmc_result_t rpmb_crypto_generate_nonce(u8 *nonce, u32 nonce_len)
{
	if (!nonce || nonce_len != RPMB_CRYPTO_NONCE_SIZE) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	u32 prng_state = get_hardware_random_seed() ^ g_nonce_counter++;
	
	for (u32 i = 0; i < RPMB_CRYPTO_NONCE_SIZE; i += 4) {
		u32 rand_val = simple_prng(&prng_state);
		u32 copy_len = (RPMB_CRYPTO_NONCE_SIZE - i) < 4 ? (RPMB_CRYPTO_NONCE_SIZE - i) : 4;
		memcpy(&nonce[i], &rand_val, copy_len);
	}
	
	return EMMC_OK;
}

static emmc_result_t rpmb_crypto_hmac_init(void **ctx, const u8 *key, u32 key_len)
{
	if (!ctx || !key || key_len != RPMB_CRYPTO_KEY_SIZE) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	*ctx = &g_rpmb_crypto_ctx.hmac_ctx;
	
	return hmac_sha256_init((hmac_sha256_ctx_t *)*ctx, key, key_len);
}

static emmc_result_t rpmb_crypto_hmac_update(void *ctx, const u8 *data, u32 data_len)
{
	if (!ctx || (!data && data_len > 0)) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	return hmac_sha256_update((hmac_sha256_ctx_t *)ctx, data, data_len);
}

static emmc_result_t rpmb_crypto_hmac_final(void *ctx, u8 *mac, u32 mac_len)
{
	if (!ctx || !mac || mac_len != RPMB_CRYPTO_MAC_SIZE) {
		return EMMC_ERROR_INVALID_PARAM;
	}

	return hmac_sha256_final((hmac_sha256_ctx_t *)ctx, mac);
}

static const emmc_rpmb_crypto_interface_t rpmb_crypto_interface = {
	.get_key = rpmb_crypto_get_key,
	.generate_nonce = rpmb_crypto_generate_nonce,
	.hmac_init = rpmb_crypto_hmac_init,
	.hmac_update = rpmb_crypto_hmac_update,
	.hmac_final = rpmb_crypto_hmac_final
};

const emmc_rpmb_crypto_interface_t *rpmb_crypto_get_interface(void)
{
	return &rpmb_crypto_interface;
}