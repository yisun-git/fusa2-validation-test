#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "regdump.h"
#include "register_op.h"
#include "misc.h"
#include "fwcfg.h"
#include "misc_msr.h"
#include "delay.h"

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

int ap_start_count = 0;
volatile u64 init_platform_id = 0;
volatile u64 set_platform_id = 0;

typedef struct {
		u32 msr;
		u32 start_bit;
		u32 end_bit;
	} t_msrbits;

t_msrbits rvdbits[] = {
			{IA32_APIC_BASE, 0, 7},
			{IA32_APIC_BASE, 9, 9},
			{IA32_APIC_BASE, 63, 63},
			{IA32_MCG_STATUS, 4, 63},
			{IA32_PAT, 3, 7},
			{IA32_PAT, 11, 15},
			{IA32_PAT, 19, 23},
			{IA32_PAT, 27, 31},
			{IA32_PAT, 35, 39},
			{IA32_PAT, 43, 47},
			{IA32_PAT, 51, 55},
			{IA32_PAT, 59, 63},
			{IA32_BIOS_SIGN_ID, 0, 31},
			{IA32_EFER, 1, 7},
			{IA32_EFER, 9, 9},
			{IA32_EFER, 12, 63},
			{IA32_FEATURE_CONTROL, 3, 7},
			{IA32_FEATURE_CONTROL, 16, 16},
			{IA32_FEATURE_CONTROL, 19, 19},
			{IA32_FEATURE_CONTROL, 21, 63},
			{IA32_MCG_CAP, 12, 15},
			{IA32_MCG_CAP, 25, 25},
			{IA32_MCG_CAP, 28, 63},
			{IA32_PLATFORM_ID, 0, 49},
			{IA32_PLATFORM_ID, 53, 63},
			{MSR_FEATURE_CONFIG, 2, 63},
			{MSR_PLATFORM_INFO, 0, 7},
		};

