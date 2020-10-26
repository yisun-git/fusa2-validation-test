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
 
#define ADD_SIGN ".byte 0x48, 0x83, 0xc0, 0x80\n\t"               // add $0x80, %%rax
#define SUB_SIGN ".byte 0x48, 0x2d, 0x00, 0x00, 0x00, 0x80\n\t"   // sub $0x80000000, %%rax

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
		"mov $10, %%rbx\n\t"
		ASM_TRY("1f")
		"cmovnc %%rbx, %0\n"
		"1:"
		: "=r" (op)
		:
		:"rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 10));
}

static bool cmovnc_8_ncf_checking(void)
{
	u64 op = 0;
	asm volatile(
	        "mov $10, %%rbx\n\t"
                "mov $0,  %0\n\t"
                "mov $0xff, %%al\n\t"
                "add $10, %%al\n\t"
		ASM_TRY("1f")
		"cmovnc %%rbx, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rbx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0));
}

static bool movl_checking(void)
{
	u32 op = 0;
	asm volatile(ASM_TRY("1f")
		"movl $10, %%eax\n"
		"movl %%eax, %0\n"
		"1:"
		: "=r" (op)
		:
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 10));
}

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

static bool sub_checking(void)
{
	u64 op = 0;
	asm volatile(
	        "mov $0, %%rax\n\t"
		ASM_TRY("1f")
		SUB_SIGN
		"mov %%rax, %0\n"
		"1:"
		: "=r" (op)
		: 
		: "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x80000000));
}

static bool neg_checking(void)
{
	u8 op = 1;
	asm volatile(ASM_TRY("1f")
		"neg %0\n\t"
		"1:"
		: "+r" (op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xff));
}

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

static bool cmp_checking(void)
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

static bool shr_checking(void)
{
	u32 op = 0x100;
	asm volatile(ASM_TRY("1f")
		"shr %0\n\t"
		"1:"
		: "+r"(op)
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x80));
}

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

static bool bsf_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x1234, %%rax\n"
		ASM_TRY("1f")
		"bsf %%rax, %0\n\t"
		"jmp 2f\n"
		"mov $0x0, %0\n"
		"2: \n"
		"1:"
		: "=r"(op) : : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0x2));
}

static bool bsr_checking(void)
{
	u64 op = 0;
	asm volatile(
		"mov $0x1234, %%rax\n"
		ASM_TRY("1f")
		"bsr %%rax, %0\n\t"
		"jmp 2f\n"
		"mov $0x0, %0\n"
		"2: \n"
		"1:"
		: "=r"(op) : : "rax"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 0xc));
}

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

static bool call_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"call *%[fn]\n"
		"1:"
		: : [fn]"r"(test_call_instr)
	);
	return ((exception_vector() == NO_EXCEPTION) && (call_cnt == 1));
}
/*
static bool loop_checking(void)
{
	u16 op = 10;
	asm volatile(
		"mov $10, %%cx\n"
		"2: inc %0\n"
		ASM_TRY("1f")
		"loop 2b\n"
		"1:"
		: "+r"(op) 
		: : "cx"
	);
	return ((exception_vector() == NO_EXCEPTION) && (op == 20));
}
*/
/*
 * @brief case name: HSI_Generic_Processor_Features_General_Purpose_Instructions_001
 *
 * Summary: Under 64 bit mode on native board, execute following instructions:
 * MOV, ADD, INC, DEC, MUL, DIV,
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35956_generic_processor_features_general_purpose_instructions_001(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* MOV, ADD, INC, DEC, MUL, DIV */
	if (cmovnc_8_checking()) {
		chk++;
	}
	
	if (cmovnc_8_ncf_checking()) {
		chk++;
	}

	if (movl_checking()) {
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
 * CMP, PUSH, POP, AND, OR, JMP, CALL, LOOP,
 * execution results are all correct and no exception occurs.
 */
static __unused void hsi_rqmid_35959_generic_processor_features_general_purpose_instructions_002(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMP, PUSH, POP, AND, OR, JMP, CALL, LOOP */
	if (push_pop_checking()) {
		chk++;
	}
	if (pushf_popf_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

static __unused void hsi_rqmid_35962_generic_processor_features_general_purpose_instructions_003(void)
{
	u16 chk = 0;

	/* execute the following instruction in IA-32e mode */
	/* CMP, PUSH, POP, AND, OR, JMP, CALL, LOOP */
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

static __unused void hsi_rqmid_35965_generic_processor_features_general_purpose_instructions_004(void)
{
	u16 chk = 0;

	if (cmp_checking()) {
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
	report("%s", (chk == 12), __FUNCTION__);
}

static __unused void hsi_rqmid_35969_generic_processor_features_general_purpose_instructions_005(void)
{
	u16 chk = 0;

	if (jmp_checking()) {
		chk++;
	}
	if (call_checking()) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t Demo test case list: \n\r");
#ifdef IN_NATIVE
	printf("\t Case ID: %d Case name: %s\n\r", 35956u, "HSI generic processor features " \
		"general purpose instructions 001");
	printf("\t Case ID: %d Case name: %s\n\r", 35959u, "HSI generic processor features " \
		"general purpose instructions 002");
	printf("\t Case ID: %d Case name: %s\n\r", 35962u, "HSI generic processor features " \
		"general purpose instructions 003");
	printf("\t Case ID: %d Case name: %s\n\r", 35965u, "HSI generic processor features " \
		"general purpose instructions 004");
	printf("\t Case ID: %d Case name: %s\n\r", 35969u, "HSI generic processor features " \
		"general purpose instructions 005");
#else
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
	hsi_rqmid_35962_generic_processor_features_general_purpose_instructions_003();
	hsi_rqmid_35965_generic_processor_features_general_purpose_instructions_004();
	hsi_rqmid_35969_generic_processor_features_general_purpose_instructions_005();

#elif __i386__

	hsi_rqmid_35961_generic_processor_features_general_purpose_instructions_003();
#endif
#else
#endif
	return report_summary();
}
