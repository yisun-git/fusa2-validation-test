#ifndef __INSTRUCTION_COMMON_H
#define __INSTRUCTION_COMMON_H

struct gp_fun_index {
	int rqmid;
	void (*func)(void);
};
typedef void (*gp_trigger_func)(void *data);

/* --------------------------common instruction(cpuid, cr*)-------------------------------- */
#define CPUID_BASIC_INFORMATION_00                      0x0
#define CPUID_BASIC_INFORMATION_01                      0x1
#define CPUID_BASIC_INFORMATION_04                      0x4
#define CPUID_BASIC_INFORMATION_07                      0x7
#define CPUID_BASIC_INFORMATION_0D                      0xd

#define CPUID_EXTERN_INFORMATION_801                    0x80000001

#define EXTENDED_STATE_SUBLEAF_0                        0x0
#define EXTENDED_STATE_SUBLEAF_1                        0x1

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

#define FEATURE_INFORMATION_32                          32

#define CPU_NOT_SUPPORT_W                               0x1ff
#define CPU_NOT_SUPPORT_P                               0x3ff
#define CPU_NOT_SUPPORT_L                               0xfff
#define CR4_RESEVED_BIT_23                              0x1ff
#define CR3_RESEVED_BIT_2                               0x3
#define CR3_RESEVED_BIT_5                               0x7f
#define CR8_RESEVED_BIT_4								0xfffffff

#define FEATURE_INFORMATION_BIT(SAL)                    ({ (0x1ul << SAL); })
#define FEATURE_INFORMATION_BIT_RANGE(VAL, SAL)         ({ (((unsigned long)VAL) << SAL); })

#define CR0_AM_MASK	(1UL << 18)

#define CPUID_1_ECX_FMA				(1 << 12)
#define CPUID_1_ECX_XSAVE			(1 << 26)
#define CPUID_1_ECX_OSXSAVE			(1 << 27)
#define CPUID_1_ECX_AVX				(1 << 28)
#define CPUID_1_ECX_F16C			(1 << 29)

/*CPUID.EAX */
#define CPUID_XSAVE_FUC			0xd
#define CPUID_BASIC_INFO_0		0x0
#define CPUID_BASIC_INFO_1		0x1
#define CPUID_BASIC_INFO_2		0x2
#define CPUID_BASIC_INFO_3		0x3
#define CPUID_CACHE_PARAMETERS_LEAF	0x4
#define CPUID_MONITOR_LEAF		0x5
#define CPUID_FEATURE_FLAGS_ENUMERATION_LEAF	0x7

/*CPUID.ECX */
#define CPUID_ECX_DEFUL			0x0

/*
 * CPUID.(DH:0):EAX
 * Processor Extended State Enumeration Main Leaf (EAX = 0DH, ECX = 0)
 */
#define STATE_X87_BIT			0
#define STATE_SSE_BIT			1
#define STATE_AVX_BIT			2
#define STATE_MPX_BNDREGS_BIT		3
#define STATE_MPX_BNDCSR_BIT		4
#define STATE_AVX_512_OPMASK		5
#define STATE_AVX_512_Hi16_ZMM_BIT	7
#define STATE_PT_BIT			8
#define STATE_PKRU_BIT			9
#define STATE_HDC_BIT			13
#define STATE_X87			(1 << STATE_X87_BIT)
#define STATE_SSE			(1 << STATE_SSE_BIT)
#define STATE_AVX			(1 << STATE_AVX_BIT)
#define STATE_MPX_BNDREGS		(1 << STATE_MPX_BNDREGS_BIT)
#define STATE_MPX_BNDCSR		(1 << STATE_MPX_BNDCSR_BIT)
#define STATE_AVX_512			(7 << STATE_AVX_512_OPMASK)
#define STATE_P				(1 << STATE_PT_BIT)
#define STATE_PKRU			(1 << STATE_PKRU_BIT)
#define STATE_HDC			(1 << STATE_HDC_BIT)
#define KBL_NUC_SUPPORTED_XCR0		0x1F
#define XCR0_BIT10_BIT63		0xFFFFFFFFFFFFFC00

/*
 * CPUID.(1:0):ECX
 * Basic CPUID Information.
 */
