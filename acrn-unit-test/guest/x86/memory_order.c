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
#include "misc.h"
#include "asm/spinlock.h"
#include "asm/barrier.h"
#include "atomic.h"
#include "apic.h"
#include "memory_type.h"
#include "debug_print.h"

#define CFG_TEST_MEMORY_ORDERING_CPU_START	0
#define CFG_TEST_MEMORY_ORDERING_READY		1
#define CFG_MEMORY_ORDER_AP_START_ID		1
#define CFG_TEST_MEMORY_ALL_CPU_READY		2
#define CFG_TEST_MEMORY_ORDERING_CPU_NR 3

#define CFG_TEST_MEMORY_ALL_CPU_READY_NATIVE	3
#define CFG_TEST_MEMORY_ORDERING_CPU_NR_NATIVE	4

#define CFG_MEMORY_ORDER_DEBUG_TIMES	10000000
#define CFG_TEST_MEMORY_ORDERING_TIMES	100000000UL

enum memory_order_type {
	/* mfence (notmemory re-ordering occur) */
	MEMORY_ORDER_TEST_144415_TABLE_1 = 0,

	/* no mfence(memory re-ordering occur) */
	MEMORY_ORDER_TEST_144415_TABLE_2,

	/* no mfence(writes are not reordered with older reads) */
	MEMORY_ORDER_TEST_144414_TABLE_2,

	/* no mfence(writes to memory are not reordered with other writes.) */
	MEMORY_ORDER_TEST_144414_TABLE_3,

	/* No write to memory may be reordered with an execution of the CLFLUSH instruction. */
	MEMORY_ORDER_TEST_144414_TABLE_4,

	/* A write may be reordered with an execution of the CLFLUSHOPT instruction. */
	MEMORY_ORDER_TEST_144414_TABLE_5,

	/* Reads may be reordered with older writes to different locations */
	MEMORY_ORDER_TEST_144414_TABLE_6,

	/* Reads are not reordered with older writes to the same location. */
	MEMORY_ORDER_TEST_144414_TABLE_7,

	/* Reads Are not Reordered with Locks */
	MEMORY_ORDER_TEST_144414_TABLE_8_READS,

	/* Stores Are not Reordered with Locks */
	MEMORY_ORDER_TEST_144414_TABLE_8_STORES,

	/* Reads cannot pass earlier MFENCE instruction */
	MEMORY_ORDER_TEST_144414_TABLE_9,

	/* Writes by a single processor are observed in the same order by all processors. */
	MEMORY_ORDER_TEST_144414_TABLE_10,

	/* Memory ordering obeys causality (memory ordering respects transitive visibility) */
	MEMORY_ORDER_TEST_144414_TABLE_11,

	/* Any two stores are seen in a consistent order by processors other than those performing the stores. */
	MEMORY_ORDER_TEST_144414_TABLE_12,

	/* Locked instructions have a total order. */
	MEMORY_ORDER_TEST_144414_TABLE_13,

	/* Store-buffer forwarding, when a read passes a write to the same memory location. */
	MEMORY_ORDER_TEST_144414_TABLE_14,

	/* Out of order store from long string store and string move operations. */
	MEMORY_ORDER_TEST_144414_TABLE_15,

	/* memory_type : UC
	 * Reads and writes always appear in programmed order.
	 * Reads will not be reordered with older writes to different locations.
	 */
	MEMORY_ORDER_TEST_144412_TABLE_1,

	/* memory_type : UC
	 * Writes are not reordered with older reads.
	 */
	MEMORY_ORDER_TEST_144412_TABLE_2,

	/* MAX TYPE */
	MEMORY_ORDER_TEST_MAX,
};

static volatile int start_run_id = 0;

static atomic_t memory_ordering_begin_sem1;
static atomic_t memory_ordering_begin_sem2;
static atomic_t memory_ordering_end_sem;
static atomic_t memory_ordering_begin_sem3;
static atomic_t memory_ordering_begin_sem_ap;
static volatile int memory_order_sync = MEMORY_ORDER_TEST_MAX;

static int X;
static int Y;
static int Z;
static int r1;
static int r2;
static int r3;
static int r4;
static int r5;
static int r6;

static void asm_test_null(void)
{
}

static void asm_test_144415_table1_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mfence\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}

static void asm_test_144415_table1_mfence_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"mfence\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X)
		: "memory");
}

static void asm_test_144415_table2_no_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}

static void asm_test_144415_table2_no_mfence_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X)
		: "memory");
}

static int memory_order_144415_table12_check()
{
	int detected = 0;
	if ((r1 == 0) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144415_table12_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table2_no_mfence_ap1(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=r"(r1), "=m" (Y)
		: "m"(X)
		: "memory");
}

static void asm_test_144414_table2_no_mfence_ap2(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=r"(r2), "=m" (X)
		: "m"(Y)
		: "memory");
}

static int memory_order_144414_table2_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 1)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table23456_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table3_no_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl $1, %0\n\t"
		: "=m"(Y), "=m" (X)
		:: "memory");
}

static void asm_test_144414_table3_no_mfence_ap2(void)
{
	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		: "=r"(r1), "=r" (r2)
		: "m"(X), "m"(Y)
		: "memory");
}

