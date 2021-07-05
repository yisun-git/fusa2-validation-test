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
#include "misc.h"
#include "desc.h"
#include "isr.h"
#include "msr.h"
#include "smp.h"
#include "vm.h"
#include "tsc.h"
#include "fwcfg.h"
#include "delay.h"

#define CR4_TSD (1 << 2)

#define TSC_10MS 0x1406F3F
#define TSC_1000MS 0x7D2B74E0
#define TSC_100MS 0xC84587D
#define TSC_10US 0x5208

#define TSC_INIT_CHECK_TIMES 10
#define ARRY_LEN TSC_INIT_CHECK_TIMES+1

#define CPUID_80000001_EDX_RDTSCP	(1 << 27)
#define CPUID_7_0_ECX_RDPID			(1 << 22)
static u32 aux[3] = {0u, 10u, 100u};


static u32 bp_eax_tscadj_greg_64;
static u32 bp_edx_tscadj_greg_64;
static u32 bp_eax_tscaux_greg_64;
static u32 bp_edx_tscaux_greg_64;
static u32 bp_eax_tsc_greg_64;
static u32 bp_edx_tsc_greg_64;
static u32 bp_cr4_greg_long;
static u64 ap_tsc_adjust_val;
extern char sipi_cnt; /*count sipi */

u64 g_temp_bp_tsc[ARRY_LEN] = {0};
u32 g_times = 0;

#define TSC_DEADLINE_TIMER_VECTOR	0xef
static volatile int tdt_count;
u64 now_tsc = 0;

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		asm ("pause");
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
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

__unused static void modify_cr4_tsd_init_value()
{
	write_cr4(read_cr4() | CR4_TSD);
}

__unused static void modify_tsc_adj_init_value()
{
	wrmsr(MSR_IA32_TSC_ADJUST, TSCADJ_UNCHANGE_NEW_VALUE);
}

__unused static void modify_tsc_aux_init_value()
{
	wrmsr(MSR_TSC_AUX, TSCAUX_UNCHANGE_NEW_VALUE);
}

__unused static void modify_tsc_init_value()
{
	wrmsr(MSR_TSC, TSC_UNCHANGE_NEW_VALUE);
}