#define SUPPORT_OSXSAVE_BIT		27
#define SUPPORT_OSXSAVE			(1 << SUPPORT_OSXSAVE_BIT)

/*
 * CPUID.(7:0):EBX
 * Structured Extended Feature Flags Enumeration Leaf (Output depends on ECX input value).
 */
#define SUPPORT_AVX2_BIT		5
#define SUPPORT_AVX512F_BIT		16
#define SUPPORT_AVX512PF_BIT		26
#define SUPPORT_AVX512ER_BIT		27
#define SUPPORT_AVX512CD_BIT		28
#define SUPPORT_AVX512VL_BIT		31
#define SUPPORT_AVX2			(1 << SUPPORT_AVX2_BIT)
#define SUPPORT_AVX512F			(1 << SUPPORT_AVX512F_BIT)
#define SUPPORT_AVX512ER		(1 << SUPPORT_AVX512ER_BIT)
#define SUPPORT_AVX512CD		(1 << SUPPORT_AVX512CD_BIT)
#define SUPPORT_AVX512VL		(1 << SUPPORT_AVX512VL_BIT)
#define SUPPORT_AVX512PF		(1 << SUPPORT_AVX512PF_BIT)

/***
 *XSAVE structure end!
 ******************************************************
 */
#define DS_SEL		0xffff
#define NULL_SEL	0x0000

struct lseg_st {
	u32  offset;
	int16_t selector;
};

struct lseg_st64 {
	u64  offset;
	int16_t selector;
};

/**CPUID Function:**/
uint64_t get_supported_xcr0(void);
int check_cpuid_1_ecx(unsigned int bit);

/**Function about Control register and EFLAG register**/
void set_cr0_AM(int am);
void set_eflag_ac(int ac);

/**Some common function **/
uint64_t get_random_value(void);
#define SETNG_LEN    3
#define BTS_LEN      4
#define LOCK_XOR_LEN 4
#define MUL_LEN      3
#define LFS_LEN 4
#define LSS_LEN 4
#define LGS_LEN 4
#define CMPXCHG8B_LEN 3

/* --------------------------common instruction(cpuid, cr*)--------------------------------------*/
#define CHECK_INSTRUCTION_INFO(str, func)do {		\
	bool result = func();				\
	if (result != true) {				\
		return;					\
	}						\
} while (0)

/* ---------------------- CPUID ---------------------- */
/**
 *@Sub-Conditions:
 *      CPUID.AVX: 1
 *@test purpose:
 *      Confirm the processor support AVX feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=01H,ECX=0):ECX[bit 28] should be 1
 */
bool cpuid_avx_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX: 0
 *@test purpose:
 *      Confirm processor not support AVX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=01H,ECX=0):ECX[bit 28] should be 0
 */
bool cpuid_avx_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX2: 1
 *@test purpose:
 *      Confirm processor support AVX2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 5] should be 1
 */
bool cpuid_avx2_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX2: 0
 *@test purpose:
 *      Confirm processor not support AVX2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 5] should be 0
 */
bool cpuid_avx2_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512F: 0
 *@test purpose:
 *      Confirm processor not support AVX512F Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 16]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 16] should be 0
 */
bool cpuid_avx512f_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512DQ: 0
 *@test purpose:
 *      Confirm processor not support AVX512DQ Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 17]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 17]  should be 0
 */
bool cpuid_avx512dq_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL: 0
 *@test purpose:
 *      Confirm processor not support AVX512VL Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 31]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 31]  should be 0
 */
bool cpuid_avx512vl_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm processor not support AVX512BW Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 30]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 30]  should be 0
 */
bool cpuid_avx512bw_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512ER: 0
 *@test purpose:
 *      Confirm processor not support AVX512ER Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 27]  should be 0
 */
bool cpuid_avx512er_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512PF: 0
 *@test purpose:
 *      Confirm processor not support AVX512PF Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 26]  should be 0
 */
bool cpuid_avx512pf_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512_4VNNIW: 0
 *@test purpose:
 *      No step
 *@Design Steps:
 *      No step
 *@Expected Result:
 *      No step
 */
