/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2019/12/16
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *
 */
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
#include <linux/pci_regs.h>
#include "alloc_page.h"
#include "alloc_phys.h"
#include "asm/page.h"
#include "x86/vm.h"

#include "instruction_common.h"
#include "xsave.h"
#include "regdump.h"


#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif

// add $0xffffffffffffff80(0x80 sign-extension), %%rax
#define ADD_SIGN ".byte 0x48, 0x83, 0xc0, 0x80\n\t"

// sub $0xffffffffffffff80(0x80 sign-extension), %%rax
#define SUB_SIGN ".byte 0x48, 0x83, 0xe8, 0x80\n\t"
#define MAIN_TSS_SEL (FIRST_SPARE_SEL + 24)
#define USING_APS                   3
#define    HOST_GDT_RING0_DATA_SEL        (0x0010U)

static char intr_alt_stack[4][4096];
static volatile int start_run_id = 0;

#ifdef __x86_64__
static atomic_t ret_sim;
static atomic_t ap_sim;
static int XSAVE_AREA_ST0_POS = 32;
#define XSAVE_AREA_XMM0_POS 160
__attribute__((aligned(64))) xsave_area_t st_xsave_area;

typedef unsigned __attribute__((vector_size(16))) unsigned_128_bit;
typedef union {
	unsigned_128_bit sse ALIGNED(16);
	unsigned m[4] ALIGNED(16);
} union_unsigned_128;
static union_unsigned_128 unsigned_128;

typedef unsigned __attribute__((vector_size(32))) unsigned_256_bit;
typedef union {
	unsigned_128_bit sse ALIGNED(32);
	unsigned m[4] ALIGNED(32);
} union_unsigned_256;
static union_unsigned_256 unsigned_256;
#endif

#ifdef __x86_64__
static void test_iret(const char *fun_name)
{
	return;
}

static bool cmovnc_8_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $10, %%rbx\n\t"
		ASM_TRY("1f")
		/* CF = 0,It will move */
		"cmovnc %%rbx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 10));
}

static bool cmovnc_8_ncf_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $10, %%rbx\n\t"
		"mov $0xff, %%al\n\t"
		"add $10, %%al\n\t"
		ASM_TRY("1f")
		/* CF = 1,It won't move */
		"cmovnc %%rbx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0));
}

/* move $10 to variable op */
static bool mov_checking(void)
{
	u32 op = 0;
	asm volatile(ASM_TRY("1f")
		"mov $10, %%eax\n"
		"mov %%eax, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 10));
}

/*
 * sign-extend byte 0x80 which sigh bit is 1 to 0xffffff80,
 * then move it to the variable op
 */
static bool movsx_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0x80, %%al\n"
		ASM_TRY("1f")
		"movsx %%al, %%ebx\n"
		"mov %%ebx, %0\n\t"
		"1:"
		: "=r" (op)
		:
		: "eax", "ebx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xffffff80));
}

/* zero-extend byte 0x80 to 0x80, then move it to the variable op  */
static bool movzx_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0x80, %%al\n"
		ASM_TRY("1f")
		"movzx %%al, %%ebx\n"
		"mov %%ebx, %0\n\t"
		"1:"
		: "=r" (op)
		:
		: "eax", "ebx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x80));
}

/* convert value 0x80000000 to double  with sign-extension */
static bool cdqe_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x80000000, %%eax\n"
		ASM_TRY("1f")
		"cdqe\n"
		"mov %%rax, %0\n\t"
		"1:"
		: "=r" (op)
		:
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xffffffff80000000));
}

/*
 * exchange the variable data1 and the variable data2,
 * then add data1 which equal to the original data2 to
 * the data2  which equal to  the original data1,
 * finally move data2 to the variable op
 */
static bool xchg_checking(void)
{
	u64 op = 0;
	u64 data1 = 2;
	u64 data2 = 5;
	asm volatile(ASM_TRY("1f")
		"xchg %1, %2\n"
		"mov %2, %0\n"
		"1:"
		: "=r" (op)
		: "r" (data1), "r" (data2)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 2));
}

/*
 * exchange the variable data1 and the variable data2,
 * then move data2 which equal to the
 * original data1 to the variable op
 */
static bool xadd_checking(void)
{
	u32 op = 0;
	u32 data1 = 2;
	u32 data2 = 5;
	asm volatile(ASM_TRY("1f")
		"xadd %1, %2\n"
		"mov %2, %0\n"
		"1:"
		: "=r" (op)
		: "r" (data1), "r" (data2)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 7));
}

/* 0x80 sign-entenden to 0xffffffffffffff80, add to rax */
static bool add_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0, %%rax\n\t"
		ASM_TRY("1f")
		ADD_SIGN
		"mov %%rax, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xffffffffffffff80));
}

/* 0x80 sign-entenden to $0xffffffffffffff80, subtract fron rax */
static bool sub_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0xffffffffffffff90, %%rax\n\t"
		ASM_TRY("1f")
		SUB_SIGN
		"mov %%rax, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x10));
}

/* mov -1 to ebx */
static bool neg_checking(void)
{
	u32 op = 1;
	asm volatile(
		"mov $0xffffffff, %%ebx\n\t"
		ASM_TRY("1f")
		"neg %%ebx\n\t"
		"mov %%ebx, %0\n\t"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1));
}

/* increment the variable op by 1 */
static bool inc_checking(void)
{
	u16 op = 0;
	asm volatile(ASM_TRY("1f")
		"inc %0\n\t"
		"1:"
		: "+r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1));
}

/* decrement the variable op by 1 */
static bool dec_checking(void)
{
	u16 op = 10;
	asm volatile(ASM_TRY("1f")
		"dec %0\n"
		"1:"
		: "+r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 9));
}

static bool mul_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $128, %%al\n"
		"mov $2, %%cl\n"
		ASM_TRY("1f")
		"mul %%cl\n"
		"mov %%ax, %0\n"
		"1:"
		: "=r"(op)
		: : "rax", "rcx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 256));
}

static bool div_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $128, %%ax\n"
		"mov $2, %%cl\n"
		ASM_TRY("1f")
		"div %%cl\n"
		"mov %%ax, %0\n"
		"1:"
		: "=r"(op)
		: : "ax", "cx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 64));
}

