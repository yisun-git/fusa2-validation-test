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
#include "power_thermal.h"

/**
 * @brief Case name: IA32_MISC_ENABLE[3] write_001.
 *
 * Summary: Execute WRMSR instruction to write IA32_MISC_ENABLE[3] with 1H shall generate #GP(0).
 *
 */
void power_rqmid_37874_IA32_MISC_ENABLE3_write_001(void)
{
	u32 index = IA32_MISC_ENABLE;
	u64 val = 0x1 << 3;

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
 * @brief Case name: IA32_MISC_ENABLE[38] write_001.
 *
 * Summary: Execute WRMSR instruction to write IA32_MISC_ENABLE[38] with 1H shall generate #GP(0).
 *
 */
void power_rqmid_29022_IA32_MISC_ENABLE38_write_001(void)
{
	u32 index = IA32_MISC_ENABLE;
	u64 offset = 1;
	u64 val = offset << 38;

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
 * Summary: Dump  CPUID.06H, EAX should be 4.
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

/**
 * @brief Case name: CPUID.6:EBX ECX EDX_001.
 *
 * Summary: Dump CPUID.06H, EAX, ECX and EDX should be 0.
 *
 */
void power_rqmid_29018_CPUID06_EBX_ECX_EDX_001(void)
{
	struct cpuid cpuid6;
	cpuid6 = cpuid_indexed(6, 0);

	if ((cpuid6.b == 0) && (cpuid6.c == 0) && (cpuid6.d == 0))
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}

/**
 * @brief Case name: CPUID 1h:ecx[7-8]_001.
 *
 * Summary: Dump CPUID.01H, ECX[7] and ECX[8] should be 0.
 *
 */
void power_rqmid_29019_CPUID01_ECX7_8_001(void)
{
	struct cpuid cpuid1;
	cpuid1 = cpuid_indexed(1, 0);

	if (((cpuid1.c & (1 << 7)) == 0) && ((cpuid1.c & (1 << 8)) == 0))
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}

/**
 * @brief Case name: CPUID 1h:edx[22]_001.
 *
 * Summary: Dump CPUID.01H, EDX[22] should be 0.
 *
 */
void power_rqmid_29581_CPUID01_EDX22_001(void)
{
	struct cpuid cpuid1;
	cpuid1 = cpuid_indexed(1, 0);

	if ((cpuid1.d & (1 << 22)) == 0)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}

/**
 * @brief Case name: CPUID 1h:edx[29]_001.
 *
 * Summary: Dump CPUID.01H, EDX[29] should be 0.
 *
 */
void power_rqmid_29020_CPUID01_EDX29_001(void)
{
	struct cpuid cpuid1;
	cpuid1 = cpuid_indexed(1, 0);

	if ((cpuid1.d & (1 << 29)) == 0)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief Case name: IA32_MISC_ENABLE[3] initial_001.
 *
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 3] at AP entry, the result shall be 0H.
 *
 */
void power_rqmid_29580_IA32_MISC_ENABLE3_INIT_001(void)
{
	u32 val = *((u32 *)AP_IA32_MISC_ENABLE_ADDR1);

	report("\t\t %s", (val & (1 << 3)) == 0, __FUNCTION__);
}

/**
 * @brief Case name: IA32_MISC_ENABLE[16] initial_001.
 *
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 16] at AP entry, the result shall be 0H.
 *
 */
void power_rqmid_29571_IA32_MISC_ENABLE16_INIT_001(void)
{
	u32 val = *((u32 *)AP_IA32_MISC_ENABLE_ADDR1);

	report("\t\t %s", (val & (1 << 16)) == 0, __FUNCTION__);
}

/**
 * @brief Case name: IA32_MISC_ENABLE[38] initial_001.
 *
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 38] at AP entry, the result shall be 0H.
 *
 */
void power_rqmid_29573_IA32_MISC_ENABLE38_INIT_001(void)
{
	u32 val = *((u32 *)AP_IA32_MISC_ENABLE_ADDR2);

	report("\t\t %s", (val & (1 << 16)) == 0, __FUNCTION__);
}
#endif

/**
 * @brief Case name: IA32_MISC_ENABLE[3] startup_001.
 *
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 3] at BP start-up, the result shall be 0H.
 *
 */
void power_rqmid_37875_IA32_MISC_ENABLE3_STARTUP_001(void)
{
	u32 val = *((u32 *)BP_IA32_MISC_ENABLE_ADDR1);

	report("\t\t %s", (val & (1 << 3)) == 0, __FUNCTION__);
}

/**
 * @brief Case name: IA32_MISC_ENABLE[16] startup_001.
 *
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 16] at BP start-up, the result shall be 0H.
 *
 */
void power_rqmid_37876_IA32_MISC_ENABLE16_STARTUP_001(void)
{
	u32 val = *((u32 *)BP_IA32_MISC_ENABLE_ADDR1);

	report("\t\t %s", (val & (1 << 16)) == 0, __FUNCTION__);
}

/**
 * @brief Case name: IA32_MISC_ENABLE[38] startup_001.
 *
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 38] at BP start-up, the result shall be 0H.
 *
 */
void power_rqmid_37877_IA32_MISC_ENABLE38_STARTUP_001(void)
{
	u32 val = *((u32 *)BP_IA32_MISC_ENABLE_ADDR2);

	report("\t\t %s", (val & (1 << 16)) == 0, __FUNCTION__);
}


void power_test_rdmsr(u32 msr, u32 vector, u32 error_code, const char *func)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(msr) : "memory");

	report("\t\t %s", (exception_vector() == vector) && (exception_error_code() == error_code), func);
}

void power_test_wrmsr(u32 msr, u32 vector, u32 error_code, const char *func)
{
	u64 val = 1;
	u32 a = val;
	u32 d = (val >> 32);

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(msr) : "memory");

	report("\t\t %s", (exception_vector() == vector) && (exception_error_code() == error_code), func);
}

/**
 * @brief case name: IA32_APERF_001.
 *
 * Summary: Dump register IA32_APERF shall generate #GP(0).
 */
