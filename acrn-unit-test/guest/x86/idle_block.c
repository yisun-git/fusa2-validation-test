/**
 * Copyright (C) 2019 Intel Corporation. All right reserved.
 * Test mode: Idle and Blocked Conditions, according to the
 * Chapter 8.10, Vol 3, SDM
 * Written by xuepengcheng, 2019-05-20
 */

#include <stdint.h>
#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "types.h"
#include "isr.h"
#include "apic.h"
#include "misc.h"
#include "register_op.h"
#include "idle_block.h"
#include "regdump.h"
#include "delay.h"
#include "fwcfg.h"

/**/
/********************************************/
/*          timer calibration  */
/********************************************/
uint64_t tsc_hz;
uint64_t apic_timer_hz;
uint64_t cpu_hz;
uint64_t bus_hz;

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

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

__unused static void modify_monitor_filter_size_init_value()
{
	wrmsr(IA32_MONITOR_FILTER_SIZE, 0xFFFF);
}

__unused static void modify_ia32_misc_enable_init_value()
{
	if (cpuid(1).c & (1ul << 3)) {
		wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) | ENABLE_MONITOR_FSM_BIT);
	}
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 24188:
		fp = modify_monitor_filter_size_init_value;
		ap_init_value_process(fp);
		break;
	case 24111:
		fp = modify_ia32_misc_enable_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

static void tsc_calibrate(void)
{
	uint32_t eax_denominator, ebx_numerator, ecx_crystal_hz, reserved;
	uint32_t eax_base_mhz = 0, ebx_max_mhz = 0, ecx_bus_mhz = 0, edx;

	__asm volatile("cpuid"
				   : "=a"(eax_denominator), "=b"(ebx_numerator), "=c"(ecx_crystal_hz), "=d"(reserved)
				   : "0" (0x15), "2" (0)
				   : "memory");

	if ((ecx_crystal_hz != 0) && (eax_denominator != 0)) {
		tsc_hz = ((uint64_t) ecx_crystal_hz *
				  ebx_numerator) / eax_denominator;
		apic_timer_hz = ecx_crystal_hz;
	} else {

		__asm volatile("cpuid"
					   : "=a"(eax_base_mhz), "=b"(ebx_max_mhz), "=c"(ecx_bus_mhz), "=d"(edx)
					   : "0" (0x16), "2" (0)
					   : "memory");

		tsc_hz = (uint64_t) eax_base_mhz * 1000000U;
		apic_timer_hz = tsc_hz * eax_denominator / ebx_numerator;
	}

	cpu_hz = eax_base_mhz * 1000000U;
	bus_hz = ecx_bus_mhz * 1000000U;
}

static void tsc_deadline_timer_isr(isr_regs_t *regs)
{
	eoi();
}
static bool start_dtimer(u64 ticks_interval)
{
	u64 tsc;
	bool ret = true;

	tsc = rdmsr(MSR_IA32_TSC);
	tsc += ticks_interval;
	wrmsr(MSR_IA32_TSCDEADLINE, tsc);
	return ret;
}

static int enable_tsc_deadline_timer(void)
{
	u32 lvtt;
#define	APIC_LVT_TIMER_TSCDEADLINE	(2 << 17)
#define TSC_DEADLINE_TIMER_VECTOR 0xef
	if (cpuid(1).c & (1 << 24)) {
		lvtt = APIC_LVT_TIMER_TSCDEADLINE | TSC_DEADLINE_TIMER_VECTOR;
		apic_write(APIC_LVTT, lvtt);
		return 1;
	} else {
		return 0;
	}
}
static bool init_rtsc_dtimer(void)
{
	tsc_calibrate();

	if (enable_tsc_deadline_timer()) {
		handle_irq(TSC_DEADLINE_TIMER_VECTOR, tsc_deadline_timer_isr);
		irq_enable();
		return true;
	}
	return false;
}
#ifndef IN_NATIVE
static int hlt_checking(void)
{
	/*LOCK prefix hlt, expect #UD*/
	asm volatile (ASM_TRY("1f")
				  "hlt\n\t"
				  "1:"
				  : : );
	return exception_vector();
}