/* compare value 0x1 with 0x8, execute instruction JNZ to another code block */
static bool cmp_1_checking(void)
{
	u8 op = 0;
	asm volatile(
		"mov $0x8, %%al\n"
		ASM_TRY("1f")
		"cmp $0x1, %%al\n"
		"jnz 2f\n"
		"mov $0xf, %%al\n"
		"2: mov %%al, %0\n"
		"1:"
		: "=r"(op)
		: : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

static bool cmp_2_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0x8, %%ax\n"
		ASM_TRY("1f")
		"cmp $0x1, %%ax\n"
		"jnz 2f\n"
		"mov $0xf, %%ax\n"
		"2: mov %%ax, %0\n"
		"1:"
		: "=r"(op)
		: : "ax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

static bool cmp_4_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0x8, %%eax\n"
		ASM_TRY("1f")
		"cmp $0x1, %%eax\n"
		"jnz 2f\n"
		"mov $0xf, %%eax\n"
		"2: mov %%eax, %0\n"
		"1:"
		: "=r"(op)
		: : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

static bool cmp_8_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x8, %%rax\n"
		ASM_TRY("1f")
		"cmp $0x1, %%rax\n"
		"jnz 2f\n"
		"mov $0xf, %%rax\n"
		"2: mov %%rax, %0\n"
		"1:"
		: "=r"(op)
		: : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

static bool push_pop_checking(void)
{
	u16 op = 0;
	asm volatile(
	    "mov $0xf, %%ax\n"
	    ASM_TRY("1f")
	    "push %%ax\n"
	    "sub %%ax, %%ax\n"
	    "pop %%ax\n"
	    "mov %%ax, %0\n"
	    "1:"
	    : "=r"(op)
	    : : "ax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 15));
}

static bool pushf_popf_checking(void)
{
	u16 op = 0;
	asm volatile(
	    ASM_TRY("1f")
	    "pushf\n\t"
	    "pop %%bx\n\t"
	    "mov %%bx, %%ax\n\t"
	    "or $1, %%ax\n\t"
	    "push %%ax\n\t"
	    "popf\n\t"
	    "pushf\n\t"
	    "pop %%ax\n\t"
	    "and $1, %%ax\n\t"
	    "mov %%ax, %0\n\t"
	    "push %%bx\n\t"
	    "popf\n\t"
	    "1:"
	    : "=r"(op)
	    : : "ax", "bx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 1));
}

static bool xor_checking(void)
{
	u32 op = 7;
	asm volatile(ASM_TRY("1f")
		"xor $0xf0ffffff, %0\n\t"
		"1:"
		: "+r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xf0fffff8));
}

static bool not_checking(void)
{
	u64 op = 7;
	asm volatile(ASM_TRY("1f")
		"not %0\n\t"
		"1:"
		: "+r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xfffffffffffffff8));
}

/* shift the bits right, equal to divide the variable op with 2 */
static bool shr_checking(void)
{
	u64 op = 0x100;
	asm volatile(ASM_TRY("1f")
		"shr %0\n\t"
		"1:"
		: "+r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x80));
}

/*
 * select the test bit in the variable op, store it in the CF flag,
 * move the CF flag to variable ret
 */
static bool bt_checking(void)
{
	u64 op  = 0x1234;
	u64 ret = 0x5;
	asm volatile(ASM_TRY("1f")
		"bt $5, %0\n\t"
		"pushf; pop %1\n\t"
		"and $1, %1\n\t"
		"1:"
		: "+r"(op), "=r"(ret)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1234) && (ret == 0x1));
}

/*
 * select the test bit in the variable op, store it in the CF flag,
 * and set the test bit
 */
static bool bts_checking(void)
{
	u64 op = 0x1234;
	u64 ret = 0x5;
	asm volatile(ASM_TRY("1f")
		"bts $5, %0\n\t"
		"pushf; pop %1\n\t"
		"and $1, %1\n\t"
		"1:"
		: "+r"(op), "=r"(ret)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1234)) && (ret == 0x1);
}

/*
 * select the test bit in the variable op, store it in the CF flag,
 * and reset the test bit
 */
static bool btr_checking(void)
{
	u64 op = 0x1234;
	u64 ret = 0x5;
	asm volatile(ASM_TRY("1f")
		"btr $5, %0\n\t"
		"pushf; pop %1\n\t"
		"and $1, %1\n\t"
		"1:"
		: "+r"(op), "=r"(ret)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1214) && (ret == 0x1));
}

/*
 * select the test bit in the variable op, store it in the CF flag,
 * and complement the test bit
 */
static bool btc_checking(void)
{
	u64 op = 0x1234;
	u64 ret = 0x5;
	asm volatile(ASM_TRY("1f")
		"btc $5, %0\n\t"
		"pushf; pop %1\n\t"
		"and $1, %1\n\t"
		"1:"
		: "+r"(op), "=r"(ret)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1214) && (ret == 0x1));
}

/* search for the least significant set bit */
static bool bsf_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x1234, %%rax\n"
		ASM_TRY("1f")
		"bsf %%rax, %0\n\t"
		"1:"
		: "=r"(op) : : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x2));
}

/*
 * search the value 0x1234 for the most significant set bit,
 * and store the index in the variable op
 */
static bool bsr_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x1234, %%rax\n"
		ASM_TRY("1f")
		"bsr %%rax, %0\n\t"
		"1:"
		: "=r"(op) : : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xc));
}

/*
 * Set CF 1, execute instruction setnc to set register al to 1,
 * and store it in the variable op
 */
static bool setcc_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x1234, %%rax\n"
		ASM_TRY("1f")
		"setnz %%al\n\t"
		"mov %%rax, %0\n"
		"1:"
		: "=r"(op) : : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1201));
}

