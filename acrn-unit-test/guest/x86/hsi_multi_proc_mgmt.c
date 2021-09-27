/*
 * Copyright (C) 2020 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2019/12/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 * Modify Author:       hexiangx.li@intel.com
 * Date :                       2020/10/13
 *
 */
//#define USE_DEBUG
#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include <linux/pci_regs.h>
#include "alloc_page.h"
#include "alloc_phys.h"
#include "asm/page.h"
#include "x86/vm.h"
#include "xsave.h"
#include "segmentation.h"
#include "interrupt.h"
#include "instruction_common.h"
#include "memory_type.h"
#include "debug_print.h"

typedef struct {
	int case_id;
	const char *case_name;
	void (*case_fun)(void);
} st_case_suit_multi;

#define CFG_TEST_ORDERING_CPU_INIT_VALUE	0
#define CFG_TEST_ORDERING_READY		1
#define CFG_ORDER_AP_START_ID		1
#define CFG_TEST_MEMORY_ALL_CPU_READY		2
#define CFG_TEST_ORDERING_CPU_NR 3

#define CFG_TEST_MEMORY_ALL_CPU_READY_NATIVE	3
#define CFG_TEST_ORDERING_CPU_NR_NATIVE	4

#define CFG_ORDER_DEBUG_TIMES	100000
#define CFG_TEST_ORDERING_TIMES	1000000UL
#define AP1_ID 1
#define AP2_ID 2

enum {
	CASE_ID_MOV = 39838,
	CASE_ID_CPUID = 39389,
	CASE_ID_MFENCE = 39843,
	CASE_ID_MEMORY_ORDER = 40151,
	CASE_ID_BUTT,
};

static volatile ulong g_cr2;
enum order_type {
	/* not use mov to control register (not serializing executed) */
	ORDER_TEST_NO_MOV_1 = 0,

	/* use mov to control register(serializing executed) */
	ORDER_TEST_MOV_2,

	/* not use cpuid (not serializing executed) */
	ORDER_TEST_NO_CPUID_1,

	/* use cpuid (serializing executed) */
	ORDER_TEST_CPUID_2,

	/* not use mfence (not serializing executed) */
	ORDER_TEST_NO_MFENCE_1,

	/* use mfence (serializing executed) */
	ORDER_TEST_MFENCE_2,

	/* use mfence (memory Re-Ordered) */
	ORDER_TEST_MEMORY_ORDER,

	/* MAX TYPE */
	ORDER_TEST_MAX,
};

//static volatile int start_run_id = 0;

static atomic_t ordering_begin_sem1;
static atomic_t ordering_begin_sem2;
static atomic_t ordering_end_sem;
static atomic_t ordering_begin_sem_ap;
static volatile int order_sync = ORDER_TEST_MAX;

static int X;
static int Y;
static int r1;
static int r2;

#define CPUID_1_ECX_MONITOR			(1 << 3)
#define USING_APS                   2
#define USING_ONE_AP 1
#define LOOP_COUNT                  100000
#define LOOP_COUNT_ADD				1000
#define INSTRUCTION_ADD                         0
#define INSTRUCTION_XCHG                        1
#define INSTRUCTION_MOV							2
#define INSTRUCTION_MWAIT						3

#define NONE_TEST                   0
#define ORG_ATO_VAL0                0xa3
#define ORG_ATO_VAL1                0xf0
#define ORG_ATO_VAL2                0x0f

/* start_run_id is used to identify which test case is running. */
static volatile int start_run_id = 0;

/* wait_bp/wait_ap are used to sync between bp and aps */
static atomic_t wait_bp;
static atomic_t wait_ap;

static volatile u8 ap_nums = 3;
static volatile int target_ap_startup = 0;
static atomic_t ap_run_nums;

static volatile int test_atomic_type = 0;
static u32 pass_ato_cnt = 0;
static u32 fail_ato_cnt = 0;
static volatile u32 reg_val_1 = 0;
static volatile u32 reg_val_2 = 0;
static volatile u8 begin_sim[2];
atomic_t end_sim;

/* used for mov instruction test */
static volatile __attribute__ ((aligned(32))) u32 val_aligned;
static volatile u32 *aligned_addr = &val_aligned;

/* used for add instruction test */
static volatile u32 val_test0;
static volatile u32 *test_addr0 = &val_test0;