static int monitor_checking(void)
{
	asm volatile (ASM_TRY("1f")
				  "monitor\n\t"
				  "1:"
				  : : );
	return exception_vector();
}

static int mwait_checking(void)
{
	asm volatile (ASM_TRY("1f")
				  "mwait\n\t"
				  "1:"
				  : : );
	return exception_vector();
}

static int test_hide_mnit(int who)
{
	int ret = 0;
	if (who == 0) {
		ret = monitor_checking();/*#UD*/
	} else if (who == 1) {
		ret = mwait_checking();/*#UD*/
	}
	return ret;
}

static void test_hide_monitor(const char *msg)
{
	int ret = 0;
	/*for ring3*/
	ret = test_hide_mnit(0);
	report("%s", ret == UD_VECTOR, msg);
}

static void test_hlt_exception(const char *msg)
{
	int ret = 0;
	/*for ring3*/
	ret = hlt_checking();
	report("%s", (ret == GP_VECTOR) && (exception_error_code() == 0), msg);
}
static int lock_hlt_checking(void)
{
	/*LOCK prefix hlt, expect #UD*/
	asm volatile (ASM_TRY("1f")
				  ".byte 0xf0, 0xf4\n\t"
				  "1:"
				  : : );
	return exception_vector();
}
/**
 * @brief Case name: IDLE hide CPUID leaf 05H_001
 * Summary: Read CPUID.05H in 64bit mode, the result should 0.
 */
static void idle_rqmid_24190_test_hide_cpuid_leaf_05H_001(void)
{
	struct cpuid cpuid5;

	cpuid5 = cpuid(5);

	report("%s",
		   (cpuid5.a == 0) && (cpuid5.b == 0) && (cpuid5.c == 0) && (cpuid5.d == 0), __FUNCTION__);
}
/**
 * @brief Case name:IDLE_expose_PAUSE_support_001
 * Summary: ACRN hypervisor shall expose PAUSE instruction to any VM, reference Chapter 8.10.2 Vol. 3, SDM.
 *    execute PAUSE instruction to ensure ACRN Hypervisor expose PAUSE instruction to any VM
 */
static void idle_rqmid_24308_expose_PAUSE_support_001()
{
	pause();

	report("%s", 1, __FUNCTION__);/*if reach here,there is no exception*/
}
/**
 * @brief Case name: IDLE_expose_PAUSE_support 002
 * Summary: In the 64bit mode, this instruction does not change the architectural state of the
 *        processor before excute PAUSE instruction and after excute PAUSE instruction.
 */
static void idle_rqmid_24318_expose_pause_support_003(void)
{
	bool ret;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile ("pause"));
	report("%s", ret, __FUNCTION__);
}
/**
 * @brief Case name: IDLE expose HLT support
 * Summary: Check cur logical processor resume by timer-interrupt
 *        after execute HLT instruction.
 */
static void idle_rqmid_24322_expose_hlt_support(void)
{
	u64 tsc1, tsc2;

	init_rtsc_dtimer();
	start_dtimer(5 * tsc_hz);/*start 5s timer,after execute hlt, the timer will bring  this core back*/

	tsc1 = rdmsr(MSR_IA32_TSC);
	asm volatile("hlt");
	tsc2 = rdmsr(MSR_IA32_TSC);

	/*the time gap shoud be greater than 5s*/
	report("%s", (tsc2 - tsc1) > (5 * tsc_hz), __FUNCTION__);
}

/**
 * @brief Case name: IDLE IA32_MONITOR_FILTER_SIZE ignore access_001
 * Summary: Write a valid and an invalid value to the IA32_MONITOR_FILTER_SIZE
 *        register in 64bit mode, ACRN hypervisor shall ignore the access.
 *
 *	Write a valid and a invalid value to the IA32_MONITOR_FILTER_SIZE register in 64bit mode,
 *	ACRN hypervisor shall ignore the access.
 */

