/*
 * Test for x86 information leakage
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
#include "info_leakage.h"
#include "host_register.h"
#include "instruction_common.h"
#include "memory_type.h"
#include "debug_print.h"
#include "fwcfg.h"
#include "delay.h"

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int ap_start_count = 0;

volatile uint64_t spec_ctrl_0 = 0;
volatile uint64_t spec_ctrl_1 = 0;
volatile uint64_t spec_ctrl_2 = 0;

#ifdef IN_NON_SAFETY_VM
static void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}
#endif

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

static void infoleak_ap_unchanged_case_33870()
{
	if (ap_start_count == 0) {
		spec_ctrl_0 = rdmsr(IA32_SPEC_CTRL_MSR);
		wrmsr(IA32_SPEC_CTRL_MSR, spec_ctrl_0 ^ 1u);
		spec_ctrl_1 = rdmsr(IA32_SPEC_CTRL_MSR);
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 1) {
		spec_ctrl_2 = rdmsr(IA32_SPEC_CTRL_MSR);
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 2) {
		/*restore IA32_SPEC_CTRL_MSR register to init value*/
		wrmsr(IA32_SPEC_CTRL_MSR, spec_ctrl_0);
		wait_ap = 1;
	}
}


void ap_main(void)
{
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	while (1) {
		switch (cur_case_id) {
		case 33870:
			infoleak_ap_unchanged_case_33870();
			cur_case_id = 0;
			break;
		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}
#endif

bool check_cpuid(u32 basic, u32 leaf, u32 bit)
{
	unsigned long check_bit = 0;
	bool result = false;

	check_bit = cpuid_indexed(basic, leaf).d;
	check_bit &= FEATURE_INFORMATION_BIT(bit);

	if (check_bit == 0)
	{
		result = false;
	}
	else
	{
		result = true;
	}

	return result;
}

void L1D_FLUSH(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(28);

	if (check_bit != 0)
	{
		wrmsr(IA32_FLUSH_CMD_MSR, 0x01);
		report("\t\t %s", 1, func);
	}
	else
	{
		report("\t\t %s", 0, func);
	}
}


void IBRS(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(26);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);
	wrmsr(IA32_SPEC_CTRL_MSR, val & ~0x1);

	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x1) != 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	wrmsr(IA32_SPEC_CTRL_MSR, val | 0x1);
	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x1) == 0)
	{
		report("\t\t %s", 0, func);
	}
	else
	{
		report("\t\t %s", 1, func);
	}
}


void SSBD(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(31);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);
	wrmsr(IA32_SPEC_CTRL_MSR, val | 0x4);

	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x4) == 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	wrmsr(IA32_SPEC_CTRL_MSR, val & ~0x4);
	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x4) == 0)
	{
		report("\t\t %s", 1, func);
	}
	else
	{
		report("\t\t %s", 0, func);
	}
}

void IBPB(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(26);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	wrmsr(IA32_PRED_CMD_MSR, 0x1);
	wrmsr(IA32_PRED_CMD_MSR, 0x0);

	report("\t\t %s", 1, func);
}

void MDS_mitigation_mechnism(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(10);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, func);
		return;
	}

	/*0x8 is code segment selector*/
	asm("mov $0x8, %ax\n\t"
		"verw %ax\n\t");

	report("\t\t %s", 1, func);
}

#ifdef IN_NATIVE

/**
 * @brief Case name: information leakage of L1D_Flush support_AC_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[28] is set to 1,Execute
 * WRMSR instruction to write 1H to IA32_FLUSH_CMD[0] no exception
 */
void infoleak_rqmid_36497_L1D_FLUSH_support_AC_001(void)
{
	L1D_FLUSH(__FUNCTION__);
}


/**
 * @brief Case name: Information_leakage_IBRS_support_AC_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,IA32_SPEC_CTRL.bit[0]  can be set and clear under VM.
 *
 */