void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	write_cr4(read_cr4() | X86_CR4_TSD);

	switch (cur_case_id) {
	case 25226:
		fp = modify_cr4_tsd_init_value;
		ap_init_value_process(fp);
		break;
	case 45987:
		fp = modify_tsc_adj_init_value;
		ap_init_value_process(fp);
		break;
	case 45988:
		fp = modify_tsc_aux_init_value;
		ap_init_value_process(fp);
		break;
	case 45998:
		fp = modify_tsc_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif


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

void read_ap_init_val(void)
{
	ap_tsc_adjust_val =  *(volatile u64 *)INIT_TSCADJ_LOW_ADDR;
}


/**
 * @brief Case name:Nominal core crystal clock frequency_001
 *
 * Summary: ACRN hypervisor shall guarantee that the nominal core crystal clock frequency in H reported to
 * any vCPU is 16C 2154H.
 * Ch17.17 vol3 SDM
 *
 */
#ifdef IN_NATIVE
#define EXPnomTSCFreq 0x0
#else
#define EXPnomTSCFreq 0x16C2154
#endif
static void tsc_rqmid_24416_nom_crystal_clock_freq(void)
{
	u32 nomFreq = cpuid(0x15).c;
	if (nomFreq != EXPnomTSCFreq) {
		printf("The expected value: 0x%x, the actual value: 0x%x\n", EXPnomTSCFreq, nomFreq);
	}
	report("\t\t%s", nomFreq == EXPnomTSCFreq, __FUNCTION__);
}

void wrap_rdtsc(const char *msg)
{
	unsigned a, d;
	asm volatile (ASM_TRY("1f")
					"rdtsc\n\t"
					"1:"
					 : "=a"(a), "=d"(d));
	report("\t\t%s", (exception_vector() == GP_VECTOR), msg);
}

/*
 * @brief Case name:TSC general support_001
 *
 * Summary: ACRN hypervisor shall expose TSC general support to any VM. Chapter 17.17, Vol. 3,
 * SDM and Chapter 18.7.3, Vol. 3, SDM.
 *
 */
static void tsc_rqmid_24463_TSC_general_support_001(void)
{
	volatile u64 counter1;
	volatile u64 counter2;

	if (cpuid(0x1).d & (1 << 4)) {
	} else {
		report("%s", false, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_TSD);
	counter1 = rdtsc();
	asm volatile("nop");
	asm volatile("nop");
	counter2 = rdtsc();
	if (counter2 < counter1) {
		report("%s", false, __FUNCTION__);
		return;
	}
	do_at_ring3(wrap_rdtsc, __FUNCTION__);
}


/*
 * @brief Case name:TSC auxiliary Support_001
 *
 * Summary:ACRN hypervisor shall expose auxiliary TSC to any VM. Chapter 17.17.2, Vol. 3, SDM.
 *
 */

static void tsc_rqmid_24466_TSC_auxiliary_Support(void)
{
	u32 ecx;
	u32 eax;
	u32 i;

	//report("RDTSC non-ring 0 access is enable",(read_cr4() & CR4_TSD) == 0x0);
	/* check the value with instruction-RDTSCP */
	if ((cpuid(0x80000001).d & CPUID_80000001_EDX_RDTSCP) != 0) {
		for (i = 0; i < 3; i++) {
			wrmsr(MSR_TSC_AUX, aux[i]);
			rdtscp(&ecx);
			if (ecx != aux[i]) {
				report("%s", false, __FUNCTION__);
				return;
			}
		}
	} else {
		report("%s", false, __FUNCTION__);
		return;
	}

	for (i = 0; i < 3; i++) {
		wrmsr(MSR_TSC_AUX, aux[i]);
		eax = rdmsr(MSR_TSC_AUX);
		if (eax != aux[i]) {
			report("%s", false, __FUNCTION__);
			return;
		}
	}
	report("\t\t%s", true, __FUNCTION__);
}

 /*
  * @brief Case name:Tsc adjustment support_001
  *
  * Summary: ACRN hypervisor shall expose TSC adjustment to any VM, in compliance with Chapter 17.17.3, Vol. 3, SDM.
  *
  */

static void tsc_rqmid_24448_Tsc_adjustment_support(void)
{
	u64 t1, t2, t3;

	/* check if MSR_IA32_TSC_ADJUST Feature is enabled */
	if (cpuid(7).b & (1 << 1)) {
		if (rdmsr(MSR_IA32_TSC_ADJUST) != 0x0) {
			report("%s", false, __FUNCTION__);
			return;
		}

		t3 = 100000000000ull;
		t1 = rdtsc();
		wrmsr(MSR_IA32_TSC_ADJUST, t3);
		t2 = rdtsc();

		if (rdmsr(MSR_IA32_TSC_ADJUST) != t3) {
			report("%s", false, __FUNCTION__);
			return;
		}

		if (!((t2 - t1) >= t3)) {
			report("%s", false, __FUNCTION__);
			return;
		}

		t3 = 0x0;
		wrmsr(MSR_IA32_TSC_ADJUST, t3);
		if (rdmsr(MSR_IA32_TSC_ADJUST) == t3) {
			report("\t\t%s", true, __FUNCTION__);
		} else {
			report("%s", false, __FUNCTION__);
		}

	} else {
		report("%s", false, __FUNCTION__);
	}
	return;
}



/**
 * @brief Case name:Invariant TSC support_001
 *
 * Summary: To check if frequency of TSC keep unchanged during the test.
 *
 */
static int tsc_rqmid_26015_invariant_tsc_support(void)
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
	report("\t\ttsc_rqmid_26015_invariant_tsc_suppor,the TSC frequency always keep same based on the formulation",
		result == 10u);

	return result;
}


 /*
  * @brief Case name:The ACRN need wrap around TSC_001
  *
  * Summary: ACRN hypervisor shall guarantee that the guest time-stamp counter
  * wraps around after overflow. Ch17.17 vol3.
  *
  */

#define DELTA_MAX	(0xFFFFFFFFFFFFFFFFull - 0xFFFFFFC000000000ull)
#define MIN_INIT_VALUE	0x3782DACe9d90000ull
static void tsc_rqmid_25225_need_wrap_around_TSC(void)
{
	u64 starttime = 0xFFFFFFC000000000;
	u64 endtime = 0u;
	u64 delta = 0u;
	int count = 100000;
	u64 tmp1;
	u64 tmp2;
	int mean_value;

	if (rdtsc() > MIN_INIT_VALUE) {
		report("\t\t%s", false, __FUNCTION__);
		return;
	}

	/*Calculate the tsc counter spending by one time loop*/
	tmp1 = rdtsc();
	do {
		endtime = rdtsc();
		delta = endtime - starttime;
	} while (count--);
	tmp2 = rdtsc();

	mean_value = (tmp2 - tmp1) / 100000;
	wrtsc(starttime);
	printf("wait about two minutes untill tsc wrap_around\n");
	do {
		endtime = rdtsc();
		delta = endtime - starttime;
	} while ((delta < DELTA_MAX) || (endtime >= 2*mean_value));
 /*
  * if tsc overflow, when while loop exit,the final endtime(tsc's value) should be a small  enough,
  *it should be smaller than 2 * mean_value
  */
	report("\t\t%s", endtime < 2*mean_value, __FUNCTION__);
}


 /*
  * @brief Case name: IA32_TSC_AUX start-up value_001
  *
  * Summary: ACRN hypervisor shall set initial guest IA32_TSC_AUX to 0H following start-up.
  *
  */

static void tsc_rqmid_39299_IA32_TSC_AUX_startup_value_001(void)
{
	u64 bp_tscaux = bp_eax_tscaux_greg_64 | ((u64)bp_edx_tscaux_greg_64 << 32);

	report("\t\t%s", bp_tscaux == 0x0, __FUNCTION__);
}

 /*
  * @brief Case name: IA32_TIME_STAMP_COUNTER start-up value_001
  *
  * Summary:ACRN hypervisor shall set guest initial IA32_TIME_STAMP_COUNTER less than 378 2DAC E9D9 0000 H.
  *
  */
#define MAX_TSC 0x3782DACE9D90000
static void tsc_rqmid_39303_IA32_TIME_STAMP_COUNTER_startup_value_001(void)
{
	u64 bp_tsc = bp_eax_tsc_greg_64 | ((u64)bp_edx_tsc_greg_64 << 32);
	report("\t\t%s", bp_tsc < MAX_TSC, __FUNCTION__);
}

 /*
  * @brief Case name: IA32_TSC_ADJUST start-up value_001
  *
  * Summary: ACRN hypervisor shall set initial guest IA32_TSC_ADJUST to 0H following start-up.
  *
  */

static void tsc_rqmid_39305_IA32_TSC_ADJUST_startup_value_001(void)
{
	u64 bp_tscadj = bp_eax_tscadj_greg_64 | ((u64)bp_edx_tscadj_greg_64 << 32);
	report("\t\t%s", bp_tscadj == 0, __FUNCTION__);

}

 /*
  * @brief Case name: CR4.TSD start-up value_001
  *
  * Summary: ACRN hypervisor shall set initial guest CR4.TSD to 0H following start-up.
  *
  */

static void tsc_rqmid_39307_CR4_TSD_startup_value_001(void)
{
	report("\t\t%s", (bp_cr4_greg_long & CR4_TSD) == 0x0, __FUNCTION__);
}


/**
 * @brief Case name: CR4.TSD init value_001
 *
 * Summary: To check whether valule of registers changed after INIT or not.
 *
 */
static void __unused tsc_rqmid_25226_cr4_tsd_init_value()
{
	volatile u32 cr4_init = *((u32 *)INIT_CR4_ADDR);
	bool is_pass = true;

	if ((cr4_init & CR4_TSD) != 0x0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(25226);
	cr4_init = *((u32 *)INIT_CR4_ADDR);
	if ((cr4_init & CR4_TSD) != 0x0) {
		is_pass = false;
	}

	report("\t\t%s", is_pass, __FUNCTION__);
}

/**
 * @brief Case name: IA32_TSC_ADJUST init value_001
 *
 * Summary: ACRN hypervisor shall keep guest IA32_TSC_ADJUST unchanged following INIT.
 *
 */
static void __unused tsc_rqmid_45987_IA32_TSC_ADJUST_init_value()
{
	volatile u64 tscadj;
	bool is_pass = true;


	if (ap_tsc_adjust_val != 0x0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(45987);
	tscadj = *((u64 *)INIT_TSCADJ_LOW_ADDR);

	if (tscadj != TSCADJ_UNCHANGE_NEW_VALUE) {
		is_pass = false;
	}
	report("\t\t%s", is_pass, __FUNCTION__);
}

/**
 * @brief Case name: IA32_TSC_AUX init value_001
 *
 * Summary: ACRN hypervisor shall keep guest IA32_TSC_AUX unchanged following INIT.
 *
 */
static void __unused tsc_rqmid_45988_IA32_TSC_AUX_init_value()
{
	volatile u64 tscaux = *((u64 *)INIT_TSCAUX_LOW_ADDR);
	bool is_pass = true;

	if (tscaux != 0x0) {
		is_pass = false;
		printf("%d: tscaux=%ld\n", __LINE__, tscaux);
	}

	notify_modify_and_read_init_value(45988);
	tscaux = *((u64 *)INIT_TSCAUX_LOW_ADDR);
	if (tscaux != TSCAUX_UNCHANGE_NEW_VALUE) {
		printf("%d: tscaux=%ld\n", __LINE__, tscaux);
		is_pass = false;
	}
	report("\t\t%s", is_pass, __FUNCTION__);
}

#define CPU_FREQ_INFO_LEAF 0x16U
u64 cpu_base_freq_khz = 2100000U;
void cpu_base_freq_init(void)
{
	struct cpuid id = cpuid(CPU_FREQ_INFO_LEAF);
	u64 base_freq_mhz = id.a;
	cpu_base_freq_khz = base_freq_mhz * 1000;
}

uint32_t get_tsc_khz(void)
{
	/** Return the frequency of TSC in KHz */
	return cpu_base_freq_khz;
}
__unused uint64_t ticks_to_us(uint64_t ticks)
{
	/** Return the converted microseconds: ticks * 1000 / get_tsc_khz(). */
	return (ticks * 1000UL) / (uint64_t)get_tsc_khz();
}

/**
 * @brief Case name:IA32_TIME_STAMP_COUNTER init value_001
 *
 * Summary: ACRN hypervisor shall keep guest IA32_TIME_STAMP_COUNTER unchanged following INIT.
 *
 */
static void __unused tsc_rqmid_45998_IA32_TIME_STAMP_COUNTER_init_value()
{
	volatile u64 tsc_ap = *((u64 *)INIT_TSC_LOW_ADDR);
	volatile u64 tsc_bp = *((u64 *)BP_TSC_LOW_ADDR);

	bool is_pass = true;

	cpu_base_freq_init();

	/* The real tsc_delta < 1ms if there is no hypervisor printing message
	 * because now hypervisor will print message if writing MSR during send SIPI
	 * here check 100ms instead of 10ms
	 */
	if (ticks_to_us(tsc_ap - tsc_bp) > (100*1000)) {
		is_pass = false;
		printf("%s: tsc_delta = %ld us\r\n", __func__, ticks_to_us(tsc_ap - tsc_bp));
	}

	notify_modify_and_read_init_value(45998);

	/* The real tsc_delta < 1ms if there is no hypervisor printing message
	 * because now hypervisor will print message if writing MSR during send SIPI
	 * here check 100ms instead of 10ms
	 */
	tsc_ap = *((u64 *)INIT_TSC_LOW_ADDR);
	if (ticks_to_us(tsc_ap - TSC_UNCHANGE_NEW_VALUE) > (100*1000)) {
		printf("%d: tsc_delta=%ld\n", __LINE__, ticks_to_us(tsc_ap - TSC_UNCHANGE_NEW_VALUE));
		is_pass = false;
	}
	report("\t\t%s", is_pass, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("\t\t TSC feature case list:\n\r");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 25226u, "CR4.TSD init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 45987u, "IA32_TSC_ADJUST init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 45988u, "IA32_TSC_AUX init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 45998u, "IA32_TIME_STAMP_COUNTER init value_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 39299u, "IA32_TSC_AUX start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39303u, "IA32_TIME_STAMP_COUNTER start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39305u, "IA32_TSC_ADJUST start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39307u, "CR4.TSD start-up value_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 26015u, "Invariant TSC support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24416u, "Nominal core crystal clock frequency_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24463u, "TSC general support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24466u, "TSC auxiliary Support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24468u, "Tsc adjustment support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25225u, "The ACRN need wrap around TSC_001");
}

static void test_tsc(void)
{

	read_bp_startup();
	read_ap_init_val();
#ifdef IN_NON_SAFETY_VM
	tsc_rqmid_45998_IA32_TIME_STAMP_COUNTER_init_value();
	tsc_rqmid_25226_cr4_tsd_init_value();
	tsc_rqmid_45987_IA32_TSC_ADJUST_init_value();
	tsc_rqmid_45988_IA32_TSC_AUX_init_value();
#endif
	tsc_rqmid_26015_invariant_tsc_support();
	tsc_rqmid_39299_IA32_TSC_AUX_startup_value_001();
	tsc_rqmid_39303_IA32_TIME_STAMP_COUNTER_startup_value_001();
	tsc_rqmid_39305_IA32_TSC_ADJUST_startup_value_001();
	tsc_rqmid_39307_CR4_TSD_startup_value_001();
	tsc_rqmid_24416_nom_crystal_clock_freq();
	tsc_rqmid_24466_TSC_auxiliary_Support();
	tsc_rqmid_24448_Tsc_adjustment_support();
	tsc_rqmid_25225_need_wrap_around_TSC();
	tsc_rqmid_24463_TSC_general_support_001();
}

int main(void)
{
	setup_ring_env();
	setup_idt();
	print_case_list();
	test_tsc();
	return report_summary();
}

#endif
