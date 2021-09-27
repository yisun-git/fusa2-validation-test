asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"

void setup_idt(void)
{
}

void setup_vm(void)
{
}

void setup_ring_env(void)
{
}

void do_at_ring0(void (*fn)(const char *), const char *arg)
{
	fn(arg);
}

void condition_LOCK_not_used(void)
{
}

void condition_LOCK_used(void)
{
}

void condition_ECX_MSR_Reserved_false(void)
{
}

void condition_ECX_MSR_Reserved_true(void)
{
}

void condition_D_segfault_not_occur(void)
{
}

void condition_D_segfault_occur(void)
{
}

void condition_Bound_test_pass(void)
{
}

void condition_Bound_test_fail(void)
{
}

void condition_Sec_Not_mem_location_true(void)
{
}


void condition_NumOutIDT_false(void)
{
}

void condition_NumOutIDT_true(void)
{
}

void condition_exceed_64K_not_hold(void)
{
}

void condition_exceed_64K_hold(void)
{
}

void condition_Divisor_0_false(void)
{
}

void condition_Divisor_0_true(void)
{
}

void condition_Sign_Quot_Large_false(void)
{
}

void condition_Sign_Quot_Large_true(void)
{
}

void condition_ESP_SP_not_hold(void)
{
}

void condition_ESP_SP_hold(void)
{
}

void condition_MemLocat_true(void)
{
}

void condition_MemLocat_false(void)
{
}

void condition_CR4_R_W_1(void)
{
}

void condition_CR4_R_W_0(void)
{
}

void condition_CR4_PCIDE_1(void)
{
}

void condition_CR4_PCIDE_0(void)
{
}

void condition_AccessCR1567_true(void)
{
}

void condition_AccessCR1567_false(void)
{
}

void condition_WCR0Invalid_true(void)
{
}

void condition_WCR0Invalid_false(void)
{
}

void condition_DeAc_true(void)
{
}

void condition_DR7_GD_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_dr7();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)));

	write_dr7(check_bit);
}

void condition_DR7_GD_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_dr7();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));

	write_dr7(check_bit);
}

void condition_Mem_Dest_hold(void)
{
}

void condition_Quot_large_true(void)
{
}

void condition_Quot_large_false(void)
{
}

void condition_Sec_Not_mem_location_false(void)
{
}

void condition_Sou_Not_mem_location_false(void)
{
}

void condition_ECX_MSR_Unimplement_true(void)
{
}

void condition_ECX_MSR_Unimplement_false(void)
{
}

void condition_Sou_Not_mem_location_true(void)
{
}

void condition_register_nc_hold(void)
{
}

void condition_register_nc_not_hold(void)
{
}

void condition_EDX_EAX_set_reserved_true(void)
{
}

void condition_EDX_EAX_set_reserved_false(void)
{
}

void condition_imm8_0_hold(void)
{
}

void condition_imm8_0_not_hold(void)
{
}


void condition_CPUID_RDSEED_1(void)
{
}

void condition_F2HorF3H_hold(void)
{
}

void condition_F2HorF3H_not_hold(void)
{
}

void condition_OpcodeExcp_false(void)
{
}

void condition_OpcodeExcp_true(void)
{
}

void condition_CPUID_RDRAND_1(void)
{
}

void condition_Mem_Dest_not_hold(void)
{
}


void condition_CR0_EM_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02)));
	write_cr0(check_bit);
}

void condition_CR0_EM_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02));

	write_cr0(check_bit);
}

void condition_CR0_TS_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR0.TS[bit 3] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03));

	write_cr0(check_bit);
}

void condition_CR0_TS_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR0.TS[bit 3] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03)));

	write_cr0(check_bit);
}

void condition_CR0_NE_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR0.NE[bit 5] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr0(check_bit);
}

void condition_CR0_NE_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr0(check_bit);
}


void condition_FPU_excp_hold(void)
{
	float op1 = 1.2;
	int op2 = 0;
	unsigned short cw;

	cw = 0x37b;/*bit2==0,report div0 exception*/
	asm volatile("fninit;fldcw %0;fld %1\n"
			 "fdiv %2\n"
			 : : "m"(cw), "m"(op1), "m"(op2));
}

void condition_FPU_excp_not_hold(void)
{
}

void condition_CR0_MP_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01));

	write_cr0(check_bit);

}

void condition_CR0_MP_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01));

	write_cr0(check_bit);
}

void condition_CPUID_MMX_1(void)
{
	unsigned long check_bit = 0;
	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_23);
}

void condition_CPUID_MMX_0(void)
{
}

void condition_VEX_used(void)
{
}

void condition_CPUID_FMA_1(void)
{
	unsigned long check_bit = 0;
	//printf("***** Check CPUID.(EAX=01H):ECX[bit 12] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12);
}

void condition_CPUID_F16C_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Check CPUID.(EAX=01H):ECX[bit 29] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29);
}

void condition_CPUID_AVX2_1(void)
{
	unsigned long check_bit = 0;

	//printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 5] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05);
}

#define CHECK_CPUID_0(func, index, reg, bit, msg) \
	if (0 != (cpuid_indexed((func), (index)).reg & (1 << (bit)))) { \
		report("%s: Ignore this case, because the processor supports " msg, \
			1, __func__); \
		return; \
	} \
	printf("%s: the processor does not supports " msg, __func__)