static void idle_rqmid_24109_test_filter_size_ignore_access_001(void)
{
	u64 ia32_mfs1 = 0;
	u64 ia32_mfs2 = 0;
	u64 ia32_mfs3 = 0;

	ia32_mfs1 = rdmsr(IA32_MONITOR_FILTER_SIZE);

	wrmsr(IA32_MONITOR_FILTER_SIZE, 0x40);
	ia32_mfs2 = rdmsr(IA32_MONITOR_FILTER_SIZE);

	wrmsr(IA32_MONITOR_FILTER_SIZE, 0x48);
	ia32_mfs3 = rdmsr(IA32_MONITOR_FILTER_SIZE);

	report("%s",
		   (ia32_mfs1 == 0) && \
		   (ia32_mfs2 == ia32_mfs1) && \
		   (ia32_mfs3 == ia32_mfs1), __FUNCTION__);

}
/**
 * @brief Case name: IDLE IA32_MISC_ENABLE disable MONITOR_001
 * Summary: Write 1 to IA32_MISC_ENABLE[bit 18] regisitor in 64bit mode,
 *        it shall be generate #GP.
 */
static void idle_rqmid_24275_test_disable_monitor_001(void)
{
	int exv = 0;

	exv = wrmsr_checking(MSR_IA32_MISC_ENABLE,
						 rdmsr(MSR_IA32_MISC_ENABLE) | \
						 MSR_IA32_MISC_ENABLE_MWAIT);
	report("%s", exv == GP_VECTOR, __FUNCTION__);
}
static int lock_pause_checking(void)
{
	/*LOCK prefix for pause, expect #UD*/
	asm volatile (ASM_TRY("1f")
				  ".byte 0xf0, 0xf3, 0x90\n\t"
				  "1:"
				  : :);
	return exception_vector();
}
/**
 * @brief Case name: idle_rqmid_expose_PAUSE_support_002
 * Summary: Execute PAUSE instruction when the LOCK prefix is used, it shall
 *        cause #UD exception.
 */
static void idle_rqmid_24315_expose_pause_support_002(void)
{
	report("%s", lock_pause_checking() == UD_VECTOR, __FUNCTION__);
}
/**
 * @brief Case name: IDLE expose HLT support_003
 * Summary: Execute the HLT when the LOCK prefix is used, it shall
 *        cause #UD exception.
 */
static void idle_rqmid_24320_expose_hlt_support_002(void)
{
	/*LOCK prefix for HLT, expect #UD*/
	report("%s", lock_hlt_checking() == UD_VECTOR, __FUNCTION__);
}
/**
 * @brief Case name: IDLE expose HLT support_003
 * Summary: When the current privilege level is 3, excute HLT
 *        cause #GP exception.
 */
static void idle_rqmid_24321_expose_hlt_support_003(void)
{
	/*expect #GP*/
	do_at_ring3(test_hlt_exception, __FUNCTION__);
}
/**
 * @brief Case name: IDLE hide MONITOR/MWAIT extensions support_001
 * Summary: The hypervisor hide MONITOR/MWAIT extensions.
 *        When CPUID.01H:ECX.MONITOR[bit 3] = 0, execute MONITOR,
 *        it shall cause #UD exception.
 */
