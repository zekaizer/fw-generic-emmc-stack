#ifndef SHA256_SW_H
#define SHA256_SW_H

#include "../include/emmc_types.h"

extern const u32 sha256_k[64];

void sha256_block_software(u32 *state, const u8 *data);

#endif /* SHA256_SW_H */