bool cpuid_avx512_4vnniw_(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_4FMAPS: 0
 *@test purpose:
 *      No step
 *@Design Steps:
 *      No step
 *@Expected Result:
 *      No step
 */
bool cpuid_avx512_4fmaps_(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512CD: 0
 *@test purpose:
 *      Confirm processor not support AVX512CD Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 28]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 28] should be 0
 */
bool cpuid_avx512cd_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512_VBMI: 0
 *@test purpose:
 *      Confirm processor not support AVX512_VBMI Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):ECX[bit 1]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):ECX[bit 1] should be 0
 */
bool cpuid_avx512_vbmi_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512_IFMA: 0
 *@test purpose:
 *      Confirm processor not support AVX512_IFMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 21]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 21] should be 0
 */
bool cpuid_avx512_ifma_to_0(void);
/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512F: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512F Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 16]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} should be 0
 */
bool cpuid_avx512vl_avx512f_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512DQ: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512DQ Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 17]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 17]} should be 0
 */
bool cpuid_avx512vl_avx512dq_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512BW Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} should be 0
 */
bool cpuid_avx512vl_avx512bw_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512B & CPUID.W: 0
 *@test purpose:
 *      Confirm processor not support AVX512BW Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 30]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 30] should be 0
 */
bool cpuid_avx512b_w_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512CD: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512CD Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 28]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 28]} should be 0
 */
bool cpuid_avx512vl_avx512cd_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512VL & CPUID.AVX512_VBMI: 0
 *@test purpose:
 *      Confirm either AVX512VL Feature or AVX512_VBMI Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):ECX[bit 1]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} & {CPUID.(EAX=07H,ECX=0):ECX[bit 1]} should be 0
 */
bool cpuid_avx512vl_avx512vbmi_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512_IFMA & CPUID.AVX512VL: 0
 *@test purpose:
 *      Confirm either AVX512_IFMA Feature or AVX512VL Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 21]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 31]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 21]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 31]} should be 0
 */
bool cpuid_avx512ifma_avx512vl_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512F & CPUID.AVX512BW: 0
 *@test purpose:
 *      Confirm either AVX512F Feature or AVX512BW Feature the processor is NOT supported
 *@Design Steps:
 *      Check {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]}
 *@Expected Result:
 *      {CPUID.(EAX=07H,ECX=0):EBX[bit 16]} & {CPUID.(EAX=07H,ECX=0):EBX[bit 30]} should be 0
 */
bool cpuid_avx512f_avx512bw_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.AVX512D & CPUID.Q: 0
 *@test purpose:
 *      Confirm processor not support AVX512DQ Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 17]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 17] should be 0
 */
bool cpuid_avx512d_avx512q_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.W
 *@test purpose:
 *      Confirm processor not support W(Ways of associativity)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 31:22]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 31:22] should be 0
 */
bool cpuid_w_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.P
 *@test purpose:
 *      Confirm processor not support P(Physical Line partitions)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 21:12]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 21:12] should be 0
 */
bool cpuid_p_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.L
 *@test purpose:
 *      Confirm processor not support L(System Coherency Line Size)
 *@Design Steps:
 *      Check CPUID.(EAX=04H):EBX[bit 11:0]
 *@Expected Result:
 *      CPUID.(EAX=04H):EBX[bit 11:0] should be 0
 */
bool cpuid_l_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSSE3: 0
 *@test purpose:
 *      Insure processor not support SSSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 9]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 9] should be 0
 */
bool cpuid_ssse3_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSSE3: 1
 *@test purpose:
 *      Insure processor support SSSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 9]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 9] should be 1
 */
bool cpuid_ssse3_to_1(void);
/**
 *@Sub-Conditions:
 *      CPUID.SSE: 1
 *@test purpose:
 *      Confirm processor support SSE Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 25]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 25] should be 1
 */
bool cpuid_sse_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE2: 1
 *@test purpose:
 *      Confirm processor support SSE2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 26] should be 1
 */
bool cpuid_sse2_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE3: 1
 *@test purpose:
 *      Confirm processor support SSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 0] should be 1
 */
bool cpuid_sse3_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_1: 1
 *@test purpose:
 *      Confirm processor support SSE4.1 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 19] should be 1
 */