/* used for xchg instruction test */
static volatile u32 val_test1;
static volatile u32 *test_addr1 = &val_test1;

typedef void (*test_fn)(u32 volatile *, u32 volatile *);

static u32 const xchg_check_array[6][3] = {
	{ORG_ATO_VAL0, ORG_ATO_VAL1, ORG_ATO_VAL2},
	{ORG_ATO_VAL0, ORG_ATO_VAL2, ORG_ATO_VAL1},
	{ORG_ATO_VAL1, ORG_ATO_VAL0, ORG_ATO_VAL2},
	{ORG_ATO_VAL1, ORG_ATO_VAL2, ORG_ATO_VAL0},
	{ORG_ATO_VAL2, ORG_ATO_VAL0, ORG_ATO_VAL1},
	{ORG_ATO_VAL2, ORG_ATO_VAL1, ORG_ATO_VAL0},
};

static volatile u32 test_hlt;
static volatile bool is_next_ins_executed;
#define HLT_EXECUTE_BEFORE 0xFF
#define HLT_EXECUTE_AFTER 1
typedef void (*trigger_func)(void *data);

static int hlt_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"hlt\n"
		"1:"
		:
	);
	return exception_vector();
}

static u8 monitor_val = 0;


static int monitor_checking(volatile u8 *addr, u64 ecx, u64 edx)
{
	asm volatile(ASM_TRY("1f")
		"monitor\n"
		"1:"
		: : "a" (addr), "c" (ecx), "d" (edx)
		: "memory"
	);
	return exception_vector();
}

static int mwait_checking(u64 eax, u64 ecx)
{
	asm volatile(ASM_TRY("1f")
		"mwait\n"
		"1:"
		: : "a" (eax), "c" (ecx)
	);
	return exception_vector();
}

static void bp_init_sem_for_ap(void)
{
	begin_sim[0] = 0;
	begin_sim[1] = 0;
	atomic_set(&end_sim, 0);
	test_atomic_type = INSTRUCTION_MWAIT;
	atomic_set(&wait_bp, USING_APS);
}

void check_next_instruction_with_irq(isr_regs_t *regs)
{
	if (test_hlt == HLT_EXECUTE_BEFORE) {
		is_next_ins_executed = false;
	}
	eoi();
}

static int pause_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"pause\n"
		"1:"
		: : : "memory"
	);
	return exception_vector();
}

static void test_delay(u32 time)
{
	unsigned long long tsc;
	tsc = rdtsc() + ((u64)time * 2100000);

	while (rdtsc() < tsc) {
		;
	}
}

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

static void lock_add_test(u32 volatile *addr, __unused u32 volatile *rval)
{
	asm volatile(
		"lock add $0x1, %0\n"
		: "+m" (*addr)
		: : "memory"
	);
}

static void lock_xchg_test(u32 volatile *addr, u32 volatile *rval)
{
	asm volatile(
		"lock xchg %0, %1\n"
		: "+r" (*rval), "+m" (*addr)
		: : "memory", "0"
	);
}

static void lock_mov_test(u32 volatile *addr, u32 volatile *rval)
{
	__asm__ __volatile__(
		" mov %1, %0 \n\t"
		: "+m" (*addr)
		: "a" (*rval)
		: "memory");
}

static __unused void mwait_test_ap1(void)
{
	debug_print("ap1 monitor_val:%d\n", monitor_val);
	test_delay(100);
	monitor_val++;
}

static __unused void null_test_ap2(void)
{

}

test_fn test_func[] = {
	lock_add_test,
	lock_xchg_test,
	lock_mov_test,
};

/* Description: BP will check result after each run of 2 APs for xchg instruction */
static void check_result_for_each_run()
{
	u32 volatile *addr;
	addr = test_addr1;
	bool ret = false;
	u32 i;
	for (i = 0; i < ARRAY_SIZE(xchg_check_array); i++) {
		if ((xchg_check_array[i][0] == *addr) &&
			(xchg_check_array[i][1] == reg_val_1) &&
			(xchg_check_array[i][2] == reg_val_2)) {
			ret = true;
			break;
		}
	}
	if (ret == true) {
		pass_ato_cnt++;
	} else {
		fail_ato_cnt++;
	}
}

