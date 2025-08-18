#include "sha256_internal.h"
#include <string.h>

#if defined(__ARM_ARCH) && __ARM_ARCH >= 8
#include <arm_neon.h>
#endif


#if defined(__ARM_ARCH) && __ARM_ARCH >= 8
bool sha256_arm_ce_is_supported(void)
{
	u32 id_isar5;
	__asm__ volatile("mrc p15, 0, %0, c0, c2, 5" : "=r"(id_isar5));
	return ((id_isar5 >> 8) & 0xf) >= 1;
}

static void sha256_block_arm32_ce(u32 *state, const u8 *data)
{
	uint32x4_t orig_h0, orig_h1;
	
	orig_h0 = vld1q_u32(&state[0]);
	orig_h1 = vld1q_u32(&state[4]);

	__asm__ volatile(
		"vldm    %[state], {q0-q1}         \n"
		
		"vld1.32 {q2-q3}, [%[data]]!       \n"
		"vld1.32 {q4-q5}, [%[data]]        \n"
		"sub     %[data], %[data], #32     \n"
		"vrev32.8 q2, q2                   \n"
		"vrev32.8 q3, q3                   \n"
		"vrev32.8 q4, q4                   \n"
		"vrev32.8 q5, q5                   \n"
		
		"add     r4, %[k_ptr], #0          \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q2, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q3, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q4, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q5, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q2, q3               \n"
		"sha256su1.32 q2, q4, q5           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q2, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q3, q4               \n"
		"sha256su1.32 q3, q5, q2           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q3, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q4, q5               \n"
		"sha256su1.32 q4, q2, q3           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q4, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q5, q2               \n"
		"sha256su1.32 q5, q3, q4           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q5, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q2, q3               \n"
		"sha256su1.32 q2, q4, q5           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q2, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q3, q4               \n"
		"sha256su1.32 q3, q5, q2           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q3, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q4, q5               \n"
		"sha256su1.32 q4, q2, q3           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q4, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q5, q2               \n"
		"sha256su1.32 q5, q3, q4           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q5, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q2, q3               \n"
		"sha256su1.32 q2, q4, q5           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q2, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q3, q4               \n"
		"sha256su1.32 q3, q5, q2           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q3, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q4, q5               \n"
		"sha256su1.32 q4, q2, q3           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q4, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"sha256su0.32 q5, q2               \n"
		"sha256su1.32 q5, q3, q4           \n"
		"vld1.32 {q6-q7}, [r4]!            \n"
		"vadd.u32 q8, q5, q6               \n"
		"vmov    q9, q1                    \n"
		"sha256h.32  q1, q0, q8            \n"
		"sha256h2.32 q0, q9, q8            \n"
		
		"vadd.u32 q0, q0, %q[orig_h0]      \n"
		"vadd.u32 q1, q1, %q[orig_h1]      \n"
		
		"vstm    %[state], {q0-q1}         \n"
		
		: [state] "+r" (state), [data] "+r" (data)
		: [k_ptr] "r" (sha256_k), [orig_h0] "w" (orig_h0), [orig_h1] "w" (orig_h1)
		: "r4", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "memory"
	);
}
#else
bool sha256_arm_ce_is_supported(void)
{
	return false;
}
#endif

void sha256_arm_ce_block(u32 *state, const u8 *data)
{
#if defined(__ARM_ARCH) && __ARM_ARCH >= 8
	sha256_block_arm32_ce(state, data);
#endif
}