t_msrbits rvdmsr[] = {
	{IA32_L2_MASK_1, 0, 0},
	{IA32_L3_MASK_1, 0, 0},
	{MSR_OFFCORE_RSP_0, 0, 0},
	{MSR_OFFCORE_RSP_1, 0, 0},
	{MSR_LASTBRANCH_0_FROM_IP, 0, 0},
	{IA32_SGXLEPUBKEYHASH0, 0, 0},
	{IA32_SGXLEPUBKEYHASH1, 0, 0},
	{IA32_SGXLEPUBKEYHASH2, 0, 0},
	{IA32_SGXLEPUBKEYHASH3, 0, 0},
	{IA32_PMC0, 0, 0},
	{IA32_PMC1, 0, 0},
	{IA32_PMC2, 0, 0},
	{IA32_PMC3, 0, 0},
	{IA32_PMC4, 0, 0},
	{IA32_PMC5, 0, 0},
	{IA32_PMC6, 0, 0},
	{IA32_PMC7, 0, 0},
	{IA32_PERFEVTSEL0, 0, 0},
	{IA32_PERFEVTSEL1, 0, 0},
	{IA32_PERFEVTSEL2, 0, 0},
	{IA32_PERFEVTSEL3, 0, 0},
	{IA32_PERF_GLOBAL_OVF_CTRL, 0, 0},
	{IA32_MC0_ADDR, 0, 0},
	{IA32_MC0_MISC, 0, 0},
	{IA32_MC1_ADDR, 0, 0},
	{IA32_MC1_MISC, 0, 0},
	{IA32_MC2_ADDR, 0, 0},
	{IA32_MC2_MISC, 0, 0},
	{IA32_MC3_ADDR, 0, 0},
	{IA32_MC3_MISC, 0, 0},
	{IA32_MC4_ADDR, 0, 0},
	{IA32_MC4_MISC, 0, 0},
	{IA32_MC5_ADDR, 0, 0},
	{IA32_MC5_MISC, 0, 0},
	{IA32_MC6_ADDR, 0, 0},
	{IA32_MC6_MISC, 0, 0},
	{IA32_MC7_ADDR, 0, 0},
	{IA32_MC7_MISC, 0, 0},
	{IA32_MC8_ADDR, 0, 0},
	{IA32_MC8_MISC, 0, 0},
	{IA32_MC9_ADDR, 0, 0},
	{IA32_MC9_MISC, 0, 0},
	{IA32_MC10_ADDR, 0, 0},
	{IA32_MC10_MISC, 0, 0},
	{IA32_MC11_ADDR, 0, 0},
	{IA32_MC11_MISC, 0, 0},
	{IA32_MC12_ADDR, 0, 0},
	{IA32_MC12_MISC, 0, 0},
	{IA32_MC13_ADDR, 0, 0},
	{IA32_MC13_MISC, 0, 0},
	{IA32_MC14_ADDR, 0, 0},
	{IA32_MC14_MISC, 0, 0},
	{IA32_MC15_ADDR, 0, 0},
	{IA32_MC15_MISC, 0, 0},
	{IA32_MC16_ADDR, 0, 0},
	{IA32_MC16_MISC, 0, 0},
	{IA32_MC17_ADDR, 0, 0},
	{IA32_MC17_MISC, 0, 0},
	{IA32_MC18_ADDR, 0, 0},
	{IA32_MC18_MISC, 0, 0},
	{IA32_MC19_ADDR, 0, 0},
	{IA32_MC19_MISC, 0, 0},
	{IA32_MC20_ADDR, 0, 0},
	{IA32_MC20_MISC, 0, 0},
	{IA32_MC21_ADDR, 0, 0},
	{IA32_MC21_MISC, 0, 0},
	{IA32_MC22_STATUS, 0, 0},
	{IA32_MC22_ADDR, 0, 0},
	{IA32_MC22_MISC, 0, 0},
	{IA32_MC23_CTL, 0, 0},
	{IA32_MC23_STATUS, 0, 0},
	{IA32_MC23_ADDR, 0, 0},
	{IA32_MC23_MISC, 0, 0},
	{IA32_MC24_CTL, 0, 0},
	{IA32_MC24_STATUS, 0, 0},
	{IA32_MC24_ADDR, 0, 0},
	{IA32_MC24_MISC, 0, 0},
	{IA32_MC25_CTL, 0, 0},
	{IA32_MC25_STATUS, 0, 0},
	{IA32_MC25_ADDR, 0, 0},
	{IA32_MC25_MISC, 0, 0},
	{IA32_MC26_CTL, 0, 0},
	{IA32_MC26_STATUS, 0, 0},
	{IA32_MC26_ADDR, 0, 0},
	{IA32_MC26_MISC, 0, 0},
	{IA32_MC27_CTL, 0, 0},
	{IA32_MC27_STATUS, 0, 0},
	{IA32_MC27_ADDR, 0, 0},
	{IA32_MC27_MISC, 0, 0},
	{IA32_MC28_CTL, 0, 0},
	{IA32_MC28_STATUS, 0, 0},
	{IA32_MC28_ADDR, 0, 0},
	{IA32_MC28_MISC, 0, 0},
	{IA32_FIXED_CTR0, 0, 0},
	{IA32_FIXED_CTR1, 0, 0},
	{IA32_FIXED_CTR2, 0, 0},
	{IA32_ARCH_CAPABILITIES_MSR, 0, 0},
	{IA32_A_PMC0, 0, 0},
	{IA32_A_PMC1, 0, 0},
	{IA32_A_PMC2, 0, 0},
	{IA32_A_PMC3, 0, 0},
	{IA32_A_PMC4, 0, 0},
	{IA32_A_PMC5, 0, 0},
	{IA32_A_PMC6, 0, 0},
	{IA32_A_PMC7, 0, 0},
	{IA32_APERF, 0, 0},
	{IA32_BIOS_UPDT_TRIG, 0, 0},
	{IA32_BNDCFGS, 0, 0},
	{IA32_CLOCK_MODULATION, 0, 0},
	{IA32_CPU_DCA_CAP, 0, 0},
	{IA32_DCA_0_CAP, 0, 0},
	{IA32_DEBUG_INTERFACE, 0, 0},
	{IA32_DEBUGCTL, 0, 0},
	{IA32_DS_AREA, 0, 0},
	{IA32_ENERGY_PERF_BIAS, 0, 0},
	{IA32_FIXED_CTR_CTRL, 0, 0},
	{IA32_HWP_CAPABILITIES, 0, 0},
	{IA32_HWP_INTERRUPT, 0, 0},
	{IA32_HWP_REQUEST, 0, 0},
	{IA32_HWP_REQUEST_PKG, 0, 0},
	{IA32_HWP_STATUS, 0, 0},
	{IA32_L2_QOS_CFG, 0, 0},
	{IA32_L3_QOS_CFG, 0, 0},
	{IA32_MCG_CTL, 0, 0},
	{IA32_MCG_EXT_CTL, 0, 0},
	{IA32_MPERF, 0, 0},
	{IA32_MTRR_DEF_TYPE, 0, 0},
	{IA32_MTRR_FIX16K_80000, 0, 0},
	{IA32_MTRR_FIX16K_A0000, 0, 0},
	{IA32_MTRR_FIX4K_C0000, 0, 0},
	{IA32_MTRR_FIX4K_C8000, 0, 0},
	{IA32_MTRR_FIX4K_D0000, 0, 0},
	{IA32_MTRR_FIX4K_D8000, 0, 0},
	{IA32_MTRR_FIX4K_E0000, 0, 0},
	{IA32_MTRR_FIX4K_E8000, 0, 0},
	{IA32_MTRR_FIX4K_F0000, 0, 0},
	{IA32_MTRR_FIX4K_F8000, 0, 0},
	{IA32_MTRR_FIX64K_00000, 0, 0},
	{IA32_MTRR_PHYSBASE0, 0, 0},
	{IA32_MTRR_PHYSBASE1, 0, 0},
	{IA32_MTRR_PHYSBASE2, 0, 0},
	{IA32_MTRR_PHYSBASE3, 0, 0},
	{IA32_MTRR_PHYSBASE4, 0, 0},
	{IA32_MTRR_PHYSBASE5, 0, 0},
	{IA32_MTRR_PHYSBASE6, 0, 0},
	{IA32_MTRR_PHYSBASE7, 0, 0},
	{IA32_MTRR_PHYSBASE8, 0, 0},
	{IA32_MTRR_PHYSBASE9, 0, 0},
	{IA32_MTRR_PHYSMASK0, 0, 0},
	{IA32_MTRR_PHYSMASK1, 0, 0},
	{IA32_MTRR_PHYSMASK2, 0, 0},
	{IA32_MTRR_PHYSMASK3, 0, 0},
	{IA32_MTRR_PHYSMASK4, 0, 0},
	{IA32_MTRR_PHYSMASK5, 0, 0},
	{IA32_MTRR_PHYSMASK6, 0, 0},
	{IA32_MTRR_PHYSMASK7, 0, 0},
	{IA32_MTRR_PHYSMASK8, 0, 0},
	{IA32_MTRR_PHYSMASK9, 0, 0},
	{IA32_MTRRCAP, 0, 0},
	{IA32_PACKAGE_THERM_INTERRUPT, 0, 0},
	{IA32_PACKAGE_THERM_STATUS, 0, 0},
	{IA32_PEBS_ENABLE, 0, 0},
	{IA32_PERF_CAPABILITIES, 0, 0},
	{IA32_PERF_CTL, 0, 0},
	{IA32_PERF_GLOBAL_CTRL, 0, 0},
	{IA32_PERF_GLOBAL_INUSE, 0, 0},
	{IA32_PERF_GLOBAL_STATUS, 0, 0},
	{IA32_PERF_GLOBAL_STATUS_SET, 0, 0},
	{IA32_PKG_HDC_CTL, 0, 0},
	{IA32_PLATFORM_DCA_CAP, 0, 0},
	{IA32_PM_CTL1, 0, 0},
	{IA32_PM_ENABLE, 0, 0},
	{IA32_PQR_ASSOC, 0, 0},
	{IA32_QM_CTR, 0, 0},
	{IA32_QM_EVTSEL, 0, 0},
	{IA32_RTIT_ADDR0_A, 0, 0},
	{IA32_RTIT_ADDR0_B, 0, 0},
	{IA32_RTIT_ADDR1_A, 0, 0},
	{IA32_RTIT_ADDR1_B, 0, 0},
	{IA32_RTIT_ADDR2_A, 0, 0},
	{IA32_RTIT_ADDR2_B, 0, 0},
	{IA32_RTIT_ADDR3_A, 0, 0},
	{IA32_RTIT_ADDR3_B, 0, 0},
	{IA32_RTIT_CR3_MATCH, 0, 0},
	{IA32_RTIT_CTL, 0, 0},
	{IA32_RTIT_OUTPUT_BASE, 0, 0},
	{IA32_RTIT_OUTPUT_MASK_PTRS, 0, 0},
	{IA32_RTIT_STATUS, 0, 0},
	{IA32_SGX_SVN_STATUS, 0, 0},
	{IA32_SMBASE, 0, 0},
	{IA32_SMM_MONITOR_CTL, 0, 0},
	{IA32_SMRR_PHYSBASE, 0, 0},
	{IA32_SMRR_PHYSMASK, 0, 0},
	{IA32_THERM_INTERRUPT, 0, 0},
	{IA32_THERM_STATUS, 0, 0},
	{IA32_THREAD_STALL, 0, 0},
	{IA32_VMX_BASIC, 0, 0},
	{IA32_VMX_CR0_FIXED0, 0, 0},
	{IA32_VMX_CR0_FIXED1, 0, 0},
	{IA32_VMX_CR4_FIXED0, 0, 0},
	{IA32_VMX_CR4_FIXED1, 0, 0},
	{IA32_VMX_ENTRY_CTLS, 0, 0},
	{IA32_VMX_EPT_VPID_CAP, 0, 0},
	{IA32_VMX_EXIT_CTLS, 0, 0},
	{IA32_VMX_MISC, 0, 0},
	{IA32_VMX_PINBASED_CTLS, 0, 0},
	{IA32_VMX_PROCBASED_CTLS, 0, 0},
	{IA32_VMX_PROCBASED_CTLS2, 0, 0},
	{IA32_VMX_TRUE_ENTRY_CTLS, 0, 0},
	{IA32_VMX_TRUE_EXIT_CTLS, 0, 0},
	{IA32_VMX_TRUE_PINBASED_CTLS, 0, 0},
	{IA32_VMX_TRUE_PROCBASED_CTLS, 0, 0},
	{IA32_VMX_VMCS_ENUM, 0, 0},
	{IA32_VMX_VMFUNC, 0, 0},
	{IA32_XSS, 0, 0},
	{MSR_ANY_CORE_C0, 0, 0},
	{MSR_ANY_GFXE_C0, 0, 0},
	{MSR_CONFIG_TDP_CONTROL, 0, 0},
	{MSR_CONFIG_TDP_LEVEL1, 0, 0},
	{MSR_CONFIG_TDP_LEVEL2, 0, 0},
	{MSR_CONFIG_TDP_NOMINAL, 0, 0},
	{MSR_CORE_C3_RESIDENCY, 0, 0},
	{MSR_CORE_C6_RESIDENCY, 0, 0},
	{MSR_CORE_C7_RESIDENCY, 0, 0},
	{MSR_CORE_GFXE_OVERLAP_C0, 0, 0},
	{MSR_CORE_HDC_RESIDENCY, 0, 0},
	{MSR_CORE_PERF_LIMIT_REASONS, 0, 0},
	{MSR_DRAM_ENERGY_STATUS, 0, 0},
	{MSR_DRAM_PERF_STATUS, 0, 0},
	{MSR_GRAPHICS_PERF_LIMIT_REASONS, 0, 0},
	{MSR_LASTBRANCH_TOS, 0, 0},
	{MSR_LBR_INFO_0, 0, 0},
	{MSR_LBR_INFO_1, 0, 0},
	{MSR_LBR_INFO_10, 0, 0},
	{MSR_LBR_INFO_11, 0, 0},
	{MSR_LBR_INFO_12, 0, 0},
	{MSR_LBR_INFO_13, 0, 0},
	{MSR_LBR_INFO_14, 0, 0},
	{MSR_LBR_INFO_15, 0, 0},
	{MSR_LBR_INFO_16, 0, 0},
	{MSR_LBR_INFO_17, 0, 0},
	{MSR_LBR_INFO_18, 0, 0},
	{MSR_LBR_INFO_19, 0, 0},
	{MSR_LBR_INFO_2, 0, 0},
	{MSR_LBR_INFO_20, 0, 0},
	{MSR_LBR_INFO_21, 0, 0},
	{MSR_LBR_INFO_22, 0, 0},
	{MSR_LBR_INFO_23, 0, 0},
	{MSR_LBR_INFO_24, 0, 0},
	{MSR_LBR_INFO_25, 0, 0},
	{MSR_LBR_INFO_26, 0, 0},
	{MSR_LBR_INFO_27, 0, 0},
	{MSR_LBR_INFO_28, 0, 0},
	{MSR_LBR_INFO_29, 0, 0},
	{MSR_LBR_INFO_3, 0, 0},
	{MSR_LBR_INFO_30, 0, 0},
	{MSR_LBR_INFO_31, 0, 0},
	{MSR_LBR_INFO_4, 0, 0},
	{MSR_LBR_INFO_5, 0, 0},
	{MSR_LBR_INFO_6, 0, 0},
	{MSR_LBR_INFO_7, 0, 0},
	{MSR_LBR_INFO_8, 0, 0},
	{MSR_LBR_INFO_9, 0, 0},
	{MSR_LBR_SELECT, 0, 0},
	{MSR_LER_FROM_LIP, 0, 0},
	{MSR_LER_TO_LIP, 0, 0},
	{MSR_MISC_FEATURE_CONTROL, 0, 0},
	{MSR_MISC_PWR_MGMT, 0, 0},
	{MSR_PEBS_ENABLE, 0, 0},
	{MSR_PEBS_FRONTEND, 0, 0},
	{MSR_PEBS_LD_LAT, 0, 0},
	{MSR_PERF_STATUS, 0, 0},
	{MSR_PKG_C2_RESIDENCY, 0, 0},
	{MSR_PKG_C3_RESIDENCY, 0, 0},
	{MSR_PKG_C6_RESIDENCY, 0, 0},
	{MSR_PKG_C7_RESIDENCY, 0, 0},
	{MSR_PKG_CST_CONFIG_CONTROL, 0, 0},
	{MSR_PKG_ENERGY_STATUS, 0, 0},
	{MSR_PKG_HDC_CONFIG, 0, 0},
	{MSR_PKG_HDC_CONFIG_CTL, 0, 0},
	{MSR_PKG_HDC_DEEP_RESIDENCY, 0, 0},
	{MSR_PKG_HDC_SHALLOW_RESIDENCY, 0, 0},
	{MSR_PKG_PERF_STATUS, 0, 0},
	{MSR_PKG_POWER_INFO, 0, 0},
	{MSR_PKG_POWER_LIMIT, 0, 0},
	{MSR_PKGC3_IRTL, 0, 0},
	{MSR_PKGC6_IRTL, 0, 0},
	{MSR_PKGC7_IRTL, 0, 0},
	{MSR_PLATFORM_ENERGY_COUNTER, 0, 0},
	{MSR_PLATFORM_POWER_LIMIT, 0, 0},
	{MSR_PMG_IO_CAPTURE_BASE, 0, 0},
	{MSR_POWER_CTL, 0, 0},
	{MSR_PP0_ENERGY_STATUS, 0, 0},
	{MSR_PP0_POLICY, 0, 0},
	{MSR_PP0_POWER_LIMIT, 0, 0},
	{MSR_PP1_ENERGY_STATUS, 0, 0},
	{MSR_PP1_POLICY, 0, 0},
	{MSR_PP1_POWER_LIMIT, 0, 0},
	{MSR_PPERF, 0, 0},
	{MSR_PRMRR_PHYS_BASE, 0, 0},
	{MSR_PRMRR_PHYS_MASK, 0, 0},
	{MSR_PRMRR_VALID_CONFIG, 0, 0},
	{MSR_RAPL_POWER_UNIT, 0, 0},
	{MSR_RING_PERF_LIMIT_REASONS, 0, 0},
	{MSR_RING_RATIO_LIMIT, 0, 0},
	{MSR_SGXOWNEREPOCH0, 0, 0},
	{MSR_SGXOWNEREPOCH1, 0, 0},
	{MSR_SMI_COUNT, 0, 0},
	{MSR_TEMPERATURE_TARGET, 0, 0},
	{MSR_TRACE_HUB_STH_ACPIBAR_BASE, 0, 0},
	{MSR_TURBO_ACTIVATION_RATIO, 0, 0},
	{MSR_TURBO_RATIO_LIMIT, 0, 0},
	{MSR_UNC_ARB_PERFCTR0, 0, 0},
	{MSR_UNC_ARB_PERFCTR1, 0, 0},
	{MSR_UNC_ARB_PERFEVTSEL0, 0, 0},
	{MSR_UNC_ARB_PERFEVTSEL1, 0, 0},
	{MSR_UNC_CBO_0_PERFCTR0, 0, 0},
	{MSR_UNC_CBO_0_PERFCTR1, 0, 0},
	{MSR_UNC_CBO_0_PERFEVTSEL0, 0, 0},
	{MSR_UNC_CBO_0_PERFEVTSEL1, 0, 0},
	{MSR_UNC_CBO_1_PERFCTR0, 0, 0},
	{MSR_UNC_CBO_1_PERFCTR1, 0, 0},
	{MSR_UNC_CBO_1_PERFEVTSEL0, 0, 0},
	{MSR_UNC_CBO_1_PERFEVTSEL1, 0, 0},
	{MSR_UNC_CBO_2_PERFCTR0, 0, 0},
	{MSR_UNC_CBO_2_PERFCTR1, 0, 0},
	{MSR_UNC_CBO_2_PERFEVTSEL0, 0, 0},
	{MSR_UNC_CBO_2_PERFEVTSEL1, 0, 0},
	{MSR_UNC_CBO_3_PERFCTR0, 0, 0},
	{MSR_UNC_CBO_3_PERFCTR1, 0, 0},
	{MSR_UNC_CBO_3_PERFEVTSEL0, 0, 0},
	{MSR_UNC_CBO_3_PERFEVTSEL1, 0, 0},
	{MSR_UNC_CBO_CONFIG, 0, 0},
	{MSR_UNC_PERF_FIXED_CTR, 0, 0},
	{MSR_UNC_PERF_FIXED_CTRL, 0, 0},
	{MSR_UNC_PERF_GLOBAL_CTRL, 0, 0},
	{MSR_UNC_PERF_GLOBAL_STATUS, 0, 0},
	{MSR_UNCORE_PRMRR_PHYS_BASE, 0, 0},
	{MSR_UNCORE_PRMRR_PHYS_MASK, 0, 0},
	{MSR_WEIGHTED_CORE_C0, 0, 0},
};

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_ap_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_misc_enable_bit22_init_value()
{
	if ((cpuid(0).a & 0xFF) > 2) {
		wrmsr(MSR_IA32_MISC_ENABLE, rdmsr(MSR_IA32_MISC_ENABLE) | (1ull << 22));
	}
}