/* Description: BP will check result after each run of 2 APs for mov instruction */
static void check_result_for_mov_each_run()
{
	u32 volatile *addr;
	addr = aligned_addr;

	if ((*addr == ORG_ATO_VAL1) ||
		(*addr == ORG_ATO_VAL2)) {
		pass_ato_cnt++;
	} else {
		fail_ato_cnt++;
	}
}

/* Description: BP will control 2 APs to start/end, and check result */
static void lock_atomic_test_bp(u8 instr)
{
	u32 i;

	begin_sim[0] = 0;
	begin_sim[1] = 0;
	atomic_set(&end_sim, 0);
	test_atomic_type = instr;
	atomic_set(&wait_bp, USING_APS);

	u32 loop_times = LOOP_COUNT;
	/* test 2000 times for add instruction */
	if (instr == INSTRUCTION_ADD) {
		loop_times = LOOP_COUNT_ADD;
	}
	for (i = 0; i < loop_times; i++) {
		/* tell AP to start */
		begin_sim[0]++;
		begin_sim[1]++;

		while (atomic_read(&end_sim) != USING_APS) {}
		switch (instr) {
		case INSTRUCTION_XCHG:
			check_result_for_each_run();
			break;
		case INSTRUCTION_MOV:
			check_result_for_mov_each_run();
			break;
		default:
			break;
		}

		atomic_set(&end_sim, 0);
	}
	test_atomic_type = NONE_TEST;
}

/* Description: 2 APs will test concurrently */
static __unused void lock_atomic_test_ap(u8 instr)
{
	u8 index = get_lapic_id() / 2 - 1;
	u32 volatile *addr;
	u32 volatile *rval;
	void (*fn)(u32 volatile *, u32 volatile *) = NULL;

	if (instr == INSTRUCTION_MOV) {
		addr = aligned_addr;
	} else if (instr == INSTRUCTION_ADD) {
		addr = test_addr0;
	} else {
		addr = test_addr1;
	}
	fn = test_func[instr];

	if (index == 0) {
		rval = &reg_val_1;
	} else {
		rval = &reg_val_2;
	}

	ap_sync();
	while (test_atomic_type == instr) {
		if (begin_sim[index] == 1) {
			begin_sim[index]--;
			fn(addr, rval);
			atomic_inc(&end_sim);
		}
	}
}

static void mwait_test_ap(u8 instr)
{
	u8 index = get_lapic_id() / 2 - 1;
	void (*fn)(void) = NULL;

	if (index == 0) {
		fn = mwait_test_ap1;
	} else {
		fn = null_test_ap2;
	}

	ap_sync();
	while (test_atomic_type == instr) {
		if (begin_sim[index] == 1) {
			begin_sim[index]--;
			fn();
			atomic_inc(&end_sim);
		}
	}
}

/* Description: BP will control 2 APs to start/end, and check result */
static __unused void mwait_test_bp(u8 instr)
{
	u32 i;

	begin_sim[0] = 0;
	begin_sim[1] = 0;
	atomic_set(&end_sim, 0);
	test_atomic_type = instr;
	atomic_set(&wait_bp, USING_APS);

	u32 loop_times = 2;

	for (i = 0; i < loop_times; i++) {
		/* tell AP to start */
		begin_sim[0]++;
		begin_sim[1]++;

		while (atomic_read(&end_sim) != USING_APS) {}
		atomic_set(&end_sim, 0);
	}
	test_atomic_type = NONE_TEST;
}

/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_001
 *
 * Summary: Test with multi-core - AP1 and AP2 to execute ADD instruction for many
 * times to add values of a global variable (val1) with 1 concurrently for 1000 times;
 * BP to control the synchronization of 2 APs. The result of val1 should equal to 2000
 */