static int memory_order_144414_table3_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table4_clflush_ap1(void)
{
	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		"movl $1, %2\n\t"
		"movl $1, %3\n\t"
		"clflush (%4)\n\t"
		: "=r"(r1), "=r"(r2), "=m"(X), "=m"(Y)
		: "r"(&r1)
		: "memory");

	atomic_inc(&memory_ordering_begin_sem_ap);
}

static void asm_test_144414_table4_clflush_ap2(void)
{
	while (atomic_read(&memory_ordering_begin_sem_ap) !=
		CFG_TEST_MEMORY_ORDERING_READY) {
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem_ap);

	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		: "=r"(r1), "=r" (r2)
		: "m"(X), "m"(Y)
		: "memory");
}

static int memory_order_144414_table4_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table5_clflushopt_ap1(void)
{
	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		"movl $1, %2\n\t"
		"movl $1, %3\n\t"
		"clflushopt (%4)\n\t"
		: "=r"(r1), "=r"(r2), "=m"(X), "=m"(Y)
		: "r"(&r1)
		: "memory");

	atomic_inc(&memory_ordering_begin_sem_ap);
}

static void asm_test_144414_table5_clflushopt_ap2(void)
{
	while (atomic_read(&memory_ordering_begin_sem_ap) !=
		CFG_TEST_MEMORY_ORDERING_READY) {
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem_ap);

	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		: "=r"(r1), "=r" (r2)
		: "m"(X), "m"(Y)
		: "memory");
}

static int memory_order_144414_table5_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table6_no_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r1), "=m" (X)
		: "m"(Y)
		: "memory");
}

static void asm_test_144414_table6_no_mfence_ap2(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %2, %0\n\t"
		: "=r"(r2), "=m" (Y)
		: "m"(X)
		: "memory");
}

static int memory_order_144414_table6_check()
{
	int detected = 0;
	if ((r1 == 0) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table7_no_mfence_ap1(void)
{
	asm volatile(
		"movl $1, %1\n\t"
		"movl %1, %0\n\t"
		: "=r"(r1), "=m" (X)
		:: "memory");
}

static int memory_order_144414_table7_check()
{
	int detected = 0;
	if (r1 == 0) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table7_init()
{
	X = 0;
}

static void asm_test_144414_table8_read_ap1(void)
{
	asm volatile(
		"xchg %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=m"(X), "=r" (r2), "=r"(r1)
		: "m"(Y)
		: "memory");
}

static void asm_test_144414_table8_read_ap2(void)
{
	asm volatile(
		"xchg %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=m"(Y), "=r" (r4), "=r"(r3)
		: "m"(X)
		: "memory");
}

static int memory_order_144414_table8_read_check()
{
	int detected = 0;
	if ((r2 == 0) && (r4 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table8_read_init()
{
	X = 0;
	Y = 0;
	r1 = 1;
	r3 = 1;
}

static void asm_test_144414_table8_stores_ap1(void)
{
	asm volatile(
		"xchg %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=m"(X), "=m" (Y), "=r"(r1)
		:: "memory");
}

static void asm_test_144414_table8_stores_ap2(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r2), "=r" (r3)
		: "m"(Y), "m"(X)
		: "memory");
}

static int memory_order_144414_table8_stores_check()
{
	int detected = 0;
	if ((r2 == 1) && (r3 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table8_stores_init()
{
	X = 0;
	Y = 0;
	r1 = 1;
}

static void asm_test_144414_table10_bp(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		"movl $1, %1\n\t"
		: "=m"(X), "=m" (Y)
		:: "memory");
}

static void asm_test_144414_table10_ap1(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r1), "=r" (r2)
		: "m"(X), "m"(Y)
		: "memory");
}

static void asm_test_144414_table10_ap2(void)
{
	asm volatile(
		"movl %3, %0\n\t"
		"movl %2, %1\n\t"
		: "=r"(r3), "=r" (r4)
		: "m"(X), "m"(Y)
		: "memory");
}

static int memory_order_144414_table10_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0) && (r3 == 1) && (r4 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table10_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table11_bp(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r2), "=r" (r3)
		: "m"(Y), "m"(X)
		: "memory");
}

static void asm_test_144414_table11_ap1(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		: "=m"(X)
		:: "memory");
}

static void asm_test_144414_table11_ap2(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=r"(r1), "=m" (Y)
		: "m"(X)
		: "memory");
}

static int memory_order_144414_table11_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 1) && (r3 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table11_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table12_bp(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		: "=m"(X)
		:: "memory");
}

static void asm_test_144414_table12_ap1(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		: "=m"(Y)
		:: "memory");
}

static void asm_test_144414_table12_ap2(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r1), "=r" (r2)
		: "m"(X), "m"(Y)
		: "memory");
}

static void asm_test_144414_table12_ap3(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r3), "=r" (r4)
		: "m"(Y), "m"(X)
		: "memory");
}