/* TEST will set ZF flag to 0 . then jump to the right place */
static bool test_checking(void)
{
	u8 op = 0;
	asm volatile(
		"mov $0x8, %0\n"
		"mov $0x25, %%al\n"
		ASM_TRY("1f")
		"test $0x9, %%al\n"
		"jnz 2f\n"
		"mov $0x0, %0\n"
		"2: \n"
		"1:"
		: "=r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

static bool jmp_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0x8, %0\n"
		ASM_TRY("1f")
		"jmp 2f\n"
		"mov $0xf, %0\n"
		"2: \n"
		"1:"
		: "=r"(op)
	);

	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

/* execute the iret instruction to return to the interrupt entry */
static bool iret_far_checking(void (*fn)(const char *), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
		"mov %%dx, %%ds\n\t"
		"mov %%dx, %%es\n\t"
		"mov %%dx, %%fs\n\t"
		"mov %%dx, %%gs\n\t"
		"mov %%" R "sp, %%" R "cx\n\t"
		"push" W " %%" R "dx \n\t"
		"lea %[user_stack_top], %%" R "dx \n\t"
		"push" W " %%" R "dx \n\t"
		"pushf" W "\n\t"
		"push" W " %[user_cs] \n\t"
		"push" W " $1f \n\t"
		ASM_TRY("1f")
		"iret" W "\n"
		"1: \n\t"
		/* save kernel SP */
		"push %%" R "cx\n\t"
		"call *%[fn]\n\t"

		"pop %%" R "cx\n\t"
		"mov $1f, %%" R "dx\n\t"
		"int %[kernel_entry_vector]\n\t"
		".section .text.entry \n\t"
		".globl kernel_entry\n\t"
		"kernel_entry: \n\t"
		"mov %%" R "cx, %%" R "sp \n\t"
		"mov %[kernel_ds], %%cx\n\t"
		"mov %%cx, %%ds\n\t"
		"mov %%cx, %%es\n\t"
		"mov %%cx, %%fs\n\t"
		"mov %%cx, %%gs\n\t"
		"jmp *%%" R "dx \n\t"
		".section .text\n\t"
		"1:\n\t"
		: [ret] "=&a" (ret)
		: [user_ds] "i" (USER_DS),
		[user_cs] "i" (USER_CS),
		[user_stack_top]"m"(user_stack[sizeof user_stack]),
		[fn]"r"(fn),
		[arg]"D"(arg),
		[kernel_ds]"i"(KERNEL_DS),
		[kernel_entry_vector]"i"(0x23)
		: "rcx", "rdx");

	return (exception_vector() == NO_EXCEPTION);
}

/* sent 0xff to port 0xe0, recieve from it */
static bool out_in_1_checking(void)
{
	u8 op = 0;
	asm volatile(
		"mov $0xff, %%al\n"
		"out %%al, $0xe0\n"
		"mov $0x0, %%al\n"
		ASM_TRY("1f")
		"in $0xe0, %%al\n"
		"mov $0, %0\n"
		"mov %%al, %0\n"
		"1:"
		: "=r" (op)
		: : "rax"
	);

	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff));
}

/* sent 0xffff to port 0xe0, recieve from it */
static bool out_in_2_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0xffff, %%ax\n"
		"out %%ax, $0xe0\n"
		"mov $0x0, %%ax\n"
		ASM_TRY("1f")
		"in $0xe0, %%ax\n"
		"mov $0, %0\n"
		"mov %%ax, %0\n"
		"1:"
		: "=r" (op)
		: : "rax"
	);

	return ((exception_vector() == NO_EXCEPTION) && (op == 0xffff));
}

/* sent 0xffffffff to port 0xe0, recieve from it */
static bool out_in_4_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0xffffffff, %%eax\n"
		"out %%eax, $0xe0\n"
		"mov $0x0, %%eax\n"
		ASM_TRY("1f")
		"in $0xe0, %%eax\n"
		"mov $0, %0\n"
		"mov %%eax, %0\n"
		"1:"
		: "=r" (op)
		: : "rax"
	);

	return ((exception_vector() == NO_EXCEPTION) && (op == 0xffffffff));
}
static bool jne_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0x8, %0\n"
		ASM_TRY("1f")
		"jne 2f\n"
		"mov $0xf, %0\n"
		"2: \n"
		"1:"
		: "=r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