static __unused void hsi_rqmid_35972_multi_proc_mgmt_features_locked_atomic_operations_001()
{
	u32 chk = 0;
	u32 total = USING_APS * LOOP_COUNT_ADD;

	start_run_id = 35972;

	/* test instruction ADD with lock prefix */
	*test_addr0 = 0;
	lock_atomic_test_bp(INSTRUCTION_ADD);
	debug_print("lock add: val=%d\n", *test_addr0);
	if (*test_addr0 == total) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_002
 *
 * Summary: Under 64 bit mode on native board, 2 APs execute XCHG instruction
 * concurrently, the execution result should show atomicity,
 */
static __unused void hsi_rqmid_35973_multi_proc_mgmt_features_locked_atomic_operations_002()
{
	u32 chk = 0;

	start_run_id = 35973;

	/* test automatic locking instruction XCHG */
	*test_addr1 = ORG_ATO_VAL0;
	reg_val_1 = ORG_ATO_VAL1;
	reg_val_2 = ORG_ATO_VAL2;
	pass_ato_cnt = 0;
	fail_ato_cnt = 0;
	lock_atomic_test_bp(INSTRUCTION_XCHG);
	debug_print("xchg: rval1=%d, rval2=%d, *addr=%d\n", reg_val_1, reg_val_2, *test_addr1);
	debug_print("pass_cnt=%d, fail_cnt=%d\n", pass_ato_cnt, fail_ato_cnt);
	if ((pass_ato_cnt == LOOP_COUNT) && (fail_ato_cnt == 0)) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Multi_Processor_Initialization_001
 *
 * Summary: On native board, startup all APs should be successfully.
 */
static __unused void hsi_rqmid_35955_multi_proc_mgmt_features_multi_processor_initialization_001()
{
	u32 chk = 0;
	u8 ap_nums = fwcfg_get_nb_cpus() - 1;

	atomic_set(&ap_run_nums, 0);
	start_run_id = 35955;

	/* wait for 8ms */
	test_delay(8);
	if (atomic_read(&ap_run_nums) == ap_nums) {
		chk++;
	}

	debug_print("ap_run_nums:%d\n", ap_run_nums.counter);
	report("%s", (chk == 1), __FUNCTION__);
}


/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_003
 *
 * Summary: Under 64 bit mode on native board, 2 APs to both read and write a double word memory aligned on 32-bit
 * via MOV instruction for many times concurrently; BP to control the synchronization of 2 APs.
 * Read value stored in the memory address after each write operation to verify whether the write is successful.
 */
static __unused void hsi_rqmid_39720_multi_proc_mgmt_features_locked_atomic_operations_003()
{
	u32 chk = 0;

	start_run_id = 39720;

	/* test the atomicity of aligned memory accesses with instruction MOV. */
	*aligned_addr = 0;

	/* set aps default value */
	reg_val_1 = ORG_ATO_VAL1;
	reg_val_2 = ORG_ATO_VAL2;

	/* clear the count */
	pass_ato_cnt = 0;
	fail_ato_cnt = 0;
	lock_atomic_test_bp(INSTRUCTION_MOV);
	debug_print("pass_cnt=%d, fail_cnt=%d\n", pass_ato_cnt, fail_ato_cnt);
	if ((pass_ato_cnt == LOOP_COUNT) && (fail_ato_cnt == 0)) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_001
 *
 * Summary: Under 64 bit mode on native board, arm apic timer, when execute
 * instruction HLT, no exception happens and the program continues to run
 * after apic timer interrupt happens.
 */
static __unused void hsi_rqmid_35975_multi_proc_mgmt_features_idle_and_blocked_conditions_001()
{
	u32 chk = 0;
	int result = 1;
	test_hlt = HLT_EXECUTE_BEFORE;
	is_next_ins_executed = true;

	/* wait for 8ms */
	test_delay(8);

	irq_disable();
	handle_irq(EXTEND_INTERRUPT_80, check_next_instruction_with_irq);
	sti();

	/* step 4 use TSC deadline timer delivery 224 interrupt*/
	if (cpuid(1).c & (1 << 24))
	{
		apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_80);
		wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x10000));
	}

	/* test hlt instruction */
	result = hlt_checking();
	/* after hlt instruction set value */
	test_hlt = HLT_EXECUTE_AFTER;
	if ((is_next_ins_executed == false) &&
		(result == NO_EXCEPTION)) {
		chk++;
	}

	debug_print("result=%d is_next_ins_executed=%d\n", result, is_next_ins_executed);
	/*if reach here,there is no exception*/
	report("%s\n", chk == 1, __FUNCTION__);
}


/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_002
 *
 * Summary: Under 64 bit mode on native board, execute instruction PAUSE, no exception happens.
 */