#define CHECK_CPUID_1(func, index, reg, bit, msg) \
	if (0 == (cpuid_indexed((func), (index)).reg & (1 << (bit)))) { \
		report("%s: The processor does not support " msg, \
			0, __func__); \
		return; \
	} \
	printf("%s: CPUID = 0x%x", __func__, cpuid_indexed((func), (index)).reg)

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
#define condition_CPUID_AVX_0() \
	CHECK_CPUID_0(0x01, 0, c, 28, "AVX instruction extensions.")

#define condition_CPUID_AVX_1() \
	CHECK_CPUID_1(0x01, 0, c, 28, "AVX instruction extensions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
#define condition_CPUID_AVX2_0() \
	CHECK_CPUID_0(0x07, 0, b, 5, "AVX2 instruction extensions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 29]
#define condition_CPUID_F16C_0() \
	CHECK_CPUID_0(0x01, 0, c, 29, "16-bit floating-point conversion instructions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 12]
#define condition_CPUID_FMA_0() \
	CHECK_CPUID_0(0x01, 0, c, 12, "FMA extensions using YMM state.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 0]
#define condition_CPUID_SSE3_0() \
	CHECK_CPUID_0(0x01, 0, c, 0, "Streaming SIMD Extensions 3 (SSE3).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 19]
#define condition_CPUID_SSE4_1_0() \
	CHECK_CPUID_0(0x01, 0, c, 19, "Streaming SIMD Extensions 4.1 (SSE4.1).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 20]
#define condition_CPUID_SSE4_2_0() \
	CHECK_CPUID_0(0x01, 0, c, 20, "Streaming SIMD Extensions 4.2 (SSE4.2).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
#define condition_CPUID_RDSEED_0() \
	CHECK_CPUID_0(0x07, 0, b, 18, "RDSEED instruction.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX.RDRAND[bit 30]
#define condition_CPUID_RDRAND_0() \
	CHECK_CPUID_0(0x01, 0, c, 30, "RDRAND instruction.")

void condition_CPUID_SSE2_1(void)
{
}

void condition_VEX_not_used(void)
{
}

void condition_Alignment_aligned(void)
{
}

void condition_CR4_OSFXSR_1(void)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09));
	write_cr4(check_bit);
}

void condition_unmasked_SIMD_hold(void)
{
	 u32 mxcsr = 0x1f;
	 asm volatile("ldmxcsr %0" : : "m"(mxcsr));
}

void condition_CR4_OSXMMEXCPT_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR4.OSXMMEXCPT[bit 10] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10)));

	write_cr4(check_bit);
}

void condition_CR4_OSXMMEXCPT_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR4.OSXMMEXCPT[bit 10] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10));

	write_cr4(check_bit);
}

void condition_unmasked_SIMD_not_hold(void)
{
}

void condition_CPUID_SSE_1(void)
{
}

void condition_CPUID_SSE3_1(void)
{
}

void condition_CPUID_SSE4_1_1(void)
{
}

void condition_CPUID_SSE4_2_1(void)
{
}

void condition_CPUID_SSSE3_1(void)
{
}

void condition_CPUID_SSE2_0(void)
{
}

void condition_Alignment_unaligned(void)
{
}

void condition_CR4_OSFXSR_0(void)
{
}

void condition_CPUID_SSE_0(void)
{
}

void condition_CPUID_SSSE3_0(void)
{
}

void condition_Bound_Lower(void)
{
}

void condition_Bound_Upper(void)
{
}

void condition_16b_addressing_hold(void)
{
}

void condition_16b_addressing_not_hold(void)
{
}

void condition_BNDCFGS_EN_1(void)
{
}

void condition_BND4_15_used(void)
{
}

void condition_BNDCFGS_EN_0(void)
{
}

void condition_BND4_15_not_used(void)
{
}

void condition_INTO_EXE_true(void)
{
}

void condition_OF_Flag_1(void)
{
	unsigned long flag = 0;
	flag = read_rflags();
	flag |= 1U << 11;
	write_rflags(flag);
	flag = read_rflags();
}

void condition_DeReLoadRead_false(void)
{
}

int set_UMIP(int set)
{
	#define CPUID_7_ECX_UMIP (1 << 2)
	#define X86_CR4_UMIP   0x00000800

	int cpuid_7_ecx;
	u32 cr4 = 0;

	cpuid_7_ecx = cpuid(7).c;
	if (!(cpuid_7_ecx & CPUID_7_ECX_UMIP)) {
		return 0;
	}

	cr4 = read_cr4();
	if (set) {
		cr4 |= X86_CR4_UMIP;
	} else {
		cr4 &= ~X86_CR4_UMIP;
	}

	write_cr4(cr4);

	return 1;
}

void condition_CR4_UMIP_1(void)
{
	//set CR4 UMIP to 1 will cause #GP.
	set_UMIP(1);
}

void condition_pgfault_occur(void)
{
}

void condition_pgfault_not_occur(void)
{
}

void condition_cs_cpl_3(void)
{
}

void condition_CR0_AC_1(void)
{
}

int do_at_ring3(void (*fn)(const char *), const char *arg)
{
	fn(arg);
	return 0;
}
