/*
 * Copyright (C) 2020 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2020/3/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 * Modify Author:              hexiangx.li@intel.com
 * Date :                       2020/11/03
 */
#define USE_DEBUG
#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include "xsave.h"
#include "debug_print.h"

#define INIT_TARGET_AP_ID                   1
#define LAPIC_CPUID_APIC_CAP_X2APCI             (0x1U << 21)
#define MSR_IA32_APIC_BASE                      0x0000001BU
#define BIT_EN_x2APIC                               (0x1U << 10)
#define BIT_EN_xAPIC                                (0x1U << 11)
#define BIT_XAPIC_X2APIC (BIT_EN_x2APIC | BIT_EN_xAPIC)
#define LAPIC_CPUID_CAPABILITY             1

static volatile u8 ap_nums;

/* start_run_id is used to identify which test case is running. */
static volatile int start_run_id = 0;

/* wait_bp/wait_ap are used to sync between bp and aps */
static atomic_t wait_bp;
static atomic_t wait_ap;

static volatile int target_ap_startup = 0;
static atomic_t ap_run_nums;
static atomic_t ap_chk;
typedef struct {
	int case_id;
	const char *case_name;
	void (*case_fun)(void);
} st_case_suit_lapic;

static __unused void ap_sync(void)
{
	while (atomic_read(&wait_bp) == 0) {
		debug_print("%d %d\n", atomic_read(&wait_ap), atomic_read(&wait_bp));
		test_delay(1);
	}
	atomic_dec(&wait_bp);
}

static __unused void bp_sync(void)
{
	while (atomic_read(&wait_ap) != ap_nums) {
		debug_print("%d %d\n", atomic_read(&wait_ap), atomic_read(&wait_bp));
		test_delay(1);
	}
	atomic_set(&wait_ap, 0);
}

/*
 * @brief case name: HSI_Local_APIC_Features_Inter_Processor_Interrupt_001
 *
 * Summary: Under 64 bit mode on native board, BP send INIT IPI
 * and startup IPI to target AP, target AP should restart successfully.
 */
static void hsi_rqmid_35951_local_apic_features_inter_processor_interrupt_001(void)
{
	int apic_id = get_lapicid_map(INIT_TARGET_AP_ID);
	start_run_id = 35951;

	bp_sync();
	test_delay(5);
	/*issue sipi to awake AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, apic_id);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, apic_id);

	/* wait for 1s fot ap to finish startup*/
	test_delay(5);
	start_run_id = 0;
	report("%s", (target_ap_startup == 2), __FUNCTION__);
}

static void hsi_rqmid_35951_ap(void)
{
	if (get_cpu_id() == INIT_TARGET_AP_ID) {
		target_ap_startup++;
		if (target_ap_startup == 1) {
			atomic_set(&wait_ap, ap_nums);
		}
	}
}

/*
 * @brief case name: HSI_Local_APIC_Features_x2APIC_mode_001
 *
 * Summary: Under 64 bit mode on native board, on each physical processor,
 * set bit[11:10] of IA32_APIC_BASE MSR to enable x2APIC mode should be successfully.
 */
static void hsi_rqmid_35950_local_apic_features_x2apic_mode_001(void)
{
	u32 bp_chk = 0;
	struct cpuid id;
	u64 base_org;
	u8 ap_nums = fwcfg_get_nb_cpus() - 1;
	atomic_set(&ap_chk, 0);

	start_run_id = 35950;

	id = cpuid(LAPIC_CPUID_CAPABILITY);

	if (id.c & LAPIC_CPUID_APIC_CAP_X2APCI) {
		base_org = rdmsr(MSR_IA32_APIC_BASE);
		/* Enable LAPIC in xAPIC mode and x2APIC mode */
		wrmsr(MSR_IA32_APIC_BASE, (base_org | BIT_XAPIC_X2APIC));

		if ((rdmsr(MSR_IA32_APIC_BASE) & BIT_XAPIC_X2APIC) == BIT_XAPIC_X2APIC) {
			bp_chk++;
		}
		wrmsr(MSR_IA32_APIC_BASE, base_org);
	}
	test_delay(10);

	start_run_id = 0;
	report("%s", ((bp_chk == 1) && (atomic_read(&ap_chk) == ap_nums)), __FUNCTION__);
}

static void hsi_rqmid_35950_ap(void)
{
	struct cpuid id;
	u64 base_org;

	id = cpuid(LAPIC_CPUID_CAPABILITY);

	if (id.c & LAPIC_CPUID_APIC_CAP_X2APCI) {
		base_org = rdmsr(MSR_IA32_APIC_BASE);
		/* Enable LAPIC in xAPIC mode and x2APIC mode */
		wrmsr(MSR_IA32_APIC_BASE, (base_org | BIT_EN_x2APIC | BIT_EN_xAPIC));

		if ((rdmsr(MSR_IA32_APIC_BASE) & BIT_XAPIC_X2APIC) == BIT_XAPIC_X2APIC) {
			atomic_inc(&ap_chk);
		}
		wrmsr(MSR_IA32_APIC_BASE, base_org);
	}
}

static st_case_suit_lapic case_suit_lapic[] = {
	{
		.case_fun = hsi_rqmid_35951_local_apic_features_inter_processor_interrupt_001,
		.case_id = 35951,
		.case_name = "HSI_Local_APIC_Features_Inter_Processor_Interrupt_001",
	},

	{
		.case_fun = hsi_rqmid_35950_local_apic_features_x2apic_mode_001,
		.case_id = 35950,
		.case_name = "HSI_Local_APIC_Features_x2APIC_mode_001",
	},
};

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI Local APIC features case list: \n\r");
#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_lapic); i++) {
		printf("\t Case ID: %d Case name: %s\n\r", case_suit_lapic[i].case_id, case_suit_lapic[i].case_name);
	}
#else
#endif
	printf("\t \n\r \n\r");
}

int ap_main(void)
{
#ifdef IN_NATIVE
	debug_print("ap %d starts running \n", get_cpu_id());
	while (start_run_id == 0) {
		test_delay(5);
	}

	while (start_run_id != 35951) {
		test_delay(5);
	}
	debug_print("ap start to run case %d\n", start_run_id);
	hsi_rqmid_35951_ap();

	while (start_run_id != 35950) {
		test_delay(5);
	}
	debug_print("ap start to run case %d\n", start_run_id);
	hsi_rqmid_35950_ap();

#endif
	return 0;
}

int main(void)
{
	setup_idt();
	asm volatile("fninit");

	print_case_list();
	atomic_set(&ap_run_nums, 0);
	ap_nums = fwcfg_get_nb_cpus();

#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_lapic); i++) {
		case_suit_lapic[i].case_fun();
	}
#else
#endif
	start_run_id = 0;
	return report_summary();
}