static __unused void hsi_rqmid_35976_multi_proc_mgmt_features_idle_and_blocked_conditions_002()
{

	u32 chk = 0;
	int result = 1;

	/* execute the following instructions in IA-32e mode */
	/* test instruction PAUSE */
	result = pause_checking();
	if (result == NO_EXCEPTION) {
		chk++;
	}

	debug_print("result=%d\n", result);
	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_003
 *
 * Summary: Under 64 bit mode on native board, execute instruction MONITOR and MWAITE
 * to monitor the change in a memory address, no exception happens and the program
 * continues to run after specified changes happens.
 */
static __unused void hsi_rqmid_35977_multi_proc_mgmt_features_idle_and_blocked_conditions_003()
{
	u32 chk = 0;
	u8 rst1 = 1;
	u8 rst2 = 1;

	/* test instructions MONITOR/MWAITE */
	if ((cpuid(1).c & CPUID_1_ECX_MONITOR) != 0) {
		/* ap start run case */
		start_run_id = 35977;
		monitor_val = 0;
		bp_init_sem_for_ap();

		rst1 = monitor_checking(&monitor_val, 0, 0);
		if (rst1 == NO_EXCEPTION) {
			chk++;
		}

		/* ap start change monitor_val value */
		begin_sim[0]++;

		/* test mait instruction */
		rst2 = mwait_checking(0, 0);

		while (atomic_read(&end_sim) != USING_ONE_AP) {}
		atomic_set(&end_sim, 0);

		if (rst2 == NO_EXCEPTION) {
			chk++;
		}
		test_atomic_type = NONE_TEST;

	}

	report("%s", (chk == 2), __FUNCTION__);
}

static void asm_test_null(void)
{
}

static void asm_test_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mfence\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}

static void asm_test_mfence_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mfence\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X)
		: "memory");
}

static void asm_test_mov_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mov %3, %%cr2\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y), "r"(g_cr2)
		: "memory");
}

static void asm_test_mov_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mov %3, %%cr2\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X), "r"(g_cr2)
		: "memory");
}

static void asm_test_cpuid_ap1(void)
{
	u32 a, b, c, d;
	asm volatile(
		"movl $1, %1\n\t"
		"cpuid \n\t"
		"movl %6, %0\n\t"
		: "=r"(r1), "=m" (X), "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		: "m"(Y), "2" (1), "4" (0)
		: "memory");
}

static void asm_test_cpuid_ap2(void)
{

	u32 a, b, c, d;
	asm volatile(
		"movl $1, %1\n\t"
		"cpuid \n\t"
		"movl %6, %0\n\t"
		: "=r"(r2), "=m" (Y), "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		: "m"(X), "2" (1), "4" (0)
		: "memory");
}

static void asm_test_no_serializing_instruction_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}

static void asm_test_no_serializing_instruction_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X)
		: "memory");
}

static void asm_test_init()
{
	X = 0;
	Y = 0;
}

static int bp_check_result()
{
	int detected = 0;
	/**
	 * According to SDM Table Example 8-3:
	 * r1 = 0 and r2 = 0 is allowed, if not use specific instruction
	 * we should catch at least 1 time that r1 and r2 are 0 at the same time,
	 * otherwise use specific instruction,  r1 = 0 and r2 = 0 is not allowed
	 */
	if ((r1 == 0) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

/* all test ordering instruction function at ap1, ap2 */
static void (*ordering_test_type[ORDER_TEST_MAX+1][CFG_TEST_ORDERING_CPU_NR_NATIVE])(void) = {
	/* ORDER_TEST_NO_MOV_1 */
	{
		NULL,
		asm_test_no_serializing_instruction_ap1,
		asm_test_no_serializing_instruction_ap2,
		NULL,
	},

	/* ORDER_TEST_MOV_2 */
	{
		NULL,
		asm_test_mov_ap1,
		asm_test_mov_ap2,
		NULL,
	},

	/* ORDER_TEST_NO_CPUID_1 */
	{
		NULL,
		asm_test_no_serializing_instruction_ap1,
		asm_test_no_serializing_instruction_ap2,
		NULL,
	},

	/* ORDER_TEST_CPUID_2 */
	{
		NULL,
		asm_test_cpuid_ap1,
		asm_test_cpuid_ap2,
		NULL,
	},

	/* ORDER_TEST_NO_MFENCE_1 */
	{
		NULL,
		asm_test_no_serializing_instruction_ap1,
		asm_test_no_serializing_instruction_ap2,
		NULL,
	},

	/* ORDER_TEST_MFENCE_2 */
	{
		NULL,
		asm_test_mfence_ap1,
		asm_test_mfence_ap2,
		NULL,
	},

	/* ORDER_TEST_MEMORY_ORDER */
	{
		NULL,
		asm_test_mfence_ap1,
		asm_test_mfence_ap2,
		NULL,
	},

	/* ORDER_TEST_MAX */
	{
		asm_test_null,
		asm_test_null,
		asm_test_null,
		asm_test_null,
	},
};

/* define for all AP1 cases entry */
static void ordering_instruction_test1(void)
{
	/* wait for bp start ap1 test case */
	while (atomic_read(&ordering_begin_sem1) !=
		CFG_TEST_ORDERING_READY) {
		nop();
	}
	atomic_dec(&ordering_begin_sem1);
	ordering_test_type[order_sync][AP1_ID]();
	atomic_inc(&ordering_end_sem);
}

/* define for all AP2 cases entry */
static void ordering_instruction_test2(void)
{
	/* wait for bp start ap2 test case */
	while (atomic_read(&ordering_begin_sem2) !=
		CFG_TEST_ORDERING_READY) {
		nop();
	}
	atomic_dec(&ordering_begin_sem2);
	ordering_test_type[order_sync][AP2_ID]();
	atomic_inc(&ordering_end_sem);
}

static void (*ordering_ap_test_entry[CFG_TEST_ORDERING_CPU_NR_NATIVE])(void) = {
	NULL,							//BP entry
	ordering_instruction_test1,	//AP1 entry
	ordering_instruction_test2,	//AP2 entry
	NULL, //AP3 not used
};

static void ordering_memory_type(int memory_type)
{
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 6(WB), 5(WP), 4(WT) 0(UC)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0xFF)) | memory_type);
}

