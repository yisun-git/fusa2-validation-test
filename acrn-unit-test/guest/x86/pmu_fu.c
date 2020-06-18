#include "libcflat.h"
#include "desc.h"
#include "processor.h"

#include "pmu_fu.h"

void pmu_test_rdmsr(u32 msr, u32 vector, u32 error_code, const char *func)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(msr) : "memory");

	report("%s", (exception_vector() == vector) && (exception_error_code() == error_code), func);
}

void pmu_test_wrmsr(u32 msr, u32 vector, u32 error_code, const char *func)
{
	u64 val = 1;
	u32 a = val;
	u32 d = (val >> 32);

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(msr) : "memory");

	report("%s", (exception_vector() == vector) && (exception_error_code() == error_code), func);
}

#ifdef IN_NATIVE

/**
 * @brief case name: PMU_physical_MSR_PLATFORM_INFO_AC_001
 *
 * Summary: Under 64 bit mode, Check physical MSR_PLATFORM_INFO.
 */
static void pmu_rqmid_36505_PMU_physical_MSR_PLATFORM_INFO_AC_001(void)
{
	u32 a, d;
	u32 msr = MSR_PLATFORM_INFO;
	asm volatile (
		"rdmsr\n\t"
		: "=a"(a), "=d"(d) : "c"(msr) : "memory");

	report("%s", true, __FUNCTION__);
}

#else

/**
 * @brief case name:When a vCPU attempts to read CPUID.0AH_001
 *
 * Summary: When a vCPU attempts to read CPUID.0AH, ACRN hypervisor shall
 *          write 0H to guest EAX, guest EBX, guest ECX, guest EDX
 */
static void pmu_rqmid_27001_cpuid_a(void)
{
	u32 res = 0;

	res |= cpuid(0xa).a;
	res |= cpuid(0xa).b;
	res |= cpuid(0xa).c;
	res |= cpuid(0xa).d;
	report("%s", (res == 0), __FUNCTION__);
}


/**
 * @brief case name:RDPMC_001
 *
 * Summary: Test RDPMC instruction execution should cause #UD when PMU is hidden.
 */
static void pmu_rqmid_27844_rdpmc(void)
{
	asm volatile(ASM_TRY("1f")
		"rdpmc \n\t"
		"1:":::);

	report("%s", exception_vector() == UD_VECTOR, __FUNCTION__);

}

/**
 * @brief case name:IA32_FIXED_CTR_CTRL_001
 *
 * Summary: When vCPU attempts to read guest IA32_FIXED_CTR_CTRL,
 *          ACRN hypervisor shall guarantee that the vCPU will receive #GP(0)
 */
static void pmu_rqmid_27141_rd_ia32_fixed_ctr_ctrl(void)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(MSR_CORE_PERF_FIXED_CTR_CTRL) : "memory");

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:IA32_FIXED_CTR_CTRL_002
 *
 * Summary: When vCPU attempts to write guest IA32_FIXED_CTR_CTRL,
 *          ACRN hypervisor shall guarantee that the vCPU will receive #GP(0)
 */
static void pmu_rqmid_27142_wt_ia32_fixed_ctr_ctrl(void)
{
	u64 val = 1;
	u32 a = val;
	u32 d = (val >> 32);

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(MSR_CORE_PERF_FIXED_CTR_CTRL) : "memory");

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:cr4_bit8_startup_001
 *
 * Summary: ACRN hypervisor shall set the initial cr4.VMXE[8] of guest VM to 0 following start-up
 *
 */