bool cpuid_sse4_1_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_2: 1
 *@test purpose:
 *      Confirm processor support SSE4.2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 20] should be 1
 */
bool cpuid_sse4_2_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE: 0
 *@test purpose:
 *      Confirm processor not support SSE Feature
 *@Design Steps:
 *      Check CPUID.(EAX=0DH,ECX=0):EAX[bit 1]
 *@Expected Result:
 *      CPUID.(EAX=0DH,ECX=0):EAX[bit 1] should be 0
 */
bool cpuid_sse_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE2: 0
 *@test purpose:
 *      Confirm processor not support SSE2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 26] should be 0
 */
bool cpuid_sse2_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE3: 0
 *@test purpose:
 *      Confirm processor not support SSE3 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 0] should be 0
 */
bool cpuid_sse3_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_1: 0
 *@test purpose:
 *      Confirm processor not support SSE4.1 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 19] should be 0
 */
bool cpuid_sse4_1_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SSE4_2: 0
 *@test purpose:
 *      Confirm processor not support SSE4.2 Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 20] should be 0
 */
bool cpuid_sse4_2_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.SMAP: 1
 *@test purpose:
 *      Confirm processor support SMAP Feature
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 20]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 20] should be 1
 */
bool cpuid_smap_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.MMX: 1
 *@test purpose:
 *      Confirm processor support MMX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 23]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 23] should be 1
 */
bool cpuid_mmx_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.MMX: 0
 *@test purpose:
 *      Confirm processor not support MMX Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):EDX[bit 23]
 *@Expected Result:
 *      CPUID.(EAX=01H):EDX[bit 23] should be 0
 */
bool cpuid_mmx_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDTSCP: 0
 *@test purpose:
 *      Confirm processor NOT support RDTSCP
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):EDX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):EDX[bit 27] should be 0
 */
bool cpuid_rdtscp_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDTSCP: 1
 *@test purpose:
 *      Confirm processor support RDTSCP
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):EDX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):EDX[bit 27] should be 1
 */
bool cpuid_rdtscp_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.XSAVEOPT: 1
 *@test purpose:
 *      Confirm processor support XSAVEOPT
 *@Design Steps:
 *      Check CPUID.(EAX=0DH,ECX=1):EAX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=0DH,ECX=1):EAX[bit 0] should be 1
 */
bool cpuid_xsaveopt_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.ADX:0
 *@test purpose:
 *      Confirm processor NOT support ADX
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 19] should be 0
 */
bool cpuid_adx_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.ADX:1
 *@test purpose:
 *      Confirm processor support ADX
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 19]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 19] should be 1
 */
bool cpuid_adx_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHFLOCK:1
 *@test purpose:
 *      Confirm processor support LAHF/SAHF
 *@Design Steps:
 *      Check CPUID.(EAX=80000001H):ECX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=80000001H):ECX[bit 0] should be 1
 */
bool cpuid_lahf_sahflock_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.FSGSBASELOCK:1
 *@test purpose:
 *      Confirm processor support FSBASE/GSBASE
 *@Design Steps:
 *      Check CPUID.(EAX=07H,ECX=0):EBX[bit 0]
 *@Expected Result:
 *      CPUID.(EAX=07H,ECX=0):EBX[bit 0] should be 1
 */
bool cpuid_fsgsbaselock_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.XSAVE:1
 *@test purpose:
 *      Confirm processor support XSAVE
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 26]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 26] should be 1
 */
bool cpuid_xsave_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.OSXSAVE:1
 *@test purpose:
 *      Confirm OS has set CR4.OSXSAVE[bit 18] to enable XSETBV/XGETBV instructions to access XCR0
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 27]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 27] should be 1
 */
bool cpuid_osxsave_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.F16C: 0
 *@test purpose:
 *      Confirm processor not support F16C Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 29]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 29] should be 0
 */
bool cpuid_f16c_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.F16C: 1
 *@test purpose:
 *      Confirm processor support F16C Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 29]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 29] should be 1
 */
bool cpuid_f16c_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.FMA: 0
 *@test purpose:
 *      Confirm processor not support FMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 12]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 12] should be 0
 */