/* bp start test case with two aps when bp send start sync */
static unsigned long ordering_bp_test_two_ap(int run_id, int test_id, int memory_type)
{
	unsigned long i;
	int detected = 0;

	start_run_id = run_id;
	order_sync = test_id;

	/* init all sync variables */
	atomic_set(&ordering_begin_sem1, CFG_TEST_ORDERING_CPU_INIT_VALUE);
	atomic_set(&ordering_begin_sem2, CFG_TEST_ORDERING_CPU_INIT_VALUE);
	atomic_set(&ordering_begin_sem_ap, CFG_TEST_ORDERING_CPU_INIT_VALUE);
	atomic_set(&ordering_end_sem, CFG_TEST_ORDERING_CPU_INIT_VALUE);

	/* set current test memory type */
	ordering_memory_type(memory_type);

	for (i = 1; i < CFG_TEST_ORDERING_TIMES; ++i) {
		asm_test_init();

		/* bp sync ap to start */
		atomic_inc(&ordering_begin_sem1);
		atomic_inc(&ordering_begin_sem2);

		/* bp wait 2 aps finished test */
		while (atomic_read(&ordering_end_sem) != CFG_TEST_MEMORY_ALL_CPU_READY) {
			nop();
		}

		atomic_set(&ordering_end_sem, CFG_TEST_ORDERING_CPU_INIT_VALUE);

		if (bp_check_result()) {
			detected++;
			if (detected <= 1) {
				printf("type=%d %d reorders detected after %ld iterations\n",
					order_sync, detected, i);
			}
		}

		if ((i % CFG_ORDER_DEBUG_TIMES) == 0) {
			debug_print("%s: times %lu\n", __FUNCTION__, i);
		}
	}

	order_sync = ORDER_TEST_MAX;
	/* return the reorder times */
	return detected;
}

/**
 * @brief case name: HSI_Multi-Processor_Management_Features_Serializing_Instructions_mov_001
 *
 * Summary: Mov to control register instructions force the processor to complete
 * all modifications to memory by previous instructions
 * and to drain all buffered writes to memory before the next instruction is fetched and executed
 *
 */
__unused static void hsi_rqmid_39838_multi_proc_mgmt_features_serializing_instructions_mov_001(void)
{
	unsigned long result[2];

	/**
	 * not used mov to control register
	 * we can catch at least 1 time that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should not be 0
	 */
	result[0] = ordering_bp_test_two_ap(CASE_ID_MOV, ORDER_TEST_NO_MOV_1, CACHE_PAT_WB);

	/**
	 * used mov to control register
	 * we can not catch the case that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should be 0
	 */
	result[1] = ordering_bp_test_two_ap(CASE_ID_MOV, ORDER_TEST_MOV_2, CACHE_PAT_WB);

	report("%s %ld %ld", ((result[0] != 0) && (result[1] == 0)), __FUNCTION__, result[0], result[1]);
}