__unused static void modify_misc_enable_bit34_init_value()
{
	wrmsr(MSR_IA32_MISC_ENABLE, rdmsr(MSR_IA32_MISC_ENABLE) | (1ull << 34));
}

__unused static void modify_ia32_efer_sce_init_value()
{
	wrmsr(IA32_EFER, rdmsr(IA32_EFER) | EFER_SCE);
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 38678:
		fp = modify_misc_enable_bit22_init_value;
		ap_init_value_process(fp);
		break;
	case 38682:
		fp = modify_misc_enable_bit34_init_value;
		ap_init_value_process(fp);
		break;
	case 38689:
		fp = modify_ia32_efer_sce_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif


void save_unchanged_reg(void)
{
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	if (ap_start_count == 0) {
		/* save init value (defalut is 1c000000000000H)*/
		init_platform_id = rdmsr(IA32_PLATFORM_ID);

		/* set new value to pat */
		wrmsr(IA32_PLATFORM_ID, 0xaa55);
		ap_start_count++;
	}

	set_platform_id = rdmsr(IA32_PLATFORM_ID);

	if (ap_start_count == 2) {
		/* resume environment */
		wrmsr(IA32_PLATFORM_ID, init_platform_id);
	}
}

/*P must be a non-null pointer */
static int read_memory_check(void *p)
{
	u64 value = 1;
	asm volatile(ASM_TRY("1f")
		"mov (%[p]), %[value]\n\t"
		"1:"
		: : [value]"r"(value), [p]"r"(p));
	return exception_vector();
}

void misc_msr_test_rdmsr(u32 msr, u32 vector, u32 error_code, const char *func)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(msr) : "memory");

	report("%s", (exception_vector() == vector) && (exception_error_code() == error_code), func);
}

void misc_msr_test_wrmsr(u32 msr, u32 vector, u32 error_code, const char *func)
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