void infoleak_rqmid_36498_IBRS_support_AC_001(void)
{
	IBRS(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of SSBD support_AC_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[31] is set to 1,Execute
 * WRMSR and RDMSR instruction to check IA32_SPEC_CTRL.bit[2].
 *
 */
void infoleak_rqmid_36499_SSBD_support_AC_001(void)
{
	SSBD(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of IBPB support_AC_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,Execute WRMSR instruction and no exception.
 *
 */
void infoleak_rqmid_36503_IBPB_support_AC_001(void)
{
	IBPB(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of MDS mitigation mechnism support_AC_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[10] is set to 1,call VERW instruction has no exception.
 *
 */
void infoleak_rqmid_37015_MDS_mitigation_mechnism_support_AC_001(void)
{
	MDS_mitigation_mechnism(__FUNCTION__);
}

#else /*end native vm*/

#ifdef	USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

static u64 *cache_test_array = NULL;
static u64 cache_l1_size = 0x800;	/* 16K/8 */
static u64 cache_malloc_size = 0x1000;	/* 32K/8 */

u64 read_mem_cache_test(u64 size)
{
	return read_mem_cache_test_addr(cache_test_array, size);
}

struct case_fun_index
{
	int rqmid;
	void (*func)(void);
};

#define test_time 41

u64 tsc_average = 0;
u64 tsc_stdevv = 0;
bool calc_flush_cmd(void)
{
	int i;
	int j;
	u64 tsc_delay1[test_time];
	u64 tsc_delay_total1;
	u64 tsc_delay2[test_time];
	u64 tsc_delay_total2;
	struct cpuid cpuid7;
	cpuid7 = cpuid_indexed(7, 0);

	/* IA32_FLUSH_CMD */
	if (!(cpuid7.d & (1U << 28)))
	{
		printf("-------------------not support L1D FLUSH\n");
		return false;
	}

	/* clear cr0.cd[bit30] and cr0.nw[bit29], enable cache and write-back or write-through */
	uint32_t cr0 = read_cr0();
	cr0 = cr0 & 0x9fffffff;
	write_cr0(cr0);

	tsc_delay_total1 = 0;
	tsc_delay_total2 = 0;
	for (j = 0; j < test_time; j++)
	{
		asm_wbinvd();
		tsc_delay1[j] = 0;
		read_mem_cache_test(cache_l1_size);
		for (i = 0; i < 500; i++)
		{
			/*no cache flush*/
			tsc_delay1[j] += read_mem_cache_test(cache_l1_size);
		}
		tsc_delay_total1 += tsc_delay1[j];
		asm_wbinvd();

		tsc_delay2[j] = 0;
		read_mem_cache_test(cache_l1_size);
		for (i = 0; i < 500; i++)
		{
			wrmsr(IA32_FLUSH_CMD_MSR, 0x1);/*bit 0 set to 1,  l1 cache flush*/
			tsc_delay2[j] += read_mem_cache_test(cache_l1_size);
		}
		tsc_delay_total2 += tsc_delay2[j];
	}

	u64 tsc_total = 0;
	u64 tsc_delta[test_time];
	for (j = 1; j < test_time; j++)
	{
		if (tsc_delay2 [j] >= tsc_delay1[j])
		{
			tsc_delta[j - 1] = tsc_delay2 [j] - tsc_delay1[j];
			tsc_total += tsc_delta[j - 1];

			continue;
		}

		return false;
	}
	tsc_average = tsc_total / (test_time - 1);

	/* calculate variance of tsc_average */
	u64 tsc_stdevt = 0;
	u64 tsc_stdev[test_time];
	for (j = 0; j < (test_time - 1); j++)
	{
		tsc_stdev[j] = (tsc_delta[j] - tsc_average) * (tsc_delta[j] - tsc_average);
		tsc_stdevt += tsc_stdev[j];
	}
	tsc_stdevv = tsc_stdevt / (test_time - 1);

	/* calculate the square root of tsc_stdevv,get standard variance */
	i = 1;
	while (1)
	{
		if ((i * i) >= tsc_stdevv)
		{
			tsc_stdevv = i;
			break;
		}
		i++;
	}

	return true;
}

/**
 * @brief Case name: information leakage of SFR-08 ACRN hypervisor shall mitigate the L1TF
 * variant affecting VMM_benchmark_001
 *
 * Summary: In 64bit mode, L1D FLUSH should support; Enable cache and paging, configure
 * the memory type to WB, the read with L1D FLUSH should slower than without L1D FLUSH read.
 *
 */
void infoleak_rqmid_33619_mitigate_L1TF_variant_affecting_VMM_benchmark_001(void)
{
	cache_test_array = (u64 *)malloc(cache_malloc_size*8);
	if (cache_test_array == NULL)
	{
		debug_error("malloc error\n");
		return;
	}

	bool ret = calc_flush_cmd();

	if (ret == true)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
	return;
}

/**
 * @brief Case name: SFR-08 ACRN hypervisor shall mitigate the L1TF variant affecting VMM_benchmark_002
 *
 * Summary: In 64bit mode, L1D FLUSH should support; Enable cache and paging, configure
 * the memory type to WB, the read with L1D FLUSH should slower than without L1D FLUSH read.
 * Compare delta average with benchmark data, should in (average-3 *stdev, average+3*stdev).
 *
 */
void infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_benchmark_002(void)
{
	int64_t tsc_average1 = 0;
	int64_t tsc_average2 = 0;
	int64_t tsc_stdevv1 = 0;

	if (cache_test_array == NULL)
	{
		debug_error("malloc error\n");
		return;
	}

	tsc_average1 = tsc_average;
	tsc_stdevv1 = tsc_stdevv;
	if ((tsc_average1 == 0x0) || (tsc_stdevv1 == 0x0))
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	if (calc_flush_cmd() == false)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}
	tsc_average2 = tsc_average;

	if ((tsc_average2 > (tsc_average1 - 3*tsc_stdevv1)) && (tsc_average2 < (tsc_average1 + 3*tsc_stdevv1)))
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
	return;
}

static u32 bp_eax_ia32_spec_ctrl = 0xff;
void read_bp_startup(void)
{
	//ia32_spec_ctrl
	uint32_t *bp_spec_ctrl_addr = (uint32_t *)BP_IA32_SPEC_CTRL_ADDR;
	asm ("mov %1, %%eax\n\t"
		"mov %%eax,%0\n\t"
		: "=m"(bp_eax_ia32_spec_ctrl), "=m"(*bp_spec_ctrl_addr));
}


/**
 * @brief Case name: information leakage of IA32_SPEC_CTRL start-up_001
 *
 * Summary: Get IA32_SPEC_CTRL at BP start-up, shall be 0 and same with SESCM definition.
 *
 */
void infoleak_rqmid_33869_IA32_SPEC_CTRL_startup_001(void)
{
	read_bp_startup();

	if (bp_eax_ia32_spec_ctrl == 0)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}

	return;
}

/**
 * @brief Case name: information leakage of L1D_Flush expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[28] is set to 1,Execute
 * WRMSR instruction to write 1H to IA32_FLUSH_CMD[0] no exception
 */
void infoleak_rqmid_33873_L1D_FLUSH_expose_001(void)
{
	L1D_FLUSH(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of L1D_Flush expose_002
 *
 * Summary: Dump register IA32_FLUSH_CMDshall generate #GP(0).
 */
void infoleak_rqmid_38262_L1D_FLUSH_expose_002(void)
{
	u32 index = IA32_FLUSH_CMD_MSR;
	u32 a = 0;
	u32 d = 0;

	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: information leakage of IBRS expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,IA32_SPEC_CTRL.bit[0]  can be set and clear under VM.
 *
 */
void infoleak_rqmid_33872_IBRS_expose_001(void)
{
	IBRS(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of SSBD expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[31] is set to 1,Execute
 * WRMSR and RDMSR instruction to check IA32_SPEC_CTRL.bit[2].
 *
 */
void infoleak_rqmid_33875_SSBD_expose_001(void)
{
	SSBD(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of IBPB expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,Execute WRMSR instruction and no exception.
 *
 */
void infoleak_rqmid_33874_IBPB_expose_001(void)
{
	IBPB(__FUNCTION__);
}

/**
 * @brief Case name: information leakage of IBPB expose_002
 *
 * Summary: Dump register IA32_PRED_CMD shall generate #GP(0).
 *
 */
void infoleak_rqmid_38261_IBPB_expose_002(void)
{
	u32 index = IA32_PRED_CMD_MSR;
	u32 a = 0;
	u32 d = 0;

	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: information leakage of MDS mitigation mechnism expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[10] is set to 1,call VERW instruction has no exception.
 *
 */
void infoleak_rqmid_33871_MDS_mitigation_mechnism_expose_001(void)
{
	MDS_mitigation_mechnism(__FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief Case name: IA32_SPEC_CTRL INIT_001
 *
 * Summary: After AP receives first INIT, set the value of IA32_SPEC_CTRL;
 * Dump IA32_SPEC_CTRL value shall get the same value after second INIT.
 *
 */
void infoleak_rqmid_33870_IA32_SPEC_CTRL_INIT_001(void)
{
	cur_case_id = 33870;/*trigger ap_main function entering switch  33870*/
	wait_ap_ready();
	/*send sipi to ap  trigger ap_main function was called to get spec_ctrl again.*/
	send_sipi();
	cur_case_id = 33870;
	wait_ap_ready();
	/*
	 *spec_ctrl_2 is the second init AP value,it should be equal with spec_ctrl_1.
	 */
	report("\t\t%s", spec_ctrl_1 == spec_ctrl_2, __FUNCTION__);
	/*send sipi to ap again for restoring ymm init value*/
	send_sipi();
	cur_case_id = 33870;
	wait_ap_ready();
}

/**
 * @brief Case name: IA32_SPEC_CTRL INIT_002
 *
 * Summary: After AP receives first INIT, Dump IA32_SPEC_CTRL value,
 * the value should be samed with the IA32_SPEC_CTRL value dumped from BP
 * when it start.
 *
 */
void infoleak_rqmid_37018_IA32_SPEC_CTRL_INIT_002(void)
{
	uint32_t *ap_spec_ctrl = (uint32_t *)AP_IA32_SPEC_CTRL_INIT_ADDR;

	if (*ap_spec_ctrl == HOST_IA32_SPEC_CTRL_VAL)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}
#endif

/**
 * @brief Case name: Information_Leakage_Check_additional_features_hidden_to_vm_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[29] is set to 0.
 *
 */
void infoleak_rqmid_27879_Check_additional_features_hidden_to_vm_001(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(29);

	if (check_bit != 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	report("\t\t %s", 1, __FUNCTION__);
}

void check_additional_features_hidden_to_vm(const char *func)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(29);

	if (check_bit != 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	u32 index = IA32_ARCH_CAPABILITIES_MSR;
	u32 a = 0;
	u32 d = 0;

	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), func);
}

/**
 * @brief Case name: Information_Leakage_Check_additional_features_hidden_to_vm_002
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[29] is set to 0,
 * try to rdmsr IA32_ARCH_CAPABILITIES_MSR[4], and GP exception expected.
 *
 */
void infoleak_rqmid_27883_Check_additional_features_hidden_to_vm_002(void)
{
	check_additional_features_hidden_to_vm(__FUNCTION__);
}

/**
 * @brief Case name: Information_Leakage_Check_STIBP_hided_to_Vm_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[27] is set to 0.
 *
 */
void infoleak_rqmid_27873_Information_Leakage_Check_STIBP_hided_to_Vm_001(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(27);

	if (check_bit != 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	report("\t\t %s", 1, __FUNCTION__);
}

/**
 * @brief Case name: Information_Leakage_Check_STIBP_hided_to_Vm_002
 *
 * Summary: Execute WRMSR instruction to write 1H to IA32_SPEC_CTRL_MSR[1] no exception
 *
 */
void infoleak_rqmid_27874_Information_Leakage_Check_STIBP_hided_to_Vm_002(void)
{
	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);

	wrmsr(IA32_SPEC_CTRL_MSR, val | 0x2);
	wrmsr(IA32_SPEC_CTRL_MSR, val & ~0x2);

	report("\t\t %s", 1, __FUNCTION__);
}
#endif /*end no-saftey and safety vm*/

static void print_case_list(void)
{
	printf("info_leak feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 36497u, "L1D_Flush support_AC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36498u, "IBRS support_AC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36499u, "SSBD support_AC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36503u, "IBPB support_AC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37015u, "MDS mitigation mechnism support_AC_001");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 33869u, "IA32_SPEC_CTRL start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33873u, "L1D_Flush expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38262u, "L1D_Flush expose_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 33872u, "IBRS expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33875u, "SSBD expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33871u, "MDS mitigation mechnism expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33874u, "IBPB expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38261u, "IBPB expose_002");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 37018u, "IA32_SPEC_CTRL INIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 33870u, "IA32_SPEC_CTRL INIT_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 33619u, "mitigate the L1TF variant affecting VMM_benchmark_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33617u, "mitigate the L1TF variant affecting VMM_benchmark_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27879u, "Check_additional_features_hidden_to_vm_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27883u, "Check_additional_features_hidden_to_vm_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27873u, "Information_Leakage_Check_STIBP_hided_to_Vm_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27874u, "Information_Leakage_Check_STIBP_hided_to_Vm_002");
#endif
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

#ifdef IN_NATIVE
	infoleak_rqmid_36497_L1D_FLUSH_support_AC_001();
	infoleak_rqmid_36498_IBRS_support_AC_001();
	infoleak_rqmid_36499_SSBD_support_AC_001();
	infoleak_rqmid_36503_IBPB_support_AC_001();
	infoleak_rqmid_37015_MDS_mitigation_mechnism_support_AC_001();
#else
	infoleak_rqmid_33869_IA32_SPEC_CTRL_startup_001();
	infoleak_rqmid_33873_L1D_FLUSH_expose_001();
	infoleak_rqmid_38262_L1D_FLUSH_expose_002();
	infoleak_rqmid_33872_IBRS_expose_001();
	infoleak_rqmid_33875_SSBD_expose_001();
	infoleak_rqmid_33871_MDS_mitigation_mechnism_expose_001();
	infoleak_rqmid_33874_IBPB_expose_001();
	infoleak_rqmid_38261_IBPB_expose_002();
#ifdef IN_NON_SAFETY_VM
	infoleak_rqmid_37018_IA32_SPEC_CTRL_INIT_002();
	infoleak_rqmid_33870_IA32_SPEC_CTRL_INIT_001();
#endif
	infoleak_rqmid_33619_mitigate_L1TF_variant_affecting_VMM_benchmark_001();
	infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_benchmark_002();
	infoleak_rqmid_27879_Check_additional_features_hidden_to_vm_001();
	infoleak_rqmid_27883_Check_additional_features_hidden_to_vm_002();
	infoleak_rqmid_27873_Information_Leakage_Check_STIBP_hided_to_Vm_001();
	infoleak_rqmid_27874_Information_Leakage_Check_STIBP_hided_to_Vm_002();
#endif
	return report_summary();
}
