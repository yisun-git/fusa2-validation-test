#ifndef __CONDITION_H
#define __CONDITION_H

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
/* -------------------------common instruction(cpuid, cr*) end----------------------------- */

/*----------------------------- ring1 ring2 -----------------------------*/
#define SEGMENT_LIMIT						0xFFFF
#define SEGMENT_PRESENT_SET					0x80
#define SEGMENT_PRESENT_CLEAR					0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_0				0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_1				0x20
#define DESCRIPTOR_PRIVILEGE_LEVEL_2				0x40
#define DESCRIPTOR_PRIVILEGE_LEVEL_3				0x60
#define DESCRIPTOR_TYPE_SYS					0x00
#define DESCRIPTOR_TYPE_CODE_OR_DATA				0x10

#define SEGMENT_TYPE_DATE_READ_ONLY				0x0
#define SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED			0x1
#define SEGMENT_TYPE_DATE_READ_WRITE                            0x2
#define SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED                   0x3
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN                    0x4
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN_ACCESSED           0x5
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN                   0x6
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN_ACCESSED          0x7

#define SEGMENT_TYPE_CODE_EXE_ONLY                              0x8
#define SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED                     0x9
#define SEGMENT_TYPE_CODE_EXE_RAED                              0xA
#define SEGMENT_TYPE_CODE_EXE_RAED_ACCESSED                     0xB
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING                   0xC
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING_ACCESSED          0xD
#define SEGMENT_TYPE_CODE_EXE_READ_CONFORMING                   0xE
#define SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED     0xF

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_LDT               0x2
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TASKGATE          0x5
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_CALLGATE          0x0C
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TSSG              0xB

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE         0xE
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE          0xF

#define GRANULARITY_SET						0x80
#define GRANULARITY_clear                                       0x00
#define DEFAULT_OPERATION_SIZE_16BIT_SEGMENT                    0x00
#define DEFAULT_OPERATION_SIZE_32BIT_SEGMENT                    0x40
#define L_64_BIT_CODE_SEGMENT                                   0x20
#define AVL0                                                    0x0

#define DPLEVEL1                                                0x20
#define DPLEVEL2                                                0x40

#define SELECTOR_TI                                             0x4

#define CPUID_1_ECX_FMA				(1 << 12)
#define CPUID_1_ECX_XSAVE			(1 << 26)
#define CPUID_1_ECX_OSXSAVE			(1 << 27)
#define CPUID_1_ECX_AVX				(1 << 28)
#define CPUID_1_ECX_F16C			(1 << 29)

#define X86_CR0_EM				(1 << 2)

#define X86_CR4_OSXSAVE				(1 << 18)
#define X86_CR4_OSFXSR				(1 << 9)
#define X86_CR4_OSXMMEXCPT			(1 << 10)

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
 * CPUID.(DH:1):EAX
 * Processor Extended State Enumeration Sub-leaf (EAX = 0DH, ECX = 1)
 */
#define SUPPORT_XSAVEOPT_BIT		0
#define SUPPORT_XSAVEC_BIT		1
#define SUPPORT_XGETBV_BIT		2
#define SUPPORT_XSAVES_XRTORS_BIT	3
#define SUPPORT_XSAVEOPT		(1 << SUPPORT_XSAVEOPT_BIT)
#define SUPPORT_XSAVEC			(1 << SUPPORT_XSAVEC_BIT)
#define SUPPORT_XGETBV			(1 << SUPPORT_XGETBV_BIT)
#define SUPPORT_XSAVES_XRTORS		(1 << SUPPORT_XSAVES_XRTORS_BIT)