int check_msr_reserve_bits_is_set(u32 msr, u32 start, u32 end)
{
	u64 check_value;
	u32 start_bit;
	u32 end_bit = end;

	check_value = rdmsr(msr);
	for (start_bit = start; start_bit <= end_bit; start_bit++) {
		if ((check_value & (1ULL << start_bit)) != 0) {
			return 1;
		}
	}
	return  0;
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts
 * to access guest MSR_TURBO_ACTIVATION_RATIO_001
 *
 * Summary:  Dump register MSR_TURBO_ACTIVATION_RATIO shall generate #GP(0).
 */
static void misc_msr_rqmid_38512_rd_MSR_TURBO_ACTIVATION_RATIO_001(void)
{
	misc_msr_test_rdmsr(MSR_TURBO_ACTIVATION_RATIO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts
 * to access guest MSR_TURBO_ACTIVATION_RATIO_002
 *
 * Summary: Execute WRMSR instruction to write MSR_TURBO_ACTIVATION_RATIO with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38514_wt_MSR_TURBO_ACTIVATION_RATIO_002(void)
{
	misc_msr_test_wrmsr(MSR_TURBO_ACTIVATION_RATIO, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts
 * to access guest MSR_UNC_ARB_PERFEVTSEL1_001
 *
 * Summary:  Dump register MSR_UNC_ARB_PERFEVTSEL1 shall generate #GP(0).
 */
static void misc_msr_rqmid_38515_rd_MSR_UNC_ARB_PERFEVTSEL1_001(void)
{
	misc_msr_test_rdmsr(MSR_UNC_ARB_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts
 * to access guest MSR_UNC_ARB_PERFEVTSEL1_002
 *
 * Summary: Execute WRMSR instruction to write MSR_UNC_ARB_PERFEVTSEL1 with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38516_wt_MSR_UNC_ARB_PERFEVTSEL1_002(void)
{
	misc_msr_test_wrmsr(MSR_UNC_ARB_PERFEVTSEL1, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access
 * guest IA32_SMBASE_001
 *
 * Summary:   Dump register IA32_SMBASE shall generate #GP(0).
 */
static void misc_msr_rqmid_38523_rd_IA32_SMBASE_001(void)
{
	misc_msr_test_rdmsr(IA32_SMBASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access
 * guest IA32_SMBASE_002
 *
 * Summary: Execute WRMSR instruction to write IA32_SMBASE with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38525_wt_IA32_SMBASE_002(void)
{
	misc_msr_test_wrmsr(IA32_SMBASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access
 * guest IA32_SMM_MONITOR_CTL_001
 *
 * Summary:   Dump register IA32_SMM_MONITOR_CTL shall generate #GP(0).
 */
static void misc_msr_rqmid_38528_rd_IA32_SMM_MONITOR_CTL_001(void)
{
	misc_msr_test_rdmsr(IA32_SMM_MONITOR_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access
 * guest IA32_SMM_MONITOR_CTL_002
 *
 * Summary: Execute WRMSR instruction to write IA32_SMM_MONITOR_CTL with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38529_wt_IA32_SMM_MONITOR_CTL_002(void)
{
	misc_msr_test_wrmsr(IA32_SMM_MONITOR_CTL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest
 * MSR_TRACE_HUB_STH_ACPIBAR_BASE_001
 *
 * Summary:   Dump register MSR_TRACE_HUB_STH_ACPIBAR_BASE shall generate #GP(0).
 */
static void misc_msr_rqmid_38530_rd_MSR_TRACE_HUB_STH_ACPIBAR_BASE_001(void)
{
	misc_msr_test_rdmsr(MSR_TRACE_HUB_STH_ACPIBAR_BASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest
 * MSR_TRACE_HUB_STH_ACPIBAR_BASE_002
 *
 * Summary: Execute WRMSR instruction to write MSR_TRACE_HUB_STH_ACPIBAR_BASE with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38531_wt_MSR_TRACE_HUB_STH_ACPIBAR_BASE_002(void)
{
	misc_msr_test_wrmsr(MSR_TRACE_HUB_STH_ACPIBAR_BASE, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest
 * IA32_BIOS_UPDT_TRIG_001
 *
 * Summary:   Dump register IA32_BIOS_UPDT_TRIG shall generate #GP(0).
 */
static void misc_msr_rqmid_38532_rd_IA32_BIOS_UPDT_TRIG_001(void)
{
	misc_msr_test_rdmsr(IA32_BIOS_UPDT_TRIG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest
 * IA32_BIOS_UPDT_TRIG_002
 *
 * Summary: Execute WRMSR instruction to write IA32_BIOS_UPDT_TRIG with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38533_wt_IA32_BIOS_UPDT_TRIG_002(void)
{
	misc_msr_test_wrmsr(IA32_BIOS_UPDT_TRIG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest
 * IA32_FEATURE_CONTROL_001
 *
 * Summary: Execute WRMSR instruction to write IA32_FEATURE_CONTROL with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38550_wt_IA32_FEATURE_CONTROL_001(void)
{
	misc_msr_test_wrmsr(IA32_FEATURE_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest
 * MSR_SMI_COUNT_001
 *
 * Summary:   Dump register MSR_SMI_COUNT shall generate #GP(0).
 */
static void misc_msr_rqmid_38551_rd_MSR_SMI_COUNT_001(void)
{
	misc_msr_test_rdmsr(MSR_SMI_COUNT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest
 * MSR_SMI_COUNT_002
 *
 * Summary: Execute WRMSR instruction to write MSR_SMI_COUNT with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38552_wt_MSR_SMI_COUNT_002(void)
{
	misc_msr_test_wrmsr(MSR_SMI_COUNT, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest
 * IA32_PLATFORM_ID_001
 *
 * Summary: Execute WRMSR instruction to write IA32_PLATFORM_ID with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38555_wt_IA32_PLATFORM_ID_001(void)
{
	misc_msr_test_wrmsr(IA32_PLATFORM_ID, GP_VECTOR, 0, __FUNCTION__);
}

static u32 misc_msr_test_msr_reserve_bits(u32 msr, u32 start, u32 end, u32 vector, u32 error_code)
{
	u32 a;
	u32 d;
	u32 check_bit;
	u64 msr_value;

	msr_value = rdmsr(msr);
	for (check_bit = start; check_bit <= end; check_bit++) {
		msr_value = msr_value ^ (1ul << check_bit);
		a = msr_value;
		d = (msr_value >> 32);
		asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(msr) : "memory");
		if ((exception_vector() != vector)
			&& (exception_error_code() != error_code)) {
			return false;
		}
	}
	return true;
}

static u32 misc_msr_access_reserve_msr(u32 msr)
{
	u32 a, d;

	asm volatile (ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(msr) : "memory");

	if (!((exception_vector() == GP_VECTOR) && (exception_error_code() == 0))) {
		return false;
	}

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(msr) : "memory");
	if ((exception_vector() != GP_VECTOR)
			&& (exception_error_code() != 0)) {
		return false;
	}

	return true;
}

/**
 * @brief case name: Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to change
 * reserve bits of any guest MSR_001
 *
 * Summary: When a vCPU attempts to change reserve bits of any guest MSR, ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0).
 */
static void misc_msr_rqmid_38556_wt_MSR_reserve_bits_001(void)
{
	u32 count;
	u32 index;

	count = sizeof(rvdbits)/sizeof(t_msrbits);
	for (index = 0; index < count; index++) {
		if (misc_msr_test_msr_reserve_bits(rvdbits[index].msr, rvdbits[index].start_bit,
			rvdbits[index].end_bit, GP_VECTOR, 0) == false) {
			report("%s(%d)", false, __FUNCTION__, index);
			return;
		}
	}

	count = sizeof(rvdmsr)/sizeof(t_msrbits);
	for (index = 0; index < count; index++) {
		if (misc_msr_access_reserve_msr(rvdmsr[index].msr) == false) {
			report("%s(%d)", false, __FUNCTION__, index);
			return;
		}
	}
	report("%s(%d)", true, __FUNCTION__, index);
}

/**
 * @brief case name: Guarantee the vCPU receives #GP(0) when a vCPU attempts to write IA32_EFER_001
 *
 * Summary: Execute WRMSR instruction to write IA32_EFER with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38558_wt_IA32_EFER_001(void)
{
	misc_msr_test_wrmsr(IA32_EFER, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write a
 * non-zero value to guest IA32_BIOS_SIGN_ID_001
 *
 * Summary: Execute WRMSR instruction to write IA32_BIOS_SIGN_ID with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38559_wt_IA32_BIOS_SIGN_ID_001(void)
{
	misc_msr_test_wrmsr(IA32_BIOS_SIGN_ID, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempt to write guest
 * IA32_MISC_ENABLE [bit 7]_001
 *
 * Summary: Execute WRMSR instruction to write IA32_MISC_ENABLE [bit 7] with different form old value
 * shall generate #GP(0).
 */
static void misc_msr_rqmid_38560_wt_IA32_MISC_ENABLE_bit7_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_EMON;
	report("%s",
		(wrmsr_checking(IA32_MISC_ENABLE, msr_ia32_misc_enable) == GP_VECTOR) &&
		(exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets the current value when a vCPU attempts to read
 * guest IA32_EFER_001
 *
 * Summary:   When a vCPU attempts to read guest IA32_EFER, ACRN hypervisor shall guarantee that
 * the vCPU gets the current value of guest IA32_EFER.
 */
static void misc_msr_rqmid_38567_rd_IA32_EFER_001(void)
{
	u64 msr_ia32_efer;
	report("%s", rdmsr_checking(IA32_EFER, &msr_ia32_efer) == NO_EXCEPTION, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets the current value when a vCPU attempts to read
 * guest IA32_MISC_ENABLE_001
 *
 * Summary: When a vCPU attempts to read guest IA32_MISC_ENABLE, ACRN hypervisor shall guarantee
 * that the vCPU gets the current value of guest IA32_MISC_ENABLE.
 */
static void misc_msr_rqmid_38572_rd_IA32_MISC_ENABLE_001(void)
{
	u64 msr_ia32_misc_enable;
	report("%s",
		rdmsr_checking(IA32_MISC_ENABLE, &msr_ia32_misc_enable) == NO_EXCEPTION, __FUNCTION__);
}

/**
 * @brief case name: Set this value to MSR_FEATURE_CONFIG when a vCPU attempts to write a value to
 * guest MSR_FEATURE_CONFIG_001
 *
 * Summary: Execute WRMSR instruction to write MSR_FEATURE_CONFIG with 1H shall generate #GP(0).
 */
static void misc_msr_rqmid_38579_wt_MSR_FEATURE_CONFIG_001(void)
{
	misc_msr_test_wrmsr(MSR_FEATURE_CONFIG, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets the current value of guest MSR_FEATURE_CONFIG
 * when a vCPU attempts to read guest MSR_FEATURE_CONFIG_002
 *
 * Summary: When a vCPU attempts to read guest MSR_FEATURE_CONFIG, ACRN hypervisor shall
 * guarantee that the vCPU gets 1H.
 */
static void misc_msr_rqmid_38590_rd_MSR_FEATURE_CONFIG_002(void)
{
	report("%s", rdmsr(MSR_FEATURE_CONFIG) == 0x1, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets b400000000H when a vCPU attempts to read guest
 * IA32_BIOS_SIGN_ID_001
 *
 * Summary: When a vCPU attempts to read guest IA32_BIOS_SIGN_ID, ACRN hypervisor shall guarantee
 * that the vCPU gets b400000000H..
 */
static void misc_msr_rqmid_38597_rd_IA32_BIOS_SIGN_ID_001(void)
{
	report("%s", rdmsr(IA32_BIOS_SIGN_ID) == 0xb400000000, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets 1H when a vCPU attempts to read guest
 *IA32_FEATURE_CONTROL [bit 0]_001
 *
 * Summary: When a vCPU attempts to read guest IA32_FEATURE_CONTROL, ACRN hypervisor
 * shall guarantee that the guest IA32_FEATURE_CONTROL [bit 0] is 1H.
 */
static void misc_msr_rqmid_38606_rd_IA32_FEATURE_CONTROL_001(void)
{
	u64 result = 0;
	result = rdmsr(IA32_FEATURE_CONTROL);
	report("%s", (result & 0x1) == 0x1, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets  1c000000000000H when a vCPU attempts to read
 * guest IA32_PLATFORM_ID_001
 *
 * Summary: When a vCPU attempts to read guest IA32_PLATFORM_ID, ACRN hypervisor shall guarantee
 * that the vCPU gets 1c000000000000H..
 */
static void misc_msr_rqmid_38610_rd_IA32_PLATFORM_ID_001(void)
{
	u64 result = 0;
	result = rdmsr(IA32_PLATFORM_ID);
	report("%s", result == 0x1c000000000000, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU gets 0H when a vCPU attempts to read guest
 * IA32_MONITOR_FILTER_SIZE_001
 *
 * Summary: When a vCPU attempts to read guest IA32_MONITOR_FILTER_SIZE, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static void misc_msr_rqmid_38611_rd_IA32_MONITOR_FILTER_SIZE_001(void)
{
	u64 result = 0;
	result = rdmsr(IA32_MONITOR_FILTER_SIZE);
	report("%s", result == 0, __FUNCTION__);
}

/**
 * @brief case name: Ignore this access when a vCPU attempts to write guest IA32_SPEC_CTRL_001
 *
 * Summary: When a vCPU attempts to write guest IA32_SPEC_CTRL and the new guest IA32_SPEC_CTRL [bit 1]
 * is different from the old guest IA32_SPEC_CTRL [bit 1], ACRN hypervisor shall ignore this access.
 */
static void misc_msr_rqmid_38612_wt_IA32_SPEC_CTRL_bit1_001(void)
{
	u64 msr_ia32_spec_ctrl;

	msr_ia32_spec_ctrl = rdmsr(IA32_SPEC_CTRL);
	wrmsr(IA32_SPEC_CTRL, msr_ia32_spec_ctrl ^ (1 << 1));
	if (rdmsr(IA32_SPEC_CTRL) == msr_ia32_spec_ctrl) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: IA32_MISC_ENABLE [bit 22] is read-write_001
 *
 * Summary: ACRN hypervisor shall guarantee guest IA32_MISC_ENABLE [bit 22] is a read-write bit.
 */
static void misc_msr_rqmid_38613_wt_IA32_MISC_ENABLE_bit22_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable ^ MSR_IA32_MISC_ENABLE_LIMIT_CPUID);
	if (rdmsr(IA32_MISC_ENABLE) != msr_ia32_misc_enable) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: IA32_EFER.SCE is read-write_001
 *
 * Summary: ACRN hypervisor shall guarantee guest IA32_EFER.SCE is a read-write bit.
 */
static void misc_msr_rqmid_38614_wt_IA32_EFER_SCE_001(void)
{
	u64 msr_ia32_efer;

	msr_ia32_efer = rdmsr(IA32_EFER);
	wrmsr(IA32_EFER, msr_ia32_efer ^ EFER_SCE);
	if (rdmsr(IA32_EFER) != msr_ia32_efer) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Guarantee IA32_MISC_ENABLE [bit 34] to be set when a vCPU attempts
 * to write guest IA32_MISC_ENABLE [bit 34]_001
 *
 * Summary: When a vCPU attempts to write a value to guest IA32_MISC_ENABLE and the new guest
 * IA32_MISC_ENABLE [bit 34] is different from the old guest IA32_MISC_ENABLE [bit 34],
 * ACRN hypervisor shall guarantee the new value is set to guest IA32_MISC_ENABLE [bit 34].
 */
static void misc_msr_rqmid_38617_wt_IA32_MISC_ENABLE_bit34_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable ^ MSR_IA32_MISC_ENABLE_XD_DISABLE);
	if (rdmsr(IA32_MISC_ENABLE) != msr_ia32_misc_enable) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Clear related CPUID when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001
 *
 * Summary: When a vCPU attempts to write IA32_MISC_ENABLE, the old guest IA32_MISC_ENABLE [bit 34] is
 * 0H and the new guest IA32_MISC_ENABLE [bit 34] is 1H, ACRN hypervisor shall write 0H to guest
 * CPUID.80000001H:EDX [bit 20].
 */
static void misc_msr_rqmid_38636_wt_IA32_MISC_ENABLE_CPUID_clear_80000001H_EDX_bit20_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable & (~(MSR_IA32_MISC_ENABLE_XD_DISABLE));
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);
	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable | (MSR_IA32_MISC_ENABLE_XD_DISABLE);
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);
	report("%s", ((cpuid_indexed(0x80000001, 0).d >> 20) & 1) == 0, __FUNCTION__);
}

/**
 * @brief case name: Set related CPUID when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001
 *
 * Summary: When a vCPU attempts to write IA32_MISC_ENABLE, the old guest IA32_MISC_ENABLE [bit 34]
 * is 1H and the new guest IA32_MISC_ENABLE [bit 34] is 0H, ACRN hypervisor shall write 1H to guest
 * CPUID.80000001H:EDX [bit 20].
 */
static void misc_msr_rqmid_38640_wt_IA32_MISC_ENABLE_CPUID_set_80000001H_EDX_bit20_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable | (MSR_IA32_MISC_ENABLE_XD_DISABLE);
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);
	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable & (~(MSR_IA32_MISC_ENABLE_XD_DISABLE));
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);
	report("%s", ((cpuid_indexed(0x80000001, 0).d >> 20) & 1) == 1, __FUNCTION__);
}

/**
 * @brief case name: Clear IA32_EFER.NXE when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001
 *
 * Summary: When a vCPU attempts to write IA32_MISC_ENABLE, the old guest IA32_MISC_ENABLE [bit 34] is 0H,
 * the old guest IA32_EFER.NXE is 1H and the new guest IA32_MISC_ENABLE [bit 34] is 1H, ACRN hypervisor shall
 * set 0H to guest IA32_EFER.NXE.
 */
static void misc_msr_rqmid_38642_wt_IA32_MISC_ENABLE_clear_IA32_EFER_NXE_001(void)
{
	u64 msr_ia32_misc_enable;
	u64 msr_ia32_efer;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable & (~(MSR_IA32_MISC_ENABLE_XD_DISABLE));
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);
	msr_ia32_efer = rdmsr(IA32_EFER);
	msr_ia32_efer = msr_ia32_efer | (IA32_EFER_NXE);
	wrmsr(IA32_EFER, msr_ia32_efer);

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable | (MSR_IA32_MISC_ENABLE_XD_DISABLE);
	wrmsr(IA32_MISC_ENABLE, msr_ia32_misc_enable);

	msr_ia32_efer = rdmsr(IA32_EFER);

	if ((msr_ia32_efer & IA32_EFER_NXE) == 0) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 22] to 0H following INIT_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 22] at AP entry, the result shall be 0H.
 */
static void misc_msr_rqmid_38678_ia32_misc_enable_bit22_following_init_001(void)
{
	bool is_pass = true;

	if ((*(volatile u32 *)INIT_IA32_MISC_ENABLE_LOW_ADDR & (MSR_IA32_MISC_ENABLE_LIMIT_CPUID)) != 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(38678);

	if ((*(volatile u32 *)INIT_IA32_MISC_ENABLE_LOW_ADDR & (MSR_IA32_MISC_ENABLE_LIMIT_CPUID)) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);

}

/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 7] to 0H following INIT_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 7] at AP entry, the result shall be 0H.
 */
static void misc_msr_rqmid_38680_ia32_misc_enable_bit7_following_init_001(void)
{
	report("%s", (*(volatile u32 *)INIT_IA32_MISC_ENABLE_LOW_ADDR & (MSR_IA32_MISC_ENABLE_EMON)) == 0,
	__FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 34] to 0H following INIT_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 34] at AP entry, the result shall be 0H.
 */
static void misc_msr_rqmid_38682_ia32_misc_enable_bit34_following_init_001(void)
{
	bool is_pass = true;
	if (((*(volatile uint64_t *)INIT_IA32_MISC_ENABLE_LOW_ADDR) & MSR_IA32_MISC_ENABLE_XD_DISABLE) != 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(38682);
	if (((*(volatile uint64_t *)INIT_IA32_MISC_ENABLE_LOW_ADDR) & MSR_IA32_MISC_ENABLE_XD_DISABLE) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);

}

/**
 * @brief case name: Set initial guest IA32_EFER_SCE to 0H following INIT_001
 * Summary: Check the initial value of IA32_EFER_SCE at AP entry, the result shall be 0H.
 */
static void misc_msr_rqmid_38689_IA32_EFER_SCE_following_init_001(void)
{
	bool is_pass = true;

	if ((*(volatile u32 *)INIT_IA32_EFER_LOW_ADDR & (EFER_SCE)) != 0) {
		is_pass = false;
	}
	notify_ap_modify_and_read_init_value(38689);
	if ((*(volatile u32 *)INIT_IA32_EFER_LOW_ADDR & (EFER_SCE)) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest MSR_FEATURE_CONFIG to 1H following INIT_001
 * Summary: ACRN hypervisor shall set initial guest MSR_FEATURE_CONFIG to 1H following INIT.
 */
static void misc_msr_rqmid_38695_MSR_FEATURE_CONFIG_following_init_001(void)
{
	volatile u32 *ptr = (volatile u32 *)INIT_MSR_FEATURE_CONFIG_LOW_ADDR;
	u64 ia32_feature_config;

	ia32_feature_config = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ia32_feature_config == 1, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_FEATURE_CONTROL [bit 0] to 1H following INIT_001
 * Summary: Check the initial value of IA32_FEATURE_CONTROL [bit 0] at AP entry, the result shall be 1H.
 */
static void misc_msr_rqmid_38702_IA32_FEATURE_CONTROL_following_init_001(void)
{
	report("%s", (*(volatile u32 *)INIT_IA32_FEATURE_CONTROL_LOW_ADDR & FEATURE_CONTROL_LOCK_BIT) == 1,
	__FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_BIOS_SIGN_ID to b400000000H following INIT_001
 * Summary: Check the initial value of IA32_BIOS_SIGN_ID  at AP entry, the result shall be b400000000H.
 */
static void misc_msr_rqmid_38716_IA32_BIOS_SIGN_ID_following_init_001(void)
{
	volatile u32 *ptr = (volatile u32 *)INIT_IA32_BIOS_SIGN_ID_LOW_ADDR;
	u64 ia32_bios_sign_id;

	ia32_bios_sign_id = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ia32_bios_sign_id == 0xb400000000ull, __FUNCTION__);
}

/**
 * @brief case name: Keep guest IA32_PLATFORM_ID unchanged following INIT_001.
 * Summary: After AP receives first INIT, set the PLATFORM_ID value to aa55H;
 * dump IA32_PLATFORM_ID register value, AP INIT again, then dump IA32_PLATFORM_ID
 * register value again, two dumps value should be equal.
 */
static void misc_msr_rqmid_38718_IA32_PLATFORM_ID_unchanged_following_INIT_001(void)
{
	volatile u64 ia32_platform_id1;
	volatile u64 ia32_platform_id2;

	/*get set_platform_id value */
	ia32_platform_id1 = set_platform_id;

	/*send sipi to ap*/
	send_sipi();

	/*get set_platform_id value again after ap reset*/
	ia32_platform_id2 = set_platform_id;

	/*compare init value with unchanged */
	report("%s ", ia32_platform_id1 == ia32_platform_id2, __FUNCTION__);
}
#endif

/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 22]  to 0H following start-up_001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 22] at BP start-up, the result shall be 0H.
 */
static void misc_msr_rqmid_38679_ia32_misc_enable_bit22_following_start_up_001(void)
{
	report("%s", (*(volatile u32 *)STARTUP_IA32_MISC_ENABLE_LOW_ADDR & MSR_IA32_MISC_ENABLE_LIMIT_CPUID) == 0,
	__FUNCTION__);

}

/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 7]  to 0H following start-up_001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 7] at BP start-up, the result shall be 0H.
 */
static void misc_msr_rqmid_38681_ia32_misc_enable_bit7_following_start_up_001(void)
{
	report("%s", (*(volatile u32 *)STARTUP_IA32_MISC_ENABLE_LOW_ADDR & MSR_IA32_MISC_ENABLE_EMON) == 0,
	__FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_MISC_ENABLE [bit 34]  to 0H following start-up_001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 34] at BP start-up, the result shall be 0H.
 */
static void misc_msr_rqmid_38683_ia32_misc_enable_bit34_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_IA32_MISC_ENABLE_LOW_ADDR;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", (ia32_init_first & MSR_IA32_MISC_ENABLE_XD_DISABLE) == 0, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_EFER.SCE to 0H following start-up_001
 * Summary: Read the initial value of IA32_EFER.SCE at BP start-up, the result shall be 0H.
 */
static void misc_msr_rqmid_38690_IA32_EFER_SCE_following_start_up_001(void)
{
	report("%s", (*(volatile u32 *)STARTUP_IA32_EFER_LOW_ADDR & EFER_SCE) == 0, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest MSR_FEATURE_CONFIG to 1H following start-up_001
 * Summary: ACRN hypervisor shall set initial guest MSR_FEATURE_CONFIG to 1H following start-up.
 */
static void misc_msr_rqmid_38696_MSR_FEATURE_CONFIG_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_MSR_FEATURE_CONFIG_LOW_ADDR;
	u64 ia32_feature_config;

	ia32_feature_config = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ia32_feature_config == 1, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_FEATURE_CONTROL [bit 0] to 1H following start-up_001
 * Summary: Read the initial value of IA32_FEATURE_CONTROL [bit 0] at BP start-up, the result shall be 1H.
 */
static void misc_msr_rqmid_38703_IA32_FEATURE_CONTROL_following_start_up_001(void)
{
	report("%s", (*(volatile u32 *)STARTUP_IA32_FEATURE_CONTROL_LOW_ADDR & FEATURE_CONTROL_LOCK_BIT) == 1,
	__FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_PLATFORM_ID to 1c000000000000H following start-up_001
 * Summary: Read the initial value of IA32_PLATFORM_ID at BP start-up, the result shall be 1c000000000000H.
 */
static void misc_msr_rqmid_38712_IA32_PLATFORM_ID_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_IA32_PLATFORM_ID_LOW_ADDR;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ia32_init_first == 0x1c000000000000ull, __FUNCTION__);
}

/**
 * @brief case name: Set initial guest IA32_BIOS_SIGN_ID to b400000000H following start-up_001
 * Summary: Check the initial value of IA32_BIOS_SIGN_ID  at BP start-up, the result shall be b400000000H.
 */
static void misc_msr_rqmid_38717_IA32_BIOS_SIGN_ID_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_IA32_BIOS_SIGN_ID_LOW_ADDR;
	u64 ia32_bios_sign_id;

	ia32_bios_sign_id = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ia32_bios_sign_id == 0xb400000000ull, __FUNCTION__);
}

/**
 * @brief case name:Flush TLB when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1, after clear P bit
 * in PTE related with gva1. we still can access gva1 normally for paging frame information
 * is cached in TLB. After changing IA32_MISC_ENABLE [bit34], we will get #PF because TLB is
 * invalidated and get PTE directly from memory.
 */
static void misc_msr_rqmid_38736_change_IA32_MISC_ENABLE_bit34_invalidate_tlb(void)
{
	u8 *gva = malloc(sizeof(u8));
	u8 result = 0;
	//u64 msr_ia32_misc_enable;

	if (gva == NULL) {
		printf("malloc error!\n");
		return;
	}

	/*make sure that the old guest IA32_MISC_ENABLE [bit 34] is 0H*/
	if (rdmsr(IA32_MISC_ENABLE) & MSR_IA32_MISC_ENABLE_XD_DISABLE) {
		wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_XD_DISABLE);
	}
	/*make sure that the old guest IA32_EFER.NXE is 1H*/
	if ((rdmsr(IA32_EFER) & IA32_EFER_NXE) == 0) {
		wrmsr(IA32_EFER, rdmsr(IA32_EFER) | IA32_EFER_NXE);
	}

	*gva = 0x12;
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == 0x12) {
		result++;
	} else {
		//printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_XD_DISABLE);
	if (read_memory_check((void *)gva) == PF_VECTOR) {
		result++;
	} else {
		//printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(gva);
}


/**
 * @brief case name:Flush paging structure cache when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]
 *
 * Summary: Config 4-level paging structure, make sure two gva: gva1 and gva2 in same PT. Then write value
 * to gva1, after clear P bit in PDE related with gva1 and gva2. we still can access gva2 normally for
 * paging-structure information is cached in paging-structure cache. After changing IA32_MISC_ENABLE,
 * we will get #PF because paging-structure cache is invalidated and get PDE directly from memory.
 */
static void misc_msr_rqmid_38742_change_IA32_MISC_ENABLE_bit34_invalidate_paging_struct(void)
{
	u8 result = 0;
	//u64 msr_ia32_misc_enable;
	ALIGNED(4096)u8 *gva = malloc(sizeof(u8)*64);

	/*make sure that the old guest IA32_MISC_ENABLE [bit 34] is 0H*/
	if (rdmsr(IA32_MISC_ENABLE) & MSR_IA32_MISC_ENABLE_XD_DISABLE) {
		wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_XD_DISABLE);
	}

	/*make sure that the old guest IA32_EFER.NXE is 1H*/
	if ((rdmsr(IA32_EFER) & IA32_EFER_NXE) == 0) {
		wrmsr(IA32_EFER, rdmsr(IA32_EFER) | IA32_EFER_NXE);
	}

	u8 *gva1 = gva;
	u8 *gva2 = gva+1;

	*gva1 = 0x12;
	set_page_control_bit((void *)gva1, PAGE_PDE, PAGE_P_FLAG, 0, false);
	*gva2 = 0x55;
	result++;

	wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_XD_DISABLE);

	if (read_memory_check((void *)gva1) == PF_VECTOR) {
		result++;
	} else {
		//printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);
	set_page_control_bit(gva, PAGE_PDE, PAGE_P_FLAG, 1, true);

}

/**
 * @brief case name:Guarantee that the vCPU will gets the current value of guest reserve bits of this MSR when
 * a vCPU attempts to read reserve bits of any guest impletemented MSR_001
 *
 * Summary: When a vCPU attempts to read reserve bits of any guest implemented MSR, ACRN hypervisor shall
 * guarantee that the vCPU will gets the current value of guest reserve bits of this MSR.
 */
static void misc_msr_rqmid_40268_read_msr_reserve_bits(void)
{
	u32 count;
	u32 index;

	t_msrbits rvdbits[] = {
			{IA32_MISC_ENABLE, 1, 2},
			{IA32_MISC_ENABLE, 4, 6},
			{IA32_MISC_ENABLE, 8, 10},
			{IA32_MISC_ENABLE, 13, 15},
			{IA32_MISC_ENABLE, 17, 17},
			{IA32_MISC_ENABLE, 19, 21},
			{IA32_MISC_ENABLE, 24, 33},
			{IA32_MISC_ENABLE, 39, 63},
			{IA32_APIC_BASE, 0, 7},
			{IA32_APIC_BASE, 9, 9},
			{IA32_APIC_BASE, 63, 63},
			{IA32_MCG_STATUS, 4, 63},
			{IA32_PAT, 3, 7},
			{IA32_PAT, 11, 15},
			{IA32_PAT, 19, 23},
			{IA32_PAT, 27, 31},
			{IA32_PAT, 35, 39},
			{IA32_PAT, 43, 47},
			{IA32_PAT, 51, 55},
			{IA32_PAT, 59, 63},
			{IA32_BIOS_SIGN_ID, 0, 31},
			{IA32_EFER, 1, 7},
			{IA32_EFER, 9, 9},
			{IA32_EFER, 12, 63},
			{IA32_FEATURE_CONTROL, 3, 7},
			{IA32_FEATURE_CONTROL, 16, 16},
			{IA32_FEATURE_CONTROL, 19, 19},
			{IA32_FEATURE_CONTROL, 21, 63},
			{IA32_MCG_CAP, 12, 15},
			{IA32_MCG_CAP, 25, 25},
			{IA32_MCG_CAP, 28, 63},
			{IA32_PLATFORM_ID, 0, 49},
			{IA32_PLATFORM_ID, 53, 63},
			{MSR_FEATURE_CONFIG, 2, 63},
			{MSR_PLATFORM_INFO, 0, 7},
	};

	count = sizeof(rvdbits)/sizeof(t_msrbits);
	for (index = 0; index < count; index++) {
		if (check_msr_reserve_bits_is_set(rvdbits[index].msr,
			rvdbits[index].start_bit, rvdbits[index].end_bit)) {
			report("%s[%d]", false, __FUNCTION__, index);
			return;
		}
	}
	report("%s", true, __FUNCTION__);
}

/**
 * @brief case name: Access guest MSR_MISC_FEATURE_CONTROL_001
 *
 * Summary:   Access register MSR_MISC_FEATURE_CONTROL shall generate #GP(0).
 */
static void misc_msr_rqmid_46542_rd_MSR_MISC_FEATURE_CONTROL_001(void)
{
	misc_msr_test_rdmsr(MSR_MISC_FEATURE_CONTROL, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Access guest IA32_CPU_DCA_CAP_001
 *
 * Summary:   Access register IA32_CPU_DCA_CAP shall generate #GP(0).
 */
static void misc_msr_rqmid_46543_rd_IA32_CPU_DCA_CAP_001(void)
{
	misc_msr_test_rdmsr(IA32_CPU_DCA_CAP, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Access guest IA32_DCA_0_CAP_001
 *
 * Summary:   Access register IA32_DCA_0_CAP shall generate #GP(0).
 */
static void misc_msr_rqmid_46544_rd_IA32_DCA_0_CAP_001(void)
{
	misc_msr_test_rdmsr(IA32_DCA_0_CAP, GP_VECTOR, 0, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the write is ignored when a vCPU attempts to write 0H to
 * guest IA32_BIOS_SIGN_ID_001
 *
 * Summary: When a vCPU attempts to write 0H guest  IA32_BIOS_SIGN_ID,
 *  ACRN hypervisor shall ignore this access.
 */
static void misc_msr_rqmid_46545_wt_IA32_BIOS_SIGN_ID_001(void)
{
	u64 msr_ia32_efer;
	msr_ia32_efer = rdmsr(IA32_BIOS_SIGN_ID);
	wrmsr(IA32_BIOS_SIGN_ID, 0);
	if (rdmsr(IA32_BIOS_SIGN_ID) == msr_ia32_efer) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}


static void print_case_list(void)
{
	printf("PMU feature case list:\n\r");

	printf("\t Case ID:%d case name:%s\n\r", 38512,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"MSR_TURBO_ACTIVATION_RATIO_001");
	printf("\t Case ID:%d case name:%s\n\r", 38514,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"MSR_TURBO_ACTIVATION_RATIO_002");
	printf("\t Case ID:%d case name:%s\n\r", 38515,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"MSR_UNC_ARB_PERFEVTSEL1_001");
	printf("\t Case ID:%d case name:%s\n\r", 38516,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"MSR_UNC_ARB_PERFEVTSEL1_002");
	printf("\t Case ID:%d case name:%s\n\r", 38523,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"IA32_SMBASE_001");
	printf("\t Case ID:%d case name:%s\n\r", 38525,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"IA32_SMBASE_002");
	printf("\t Case ID:%d case name:%s\n\r", 38528,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"IA32_SMM_MONITOR_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 38529,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest "
	"IA32_SMM_MONITOR_CTL_002");
	printf("\t Case ID:%d case name:%s\n\r", 38530,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest "
	"MSR_TRACE_HUB_STH_ACPIBAR_BASE_001");
	printf("\t Case ID:%d case name:%s\n\r", 38531,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest "
	"MSR_TRACE_HUB_STH_ACPIBAR_BASE_002");
	printf("\t Case ID:%d case name:%s\n\r", 38532,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest IA32_BIOS_UPDT_TRIG_001");
	printf("\t Case ID:%d case name:%s\n\r", 38533,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access guest IA32_BIOS_UPDT_TRIG_002");
	printf("\t Case ID:%d case name:%s\n\r", 38550,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest IA32_FEATURE_CONTROL_001");
	printf("\t Case ID:%d case name:%s\n\r", 38551,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest MSR_SMI_COUNT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38552,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to access guest MSR_SMI_COUNT_002");
	printf("\t Case ID:%d case name:%s\n\r", 38555,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest IA32_PLATFORM_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 38556,
	"Guarantee that the vCPU will receive #GP(0) when a vCPU attempts to change reserve bits of any guest MSR_001");
	printf("\t Case ID:%d case name:%s\n\r", 38558,
	"Guarantee the vCPU receives #GP(0) when a vCPU attempts to write IA32_EFER_001");
	printf("\t Case ID:%d case name:%s\n\r", 38559,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write a non-zero value to "
	"guest IA32_BIOS_SIGN_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 38560,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempt to write guest IA32_MISC_ENABLE [bit 7]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38567,
	"Guarantee that the vCPU gets the current value when a vCPU attempts to read guest IA32_EFER_001");
	printf("\t Case ID:%d case name:%s\n\r", 38572,
	"Guarantee that the vCPU gets the current value when a vCPU attempts to read guest IA32_MISC_ENABLE_001");
	printf("\t Case ID:%d case name:%s\n\r", 38579,
	"Set this value to MSR_FEATURE_CONFIG when a vCPU attempts to write a value to guest "
	"MSR_FEATURE_CONFIG_001");
	printf("\t Case ID:%d case name:%s\n\r", 38590, "Guarantee that the vCPU gets the current value of guest "
	"MSR_FEATURE_CONFIG when a vCPU attempts to read guest MSR_FEATURE_CONFIG_001");
	printf("\t Case ID:%d case name:%s\n\r", 38597,
	"Guarantee that the vCPU gets b400000000H when a vCPU attempts to read guest IA32_BIOS_SIGN_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 38606,
	"Guarantee that the vCPU gets 1H when a vCPU attempts to read guest IA32_FEATURE_CONTROL [bit 0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38610,
	"Guarantee that the vCPU gets  1c000000000000H when a vCPU attempts to read guest IA32_PLATFORM_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 38611,
	"Guarantee that the vCPU gets 0H when a vCPU attempts to read guest IA32_MONITOR_FILTER_SIZE_001");
	printf("\t Case ID:%d case name:%s\n\r", 38612,
	"Ignore this access when a vCPU attempts to write guest IA32_SPEC_CTRL_001");
	printf("\t Case ID:%d case name:%s\n\r", 38613, "IA32_MISC_ENABLE [bit 22] is read-write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38614, "IA32_EFER.SCE is read-write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38617,
	"Guarantee IA32_MISC_ENABLE [bit 34] to be set when a vCPU attempts to write guest IA32_MISC_ENABLE [bit 34]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38636,
	"Clear related CPUID when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38640,
	"Set related CPUID when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38642,
	"Clear IA32_EFER.NXE when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 38678,
	"Set initial guest IA32_MISC_ENABLE [bit 22] to 0H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38680,
	"Set initial guest IA32_MISC_ENABLE [bit 7] to 0H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38682,
	"Set initial guest IA32_MISC_ENABLE [bit 34] to 0H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38689,
	"Set initial guest IA32_EFER_SCE to 0H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38695,
	"Set initial guest IA32_EFER_SCE to 0H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38702,
	"Set initial guest IA32_FEATURE_CONTROL [bit 0] to 1H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38716,
	"Set initial guest IA32_BIOS_SIGN_ID to b400000000H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38718,
	"Keep guest IA32_PLATFORM_ID unchanged following INIT_001.");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 38679,
	"Set initial guest IA32_MISC_ENABLE [bit 22]  to 0H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38681,
	"Set initial guest IA32_MISC_ENABLE [bit 7]  to 0H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38683,
	"Set initial guest IA32_MISC_ENABLE [bit 34]  to 0H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38690,
	"Set initial guest IA32_EFER.SCE to 0H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38696,
	"Set initial guest MSR_FEATURE_CONFIG to 1H following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 38703,
	"Set initial guest IA32_FEATURE_CONTROL [bit 0] to 1H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38712,
	"Set initial guest IA32_PLATFORM_ID to 1c000000000000H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38717,
	"Set initial guest IA32_BIOS_SIGN_ID to b400000000H following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 38736,
	"Flush TLB when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001");
	printf("\t Case ID:%d case name:%s\n\r", 38742,
	"Flush paging structure cache when a vCPU attempts to write IA32_MISC_ENABLE [bit 34]_001");
	printf("\t Case ID:%d case name:%s %s\n\r", 40268,
	"Guarantee that the vCPU will gets the current value of guest reserve bits of this MSR when",
	"a vCPU attempts to read reserve bits of any guest impletemented MSR_001");
	printf("\t Case ID:%d case name:%s\n\r", 46545,
	"Guarantee that the write is ignored when a vCPU attempts to write 0H to guest IA32_BIOS_SIGN_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 46542,	"Access guest MSR_MISC_FEATURE_CONTROL_001");
	printf("\t Case ID:%d case name:%s\n\r", 46543,	"Access guest IA32_CPU_DCA_CAP_001");
	printf("\t Case ID:%d case name:%s\n\r", 46544,	"Access guest IA32_DCA_0_CAP_001");
}

int main(void)
{
	setup_vm();
	setup_idt();
	print_case_list();

	misc_msr_rqmid_38512_rd_MSR_TURBO_ACTIVATION_RATIO_001();
	misc_msr_rqmid_38514_wt_MSR_TURBO_ACTIVATION_RATIO_002();
	misc_msr_rqmid_38515_rd_MSR_UNC_ARB_PERFEVTSEL1_001();
	misc_msr_rqmid_38516_wt_MSR_UNC_ARB_PERFEVTSEL1_002();
	misc_msr_rqmid_38523_rd_IA32_SMBASE_001();
	misc_msr_rqmid_38525_wt_IA32_SMBASE_002();
	misc_msr_rqmid_38528_rd_IA32_SMM_MONITOR_CTL_001();
	misc_msr_rqmid_38529_wt_IA32_SMM_MONITOR_CTL_002();
	misc_msr_rqmid_38530_rd_MSR_TRACE_HUB_STH_ACPIBAR_BASE_001();
	misc_msr_rqmid_38531_wt_MSR_TRACE_HUB_STH_ACPIBAR_BASE_002();
	misc_msr_rqmid_38532_rd_IA32_BIOS_UPDT_TRIG_001();
	misc_msr_rqmid_38533_wt_IA32_BIOS_UPDT_TRIG_002();
	misc_msr_rqmid_38550_wt_IA32_FEATURE_CONTROL_001();
	misc_msr_rqmid_38551_rd_MSR_SMI_COUNT_001();
	misc_msr_rqmid_38552_wt_MSR_SMI_COUNT_002();
	misc_msr_rqmid_38555_wt_IA32_PLATFORM_ID_001();
	misc_msr_rqmid_38556_wt_MSR_reserve_bits_001();
	misc_msr_rqmid_38558_wt_IA32_EFER_001();
	misc_msr_rqmid_38559_wt_IA32_BIOS_SIGN_ID_001();
	misc_msr_rqmid_38560_wt_IA32_MISC_ENABLE_bit7_001();
	misc_msr_rqmid_38567_rd_IA32_EFER_001();
	misc_msr_rqmid_38572_rd_IA32_MISC_ENABLE_001();
	misc_msr_rqmid_38579_wt_MSR_FEATURE_CONFIG_001();
	misc_msr_rqmid_38590_rd_MSR_FEATURE_CONFIG_002();
	misc_msr_rqmid_38597_rd_IA32_BIOS_SIGN_ID_001();
	misc_msr_rqmid_38606_rd_IA32_FEATURE_CONTROL_001();
	misc_msr_rqmid_38610_rd_IA32_PLATFORM_ID_001();
	misc_msr_rqmid_38611_rd_IA32_MONITOR_FILTER_SIZE_001();
	misc_msr_rqmid_38612_wt_IA32_SPEC_CTRL_bit1_001();
	misc_msr_rqmid_38613_wt_IA32_MISC_ENABLE_bit22_001();
	misc_msr_rqmid_38614_wt_IA32_EFER_SCE_001();
	misc_msr_rqmid_38617_wt_IA32_MISC_ENABLE_bit34_001();
	misc_msr_rqmid_38636_wt_IA32_MISC_ENABLE_CPUID_clear_80000001H_EDX_bit20_001();
	misc_msr_rqmid_38640_wt_IA32_MISC_ENABLE_CPUID_set_80000001H_EDX_bit20_001();
	misc_msr_rqmid_38642_wt_IA32_MISC_ENABLE_clear_IA32_EFER_NXE_001();
#ifdef IN_NON_SAFETY_VM
	misc_msr_rqmid_38678_ia32_misc_enable_bit22_following_init_001();
	misc_msr_rqmid_38680_ia32_misc_enable_bit7_following_init_001();
	misc_msr_rqmid_38682_ia32_misc_enable_bit34_following_init_001();
	misc_msr_rqmid_38689_IA32_EFER_SCE_following_init_001();
	misc_msr_rqmid_38695_MSR_FEATURE_CONFIG_following_init_001();
	misc_msr_rqmid_38702_IA32_FEATURE_CONTROL_following_init_001();
	misc_msr_rqmid_38716_IA32_BIOS_SIGN_ID_following_init_001();
	misc_msr_rqmid_38718_IA32_PLATFORM_ID_unchanged_following_INIT_001();
#endif
	misc_msr_rqmid_38679_ia32_misc_enable_bit22_following_start_up_001();
	misc_msr_rqmid_38681_ia32_misc_enable_bit7_following_start_up_001();
	misc_msr_rqmid_38683_ia32_misc_enable_bit34_following_start_up_001();
	misc_msr_rqmid_38690_IA32_EFER_SCE_following_start_up_001();
	misc_msr_rqmid_38696_MSR_FEATURE_CONFIG_following_start_up_001();
	misc_msr_rqmid_38703_IA32_FEATURE_CONTROL_following_start_up_001();
	misc_msr_rqmid_38712_IA32_PLATFORM_ID_following_start_up_001();
	misc_msr_rqmid_38717_IA32_BIOS_SIGN_ID_following_start_up_001();
	misc_msr_rqmid_38736_change_IA32_MISC_ENABLE_bit34_invalidate_tlb();
	misc_msr_rqmid_38742_change_IA32_MISC_ENABLE_bit34_invalidate_paging_struct();
	misc_msr_rqmid_40268_read_msr_reserve_bits();
	misc_msr_rqmid_46545_wt_IA32_BIOS_SIGN_ID_001();
	misc_msr_rqmid_46542_rd_MSR_MISC_FEATURE_CONTROL_001();
	misc_msr_rqmid_46543_rd_IA32_CPU_DCA_CAP_001();
	misc_msr_rqmid_46544_rd_IA32_DCA_0_CAP_001();

	return report_summary();
}
