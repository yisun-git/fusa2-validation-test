/*
 * Test for x86 HSI memory management instructions
 *
 * Copyright (c) 2020 Intel
 *
 * Authors:
 *  HeXiang Li <hexiangx.li@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#define USE_DEBUG

#include "segmentation.h"
#include "instruction_common.h"
#include "debug_print.h"
#include "desc.h"
#include <asm/spinlock.h>
#include "atomic.h"
#include "memory_type.h"
#include "debug_print.h"
#include "delay.h"
#include "paging.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "alloc.h"
#include "misc.h"
#include "register_op.h"

#define CPUID_1_ECX_MONITOR			(1 << 3)
#define CPUID_1_EDX_PAE		                (1 << 6)
#define CPUID_801_EDX_Page1GB                   (1 << 26)
#define CPUID_801_EDX_LM			(1 << 29)
#define PHYSICAL_ADDRESS_WIDTH	                39

#define CPUID_EXTERN_INFORMATION_801            0x80000001
#define CPUID_EXTERN_INFORMATION_808            0x80000008
#define BIT_SHIFT(n)                    ((1U) << (n))
#define CR0_BIT_NW                              29
#define CR0_BIT_CD                              30
#define CR0_BIT_PG				31
#define CR4_BIT_PAE			        5
#define CR4_BIT_SMEP                            20
#define CR4_BIT_SMAP                            21
#define GET_CASE_NAME(x) #x

typedef struct dst_case_suit {
	const char *case_name;
	int case_id;
	void (*case_fun)(void);
} st_case_suit;

/*index:0x50, RPL:0 GDT*/
#define SELECTOR_INDEX_MODIFY				(SELECTOR_INDEX_280H >> 3)
#define SEGMENT_LIMIT_NOT_ALL	0xfffe

static bool do_change_seg;

/* when change ring3 mode, the cs, ds, ss is change, can run success */
__unused static void test_segmemt_config_at_ring3()
{
	do_change_seg = true;
}

static int sgdt_checking(struct descriptor_table_ptr *ptr)
{
	asm volatile(ASM_TRY("1f")
		"sgdt %0\n"
		"1:"
		: "=m" (*ptr) : : "memory"
	);
	return exception_vector();
}

static int lgdt_checking(const struct descriptor_table_ptr *ptr)
{
	asm volatile(ASM_TRY("1f")
		"lgdt %0\n"
		"1:"
		: : "m" (*ptr) :
	);
	return exception_vector();
}

static int sldt_checking(volatile u16 *ptr)
{
	asm volatile(ASM_TRY("1f")
		"sldt %0\n"
		"1:"
		: "=m" (*ptr) : : "memory"
	);
	return exception_vector();
}

static int lldt_checking(const volatile u16 *ptr)
{
	asm volatile(ASM_TRY("1f")
		"lldt %0\n"
		"1:"
		: : "m" (*ptr) :
	);
	return exception_vector();
}

void set_ldt_entry(gdt_entry_t *gdt, int sel, u32 base,  u32 limit, u8 access, u8 gran)
{
	int num = sel >> 3;

	/* Setup the descriptor base address */
	gdt[num].base_low = (base & 0xFFFF);
	gdt[num].base_middle = (base >> 16) & 0xFF;
	gdt[num].base_high = (base >> 24) & 0xFF;

	/* Setup the descriptor limits */
	gdt[num].limit_low = (limit & 0xFFFF);
	gdt[num].granularity = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	gdt[num].granularity |= (gran & 0xF0);
	gdt[num].access = access;
}

static bool segment_lgdt_sgdt_test(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;
	struct descriptor_table_ptr save_gdt_desc;

	u8 *new_gdt_base;
	int ret1 = 1;
	int ret2 = 1;
	u32 chk = 0;

	sgdt(&old_gdt_desc);
	new_gdt_base = (u8 *)malloc(2 * PAGE_SIZE);
	if (new_gdt_base == NULL) {
		debug_print("Alloc memory fail for new gdt table.");
	}
	/* create a new gdt for test */
	memcpy((void *)new_gdt_base, (void *)old_gdt_desc.base, PAGE_SIZE);
	memcpy((void *)(new_gdt_base + PAGE_SIZE), (void *)old_gdt_desc.base, PAGE_SIZE);
	new_gdt_desc.base = (ulong)new_gdt_base;
	new_gdt_desc.limit = 2 * PAGE_SIZE;

	ret1 = lgdt_checking(&new_gdt_desc);
	if (ret1 == PASS) {
		chk++;
	}

	delay(100);
	ret2 = sgdt_checking(&save_gdt_desc);
	if (ret2 == PASS) {
		chk++;
	}

	if ((new_gdt_desc.base == save_gdt_desc.base) &&
		(new_gdt_desc.limit == save_gdt_desc.limit)) {
		chk++;
	}

	debug_print("lgdt ret1:%d sgdt ret2:%d\n", ret1, ret2);

	/* resume environment */
	lgdt(&old_gdt_desc);
	free(new_gdt_base);
	new_gdt_base = NULL;

	if (chk == 3) {
		return true;
	}

	return false;
}

static bool segment_lldt_sldt_test(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	u8 *new_gdt_base;
	int ret1 = 1;
	int ret2 = 1;
	u32 chk = 0;
	u16 volatile sel = SELECTOR_INDEX_280H;
	u16 volatile new_ldt = 0;

	sgdt(&old_gdt_desc);
	new_gdt_base = (u8 *)malloc(2 * PAGE_SIZE);
	if (new_gdt_base == NULL) {
		debug_print("Alloc memory fail for new gdt table.");
	}
	/* create a new gdt for test */
	memcpy((void *)new_gdt_base, (void *)old_gdt_desc.base, PAGE_SIZE);
	memcpy((void *)(new_gdt_base + PAGE_SIZE), (void *)old_gdt_desc.base, PAGE_SIZE);
	new_gdt_desc.base = (ulong)new_gdt_base;
	new_gdt_desc.limit = 2 * PAGE_SIZE;

	set_ldt_entry((gdt_entry_t *)new_gdt_base, SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL,
		SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_SYS|SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_LDT,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&new_gdt_desc);
	ret1 = lldt_checking(&sel);
	if (ret1 == PASS) {
		chk++;
	}

	delay(100);
	ret2 = sldt_checking(&new_ldt);
	if (ret2 == PASS) {
		chk++;
	}

	if (new_ldt == SELECTOR_INDEX_280H) {
		chk++;
	}

	debug_print("lldt ret1:%d sldt ret2:%d new_sel:%x\n", ret1, ret2, new_ldt);

	/* resume environment */
	lgdt(&old_gdt_desc);
	free(new_gdt_base);
	new_gdt_base = NULL;

	if (chk == 3) {
		return true;
	}

	return false;
}


#ifdef IN_NATIVE
#ifdef __x86_64__
/* test case which should run under 64bit */
#include "64/hsi_mem_mgmt_fn.c"
#elif __i386__
/* test case which should run under 32bit */
#include "32/hsi_mem_mgmt_fn.c"
#endif
#endif

static void print_case_list()
{
	printf("HSI memory management feature case list:\n\r");
#if defined(IN_NATIVE)
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit); i++) {
		printf("\t Case ID: %d Case name: %s\n\r", case_suit[i].case_id,
			case_suit[i].case_name);
	}
#endif
}


int main(int ac, char **av)
{
	print_case_list();
	debug_print("\n");

	setup_idt();

	setup_vm();
	setup_ring_env();

#if defined(IN_NATIVE)
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit); i++) {
		case_suit[i].case_fun();
	}
#endif
	return report_summary();
}

