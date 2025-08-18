#ifndef RPMB_CRYPTO_H
#define RPMB_CRYPTO_H

#include "../protocol/emmc_rpmb.h"
#include "hmac_sha256.h"

#define RPMB_CRYPTO_KEY_SIZE	32
#define RPMB_CRYPTO_NONCE_SIZE	16
#define RPMB_CRYPTO_MAC_SIZE	32

typedef struct {
	u8 rpmb_key[RPMB_CRYPTO_KEY_SIZE];
	bool key_set;
	hmac_sha256_ctx_t hmac_ctx;
} rpmb_crypto_context_t;

emmc_result_t rpmb_crypto_init(const u8 *rpmb_key);

const emmc_rpmb_crypto_interface_t *rpmb_crypto_get_interface(void);

#endif /* RPMB_CRYPTO_H */