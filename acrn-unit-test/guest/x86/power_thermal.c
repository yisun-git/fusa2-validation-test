/*
 * Test for x86 power and thermal
 *
 * Copyright (c) intel, 2020
 *
 * Authors:
 *  wenwumax <wenwux.ma@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "msr.h"
#include "vmalloc.h"
#include "alloc.h"
#include "misc.h"

#define IA32_CLOCK_MODULATION	0x19A
#define IA32_MISC_ENABLE	0x1A0
/**
 * @brief Case name: IA32_MISC_ENABLE[16] write_001.
 *
 * Summary: Execute WRMSR instruction to write IA32_MISC_ENABLE[16] with 1H shall generate #GP(0).
 *
 */

void power_rqmid_29021_IA32_MISC_ENABLE16_write_001(void)
{
	u32 index = IA32_MISC_ENABLE;
	u64 val = 0x1 << 16;

	u32 a = val, d = val >> 32;

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		:
		: "a"(a), "d"(d), "c"(index)
		: "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: IA32_CLOCK_MODULATION_001.
 *
 * Summary: Dump register IA32_CLOCK_MODULATION shall generate #GP(0).
 *
 */

void power_rqmid_28033_IA32_CLOCK_MODULATION_001(void)
{
	u32 index = IA32_CLOCK_MODULATION;
	u32 a = 0, d = 0;

	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: IA32_CLOCK_MODULATION_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_CLOCK_MODULATION with 1H shall generate #GP(0).
 *
 */

void power_rqmid_28034_IA32_CLOCK_MODULATION_002(void)
{
	u32 index = IA32_CLOCK_MODULATION;
	u64 val = 0x1;

	u32 a = val, d = val >> 32;

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		:
		: "a"(a), "d"(d), "c"(index)
		: "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: CPUID.6:EAX_001.
 *
 * Summary: Dump  CPUID.06H ,EAX should be 4.
 *
 */

void power_rqmid_29017_CPUID06_EAX_001(void)
{
	struct cpuid cpuid6;
	cpuid6 = cpuid_indexed(6, 0);

	if (cpuid6.a == 4)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}



static void print_case_list(void)
{
	printf("cpumode feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 29021u, "IA32_MISC_ENABLE[16] write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28033u, "IA32_CLOCK_MODULATION_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28034u, "IA32_CLOCK_MODULATION_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29017u, "CPUID.6:EAX_001");
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

	power_rqmid_29021_IA32_MISC_ENABLE16_write_001();
	power_rqmid_28033_IA32_CLOCK_MODULATION_001();
	power_rqmid_28034_IA32_CLOCK_MODULATION_002();
	power_rqmid_29017_CPUID06_EAX_001();

	return report_summary();
}