/* adding $0x8 to $0x7f can make OF flag to 1 */
static bool jo_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0x8, %0\n"
		"mov $0x7f, %%bl\n"
		"add $0x3,  %%bl\n"
		ASM_TRY("1f")
		"jo 2f\n"
		"mov $0xf, %0\n"
		"2: \n"
		"1:"
		: "=r"(op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

/* move the byte from address RSI to address RDI */
static bool movs_checking(void)
{
	u64 op = 0;
	u64 data = 12;
	asm volatile(
		"lea %1, %%rsi\n\t"
		"lea %0, %%rdi\n"
		"mov $0x8,  %%rcx\n"
		ASM_TRY("1f")
		"rep movsb \n"
		"1:"
		: "+m" (op)
		: "m" (data)
		: "rsi", "rdi", "rcx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 12));
}

/*
 * store the byte from register al to the memory which address is in register rdi
 * repeat it the number of times specified in the register rcx
 */
static bool stos_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $1, %%al\n\t"
		"lea %0, %%rdi\n"
		"mov $0x4,  %%rcx\n"
		ASM_TRY("1f")
		"rep stosb \n"
		"1:"
		: "+m" (op)
		:
		: "rax", "rdi", "rcx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x1010101));
}

/* release the stack frame set up before, restore the register RBP and RSP */
static bool leave_checking(void)
{
	u64 op = 0;
	asm volatile(
		"call 2f\n"
		"jmp 3f\n\t"
		"2: endbr64\n\t"
		"push %%rbp\n"
		"mov  %%rsp,%%rbp\n\t"
		ASM_TRY("1f")
		"leave\n\t"
		"ret\n\t"
		"3: mov $2, %0\n"
		"1:"
		: "=r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 2));
}

/* string operations increment the index registers RSI and RDI */
static bool cld_checking(void)
{
	u64 op = 0;
	u64 data = 12;
	asm volatile(
		ASM_TRY("1f")
		"cld\n\t"
		"lea %1, %%rsi\n\t"
		"lea %0, %%rdi\n"
		"mov $0x8,  %%rcx\n"
		"rep movsb \n"
		"1:"
		: "+m" (op)
		: "m" (data)
		: "rsi", "rdi", "rcx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 12));
}

/* get the effective address of variable data and store it in register RBX */
static bool lea_checking(void)
{
	u64 op = 0;
	u64 data = 12;
	asm volatile(
		ASM_TRY("1f")
		"lea %1, %%rbx\n\t"
		"mov (%%rbx), %0\n\t"
		"1:"
		: "=r" (op)
		: "m" (data)
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 12));
}

/*
 * EBX "Genu"
 * EDX "inel"
 * ECX "ntel"
 */
static bool cpuid_checking(void)
{
	u32 op = 0;
	u32 op2 = 0;
	u32 op3 = 0;
	asm volatile(
		"mov $0, %%eax\n\t"
		ASM_TRY("1f")
		"cpuid\n\t"
		"mov %%ebx, %0\n\t"
		"mov %%edx, %1\n\t"
		"mov %%ecx, %2\n\t"
		"1:"
		: "=r" (op), "=r" (op2), "=r" (op3)
		:
		: "ebx", "ecx", "edx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x756E6547) && \
			(op2 ==  0x49656E69) && (op3 == 0x6C65746E));
}

/* no operation */
static bool nop_checking(void)
{
	u64 op = 0;
	asm volatile(
		ASM_TRY("1f")
		"nop\n\t"
		"mov $3, %0\n\t"
		"1:"
		: "=r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 3));
}

/* Description: BP will control 2 APs to start/end, and check result */
static void test_bp()
{
	u16 val = 0;
	u16 desc_size = sizeof(tss64_t);
	tss64_t main_tss;
	/* tell AP to start */
	while (atomic_read(&ap_sim) != USING_APS) {}

	/* create TSS descriptor */
	main_tss.ist1 = (u64)intr_alt_stack + 4096;
	main_tss.iomap_base = (u16)desc_size;
	set_gdt64_entry(MAIN_TSS_SEL, (u64)&main_tss, desc_size - 1, 0x89, 0x0f);
	ltr(MAIN_TSS_SEL);
	asm volatile(ASM_TRY("1f")
		"str %%ax\n\t"
		"mov %%ax,%0\n\t"
		"1:"
		: "=m"(val)
	);
	if (val == MAIN_TSS_SEL) {
		atomic_inc(&ret_sim);
	}
}

/* Description: 2 APs will test concurrently */
static __unused void test_ap(int index)
{
	u16 val = 0;
	u16 desc_size = sizeof(tss64_t);
	tss64_t ap_tss;

	/* create TSS descriptor */
	ap_tss.ist1 = (u64)intr_alt_stack[index / 2] + 4096;
	ap_tss.iomap_base = (u16)desc_size;
	set_gdt64_entry((MAIN_TSS_SEL+index*8), (u64)&ap_tss, desc_size - 1, 0x89, 0x0f);
	ltr(MAIN_TSS_SEL + index*8);
	asm volatile(ASM_TRY("1f")
		"str %%ax\n\t"
		"mov %%ax,%0\n\t"
		"1:"
		: "=m"(val)
	);
	if (val == (MAIN_TSS_SEL + index*8)) {
		atomic_inc(&ret_sim);
	}
	atomic_inc(&ap_sim);

	return;
}

static bool rdtsc_checking(void)
{
	unsigned a, d;

	asm volatile(
		ASM_TRY("1f")
		"rdtsc\n\t"
		"1:"
		: "=a"(a), "=d"(d)
	);

	return (exception_vector() == NO_EXCEPTION);
}

static bool rdtscp_checking(void)
{
	unsigned a, d;

	asm volatile(
		ASM_TRY("1f")
		"rdtscp\n\t"
		"1:"
		: "=a"(a), "=d"(d)
	);

	return (exception_vector() == NO_EXCEPTION);
}

static bool verw_checking(void)
{
	u16 ds = HOST_GDT_RING0_DATA_SEL;

	asm volatile(
		ASM_TRY("1f")
		"verw %[ds]\n\t"
		"1:"
		: : [ds] "m" (ds) : "cc"
	);

	return (exception_vector() == NO_EXCEPTION);
}

static __unused int xrstors_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xrstors %[addr]\n\t" /* xrstors */
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return (exception_vector() == NO_EXCEPTION);
}

static int xsetbv_checking(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile(ASM_TRY("1f")
		"xsetbv\n\t" /* xsetbv */
		"1:"
		: : "a" (eax), "d" (edx), "c" (index));

	return (exception_vector() == NO_EXCEPTION);
}

static void fpu_add(float *p, float *q)
{
	asm volatile(
		"fninit\n"
		"fld %0\n"   // push to fpu register stack
		"fld %1\n"
		"fadd %%st(1), %%st(0)\n"
		"fst %0\n"  // copy st(0), don't pop stack
		: "+m"(*p)
		: "m"(*q) : "memory"
	);
}

/* Description: Execute x87 instruction */
static void execute_x87_test()
{
	/* MP, EM, TS */
	write_cr0(read_cr0() & ~0xe);
	/* OSFXSR */
	write_cr4(read_cr4() | 0x600);

	float f = 30.00f;
	float f1 = 60.00f;

	fpu_add(&f, &f1);
}

static __unused int xsaves_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xsaves %[addr]\n\t"
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return (exception_vector() == NO_EXCEPTION);
}

/* Description: Execute SSE instruction */
static __attribute__((target("sse")))
void execute_sse_test(unsigned sse_data)
{
	sse_union sse;

	sse.u[0] = sse_data;
	sse.u[1] = sse_data + 1;
	sse.u[2] = sse_data + 2;
	sse.u[3] = sse_data + 3;

	write_cr0(read_cr0() & ~6);
	write_cr4(read_cr4() | 0x200);

	asm volatile("movapd %0, %%xmm0" : : "m"(sse));
}

static int vmovapd_check(avx_union *avx_data)
{
	asm volatile(ASM_TRY("1f")
		"vmovapd %0, %%ymm0\n\t"
		"1:" : : "m" (*avx_data));

	return (exception_vector() == NO_EXCEPTION);
}

/* Description: Execute AVX instruction */
static __attribute__((target("avx"))) int execute_avx_test()
{
	ALIGNED(32) avx_union avx1;

	/* EM, TS */
	write_cr0(read_cr0() & ~6);
	/* OSFXSR */
	write_cr4(read_cr4() | 0x600);

	avx1.m[0] = 1.0f;
	avx1.m[1] = 2.0f;
	avx1.m[2] = 3.0f;
	avx1.m[3] = 4.0f;
	avx1.m[4] = 5.0f;
	avx1.m[5] = 6.0f;
	avx1.m[6] = 7.0f;
	avx1.m[7] = 8.0f;

	return vmovapd_check(&avx1);
}

#elif __i386__
/* CF = 0,It will move */
static bool cmovae_pro_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $0xff12, %%bx\n\t"
		ASM_TRY("1f")
		"cmovae %%bx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff12));
}

/* ZF = 0,It will move */
static bool cmovne_pro_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $0xff12, %%ebx\n\t"
		ASM_TRY("1f")
		"cmovne %%ebx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff12));
}

