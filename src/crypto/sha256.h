#ifndef SHA256_H
#define SHA256_H

#include "../include/emmc_types.h"

#define SHA256_BLOCK_SIZE	64
#define SHA256_DIGEST_SIZE	32

typedef struct sha256_ctx sha256_ctx_t;

emmc_result_t sha256_init(sha256_ctx_t *ctx);

emmc_result_t sha256_update(sha256_ctx_t *ctx, const u8 *data, u32 len);

emmc_result_t sha256_final(sha256_ctx_t *ctx, u8 *digest);

emmc_result_t sha256_compute(const u8 *data, u32 len, u8 *digest);

#endif /* SHA256_H */