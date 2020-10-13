/*
 * Copyright (C) 2020 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2019/12/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 * Modify Author:       hexiagx.li@intel.com
 * Date :                       2020/10/13
 *
 */
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

#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif

#define TARGET_AP                   2
#define USING_APS                   2
#define LOOP_COUNT                  100000
#define LOOP_COUNT_ADD				1000
#define INSTRUCTION_ADD                         0
#define INSTRUCTION_XCHG                        1
#define INSTRUCTION_MOV							2
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
//static atomic_t ap_chk;

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

/*
 * @brief case name: HSI_Generic_Processor_Features_Locked_Atomic_Operations_001
 *
 * Summary: Test with multi-core - AP1 and AP2 to execute ADD instruction for many
 * times to add values of a global variable (val1) with 1 concurrently for 1000 times;
 * BP to control the synchronization of 2 APs. The result of val1 should equal to 2000
 */
static __unused void hsi_rqmid_35972_generic_processor_features_locked_atomic_operations_001()
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
 * @brief case name: HSI_Generic_Processor_Features_Locked_Atomic_Operations_002
 *
 * Summary: Under 64 bit mode on native board, 2 APs execute XCHG instruction
 * concurrently, the execution result should show atomicity,
 */
static __unused void hsi_rqmid_35973_generic_processor_features_locked_atomic_operations_002()
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
 * @brief case name: HSI_Generic_Processor_Features_Multi_Processor_Initialization_001
 *
 * Summary: On native board, startup all APs should be successfully.
 */
static __unused void hsi_rqmid_35955_generic_processor_features_multi_processor_initialization_001()
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
 * @brief case name: hsi_rqmid_39720_generic_processor_features_locked_atomic_operations_003
 *
 * Summary: Under 64 bit mode on native board, 2 APs to both read and write a double word memory aligned on 32-bit
 * via MOV instruction for many times concurrently; BP to control the synchronization of 2 APs.
 * Read value stored in the memory address after each write operation to verify whether the write is successful.
 */
static __unused void hsi_rqmid_39720_generic_processor_features_locked_atomic_operations_003()
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

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI test case list: \n\r");
#ifdef IN_NATIVE

	printf("\t Case ID: %d Case name: %s\n\r", 35955u, "HSI generic processor features " \
		"multi processor initialization 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35972u, "HSI generic processor features " \
		"locked atomic operations 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35973u, "HSI generic processor features " \
		"locked atomic operations 002");
	printf("\t Case ID: %d Case name: %s\n\r", 39720u, "HSI generic processor features " \
		"locked atomic operations 003");
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
	hsi_rqmid_35955_generic_processor_features_multi_processor_initialization_001();
	hsi_rqmid_35972_generic_processor_features_locked_atomic_operations_001();
	hsi_rqmid_35973_generic_processor_features_locked_atomic_operations_002();
	hsi_rqmid_39720_generic_processor_features_locked_atomic_operations_003();
#else
#endif
	start_run_id = 0;
	return report_summary();
}