/**
 * @brief case name: HSI_Multi-Processor_Management_Features_Serializing_Instructions_cpuid_001
 *
 * Summary: CPUID instructions force the processor to complete
 * all modifications to memory by previous instructions
 * and to drain all buffered writes to memory before the next instruction is fetched and executed
 *
 */
__unused static void hsi_rqmid_39839_multi_proc_mgmt_features_serializing_instructions_cpuid_002(void)
{
	unsigned long result[2];

	/**
	 * not used cpuid
	 * we can catch at least 1 time that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should not be 0
	 */
	result[0] = ordering_bp_test_two_ap(CASE_ID_CPUID, ORDER_TEST_NO_CPUID_1, CACHE_PAT_WB);

	/**
	 * used cpuid
	 * we can not catch the case that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should be 0
	 */
	result[1] = ordering_bp_test_two_ap(CASE_ID_CPUID, ORDER_TEST_CPUID_2, CACHE_PAT_WB);

	report("%s %ld %ld", ((result[0] != 0) && (result[1] == 0)), __FUNCTION__, result[0], result[1]);
}


/**
 * @brief case name: HSI_Multi-Processor_Management_Features_Serializing_Instructions_mfence_001
 *
 * Summary: MFENCE instructions force the processor to complete
 * all modifications to memory by previous instructions
 * and to drain all buffered writes to memory before the next instruction is fetched and executed
 *
 */
__unused static void hsi_rqmid_39843_multi_proc_mgmt_features_serializing_instructions_mfence_003(void)
{
	unsigned long result[2];

	/**
	 * not used mfence
	 * we can catch at least 1 time that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should not be 0
	 */
	result[0] = ordering_bp_test_two_ap(CASE_ID_MFENCE, ORDER_TEST_NO_MFENCE_1, CACHE_PAT_WB);

	/**
	 * used mfence
	 * we can not catch the case that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times(r1 = 0 and r2 = 0), so it should be 0
	 */
	result[1] = ordering_bp_test_two_ap(CASE_ID_MFENCE, ORDER_TEST_MFENCE_2, CACHE_PAT_WB);

	report("%s %ld %ld", ((result[0] != 0) && (result[1] == 0)), __FUNCTION__, result[0], result[1]);
}

/**
 * @brief case name: HSI_Multi-Processor_Management_Features_Memory_Ordering_Mfence_001
 *
 * Summary: MFENCE instructions is normal by producing a memory re-ordering and prevent it on SMP.
 */