bool cpuid_fma_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.FMA: 1
 *@test purpose:
 *      Confirm processor support FMA Feature
 *@Design Steps:
 *      Check CPUID.(EAX=01H):ECX[bit 12]
 *@Expected Result:
 *      CPUID.(EAX=01H):ECX[bit 12] should be 1
 */
bool cpuid_fma_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.CMPXCHG16B: 1
 *@test purpose:
 *      Confirm processor support CMPXCHG16B
 *@Design Steps:
 *      Check CPUID.01H:ECX.CMPXCHG16B[bit 13]
 *@Expected Result:
 *      CPUID.01H:ECX.CMPXCHG16B[bit 13] should be 1
 */
bool cpuid_cmpxchg16b_to_1(void);
/**
 *@Sub-Conditions:
 *      CPUID.CMPXCHG16B: 0
 *@test purpose:
 *      Confirm processor NOT support CMPXCHG16B
 *@Design Steps:
 *      Check CPUID.01H:ECX.CMPXCHG16B[bit 13]
 *@Expected Result:
 *      CPUID.01H:ECX.CMPXCHG16B[bit 13] should be 0
 */
bool cpuid_cmpxchg16b_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHF: 1
 *@test purpose:
 *      Confirm processor support LAHF-SAH
 *@Design Steps:
 *      Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0]
 *@Expected Result:
 *      CPUID.80000001H:ECX.LAHF-SAHF[bit 0] should be 1
 */
bool cpuid_lahf_sahf_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.LAHF-SAHF: 0
 *@test purpose:
 *      Confirm processor NOT support LAHF-SAHF
 *@Design Steps:
 *      Check CPUID.80000001H:ECX.LAHF-SAHF[bit 0]
 *@Expected Result:
 *      CPUID.80000001H:ECX.LAHF-SAHF[bit 0] should be 0
 */
bool cpuid_lahf_sahf_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDSEED: 1
 *@test purpose:
 *      Confirm processor support RDSEED
 *@Design Steps:
 *      Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
 *@Expected Result:
 *      CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] should be 1
 */
bool cpuid_rdseed_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDSEED: 0
 *@test purpose:
 *      Confirm processor NOT support RDSEED
 *@Design Steps:
 *      Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
 *@Expected Result:
 *      CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18] should be 0
 */