static void pmu_rqmid_29693_cr4_bit8_startup_001()
{
	u32 cr4 = *(u32 *)0x8000;

	report("%s", ((cr4 & (1 << 8)) == 0), __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_001
 *
 * Summary: Dump register IA32_A_PMC0 shall generate #GP(0).
 */
static void pmu_rqmid_27114_rd_IA32_A_PMC0_7_001(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_002
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27115_wt_IA32_A_PMC0_7_002(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_003
 *
 * Summary: Dump register IA32_A_PMC1 shall generate #GP(0).
 */
static void pmu_rqmid_27116_rd_IA32_A_PMC0_7_003(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_004
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27117_wt_IA32_A_PMC0_7_004(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_005
 *
 * Summary: Dump register IA32_A_PMC2 shall generate #GP(0).
 */
static void pmu_rqmid_27118_rd_IA32_A_PMC0_7_005(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_006
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC2 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27119_wt_IA32_A_PMC0_7_006(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_007
 *
 * Summary: Dump register IA32_A_PMC3 shall generate #GP(0).
 */
static void pmu_rqmid_27120_rd_IA32_A_PMC0_7_007(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_008
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC3 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27121_wt_IA32_A_PMC0_7_008(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_009
 *
 * Summary: Dump register IA32_A_PMC4 shall generate #GP(0).
 */
static void pmu_rqmid_27122_rd_IA32_A_PMC0_7_009(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC4, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_010
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC4 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27123_wt_IA32_A_PMC0_7_010(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC4, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_011
 *
 * Summary: Dump register IA32_A_PMC5 shall generate #GP(0).
 */
static void pmu_rqmid_27124_rd_IA32_A_PMC0_7_011(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC5, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_012
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC5 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27125_wt_IA32_A_PMC0_7_012(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC5, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_013
 *
 * Summary: Dump register IA32_A_PMC6 shall generate #GP(0).
 */
static void pmu_rqmid_27126_rd_IA32_A_PMC0_7_013(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC6, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_014
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC6 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27127_wt_IA32_A_PMC0_7_014(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC6, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_015
 *
 * Summary: Dump register IA32_A_PMC7 shall generate #GP(0).
 */
static void pmu_rqmid_27128_rd_IA32_A_PMC0_7_015(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC7, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_A_PMC0-7_016
 *
 * Summary: Execute WRMSR instruction to write IA32_A_PMC7 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27129_wt_IA32_A_PMC0_7_016(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC7, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_001
 *
 * Summary: Dump register IA32_FIXED_CRT0 shall generate #GP(0).
 */
static void pmu_rqmid_27134_rd_IA32_FIXED_CTR0_2_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_FIXED_CTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_002
 *
 * Summary: Execute WRMSR instruction to write IA32_FIXED_CRT0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27135_wt_IA32_FIXED_CTR0_2_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_FIXED_CTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_003
 *
 * Summary: Dump register IA32_FIXED_CRT1 shall generate #GP(0).
 */
static void pmu_rqmid_27136_rd_IA32_FIXED_CTR0_2_003(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_FIXED_CTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_004
 *
 * Summary: Execute WRMSR instruction to write IA32_FIXED_CRT1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27138_wt_IA32_FIXED_CTR0_2_004(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_FIXED_CTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_005
 *
 * Summary: Dump register IA32_FIXED_CRT2 shall generate #GP(0).
 */
static void pmu_rqmid_27139_rd_IA32_FIXED_CTR0_2_005(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_FIXED_CTR2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_FIXED_CTR0-2_006
 *
 * Summary: Execute WRMSR instruction to write IA32_FIXED_CRT2 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27140_wt_IA32_FIXED_CTR0_2_006(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_FIXED_CTR2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PEBS_ENABLE_001
 *
 * Summary: Dump register IA32_PEBS_ENABLE shall generate #GP(0).
 */
static void pmu_rqmid_27199_rd_IA32_PEBS_ENABLE_001(void)
{
	pmu_test_rdmsr(MSR_IA32_PEBS_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PEBS_ENABLE_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PEBS_ENABLE with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27200_wt_IA32_PEBS_ENABLE_002(void)
{
	pmu_test_wrmsr(MSR_IA32_PEBS_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_CAPABILITIES_001
 *
 * Summary: Dump register IA32_PERF_CAPABILITIES shall generate #GP(0).
 */
static void pmu_rqmid_27205_rd_IA32_PERF_CAPABILITIES_001(void)
{
	pmu_test_rdmsr(MSR_IA32_PERF_CAPABILITIES, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_CAPABILITIES_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_CAPABILITIES with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27206_wt_IA32_PERF_CAPABILITIES_002(void)
{
	pmu_test_wrmsr(MSR_IA32_PERF_CAPABILITIES, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_CTRL_001
 *
 * Summary: Dump register IA32_PERF_GLOBAL_CTRL shall generate #GP(0).
 */
static void pmu_rqmid_27150_rd_IA32_PERF_GLOBAL_CTRL_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_GLOBAL_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_CTRL_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_GLOBAL_CTRL with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27151_wt_IA32_PERF_GLOBAL_CTRL_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_GLOBAL_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_INUSE_001
 *
 * Summary: Dump register IA32_PERF_GLOBAL_INUSE shall generate #GP(0).
 */
static void pmu_rqmid_27193_rd_IA32_PERF_GLOBAL_INUSE_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_GLOBAL_INUSE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_INUSE_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_GLOBAL_INUSE with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27195_wt_IA32_PERF_GLOBAL_INUSE_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_GLOBAL_INUSE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_001
 *
 * Summary: Dump register IA32_PERF_GLOBAL_STATUS shall generate #GP(0).
 */
static void pmu_rqmid_27146_rd_IA32_PERF_GLOBAL_STATUS_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_GLOBAL_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_GLOBAL_STATUS with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27147_wt_IA32_PERF_GLOBAL_STATUS_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_GLOBAL_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_SET_001
 *
 * Summary: Dump register IA32_PERF_GLOBAL_STATUS_SET shall generate #GP(0).
 */
static void pmu_rqmid_27191_rd_IA32_PERF_GLOBAL_STATUS_SET_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_GLOBAL_STATUS_SET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_SET_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_GLOBAL_STATUS_SET with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27192_wt_IA32_PERF_GLOBAL_STATUS_SET_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_GLOBAL_STATUS_SET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_RESET_001
 *
 * Summary: Dump register IA32_PERF_GLOBAL_STATUS_RESET shall generate #GP(0).
 */
static void pmu_rqmid_27148_rd_IA32_PERF_GLOBAL_STATUS_RESET_001(void)
{
	pmu_test_rdmsr(MSR_CORE_PERF_GLOBAL_STATUS_RESET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERF_GLOBAL_STATUS_RESET_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERF_GLOBAL_STATUS_RESET with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27149_wt_IA32_PERF_GLOBAL_STATUS_RESET_002(void)
{
	pmu_test_wrmsr(MSR_CORE_PERF_GLOBAL_STATUS_RESET, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_001
 *
 * Summary: Dump register IA32_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_27002_rd_IA32_PERFEVTSEL0_3_001(void)
{
	pmu_test_rdmsr(MSR_IA32_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PERFEVTSEL0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27003_wt_IA32_PERFEVTSEL0_3_002(void)
{
	pmu_test_wrmsr(MSR_IA32_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_003
 *
 * Summary: Dump register IA32_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_27004_rd_IA32_PERFEVTSEL0_3_003(void)
{
	pmu_test_rdmsr(MSR_IA32_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_004
 *
 * Summary: Execute WRMSR instruction to write IA32_PERFEVTSEL1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27005_wt_IA32_PERFEVTSEL0_3_004(void)
{
	pmu_test_wrmsr(MSR_IA32_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_005
 *
 * Summary: Dump register IA32_PERFEVTSEL2 shall generate #GP(0).
 */
static void pmu_rqmid_27006_rd_IA32_PERFEVTSEL0_3_005(void)
{
	pmu_test_rdmsr(MSR_IA32_PERFEVTSEL2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_006
 *
 * Summary: Execute WRMSR instruction to write IA32_PERFEVTSEL2 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27007_wt_IA32_PERFEVTSEL0_3_006(void)
{
	pmu_test_wrmsr(MSR_IA32_PERFEVTSEL2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_007
 *
 * Summary: Dump register IA32_PERFEVTSEL3 shall generate #GP(0).
 */
static void pmu_rqmid_27008_rd_IA32_PERFEVTSEL0_3_007(void)
{
	pmu_test_rdmsr(MSR_IA32_PERFEVTSEL3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PERFEVTSEL0-3_008
 *
 * Summary: Execute WRMSR instruction to write IA32_PERFEVTSEL3 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27009_wt_IA32_PERFEVTSEL0_3_008(void)
{
	pmu_test_wrmsr(MSR_IA32_PERFEVTSEL3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_001
 *
 * Summary: Dump register IA32_PMC0 shall generate #GP(0).
 */
static void pmu_rqmid_27097_rd_IA32_PMC0_7_001(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_002
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27098_wt_IA32_PMC0_7_002(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_003
 *
 * Summary: Dump register IA32_PMC1 shall generate #GP(0).
 */
static void pmu_rqmid_27099_rd_IA32_PMC0_7_003(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_004
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27100_wt_IA32_PMC0_7_004(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_005
 *
 * Summary: Dump register IA32_PMC2 shall generate #GP(0).
 */
static void pmu_rqmid_27101_rd_IA32_PMC0_7_005(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_006
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC2 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27102_wt_IA32_PMC0_7_006(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC2, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_007
 *
 * Summary: Dump register IA32_PMC3 shall generate #GP(0).
 */
static void pmu_rqmid_27103_rd_IA32_PMC0_7_007(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_008
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC3 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27104_wt_IA32_PMC0_7_008(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC3, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_009
 *
 * Summary: Dump register IA32_PMC4 shall generate #GP(0).
 */
static void pmu_rqmid_27105_rd_IA32_PMC0_7_009(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC4, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_010
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC4 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27106_wt_IA32_PMC0_7_010(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC4, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_011
 *
 * Summary: Dump register IA32_PMC5 shall generate #GP(0).
 */
static void pmu_rqmid_27107_rd_IA32_PMC0_7_011(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC5, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_012
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC5 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27108_wt_IA32_PMC0_7_012(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC5, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_013
 *
 * Summary: Dump register IA32_PMC6 shall generate #GP(0).
 */
static void pmu_rqmid_27109_rd_IA32_PMC0_7_013(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC6, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_014
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC6 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27110_wt_IA32_PMC0_7_014(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC6, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_015
 *
 * Summary: Dump register IA32_PMC7 shall generate #GP(0).
 */
static void pmu_rqmid_27112_rd_IA32_PMC0_7_015(void)
{
	pmu_test_rdmsr(MSR_IA32_PMC7, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: IA32_PMC0-7_016
 *
 * Summary: Execute WRMSR instruction to write IA32_PMC7 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27113_wt_IA32_PMC0_7_016(void)
{
	pmu_test_wrmsr(MSR_IA32_PMC7, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_OFFCORE_RSP_0-1_001
 *
 * Summary: Dump register MSR_OFFCORE_RSP_0 shall generate #GP(0).
 */
static void pmu_rqmid_27201_rd_MSR_OFFCORE_RSP_0_1_001(void)
{
	pmu_test_rdmsr(MSR_OFFCORE_RSP_0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_OFFCORE_RSP_0-1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_OFFCORE_RSP_0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27202_wt_MSR_OFFCORE_RSP_0_1_002(void)
{
	pmu_test_wrmsr(MSR_OFFCORE_RSP_0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_OFFCORE_RSP_0-1_003
 *
 * Summary: Dump register MSR_OFFCORE_RSP_1 shall generate #GP(0).
 */
static void pmu_rqmid_27203_rd_MSR_OFFCORE_RSP_0_1_003(void)
{
	pmu_test_rdmsr(MSR_OFFCORE_RSP_1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_OFFCORE_RSP_0-1_004
 *
 * Summary: Execute WRMSR instruction to write MSR_OFFCORE_RSP_1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27204_wt_MSR_OFFCORE_RSP_0_1_004(void)
{
	pmu_test_wrmsr(MSR_OFFCORE_RSP_1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_FRONTEND_001
 *
 * Summary: Dump register MSR_PEBS_FRONTEND shall generate #GP(0).
 */
static void pmu_rqmid_27196_rd_MSR_PEBS_FRONTEND_001(void)
{
	pmu_test_rdmsr(MSR_PEBS_FRONTEND, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_FRONTEND_002
 *
 * Summary: Execute WRMSR instruction to write MSR_PEBS_FRONTEND with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27197_wt_MSR_PEBS_FRONTEND_002(void)
{
	pmu_test_wrmsr(MSR_PEBS_FRONTEND, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFCTR0_001
 *
 * Summary: Dump register MSR_UNC_CBO_0_PERFCTR0 shall generate #GP(0).
 */
static void pmu_rqmid_27835_rd_MSR_UNC_CBO_0_PERFCTR0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_0_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFCTR0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_0_PERFCTR0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27836_wt_MSR_UNC_CBO_0_PERFCTR0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_0_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFCTR0_001
 *
 * Summary: Dump register MSR_UNC_CBO_1_PERFCTR0 shall generate #GP(0).
 */
static void pmu_rqmid_27826_rd_MSR_UNC_CBO_1_PERFCTR0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_1_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFCTR0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_1_PERFCTR0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27827_wt_MSR_UNC_CBO_1_PERFCTR0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_1_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFCTR0_001
 *
 * Summary: Dump register MSR_UNC_CBO_2_PERFCTR0 shall generate #GP(0).
 */
static void pmu_rqmid_27405_rd_MSR_UNC_CBO_2_PERFCTR0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_2_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFCTR0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_2_PERFCTR0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27406_wt_MSR_UNC_CBO_2_PERFCTR0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_2_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFCTR0_001
 *
 * Summary: Dump register MSR_UNC_CBO_3_PERFCTR0 shall generate #GP(0).
 */
static void pmu_rqmid_27271_rd_MSR_UNC_CBO_3_PERFCTR0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_3_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFCTR0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_3_PERFCTR0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27272_wt_MSR_UNC_CBO_3_PERFCTR0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_3_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFCTR1_001
 *
 * Summary: Dump register MSR_UNC_CBO_0_PERFCTR1 shall generate #GP(0).
 */
static void pmu_rqmid_27837_rd_MSR_UNC_CBO_0_PERFCTR1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_0_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFCTR1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_0_PERFCTR1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27838_wt_MSR_UNC_CBO_0_PERFCTR1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_0_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFCTR1_001
 *
 * Summary: Dump register MSR_UNC_CBO_1_PERFCTR1 shall generate #GP(0).
 */
static void pmu_rqmid_27828_rd_MSR_UNC_CBO_1_PERFCTR1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_1_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFCTR1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_1_PERFCTR1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27829_wt_MSR_UNC_CBO_1_PERFCTR1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_1_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFCTR1_001
 *
 * Summary: Dump register MSR_UNC_CBO_2_PERFCTR1 shall generate #GP(0).
 */
static void pmu_rqmid_27458_rd_MSR_UNC_CBO_2_PERFCTR1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_2_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFCTR1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_2_PERFCTR1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27459_wt_MSR_UNC_CBO_2_PERFCTR1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_2_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFCTR1_001
 *
 * Summary: Dump register MSR_UNC_CBO_3_PERFCTR1 shall generate #GP(0).
 */
static void pmu_rqmid_27273_rd_MSR_UNC_CBO_3_PERFCTR1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_3_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFCTR1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_3_PERFCTR1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27274_wt_MSR_UNC_CBO_3_PERFCTR1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_3_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFEVTSEL0_001
 *
 * Summary: Dump register MSR_UNC_CBO_0_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_27831_rd_MSR_UNC_CBO_0_PERFEVTSEL0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_0_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFEVTSEL0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_0_PERFEVTSEL0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27832_wt_MSR_UNC_CBO_0_PERFEVTSEL0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_0_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFEVTSEL0_001
 *
 * Summary: Dump register MSR_UNC_CBO_1_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_27839_rd_MSR_UNC_CBO_1_PERFEVTSEL0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_1_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFEVTSEL0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_1_PERFEVTSEL0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27840_wt_MSR_UNC_CBO_1_PERFEVTSEL0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_1_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFEVTSEL0_001
 *
 * Summary: Dump register MSR_UNC_CBO_2_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_27460_rd_MSR_UNC_CBO_2_PERFEVTSEL0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_2_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFEVTSEL0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_2_PERFEVTSEL0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27461_wt_MSR_UNC_CBO_2_PERFEVTSEL0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_2_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFEVTSEL0_001
 *
 * Summary: Dump register MSR_UNC_CBO_3_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_27275_rd_MSR_UNC_CBO_3_PERFEVTSEL0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_3_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFEVTSEL0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_3_PERFEVTSEL0_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27276_wt_MSR_UNC_CBO_3_PERFEVTSEL0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_3_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFEVTSEL1_001
 *
 * Summary: Dump register MSR_UNC_CBO_0_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_27833_rd_MSR_UNC_CBO_0_PERFEVTSEL1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_0_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_0_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_0_PERFEVTSEL1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27834_wt_MSR_UNC_CBO_0_PERFEVTSEL1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_0_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFEVTSEL1_001
 *
 * Summary: Dump register MSR_UNC_CBO_1_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_27841_rd_MSR_UNC_CBO_1_PERFEVTSEL1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_1_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_1_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_1_PERFEVTSEL1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27842_wt_MSR_UNC_CBO_1_PERFEVTSEL1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_1_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFEVTSEL1_001
 *
 * Summary: Dump register MSR_UNC_CBO_2_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_27462_rd_MSR_UNC_CBO_2_PERFEVTSEL1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_2_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_2_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_2_PERFEVTSEL1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27463_wt_MSR_UNC_CBO_2_PERFEVTSEL1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_2_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFEVTSEL1_001
 *
 * Summary: Dump register MSR_UNC_CBO_3_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_27277_rd_MSR_UNC_CBO_3_PERFEVTSEL1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_3_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_3_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_3_PERFEVTSEL1_002 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_27278_wt_MSR_UNC_CBO_3_PERFEVTSEL1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_3_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_LD_LAT_001
 *
 * Summary: Dump register MSR_PEBS_LD_LAT shall generate #GP(0).
 */
static void pmu_rqmid_28473_rd_MSR_PEBS_LD_LAT_001(void)
{
	pmu_test_rdmsr(MSR_PEBS_LD_LAT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_LD_LAT_002
 *
 * Summary: Execute WRMSR instruction to write MSR_PEBS_LD_LAT with 1H shall generate #GP(0).
 */
static void pmu_rqmid_28474_wt_MSR_PEBS_LD_LAT_002(void)
{
	pmu_test_wrmsr(MSR_PEBS_LD_LAT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_ENABLE_001
 *
 * Summary: Dump register MSR_PEBS_ENABLE shall generate #GP(0).
 */
static void pmu_rqmid_28471_rd_MSR_PEBS_ENABLE_001(void)
{
	pmu_test_rdmsr(MSR_PEBS_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PEBS_ENABLE_002
 *
 * Summary: Execute WRMSR instruction to write MSR_PEBS_ENABLE with 1H shall generate #GP(0).
 */
static void pmu_rqmid_28472_wt_MSR_PEBS_ENABLE_002(void)
{
	pmu_test_wrmsr(MSR_PEBS_ENABLE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_FIXED_CTRL_001
 *
 * Summary: Dump register MSR_UNC_PERF_FIXED_CTRL shall generate #GP(0).
 */
static void pmu_rqmid_29085_rd_MSR_UNC_PERF_FIXED_CTRL_001(void)
{
	pmu_test_rdmsr(MSR_UNC_PERF_FIXED_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_FIXED_CTRL_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_PERF_FIXED_CTRL with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29086_wt_MSR_UNC_PERF_FIXED_CTRL_002(void)
{
	pmu_test_wrmsr(MSR_UNC_PERF_FIXED_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_FIXED_CTR_001
 *
 * Summary: Dump register MSR_UNC_PERF_FIXED_CTR shall generate #GP(0).
 */
static void pmu_rqmid_29087_rd_MSR_UNC_PERF_FIXED_CTR_001(void)
{
	pmu_test_rdmsr(MSR_UNC_PERF_FIXED_CTR, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_FIXED_CTR_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_PERF_FIXED_CTR with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29088_wt_MSR_UNC_PERF_FIXED_CTR_002(void)
{
	pmu_test_wrmsr(MSR_UNC_PERF_FIXED_CTR, GP_VECTOR, 0, __FUNCTION__);
}


/**
 * @brief case name: MSR_UNC_ARB_PERFCTR0_001
 *
 * Summary: Dump register MSR_UNC_ARB_PERFCTR0 shall generate #GP(0).
 */
static void pmu_rqmid_29353_rd_MSR_UNC_ARB_PERFCTR0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_ARB_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFCTR0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_ARB_PERFCTR0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29355_wt_MSR_UNC_ARB_PERFCTR0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_ARB_PERFCTR0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFCTR1_001
 *
 * Summary: Dump register MSR_UNC_ARB_PERFCTR1 shall generate #GP(0).
 */
static void pmu_rqmid_29356_rd_MSR_UNC_ARB_PERFCTR1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_ARB_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFCTR1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_ARB_PERFCTR1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29357_wt_MSR_UNC_ARB_PERFCTR1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_ARB_PERFCTR1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFEVTSEL0_001
 *
 * Summary: Dump register MSR_UNC_ARB_PERFEVTSEL0 shall generate #GP(0).
 */
static void pmu_rqmid_29358_rd_MSR_UNC_ARB_PERFEVTSEL0_001(void)
{
	pmu_test_rdmsr(MSR_UNC_ARB_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFEVTSEL0_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_ARB_PERFEVTSEL0 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29359_wt_MSR_UNC_ARB_PERFEVTSEL0_002(void)
{
	pmu_test_wrmsr(MSR_UNC_ARB_PERFEVTSEL0, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFEVTSEL1_001
 *
 * Summary: Dump register MSR_UNC_ARB_PERFEVTSEL1 shall generate #GP(0).
 */
static void pmu_rqmid_29360_rd_MSR_UNC_ARB_PERFEVTSEL1_001(void)
{
	pmu_test_rdmsr(MSR_UNC_ARB_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_ARB_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_ARB_PERFEVTSEL1 with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29361_wt_MSR_UNC_ARB_PERFEVTSEL1_002(void)
{
	pmu_test_wrmsr(MSR_UNC_ARB_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_GLOBAL_CTRL_001
 *
 * Summary: Dump register MSR_UNC_PERF_GLOBAL_CTRL shall generate #GP(0).
 */
static void pmu_rqmid_29089_rd_MSR_UNC_PERF_GLOBAL_CTRL_001(void)
{
	pmu_test_rdmsr(MSR_UNC_PERF_GLOBAL_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_GLOBAL_CTRL_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_PERF_GLOBAL_CTRL with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29090_wt_MSR_UNC_PERF_GLOBAL_CTRL_002(void)
{
	pmu_test_wrmsr(MSR_UNC_PERF_GLOBAL_CTRL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_GLOBAL_STATUS_001
 *
 * Summary: Dump register MSR_UNC_PERF_GLOBAL_STATUS shall generate #GP(0).
 */
static void pmu_rqmid_29091_rd_MSR_UNC_PERF_GLOBAL_STATUS_001(void)
{
	pmu_test_rdmsr(MSR_UNC_PERF_GLOBAL_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_PERF_GLOBAL_STATUS_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_PERF_GLOBAL_STATUS with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29092_wt_MSR_UNC_PERF_GLOBAL_STATUS_002(void)
{
	pmu_test_wrmsr(MSR_UNC_PERF_GLOBAL_STATUS, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_CONFIG_001
 *
 * Summary: Dump register MSR_UNC_CBO_CONFIG shall generate #GP(0).
 */
static void pmu_rqmid_29220_rd_MSR_UNC_CBO_CONFIG_001(void)
{
	pmu_test_rdmsr(MSR_UNC_CBO_CONFIG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_UNC_CBO_CONFIG_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_CBO_CONFIG with 1H shall generate #GP(0).
 */
static void pmu_rqmid_29221_wt_MSR_UNC_CBO_CONFIG_002(void)
{
	pmu_test_wrmsr(MSR_UNC_CBO_CONFIG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: MSR_PLATFORM_INFO_002
 *
 * Summary: Execute WRMSR instruction to write MSR_PLATFORM_INFO with 1H shall generate #GP(0).
 */
static void pmu_rqmid_37355_wt_MSR_PLATFORM_INFO_002(void)
{
	pmu_test_wrmsr(MSR_PLATFORM_INFO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: CPUID.01H_001
 *
 * Summary:Test CPUID.1:ECX.PDCM[bit 15] should be 0 when PMU is hidden.
 */
static void pmu_rqmid_27843_CPUID_01H_001(void)
{
	u32 res = 0;

	res = cpuid(0x1).c;
	report("%s", ((res & 0x8000) == 0), __FUNCTION__);
}

/**
 * @brief case name: cr4[8] read_001
 *
 * Summary: Dump the CR4.PCE[bit 8] shall be 0.
 */
static void pmu_rqmid_29690_CR4_read_001(void)
{
	report("%s", (read_cr4() & 0x100) == 0, __FUNCTION__);
}

/**
 * @brief case name: cr4[8] write_001
 *
 * Summary: Write CR4.PCE[bit 8] shall generate #GP(0).
 */
static void pmu_rqmid_29689_CR4_write_001(void)
{
	unsigned long val = read_cr4();

	if ((val & 0x100) == 0)
	{
		val |= 0x100;
	}
	else
	{
		val &= ~0x100;
	}

	asm volatile(ASM_TRY("1f")
		"mov %0,%%cr4\n\t"
		"1:" : : "r" (val));

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name: cr4 pce init_001
 *
 * Summary: After AP receives first INIT, dump CR4.PCE[bit 8], the value should be 0.
 *
 */

static void pmu_rqmid_29694_cr4_pce_init_001()
{
	u32 cr4 = *(u32 *)0x8100;

	report("%s", ((cr4 & (1 << 8)) == 0), __FUNCTION__);
}
#endif
#endif

static void print_case_list(void)
{
	printf("PMU feature case list:\n\r");

#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 36505, "PMU_physical_MSR_PLATFORM_INFO_AC_001");
#else
	printf("\t Case ID:%d case name:%s\n\r", 27001, "When a vCPU attempts to read CPUID.0AH_001");
	printf("\t Case ID:%d case name:%s\n\r", 27141, "IA32_FIXED_CTR_CTRL_001");
	printf("\t Case ID:%d case name:%s\n\r", 27142, "IA32_FIXED_CTR_CTRL_002");
	printf("\t Case ID:%d case name:%s\n\r", 27844, "RDPMC_001");
	printf("\t Case ID:%d case name:%s\n\r", 29693, "cr4[8] start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 27114, "IA32_A_PMC0-7_001");
	printf("\t Case ID:%d case name:%s\n\r", 27115, "IA32_A_PMC0-7_002");
	printf("\t Case ID:%d case name:%s\n\r", 27116, "IA32_A_PMC0-7_003");
	printf("\t Case ID:%d case name:%s\n\r", 27117, "IA32_A_PMC0-7_004");
	printf("\t Case ID:%d case name:%s\n\r", 27118, "IA32_A_PMC0-7_005");
	printf("\t Case ID:%d case name:%s\n\r", 27119, "IA32_A_PMC0-7_006");
	printf("\t Case ID:%d case name:%s\n\r", 27120, "IA32_A_PMC0-7_007");
	printf("\t Case ID:%d case name:%s\n\r", 27121, "IA32_A_PMC0-7_008");
	printf("\t Case ID:%d case name:%s\n\r", 27122, "IA32_A_PMC0-7_009");
	printf("\t Case ID:%d case name:%s\n\r", 27123, "IA32_A_PMC0-7_010");
	printf("\t Case ID:%d case name:%s\n\r", 27124, "IA32_A_PMC0-7_011");
	printf("\t Case ID:%d case name:%s\n\r", 27125, "IA32_A_PMC0-7_012");
	printf("\t Case ID:%d case name:%s\n\r", 27126, "IA32_A_PMC0-7_013");
	printf("\t Case ID:%d case name:%s\n\r", 27127, "IA32_A_PMC0-7_014");
	printf("\t Case ID:%d case name:%s\n\r", 27128, "IA32_A_PMC0-7_015");
	printf("\t Case ID:%d case name:%s\n\r", 27129, "IA32_A_PMC0-7_016");
	printf("\t Case ID:%d case name:%s\n\r", 27134, "IA32_FIXED_CTR0-2_001");
	printf("\t Case ID:%d case name:%s\n\r", 27135, "IA32_FIXED_CTR0-2_002");
	printf("\t Case ID:%d case name:%s\n\r", 27136, "IA32_FIXED_CTR0-2_003");
	printf("\t Case ID:%d case name:%s\n\r", 27138, "IA32_FIXED_CTR0-2_004");
	printf("\t Case ID:%d case name:%s\n\r", 27139, "IA32_FIXED_CTR0-2_005");
	printf("\t Case ID:%d case name:%s\n\r", 27140, "IA32_FIXED_CTR0-2_006");
	printf("\t Case ID:%d case name:%s\n\r", 27199, "IA32_PEBS_ENABLE_001");
	printf("\t Case ID:%d case name:%s\n\r", 27200, "IA32_PEBS_ENABLE_002");
	printf("\t Case ID:%d case name:%s\n\r", 27205, "IA32_PERF_CAPABILITIES_001");
	printf("\t Case ID:%d case name:%s\n\r", 27206, "IA32_PERF_CAPABILITIES_002");
	printf("\t Case ID:%d case name:%s\n\r", 27150, "IA32_PERF_GLOBAL_CTRL_001");
	printf("\t Case ID:%d case name:%s\n\r", 27151, "IA32_PERF_GLOBAL_CTRL_002");
	printf("\t Case ID:%d case name:%s\n\r", 27193, "IA32_PERF_GLOBAL_INUSE_001");
	printf("\t Case ID:%d case name:%s\n\r", 27195, "IA32_PERF_GLOBAL_INUSE_002");
	printf("\t Case ID:%d case name:%s\n\r", 27146, "IA32_PERF_GLOBAL_STATUS_001");
	printf("\t Case ID:%d case name:%s\n\r", 27147, "IA32_PERF_GLOBAL_STATUS_002");
	printf("\t Case ID:%d case name:%s\n\r", 27148, "IA32_PERF_GLOBAL_STATUS_RESET_001");
	printf("\t Case ID:%d case name:%s\n\r", 27149, "IA32_PERF_GLOBAL_STATUS_RESET_002");
	printf("\t Case ID:%d case name:%s\n\r", 27191, "IA32_PERF_GLOBAL_STATUS_SET_001");
	printf("\t Case ID:%d case name:%s\n\r", 27192, "IA32_PERF_GLOBAL_STATUS_SET_002");
	printf("\t Case ID:%d case name:%s\n\r", 27002, "IA32_PERFEVTSEL0-3_001");
	printf("\t Case ID:%d case name:%s\n\r", 27003, "IA32_PERFEVTSEL0-3_002");
	printf("\t Case ID:%d case name:%s\n\r", 27004, "IA32_PERFEVTSEL0-3_003");
	printf("\t Case ID:%d case name:%s\n\r", 27005, "IA32_PERFEVTSEL0-3_004");
	printf("\t Case ID:%d case name:%s\n\r", 27006, "IA32_PERFEVTSEL0-3_005");
	printf("\t Case ID:%d case name:%s\n\r", 27007, "IA32_PERFEVTSEL0-3_006");
	printf("\t Case ID:%d case name:%s\n\r", 27008, "IA32_PERFEVTSEL0-3_007");
	printf("\t Case ID:%d case name:%s\n\r", 27009, "IA32_PERFEVTSEL0-3_008");
	printf("\t Case ID:%d case name:%s\n\r", 27097, "IA32_PMC0-7_001");
	printf("\t Case ID:%d case name:%s\n\r", 27098, "IA32_PMC0-7_002");
	printf("\t Case ID:%d case name:%s\n\r", 27099, "IA32_PMC0-7_003");
	printf("\t Case ID:%d case name:%s\n\r", 27100, "IA32_PMC0-7_004");
	printf("\t Case ID:%d case name:%s\n\r", 27101, "IA32_PMC0-7_005");
	printf("\t Case ID:%d case name:%s\n\r", 27102, "IA32_PMC0-7_006");
	printf("\t Case ID:%d case name:%s\n\r", 27103, "IA32_PMC0-7_007");
	printf("\t Case ID:%d case name:%s\n\r", 27104, "IA32_PMC0-7_008");
	printf("\t Case ID:%d case name:%s\n\r", 27105, "IA32_PMC0-7_009");
	printf("\t Case ID:%d case name:%s\n\r", 27106, "IA32_PMC0-7_010");
	printf("\t Case ID:%d case name:%s\n\r", 27107, "IA32_PMC0-7_011");
	printf("\t Case ID:%d case name:%s\n\r", 27108, "IA32_PMC0-7_012");
	printf("\t Case ID:%d case name:%s\n\r", 27109, "IA32_PMC0-7_013");
	printf("\t Case ID:%d case name:%s\n\r", 27110, "IA32_PMC0-7_014");
	printf("\t Case ID:%d case name:%s\n\r", 27112, "IA32_PMC0-7_015");
	printf("\t Case ID:%d case name:%s\n\r", 27113, "IA32_PMC0-7_016");
	printf("\t Case ID:%d case name:%s\n\r", 27196, "MSR_PEBS_FRONTEND_001");
	printf("\t Case ID:%d case name:%s\n\r", 27197, "MSR_PEBS_FRONTEND_002");
	printf("\t Case ID:%d case name:%s\n\r", 27835, "MSR_UNC_CBO_0_PERFCTR0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27836, "MSR_UNC_CBO_0_PERFCTR0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27837, "MSR_UNC_CBO_0_PERFCTR1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27838, "MSR_UNC_CBO_0_PERFCTR1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27831, "MSR_UNC_CBO_0_PERFEVTSEL0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27832, "MSR_UNC_CBO_0_PERFEVTSEL0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27833, "MSR_UNC_CBO_0_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27834, "MSR_UNC_CBO_0_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27826, "MSR_UNC_CBO_1_PERFCTR0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27827, "MSR_UNC_CBO_1_PERFCTR0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27828, "MSR_UNC_CBO_1_PERFCTR1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27829, "MSR_UNC_CBO_1_PERFCTR1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27839, "MSR_UNC_CBO_1_PERFEVTSEL0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27840, "MSR_UNC_CBO_1_PERFEVTSEL0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27841, "MSR_UNC_CBO_1_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27842, "MSR_UNC_CBO_1_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27405, "MSR_UNC_CBO_2_PERFCTR0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27406, "MSR_UNC_CBO_2_PERFCTR0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27458, "MSR_UNC_CBO_2_PERFCTR1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27459, "MSR_UNC_CBO_2_PERFCTR1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27460, "MSR_UNC_CBO_2_PERFEVTSEL0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27461, "MSR_UNC_CBO_2_PERFEVTSEL0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27462, "MSR_UNC_CBO_2_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27463, "MSR_UNC_CBO_2_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27271, "MSR_UNC_CBO_3_PERFCTR0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27272, "MSR_UNC_CBO_3_PERFCTR0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27273, "MSR_UNC_CBO_3_PERFCTR1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27274, "MSR_UNC_CBO_3_PERFCTR1_002");
	printf("\t Case ID:%d case name:%s\n\r", 27275, "MSR_UNC_CBO_3_PERFEVTSEL0_001");
	printf("\t Case ID:%d case name:%s\n\r", 27276, "MSR_UNC_CBO_3_PERFEVTSEL0_002");
	printf("\t Case ID:%d case name:%s\n\r", 27277, "MSR_UNC_CBO_3_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 27278, "MSR_UNC_CBO_3_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 28471, "MSR_PEBS_ENABLE_001");
	printf("\t Case ID:%d case name:%s\n\r", 28472, "MSR_PEBS_ENABLE_002");
	printf("\t Case ID:%d case name:%s\n\r", 28473, "MSR_PEBS_LD_LAT_001");
	printf("\t Case ID:%d case name:%s\n\r", 28474, "MSR_PEBS_LD_LAT_002");
	printf("\t Case ID:%d case name:%s\n\r", 29085, "MSR_UNC_PERF_FIXED_CTRL_001");
	printf("\t Case ID:%d case name:%s\n\r", 29086, "MSR_UNC_PERF_FIXED_CTRL_002");
	printf("\t Case ID:%d case name:%s\n\r", 29087, "MSR_UNC_PERF_FIXED_CTR_001");
	printf("\t Case ID:%d case name:%s\n\r", 29088, "MSR_UNC_PERF_FIXED_CTR_002");
	printf("\t Case ID:%d case name:%s\n\r", 29353, "MSR_UNC_ARB_PERFCTR0_001");
	printf("\t Case ID:%d case name:%s\n\r", 29355, "MSR_UNC_ARB_PERFCTR0_002");
	printf("\t Case ID:%d case name:%s\n\r", 29356, "MSR_UNC_ARB_PERFCTR1_001");
	printf("\t Case ID:%d case name:%s\n\r", 29357, "MSR_UNC_ARB_PERFCTR1_002");
	printf("\t Case ID:%d case name:%s\n\r", 29358, "MSR_UNC_ARB_PERFEVTSEL0_001");
	printf("\t Case ID:%d case name:%s\n\r", 29359, "MSR_UNC_ARB_PERFEVTSEL0_002");
	printf("\t Case ID:%d case name:%s\n\r", 29360, "MSR_UNC_ARB_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 29361, "MSR_UNC_ARB_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 29089, "MSR_UNC_PERF_GLOBAL_CTRL_001");
	printf("\t Case ID:%d case name:%s\n\r", 29090, "MSR_UNC_PERF_GLOBAL_CTRL_002");
	printf("\t Case ID:%d case name:%s\n\r", 29091, "MSR_UNC_PERF_GLOBAL_STATUS_001");
	printf("\t Case ID:%d case name:%s\n\r", 29092, "MSR_UNC_PERF_GLOBAL_STATUS_002");
	printf("\t Case ID:%d case name:%s\n\r", 29220, "MSR_UNC_CBO_CONFIG_001");
	printf("\t Case ID:%d case name:%s\n\r", 29221, "MSR_UNC_CBO_CONFIG_002");
	printf("\t Case ID:%d case name:%s\n\r", 37355, "MSR_PLATFORM_INFO_002");
	printf("\t Case ID:%d case name:%s\n\r", 27843, "CPUID.01H_001");
	printf("\t Case ID:%d case name:%s\n\r", 29690, "cr4[8] read_001");
	printf("\t Case ID:%d case name:%s\n\r", 29689, "cr4[8] write_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29694, "cr4 pce init_001");
#endif
#endif
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();

#ifdef IN_NATIVE
	pmu_rqmid_36505_PMU_physical_MSR_PLATFORM_INFO_AC_001();
#else
	pmu_rqmid_27001_cpuid_a();
	pmu_rqmid_27141_rd_ia32_fixed_ctr_ctrl();
	pmu_rqmid_27142_wt_ia32_fixed_ctr_ctrl();
	pmu_rqmid_27844_rdpmc();
	pmu_rqmid_29693_cr4_bit8_startup_001();
	pmu_rqmid_27114_rd_IA32_A_PMC0_7_001();
	pmu_rqmid_27115_wt_IA32_A_PMC0_7_002();
	pmu_rqmid_27116_rd_IA32_A_PMC0_7_003();
	pmu_rqmid_27117_wt_IA32_A_PMC0_7_004();
	pmu_rqmid_27118_rd_IA32_A_PMC0_7_005();
	pmu_rqmid_27119_wt_IA32_A_PMC0_7_006();
	pmu_rqmid_27120_rd_IA32_A_PMC0_7_007();
	pmu_rqmid_27121_wt_IA32_A_PMC0_7_008();
	pmu_rqmid_27122_rd_IA32_A_PMC0_7_009();
	pmu_rqmid_27123_wt_IA32_A_PMC0_7_010();
	pmu_rqmid_27124_rd_IA32_A_PMC0_7_011();
	pmu_rqmid_27125_wt_IA32_A_PMC0_7_012();
	pmu_rqmid_27126_rd_IA32_A_PMC0_7_013();
	pmu_rqmid_27127_wt_IA32_A_PMC0_7_014();
	pmu_rqmid_27128_rd_IA32_A_PMC0_7_015();
	pmu_rqmid_27129_wt_IA32_A_PMC0_7_016();
	pmu_rqmid_27134_rd_IA32_FIXED_CTR0_2_001();
	pmu_rqmid_27135_wt_IA32_FIXED_CTR0_2_002();
	pmu_rqmid_27136_rd_IA32_FIXED_CTR0_2_003();
	pmu_rqmid_27138_wt_IA32_FIXED_CTR0_2_004();
	pmu_rqmid_27139_rd_IA32_FIXED_CTR0_2_005();
	pmu_rqmid_27140_wt_IA32_FIXED_CTR0_2_006();
	pmu_rqmid_27199_rd_IA32_PEBS_ENABLE_001();
	pmu_rqmid_27200_wt_IA32_PEBS_ENABLE_002();
	pmu_rqmid_27205_rd_IA32_PERF_CAPABILITIES_001();
	pmu_rqmid_27206_wt_IA32_PERF_CAPABILITIES_002();
	pmu_rqmid_27150_rd_IA32_PERF_GLOBAL_CTRL_001();
	pmu_rqmid_27151_wt_IA32_PERF_GLOBAL_CTRL_002();
	pmu_rqmid_27193_rd_IA32_PERF_GLOBAL_INUSE_001();
	pmu_rqmid_27195_wt_IA32_PERF_GLOBAL_INUSE_002();
	pmu_rqmid_27146_rd_IA32_PERF_GLOBAL_STATUS_001();
	pmu_rqmid_27147_wt_IA32_PERF_GLOBAL_STATUS_002();
	pmu_rqmid_27148_rd_IA32_PERF_GLOBAL_STATUS_RESET_001();
	pmu_rqmid_27149_wt_IA32_PERF_GLOBAL_STATUS_RESET_002();
	pmu_rqmid_27191_rd_IA32_PERF_GLOBAL_STATUS_SET_001();
	pmu_rqmid_27192_wt_IA32_PERF_GLOBAL_STATUS_SET_002();
	pmu_rqmid_27002_rd_IA32_PERFEVTSEL0_3_001();
	pmu_rqmid_27003_wt_IA32_PERFEVTSEL0_3_002();
	pmu_rqmid_27004_rd_IA32_PERFEVTSEL0_3_003();
	pmu_rqmid_27005_wt_IA32_PERFEVTSEL0_3_004();
	pmu_rqmid_27006_rd_IA32_PERFEVTSEL0_3_005();
	pmu_rqmid_27007_wt_IA32_PERFEVTSEL0_3_006();
	pmu_rqmid_27008_rd_IA32_PERFEVTSEL0_3_007();
	pmu_rqmid_27009_wt_IA32_PERFEVTSEL0_3_008();
	pmu_rqmid_27097_rd_IA32_PMC0_7_001();
	pmu_rqmid_27098_wt_IA32_PMC0_7_002();
	pmu_rqmid_27099_rd_IA32_PMC0_7_003();
	pmu_rqmid_27100_wt_IA32_PMC0_7_004();
	pmu_rqmid_27101_rd_IA32_PMC0_7_005();
	pmu_rqmid_27102_wt_IA32_PMC0_7_006();
	pmu_rqmid_27103_rd_IA32_PMC0_7_007();
	pmu_rqmid_27104_wt_IA32_PMC0_7_008();
	pmu_rqmid_27105_rd_IA32_PMC0_7_009();
	pmu_rqmid_27106_wt_IA32_PMC0_7_010();
	pmu_rqmid_27107_rd_IA32_PMC0_7_011();
	pmu_rqmid_27108_wt_IA32_PMC0_7_012();
	pmu_rqmid_27109_rd_IA32_PMC0_7_013();
	pmu_rqmid_27110_wt_IA32_PMC0_7_014();
	pmu_rqmid_27112_rd_IA32_PMC0_7_015();
	pmu_rqmid_27113_wt_IA32_PMC0_7_016();
	pmu_rqmid_27201_rd_MSR_OFFCORE_RSP_0_1_001();
	pmu_rqmid_27202_wt_MSR_OFFCORE_RSP_0_1_002();
	pmu_rqmid_27203_rd_MSR_OFFCORE_RSP_0_1_003();
	pmu_rqmid_27204_wt_MSR_OFFCORE_RSP_0_1_004();
	pmu_rqmid_27196_rd_MSR_PEBS_FRONTEND_001();
	pmu_rqmid_27197_wt_MSR_PEBS_FRONTEND_002();
	pmu_rqmid_27835_rd_MSR_UNC_CBO_0_PERFCTR0_001();
	pmu_rqmid_27836_wt_MSR_UNC_CBO_0_PERFCTR0_002();
	pmu_rqmid_27837_rd_MSR_UNC_CBO_0_PERFCTR1_001();
	pmu_rqmid_27838_wt_MSR_UNC_CBO_0_PERFCTR1_002();
	pmu_rqmid_27831_rd_MSR_UNC_CBO_0_PERFEVTSEL0_001();
	pmu_rqmid_27832_wt_MSR_UNC_CBO_0_PERFEVTSEL0_002();
	pmu_rqmid_27833_rd_MSR_UNC_CBO_0_PERFEVTSEL1_001();
	pmu_rqmid_27834_wt_MSR_UNC_CBO_0_PERFEVTSEL1_002();
	pmu_rqmid_27826_rd_MSR_UNC_CBO_1_PERFCTR0_001();
	pmu_rqmid_27827_wt_MSR_UNC_CBO_1_PERFCTR0_002();
	pmu_rqmid_27828_rd_MSR_UNC_CBO_1_PERFCTR1_001();
	pmu_rqmid_27829_wt_MSR_UNC_CBO_1_PERFCTR1_002();
	pmu_rqmid_27839_rd_MSR_UNC_CBO_1_PERFEVTSEL0_001();
	pmu_rqmid_27840_wt_MSR_UNC_CBO_1_PERFEVTSEL0_002();
	pmu_rqmid_27841_rd_MSR_UNC_CBO_1_PERFEVTSEL1_001();
	pmu_rqmid_27842_wt_MSR_UNC_CBO_1_PERFEVTSEL1_002();
	pmu_rqmid_27405_rd_MSR_UNC_CBO_2_PERFCTR0_001();
	pmu_rqmid_27406_wt_MSR_UNC_CBO_2_PERFCTR0_002();
	pmu_rqmid_27458_rd_MSR_UNC_CBO_2_PERFCTR1_001();
	pmu_rqmid_27459_wt_MSR_UNC_CBO_2_PERFCTR1_002();
	pmu_rqmid_27460_rd_MSR_UNC_CBO_2_PERFEVTSEL0_001();
	pmu_rqmid_27461_wt_MSR_UNC_CBO_2_PERFEVTSEL0_002();
	pmu_rqmid_27462_rd_MSR_UNC_CBO_2_PERFEVTSEL1_001();
	pmu_rqmid_27463_wt_MSR_UNC_CBO_2_PERFEVTSEL1_002();
	pmu_rqmid_27271_rd_MSR_UNC_CBO_3_PERFCTR0_001();
	pmu_rqmid_27272_wt_MSR_UNC_CBO_3_PERFCTR0_002();
	pmu_rqmid_27273_rd_MSR_UNC_CBO_3_PERFCTR1_001();
	pmu_rqmid_27274_wt_MSR_UNC_CBO_3_PERFCTR1_002();
	pmu_rqmid_27275_rd_MSR_UNC_CBO_3_PERFEVTSEL0_001();
	pmu_rqmid_27276_wt_MSR_UNC_CBO_3_PERFEVTSEL0_002();
	pmu_rqmid_27277_rd_MSR_UNC_CBO_3_PERFEVTSEL1_001();
	pmu_rqmid_27278_wt_MSR_UNC_CBO_3_PERFEVTSEL1_002();
	pmu_rqmid_28471_rd_MSR_PEBS_ENABLE_001();
	pmu_rqmid_28472_wt_MSR_PEBS_ENABLE_002();
	pmu_rqmid_28473_rd_MSR_PEBS_LD_LAT_001();
	pmu_rqmid_28474_wt_MSR_PEBS_LD_LAT_002();
	pmu_rqmid_29085_rd_MSR_UNC_PERF_FIXED_CTRL_001();
	pmu_rqmid_29086_wt_MSR_UNC_PERF_FIXED_CTRL_002();
	pmu_rqmid_29087_rd_MSR_UNC_PERF_FIXED_CTR_001();
	pmu_rqmid_29088_wt_MSR_UNC_PERF_FIXED_CTR_002();
	pmu_rqmid_29353_rd_MSR_UNC_ARB_PERFCTR0_001();
	pmu_rqmid_29355_wt_MSR_UNC_ARB_PERFCTR0_002();
	pmu_rqmid_29356_rd_MSR_UNC_ARB_PERFCTR1_001();
	pmu_rqmid_29357_wt_MSR_UNC_ARB_PERFCTR1_002();
	pmu_rqmid_29358_rd_MSR_UNC_ARB_PERFEVTSEL0_001();
	pmu_rqmid_29359_wt_MSR_UNC_ARB_PERFEVTSEL0_002();
	pmu_rqmid_29360_rd_MSR_UNC_ARB_PERFEVTSEL1_001();
	pmu_rqmid_29361_wt_MSR_UNC_ARB_PERFEVTSEL1_002();
	pmu_rqmid_29089_rd_MSR_UNC_PERF_GLOBAL_CTRL_001();
	pmu_rqmid_29090_wt_MSR_UNC_PERF_GLOBAL_CTRL_002();
	pmu_rqmid_29091_rd_MSR_UNC_PERF_GLOBAL_STATUS_001();
	pmu_rqmid_29092_wt_MSR_UNC_PERF_GLOBAL_STATUS_002();
	pmu_rqmid_29220_rd_MSR_UNC_CBO_CONFIG_001();
	pmu_rqmid_29221_wt_MSR_UNC_CBO_CONFIG_002();
	pmu_rqmid_37355_wt_MSR_PLATFORM_INFO_002();
	pmu_rqmid_27843_CPUID_01H_001();
	pmu_rqmid_29690_CR4_read_001();
	pmu_rqmid_29689_CR4_write_001();
#ifdef IN_NON_SAFETY_VM
	pmu_rqmid_29694_cr4_pce_init_001();
#endif
#endif
	return report_summary();
}
