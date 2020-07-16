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
#include "instruction_common.h"
#include "memory_type.h"
#include "debug_print.h"

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
 * @brief Case name: SFR-08 ACRN hypervisor shall mitigate the L1TF variant affecting VMM_002
 *
 * Summary: In 64bit mode, L1D FLUSH should support; Enable cache and paging, configure
 * the memory type to WB, the read with L1D FLUSH should slower than without L1D FLUSH read.
 * Compare delta average with benchmark data, should in (average-3 *stdev, average+3*stdev).
 *
 */
void infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_002(void)
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

bool infoleak_L1D_FLUSH_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(28);

	if (check_bit == 0) {
		result = false;
	} else {
		result = true;
	}

	return result;
}

/**
 * @brief Case name: information leakage of L1D_Flush expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[28] is set to 1,Execute
 * WRMSR instruction to write 1H to IA32_FLUSH_CMD[0] no exception
 */
void infoleak_rqmid_33873_L1D_FLUSH_expose_001(void)
{
	if (infoleak_L1D_FLUSH_to_1() == true)
	{
		wrmsr(IA32_FLUSH_CMD_MSR, 0x01);
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
 * @brief Case name: information leakage of IBRS expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,IA32_SPEC_CTRL.bit[0]  can be set and clear under VM.
 *
 */
void infoleak_rqmid_33872_IBRS_expose_001(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(26);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);
	wrmsr(IA32_SPEC_CTRL_MSR, val & ~0x1);

	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x1) != 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	wrmsr(IA32_SPEC_CTRL_MSR, val | 0x1);
	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x1) == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
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
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(31);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);
	wrmsr(IA32_SPEC_CTRL_MSR, val | 0x4);

	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x4) == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	wrmsr(IA32_SPEC_CTRL_MSR, val & ~0x4);
	val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((val & 0x4) == 0)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}

/**
 * @brief Case name: information leakage of MDS mitigation mechnism expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[10] is set to 1,call VERW instruction has no exception.
 *
 */
void infoleak_rqmid_33871_MDS_mitigation_mechnism_expose_001(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(10);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	asm("mov $8, %ax\n\t"
		"verw %ax\n\t");

	report("\t\t %s", 1, __FUNCTION__);
}

/**
 * @brief Case name: information leakage of IBPB expose_001
 *
 * Summary: CPUID.(EAX=7H,ECX=0):EDX[26] is set to 1,Execute WRMSR instruction and no exception.
 *
 */
void infoleak_rqmid_33874_IBPB_expose_001(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(31);

	if (check_bit == 0)
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	wrmsr(IA32_PRED_CMD_MSR, 0x1);
	wrmsr(IA32_PRED_CMD_MSR, 0x0);

	report("\t\t %s", 1, __FUNCTION__);
}

static u32 ap_eax_ia32_spec_ctrl = 0xff;
void read_ap_init(void)
{
	//ia32_spec_ctrl

	uint32_t *ap_spec_ctrl_addr = (uint32_t *)AP_IA32_SPEC_CTRL_ADDR;
	asm ("mov %1, %%eax\n\t"
		"mov %%eax, %0\n\t"
		"mov $0xff, %%eax\n\t"
		"mov %%eax, %1\n\t"
		: "=m"(ap_eax_ia32_spec_ctrl), "=m"(*ap_spec_ctrl_addr));
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
	uint32_t spec_ctrl_1 = 0;
	uint32_t spec_ctrl_2 = 0;

	read_ap_init();
	spec_ctrl_1 = ap_eax_ia32_spec_ctrl;

	u64 val = rdmsr(IA32_SPEC_CTRL_MSR);
	if ((spec_ctrl_1 & 0x1) == 0)
	{
		val |= 0x1;
	}
	else
	{
		val &= ~0x1;
	}

	wrmsr(IA32_SPEC_CTRL_MSR, val);
	send_sipi();

	read_ap_init();
	spec_ctrl_2 = ap_eax_ia32_spec_ctrl;

	if (spec_ctrl_1 == spec_ctrl_2)
	{
		report("\t\t %s", 1, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
}
#endif

static void print_case_list(void)
{
	printf("info_leak feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 33869u, "IA32_SPEC_CTRL start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33873u, "L1D_Flush expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33872u, "IBRS expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33875u, "SSBD expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33871u, "MDS mitigation mechnism expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33874u, "IBPB expose_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 33870u, "IA32_SPEC_CTRL INIT_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 33619u, "mitigate the L1TF variant affecting VMM_benchmark_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33617u, "mitigate the L1TF variant affecting VMM_002");
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

	infoleak_rqmid_33869_IA32_SPEC_CTRL_startup_001();
	infoleak_rqmid_33873_L1D_FLUSH_expose_001();
	infoleak_rqmid_33872_IBRS_expose_001();
	infoleak_rqmid_33875_SSBD_expose_001();
	infoleak_rqmid_33871_MDS_mitigation_mechnism_expose_001();
	infoleak_rqmid_33874_IBPB_expose_001();
#ifdef IN_NON_SAFETY_VM
	infoleak_rqmid_33870_IA32_SPEC_CTRL_INIT_001();
#endif
	infoleak_rqmid_33619_mitigate_L1TF_variant_affecting_VMM_benchmark_001();
	infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_002();

	return report_summary();
}
