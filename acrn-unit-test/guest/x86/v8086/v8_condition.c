#include "v8_condition.h"

int do_at_ring3(void (*fn)(const char *), const char *arg)
{
	fn(arg);
	return 0;
}

void do_at_ring0(void (*fn)(const char *), const char *arg)
{
	fn(arg);
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

void condition_CR4_UMIP_1(void)
{
	//set CR4 UMIP to 1 will cause #GP.
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
	unsigned long check_bit = 0;

	//printf("***** Set CR0.AM[bit 18] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr0(check_bit);
}

void condition_CR0_AC_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR0.AM[bit 18] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr0(check_bit);
}

void condition_ESP_SP_hold(void)
{
}

void condition_CPUID_SSE3_1(void)
{
}

void condition_LOCK_not_used(void)
{
}

void condition_VEX_not_used(void)
{
}

void condition_EFLAGS_AC_set_to_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_rflags(check_bit);
}

void condition_EFLAGS_AC_set_to_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_rflags(check_bit);
}

void condition_Alignment_aligned(void)
{
	condition_EFLAGS_AC_set_to_0();
}

void condition_CR0_EM_0(void)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02)));
	write_cr0(check_bit);
}

void condition_CR4_OSFXSR_1(void)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09));
	write_cr4(check_bit);
}

void condition_CPUID_SSE2_1(void)
{
}

void condition_CPUID_SSE4_1_1(void)
{
}

void condition_CPUID_SSSE3_1(void)
{
}

void condition_CPUID_SSE_1(void)
{
}

void condition_CPUID_SSE4_2_1(void)
{
}

void condition_Alignment_unaligned(void)
{
	condition_EFLAGS_AC_set_to_1();
}