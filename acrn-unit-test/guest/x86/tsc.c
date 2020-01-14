/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * Test mode: Time Stamp Count, according to the SDM chaper 17
 * vol 3
 * Written by Zhu.Jie, 2019-06-13
 */
#include "types.h"
#include "libcflat.h"
#include "processor.h"
#include "apic.h"

#include "desc.h"
#include "isr.h"
#include "msr.h"
#include "smp.h"
#include "vm.h"

#define CR4_TSD (1 << 2)

#define TSC_10MS 0x1406F3F
#define TSC_1000MS 0x7D2B74E0
#define TSC_100MS 0xC84587D
#define TSC_10US 0x5208

#define TSC_INIT_CHECK_TIMES 10
#define ARRY_LEN TSC_INIT_CHECK_TIMES+1

static u32 bp_eax_tscadj_greg_64;
static u32 bp_edx_tscadj_greg_64;
static u32 bp_eax_tscaux_greg_64;
static u32 bp_edx_tscaux_greg_64;
static u32 bp_eax_tsc_greg_64;
static u32 bp_edx_tsc_greg_64;
static u32 bp_cr4_greg_long;
extern char sipi_cnt; /*count sipi */
static u32 ap_eax_tscadj_greg_64;
static u32 ap_edx_tscadj_greg_64;
static u32 ap_eax_tscaux_greg_64;
static u32 ap_edx_tscaux_greg_64;
static u32 ap_eax_tsc_greg_64;
static u32 ap_edx_tsc_greg_64;
static u32 ap_cr4_greg_long;

u64 g_temp_bp_tsc[ARRY_LEN] = {0};
u32 g_times = 0;

#define TSC_DEADLINE_TIMER_VECTOR	0xef
static volatile int tdt_count;
u64 now_tsc = 0;

#ifdef __x86_64__
static void tsc_deadline_timer_isr(isr_regs_t *regs)
{
    now_tsc = rdtsc();
    ++tdt_count;

    apic_write(APIC_EOI, 0);
    return;
}

static void start_tsc_deadline_timer(u64 delta)
{
    handle_irq(TSC_DEADLINE_TIMER_VECTOR, tsc_deadline_timer_isr);
    irq_enable();

    wrmsr(MSR_IA32_TSCDEADLINE, rdtsc()+delta);
    asm volatile ("nop");
}

/* calculate msr value by register eax and edx */
u64 msr_cal(u64 eax, u64 edx)
{
	return (eax | (edx << 32));
}

static int enable_tsc_deadline_timer(void)
{
	uint32_t lvtt;

	if (cpuid(1).c & (1 << 24)) {
		lvtt = APIC_LVT_TIMER_TSCDEADLINE | TSC_DEADLINE_TIMER_VECTOR;
		apic_write(APIC_LVTT, lvtt);
		return 1;
	} else {
		return 0;
    }
}