static int memory_order_144414_table12_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0) && (r3 == 1) && (r4 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table12_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table13_ap1(void)
{
	asm volatile(
		"xchg %1, %0\n\t"
		: "=m"(X), "=r"(r1)
		:: "memory");

	atomic_inc(&memory_ordering_begin_sem_ap);
}

static void asm_test_144414_table13_ap2(void)
{
	asm volatile(
		"xchg %1, %0\n\t"
		: "=m"(Y), "=r"(r2)
		:: "memory");

	atomic_inc(&memory_ordering_begin_sem_ap);
}

static void asm_test_144414_table13_ap3(void)
{
	/* wait ap1 ap2 done */
	while (atomic_read(&memory_ordering_begin_sem_ap) != 2) {
		nop();
	}
	atomic_set(&memory_ordering_begin_sem_ap, CFG_TEST_MEMORY_ORDERING_CPU_START);

	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r3), "=r" (r4)
		: "m"(X), "m"(Y)
		: "memory");
}

static void asm_test_144414_table13_bp(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r"(r5), "=r" (r6)
		: "m"(Y), "m"(X)
		: "memory");
}

static int memory_order_144414_table13_check()
{
	int detected = 0;
	if ((r3 == 1) && (r4 == 0) && (r5 == 1) && (r6 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table13_init()
{
	X = 0;
	Y = 0;
	r1 = 1;
	r2 = 1;
}

static void asm_test_144414_table14_ap1(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		"movl %0, %1\n\t"
		"movl %3, %2\n\t"
		: "=m"(X), "=r" (r1), "=r"(r2)
		: "m"(Y)
		: "memory");
}

static void asm_test_144414_table14_ap2(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		"movl %0, %1\n\t"
		"movl %3, %2\n\t"
		: "=m"(Y), "=r" (r3), "=r"(r4)
		: "m"(X)
		: "memory");
}

static int memory_order_144414_table14_check()
{
	int detected = 0;
	if ((r2 == 0) && (r4 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table14_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144414_table15_ap1(void)
{
	asm volatile(
		"mov $1, %%eax\n\t"
		"mov $0x80, %%ecx\n\t"
		"mov %0, %%edi\n\t"		/*ES:EDI =X*/
		".byte 0xF3, 0xAB\n\t"	/*rep:stosd*/
		:: "m"(X)
		: "memory");
}

static void asm_test_144414_table15_ap2(void)
{
	int *_z = (int *)(unsigned long)Z;
	int *_y = (int *)(unsigned long)Y;
	asm volatile(
		"movl %2, %0\n\t"
		"movl %3, %1\n\t"
		: "=r" (r1), "=r"(r2)
		: "m"(*_z), "m"(*_y)
		: "memory");
}

static int memory_order_144414_table15_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144414_table15_init()
{
	unsigned char *point;

	/* _x <= _y < _z < _x+512 */
	X = 0x6000;
	Y = X + 0x50;
	Z = Y + 0x50;

	point = (unsigned char *)0x6000;
	memset(point, 0x0, 512);
}

static void asm_test_144412_table1_ap1(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		"movl %2, %1\n\t"
		: "=m"(X), "=r"(r1)
		: "m"(Y)
		: "memory");
}

static void asm_test_144412_table1_ap2(void)
{
	asm volatile(
		"movl $1, %0\n\t"
		"movl %2, %1\n\t"
		: "=m"(Y), "=r"(r2)
		: "m"(X)
		: "memory");
}

static int memory_order_144412_table1_check()
{
	int detected = 0;
	if ((r1 == 0) && (r2 == 0)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144412_table1_init()
{
	X = 0;
	Y = 0;
}

static void asm_test_144412_table2_ap1(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=r"(r1), "=m"(Y)
		: "m"(X)
		: "memory");
}

static void asm_test_144412_table2_ap2(void)
{
	asm volatile(
		"movl %2, %0\n\t"
		"movl $1, %1\n\t"
		: "=r"(r2), "=m"(X)
		: "m"(Y)
		: "memory");
}

static int memory_order_144412_table2_check()
{
	int detected = 0;
	if ((r1 == 1) && (r2 == 1)) {
		detected = 1;
	}
	return detected;
}

static void asm_test_144412_table2_init()
{
	X = 0;
	Y = 0;
}

static void (*memory_ordering_test_type[MEMORY_ORDER_TEST_MAX+1][CFG_TEST_MEMORY_ORDERING_CPU_NR_NATIVE])(void) = {
	/* MEMORY_ORDER_TEST_144415_TABLE_1 */
	{
		NULL,
		asm_test_144415_table2_no_mfence_ap1,
		asm_test_144415_table2_no_mfence_ap2,
		NULL,
	},


	/* MEMORY_ORDER_TEST_144415_TABLE_2 */
	{
		NULL,
		asm_test_144415_table1_mfence_ap1,
		asm_test_144415_table1_mfence_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_2 */
	{
		NULL,
		asm_test_144414_table2_no_mfence_ap1,
		asm_test_144414_table2_no_mfence_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_3 */
	{
		NULL,
		asm_test_144414_table3_no_mfence_ap1,
		asm_test_144414_table3_no_mfence_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_4 */
	{
		NULL,
		asm_test_144414_table4_clflush_ap1,
		asm_test_144414_table4_clflush_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_5 */
	{
		NULL,
		asm_test_144414_table5_clflushopt_ap1,
		asm_test_144414_table5_clflushopt_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_6 */
	{
		NULL,
		asm_test_144414_table6_no_mfence_ap1,
		asm_test_144414_table6_no_mfence_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_7 */
	{
		NULL,
		asm_test_144414_table7_no_mfence_ap1,
		NULL,	// only ap1 run
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_8_READS */
	{
		NULL,
		asm_test_144414_table8_read_ap1,
		asm_test_144414_table8_read_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_8_STORES */
	{
		NULL,
		asm_test_144414_table8_stores_ap1,
		asm_test_144414_table8_stores_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_9 */
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_10 */
	{
		asm_test_144414_table10_bp,
		asm_test_144414_table10_ap1,
		asm_test_144414_table10_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_11 */
	{
		asm_test_144414_table11_bp,
		asm_test_144414_table11_ap1,
		asm_test_144414_table11_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_12 */
	{
		asm_test_144414_table12_bp,
		asm_test_144414_table12_ap1,
		asm_test_144414_table12_ap2,
		asm_test_144414_table12_ap3,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_13 */
	{
		asm_test_144414_table13_bp,
		asm_test_144414_table13_ap1,
		asm_test_144414_table13_ap2,
		asm_test_144414_table13_ap3,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_14 */
	{
		NULL,
		asm_test_144414_table14_ap1,
		asm_test_144414_table14_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144414_TABLE_15 */
	{
		NULL,
		asm_test_144414_table15_ap1,
		asm_test_144414_table15_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144412_TABLE_1 */
	{
		NULL,
		asm_test_144412_table1_ap1,
		asm_test_144412_table1_ap2,
		NULL,
	},

	/* MEMORY_ORDER_TEST_144412_TABLE_2 */
	{
		NULL,
		asm_test_144412_table2_ap1,
		asm_test_144412_table2_ap2,
		NULL,
	},
	/* MEMORY_ORDER_TEST_MAX */
	{
		asm_test_null,
		asm_test_null,
		asm_test_null,
		asm_test_null,
	},
};


static void (*memory_ordering_init[MEMORY_ORDER_TEST_MAX+1])(void) = {
	/* MEMORY_ORDER_TEST_144415_TABLE_1 */
	asm_test_144415_table12_init,

	/* MEMORY_ORDER_TEST_144415_TABLE_2 */
	asm_test_144415_table12_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_2 */
	asm_test_144414_table23456_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_3 */
	asm_test_144414_table23456_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_4 */
	asm_test_144414_table23456_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_5 */
	asm_test_144414_table23456_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_6 */
	asm_test_144414_table23456_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_7 */
	asm_test_144414_table7_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_8_READS */
	asm_test_144414_table8_read_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_8_STORES */
	asm_test_144414_table8_stores_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_9 */
	NULL,

	/* MEMORY_ORDER_TEST_144414_TABLE_10 */
	asm_test_144414_table10_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_11 */
	asm_test_144414_table11_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_12 */
	asm_test_144414_table12_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_13 */
	asm_test_144414_table13_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_14 */
	asm_test_144414_table14_init,

	/* MEMORY_ORDER_TEST_144414_TABLE_15 */
	asm_test_144414_table15_init,

	/* MEMORY_ORDER_TEST_144412_TABLE_1 */
	asm_test_144412_table1_init,

	/* MEMORY_ORDER_TEST_144412_TABLE_2 */
	asm_test_144412_table2_init,
};

static void memory_ordering_mfence_test1(void)
{
	while (atomic_read(&memory_ordering_begin_sem1) !=
		CFG_TEST_MEMORY_ORDERING_READY) {
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem1);
	memory_ordering_test_type[memory_order_sync][1]();
	atomic_inc(&memory_ordering_end_sem);
}

static void memory_ordering_mfence_test2(void)
{
	while (atomic_read(&memory_ordering_begin_sem2) !=
		CFG_TEST_MEMORY_ORDERING_READY) {
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem2);
	memory_ordering_test_type[memory_order_sync][2]();
	atomic_inc(&memory_ordering_end_sem);
}

static void memory_ordering_mfence_test3(void)
{
	while (atomic_read(&memory_ordering_begin_sem3) !=
		CFG_TEST_MEMORY_ORDERING_READY) {
		nop();
	}
	atomic_dec(&memory_ordering_begin_sem3);
	memory_ordering_test_type[memory_order_sync][3]();
	atomic_inc(&memory_ordering_end_sem);
}


static void (*memory_ordering_ap_test_entry[CFG_TEST_MEMORY_ORDERING_CPU_NR_NATIVE])(void) = {
	NULL,							//BP entry
	memory_ordering_mfence_test1,	//AP1 entry
	memory_ordering_mfence_test2,	//AP2 entry
	memory_ordering_mfence_test3,	//AP3 entry only native used
};

static int (*memory_ordering_bp_check[MEMORY_ORDER_TEST_MAX+1])() = {
	memory_order_144415_table12_check,		//MEMORY_ORDER_TEST_144415_TABLE_1
	memory_order_144415_table12_check,		//MEMORY_ORDER_TEST_144415_TABLE_2
	memory_order_144414_table2_check,		//MEMORY_ORDER_TEST_144414_TABLE_2
	memory_order_144414_table3_check,		//MEMORY_ORDER_TEST_144414_TABLE_3
	memory_order_144414_table4_check,		//MEMORY_ORDER_TEST_144414_TABLE_4
	memory_order_144414_table5_check,		//MEMORY_ORDER_TEST_144414_TABLE_5
	memory_order_144414_table6_check,		//MEMORY_ORDER_TEST_144414_TABLE_6
	memory_order_144414_table7_check,		//MEMORY_ORDER_TEST_144414_TABLE_7
	memory_order_144414_table8_read_check,	//MEMORY_ORDER_TEST_144414_TABLE_8_READS
	memory_order_144414_table8_stores_check,//MEMORY_ORDER_TEST_144414_TABLE_8_STORES
	NULL,									//MEMORY_ORDER_TEST_144414_TABLE_9
	memory_order_144414_table10_check,		//MEMORY_ORDER_TEST_144414_TABLE_10
	memory_order_144414_table11_check,		//MEMORY_ORDER_TEST_144414_TABLE_11
	memory_order_144414_table12_check,		//MEMORY_ORDER_TEST_144414_TABLE_12
	memory_order_144414_table13_check,		//MEMORY_ORDER_TEST_144414_TABLE_13
	memory_order_144414_table14_check,		//MEMORY_ORDER_TEST_144414_TABLE_14
	memory_order_144414_table15_check,		//MEMORY_ORDER_TEST_144414_TABLE_15
	memory_order_144412_table1_check,		//MEMORY_ORDER_TEST_144412_TABLE_1
	memory_order_144412_table2_check,		//MEMORY_ORDER_TEST_144412_TABLE_2
};

static void memory_ordering_memory_type(int memory_type)
{
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 6(WB), 5(WP), 4(WT) 0(UC)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0xFF))|memory_type);
}

static unsigned long memory_ordering_bp_test_two_ap(int run_id, int test_id, int memory_type)
{
	unsigned long i;
	int detected = 0;

	start_run_id = run_id;
	memory_order_sync = test_id;

	atomic_set(&memory_ordering_begin_sem1, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem2, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem_ap, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

	/* set current test memory type */
	memory_ordering_memory_type(memory_type);

	for (i = 1; i < CFG_TEST_MEMORY_ORDERING_TIMES; ++i) {
		memory_ordering_init[memory_order_sync]();

		atomic_inc(&memory_ordering_begin_sem1);
		atomic_inc(&memory_ordering_begin_sem2);

		while (atomic_read(&memory_ordering_end_sem) != CFG_TEST_MEMORY_ALL_CPU_READY) {
			nop();
		}

		atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

		if (memory_ordering_bp_check[memory_order_sync]()) {
			detected++;
			if (detected <= 1) {
				printf("type=%d %d reorders detected after %ld iterations\n",
					memory_order_sync, detected, i);
			}
		}

		if ((i % CFG_MEMORY_ORDER_DEBUG_TIMES) == 0) {
			debug_print("%s: times %lu\n", __FUNCTION__, i);
		}
	}

	memory_order_sync = MEMORY_ORDER_TEST_MAX;
	return detected;
}

static unsigned long memory_ordering_bp_test_one_ap(int run_id, int test_id, int memory_type)
{
	unsigned long i;
	int detected = 0;

	start_run_id = run_id;
	memory_order_sync = test_id;

	atomic_set(&memory_ordering_begin_sem1, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

	/* set current test memory type */
	memory_ordering_memory_type(memory_type);

	for (i = 1; i < CFG_TEST_MEMORY_ORDERING_TIMES; ++i) {
		memory_ordering_init[memory_order_sync]();

		atomic_inc(&memory_ordering_begin_sem1);

		/* this test only 1 ap*/
		while (atomic_read(&memory_ordering_end_sem) != 1) {
			nop();
		}

		atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

		if (memory_ordering_bp_check[memory_order_sync]()) {
			detected++;
			if (detected <= 1) {
				printf("%d reorders detected after %ld iterations\n", detected, i);
			}
		}

		if ((i % CFG_MEMORY_ORDER_DEBUG_TIMES) == 0) {
			debug_print("%s: times %lu\n", __FUNCTION__, i);
		}
	}

	memory_order_sync = MEMORY_ORDER_TEST_MAX;
	return detected;
}

static unsigned long memory_ordering_bp_test_three_ap(int run_id, int test_id, int memory_type)
{
	unsigned long i;
	int detected = 0;

	start_run_id = run_id;
	memory_order_sync = test_id;

	atomic_set(&memory_ordering_begin_sem1, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem2, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem_ap, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

	/* set current test memory type */
	memory_ordering_memory_type(memory_type);

	for (i = 1; i < CFG_TEST_MEMORY_ORDERING_TIMES; ++i) {
		memory_ordering_init[memory_order_sync]();

		atomic_inc(&memory_ordering_begin_sem1);
		atomic_inc(&memory_ordering_begin_sem2);

		/* only table 10 */
		if ((memory_order_sync == MEMORY_ORDER_TEST_144414_TABLE_10)
			&& (memory_ordering_test_type[memory_order_sync][0] != NULL)) {
			memory_ordering_test_type[memory_order_sync][0]();
		}

		while (atomic_read(&memory_ordering_end_sem) != CFG_TEST_MEMORY_ALL_CPU_READY) {
			nop();
		}

		/* only table 11 */
		if ((memory_order_sync == MEMORY_ORDER_TEST_144414_TABLE_11)
			&& (memory_ordering_test_type[memory_order_sync][0] != NULL)) {
			memory_ordering_test_type[memory_order_sync][0]();
		}

		atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

		if (memory_ordering_bp_check[memory_order_sync]()) {
			detected++;
			if (detected <= 1) {
				printf("type=%d %d reorders detected after %ld iterations\n",
					memory_order_sync, detected, i);
			}
		}

		if ((i % CFG_MEMORY_ORDER_DEBUG_TIMES) == 0) {
			debug_print("%s: times %lu\n", __FUNCTION__, i);
		}
	}

	memory_order_sync = MEMORY_ORDER_TEST_MAX;
	return detected;
}

static unsigned long memory_ordering_bp_test_four_ap(int run_id, int test_id, int memory_type)
{
	unsigned long i;
	int detected = 0;

	start_run_id = run_id;
	memory_order_sync = test_id;

	atomic_set(&memory_ordering_begin_sem1, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem2, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem3, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_begin_sem_ap, CFG_TEST_MEMORY_ORDERING_CPU_START);
	atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

	/* set current test memory type */
	memory_ordering_memory_type(memory_type);

	for (i = 1; i < CFG_TEST_MEMORY_ORDERING_TIMES; ++i) {
		memory_ordering_init[memory_order_sync]();

		atomic_inc(&memory_ordering_begin_sem1);
		atomic_inc(&memory_ordering_begin_sem2);
		atomic_inc(&memory_ordering_begin_sem3);

		/* only table 12 test */
		if ((memory_order_sync == MEMORY_ORDER_TEST_144414_TABLE_12)
			&& (memory_ordering_test_type[memory_order_sync][0] != NULL)) {
			memory_ordering_test_type[memory_order_sync][0]();
		}

		while (atomic_read(&memory_ordering_end_sem) != CFG_TEST_MEMORY_ALL_CPU_READY_NATIVE) {
			nop();
		}

		/* only table 13 test, wait ap1 ap2 done */
		if ((memory_order_sync == MEMORY_ORDER_TEST_144414_TABLE_13)
			&& (memory_ordering_test_type[memory_order_sync][0] != NULL)) {
			memory_ordering_test_type[memory_order_sync][0]();
		}

		atomic_set(&memory_ordering_end_sem, CFG_TEST_MEMORY_ORDERING_CPU_START);

		if (memory_ordering_bp_check[memory_order_sync]()) {
			detected++;
			if (detected <= 1) {
				printf("type=%d %d reorders detected after %ld iterations\n",
					memory_order_sync, detected, i);
			}
		}

		if ((i % CFG_MEMORY_ORDER_DEBUG_TIMES) == 0) {
			debug_print("%s: times %lu\n", __FUNCTION__, i);
		}
	}

	memory_order_sync = MEMORY_ORDER_TEST_MAX;
	return detected;
}

/**
 * @brief case name: ACRN hypervisor shall expose memory ordering instructions to any VM
 *
 * Summary: ACRN hypervisor shall expose memory ordering instructions to any VM,
 * in compliance with Chapter 8.2.5, Vol. 3, SDM.
 *
 */
__unused static void memory_ordering_rqmid_24597_mfence_001(void)
{
	unsigned long result[2];

	/* memory type WB, not used MFENCE */
	result[0] = memory_ordering_bp_test_two_ap(24597, MEMORY_ORDER_TEST_144415_TABLE_1, CACHE_PAT_WB);

	/* memory type WB, used MFENCE */
	result[1] = memory_ordering_bp_test_two_ap(24597, MEMORY_ORDER_TEST_144415_TABLE_2, CACHE_PAT_WB);

	report("%s %ld %ld", ((result[0] != 0) && (result[1] == 0)), __FUNCTION__, result[0], result[1]);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_002
 *
 * Summary: At 64bit mode, set the memory type to WB, writes are not reordered with older reads.
 */
__unused static void memory_ordering_rqmid_39236_processor_ordering_002(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 1 is not allowed */
	result = memory_ordering_bp_test_two_ap(39236, MEMORY_ORDER_TEST_144414_TABLE_2, CACHE_PAT_WB);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_003
 *
 * Summary: At 64bit mode, set the memory type to WT, writes to memory are not reordered with other writes.
 */
__unused static void memory_ordering_rqmid_39265_processor_ordering_003(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 0 is not allowed */
	result = memory_ordering_bp_test_two_ap(39265, MEMORY_ORDER_TEST_144414_TABLE_3, CACHE_PAT_WT);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_004
 *
 * Summary: At 64bit mode, set the memory type to WP,
 * no write to memory may be reordered with an execution of the CLFLUSH instruction.
 */
__unused static void memory_ordering_rqmid_39266_processor_ordering_004(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 0 is not allowed */
	result = memory_ordering_bp_test_two_ap(39266, MEMORY_ORDER_TEST_144414_TABLE_4, CACHE_PAT_WP);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_005
 *
 * Summary: At 64bit mode, set the memory type to WB,
 * a write may be reordered with an execution of the CLFLUSHOPT instruction.
 */
__unused static void memory_ordering_rqmid_39298_processor_ordering_005(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 0 is allowed. */
	result = memory_ordering_bp_test_two_ap(39298, MEMORY_ORDER_TEST_144414_TABLE_5, CACHE_PAT_WB);

	report("%s %ld", (result != 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_006
 *
 * Summary: At 64bit mode, set the memory type to WB, reads may be reordered with older writes to different locations
 */
__unused static void memory_ordering_rqmid_39300_processor_ordering_006(void)
{
	unsigned long result;

	/* r1 = 0 and r2 = 0 is allowed */
	result = memory_ordering_bp_test_two_ap(39300, MEMORY_ORDER_TEST_144414_TABLE_6, CACHE_PAT_WB);

	report("%s %ld", (result != 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_007
 *
 * Summary: At 64bit mode, set the memory type to WP, reads are not reordered with older writes to the same location.
 */
__unused static void memory_ordering_rqmid_39302_processor_ordering_007(void)
{
	unsigned long result;

	/* r1 = 0 is not allowed */
	result = memory_ordering_bp_test_one_ap(39302, MEMORY_ORDER_TEST_144414_TABLE_7, CACHE_PAT_WP);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_008
 *
 * Summary: At 64bit mode, set the memory type to WB,
 * reads are not reordered with Locks, stores are not reordered with Locks.
 */
__unused static void memory_ordering_rqmid_39312_processor_ordering_008(void)
{
	unsigned long result[2];

	/* r2 = 0 and r4 = 0 is not allowed */
	result[0] = memory_ordering_bp_test_two_ap(39312, MEMORY_ORDER_TEST_144414_TABLE_8_READS, CACHE_PAT_WB);

	/* r2 = 1 and r3 = 0 is not allowed */
	result[1] = memory_ordering_bp_test_two_ap(39312, MEMORY_ORDER_TEST_144414_TABLE_8_STORES, CACHE_PAT_WB);

	report("%s %ld %ld", ((result[0] == 0) && (result[1] == 0)), __FUNCTION__, result[0], result[1]);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_010
 *
 * Summary: At 64bit mode, set the memory type to WT,
 * writes by a single processor are observed in the same order by all processors.
 */
__unused static void memory_ordering_rqmid_39313_processor_ordering_010(void)
{
	unsigned long result;

	/* r1 = 1, r2 = 0, r3 = 1, r4 = 0 is not allowed */
	result = memory_ordering_bp_test_three_ap(39313, MEMORY_ORDER_TEST_144414_TABLE_10, CACHE_PAT_WT);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_011
 *
 * Summary: At 64bit mode, set the memory type to WP,
 * memory ordering obeys causality (memory ordering respects transitive visibility).
 */
__unused static void memory_ordering_rqmid_39314_processor_ordering_011(void)
{
	unsigned long result;

	/* r1 = 1, r2 = 1, r3 = 0 is not allowed */
	result = memory_ordering_bp_test_three_ap(39314, MEMORY_ORDER_TEST_144414_TABLE_11, CACHE_PAT_WP);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_012
 *
 * Summary: At 64bit mode, set the memory type to WB, any two stores are seen in a consistent
 * order by processors other than those performing the stores.
 */
__unused static void memory_ordering_rqmid_39315_processor_ordering_012(void)
{
	unsigned long result;

	/* r1 = 1, r2 = 0, r3 = 1, r4 = 0 is not allowed */
	result = memory_ordering_bp_test_four_ap(39315, MEMORY_ORDER_TEST_144414_TABLE_12, CACHE_PAT_WB);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_013
 *
 * Summary: At 64bit mode, set the memory type to WT, locked instructions have a total order.
 */
__unused static void memory_ordering_rqmid_39316_processor_ordering_013(void)
{
	unsigned long result;

	/* r3 = 1, r4 = 0, r5 = 1, r6 = 0 is not allowed */
	result = memory_ordering_bp_test_four_ap(39316, MEMORY_ORDER_TEST_144414_TABLE_13, CACHE_PAT_WT);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_014
 *
 * Summary: At 64bit mode, set the memory type to WB, store-buffer forwarding,
 * when a read passes a write to the same memory location.
 */
__unused static void memory_ordering_rqmid_39317_processor_ordering_014(void)
{
	unsigned long result;

	/* r2 = 0 and r4 = 0 is allowed */
	result = memory_ordering_bp_test_two_ap(39317, MEMORY_ORDER_TEST_144414_TABLE_14, CACHE_PAT_WB);

	report("%s %ld", (result != 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to WT, WB or WP memory ranges follows processor ordering_015
 *
 * Summary: At 64bit mode, set the memory type to WB,
 * out of order store from long string store and string move operations.
 */
__unused static void memory_ordering_rqmid_39322_processor_ordering_015(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 0 is allowed */
	result = memory_ordering_bp_test_two_ap(39322, MEMORY_ORDER_TEST_144414_TABLE_15, CACHE_PAT_WB);

	report("%s %ld", (result != 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to UC or UC- memory ranges follows strong ordering_001
 *
 * Summary: At 64bit mode, set the memory type to UC.
 * Reads and writes always appear in programmed order.
 * Reads will not be reordered with older writes to different locations.
 */
__unused static void memory_ordering_rqmid_39348_uc_ordering_001(void)
{
	unsigned long result;

	/* r1 = 0 and r2 = 0 is not allowed. */
	result = memory_ordering_bp_test_two_ap(39348, MEMORY_ORDER_TEST_144412_TABLE_1, CACHE_PAT_UC);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

/**
 * @brief case name: Accesses to UC or UC- memory ranges follows strong ordering_002
 *
 * Summary: At 64bit mode, set the memory type to UC.
 * Writes are not reordered with older reads.
 */
__unused static void memory_ordering_rqmid_39349_uc_ordering_002(void)
{
	unsigned long result;

	/* r1 = 1 and r2 = 1 is not allowed. */
	result = memory_ordering_bp_test_two_ap(39349, MEMORY_ORDER_TEST_144412_TABLE_2, CACHE_PAT_UC);

	report("%s %ld", (result == 0), __FUNCTION__, result);
}

static void print_case_list()
{
	printf("Memory ordering feature case list:\n\r");
#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	printf("\t\t Case ID:%d case name:%s\n\r", 24597u, "Memory ordering and "
		"serialization_ACRN hypervisor shall expose memory ordering instructions to any VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39236u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 39265u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 39266u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 39298u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_005");

	printf("\t\t Case ID:%d case name:%s\n\r", 39300u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 39302u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_007");
	printf("\t\t Case ID:%d case name:%s\n\r", 39312u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_008");
	printf("\t\t Case ID:%d case name:%s\n\r", 39313u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_010");
	printf("\t\t Case ID:%d case name:%s\n\r", 39314u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_011");
#endif
#ifdef IN_NATIVE
		printf("\t\t Case ID:%d case name:%s\n\r", 39315u,
			"Accesses to WT, WB or WP memory ranges follows processor ordering_012");
		printf("\t\t Case ID:%d case name:%s\n\r", 39316u,
			"Accesses to WT, WB or WP memory ranges follows processor ordering_013");
#endif
#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	printf("\t\t Case ID:%d case name:%s\n\r", 39317u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_014");
	printf("\t\t Case ID:%d case name:%s\n\r", 39322u,
		"Accesses to WT, WB or WP memory ranges follows processor ordering_015");
	printf("\t\t Case ID:%d case name:%s\n\r", 39348u,
		"Accesses to UC or UC- memory ranges follows strong ordering_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39349u,
		"Accesses to UC or UC- memory ranges follows strong ordering_002");
#endif
}

void memory_order_ap(int local_id)
{
	while (memory_order_sync != MEMORY_ORDER_TEST_MAX) {
		memory_ordering_ap_test_entry[local_id]();
	}
}

void ap_main(void)
{
	int local_id = get_lapic_id();
	int max_local_id = CFG_TEST_MEMORY_ORDERING_CPU_NR;
#ifdef IN_NATIVE
	local_id /= 2;
	max_local_id = CFG_TEST_MEMORY_ORDERING_CPU_NR_NATIVE;
#endif
	if (local_id >= max_local_id) {
		debug_print("<HALT *AP* > un-used processor id %d\n", local_id);
		return;
	} else {
		debug_print("<Enter *AP* > processor id %d\n", local_id);
	}

	while (1) {
		memory_order_ap(local_id);
	}
}

int main(int ac, char **av)
{
	print_case_list();
	debug_print("\n");

	setup_idt();
#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	memory_ordering_rqmid_24597_mfence_001();
	memory_ordering_rqmid_39236_processor_ordering_002();
	memory_ordering_rqmid_39265_processor_ordering_003();
	memory_ordering_rqmid_39266_processor_ordering_004();
	memory_ordering_rqmid_39298_processor_ordering_005();

	memory_ordering_rqmid_39300_processor_ordering_006();
	memory_ordering_rqmid_39302_processor_ordering_007();
	memory_ordering_rqmid_39312_processor_ordering_008();
	memory_ordering_rqmid_39313_processor_ordering_010();
	memory_ordering_rqmid_39314_processor_ordering_011();
#endif
#ifdef IN_NATIVE
	memory_ordering_rqmid_39315_processor_ordering_012();
	memory_ordering_rqmid_39316_processor_ordering_013();
#endif
#if defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE)
	memory_ordering_rqmid_39317_processor_ordering_014();
	memory_ordering_rqmid_39322_processor_ordering_015();
	memory_ordering_rqmid_39348_uc_ordering_001();
	memory_ordering_rqmid_39349_uc_ordering_002();
#endif
	return report_summary();
}
