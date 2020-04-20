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

#define CPUID_BASIC_INFORMATION_07	0x07
#define EXTENDED_STATE_SUBLEAF_0	0x00

#define FEATURE_INFORMATION_BIT(SAL)	({ (0x1ul << SAL); })

#ifdef	USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

#define IA32_FLUSH_CMD_MSR 0x0000010B

u64 *cache_test_array = NULL;
u64 cache_l1_size = 0x800;	/* 16K/8 */
u64 cache_malloc_size = 0x1000;	/* 32K/8 */

unsigned long long asm_read_tsc(void)
{
	long long r;
	unsigned a, d;

	asm volatile("mfence" ::: "memory");
	asm volatile ("rdtsc" : "=a"(a), "=d"(d));
	r = a | ((long long)d << 32);
	asm volatile("mfence" ::: "memory");
	return r;
}

static inline void asm_read_access_memory(u64 *p)
{
	asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

void asm_mfence()
{
	asm volatile("mfence" ::: "memory");
}

u64 read_mem_cache_test(u64 size)
{
	u64 index;
	u64 t[2] = {0};

	t[0] = asm_read_tsc();
	for (index = 0; index < size; index++)
	{
		asm_read_access_memory(&cache_test_array[index]);
	}

	t[1] = asm_read_tsc();
	asm_mfence();

	return t[1] - t[0];
}

void asm_wbinvd()
{
	asm volatile ("wbinvd\n" : : : "memory");
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
	u64 t[2];
	struct cpuid cpuid7;
	cpuid7 = cpuid_indexed(7, 0);

	/* IA32_FLUSH_CMD */
<<<<<<< HEAD
	if (!(cpuid7.d & (1U << 28)))
=======
	if(!(cpuid7.d & (1U << 28)))
>>>>>>> add infoleak_sample_case_code,9 case fix
	{
		printf("-------------------not support L1D FLUSH\n");
		return false;
	}

	t[0] = asm_read_tsc();
	while (1)
	{
		t[1] = asm_read_tsc();
		if ((t[1] - t[0]) > 5000000000)
		{
			break;
		}
	}

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

	u64 tsc_stdevt = 0;
	u64 tsc_stdev[test_time];
	for (j = 0; j < (test_time - 1); j++)
	{
		tsc_stdev[j] = (tsc_delta[j] - tsc_average) * (tsc_delta[j] - tsc_average);
		tsc_stdevt += tsc_stdev[j];
	}
	tsc_stdevv = tsc_stdevt / (test_time - 1);

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
 * Summary: This part verifies L1D FLUSH cache on hypervisor, target is to get benchmark data
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
 * Summary: ACRN hypervisor shall expose IA32_FLUSH_CMD general support from any VM
 *
 */
void infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_002(void)
{
	int64_t tsc_average1 = 0;
	int64_t tsc_average2 = 0;
	int64_t tsc_stdevv1 = 0;

<<<<<<< HEAD
=======
	cache_test_array = (u64 *)malloc(cache_malloc_size*8);
>>>>>>> add infoleak_sample_case_code,9 case fix
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

/*check CPUID.(EAX=7H,ECX=0):EDX EDX[28] got enumerated*/
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
 * Summary:  ACRN hypervisor shall expose writeback and invalidate the L1 data cache and flush
 * microarchitectural structures to any VM in compliance with Chapter 5.1 and 5.5, SESCM.
 *
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
	asm ("mov (0x8100) ,%%eax\n\t"
		"mov %%eax,%0\n\t"
		: "=q"(bp_eax_ia32_spec_ctrl));
}


/**
 * @brief Case name: information leakage of IA32_SPEC_CTRL start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_SPEC_CTRL to 0 following start-up. 
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


#define IA32_SPEC_CTRL_MSR	0x48
/**
 * @brief Case name: information leakage of IBRS expose_001
 *
 * Summary: ACRN hypervisor shall expose indirect branch restricted speculation(IBRS) to
 * any VM in compliance with Chapter 2.4.1, SESCM.
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
 * Summary: ACRN hypervisor shall expose speculative store bypass disable (SSBD) to
 * any VM in compliance with Chapter 4.2.2 and 5.1, SESCM.
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
 * Summary: ACRN hypervisor shall expose additional functionality that will flush
 * microarchitectural structures to any VM in compliance with MDS mitigation guide from Intel.
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


#define IA32_PRED_CMD_MSR	0x49
/**
 * @brief Case name: information leakage of IBPB expose_001
 *
 * Summary: ACRN hypervisor shall expose indirect branch predictor barrier(IBPB) to
 * any VM in compliance with Chapter 2.4.3 and 5.4, SESCM.
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
	asm ("mov (0x8200),%%eax\n\t"
		"mov %%eax, %0\n\t"
		"mov $0xff, %%eax\n\t"
		"mov %%eax, (0x8200)\n\t"
		: "=q"(ap_eax_ia32_spec_ctrl));
}


/**
 * @brief Case name: IA32_SPEC_CTRL INIT_001
 *
 * Summary: ACRN hypervisor shall keep guest IA32_SPEC_CTRL unchanged following INIT.
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

static void print_case_list(void)
{
	printf("info_leak feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 33869u, "IA32_SPEC_CTRL start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33873u, "L1D_Flush expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33872u, "IBRS expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33875u, "SSBD expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33871u, "MDS mitigation mechnism expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33874u, "IBPB expose_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33870u, "IA32_SPEC_CTRL INIT_001");
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
	infoleak_rqmid_33870_IA32_SPEC_CTRL_INIT_001();
	infoleak_rqmid_33619_mitigate_L1TF_variant_affecting_VMM_benchmark_001();
	infoleak_rqmid_33617_mitigate_L1TF_variant_affecting_VMM_002();

	return report_summary();
}
