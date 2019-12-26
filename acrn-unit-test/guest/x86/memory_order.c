/*
 * Test for x86 cache and memory instructions
 *
 * Copyright (c) 2019 Intel
 *
 * Authors:
 *  Yi Sun <yi.sun@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#include "libcflat.h"
#include "desc.h"
#include "processor.h"

#include "asm/spinlock.h"
#include "asm/barrier.h"
#include "atomic.h"
#include "apic.h"

#define CFG_TEST_MEMORY_ORDERING_CPU_START	0
#define CFG_TEST_MEMORY_ORDERING_READY		1
#define CFG_MEMORY_ORDER_AP_START_ID		1
#define CFG_TEST_MEMORY_ALL_CPU_READY		2

#define CFG_MEMORY_ORDER_DEBUG_TIMES	10000000
#define CFG_TEST_MEMORY_ORDERING_TIMES	100000000UL
#define CFG_TEST_MEMORY_ORDERING_MFENCE 1
// #undef CFG_TEST_MEMORY_ORDERING_MFENCE
#define CFG_TEST_MEMORY_ORDERING_CPU_NR 3
#ifndef nop
#define nop()		do { asm volatile ("nop\n\t" :::"memory"); } while(0)
#endif
//#define test_debug(fmt...) printf("[%s:%s] line=%d "fmt"",__FILE__, __func__, __LINE__,  ##args)
#define test_debug(fmt, args...) 	printf("[%s:%s] line=%d "fmt"",__FILE__, __func__, __LINE__,  ##args)

void memory_ordering_test1_entry(void);
void memory_ordering_test2_entry(void);
void (*memory_ordering_rqmid_24597_test_entry[CFG_TEST_MEMORY_ORDERING_CPU_NR])(void) = {
	NULL,
	memory_ordering_test1_entry,
	memory_ordering_test2_entry,
};

atomic_t memory_ordering_begin_sem1;
atomic_t memory_ordering_begin_sem2;
atomic_t memory_ordering_end_sem;

static int logical_processor_arbitration(void)
{
	int id;

	/* share with cpus_booted; AP from 1, 0 is BP */
	static int s_booting_id = CFG_MEMORY_ORDER_AP_START_ID;
	static struct spinlock cpu_id_lock = {0};

	spin_lock(&cpu_id_lock);
	id = s_booting_id;
	++s_booting_id;

	test_debug("arbitration id: %d\n", id);
	spin_unlock(&cpu_id_lock);
	return id;
}

int X, Y;
int r1, r2;
// volatile long _x, _y;
// volatile long _rx, _ry;

void asm_test1(void)
{
	asm volatile(
		"xor %0, %0\n\t"
		"movl $1, %1\n\t"
#ifdef CFG_TEST_MEMORY_ORDERING_MFENCE
		"mfence\n\t"
#endif
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}
void memory_ordering_test1_entry(void)
{
	while(atomic_read(&memory_ordering_begin_sem1) !=
		CFG_TEST_MEMORY_ORDERING_READY)
		nop();
	atomic_dec(&memory_ordering_begin_sem1);
	asm_test1();
	atomic_inc(&memory_ordering_end_sem);
}

void asm_test2(void)
{
	asm volatile(
			"xor %0, %0\n\t"
			"movl $1, %1\n\t"
#ifdef CFG_TEST_MEMORY_ORDERING_MFENCE
			"mfence\n\t"
#endif
			"movl %2, %0\n\t"
			: "=r"(r2), "=m" (Y)
			: "m"(X)
			: "memory");
}

void memory_ordering_test2_entry(void)
{
	while(atomic_read(&memory_ordering_begin_sem2) !=CFG_TEST_MEMORY_ORDERING_READY){
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem2);

	asm_test2();
	atomic_inc(&memory_ordering_end_sem);
}

/**
 * @brief case name ACRN hypervisor shall expose memory ordering instructions to any VM
 *
 * Summary: ACRN hypervisor shall expose memory ordering instructions to any VM,
 *  in compliance with Chapter 8.2.5, Vol. 3, SDM.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
void memory_ordering_rqmid_24597_memory_ordering_instructions_001(void)
{
	const char *msg = __func__;
	unsigned long i;
	int detected = 0;

	atomic_set(&memory_ordering_begin_sem1, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem2, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

	for (i = 1; i < CFG_TEST_MEMORY_ORDERING_TIMES; ++i) {
		X = Y = 0;
		r1 = r2 = 1;

		atomic_inc(&memory_ordering_begin_sem1);
		atomic_inc(&memory_ordering_begin_sem2);

		while(atomic_read(&memory_ordering_end_sem) != CFG_TEST_MEMORY_ALL_CPU_READY) {
			nop();
		}

		atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

		if (r1 == 0 && r2 == 0) {
			detected++;
			test_debug("%d reorders detected after %ld iterations\n", detected, i);
			break;
		}

		if((i % CFG_MEMORY_ORDER_DEBUG_TIMES) == 0) {
			printf("%s: times %lu\n", msg, i);
		}
	}

	report("%s", detected == 0, msg);
}

void ap_main(void)
{
	int local_id = logical_processor_arbitration();

	if (local_id >= CFG_TEST_MEMORY_ORDERING_CPU_NR) {
		test_debug("<HALT *AP* > un-used processor id %d\n", local_id);
		return;
	}
	else {
		test_debug("<Enter *AP* > processor id %d\n", local_id);
	}

	while(1) {
		memory_ordering_rqmid_24597_test_entry[local_id]();
	}
}

int main(int ac, char **av)
{
	test_debug("\n");

	setup_idt();
	memory_ordering_rqmid_24597_memory_ordering_instructions_001();

	return report_summary();
}