bool cpuid_rdseed_to_0(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDRAND: 1
 *@test purpose:
 *      Confirm processor support RDRAND
 *@Design Steps:
 *      Check CPUID.01H:ECX.RDRAND[bit 30]
 *@Expected Result:
 *      CPUID.01H:ECX.RDRAND[bit 30] should be 1
 */
bool cpuid_rdrand_to_1(void);

/**
 *@Sub-Conditions:
 *      CPUID.RDRAND: 0
 *@test purpose:
 *      Confirm processor NOT support RDRAND
 *@Design Steps:
 *      Check CPUID.01H:ECX.RDRAND[bit 30]
 *@Expected Result:
 *      CPUID.01H:ECX.RDRAND[bit 30] should be 0
 */
bool cpuid_rdrand_to_0(void);

/* ---------------------- CPUID end------------------- */

/* ---------------------- CR4 ---------------------- */

/**
 *@Sub-Conditions:
 *      CR4.OSXSAVE: 0
 *@test purpose:
 *      Disable XSAVE feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osxsave_to_0(void);

/**
 *@Sub-Conditions:
 *      CR4.OSXSAVE: 1
 *@test purpose:
 *      Enable XSAVE feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osxsave_to_1(void);

/**
 *@Sub-Conditions:
 *      CR4.OSFXSR: 0
 *@test purpose:
 *      Disable FXSAVE and FXRSTOR feature set
 *@Design Steps:
 *      Clear CR4.OSXSAVE[bit 19] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osfxsr_to_0(void);

/**
 *@Sub-Conditions:
 *      CR4.OSFXSR: 1
 *@test purpose:
 *      Enable FXSAVE and FXRSTOR feature set
 *@Design Steps:
 *      Set CR4.OSXSAVE[bit 19] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osfxsr_to_1(void);
/**
 *@Sub-Conditions:
 *      CR4.OSXMMEXCPT: 0
 *@test purpose:
 *      Clear CR4.OSXMMEXCPT[bit 10]
 *@Design Steps:
 *      Clear CR4.OSXMMEXCPT[bit 10] to 0
 *@Expected Result:
 *      N/A
 */
bool cr4_osxmmexcpt_to_0(void);

/**
 *@Sub-Conditions:
 *      CR4.OSXMMEXCPT: 1
 *@test purpose:
 *      To use CR4.OSXMMEXCPT[bit 10] to provide additional control
 *      over generation of SIMD floating-pobool exceptions
 *@Design Steps:
 *      Set CR4.OSXMMEXCPT[bit 10] to 1
 *@Expected Result:
 *      N/A
 */
bool cr4_osxmmexcpt_to_1(void);

/* ---------------------- CR4 end------------------- */

/* ---------------------- CR0 ---------------------- */

/**
 *@Sub-Conditions:
 *      CR0.PE: 0
 *@test purpose:
 *      Clear Page Enable flag, to enables real-address
 *      mode(only enables segment-level protection)
 *@Design Steps:
 *      Clear CR0.PE[bit 0] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_pe_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.PE: 1
 *@test purpose:
 *      Set Page Enable flag, to enables protected
 *      mode(only enables segment-level protection)
 *@Design Steps:
 *      Set CR0.PE[bit 0] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_pe_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.MP: 0
 *@test purpose:
 *      Clear Monitor Coprocessor flag
 *@Design Steps:
 *      Clear CR0.MP[bit 1] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_mp_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.MP: 1
 *@test purpose:
 *      Set Monitor Coprocessor flag
 *@Design Steps:
 *      Set CR0.MP[bit 1] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_mp_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.EM: 0
 *@test purpose:
 *      Clear Emulation flag
 *@Design Steps:
 *      Clear CR0.EM[bit 2] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_em_to_0(void);
/**
 *@Sub-Conditions:
 *      CR0.EM: 1
 *@test purpose:
 *      Set Emulation flag
 *@Design Steps:
 *      Set CR0.EM[bit 2] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_em_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.TS: 0
 *@test purpose:
 *      Clear Task Switched flag
 *@Design Steps:
 *      Clear CR0.TS[bit 3] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ts_to_0(void);
/**
 *@Sub-Conditions:
 *      CR0.TS: 1
 *@test purpose:
 *      Sets Task Switched flag on every task switch and tests it
 *      when executing x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions
 *@Design Steps:
 *      Set CR0.TS[bit 3] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ts_to_1(void);
/**
 *@Sub-Conditions:
 *      CR0.ET: 1
 *@test purpose:
 *      Extension Type,In the Pentium 4, Intel Xeon,
 *      and P6 family processors, this flag is hardcoded to 1
 *@Design Steps:
 *      Set CR0.ET[bit 4] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_et_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.NE: 0
 *@test purpose:
 *      Clear Numeric Error flag,enables the PC-style x87 FPU error reporting mechanism
 *@Design Steps:
 *      Clear CR0.NE[bit 5] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ne_to_0(void);
/**
 *@Sub-Conditions:
 *      CR0.NE: 1
 *@test purpose:
 *      Set Numeric Error flag,enables the native (boolernal) mechanism for reporting x87 FPU errors
 *@Design Steps:
 *      Set CR0.NE[bit 5] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ne_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.WP: 0
 *@test purpose:
 *      Clear Write Protect flag to allow supervisor-level procedures to write boolo read-only pages
 *@Design Steps:
 *      Clear CR0.WP[bit 16] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_wp_to_0(void);
/**
 *@Sub-Conditions:
 *      CR0.WP: 1
 *@test purpose:
 *      Set Write Protect flag, inhibits supervisor-level procedures from writing boolo readonly pages
 *@Design Steps:
 *      Set CR0.WP[bit 16] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_wp_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.AM: 0
 *@test purpose:
 *      Disables automatic alignment checking
 *@Design Steps:
 *      Clear CR0.AM[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_am_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.AM: 1
 *@test purpose:
 *      Enables automatic alignment checking
 *@Design Steps:
 *      Set CR0.AM[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_am_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.AC: 0
 *@test purpose:
 *      Disables automatic alignment checking
 *@Design Steps:
 *      Clear CR0.AM[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_ac_to_0(void);
/**
 *@Sub-Conditions:
 *      CR0.AC: 1
 *@test purpose:
 *      Enables automatic alignment checking
 *@Design Steps:
 *      Set CR0.AM[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_ac_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.NW: 0
 *@test purpose:
 *      Enable write-back/write-throug
 *@Design Steps:
 *      Clear CR0.NW[bit 29] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_nw_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.NW: 1
 *@test purpose:
 *      Disable write-back/write-through
 *@Design Steps:
 *      Set CR0.NW[bit 29] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_nw_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.CD: 0
 *@test purpose:
 *      Enable Cache
 *@Design Steps:
 *      Clear CR0.CD[bit 30] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_cd_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.CD: 1
 *@test purpose:
 *      Disable Cache
 *@Design Steps:
 *      Set CR0.CD[bit 30] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_cd_to_1(void);

/**
 *@Sub-Conditions:
 *      CR0.PG: 0
 *@test purpose:
 *      Disables paging, try to clear the bit of CR0.PG
 *@Design Steps:
 *      Clear CR0.PG[bit 31] to 0
 *@Expected Result:
 *      N/A
 */
bool cr0_pg_to_0(void);

/**
 *@Sub-Conditions:
 *      CR0.PG: 1
 *@test purpose:
 *      Enables paging
 *@Design Steps:
 *      Instruction execute Normally:
 *      after set PE(CR0[bit 0]) to 0, set CR0.PG[bit 31] to 1;
 *      #GP: clear PE(CR0[bit 0]), then set CR0.PG[bit 31] to 1
 *@Expected Result:
 *      N/A
 */
bool cr0_pg_to_1(void);

/* ---------------------- CR0 end---------------------- */

/* ---------------------- EFLAGS ---------------------- */

/**
 *@Sub-Conditions:
 *      EFLAGS.CF: 1
 *@Design Steps:
 *      Set EFLAGS.CF[bit 0] to 1
 *@Expected Result:
 *      N/A
 */
bool eflags_cf_to_1(void);

/**
 *@Sub-Conditions:
 *      EFLAGS.AC: 1
 *@test purpose:
 *      Enable Alignment check/access control flag, when CR0.AM is set
 *@Design Steps:
 *      Set EFLAGS.AC[bit 18] to 1
 *@Expected Result:
 *      N/A
 */
bool eflags_ac_to_1(void);
/**
 *@Sub-Conditions:
 *      EFLAGS.AC: 0
 *@test purpose:
 *      Clear Alignment check/access control flag
 *@Design Steps:
 *      Set EFLAGS.AC[bit 18] to 0
 *@Expected Result:
 *      N/A
 */
bool eflags_ac_to_0(void);
/**
 *@Sub-Conditions:
 *      EFLAGS.NT: 1
 *@test purpose:
 *      Set EFLAGS.NT to 1 in the current task's TSS
 *@Design Steps:
 *      Set current TSS's EFLAGS.NT[bit 14] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception.
 */
bool eflags_nt_to_1(void);

/**
 *@Sub-Conditions:
 *      EFLAGS.NT: 0
 *@test purpose:
 *      Set EFLAGS.NT to 0 in the current task's TSS
 *@Design Steps:
 *      Set current TSS's EFLAGS.NT[bit 14] to 0
 *@Expected Result:
 *      N/A
 */
bool eflags_nt_to_0(void);

void cr4_r_w_to_1(void);

void cr3_r_w_to_1(const char *fun_name);

void cr8_r_w_to_1(const char *fun_name);

/**
 *@Sub-Conditions:
 *      CR4.PCIDE: 1
 *@test purpose:
 *      Try to Set the bit of Enables process-context identifiers in CR4
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pcide_to_1(void);

/**
 *@Sub-Conditions:
 *      CR4.PCIDE: 0
 *@test purpose:
 *      Try to clear the bit of Enables process-context identifiers in CR4
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 0
 *@Expected Result:
 *      No exception
 */
bool cr4_pcide_to_0(void);

/**
 *@Sub-Conditions:
 *      CR4.PCIDE_UP: true
 *@test purpose:
 *      Try to Set the bit of Enables process-context identifiers in CR4
 *      with CR3[11:0] isn't equal with 000H
 *@Design Steps:
 *      Using MOV instruction to set the PCID-Enable Bit CR4.PCIDE[bit 17] to 1,
 *      when CR3[11:0] isn't equal with 000H
 *      (if CR3[11:0] isn't equal 000H, set CR3[11:0] to 000H)
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pcide_up_to_1(void);

/**
 *@Sub-Conditions:
 *      CR4.PAE: 0
 *@test purpose:
 *      Try to clear the bit of CR4.PAE[bit5] to leave IA-32e mode
 *@Design Steps:
 *      Using MOV instruction to clear CR4.PAE[bit 5] to 0
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception
 */
bool cr4_pae_to_0(void);

/**
 *@Sub-Conditions:
 *      CR4.PAE: 1
 *@test purpose:
 *      Try to set the bit of CR4.PAE[bit5] to leave IA-32e mode
 *@Design Steps:
 *      Using MOV instruction to set CR4.PAE[bit 5] to 1
 *@Expected Result:
 *      No exception
 */
bool cr4_pae_to_1(void);

/* ---------------------- EFLAGS end------------------- */

/* ---------------------- GP ---------------------- */

/**
 *@Sub-Conditions:
 *      DR7.GD: 0
 *@test purpose:
 *      Clear GD flag in DR7(general detect enable flag)
 *@Design Steps:
 *      Set DR7.GD[bit 13]  to 0
 *@Expected Result:
 *      N/A
 */
bool dr7_gd_to_0(void);

/**
 *@Sub-Conditions:
 *      DR7.GD: 1
 *@test purpose:
 *      Set GD flag in DR7(general detect enable flag)
 *@Design Steps:
 *      Set DR7.GD[bit 13] to 1
 *@Expected Result:
 *      N/A
 */
bool dr7_gd_to_1(void);

/**
 *@Sub-Conditions:
 *      DR6: 1
 *@test purpose:
 *      Using MOV instruction to write a 1 to any of bits 63:32 in DR6
 *@Design Steps:
 *      Using MOV instruction to set DR6[any bit 32:63]to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception in real mode
 */
void write_dr6_checking(unsigned long val);
void dr6_to_1(void);

/**
 *@Sub-Conditions:
 *      DR7: 1
 *@test purpose:
 *      Using MOV instruction to write a 1 to any of bits 63:32 in DR7
 *@Design Steps:
 *      Using MOV instruction to set DR7[any bit 32:63] to 1
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception in real mode
 */
void write_dr7_checking(unsigned long val);
void dr7_to_1(void);

/**
 *@Sub-Conditions:
 *      ECX_MSR_Reserved: true
 *@test purpose:
 *      When executing RDMSR/WRMSR, MSR specified by ECX is reserved
 *@Design Steps:
 *      Execute RDMSR/WRMSR with ECX specify to reserved MSR
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception.
 */
void ecx_msr_reserved_to_true(void);

/**
 *@Sub-Conditions:
 *      ECX_MSR_Unimplement: true
 *@test purpose:
 *      When executing RDMSR/WRMSR, MSR specified by ECX is unimplemented
 *@Design Steps:
 *      Execute RDMSR/WRMSR with ECX specify to unimplement MSR
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception.
 */
void ecx_msr_unimplement_to_true(void);

/**
 *@Sub-Conditions:
 *      EDX:EAX_set_reserved: true
 *@test purpose:
 *      When executing RDMSR/WRMSR, EDX:EAX attempt to set bits
 *      that are reserved in the MSR specified by ECX
 *@Design Steps:
 *      Execute RDMSR/WRMSR with EDX:EAX attempt to set bits
 *      that are reserved in the MSR specified by ECX
 *@Expected Result:
 *      When executing the instruction in next step will generate #GP exception.
 */
void ecx_eax_set_reserved_to_true(void);

/**
 *@Sub-Conditions:
 *      OF_Flag: 1
 *@test purpose:
 *      Set OF flag to 1
 *@Design Steps:
 *      Set EFLAGE.OF to 1
 *@Expected Result:
 *      N/A
 */
bool of_flag_to_1(void);

#endif /* __INSTRUCTION_COMMON_H */

