#ifndef __R_CONDITION_H__
#define __R_CONDITION_H__
typedef short int16_t;
typedef unsigned long long uint64_t;
#include "../instruction_common.h"

#define X86_CR0_EM				(1 << 2)
#define X86_CR4_OSXSAVE				(1 << 18)
#define X86_CR4_OSFXSR				(1 << 9)
#define X86_CR4_OSXMMEXCPT			(1 << 10)

typedef unsigned long	ulong;
struct addr_m16_16_type {
	u16 offset;
	u16 selector;
};

typedef unsigned __attribute__((vector_size(16))) unsigned_128_bit;
typedef unsigned __attribute__((vector_size(32))) unsigned_256_bit;
typedef union {
	unsigned_128_bit sse;
	unsigned m[4];
} union_unsigned_128;

typedef union {
	unsigned_256_bit avx;
	unsigned m[8];
} union_unsigned_256;


void setup_vm(void);
void setup_ring_env(void);
void set_handle_exception(void);
void do_at_ring0(void (*fn)(const char *), const char *arg);
void set_handle_exception(void);
void condition_LOCK_not_used(void);
void condition_LOCK_used(void);
void condition_ECX_MSR_Reserved_false(void);
void condition_ECX_MSR_Reserved_true(void);
void condition_D_segfault_not_occur(void);
void condition_D_segfault_occur(void);
void condition_Bound_test_pass(void);
void condition_Bound_test_fail(void);
void condition_Sec_Not_mem_location_true(void);
void condition_NumOutIDT_false(void);
void condition_NumOutIDT_true(void);
void condition_exceed_64K_not_hold(void);
void condition_exceed_64K_hold(void);
void condition_Divisor_0_false(void);
void condition_Divisor_0_true(void);
void condition_Sign_Quot_Large_false(void);
void condition_Sign_Quot_Large_true(void);
void condition_ESP_SP_not_hold(void);
void condition_ESP_SP_hold(void);
void condition_MemLocat_true(void);
void condition_MemLocat_false(void);
void condition_CR4_R_W_1(void);
void condition_CR4_R_W_0(void);
void condition_CR4_PCIDE_1(void);
void condition_CR4_PCIDE_0(void);
void condition_AccessCR1567_true(void);
void condition_AccessCR1567_false(void);
void condition_WCR0Invalid_true(void);
void condition_WCR0Invalid_false(void);
void condition_DeAc_true(void);
void condition_DR7_GD_0(void);
void condition_DR7_GD_1(void);
void condition_Mem_Dest_hold(void);
void condition_Quot_large_true(void);
void condition_Quot_large_false(void);
void condition_Sec_Not_mem_location_false(void);
void condition_Sou_Not_mem_location_false(void);
void condition_ECX_MSR_Unimplement_true(void);
void condition_ECX_MSR_Unimplement_false(void);
void condition_Sou_Not_mem_location_true(void);
void condition_register_nc_hold(void);
void condition_register_nc_not_hold(void);
void condition_EDX_EAX_set_reserved_true(void);
void condition_EDX_EAX_set_reserved_false(void);
void condition_imm8_0_hold(void);
void condition_imm8_0_not_hold(void);
void condition_CPUID_RDSEED_0(void);
void condition_CPUID_RDSEED_1(void);
void condition_F2HorF3H_hold(void);
void condition_F2HorF3H_not_hold(void);
void condition_OpcodeExcp_false(void);
void condition_OpcodeExcp_true(void);
void condition_CPUID_RDRAND_0(void);
void condition_CPUID_RDRAND_1(void);
void condition_Mem_Dest_not_hold(void);
void condition_CR0_EM_0(void);
void condition_CR0_EM_1(void);
void condition_CR0_TS_0(void);
void condition_CR0_TS_1(void);
void condition_CR0_NE_1(void);
void condition_FPU_excp_hold(void);
void condition_FPU_excp_not_hold(void);
void condition_CR0_MP_1(void);
void condition_CR0_MP_0(void);
void condition_CPUID_MMX_1(void);
void condition_CPUID_MMX_0(void);
void condition_VEX_used(void);
void condition_CPUID_FMA_1(void);
void condition_CPUID_F16C_1(void);
void condition_CPUID_AVX_1(void);
void condition_CPUID_AVX2_1(void);
void condition_CPUID_AVX2_0(void);
void condition_CPUID_F16C_0(void);
void condition_CPUID_FMA_0(void);
void condition_CPUID_SSE2_1(void);
void condition_VEX_not_used(void);
void condition_Alignment_aligned(void);
void condition_CR4_OSFXSR_1(void);
void condition_unmasked_SIMD_hold(void);
void condition_CR4_OSXMMEXCPT_0(void);
void condition_CR4_OSXMMEXCPT_1(void);
void condition_unmasked_SIMD_not_hold(void);
void condition_CPUID_SSE_1(void);
void condition_CPUID_SSE3_1(void);
void condition_CPUID_SSE4_1_1(void);
void condition_CPUID_SSE4_2_1(void);
void condition_CPUID_SSSE3_1(void);
void condition_CPUID_SSE2_0(void);
void condition_Alignment_unaligned(void);
void condition_CR4_OSFXSR_0(void);
void condition_CPUID_SSE_0(void);
void condition_CPUID_SSE3_0(void);
void condition_CPUID_SSE4_1_0(void);
void condition_CPUID_SSSE3_0(void);
void condition_CPUID_SSE4_2_0(void);
void condition_Bound_Lower(void);
void condition_Bound_Upper(void);
void condition_16b_addressing_hold(void);
void condition_16b_addressing_not_hold(void);
void condition_BNDCFGS_EN_1(void);
void condition_BND4_15_used(void);
void condition_BNDCFGS_EN_0(void);
void condition_BND4_15_not_used(void);
void condition_INTO_EXE_true(void);
void condition_OF_Flag_1(void);
void condition_DeReLoadRead_false(void);
void condition_CR4_UMIP_1(void);
void condition_pgfault_occur(void);
void condition_pgfault_not_occur(void);
void condition_cs_cpl_3(void);
void condition_CR0_AC_1(void);
int do_at_ring3(void (*fn)(const char *), const char *arg);

#endif
