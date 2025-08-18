#ifndef SHA256_ARM_CE_H
#define SHA256_ARM_CE_H

#include "../include/emmc_types.h"

#define SHA256_BLOCK_SIZE	64
#define SHA256_DIGEST_SIZE	32

typedef struct {
	u32 state[8];
	u64 count;
	u8 buffer[SHA256_BLOCK_SIZE];
	u32 buffer_len;
} sha256_arm_ce_ctx_t;

emmc_result_t sha256_arm_ce_init(sha256_arm_ce_ctx_t *ctx);

emmc_result_t sha256_arm_ce_update(sha256_arm_ce_ctx_t *ctx, const u8 *data, u32 len);

emmc_result_t sha256_arm_ce_final(sha256_arm_ce_ctx_t *ctx, u8 *digest);

emmc_result_t sha256_arm_ce_compute(const u8 *data, u32 len, u8 *digest);

bool sha256_arm_ce_is_supported(void);

#endif /* SHA256_ARM_CE_H */