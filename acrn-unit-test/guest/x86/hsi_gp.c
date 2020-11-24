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

static int call_cnt = 0;
bool eflags_cf_to_1(void);

static void test_call_instr(void)
{
	call_cnt++;
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
	u32 op = 0;
	int data1 = 2;
	int data2 = 5;
	asm volatile(ASM_TRY("1f")
		"xchg %1, %2\n"
		"mov %2, %0\n"
		"1:"
		: "+r" (op)
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
	int data1 = 2;
	int data2 = 5;
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

/* execute instruction CALL to jump to another code block and return */
static bool call_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"call *%[fn]\n"
		"1:"
		: : [fn]"r"(test_call_instr)
	);
	return ((exception_vector() == NO_EXCEPTION) && (call_cnt == 1));
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

/* adding $0x3 to $0xff can make OF flag to 1 */
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

/* store the byte from register al to the memory which address is in register rdi */
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

static bool leave_checking(void)
{
	u64 op = 0;
	asm volatile(
	        "call 2f\n"
                "jmp 3f\n\t"
                "2: endbr64\n\t"
                "push   %%rbp\n"
                "mov    %%rsp,%%rbp\n\t"
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
	u32 op2 = 12;
	u32 op3 = 12;
	asm volatile(
		"mov $0, %%eax\n\t"
		ASM_TRY("1f")
		"cpuid\n\t"
		"mov %%ebx, %0\n\t"
		"mov %%edx, %1\n\t"
		"mov %%ecx, %2\n\t"
		"1:"
		: "=r" (op), "=r" (op2),"=r" (op3)
		:
		: "ebx", "ecx", "edx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x756E6547) && \
			(op2 ==  0x49656E69) && (op3 == 0x6C65746E));
}

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

static bool cmovae_pro_checking(void)
{
	u16 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $0xff12, %%bx\n\t"
		ASM_TRY("1f")
		/* CF = 0,It will move */
		"cmovae %%bx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff12));
}

static bool cmovne_pro_checking(void)
{
	u32 op = 0;
	asm volatile(
		"mov $0,  %0\n\t"
		"mov $0xff12, %%ebx\n\t"
		ASM_TRY("1f")
		/* ZF = 0,It will move */
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
		ASM_TRY("1f")
		"jmp 2f\n"
		"mov $0xf, %0\n"
		"2: \n"
		"1:"
		: "=r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 8));
}

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

/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * CMOVNC, MOV, XADD, XCHG, MOVSX, MOVZX, CDQE.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35956_generic_processor_features_general_purpose_instructions_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMOVNC, MOV, XADD, XCHG, MOVSX, MOVZX, CDQE */
	if (cmovnc_8_checking()) {
		chk++;
	}

	if (cmovnc_8_ncf_checking()) {
		chk++;
	}

	if (mov_checking()) {
		chk++;
	}
	if (xadd_checking()) {
		chk++;
	}
	if (xchg_checking()) {
		chk++;
	}
	if (movsx_checking()) {
		chk++;
	}
	if (movzx_checking()) {
		chk++;
	}
	if (cdqe_checking()) {
		chk++;
	}

	report("%s", (chk == 8), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_002
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * PUSH, POP, PUSHF, POPF.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35959_generic_processor_features_general_purpose_instructions_002(void)
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
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_003
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * ADD, SUB, NEG, INC, DEC, MUL, DIV.
 * execution results are all correct and no exception occurs.
 */

static __unused void hsi_rqmid_40347_generic_processor_features_general_purpose_instructions_003(void)
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
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_004
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * CMP, XOR, NOT, SHR, BT, BTS, BTR, BTC, BSF, BSR, SETcc, TEST.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40348_generic_processor_features_general_purpose_instructions_004(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMP, XOR, NOT, SHR, BT, BTS, BTR, BTC, BSF, BSR, SETcc, TEST */
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
	if (xor_checking()) {
		chk++;
	}
	if (not_checking()) {
		chk++;
	}
	if (shr_checking()) {
		chk++;
	}
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
	report("%s", (chk == 15), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_005
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * JMP, CALL, JNE, JO.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40349_generic_processor_features_general_purpose_instructions_005(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* JMP, CALL, JNE, JO */
	if (jmp_checking()) {
		chk++;
	}
	if (call_checking()) {
		chk++;
	}
	if (jne_checking()) {
		chk++;
	}
	if (jo_checking()) {
		chk++;
	}

	report("%s", (chk == 4), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_006
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * MOVS, STOS, LEAVE, CLD, LEA, CPUID, NOP.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40614_generic_processor_features_general_purpose_instructions_006(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* MOVS, STOS, LEAVE, CLD, LEA, CPUID, NOP */
	if (movs_checking()) {
		chk++;
	}
	if (stos_checking()) {
		chk++;
	}
	if (leave_checking()) {
		chk++;
	}
	if (cld_checking()) {
		chk++;
	}
	if (lea_checking()) {
		chk++;
	}
	if (cpuid_checking()) {
		chk++;
	}
	if (nop_checking()) {
		chk++;
	}

	report("%s", (chk == 7), __FUNCTION__);
}

/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_007
 *
 * Summary: Under protect mode on native board, execute following instructions:
 * CMOVAE, CMOVNE, OR, SHL, JMP, RET.
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_40615_generic_processor_features_general_purpose_instructions_007(void)
{
	u16 chk = 0;

	/* execute the following instruction in protect mode */
	/*  CMOVAE, CMOVNE, OR, SHL, JMP, RET */
	if (cmovae_pro_checking()) {
		chk++;
	}
	if (cmovne_pro_checking()) {
		chk++;
	}
	if (or_pro_checking()) {
		chk++;
	}
	if (shl_pro_checking()) {
		chk++;
	}
	if (jmp_pro_checking()) {
		chk++;
	}
	if (ret_pro_checking()) {
		chk++;
	}

	report("%s", (chk == 6), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t Demo test case list: \n\r");
#ifdef IN_NATIVE
#ifdef __x86_64__
	printf("\t Case ID: %d Case name: %s\n\r", 35956u, "HSI generic processor features " \
		"general purpose instructions 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35959u, "HSI generic processor features " \
		"general purpose instructions 002");
	printf("\t Case ID: %d Case name: %s\n\r", 40347u, "HSI generic processor features " \
		"general purpose instructions 003");
	printf("\t Case ID: %d Case name: %s\n\r", 40348u, "HSI generic processor features " \
		"general purpose instructions 004");
	printf("\t Case ID: %d Case name: %s\n\r", 40349u, "HSI generic processor features " \
		"general purpose instructions 005");
	printf("\t Case ID: %d Case name: %s\n\r", 40614u, "HSI generic processor features " \
		"general purpose instructions 006");

#endif
#ifdef __i386__
	printf("\t Case ID: %d Case name: %s\n\r", 40615u, "HSI generic processor features " \
		"general purpose instructions 007");
#endif
#endif
	printf("\t \n\r \n\r");
}

int main(void)
{
	//extern unsigned char kernel_entry;
	setup_idt();
	//set_idt_entry(0x20, &kernel_entry, 3);
	setup_vm();

	print_case_list();


#ifdef IN_NATIVE
#ifdef __x86_64__
	hsi_rqmid_35956_generic_processor_features_general_purpose_instructions_001();
	hsi_rqmid_35959_generic_processor_features_general_purpose_instructions_002();
	hsi_rqmid_40347_generic_processor_features_general_purpose_instructions_003();
	hsi_rqmid_40348_generic_processor_features_general_purpose_instructions_004();
	hsi_rqmid_40349_generic_processor_features_general_purpose_instructions_005();
	hsi_rqmid_40614_generic_processor_features_general_purpose_instructions_006();
#elif __i386__
	hsi_rqmid_40615_generic_processor_features_general_purpose_instructions_007();
#endif
#endif
	return report_summary();
}