#define SUPPORT_XCR0			0x7

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
#define SUPPORT_AVX512ER_BIT		27
#define SUPPORT_AVX512CD_BIT		28
#define SUPPORT_AVX512VL_BIT		31
#define SUPPORT_AVX2			(1 << SUPPORT_AVX2_BIT)
#define SUPPORT_AVX512F			(1 << SUPPORT_AVX512F_BIT)
#define SUPPORT_AVX512ER		(1 << SUPPORT_AVX512ER_BIT)
#define SUPPORT_AVX512CD		(1 << SUPPORT_AVX512CD_BIT)
#define SUPPORT_AVX512VL		(1 << SUPPORT_AVX512VL_BIT)

#define XSAVE_AREA_MIN_SIZE (512+64)
#define XSAVE_AREA_MAX_SIZE 0x440
#define XSAVE_AREA_SIZE 200
#define SUPPORT_INIT_OPTIMIZATION 1
#define IA32_XSS_ADDR	0xDA0
#define XMM_REGISTER_SIZE	16*16
#define XSAVE_BV_LOCATION 512
#define YMM0_LOCATION 576

#define XCR0_MASK       0x00000000
#define CR0_AM_MASK	(1UL << 18)
#define X86_EFLAGS_AC	0x00040000    // (1UL << 18)

#define xstr(s...) xxstr(s)
#define xxstr(s...) #s

#define ADDR_ALIGN(x, align) (((x) + ((align) - 1U)) & (~((align) - 1U)))
#define ALIGNED(x) __attribute__((aligned(x)))

typedef unsigned __attribute__((vector_size(16))) unsigned_128_bit;
typedef unsigned __attribute__((vector_size(32))) unsigned_256_bit;
typedef union {
	unsigned_128_bit sse ALIGNED(16);
	unsigned m[4] ALIGNED(16);
} union_unsigned_128;

typedef union {
	unsigned_256_bit avx;
	unsigned m[8];
} union_unsigned_256;


#define DS_SEL		0xffff
#define NULL_SEL	0x0000

/* Exception */
#define DE_VECTOR 0
#define DB_VECTOR 1
#define NMI_VECTOR 2
#define BP_VECTOR 3
#define OF_VECTOR 4
#define BR_VECTOR 5
#define UD_VECTOR 6
#define NM_VECTOR 7
#define DF_VECTOR 8
#define TS_VECTOR 10
#define NP_VECTOR 11
#define SS_VECTOR 12
#define GP_VECTOR 13
#define PF_VECTOR 14
#define MF_VECTOR 16
#define AC_VECTOR 17
#define MC_VECTOR 18
#define XM_VECTOR 19


/**Some common function **/

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

