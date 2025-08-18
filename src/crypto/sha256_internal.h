#ifndef SHA256_INTERNAL_H
#define SHA256_INTERNAL_H

#include "sha256.h"

struct sha256_ctx {
	u32 state[8];
	u64 count;
	u8 buffer[SHA256_BLOCK_SIZE];
	u32 buffer_len;
};

bool sha256_arm_ce_is_supported(void);

void sha256_arm_ce_block(u32 *state, const u8 *data);

void sha256_block_software(u32 *state, const u8 *data);

#endif /* SHA256_INTERNAL_H */