static void power_rqmid_27750_MSR_IA32_APERF_001(void)
{
	power_test_rdmsr(MSR_IA32_APERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_APERF_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_APERF with 1H shall generate #GP(0).
 */
static void power_rqmid_27751_MSR_IA32_APERF_002(void)
{
	power_test_wrmsr(MSR_IA32_APERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_ENERGY_PERF_BIAS_001.
 *
 * Summary: Dump register IA32_ENERGY_PERF_BIAS shall generate #GP(0).
 */
static void power_rqmid_27756_MSR_IA32_ENERGY_PERF_BIAS_001(void)
{
	power_test_rdmsr(MSR_IA32_ENERGY_PERF_BIAS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_ENERGY_PERF_BIAS_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_ENERGY_PERF_BIAS with 1H shall generate #GP(0).
 */
static void power_rqmid_27757_MSR_IA32_ENERGY_PERF_BIAS_002(void)
{
	power_test_wrmsr(MSR_IA32_ENERGY_PERF_BIAS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_CAPABILITIES_001.
 *
 * Summary: Dump register IA32_HWP_CAPABILITIES shall generate #GP(0).
 */
static void power_rqmid_27851_MSR_IA32_HWP_CAPABILITIES_001(void)
{
	power_test_rdmsr(MSR_IA32_HWP_CAPABILITIES, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_CAPABILITIES_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_HWP_CAPABILITIES with 1H shall generate #GP(0).
 */
static void power_rqmid_27852_MSR_IA32_HWP_CAPABILITIES_002(void)
{
	power_test_wrmsr(MSR_IA32_HWP_CAPABILITIES, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_INTERRUPT_001.
 *
 * Summary: Dump register IA32_HWP_INTERRUPT shall generate #GP(0).
 */
static void power_rqmid_27853_MSR_IA32_HWP_INTERRUPT_001(void)
{
	power_test_rdmsr(MSR_IA32_HWP_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_INTERRUPT_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_HWP_INTERRUPT with 1H shall generate #GP(0).
 */
static void power_rqmid_27854_MSR_IA32_HWP_INTERRUPT_002(void)
{
	power_test_wrmsr(MSR_IA32_HWP_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_REQUEST_001.
 *
 * Summary: Dump register IA32_HWP_REQUEST shall generate #GP(0).
 */
static void power_rqmid_27849_MSR_IA32_HWP_REQUEST_001(void)
{
	power_test_rdmsr(MSR_IA32_HWP_REQUEST, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_REQUEST_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_HWP_REQUEST with 1H shall generate #GP(0).
 */
static void power_rqmid_27850_MSR_IA32_HWP_REQUEST_002(void)
{
	power_test_wrmsr(MSR_IA32_HWP_REQUEST, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_REQUEST_PKG_001.
 *
 * Summary: Dump register IA32_HWP_REQUEST_PKG shall generate #GP(0).
 */
static void power_rqmid_27855_MSR_IA32_HWP_REQUEST_PKG_001(void)
{
	power_test_rdmsr(MSR_IA32_HWP_REQUEST_PKG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_REQUEST_PKG_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_HWP_REQUEST_PKG with 1H shall generate #GP(0).
 */
static void power_rqmid_27856_MSR_IA32_HWP_REQUEST_PKG_002(void)
{
	power_test_wrmsr(MSR_IA32_HWP_REQUEST_PKG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_STATUS_001.
 *
 * Summary: Dump register IA32_HWP_STATUS shall generate #GP(0).
 */
static void power_rqmid_27847_MSR_IA32_HWP_STATUS_001(void)
{
	power_test_rdmsr(MSR_IA32_HWP_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_HWP_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_HWP_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_27848_MSR_IA32_HWP_STATUS_002(void)
{
	power_test_wrmsr(MSR_IA32_HWP_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_MPERF_001.
 *
 * Summary: Dump register IA32_MPERF shall generate #GP(0).
 */
static void power_rqmid_27754_MSR_IA32_MPERF_001(void)
{
	power_test_rdmsr(MSR_IA32_MPERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_MPERF_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_MPERF with 1H shall generate #GP(0).
 */
static void power_rqmid_27755_MSR_IA32_MPERF_002(void)
{
	power_test_wrmsr(MSR_IA32_MPERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PACKAGE_THERM_INTERRUPT_001.
 *
 * Summary: Dump register IA32_PACKAGE_THERM_INTERRUPT shall generate #GP(0).
 */
static void power_rqmid_28037_MSR_IA32_PACKAGE_THERM_INTERRUPT_001(void)
{
	power_test_rdmsr(MSR_IA32_PACKAGE_THERM_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PACKAGE_THERM_INTERRUPT_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PACKAGE_THERM_INTERRUPT with 1H shall generate #GP(0).
 */
static void power_rqmid_28038_MSR_IA32_PACKAGE_THERM_INTERRUPT_002(void)
{
	power_test_wrmsr(MSR_IA32_PACKAGE_THERM_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PACKAGE_THERM_STATUS_001.
 *
 * Summary: Dump register IA32_PACKAGE_THERM_STATUS shall generate #GP(0).
 */
static void power_rqmid_28035_MSR_IA32_PACKAGE_THERM_STATUS_001(void)
{
	power_test_rdmsr(MSR_IA32_PACKAGE_THERM_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PACKAGE_THERM_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PACKAGE_THERM_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28036_MSR_IA32_PACKAGE_THERM_STATUS_002(void)
{
	power_test_wrmsr(MSR_IA32_PACKAGE_THERM_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_CTL_001.
 *
 * Summary: Dump register IA32_PERF_CTL shall generate #GP(0).
 */
static void power_rqmid_27748_MSR_IA32_PERF_CTL_001(void)
{
	power_test_rdmsr(MSR_IA32_PERF_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_CTL_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_CTL with 1H shall generate #GP(0).
 */
static void power_rqmid_27749_MSR_IA32_PERF_CTL_002(void)
{
	power_test_wrmsr(MSR_IA32_PERF_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PKG_HDC_CTL_001.
 *
 * Summary: Dump register IA32_PKG_HDC_CTL shall generate #GP(0).
 */
static void power_rqmid_27901_MSR_IA32_PKG_HDC_CTL_001(void)
{
	power_test_rdmsr(MSR_IA32_PKG_HDC_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PKG_HDC_CTL_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PKG_HDC_CTL with 1H shall generate #GP(0).
 */
static void power_rqmid_27902_MSR_IA32_PKG_HDC_CTL_002(void)
{
	power_test_wrmsr(MSR_IA32_PKG_HDC_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PM_CTL1_001.
 *
 * Summary: Dump register IA32_PM_CTL1 shall generate #GP(0).
 */
static void power_rqmid_27903_MSR_IA32_PM_CTL1_001(void)
{
	power_test_rdmsr(MSR_IA32_PM_CTL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PM_CTL1_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PM_CTL1 with 1H shall generate #GP(0).
 */
static void power_rqmid_27904_MSR_IA32_PM_CTL1_002(void)
{
	power_test_wrmsr(MSR_IA32_PM_CTL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PM_ENABLE_001.
 *
 * Summary: Dump register IA32_PM_ENABLE shall generate #GP(0).
 */
static void power_rqmid_27845_MSR_IA32_PM_ENABLE_001(void)
{
	power_test_rdmsr(MSR_IA32_PM_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PM_ENABLE_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_PM_ENABLE with 1H shall generate #GP(0).
 */
static void power_rqmid_27846_MSR_IA32_PM_ENABLE_002(void)
{
	power_test_wrmsr(MSR_IA32_PM_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THERM_INTERRUPT_001.
 *
 * Summary: Dump register IA32_THERM_INTERRUPT shall generate #GP(0).
 */
static void power_rqmid_28031_MSR_IA32_THERM_INTERRUPT_001(void)
{
	power_test_rdmsr(MSR_IA32_THERM_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THERM_INTERRUPT_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_THERM_INTERRUPT with 1H shall generate #GP(0).
 */
static void power_rqmid_28032_MSR_IA32_THERM_INTERRUPT_002(void)
{
	power_test_wrmsr(MSR_IA32_THERM_INTERRUPT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THERM_STATUS_001.
 *
 * Summary: Dump register IA32_THERM_STATUS shall generate #GP(0).
 */
static void power_rqmid_28029_MSR_IA32_THERM_STATUS_001(void)
{
	power_test_rdmsr(MSR_IA32_THERM_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THERM_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_THERM_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28030_MSR_IA32_THERM_STATUS_002(void)
{
	power_test_wrmsr(MSR_IA32_THERM_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THREAD_STALL_001.
 *
 * Summary: Dump register IA32_THREAD_STALL shall generate #GP(0).
 */
static void power_rqmid_27905_MSR_IA32_THREAD_STALL_001(void)
{
	power_test_rdmsr(MSR_IA32_THREAD_STALL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_THREAD_STALL_002.
 *
 * Summary: Execute WRMSR instruction to write IA32_THREAD_STALL with 1H shall generate #GP(0).
 */
static void power_rqmid_27906_MSR_IA32_THREAD_STALL_002(void)
{
	power_test_wrmsr(MSR_IA32_THREAD_STALL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_ANY_CORE_C0_001.
 *
 * Summary: Dump register MSR_ANY_CORE_C0 shall generate #GP(0).
 */
static void power_rqmid_28159_MSR_ANY_CORE_C0_001(void)
{
	power_test_rdmsr(MSR_ANY_CORE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_ANY_CORE_C0_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_ANY_CORE_C0 with 1H shall generate #GP(0).
 */
static void power_rqmid_28160_MSR_ANY_CORE_C0_002(void)
{
	power_test_wrmsr(MSR_ANY_CORE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_ANY_GFXE_C0_001.
 *
 * Summary: Dump register MSR_ANY_GFXE_C0 shall generate #GP(0).
 */
static void power_rqmid_28138_MSR_ANY_GFXE_C0_001(void)
{
	power_test_rdmsr(MSR_ANY_GFXE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_ANY_GFXE_C0_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_ANY_GFXE_C0 with 1H shall generate #GP(0).
 */
static void power_rqmid_28139_MSR_ANY_GFXE_C0_002(void)
{
	power_test_wrmsr(MSR_ANY_GFXE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_CONTROL_001.
 *
 * Summary: Dump register MSR_CONFIG_TDP_CONTROL shall generate #GP(0).
 */
static void power_rqmid_28345_MSR_CONFIG_TDP_CONTROL_001(void)
{
	power_test_rdmsr(MSR_CONFIG_TDP_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_CONTROL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CONFIG_TDP_CONTROL with 1H shall generate #GP(0).
 */
static void power_rqmid_28346_MSR_CONFIG_TDP_CONTROL_002(void)
{
	power_test_wrmsr(MSR_CONFIG_TDP_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_LEVEL1_001.
 *
 * Summary: Dump register MSR_CONFIG_TDP_LEVEL1 shall generate #GP(0).
 */
static void power_rqmid_28347_MSR_CONFIG_TDP_LEVEL1_001(void)
{
	power_test_rdmsr(MSR_CONFIG_TDP_LEVEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_LEVEL1_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CONFIG_TDP_LEVEL1 with 1H shall generate #GP(0).
 */
static void power_rqmid_28348_MSR_CONFIG_TDP_LEVEL1_002(void)
{
	power_test_wrmsr(MSR_CONFIG_TDP_LEVEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_LEVEL2_001.
 *
 * Summary: Dump register MSR_CONFIG_TDP_LEVEL2 shall generate #GP(0).
 */
static void power_rqmid_28349_MSR_CONFIG_TDP_LEVEL2_001(void)
{
	power_test_rdmsr(MSR_CONFIG_TDP_LEVEL2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_LEVEL2_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CONFIG_TDP_LEVEL2 with 1H shall generate #GP(0).
 */
static void power_rqmid_28350_MSR_CONFIG_TDP_LEVEL2_002(void)
{
	power_test_wrmsr(MSR_CONFIG_TDP_LEVEL2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_NOMINAL_001.
 *
 * Summary: Dump register MSR_CONFIG_TDP_NOMINAL shall generate #GP(0).
 */
static void power_rqmid_28551_MSR_CONFIG_TDP_NOMINAL_001(void)
{
	power_test_rdmsr(MSR_CONFIG_TDP_NOMINAL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CONFIG_TDP_NOMINAL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CONFIG_TDP_NOMINAL with 1H shall generate #GP(0).
 */
static void power_rqmid_28552_MSR_CONFIG_TDP_NOMINAL_002(void)
{
	power_test_wrmsr(MSR_CONFIG_TDP_NOMINAL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C3_RESIDENCY_001.
 *
 * Summary: Dump register MSR_CORE_C3_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28553_MSR_CORE_C3_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_CORE_C3_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C3_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_C3_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28554_MSR_CORE_C3_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_CORE_C3_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C6_RESIDENCY_001.
 *
 * Summary: Dump register MSR_CORE_C6_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28555_MSR_CORE_C6_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_CORE_C6_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C6_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_C6_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28556_MSR_CORE_C6_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_CORE_C6_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C7_RESIDENCY_001.
 *
 * Summary: Dump register MSR_CORE_C7_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28557_MSR_CORE_C7_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_CORE_C7_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_C7_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_C7_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28558_MSR_CORE_C7_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_CORE_C7_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_GFXE_OVERLAP_C0_001.
 *
 * Summary: Dump register MSR_CORE_GFXE_OVERLAP_C0 shall generate #GP(0).
 */
static void power_rqmid_28687_MSR_CORE_GFXE_OVERLAP_C0_001(void)
{
	power_test_rdmsr(MSR_CORE_GFXE_OVERLAP_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_GFXE_OVERLAP_C0_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_GFXE_OVERLAP_C0 with 1H shall generate #GP(0).
 */
static void power_rqmid_28688_MSR_CORE_GFXE_OVERLAP_C0_002(void)
{
	power_test_wrmsr(MSR_CORE_GFXE_OVERLAP_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_HDC_RESIDENCY_001.
 *
 * Summary: Dump register MSR_CORE_HDC_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28690_MSR_CORE_HDC_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_CORE_HDC_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_HDC_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_HDC_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28691_MSR_CORE_HDC_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_CORE_HDC_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_PERF_LIMIT_REASONS_001.
 *
 * Summary: Dump register MSR_CORE_PERF_LIMIT_REASONS shall generate #GP(0).
 */
static void power_rqmid_28758_MSR_CORE_PERF_LIMIT_REASONS_001(void)
{
	power_test_rdmsr(MSR_CORE_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_CORE_PERF_LIMIT_REASONS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_CORE_PERF_LIMIT_REASONS with 1H shall generate #GP(0).
 */
static void power_rqmid_28759_MSR_CORE_PERF_LIMIT_REASONS_002(void)
{
	power_test_wrmsr(MSR_CORE_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_DRAM_ENERGY_STATUS_001.
 *
 * Summary: Dump register MSR_DRAM_ENERGY_STATUS shall generate #GP(0).
 */
static void power_rqmid_28205_MSR_DRAM_ENERGY_STATUS_001(void)
{
	power_test_rdmsr(MSR_DRAM_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_DRAM_ENERGY_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_DRAM_ENERGY_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28206_MSR_DRAM_ENERGY_STATUS_002(void)
{
	power_test_wrmsr(MSR_DRAM_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_DRAM_PERF_STATUS_001.
 *
 * Summary: Dump register MSR_DRAM_PERF_STATUS shall generate #GP(0).
 */
static void power_rqmid_28209_MSR_DRAM_PERF_STATUS_001(void)
{
	power_test_rdmsr(MSR_DRAM_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_DRAM_PERF_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_DRAM_PERF_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28210_MSR_DRAM_PERF_STATUS_002(void)
{
	power_test_wrmsr(MSR_DRAM_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_GRAPHICS_PERF_LIMIT_REASONS_001.
 *
 * Summary: Dump register MSR_GRAPHICS_PERF_LIMIT_REASONS shall generate #GP(0).
 */
static void power_rqmid_28760_MSR_GRAPHICS_PERF_LIMIT_REASONS_001(void)
{
	power_test_rdmsr(MSR_GRAPHICS_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_GRAPHICS_PERF_LIMIT_REASONS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_GRAPHICS_PERF_LIMIT_REASONS with 1H shall generate #GP(0).
 */
static void power_rqmid_28761_MSR_GRAPHICS_PERF_LIMIT_REASONS_002(void)
{
	power_test_wrmsr(MSR_GRAPHICS_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_MISC_PWR_MGMT_001.
 *
 * Summary: Dump register MSR_MISC_PWR_MGMT shall generate #GP(0).
 */
static void power_rqmid_28763_MSR_MISC_PWR_MGMT_001(void)
{
	power_test_rdmsr(MSR_MISC_PWR_MGMT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_MISC_PWR_MGMT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_MISC_PWR_MGMT with 1H shall generate #GP(0).
 */
static void power_rqmid_28764_MSR_MISC_PWR_MGMT_002(void)
{
	power_test_wrmsr(MSR_MISC_PWR_MGMT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PERF_STATUS_001.
 *
 * Summary: Dump register MSR_PERF_STATUS shall generate #GP(0).
 */
static void power_rqmid_28783_MSR_PERF_STATUS_001(void)
{
	power_test_rdmsr(MSR_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PERF_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PERF_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28784_MSR_PERF_STATUS_002(void)
{
	power_test_wrmsr(MSR_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C2_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_C2_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28785_MSR_PKG_C2_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_C2_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C2_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_C2_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28786_MSR_PKG_C2_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_C2_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C3_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_C3_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28819_MSR_PKG_C3_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_C3_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C3_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_C3_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28821_MSR_PKG_C3_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_C3_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C6_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_C6_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28824_MSR_PKG_C6_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_C6_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C6_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_C6_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28825_MSR_PKG_C6_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_C6_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C7_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_C7_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28876_MSR_PKG_C7_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_C7_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_C7_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_C7_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28877_MSR_PKG_C7_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_C7_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_CST_CONFIG_CONTROL_001.
 *
 * Summary: Dump register MSR_PKG_CST_CONFIG_CONTROL shall generate #GP(0).
 */
static void power_rqmid_28947_MSR_PKG_CST_CONFIG_CONTROL_001(void)
{
	power_test_rdmsr(MSR_PKG_CST_CONFIG_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_CST_CONFIG_CONTROL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_CST_CONFIG_CONTROL with 1H shall generate #GP(0).
 */
static void power_rqmid_28948_MSR_PKG_CST_CONFIG_CONTROL_002(void)
{
	power_test_wrmsr(MSR_PKG_CST_CONFIG_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_CONFIG_CTL_001.
 *
 * Summary: Dump register MSR_PKG_HDC_CONFIG_CTL shall generate #GP(0).
 */
static void power_rqmid_28027_MSR_PKG_HDC_CONFIG_CTL_001(void)
{
	power_test_rdmsr(MSR_PKG_HDC_CONFIG_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_CONFIG_CTL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_HDC_CONFIG_CTL with 1H shall generate #GP(0).
 */
static void power_rqmid_28028_MSR_PKG_HDC_CONFIG_CTL_002(void)
{
	power_test_wrmsr(MSR_PKG_HDC_CONFIG_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_DEEP_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_HDC_DEEP_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28025_MSR_PKG_HDC_DEEP_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_HDC_DEEP_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_DEEP_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_HDC_DEEP_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28026_MSR_PKG_HDC_DEEP_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_HDC_DEEP_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_SHALLOW_RESIDENCY_001.
 *
 * Summary: Dump register MSR_PKG_HDC_SHALLOW_RESIDENCY shall generate #GP(0).
 */
static void power_rqmid_28023_MSR_PKG_HDC_SHALLOW_RESIDENCY_001(void)
{
	power_test_rdmsr(MSR_PKG_HDC_SHALLOW_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_HDC_SHALLOW_RESIDENCY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_HDC_SHALLOW_RESIDENCY with 1H shall generate #GP(0).
 */
static void power_rqmid_28024_MSR_PKG_HDC_SHALLOW_RESIDENCY_002(void)
{
	power_test_wrmsr(MSR_PKG_HDC_SHALLOW_RESIDENCY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_PERF_STATUS_001.
 *
 * Summary: Dump register MSR_PKG_PERF_STATUS shall generate #GP(0).
 */
static void power_rqmid_28150_MSR_PKG_PERF_STATUS_001(void)
{
	power_test_rdmsr(MSR_PKG_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_PERF_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_PERF_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28151_MSR_PKG_PERF_STATUS_002(void)
{
	power_test_wrmsr(MSR_PKG_PERF_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_POWER_INFO_001.
 *
 * Summary: Dump register MSR_PKG_POWER_INFO shall generate #GP(0).
 */
static void power_rqmid_28147_MSR_PKG_POWER_INFO_001(void)
{
	power_test_rdmsr(MSR_PKG_POWER_INFO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_POWER_INFO_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_POWER_INFO with 1H shall generate #GP(0).
 */
static void power_rqmid_28149_MSR_PKG_POWER_INFO_002(void)
{
	power_test_wrmsr(MSR_PKG_POWER_INFO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_POWER_LIMIT_001.
 *
 * Summary: Dump register MSR_PKG_POWER_LIMIT shall generate #GP(0).
 */
static void power_rqmid_28041_MSR_PKG_POWER_LIMIT_001(void)
{
	power_test_rdmsr(MSR_PKG_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_POWER_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_POWER_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_28042_MSR_PKG_POWER_LIMIT_002(void)
{
	power_test_wrmsr(MSR_PKG_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC3_IRTL_001.
 *
 * Summary: Dump register MSR_PKGC3_IRTL shall generate #GP(0).
 */
static void power_rqmid_28787_MSR_PKGC3_IRTL_001(void)
{
	power_test_rdmsr(MSR_PKGC3_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC3_IRTL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKGC3_IRTL with 1H shall generate #GP(0).
 */
static void power_rqmid_28788_MSR_PKGC3_IRTL_002(void)
{
	power_test_wrmsr(MSR_PKGC3_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC6_IRTL_001.
 *
 * Summary: Dump register MSR_PKGC6_IRTL shall generate #GP(0).
 */
static void power_rqmid_28822_MSR_PKGC6_IRTL_001(void)
{
	power_test_rdmsr(MSR_PKGC6_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC6_IRTL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKGC6_IRTL with 1H shall generate #GP(0).
 */
static void power_rqmid_28823_MSR_PKGC6_IRTL_002(void)
{
	power_test_wrmsr(MSR_PKGC6_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC7_IRTL_001.
 *
 * Summary: Dump register MSR_PKGC7_IRTL shall generate #GP(0).
 */
static void power_rqmid_28874_MSR_PKGC7_IRTL_001(void)
{
	power_test_rdmsr(MSR_PKGC7_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKGC7_IRTL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKGC7_IRTL with 1H shall generate #GP(0).
 */
static void power_rqmid_28875_MSR_PKGC7_IRTL_002(void)
{
	power_test_wrmsr(MSR_PKGC7_IRTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_ENERGY_STATUS_001.
 *
 * Summary: Dump register MSR_PP0_ENERGY_STATUS shall generate #GP(0).
 */
static void power_rqmid_28154_MSR_PP0_ENERGY_STATUS_001(void)
{
	power_test_rdmsr(MSR_PP0_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_ENERGY_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP0_ENERGY_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28155_MSR_PP0_ENERGY_STATUS_002(void)
{
	power_test_wrmsr(MSR_PP0_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_POLICY_001.
 *
 * Summary: Dump register MSR_PP0_POLICY shall generate #GP(0).
 */
static void power_rqmid_28157_MSR_PP0_POLICY_001(void)
{
	power_test_rdmsr(MSR_PP0_POLICY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_POLICY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP0_POLICY with 1H shall generate #GP(0).
 */
static void power_rqmid_28158_MSR_PP0_POLICY_002(void)
{
	power_test_wrmsr(MSR_PP0_POLICY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_POWER_LIMIT_001.
 *
 * Summary: Dump register MSR_PP0_POWER_LIMIT shall generate #GP(0).
 */
static void power_rqmid_28152_MSR_PP0_POWER_LIMIT_001(void)
{
	power_test_rdmsr(MSR_PP0_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP0_POWER_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP0_POWER_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_28153_MSR_PP0_POWER_LIMIT_002(void)
{
	power_test_wrmsr(MSR_PP0_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_ENERGY_STATUS_001.
 *
 * Summary: Dump register MSR_PP1_ENERGY_STATUS shall generate #GP(0).
 */
static void power_rqmid_28341_MSR_PP1_ENERGY_STATUS_001(void)
{
	power_test_rdmsr(MSR_PP1_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_ENERGY_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP1_ENERGY_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_28342_MSR_PP1_ENERGY_STATUS_002(void)
{
	power_test_wrmsr(MSR_PP1_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_POLICY_001.
 *
 * Summary: Dump register MSR_PP1_POLICY shall generate #GP(0).
 */
static void power_rqmid_28343_MSR_PP1_POLICY_001(void)
{
	power_test_rdmsr(MSR_PP1_POLICY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_POLICY_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP1_POLICY with 1H shall generate #GP(0).
 */
static void power_rqmid_28344_MSR_PP1_POLICY_002(void)
{
	power_test_wrmsr(MSR_PP1_POLICY, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_POWER_LIMIT_001.
 *
 * Summary: Dump register MSR_PP1_POWER_LIMIT shall generate #GP(0).
 */
static void power_rqmid_28211_MSR_PP1_POWER_LIMIT_001(void)
{
	power_test_rdmsr(MSR_PP1_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PP1_POWER_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PP1_POWER_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_28212_MSR_PP1_POWER_LIMIT_002(void)
{
	power_test_wrmsr(MSR_PP1_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PPERF_001.
 *
 * Summary: Dump register MSR_PPERF shall generate #GP(0).
 */
static void power_rqmid_27893_MSR_PPERF_001(void)
{
	power_test_rdmsr(MSR_PPERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PPERF_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PPERF with 1H shall generate #GP(0).
 */
static void power_rqmid_27894_MSR_PPERF_002(void)
{
	power_test_wrmsr(MSR_PPERF, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RAPL_POWER_UNIT_001.
 *
 * Summary: Dump register MSR_RAPL_POWER_UNIT shall generate #GP(0).
 */
static void power_rqmid_28039_MSR_RAPL_POWER_UNIT_001(void)
{
	power_test_rdmsr(MSR_RAPL_POWER_UNIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RAPL_POWER_UNIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_RAPL_POWER_UNIT with 1H shall generate #GP(0).
 */
static void power_rqmid_28040_MSR_RAPL_POWER_UNIT_002(void)
{
	power_test_wrmsr(MSR_RAPL_POWER_UNIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RING_PERF_LIMIT_REASONS_001.
 *
 * Summary: Dump register MSR_RING_PERF_LIMIT_REASONS shall generate #GP(0).
 */
static void power_rqmid_27899_MSR_RING_PERF_LIMIT_REASONS_001(void)
{
	power_test_rdmsr(MSR_RING_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RING_PERF_LIMIT_REASONS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_RING_PERF_LIMIT_REASONS with 1H shall generate #GP(0).
 */
static void power_rqmid_27900_MSR_RING_PERF_LIMIT_REASONS_002(void)
{
	power_test_wrmsr(MSR_RING_PERF_LIMIT_REASONS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RING_RATIO_LIMIT_001.
 *
 * Summary: Dump register MSR_RING_RATIO_LIMIT shall generate #GP(0).
 */
static void power_rqmid_27897_MSR_RING_RATIO_LIMIT_001(void)
{
	power_test_rdmsr(MSR_RING_RATIO_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_RING_RATIO_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_RING_RATIO_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_27898_MSR_RING_RATIO_LIMIT_002(void)
{
	power_test_wrmsr(MSR_RING_RATIO_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TEMPERATURE_TARGET_001.
 *
 * Summary: Dump register MSR_TEMPERATURE_TARGET shall generate #GP(0).
 */
static void power_rqmid_27895_MSR_TEMPERATURE_TARGET_001(void)
{
	power_test_rdmsr(MSR_TEMPERATURE_TARGET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TEMPERATURE_TARGET_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_TEMPERATURE_TARGET with 1H shall generate #GP(0).
 */
static void power_rqmid_27896_MSR_TEMPERATURE_TARGET_002(void)
{
	power_test_wrmsr(MSR_TEMPERATURE_TARGET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TURBO_ACTIVATION_RATIO_001.
 *
 * Summary: Dump register MSR_TURBO_ACTIVATION_RATIO shall generate #GP(0).
 */
static void power_rqmid_27857_MSR_TURBO_ACTIVATION_RATIO_001(void)
{
	power_test_rdmsr(MSR_TURBO_ACTIVATION_RATIO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TURBO_ACTIVATION_RATIO_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_TURBO_ACTIVATION_RATIO with 1H shall generate #GP(0).
 */
static void power_rqmid_27858_MSR_TURBO_ACTIVATION_RATIO_002(void)
{
	power_test_wrmsr(MSR_TURBO_ACTIVATION_RATIO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TURBO_RATIO_LIMIT_001.
 *
 * Summary: Dump register MSR_TURBO_RATIO_LIMIT shall generate #GP(0).
 */
static void power_rqmid_27464_MSR_TURBO_RATIO_LIMIT_001(void)
{
	power_test_rdmsr(MSR_TURBO_RATIO_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_TURBO_RATIO_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_TURBO_RATIO_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_27465_MSR_TURBO_RATIO_LIMIT_002(void)
{
	power_test_wrmsr(MSR_TURBO_RATIO_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_WEIGHTED_CORE_C0_001.
 *
 * Summary: Dump register MSR_WEIGHTED_CORE_C0 shall generate #GP(0).
 */
static void power_rqmid_28207_MSR_WEIGHTED_CORE_C0_001(void)
{
	power_test_rdmsr(MSR_WEIGHTED_CORE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_WEIGHTED_CORE_C0_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_WEIGHTED_CORE_C0 with 1H shall generate #GP(0).
 */
static void power_rqmid_28208_MSR_WEIGHTED_CORE_C0_002(void)
{
	power_test_wrmsr(MSR_WEIGHTED_CORE_C0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PMG_IO_CAPTURE_BASE_001.
 *
 * Summary: Dump register MSR_PMG_IO_CAPTURE_BASE shall generate #GP(0).
 */
static void power_rqmid_28566_MSR_PMG_IO_CAPTURE_BASE_001(void)
{
	power_test_rdmsr(MSR_PMG_IO_CAPTURE_BASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PMG_IO_CAPTURE_BASE_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PMG_IO_CAPTURE_BASE with 1H shall generate #GP(0).
 */
static void power_rqmid_28567_MSR_PMG_IO_CAPTURE_BASE_002(void)
{
	power_test_wrmsr(MSR_PMG_IO_CAPTURE_BASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_POWER_CTL_001.
 *
 * Summary: Dump register MSR_POWER_CTL shall generate #GP(0).
 */
static void power_rqmid_28568_MSR_POWER_CTL_001(void)
{
	power_test_rdmsr(MSR_POWER_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_POWER_CTL_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_POWER_CTL with 1H shall generate #GP(0).
 */
static void power_rqmid_28569_MSR_POWER_CTL_002(void)
{
	power_test_wrmsr(MSR_POWER_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PLATFORM_ENERGY_COUNTER_001.
 *
 * Summary: Dump register MSR_PLATFORM_ENERGY_COUNTER shall generate #GP(0).
 */
static void power_rqmid_29516_MSR_PLATFORM_ENERGY_COUNTER_001(void)
{
	power_test_rdmsr(MSR_PLATFORM_ENERGY_COUNTER, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PLATFORM_ENERGY_COUNTER_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PLATFORM_ENERGY_COUNTER with 1H shall generate #GP(0).
 */
static void power_rqmid_29517_MSR_PLATFORM_ENERGY_COUNTER_002(void)
{
	power_test_wrmsr(MSR_PLATFORM_ENERGY_COUNTER, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PLATFORM_POWER_LIMIT_001.
 *
 * Summary: Dump register MSR_PLATFORM_POWER_LIMIT shall generate #GP(0).
 */
static void power_rqmid_29564_MSR_PLATFORM_POWER_LIMIT_001(void)
{
	power_test_rdmsr(MSR_PLATFORM_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PLATFORM_POWER_LIMIT_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PLATFORM_POWER_LIMIT with 1H shall generate #GP(0).
 */
static void power_rqmid_29565_MSR_PLATFORM_POWER_LIMIT_002(void)
{
	power_test_wrmsr(MSR_PLATFORM_POWER_LIMIT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_ENERGY_STATUS_001.
 *
 * Summary: Dump register MSR_PKG_ENERGY_STATUS shall generate #GP(0).
 */
static void power_rqmid_29513_MSR_PKG_ENERGY_STATUS_001(void)
{
	power_test_rdmsr(MSR_PKG_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PKG_ENERGY_STATUS_002.
 *
 * Summary: Execute WRMSR instruction to write MSR_PKG_ENERGY_STATUS with 1H shall generate #GP(0).
 */
static void power_rqmid_29515_MSR_PKG_ENERGY_STATUS_002(void)
{
	power_test_wrmsr(MSR_PKG_ENERGY_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("power and thermal feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 37874u, "IA32_MISC_ENABLE[3] write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29021u, "IA32_MISC_ENABLE[16] write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29022u, "IA32_MISC_ENABLE[38] write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28033u, "IA32_CLOCK_MODULATION_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28034u, "IA32_CLOCK_MODULATION_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29017u, "CPUID.6:EAX_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29018u, "CPUID.6:EBX ECX EDX_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29019u, "CPUID 1h:ecx[7-8]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29581u, "CPUID 1h:edx[22]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29020u, "CPUID 1h:edx[29]_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 29580u, "IA32_MISC_ENABLE[3] initial_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29571u, "IA32_MISC_ENABLE[16] initial_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29573u, "IA32_MISC_ENABLE[38] initial_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 37875u, "IA32_MISC_ENABLE[3] startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37876u, "IA32_MISC_ENABLE[16] startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37877u, "IA32_MISC_ENABLE[38] startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27750u, "IA32_APERF_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27751u, "IA32_APERF_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27756u, "IA32_ENERGY_PERF_BIAS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27757u, "IA32_ENERGY_PERF_BIAS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27851u, "IA32_HWP_CAPABILITIES_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27852u, "IA32_HWP_CAPABILITIES_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27853u, "IA32_HWP_INTERRUPT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27854u, "IA32_HWP_INTERRUPT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27849u, "IA32_HWP_REQUEST_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27850u, "IA32_HWP_REQUEST_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27855u, "IA32_HWP_REQUEST_PKG_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27856u, "IA32_HWP_REQUEST_PKG_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27847u, "IA32_HWP_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27848u, "IA32_HWP_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27754u, "IA32_MPERF_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27755u, "IA32_MPERF_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28037u, "IA32_PACKAGE_THERM_INTERRUPT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28038u, "IA32_PACKAGE_THERM_INTERRUPT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28035u, "IA32_PACKAGE_THERM_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28036u, "IA32_PACKAGE_THERM_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27748u, "IA32_PERF_CTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27749u, "IA32_PERF_CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27901u, "IA32_PKG_HDC_CTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27902u, "IA32_PKG_HDC_CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27903u, "IA32_PM_CTL1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27904u, "IA32_PM_CTL1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27845u, "IA32_PM_ENABLE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27846u, "IA32_PM_ENABLE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28031u, "IA32_THERM_INTERRUPT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28032u, "IA32_THERM_INTERRUPT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28029u, "IA32_THERM_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28030u, "IA32_THERM_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27905u, "IA32_THREAD_STALL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27906u, "IA32_THREAD_STALL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28159u, "MSR_ANY_CORE_C0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28160u, "MSR_ANY_CORE_C0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28138u, "MSR_ANY_GFXE_C0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28139u, "MSR_ANY_GFXE_C0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28345u, "MSR_CONFIG_TDP_CONTROL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28346u, "MSR_CONFIG_TDP_CONTROL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28347u, "MSR_CONFIG_TDP_LEVEL1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28348u, "MSR_CONFIG_TDP_LEVEL1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28349u, "MSR_CONFIG_TDP_LEVEL2_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28350u, "MSR_CONFIG_TDP_LEVEL2_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28551u, "MSR_CONFIG_TDP_NOMINAL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28552u, "MSR_CONFIG_TDP_NOMINAL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28553u, "MSR_CORE_C3_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28554u, "MSR_CORE_C3_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28555u, "MSR_CORE_C6_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28556u, "MSR_CORE_C6_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28557u, "MSR_CORE_C7_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28558u, "MSR_CORE_C7_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28687u, "MSR_CORE_GFXE_OVERLAP_C0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28688u, "MSR_CORE_GFXE_OVERLAP_C0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28690u, "MSR_CORE_HDC_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28691u, "MSR_CORE_HDC_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28758u, "MSR_CORE_PERF_LIMIT_REASONS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28759u, "MSR_CORE_PERF_LIMIT_REASONS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28205u, "MSR_DRAM_ENERGY_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28206u, "MSR_DRAM_ENERGY_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28209u, "MSR_DRAM_PERF_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28210u, "MSR_DRAM_PERF_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28760u, "MSR_GRAPHICS_PERF_LIMIT_REASONS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28761u, "MSR_GRAPHICS_PERF_LIMIT_REASONS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28763u, "MSR_MISC_PWR_MGMT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28764u, "MSR_MISC_PWR_MGMT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28783u, "MSR_PERF_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28784u, "MSR_PERF_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28785u, "MSR_PKG_C2_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28786u, "MSR_PKG_C2_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28819u, "MSR_PKG_C3_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28821u, "MSR_PKG_C3_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28824u, "MSR_PKG_C6_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28825u, "MSR_PKG_C6_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28876u, "MSR_PKG_C7_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28877u, "MSR_PKG_C7_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28947u, "MSR_PKG_CST_CONFIG_CONTROL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28948u, "MSR_PKG_CST_CONFIG_CONTROL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28027u, "MSR_PKG_HDC_CONFIG_CTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28028u, "MSR_PKG_HDC_CONFIG_CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28025u, "MSR_PKG_HDC_DEEP_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28026u, "MSR_PKG_HDC_DEEP_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28023u, "MSR_PKG_HDC_SHALLOW_RESIDENCY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28024u, "MSR_PKG_HDC_SHALLOW_RESIDENCY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28150u, "MSR_PKG_PERF_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28151u, "MSR_PKG_PERF_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28147u, "MSR_PKG_POWER_INFO_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28149u, "MSR_PKG_POWER_INFO_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28041u, "MSR_PKG_POWER_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28042u, "MSR_PKG_POWER_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28787u, "MSR_PKGC3_IRTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28788u, "MSR_PKGC3_IRTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28822u, "MSR_PKGC6_IRTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28823u, "MSR_PKGC6_IRTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28874u, "MSR_PKGC7_IRTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28875u, "MSR_PKGC7_IRTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28154u, "MSR_PP0_ENERGY_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28155u, "MSR_PP0_ENERGY_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28157u, "MSR_PP0_POLICY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28158u, "MSR_PP0_POLICY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28152u, "MSR_PP0_POWER_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28153u, "MSR_PP0_POWER_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28341u, "MSR_PP1_ENERGY_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28342u, "MSR_PP1_ENERGY_STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28343u, "MSR_PP1_POLICY_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28344u, "MSR_PP1_POLICY_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28211u, "MSR_PP1_POWER_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28212u, "MSR_PP1_POWER_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27893u, "MSR_PPERF_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27894u, "MSR_PPERF_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28039u, "MSR_RAPL_POWER_UNIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28040u, "MSR_RAPL_POWER_UNIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27899u, "MSR_RING_PERF_LIMIT_REASONS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27900u, "MSR_RING_PERF_LIMIT_REASONS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27897u, "MSR_RING_RATIO_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27898u, "MSR_RING_RATIO_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27895u, "MSR_TEMPERATURE_TARGET_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27896u, "MSR_TEMPERATURE_TARGET_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27857u, "MSR_TURBO_ACTIVATION_RATIO_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27858u, "MSR_TURBO_ACTIVATION_RATIO_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27464u, "MSR_TURBO_RATIO_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27465u, "MSR_TURBO_RATIO_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28207u, "MSR_WEIGHTED_CORE_C0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28208u, "MSR_WEIGHTED_CORE_C0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29566u, "MSR_PMG_IO_CAPTURE_BASE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29567u, "MSR_PMG_IO_CAPTURE_BASE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29568u, "MSR_POWER_CTL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29569u, "MSR_POWER_CTL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29516u, "MSR_PLATFORM_ENERGY_COUNTER_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29517u, "MSR_PLATFORM_ENERGY_COUNTER_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29564u, "MSR_PLATFORM_POWER_LIMIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29565u, "MSR_PLATFORM_POWER_LIMIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29513u, "MSR_PKG_ENERGY_STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29515u, "MSR_PKG_ENERGY_STATUS_002");
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

	power_rqmid_37874_IA32_MISC_ENABLE3_write_001();
	power_rqmid_29021_IA32_MISC_ENABLE16_write_001();
	power_rqmid_29022_IA32_MISC_ENABLE38_write_001();
	power_rqmid_28033_IA32_CLOCK_MODULATION_001();
	power_rqmid_28034_IA32_CLOCK_MODULATION_002();
	power_rqmid_29017_CPUID06_EAX_001();
	power_rqmid_29018_CPUID06_EBX_ECX_EDX_001();
	power_rqmid_29019_CPUID01_ECX7_8_001();
	power_rqmid_29581_CPUID01_EDX22_001();
	power_rqmid_29020_CPUID01_EDX29_001();
#ifdef IN_NON_SAFETY_VM
	power_rqmid_29580_IA32_MISC_ENABLE3_INIT_001();
	power_rqmid_29571_IA32_MISC_ENABLE16_INIT_001();
	power_rqmid_29573_IA32_MISC_ENABLE38_INIT_001();
#endif
	power_rqmid_37875_IA32_MISC_ENABLE3_STARTUP_001();
	power_rqmid_37876_IA32_MISC_ENABLE16_STARTUP_001();
	power_rqmid_37877_IA32_MISC_ENABLE38_STARTUP_001();

	power_rqmid_27750_MSR_IA32_APERF_001();
	power_rqmid_27751_MSR_IA32_APERF_002();
	power_rqmid_27756_MSR_IA32_ENERGY_PERF_BIAS_001();
	power_rqmid_27757_MSR_IA32_ENERGY_PERF_BIAS_002();
	power_rqmid_27851_MSR_IA32_HWP_CAPABILITIES_001();
	power_rqmid_27852_MSR_IA32_HWP_CAPABILITIES_002();
	power_rqmid_27853_MSR_IA32_HWP_INTERRUPT_001();
	power_rqmid_27854_MSR_IA32_HWP_INTERRUPT_002();
	power_rqmid_27849_MSR_IA32_HWP_REQUEST_001();
	power_rqmid_27850_MSR_IA32_HWP_REQUEST_002();
	power_rqmid_27855_MSR_IA32_HWP_REQUEST_PKG_001();
	power_rqmid_27856_MSR_IA32_HWP_REQUEST_PKG_002();
	power_rqmid_27847_MSR_IA32_HWP_STATUS_001();
	power_rqmid_27848_MSR_IA32_HWP_STATUS_002();
	power_rqmid_27754_MSR_IA32_MPERF_001();
	power_rqmid_27755_MSR_IA32_MPERF_002();
	power_rqmid_28037_MSR_IA32_PACKAGE_THERM_INTERRUPT_001();
	power_rqmid_28038_MSR_IA32_PACKAGE_THERM_INTERRUPT_002();
	power_rqmid_28035_MSR_IA32_PACKAGE_THERM_STATUS_001();
	power_rqmid_28036_MSR_IA32_PACKAGE_THERM_STATUS_002();
	power_rqmid_27748_MSR_IA32_PERF_CTL_001();
	power_rqmid_27749_MSR_IA32_PERF_CTL_002();
	power_rqmid_27901_MSR_IA32_PKG_HDC_CTL_001();
	power_rqmid_27902_MSR_IA32_PKG_HDC_CTL_002();
	power_rqmid_27903_MSR_IA32_PM_CTL1_001();
	power_rqmid_27904_MSR_IA32_PM_CTL1_002();
	power_rqmid_27845_MSR_IA32_PM_ENABLE_001();
	power_rqmid_27846_MSR_IA32_PM_ENABLE_002();
	power_rqmid_28031_MSR_IA32_THERM_INTERRUPT_001();
	power_rqmid_28032_MSR_IA32_THERM_INTERRUPT_002();
	power_rqmid_28029_MSR_IA32_THERM_STATUS_001();
	power_rqmid_28030_MSR_IA32_THERM_STATUS_002();
	power_rqmid_27905_MSR_IA32_THREAD_STALL_001();
	power_rqmid_27906_MSR_IA32_THREAD_STALL_002();
	power_rqmid_28159_MSR_ANY_CORE_C0_001();
	power_rqmid_28160_MSR_ANY_CORE_C0_002();
	power_rqmid_28138_MSR_ANY_GFXE_C0_001();
	power_rqmid_28139_MSR_ANY_GFXE_C0_002();
	power_rqmid_28345_MSR_CONFIG_TDP_CONTROL_001();
	power_rqmid_28346_MSR_CONFIG_TDP_CONTROL_002();
	power_rqmid_28347_MSR_CONFIG_TDP_LEVEL1_001();
	power_rqmid_28348_MSR_CONFIG_TDP_LEVEL1_002();
	power_rqmid_28349_MSR_CONFIG_TDP_LEVEL2_001();
	power_rqmid_28350_MSR_CONFIG_TDP_LEVEL2_002();
	power_rqmid_28551_MSR_CONFIG_TDP_NOMINAL_001();
	power_rqmid_28552_MSR_CONFIG_TDP_NOMINAL_002();
	power_rqmid_28553_MSR_CORE_C3_RESIDENCY_001();
	power_rqmid_28554_MSR_CORE_C3_RESIDENCY_002();
	power_rqmid_28555_MSR_CORE_C6_RESIDENCY_001();
	power_rqmid_28556_MSR_CORE_C6_RESIDENCY_002();
	power_rqmid_28557_MSR_CORE_C7_RESIDENCY_001();
	power_rqmid_28558_MSR_CORE_C7_RESIDENCY_002();
	power_rqmid_28687_MSR_CORE_GFXE_OVERLAP_C0_001();
	power_rqmid_28688_MSR_CORE_GFXE_OVERLAP_C0_002();
	power_rqmid_28690_MSR_CORE_HDC_RESIDENCY_001();
	power_rqmid_28691_MSR_CORE_HDC_RESIDENCY_002();
	power_rqmid_28758_MSR_CORE_PERF_LIMIT_REASONS_001();
	power_rqmid_28759_MSR_CORE_PERF_LIMIT_REASONS_002();
	power_rqmid_28205_MSR_DRAM_ENERGY_STATUS_001();
	power_rqmid_28206_MSR_DRAM_ENERGY_STATUS_002();
	power_rqmid_28209_MSR_DRAM_PERF_STATUS_001();
	power_rqmid_28210_MSR_DRAM_PERF_STATUS_002();
	power_rqmid_28760_MSR_GRAPHICS_PERF_LIMIT_REASONS_001();
	power_rqmid_28761_MSR_GRAPHICS_PERF_LIMIT_REASONS_002();
	power_rqmid_28763_MSR_MISC_PWR_MGMT_001();
	power_rqmid_28764_MSR_MISC_PWR_MGMT_002();
	power_rqmid_28783_MSR_PERF_STATUS_001();
	power_rqmid_28784_MSR_PERF_STATUS_002();
	power_rqmid_28785_MSR_PKG_C2_RESIDENCY_001();
	power_rqmid_28786_MSR_PKG_C2_RESIDENCY_002();
	power_rqmid_28819_MSR_PKG_C3_RESIDENCY_001();
	power_rqmid_28821_MSR_PKG_C3_RESIDENCY_002();
	power_rqmid_28824_MSR_PKG_C6_RESIDENCY_001();
	power_rqmid_28825_MSR_PKG_C6_RESIDENCY_002();
	power_rqmid_28876_MSR_PKG_C7_RESIDENCY_001();
	power_rqmid_28877_MSR_PKG_C7_RESIDENCY_002();
	power_rqmid_28947_MSR_PKG_CST_CONFIG_CONTROL_001();
	power_rqmid_28948_MSR_PKG_CST_CONFIG_CONTROL_002();
	power_rqmid_28027_MSR_PKG_HDC_CONFIG_CTL_001();
	power_rqmid_28028_MSR_PKG_HDC_CONFIG_CTL_002();
	power_rqmid_28025_MSR_PKG_HDC_DEEP_RESIDENCY_001();
	power_rqmid_28026_MSR_PKG_HDC_DEEP_RESIDENCY_002();
	power_rqmid_28023_MSR_PKG_HDC_SHALLOW_RESIDENCY_001();
	power_rqmid_28024_MSR_PKG_HDC_SHALLOW_RESIDENCY_002();
	power_rqmid_28150_MSR_PKG_PERF_STATUS_001();
	power_rqmid_28151_MSR_PKG_PERF_STATUS_002();
	power_rqmid_28147_MSR_PKG_POWER_INFO_001();
	power_rqmid_28149_MSR_PKG_POWER_INFO_002();
	power_rqmid_28041_MSR_PKG_POWER_LIMIT_001();
	power_rqmid_28042_MSR_PKG_POWER_LIMIT_002();
	power_rqmid_28787_MSR_PKGC3_IRTL_001();
	power_rqmid_28788_MSR_PKGC3_IRTL_002();
	power_rqmid_28822_MSR_PKGC6_IRTL_001();
	power_rqmid_28823_MSR_PKGC6_IRTL_002();
	power_rqmid_28874_MSR_PKGC7_IRTL_001();
	power_rqmid_28875_MSR_PKGC7_IRTL_002();
	power_rqmid_28154_MSR_PP0_ENERGY_STATUS_001();
	power_rqmid_28155_MSR_PP0_ENERGY_STATUS_002();
	power_rqmid_28157_MSR_PP0_POLICY_001();
	power_rqmid_28158_MSR_PP0_POLICY_002();
	power_rqmid_28152_MSR_PP0_POWER_LIMIT_001();
	power_rqmid_28153_MSR_PP0_POWER_LIMIT_002();
	power_rqmid_28341_MSR_PP1_ENERGY_STATUS_001();
	power_rqmid_28342_MSR_PP1_ENERGY_STATUS_002();
	power_rqmid_28343_MSR_PP1_POLICY_001();
	power_rqmid_28344_MSR_PP1_POLICY_002();
	power_rqmid_28211_MSR_PP1_POWER_LIMIT_001();
	power_rqmid_28212_MSR_PP1_POWER_LIMIT_002();
	power_rqmid_27893_MSR_PPERF_001();
	power_rqmid_27894_MSR_PPERF_002();
	power_rqmid_28039_MSR_RAPL_POWER_UNIT_001();
	power_rqmid_28040_MSR_RAPL_POWER_UNIT_002();
	power_rqmid_27899_MSR_RING_PERF_LIMIT_REASONS_001();
	power_rqmid_27900_MSR_RING_PERF_LIMIT_REASONS_002();
	power_rqmid_27897_MSR_RING_RATIO_LIMIT_001();
	power_rqmid_27898_MSR_RING_RATIO_LIMIT_002();
	power_rqmid_27895_MSR_TEMPERATURE_TARGET_001();
	power_rqmid_27896_MSR_TEMPERATURE_TARGET_002();
	power_rqmid_27857_MSR_TURBO_ACTIVATION_RATIO_001();
	power_rqmid_27858_MSR_TURBO_ACTIVATION_RATIO_002();
	power_rqmid_27464_MSR_TURBO_RATIO_LIMIT_001();
	power_rqmid_27465_MSR_TURBO_RATIO_LIMIT_002();
	power_rqmid_28207_MSR_WEIGHTED_CORE_C0_001();
	power_rqmid_28208_MSR_WEIGHTED_CORE_C0_002();
	power_rqmid_28566_MSR_PMG_IO_CAPTURE_BASE_001();
	power_rqmid_28567_MSR_PMG_IO_CAPTURE_BASE_002();
	power_rqmid_28568_MSR_POWER_CTL_001();
	power_rqmid_28569_MSR_POWER_CTL_002();
	power_rqmid_29516_MSR_PLATFORM_ENERGY_COUNTER_001();
	power_rqmid_29517_MSR_PLATFORM_ENERGY_COUNTER_002();
	power_rqmid_29564_MSR_PLATFORM_POWER_LIMIT_001();
	power_rqmid_29565_MSR_PLATFORM_POWER_LIMIT_002();
	power_rqmid_29513_MSR_PKG_ENERGY_STATUS_001();
	power_rqmid_29515_MSR_PKG_ENERGY_STATUS_002();

	return report_summary();
}