void read_bp_startup(void)
{
	asm ("mov (0x8100) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(bp_eax_tscadj_greg_64));

	asm ("mov (0x8108) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(bp_edx_tscadj_greg_64));

	//tscaux
	asm ("mov (0x8110) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(bp_eax_tscaux_greg_64));

	asm ("mov (0x8118) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(bp_edx_tscaux_greg_64));

	//tsc
	asm ("mov (0x8120) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(bp_eax_tsc_greg_64));

	asm ("mov (0x8128) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(bp_edx_tsc_greg_64));

	//cr4
	asm ("mov (0x8130) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(bp_cr4_greg_long));
}

extern void send_sipi();
void save_unchanged_reg(void)
{
	asm ("mov (0x8008) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(ap_cr4_greg_long));

	asm ("mov (0x8010) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(ap_eax_tscadj_greg_64));

	asm ("mov (0x8018) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(ap_edx_tscadj_greg_64));

	asm ("mov (0x8020) ,%%eax\n\t"
		 "mov %%eax,%0\n\t"
		 : "=q"(ap_eax_tscaux_greg_64));

	asm ("mov (0x8028) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(ap_edx_tscaux_greg_64));

	asm ("mov (0x8030) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(ap_eax_tsc_greg_64));

	asm ("mov (0x8038) ,%%eax\n\t"
			 "mov %%eax,%0\n\t"
			 : "=q"(ap_edx_tsc_greg_64));
}

/**
 * @brief Case name:Invariant TSC support_001
 *
 * Summary: To check if frequency of TSC keep unchanged during the test.
 *
 */
static int tsc_rqmid_26015_invariant_tsc_suppor(void)
{
	u64 temp_tsc[10] = {0};
	u64 differ[10] = {0};
	u32 result = 0;
	u32 i = 0;

	if (enable_tsc_deadline_timer()) {
		//printf("TSC deadline timer enabled\n");
	} else {
		//printf("TSC deadline timer not detected, aborting\n");
		abort();
	}

	for (i = 0; i < 10; i++) {
		temp_tsc[i] = rdtsc();
		start_tsc_deadline_timer(TSC_1000MS);

		while ((i+1) > tdt_count) {
			asm volatile("nop");
		};
		differ[i] = now_tsc - temp_tsc[i] - TSC_1000MS;
		/*
		 * printf("now_tsc:%ld - temp_tsc[%d] = %ld   diffe = %ld tdt_count:%d \n",
		 *	now_tsc, i, temp_tsc[i], differ[i], tdt_count);
		 */
		if ((0 < differ[i]) && (differ[i] < TSC_10US)) {
			result++;
		}
	}
	//printf("i:%d ,tdt_count:%d result:%d\n",i,tdt_count,result);
	report("\t\t tsc_rqmid_26015_invariant_tsc_suppor,the TSC frequency always keep same based on the formulation",
		result == 10u);

	return result;
}

/**
 * @brief Case name: CR4.TSD init value_001
 *
 * Summary: To check whether valule of registers changed after INIT or not.
 *
 */
static void  tsc_rqmid_25226_init_unchanged_check(void)
{
	u64 temp_ap_eax_tscadj_greg_64[2] = {0};
	u64 temp_ap_edx_tscadj_greg_64[2] = {0};
	u64 temp_ap_eax_tscaux_greg_64[2] = {0};
	u64 temp_ap_edx_tscaux_greg_64[2] = {0};
	u64 temp_ap_eax_tsc_greg_64[2] = {0};
	u64 temp_ap_edx_tsc_greg_64[2] = {0};

	u64 temp_ap_tscadj[2] = {0};
	u64 temp_ap_tscaux[2] = {0};
	u64 temp_ap_tsc[2] = {0};

	//u64 ap_tsc = 0;
	//u64 result_64 = 0;

	report("\t\t tsc_rqmid_25226_init_unchanged_check,CR4_TSD is initialized to 0x0 following INIT",
		(ap_cr4_greg_long&CR4_TSD) == 0x0);

	/* save TSC value to buffer */
	temp_ap_eax_tscadj_greg_64[0] = ap_eax_tscadj_greg_64;
	temp_ap_edx_tscadj_greg_64[0] = ap_edx_tscadj_greg_64;

	temp_ap_eax_tscaux_greg_64[0] = ap_eax_tscaux_greg_64;
	temp_ap_edx_tscaux_greg_64[0] = ap_edx_tscaux_greg_64;

	temp_ap_eax_tsc_greg_64[0] = ap_eax_tsc_greg_64;
	temp_ap_edx_tsc_greg_64[0] = ap_edx_tsc_greg_64;

	temp_ap_tscadj[0] =	msr_cal(temp_ap_eax_tscadj_greg_64[0], temp_ap_edx_tscadj_greg_64[0]);
	temp_ap_tscaux[0] =	msr_cal(temp_ap_eax_tscaux_greg_64[0], temp_ap_edx_tscaux_greg_64[0]);
	temp_ap_tsc[0] = msr_cal(temp_ap_eax_tsc_greg_64[0], temp_ap_edx_tsc_greg_64[0]);

	/* issue sipi to awake AP */
	//printf("--------------------2nd INIT--------------------------\n");
	send_sipi();

	/* save value to buffer again */
	temp_ap_eax_tscadj_greg_64[1] = ap_eax_tscadj_greg_64;
	temp_ap_edx_tscadj_greg_64[1] = ap_edx_tscadj_greg_64;

	temp_ap_eax_tscaux_greg_64[1] = ap_eax_tscaux_greg_64;
	temp_ap_edx_tscaux_greg_64[1] = ap_edx_tscaux_greg_64;

	temp_ap_eax_tsc_greg_64[1] = ap_eax_tsc_greg_64;
	temp_ap_edx_tsc_greg_64[1] = ap_edx_tsc_greg_64;

	temp_ap_tscadj[1] = msr_cal(temp_ap_eax_tscadj_greg_64[1], temp_ap_edx_tscadj_greg_64[1]);
	temp_ap_tscaux[1] =	msr_cal(temp_ap_eax_tscaux_greg_64[1], temp_ap_edx_tscaux_greg_64[1]);
	temp_ap_tsc[1] = msr_cal(temp_ap_eax_tsc_greg_64[1], temp_ap_edx_tsc_greg_64[1]);

	/* compare with first start up */
	report("\t\t tsc_rqmid_25226_init_unchanged_check,IA32_TSC_ADJUST unchanged following INIT",
		temp_ap_tscadj[0] == temp_ap_tscadj[1]);

	report("\t\t tsc_rqmid_25226_init_unchanged_check,IA32_TSC_AUX unchanged following INIT",
		temp_ap_tscaux[0] == temp_ap_tscaux[1]);

	report("\t\t tsc_rqmid_25226_init_unchanged_check,IA32_TIME_STAMP_COUNTER unchanged following INIT",
		(temp_ap_tsc[0] == temp_ap_tsc[1]) && (temp_ap_tsc[0] == 0));

	return;
}

static void print_case_list(void)
{
	printf("\t\t TSC feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 25226u, "CR4.TSD init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26015u, "Invariant TSC support_001");
}

static void test_tsc(void)
{
	tsc_rqmid_25226_init_unchanged_check();
	tsc_rqmid_26015_invariant_tsc_suppor();
}

int main(void)
{
	print_case_list();
	test_tsc();
	return report_summary();
}

#endif