static bool or_pro_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0xff,  %%bx\n\t"
		ASM_TRY("1f")
		"or %%bx, %0\n\t"
		"1:"
		: "+r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff));
}

/* shift the bits left, equal to multiply the variable op with 2 */
static bool shl_pro_checking(void)
{
	u32 op = 0x1;
	asm volatile(ASM_TRY("1f")
		"shl $2, %0\n\t"
		"1:"
		: "+r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x4));
}

static bool jmp_pro_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0x8, %0\n"
		"3:nop\n"
		ASM_TRY("1f")
		"jmp 2f\n"
		".= 3b+128\n"
		"2: \n"
		"1:"
		: "=r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

/* jump to original code block successfully */
static bool ret_pro_checking(void)
{
	u32 op = 0;
	asm volatile(
		"call 2f\n"
		"jmp 3f\n\t"
		"2: \n\t"
		ASM_TRY("1f")
		"ret\n\t"
		"mov $10, %0\n"
		"3: mov $2, %0\n"
		"1:"
		: "=r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 2));
}

/* CLI instruction make the rflags.IF to 0 */
static bool cli_pro_checking(void)
{
	u32 op = 1;
	asm volatile(
		"pushf\n"
		"pop %%ebx\n\t"
		ASM_TRY("1f")
		"cli\n\t"
		"pushf\n"
		"pop %0\n"
		"push %%ebx\n"
		"popf\n"
		"and $0x200, %0\n"
		"1:"
		: "+r" (op)
		: : "ebx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0));
}
#endif

/* move $0x10 to segment FS */
static bool mov_segment_checking(void)
{
	u16 op = 0x8;
	asm volatile(
		"mov %%fs, %%bx\n"
		ASM_TRY("1f")
		"mov $0x10, %%cx\n\t"
		"mov %%cx, %%fs\n"
		"mov %%fs, %0\n"
		"mov %%bx, %%fs\n"
		"1:"
		: "=r" (op)
		: : "ebx", "ecx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x10));
}

/* read and write TSC msr */
static bool msr_checking(void)
{
	u64 tsc;
	u64 op = 0;

	tsc = rdmsr(MSR_IA32_TSC);
	tsc += 1000;
	wrmsr(MSR_IA32_TSC, tsc);
	op  = rdmsr(MSR_IA32_TSC);

	return ((exception_vector() == NO_EXCEPTION) && (op > tsc));
}

#ifdef __x86_64__
/*
 * @brief case name: HSI_Generic_Processor_Features_Data_Move_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * CMOVNC, MOV.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35956_generic_processor_features_data_move_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMOVNC, MOV */
	if (cmovnc_8_checking()) {
		chk++;
	}

	if (cmovnc_8_ncf_checking()) {
		chk++;
	}

	if (mov_checking()) {
		chk++;
	}

	report("%s", (chk == 3), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Data_Exchange_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * XADD, XCHG.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41095_generic_processor_features_data_exchange_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* XADD, XCHG */
	if (xadd_checking()) {
		chk++;
	}
	if (xchg_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Stack_Manipulation_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * PUSH, POP, PUSHF, POPF.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35959_generic_processor_features_stack_manipulation_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* PUSH, POP, PUSHF, POPF */
	if (push_pop_checking()) {
		chk++;
	}
	if (pushf_popf_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Type_Conversion_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * MOVSX, MOVZX, CDQE.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41096_generic_processor_features_type_conversion_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* MOVSX, MOVZX, CDQE */
	if (movsx_checking()) {
		chk++;
	}
	if (movzx_checking()) {
		chk++;
	}
	if (cdqe_checking()) {
		chk++;
	}

	report("%s", (chk == 3), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Binary_Arithmetic_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * ADD, SUB, NEG, INC, DEC, MUL, DIV.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40347_generic_processor_features_binary_arithmetic_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* ADD, SUB, NEG, INC, DEC, MUL, DIV */
	if (add_checking()) {
		chk++;
	}
	if (sub_checking()) {
		chk++;
	}
	if (neg_checking()) {
		chk++;
	}
	if (inc_checking()) {
		chk++;
	}
	if (dec_checking()) {
		chk++;
	}
	if (mul_checking()) {
		chk++;
	}
	if (div_checking()) {
		chk++;
	}

	report("%s", (chk == 7), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Comparision_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * CMP.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41097_generic_processor_features_comparision_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMP */
	if (cmp_1_checking()) {
		chk++;
	}
	if (cmp_2_checking()) {
		chk++;
	}
	if (cmp_4_checking()) {
		chk++;
	}
	if (cmp_8_checking()) {
		chk++;
	}

	report("%s", (chk == 4), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Logical_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * XOR, NOT.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41098_generic_processor_features_logical_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* XOR, NOT */
	if (xor_checking()) {
		chk++;
	}
	if (not_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Shift_Rotate_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * SHR.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41099_generic_processor_features_shift_rotate_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* SHR */
	if (shr_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Bit_Byte_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * BT, BTS, BTR, BTC, BSF, BSR, SETcc, TEST.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40348_generic_processor_features_bit_byte_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* BT, BTS, BTR, BTC, BSF, BSR, SETcc, TEST */
	if (bt_checking()) {
		chk++;
	}
	if (bts_checking()) {
		chk++;
	}
	if (btr_checking()) {
		chk++;
	}
	if (btc_checking()) {
		chk++;
	}
	if (bsf_checking()) {
		chk++;
	}
	if (bsr_checking()) {
		chk++;
	}
	if (setcc_checking()) {
		chk++;
	}
	if (test_checking()) {
		chk++;
	}

	report("%s", (chk == 8), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Unconditional_Control_Transfer_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * JMP, IRET.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41101_generic_processor_features_uncondition_control_transfer_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* JMP, IRET */
	if (jmp_checking()) {
		chk++;
	}

	cr0_am_to_0();
	if (iret_far_checking(test_iret, __FUNCTION__)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Condition_Control_Transfe_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * JNE, JO.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40349_generic_processor_features_condition_control_transfer_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* JNE, JO */
	if (jne_checking()) {
		chk++;
	}
	if (jo_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_String_Move_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * MOVS, STOS.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41103_generic_processor_features_string_move_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* MOVS, STOS */
	if (movs_checking()) {
		chk++;
	}
	if (stos_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Enter_Leave_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * LEAVE.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41104_generic_processor_features_enter_leave_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* LEAVE */
	if (leave_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Flag_Control_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * CLD.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41105_generic_processor_features_flag_control_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CLD */
	if (cld_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Segment_Register_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * MOV.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41941_generic_processor_features_segment_register_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* MOV */
	if (mov_segment_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Miscellaneous_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * LEA, CPUID, NOP.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40614_generic_processor_features_miscellaneous_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* LEA, CPUID, NOP */
	if (lea_checking()) {
		chk++;
	}
	if (cpuid_checking()) {
		chk++;
	}
	if (nop_checking()) {
		chk++;
	}

	report("%s", (chk == 3), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Output_Input_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * IN, OUT.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41178_generic_processor_features_output_input_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* OUT, IN */
	if (out_in_1_checking()) {
		chk++;
	}
	if (out_in_2_checking()) {
		chk++;
	}
	if (out_in_4_checking()) {
		chk++;
	}

	report("%s", (chk == 3), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Microcode_update_signature_001
 *
 * Summary: Under 64 bit mode on native board, read IA32_BIOS_SIGN_ID MSR should get
 * information of the microcode update signature successfully.
 */
static __unused void hsi_rqmid_35971_generic_processor_features_microcode_update_signature_001(void)
{
	u16 chk = 0;
	u64 msr = 0;

	/* enumerate CPUID before */
	cpuid(0);

	/* read IA32_BIOS_SIGN_ID */
	msr = rdmsr(IA32_BIOS_SIGN_ID);
	msr = msr >> 32;
	if (msr != 0) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Task_Management_001
 *
 * Summary: Under 64 bit mode on native board.
 * create TSS descriptor in GDT, execute LTR instruction on each physical processor.
 * to load the TSS structure into task register, the load operation is successful.
 */
static __unused void hsi_rqmid_42227_generic_processor_features_task_management_001(void)
{
	u16 chk = 0;

	atomic_set(&ret_sim, 0);
	atomic_set(&ap_sim, 0);

	/* test AP */
	start_run_id = 42227;

	/* test BP */
	test_bp();
	if (atomic_read(&ret_sim) == (USING_APS + 1)) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Msr_001
 *
 * Summary: Under 64 bit mode on native board.
 * read from and write to the TSC_MSR successfully.
 */
static __unused void hsi_rqmid_42228_generic_processor_features_msr_001(void)
{
	u16 chk = 0;

	if (msr_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Time_Stamp_Counter_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * RDTSC, RDTSCP.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_42229_generic_processor_features_time_stamp_counter_001(void)
{
	u16 chk = 0;

	if (rdtsc_checking()) {
		chk++;
	}
	if (rdtscp_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Clean_Cpu_Internal_Buffer_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * VERW.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_42230_generic_processor_features_clean_cpu_internal_buffer_001(void)
{
	u16 chk = 0;

	if (verw_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_XSAVE_X87_001
 *
 * Summary: Under 64 bit mode on native board, execute fpu instruction to
 * modify x87 data register ST0, execute XSAVES instruction, x87 state field
 * of XSAVE Area should be changed, clear st0 register, execute xrstors
 * instructions, the st0 register value should be restored from memory.
 */
static __unused void hsi_rqmid_35957_generic_processor_features_xsave_x87_001(void)
{
	u16 chk = 0;
	u16 op = 1;
	u32 st = 0;

	struct cpuid r;
	u32 r_eax;
	//u64 states = STATE_X87 | STATE_SSE | STATE_AVX;
	u64 states = STATE_X87;
	u32 i = 0;

	/* check XSAVE general support */
	if ((cpuid(1).c & CPUID_1_ECX_XSAVE) != 0) {
		chk++;
	}

	/* check XSAVE - x87, SSE, AVX - support */
	r = cpuid_indexed(0xd, 0);
	debug_print("eax 0x%x, ebx 0x%x, ecx 0x%x, edx 0x%x\n", r.a, r.b, r.c, r.d);
	r_eax = r.a;
	if ((r_eax & states) == states) {
		chk++;
	}

	/* set CR4.OSXSAVE to 1 */
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);

	/* xsetbv XCR0 to X87|SSE|AVX */
	if (xsetbv_checking(XCR0_MASK, states)) {
		chk++;
	}

	/* test using XSAVE state component to manage x87 state */
	execute_x87_test();
	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsaves_checking(&st_xsave_area, states)) {
		chk++;
	}

	/* check st0 is stored into x87 state component */
	for (i = XSAVE_AREA_ST0_POS; i < (XSAVE_AREA_ST0_POS + 10); i++) {
		if (st_xsave_area.fpu_sse[i] != 0) {
			chk++;
			debug_print("Test XSAVE manage x87 state pass.\n");
			break;
		}
	}

	/* clear st0 register */
	asm volatile(ASM_TRY("1f")
		"rex.w\n\t"
		"fild %0\n\t"
		"1:"
		: : "m" (st));

	if (xrstors_checking(&st_xsave_area, states)) {
		chk++;
	}
	asm volatile("fist %0\n" : : "m"(op) :);

	if (op != 0) {
		chk++;
	}

	report("%s", (chk == 7), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_XSAVE_SSE_001
 *
 * Summary: Under 64 bit mode on native board, execute SSE instruction
 * to modify SSE data register XMM0, execute XSAVE instruction, SSE state field
 * of XSAVE Area should be changed, clear XMM register, execute XRSTORS
 * instruction, the XMM register value shold be restored from memory.
 */
static __unused void hsi_rqmid_35960_generic_processor_features_xsave_sse_001(void)
{
	u32 chk = 0;

	struct cpuid r;
	u32 r_eax;
	u64 states = STATE_X87 | STATE_SSE | STATE_AVX;
	u32 i = 0;
	char *p = (char *)&unsigned_128;

	/* check XSAVE general support */
	if ((cpuid(1).c & CPUID_1_ECX_XSAVE) != 0) {
		chk++;
	}

	/* check XSAVE - x87, SSE, AVX - support */
	r = cpuid_indexed(0xd, 0);
	debug_print("eax 0x%x, ebx 0x%x, ecx 0x%x, edx 0x%x\n", r.a, r.b, r.c, r.d);
	r_eax = r.a;
	if ((r_eax & states) == states) {
		chk++;
	}

	/* set CR4.OSXSAVE to 1 */
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);

	/* xsetbv XCR0 to X87|SSE|AVX */
	if (xsetbv_checking(XCR0_MASK, states)) {
		chk++;
	}

	/* test using XSAVE state component to manage SSE state */
	execute_sse_test(1);
	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsaves_checking(&st_xsave_area, states)) {
		chk++;
	}

	/* check xmm0 is stored into SSE state component */
	for (i = XSAVE_AREA_XMM0_POS; i < (XSAVE_AREA_XMM0_POS + 16); i++) {
		if (st_xsave_area.fpu_sse[i] != 0) {
			chk++;
			debug_print("Test XSAVE manage SSE state pass.\n");
			break;
		}
	}

	/* clear XMM register */
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	asm("movdqu %[input_1], %%xmm0\n" : : [input_1] "m" (unsigned_128) : "memory");
	if (xrstors_checking(&st_xsave_area, states)) {
		chk++;
	}

	asm("movdqu %%xmm0, %[input_1]\n" : [input_1] "=m" (unsigned_128) : : "memory");
	for (i = 0; i < sizeof(unsigned_128); i++) {
		if (p[i] != 0) {
			chk++;
			break;
		}
	}

	report("%s", (chk == 7), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_XSAVE_AVX_001
 *
 * Summary: Under 64 bit mode on native board, execute AVX instruction
 * to modify AVX data register YMM0, execute XSAVE instruction, AVX state field
 * of XSAVE Area should be changed, clear YMM register, execute XRSTORS
 * instruction, the YMM register value should be restored from memory.
 */
static __unused void hsi_rqmid_35961_generic_processor_features_xsave_avx_001(void)
{
	u32 chk = 0;

	struct cpuid r;
	u32 r_eax;
	u64 states = STATE_X87 | STATE_SSE | STATE_AVX;
	u32 i = 0;
	char *p = (char *)&unsigned_256;

	/* check XSAVE general support */
	if ((cpuid(1).c & CPUID_1_ECX_XSAVE) != 0) {
		chk++;
	}

	/* check XSAVE - x87, SSE, AVX - support */
	r = cpuid_indexed(0xd, 0);
	debug_print("eax 0x%x, ebx 0x%x, ecx 0x%x, edx 0x%x\n", r.a, r.b, r.c, r.d);
	r_eax = r.a;
	if ((r_eax & states) == states) {
		chk++;
	}

	/* set CR4.OSXSAVE to 1 */
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);

	/* xsetbv XCR0 to X87|SSE|AVX */
	if (xsetbv_checking(XCR0_MASK, states)) {
		chk++;
	}

	/* test using XSAVE state component to manage AVX state */
	execute_avx_test();
	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsaves_checking(&st_xsave_area, states)) {
		chk++;
	}

	/* check ymm0 is stored into AVX state component */
	for (i = 0; i < 16; i++) {
		if (st_xsave_area.ymm[i] != 0) {
			chk++;
			debug_print("Test XSAVE manage AVX state pass.\n");
			break;
		}
	}

	/* clear YMM register */
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm("vmovdqu %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	if (xrstors_checking(&st_xsave_area, states)) {
		chk++;
	}


	asm("vmovdqu %%ymm0, %[input_1]\n" : [input_1] "=m" (unsigned_256) : : "memory");
	for (i = 0; i < sizeof(unsigned_256); i++) {
		if (p[i] != 0) {
			chk++;
			break;
		}
	}

	report("%s", (chk == 7), __FUNCTION__);
}

#elif __i386__
/*
 * @brief case name: HSI_Generic_Processor_Features_Shift_Rotate_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * SHL.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41100_generic_processor_features_shift_rotate_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/* SHL */
	if (shl_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}
/*
 * @brief case name: HSI_Generic_Processor_Features_Data_Move_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * CMOVAE, CMOVNE.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41106_generic_processor_features_data_move_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/*  CMOVAE, CMOVNE */
	if (cmovae_pro_checking()) {
		chk++;
	}
	if (cmovne_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Logical_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * OR.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41107_generic_processor_features_logical_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/* OR */
	if (or_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Flag_Control_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * CLI.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41179_generic_processor_features_flag_control_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/* CLI */
	if (cli_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Segment_Register_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * MOV.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_41180_generic_processor_features_segment_register_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/* MOV */
	if (mov_segment_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Uncondition_Control_Transfer_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * JMP, RET.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40615_generic_processor_features_uncondition_control_transfer_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/* JMP, RET */
	if (jmp_pro_checking()) {
		chk++;
	}
	if (ret_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Task_Management_002
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * STR.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_42231_generic_processor_features_task_management_002(void)
{
	u16 chk = 0;
	u16 val = 0;
	u16 desc_size = sizeof(tss32_t);
	tss32_t tss_intr;

	tss_intr.cr3 = read_cr3();
	tss_intr.ss0 = tss_intr.ss1 = tss_intr.ss2 = 0x10;
	tss_intr.esp = tss_intr.esp0 = tss_intr.esp1 = tss_intr.esp2 =
					(u32)intr_alt_stack + 4096;
	tss_intr.cs = 0x08;
	tss_intr.ds = tss_intr.es = tss_intr.fs = tss_intr.gs = tss_intr.ss = 0x10;
	tss_intr.iomap_base = (u16)desc_size;
	set_gdt_entry(MAIN_TSS_SEL, (u32)&tss_intr, desc_size - 1, 0x89, 0x0f);
	ltr(MAIN_TSS_SEL);
	asm volatile(ASM_TRY("1f")
		"str %%ax\n\t"
		"mov %%ax,%0\n\t"
		"1:"
		: "=m"(val)
	);
	if (val == MAIN_TSS_SEL) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Msr_001
 *
 * Summary: Under protect mode on native board.
 * read from and write to the TSC_MSR successfully.
 */
static __unused void hsi_rqmid_42232_generic_processor_features_msr_002(void)
{
	u16 chk = 0;

	if (msr_checking()) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

#endif

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t Demo test case list: \n\r");
#ifdef IN_NATIVE
#ifdef __x86_64__
	printf("\t Case ID: %d Case name: %s\n\r", 35956u, "HSI generic processor features " \
		"data move 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41095u, "HSI generic processor features " \
		"data exchange 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35959u, "HSI generic processor features " \
		"stack manipulation 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41096u, "HSI generic processor features " \
		"type conversion 001");
	printf("\t Case ID: %d Case name: %s\n\r", 40347u, "HSI generic processor features " \
		"binary arithmetic 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41097u, "HSI generic processor features " \
		"comparision 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41098u, "HSI generic processor features " \
		"logical 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41099u, "HSI generic processor features " \
		"shift rotate 001");
	printf("\t Case ID: %d Case name: %s\n\r", 40348u, "HSI generic processor features " \
		"bit byte 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41101u, "HSI generic processor features " \
		"uncondition control transfer 001");
	printf("\t Case ID: %d Case name: %s\n\r", 40349u, "HSI generic processor features " \
		"condition control transfer 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41103u, "HSI generic processor features " \
		"string move 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41104u, "HSI generic processor features " \
		"enter leave 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41105u, "HSI generic processor features " \
		"flag control 001");
	printf("\t Case ID: %d Case name: %s\n\r", 40614u, "HSI generic processor features " \
		"miscellaneous 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41178u, "HSI generic processor features " \
		"output input 001");
	printf("\t Case ID: %d Case name: %s\n\r", 41941u, "HSI generic processor features " \
		"segment register 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35971u, "HSI generic processor features " \
		"microcode update signature 001");
	printf("\t Case ID: %d Case name: %s\n\r", 42227u, "HSI generic processor features " \
		"task management 001");
	printf("\t Case ID: %d Case name: %s\n\r", 42228u, "HSI generic processor features " \
		"msr 001");
	printf("\t Case ID: %d Case name: %s\n\r", 42229u, "HSI generic processor features " \
		"time stamp counter 001");
	printf("\t Case ID: %d Case name: %s\n\r", 42230u, "HSI generic processor features " \
		"clean cpu internal buffer 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35957u, "HSI generic processor features " \
		"xsave x87 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35960u, "HSI generic processor features " \
		"xsave sse 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35961u, "HSI generic processor features " \
		"xsave avx 001");

#elif __i386__
	printf("\t Case ID: %d Case name: %s\n\r", 41100u, "HSI generic processor features " \
		"shift rotate 002");
	printf("\t Case ID: %d Case name: %s\n\r", 41106u, "HSI generic processor features " \
		"data move 002");
	printf("\t Case ID: %d Case name: %s\n\r", 41107u, "HSI generic processor features " \
		"logical 002");
	printf("\t Case ID: %d Case name: %s\n\r", 41179u, "HSI generic processor features " \
		"flag control 002");
	printf("\t Case ID: %d Case name: %s\n\r", 41180u, "HSI generic processor features " \
		"segment register 002");
	printf("\t Case ID: %d Case name: %s\n\r", 40615u, "HSI generic processor features " \
		"uncondition control transfer 002");
	printf("\t Case ID: %d Case name: %s\n\r", 42231u, "HSI generic processor features " \
		"task management 002");
	printf("\t Case ID: %d Case name: %s\n\r", 42232u, "HSI generic processor features " \
		"msr 002");
#endif
#endif
	printf("\t \n\r \n\r");
}

int ap_main(void)
{
#ifdef IN_NATIVE
#ifdef __x86_64__
	/***********************************/
	debug_print("ap %d starts running \n", get_lapic_id());
	while (start_run_id == 0) {
		test_delay(5);
	}

	/* ap test */
	while (start_run_id != 42227) {
		test_delay(5);
	}
	if (get_lapic_id() == 4 || get_lapic_id() == 2 || get_lapic_id() == 6) {
		test_ap(get_lapic_id());
	}


#endif
#endif
	return 0;
}

int main(void)
{
	//extern unsigned char kernel_entry;
	setup_idt();
	//set_idt_entry(0x20, &kernel_entry, 3);
	setup_vm();
	setup_ring_env();

	print_case_list();


#ifdef IN_NATIVE
#ifdef __x86_64__
	hsi_rqmid_35956_generic_processor_features_data_move_001();
	hsi_rqmid_41095_generic_processor_features_data_exchange_001();
	hsi_rqmid_35959_generic_processor_features_stack_manipulation_001();
	hsi_rqmid_41096_generic_processor_features_type_conversion_001();
	hsi_rqmid_40347_generic_processor_features_binary_arithmetic_001();
	hsi_rqmid_41097_generic_processor_features_comparision_001();
	hsi_rqmid_41098_generic_processor_features_logical_001();
	hsi_rqmid_41099_generic_processor_features_shift_rotate_001();
	hsi_rqmid_40348_generic_processor_features_bit_byte_001();
	hsi_rqmid_41101_generic_processor_features_uncondition_control_transfer_001();
	hsi_rqmid_40349_generic_processor_features_condition_control_transfer_001();
	hsi_rqmid_41103_generic_processor_features_string_move_001();
	hsi_rqmid_41104_generic_processor_features_enter_leave_001();
	hsi_rqmid_41105_generic_processor_features_flag_control_001();
	hsi_rqmid_40614_generic_processor_features_miscellaneous_001();
	hsi_rqmid_41178_generic_processor_features_output_input_001();
	hsi_rqmid_41941_generic_processor_features_segment_register_001();
	hsi_rqmid_35971_generic_processor_features_microcode_update_signature_001();
	hsi_rqmid_42227_generic_processor_features_task_management_001();
	hsi_rqmid_42228_generic_processor_features_msr_001();
	hsi_rqmid_42229_generic_processor_features_time_stamp_counter_001();
	hsi_rqmid_42230_generic_processor_features_clean_cpu_internal_buffer_001();
	hsi_rqmid_35957_generic_processor_features_xsave_x87_001();
	hsi_rqmid_35960_generic_processor_features_xsave_sse_001();
	hsi_rqmid_35961_generic_processor_features_xsave_avx_001();
#elif __i386__
	hsi_rqmid_41100_generic_processor_features_shift_rotate_002();
	hsi_rqmid_41106_generic_processor_features_data_move_002();
	hsi_rqmid_41107_generic_processor_features_logical_002();
	hsi_rqmid_41179_generic_processor_features_flag_control_002();
	hsi_rqmid_41180_generic_processor_features_segment_register_002();
	hsi_rqmid_40615_generic_processor_features_uncondition_control_transfer_002();
	hsi_rqmid_42231_generic_processor_features_task_management_002();
	hsi_rqmid_42232_generic_processor_features_msr_002();
#endif
#endif
	return report_summary();
}