static void idle_rqmid_24300_test_hide_monitor_extensions_support_001(void)
{
	/*check hypervisor hide monitor firstly*/
	if ((cpuid(1).c & CPUID_1_ECX_MONITOR) || (cpuid(5).c & CPUID_5_ECX_MONITOR_EXTEN)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", monitor_checking() == UD_VECTOR, __FUNCTION__);
}

/**
 * @brief Case name: IDLE hide MONITOR/MWAIT extensions support_002
 * Summary: If the current privilege level is 3, execute MONITOR, it should
 *        cause #UD exception.
 */

static void idle_rqmid_24306_test_hide_monitor_extensions_support_002(void)
{
	do_at_ring3(test_hide_monitor, __FUNCTION__);

}

void test_hide_mwait_at_ring3(const char *msg)
{
	u8 level;

	level = read_cs() & 0x3;
	report("%s", (mwait_checking() == UD_VECTOR) && (level == 3), msg);
}

/**
 * @brief Case name: IDLE hide MONITOR/MWAIT extensions support_003
 * Summary: If the current privilege level is 3, execute MWAIT, it should cause
 *        #UD exception.
 */
static void idle_rqmid_29578_test_hide_monitor_extensions_support_003(void)
{
	do_at_ring3(test_hide_mwait_at_ring3, __FUNCTION__);
}
/**
 * @brief Case name: IDLE hide MONITOR/MWAIT extensions support_004
 * Summary: if CPUID.01H:ECX.MONITOR[bit 3] = 0, cause #UD exception.
 * And CPUID.05H.ECX[bit 0].MONITOR = 0,
 */
static void idle_rqmid_29579_test_hide_monitor_extensions_support_004(void)
{
	/*check hypervisor hide monitor firstly*/
	if ((cpuid(1).c & CPUID_1_ECX_MONITOR) || (cpuid(5).c & CPUID_5_ECX_MONITOR_EXTEN)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", mwait_checking() == UD_VECTOR, __FUNCTION__);
}
/**
 * @brief Case name: IDLE hide MONITOR MWAIT support 001
 * Summary:
 * If CPUID.01H:ECX.MONITOR[bit 3] = 0, cause #UD exception.
 * MONITOR,reference the Chapter 8.10 , Vol. 3, SDM and Chapter 4.3, Vol 2, SDM
 *
 */
static void idle_rqmid_24112_hide_Moniter_Mwait_support_001(void)
{
	/*CPUID.01H:ECX[bit 3] should be 0*/
	if ((cpuid(1).c & CPUID_1_ECX_MONITOR) || (rdmsr(MSR_IA32_MISC_ENABLE) & ENABLE_MONITOR_FSM_BIT)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	report("%s", monitor_checking() == UD_VECTOR, __FUNCTION__);
}
/**
 * @brief Case name: IDLE hide MONITOR/MWAIT support_002
 * Summary: If the current privilege level is 3, execute MONITOR, it should
 *        cause #UD exception.
 */
static void idle_rqmid_24113_hide_Moniter_Mwait_support_002(void)
{
	do_at_ring3(test_hide_monitor, __FUNCTION__);
}

/**
 * @brief Case name: IDLE hide MONITOR/MWAIT support_003
 * Summary: The hypervisor hide MONITOR/MWAIT.
 *        When CPUID.01H:ECX.MONITOR[bit 3] = 0, execute MWAIT, 
 *        it shall cause #UD exception.
 */
static void idle_rqmid_29780_hide_Moniter_Mwait_support_003(void)
{
	if ((cpuid(1).c & CPUID_1_ECX_MONITOR) || (rdmsr(MSR_IA32_MISC_ENABLE) & ENABLE_MONITOR_FSM_BIT)) {
		report("%s", 0, __FUNCTION__);
		return;
	}
	report("%s", mwait_checking() == UD_VECTOR, __FUNCTION__);
}
/**
 * @brief Case name: IDLE hide MONITOR/MWAIT support_004
 * Summary: If the current privilege level is 3, execute MWAIT, it should cause
 *        #UD exception.
 */
static void idle_rqmid_29781_hide_monitor_Mwati_support_004(void)
{
	do_at_ring3(test_hide_mwait_at_ring3, __FUNCTION__);
}
/**
 * @brief Case name: IDLE_IA32_MONITOR_FILTER_SIZE_initial_state_following_start-up_001
 * Summary: ACRN hypervisor shall set initial guest IA32_MONITOR_FILTER_SIZE  to 0 following start-up.
 *	IA32_MONITOR_FILTER_SIZE, reference the Table 2-2, Vol. 4, SDM.
 */

static void idle_rqmid_24186_IA32_monitor_filter_size_following_start_up_001(void)
{
	volatile u64 *ptr = (volatile u64 *)IA32_MONITOR_FILTER_SIZE_STARTUP_ADDR;

	report("%s", *ptr == 0, __FUNCTION__);
}
/**
 * @brief case name: IDLE IA32_MISC_ENABLE initial state following start up 001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 18] at BP start-up, the result shall be 0H.
 */
static void idle_rqmid_24185_ia32_misc_enable_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)IA32_MISC_ENABLE_STARTUP_ADDR;

	report("%s", (*ptr & ENABLE_MONITOR_FSM_BIT) == 0U,
		   __FUNCTION__);
}
#ifdef IN_NON_SAFETY_VM
/**
 * @brief Case name: IDLE_IA32_MONITOR_FILTER_SIZE_initial_state_following_INIT_001
 * Summary: AACRN hypervisor shall set initial guest IA32_MONITOR_FILTER_SIZE  to 0 following INIT.
 * IA32_MONITOR_FILTER_SIZE, reference the Table 2-2, Vol. 4, SDM.
 */

static void idle_rqmid_24188_IA32_monitor_filter_size_following_init_001(void)
{
	volatile u64 *ptr = (volatile u64 *)IA32_MONITOR_FILTER_SIZE_INIT_ADDR;
	bool is_pass = true;

	if (*ptr != 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(24188);
	if (*ptr != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: IDLE IA32_MISC_ENABLE initial state following start-up_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 18] at AP entry, the result shall be 0H.
 */
static void idle_rqmid_24111_ia32_misc_enable_following_init_001(void)
{
	volatile u32 *ptr = (volatile u32 *)IA32_MISC_ENABLE_INIT1_ADDR;
	bool is_pass = true;

	if ((*ptr & ENABLE_MONITOR_FSM_BIT) != 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(24111);

	if ((*ptr & ENABLE_MONITOR_FSM_BIT) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}
#endif
#endif
#ifdef IN_NATIVE
/**
 * @brief Case name:IDLE physical expose PAUSE support 001
 * Summary: PAUSE instruction shall be available on the physical platform.
 *
 * execute PAUSE instruction to ensure the physical platform expose PAUSE instruction.
 */
static void idle_rqmid_39159_idle_physical_expose_PAUSE_support_001()
{
	pause();

	report("%s", 1, __FUNCTION__);/*if reach here,there is no exception*/
}
/**
 * @brief Case IDLE physical expose PAUSE support 002
 * Summary:
 *
 *  This instruction does not change the architectural state of the processor
 * (that is, it performs essentially a delaying no-op operation).
 * PAUSE instruction, refer to the Chapter 4.3, VOL. 2, SDM
 *
 *In the 64bit mode, on native platform,
 * this instruction does not change the architectural state of the processor
 * before execute PAUSE instruction and after execute PAUSE instruction.
 */
static void idle_rqmid_39160_idle_physical_expose_PAUSE_support_002()
{
	bool ret;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile ("pause"));
	report("%s", ret, __FUNCTION__);
}
/**
 * @brief Case name: IDLE physical HLT support
 * Summary: Check the processor resume by timer-interrupt
 *        after execute HLT instruction.
 */
static void idle_rqmid_39161_idle_physical_hlt_support(void)
{
	u64 tsc1, tsc2;

	init_rtsc_dtimer();
	start_dtimer(5 * tsc_hz);/*start 5s timer,after execute hlt, the timer will bring  this core back*/

	tsc1 = rdmsr(MSR_IA32_TSC);
	asm volatile("hlt");
	tsc2 = rdmsr(MSR_IA32_TSC);

	/*the time gap shoud be greater than 5s*/
	report("%s", (tsc2 - tsc1) > (5 * tsc_hz), __FUNCTION__);
}

#endif
static void print_case_list(void)
{
#ifndef IN_NATIVE
	printf("idle && block feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 24190,
		   "IDLE hide CPUID leaf 05H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24308,
		   "idle_rqmid_24308_expose_PAUSE_support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24318,
		   "IDLE expose PAUSE support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24322,
		   "idle_rqmid_24322_expose_hlt_support");
	printf("\t\t Case ID:%d case name:%s\n\r", 24109,
		   "IDLE IA32_MONITOR_FILTER_SIZE ignore access_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24275,
		   "IDLE IA32_MISC_ENABLE disable MONITOR_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24315,
		   "idle_rqmid_24315_expose_pause_support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 24320,
		   "idle_rqmid_24320_expose_hlt_support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 24321,
		   "idle_rqmid_24321_expose_hlt_support_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 24300,
		   "idle_rqmid_24300_test_hide_monitor_extensions_support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24306,
		   "idle_rqmid_24306_test_hide_monitor_extensions_support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29578,
		   "idle_rqmid_29578_test_hide_monitor_extensions_support_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 29579,
		   "idle_rqmid_29579_test_hide_monitor_extensions_support_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 24112,
		   "idle_rqmid_24112_hide_Moniter_Mwait_support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24113,
		   "idle_rqmid_24113_hide_Moniter_Mwait_support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29780,
		   "idle_rqmid_29780_hide_Moniter_Mwait_support_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 29781,
		   "idle_rqmid_29780_hide_Moniter_Mwait_support_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 24186,
		   "idle_rqmid_24186_IA32_monitor_filter_size_following_start_up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24185,
		   "idle_rqmid_24185_ia32_misc_enable_following_start_up_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 24188,
		   "idle_rqmid_24188_IA32_monitor_filter_size_following_init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24111,
		   "idle_rqmid_24111_ia32_misc_enable_following_init_001");
#endif
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 39159,
		   "idle_rqmid_39159_idle_physical_expose_PAUSE_support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39160,
		   "idle_rqmid_39160_idle_physical_expose_PAUSE_support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 39161,
		   "idle_rqmid_39161_idle_physical_hlt_support");
#endif
}
static void test_idbl(void)
{
#ifndef IN_NATIVE
	idle_rqmid_24190_test_hide_cpuid_leaf_05H_001();
	idle_rqmid_24308_expose_PAUSE_support_001();
	idle_rqmid_24318_expose_pause_support_003();
	idle_rqmid_24322_expose_hlt_support();
	idle_rqmid_24109_test_filter_size_ignore_access_001();
	idle_rqmid_24275_test_disable_monitor_001();
	idle_rqmid_24315_expose_pause_support_002();
	idle_rqmid_24320_expose_hlt_support_002();
	idle_rqmid_24321_expose_hlt_support_003();
	idle_rqmid_24300_test_hide_monitor_extensions_support_001();
	idle_rqmid_24306_test_hide_monitor_extensions_support_002();
	idle_rqmid_29578_test_hide_monitor_extensions_support_003();
	idle_rqmid_29579_test_hide_monitor_extensions_support_004();
	idle_rqmid_24112_hide_Moniter_Mwait_support_001();
	idle_rqmid_24113_hide_Moniter_Mwait_support_002();
	idle_rqmid_29780_hide_Moniter_Mwait_support_003();
	idle_rqmid_29781_hide_monitor_Mwati_support_004();
	idle_rqmid_24186_IA32_monitor_filter_size_following_start_up_001();
	idle_rqmid_24185_ia32_misc_enable_following_start_up_001();
#ifdef IN_NON_SAFETY_VM
	idle_rqmid_24188_IA32_monitor_filter_size_following_init_001();
	idle_rqmid_24111_ia32_misc_enable_following_init_001();
#endif
#else
	idle_rqmid_39159_idle_physical_expose_PAUSE_support_001();
	idle_rqmid_39160_idle_physical_expose_PAUSE_support_002();
	idle_rqmid_39161_idle_physical_hlt_support();
#endif
}

int main(void)
{
	setup_idt();
	setup_ring_env();

	print_case_list();
	test_idbl();

	return report_summary();
}