__unused static void hsi_rqmid_40151_multi_proc_mgmt_features_memory_ordering_mfence_001(void)
{
	unsigned long result;

	/**
	 * used mfence, cache WB type, memory re-order
	 * we can not catch the case that r1 and r2 are 0
	 * at the same time within 1000,000 times loop,
	 * result means reorder times, so it should be 0
	 */
	result = ordering_bp_test_two_ap(CASE_ID_MEMORY_ORDER, ORDER_TEST_MEMORY_ORDER, CACHE_PAT_WB);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

void order_ap(int local_id)
{
	while (order_sync != ORDER_TEST_MAX) {
		ordering_ap_test_entry[local_id]();
	}
}

static st_case_suit_multi case_suit_multi[] = {
	{
		.case_fun = hsi_rqmid_35955_multi_proc_mgmt_features_multi_processor_initialization_001,
		.case_id = 35955,
		.case_name = "HSI_Multi-Processor_Management_Features_Multi_Processor_Initialization_001",
	},

	{
		.case_fun = hsi_rqmid_35972_multi_proc_mgmt_features_locked_atomic_operations_001,
		.case_id = 35972,
		.case_name = "HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_001",
	},

	{
		.case_fun = hsi_rqmid_35973_multi_proc_mgmt_features_locked_atomic_operations_002,
		.case_id = 35973,
		.case_name = "HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_002",
	},

	{
		.case_fun = hsi_rqmid_39720_multi_proc_mgmt_features_locked_atomic_operations_003,
		.case_id = 39720,
		.case_name = "HSI_Multi-Processor_Management_Features_Locked_Atomic_Operations_003",
	},

	{
		.case_fun = hsi_rqmid_35975_multi_proc_mgmt_features_idle_and_blocked_conditions_001,
		.case_id = 35975,
		.case_name = "HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_001",
	},

	{
		.case_fun = hsi_rqmid_35976_multi_proc_mgmt_features_idle_and_blocked_conditions_002,
		.case_id = 35976,
		.case_name = "HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_002",
	},

	{
		.case_fun = hsi_rqmid_35977_multi_proc_mgmt_features_idle_and_blocked_conditions_003,
		.case_id = 35977,
		.case_name = "HSI_Multi-Processor_Management_Features_Idle_and_Blocked_Conditions_003",
	},

	{
		.case_fun = hsi_rqmid_39838_multi_proc_mgmt_features_serializing_instructions_mov_001,
		.case_id = 39838,
		.case_name = "HSI_Multi-Processor_Management_Features_Serializing_Instructions_mov_001",
	},

	{
		.case_fun = hsi_rqmid_39839_multi_proc_mgmt_features_serializing_instructions_cpuid_002,
		.case_id = 39839,
		.case_name = "HSI_Multi-Processor_Management_Features_Serializing_Instructions_cpuid_001",
	},

	{
		.case_fun = hsi_rqmid_39843_multi_proc_mgmt_features_serializing_instructions_mfence_003,
		.case_id = 39843,
		.case_name = "HSI_Multi-Processor_Management_Features_Serializing_Instructions_mfence_001",
	},

	{
		.case_fun = hsi_rqmid_40151_multi_proc_mgmt_features_memory_ordering_mfence_001,
		.case_id = 40151,
		.case_name = "HSI_Multi-Processor_Management_Features_Memory_Ordering_Mfence_001",
	},

};

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI test case list: \n\r");
#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_multi); i++) {
		printf("\t Case ID: %d Case name: %s\n\r", case_suit_multi[i].case_id, case_suit_multi[i].case_name);
	}
#else
#endif
	printf("\t \n\r \n\r");
}
int ap_main(void)
{
#ifdef IN_NATIVE
	/***********************************/
	debug_print("ap %d starts running \n", get_lapic_id());
	while (start_run_id == 0) {
		test_delay(5);
	}
	if (start_run_id == 35955) {
		atomic_inc(&ap_run_nums);
	}

	/* ap test add ins */
	while (start_run_id != 35972) {
		test_delay(5);
	}
	if (get_lapic_id() == 4 || get_lapic_id() == 2) {
		debug_print("ap start to run case %d\n", start_run_id);
		lock_atomic_test_ap(INSTRUCTION_ADD);
	}

	/* ap test xchg ins */
	while (start_run_id != 35973) {
		test_delay(5);
	}
	if (get_lapic_id() == 4 || get_lapic_id() == 2) {
		debug_print("ap start to run case %d addr：%d\n", start_run_id, *test_addr1);
		lock_atomic_test_ap(INSTRUCTION_XCHG);
	}

	/* ap test mov ins */
	while (start_run_id != 39720) {
		test_delay(10);
	}
	if (get_lapic_id() == 4 || get_lapic_id() == 2) {
		debug_print("ap start to run case %d\n", start_run_id);
		lock_atomic_test_ap(INSTRUCTION_MOV);
	}

	/* ap test mwait/monitor ins */
	while (start_run_id != 35977) {
		test_delay(5);
	}
	if (get_lapic_id() == 2) {
		debug_print("ap start to run case %d\n", start_run_id);
		mwait_test_ap(INSTRUCTION_MWAIT);
	}

	int local_id = get_lapic_id();
	int max_local_id = CFG_TEST_ORDERING_CPU_NR_NATIVE - 1;

	local_id /= 2;
	if (local_id >= max_local_id) {
		debug_print("<HALT *AP* > un-used processor id %d\n", local_id);
		return 0;
	} else {
		debug_print("<Enter *AP* > processor id %d\n", local_id);
	}
	while (1) {
		order_ap(local_id);
	}

#endif
	return 0;
}

int main(void)
{
	setup_idt();
	asm volatile("fninit");

	print_case_list();
	atomic_set(&ap_run_nums, 0);

#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_multi); i++) {
		case_suit_multi[i].case_fun();
	}
#else
#endif
	start_run_id = 0;
	return report_summary();
}