void condition_CPUID_AVX_1(void);
void condition_CPUID_AVX2_1(void);
void condition_CPUID_F16C_1(void);
void condition_CPUID_FMA_1(void);
void condition_LOCK_not_used(void);
void condition_VEX_used(void);
bool condition_st_comp_enabled(void);
int condition_write_cr4_checking(unsigned long val);
bool condition_CR4_OSXSAVE_1(void);
void condition_CR4_OSXSAVE_0(void);
void condition_st_comp_not_enabled(void);
void condition_Inv_AVX2_hold(void);
void condition_VEX_4v_1s_not_hold(void);
void condition_VEX_4v_1s_hold(void);
void condition_Inv_Prf_not_hold(void);
void condition_Inv_Prf_hold(void);
void condition_pgfault_not_occur(void);
void condition_pgfault_occur(void);
void condition_Ad_Cann_non_stack(void);
void condition_Ad_Cann_non_mem(void);
void condition_Ad_Cann_cann(void);
void condition_unmasked_SIMD_hold(void);
void condition_unmasked_SIMD_not_hold(void);
void condition_CR4_OSXMMEXCPT_0(void);
void condition_CR4_OSXMMEXCPT_1(void);
void condition_CR0_TS_1(void);
void condition_CR0_TS_0(void);
void condition_CR0_NE_1(void);
void condition_cs_cpl_1(void);
void condition_cs_cpl_2(void);
void condition_cs_cpl_3(void);
void condition_CR0_AC_1(void);
void condition_CR0_AC_0(void);
void condition_Alignment_aligned(void);
void condition_Alignment_unaligned(void);
void condition_CR0_PG_0(void);
void condition_CR0_PG_1(void);
void CR4_R_W_1_write_cr4_checking(unsigned long val);
void condition_CR4_R_W_1(void);
void condition_CR4_PAE_0(void);
void condition_CR4_PAE_1(void);
void condition_CR4_PCIDE_UP_true(void);
void condition_CR4_PCIDE_UP_false(void);
void DR6_1_write_dr6_checking(unsigned long val);
void condition_DR6_1(void);
//void DR7_1_write_dr7_checking(unsigned long val);
void condition_DR7_1(void);
void condition_DR7_GD_0(void);
void condition_DR7_GD_1(void);
void condition_CSLoad_true(void);
void condition_CSLoad_false(void);
void condition_REXR_hold(void);
void condition_CR8_not_used(void);
void condition_REXR_not_hold(void);
void condition_CR8_used(void);
void condition_CR4_R_W_0(void);
void condition_D_segfault_not_occur(void);
void condition_AccessCR1567_true(void);
void condition_AccessCR1567_false(void);
void condition_WCR0Invalid_true(void);
void condition_WCR0Invalid_false(void);
void condition_CR3NOT0_true(void);
void condition_CR3NOT0_false(void);
void condition_DR6_0(void);
void condition_DR7_0(void);
void condition_DeAc_true(void);
void condition_DeAc_false(void);
void condition_Mem_Dest_hold(void);
void condition_CMPXCHG16B_Aligned_false(void);
void condition_CMPXCHG16B_Aligned_true(void);
void condition_MemLocat_false(void);
void condition_MemLocat_true(void);
void condition_Push_Seg_hold(void);
void condition_Push_Seg_not_hold(void);
void condition_Descriptor_limit_inside(void);
void condition_Descriptor_limit_outside(void);
void condition_Divisor_0_true(void);
void condition_Divisor_0_false(void);
void condition_Quot_large_true(void);
void condition_Quot_large_false(void);
void condition_JMP_Abs_address_true(void);
void condition_JMP_Abs_address_false(void);
void condition_CALL_Abs_address_true(void);
void condition_CALL_Abs_address_false(void);
void condition_D_segfault_occur(void);
void condition_StackPushAlignment_aligned(void);
void condition_StackPushAlignment_unaligned(void);
void condition_cs_dpl_0(void);
void condition_cs_dpl_1(void);
void condition_cs_dpl_2(void);
void condition_DesOutIDT_true(void);
void condition_DesOutIDT_false(void);
void condition_CPUID_LAHF_SAHF_0(void);
void condition_S_segfault_occur(void);
void condition_RPL_CPL_true(void);
void condition_RPL_CPL_false(void);
void condition_S_segfault_not_occur(void);
void condition_Sou_Not_mem_location_true(void);
void condition_Sou_Not_mem_location_false(void);
void CR8_R_W_1_write_cr8_checking(unsigned long val);
void condition_CR8_R_W_1(void);
void condition_CR8_R_W_0(void);
void condition_LOCK_used(void);
void condition_CR3_R_W_1(void);
void condition_CR3_R_W_0(void);
void condition_CPUID_CMPXCHG16B_set_to_1(void);
void condition_EFLAGS_NT_1(void);
void condition_EFLAGS_NT_0(void);
void condition_CPUID_LAHF_SAHF_1(void);
void condition_EFLAGS_AC_set_to_1(void);
void condition_EFLAGS_AC_set_to_0(void);
void condition_CR0_AM_1(void);
void condition_Inv_Prt_not_hold(void);
void condition_Seg_Limit_SSeg(void);
void condition_Seg_Limit_DSeg(void);
void condition_Seg_Limit_No(void);
void condition_VEX_not_used(void);
void condition_CPUID_SSE2_1(void);
void condition_CPUID_SSE2_0(void);
void condition_CR0_EM_0(void);
void condition_CR0_EM_1(void);
void condition_CR4_OSFXSR_1(void);
void condition_CR4_OSFXSR_0(void);
void condition_CPUID_SSE_1(void);
void condition_CPUID_SSE_0(void);
void condition_CPUID_SSE3_1(void);
void condition_CPUID_SSE4_1_1(void);
void condition_CPUID_SSSE3_1(void);
void condition_CPUID_SSSE3_0(void);
void condition_FPU_excp_hold(void);
void condition_FPU_excp_not_hold(void);
void condition_CPUID_SSE4_2_1(void);
void condition_mem_descriptor_nc_hold(void);
void condition_mem_descriptor_nc_not_hold(void);
void condition_offset_operand_nc_hold(void);
void condition_offset_operand_nc_not_hold(void);
void condition_inst_nc_hold(void);
void condition_inst_nc_not_hold(void);
void condition_offset_jump_nc_hold(void);
void condition_offset_jump_nc_not_hold(void);
void condition_descriptor_selector_nc_hold(void);
void condition_descriptor_selector_nc_not_hold(void);
void condition_complex_nc_hold(void);
void condition_complex_nc_not_hold(void);
void condition_Inv_AVX2_not_hold(void);
void condition_CR4_PCIDE_1(void);
void condition_CR4_PCIDE_0(void);
void condition_PDPT_RB_1(void);
void condition_PDPT_RB_0(void);
void condition_PDPTLoad_true(void);
void condition_PDPTLoad_false(void);
void condition_Mem_Dest_not_hold(void);
void condition_Sign_Quot_Large_true(void);
void condition_Sign_Quot_Large_false(void);
void condition_Bound_test_fail(void);
void condition_Bound_test_pass(void);
void condition_Sec_Not_mem_location_true(void);
void condition_Sec_Not_mem_location_false(void);
void condition_CR4_UMIP_0(void);
void condition_CR4_UMIP_1(void);
void condition_write_prot_not_hold(void);
void condition_write_prot_hold(void);
void condition_Prefix_67H_used(void);
void condition_Prefix_67H_not_used(void);
void condition_CS_D_1(void);
void condition_CS_D_0(void);
void condition_RIP_relative_hold(void);
void condition_RIP_relative_not_hold(void);
void condition_BNDCFGS_EN_1(void);
void condition_BNDCFGS_EN_0(void);
void condition_BND4_15_used(void);
void condition_BND4_15_not_used(void);
void condition_Bound_Lower(void);
void condition_Bound_Upper(void);
void do_at_ring0(void (*fn)(const char *), const char *arg);
void condition_imm8_0_hold(void);
void condition_imm8_0_not_hold(void);
void condition_cs_dpl_3(void);
void condition_ECX_MSR_Reserved_true(void);
void condition_ECX_MSR_Reserved_false(void);
void condition_ECX_MSR_Unimplement_true(void);
void condition_register_nc_hold(void);
void condition_register_nc_not_hold(void);
void condition_EDX_EAX_set_reserved_true(void);
void condition_EDX_EAX_set_reserved_false(void);
void condition_ECX_MSR_Unimplement_false(void);
void condition_CPUID_SSE3_0(void);
void condition_CPUID_SSE4_1_0(void);
void condition_CPUID_SSE4_2_0(void);
void condition_CPUID_MMX_0(void);
void condition_CR0_MP_1(void);
void condition_CR0_MP_0(void);
void condition_CPUID_RDRAND_0(void);
void condition_CPUID_RDRAND_1(void);
void condition_CPUID_RDSEED_0(void);
void condition_CPUID_RDSEED_1(void);
void condition_F2HorF3H_hold(void);
void condition_F2HorF3H_not_hold(void);
void condition_OpcodeExcp_false(void);
void condition_OpcodeExcp_true(void);

#endif /* __CONDITION_H */
