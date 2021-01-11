#ifndef __V8_CONDITION_H__
#define __V8_CONDITION_H__
#include "v8086_lib.h"

#define FEATURE_INFORMATION_00                          0
#define FEATURE_INFORMATION_01                          1
#define FEATURE_INFORMATION_02                          2
#define FEATURE_INFORMATION_03                          3
#define FEATURE_INFORMATION_04                          4
#define FEATURE_INFORMATION_05                          5
#define FEATURE_INFORMATION_06                          6
#define FEATURE_INFORMATION_07                          7
#define FEATURE_INFORMATION_08                          8
#define FEATURE_INFORMATION_09                          9
#define FEATURE_INFORMATION_10                          10
#define FEATURE_INFORMATION_11                          11
#define FEATURE_INFORMATION_12                          12
#define FEATURE_INFORMATION_13                          13
#define FEATURE_INFORMATION_14                          14
#define FEATURE_INFORMATION_15                          15
#define FEATURE_INFORMATION_16                          16
#define FEATURE_INFORMATION_17                          17
#define FEATURE_INFORMATION_18                          18
#define FEATURE_INFORMATION_19                          19
#define FEATURE_INFORMATION_20                          20
#define FEATURE_INFORMATION_21                          21
#define FEATURE_INFORMATION_22                          22
#define FEATURE_INFORMATION_23                          23
#define FEATURE_INFORMATION_24                          24
#define FEATURE_INFORMATION_25                          25
#define FEATURE_INFORMATION_26                          26
#define FEATURE_INFORMATION_27                          27
#define FEATURE_INFORMATION_28                          28
#define FEATURE_INFORMATION_29                          29
#define FEATURE_INFORMATION_30                          30
#define FEATURE_INFORMATION_31                          31

#define FEATURE_INFORMATION_BIT(SAL)                    ({ (0x1ul << SAL); })

#define NULL ((void *)(0))

typedef unsigned long long u64;
typedef u16 bool;
typedef unsigned __attribute__((vector_size(16))) unsigned_128_bit;
typedef unsigned __attribute__((vector_size(32))) unsigned_256_bit;
typedef union {
	unsigned_128_bit sse __attribute__((aligned(16)));
	unsigned m[4] __attribute__((aligned(16)));
} union_unsigned_128;

typedef union {
	unsigned_256_bit avx;
	unsigned m[8];
} union_unsigned_256;

int do_at_ring3(void (*fn)(const char *), const char *arg);
void do_at_ring0(void (*fn)(const char *), const char *arg);
void condition_INTO_EXE_true(void);
void condition_OF_Flag_1(void);
void condition_DeReLoadRead_false(void);
void condition_CR4_UMIP_1(void);
void condition_pgfault_occur(void);
void condition_pgfault_not_occur(void);
void condition_cs_cpl_3(void);
void condition_CR0_AC_1(void);
void condition_ESP_SP_hold(void);
void condition_CPUID_SSE3_1(void);
void condition_LOCK_not_used(void);
void condition_VEX_not_used(void);
void condition_Alignment_aligned(void);
void condition_CR0_EM_0(void);
void condition_CR4_OSFXSR_1(void);
void condition_CPUID_SSE2_1(void);
void condition_CPUID_SSE4_1_1(void);
void condition_CPUID_SSSE3_1(void);
void condition_CPUID_SSE_1(void);
void condition_CPUID_SSE4_2_1(void);
void condition_Alignment_unaligned(void);

static inline u32 read_cr2(void)
{
	return 0;
}

static inline void write_cr2(u32 cr2)
{
}

#endif