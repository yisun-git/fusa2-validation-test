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

#define CPUID_1_ECX_MONITOR		(1U << 3)
#define CPUID_5_ECX_MONITOR_EXTEN	(1U << 0)
#define IA32_MONITOR_FILTER_SIZE	0x00000006UL

#define IA32_MISC_ENABLE_STARTUP_ADDR	(0x6000)
#define IA32_MISC_ENABLE_INIT1_ADDR	(0x8000)
#define ENABLE_MONITOR_FSM_BIT		(1U << 18)

typedef struct gen_reg_dump {
	u64 rax, rbx, rcx, rdx;
	u64 rsi, rdi, rsp, rbp;
	u64 rflags;
#ifdef __x86_64__
	u64 r8, r9, r10, r11;
	u64 r12, r13, r14, r15;
#endif
	u64 cr0, cr2, cr3, cr4, cr8;
} gen_reg_dump_t;

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
	ring3_ret = (ret == UD_VECTOR);
}

static void test_hlt_exception(const char *msg)
{
	int ret = 0;
	/*for ring3*/
	ret = hlt_checking();
	ring3_ret = (ret == GP_VECTOR) && (exception_error_code() == 0);
}

/**
 * @brief Case name: IDLE hide MONITOR/MWAIT extensions support_002
 * Summary: If the current privilege level is 3, execute MONITOR, it should
 *        cause #UD exception.
 */

static void idle_rqmid_24306_test_hide_monitor_extensions_support_002(void)
{
	printf("%s test...\n", __func__);
	do_at_ring3(test_hide_monitor, "");

	report("%s", ring3_ret, __FUNCTION__);
	ring3_ret = false;
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

/**
 * @brief Case name: IDLE IA32_MONITOR_FILTER_SIZE ignore access_001
 * Summary: Write a valid and a invalid value to the IA32_MONITOR_FILTER_SIZE
 *        register in 64bit mode, ACRN hypervisor shall ignore the access.
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
/*
 *save register
 *
 *
 */
static  inline __attribute__((always_inline)) void save_gen_reg(void *ptr)
{
	gen_reg_dump_t *reg_dump;
	ulong temp;

	reg_dump = (gen_reg_dump_t *)ptr;
	reg_dump->rflags = read_rflags();

	asm volatile ("mov %%" R "ax," "%0" : "=m"(reg_dump->rax):: "memory");
	asm volatile ("mov %%" R "bx," "%0" : "=m"(reg_dump->rbx):: "memory");
	asm volatile ("mov %%" R "cx," "%0" : "=m"(reg_dump->rcx):: "memory");
	asm volatile ("mov %%" R "dx," "%0" : "=m"(reg_dump->rdx):: "memory");
	asm volatile ("mov %%" R "si," "%0" : "=m"(reg_dump->rsi):: "memory");
	asm volatile ("mov %%" R "di," "%0" : "=m"(reg_dump->rdi):: "memory");
	asm volatile ("mov %%" R "sp," "%0" : "=m"(reg_dump->rsp):: "memory");
	asm volatile ("mov %%" R "bp," "%0" : "=m"(reg_dump->rbp):: "memory");
#ifdef __x86_64__
	asm volatile ("mov %%r8, %0" : "=m"(reg_dump->r8):: "memory");
	asm volatile ("mov %%r9, %0" : "=m"(reg_dump->r9):: "memory");
	asm volatile ("mov %%r10," "%0" : "=m"(reg_dump->r10):: "memory");
	asm volatile ("mov %%r11," "%0" : "=m"(reg_dump->r11):: "memory");
	asm volatile ("mov %%r12," "%0" : "=m"(reg_dump->r12):: "memory");
	asm volatile ("mov %%r13," "%0" : "=m"(reg_dump->r13):: "memory");
	asm volatile ("mov %%r14," "%0" : "=m"(reg_dump->r14):: "memory");
	asm volatile ("mov %%r15," "%0" : "=m"(reg_dump->r15):: "memory");
#endif
	asm volatile ("mov %%" R "ax," "%0":"=m"(temp) : : "memory");

	asm volatile ("mov %%cr0,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr0):: "memory");
	// asm volatile ("mov %%cr1,%%" R "ax \n" /*read cr1 will occur error*/
	//               "mov %%" R "ax," "%0"
	//               :"=m"(reg_dump->cr1)::"memory");
	asm volatile ("mov %%cr2,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr2):: "memory");
	asm volatile ("mov %%cr3,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr3):: "memory");
	asm volatile ("mov %%cr4,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr4):: "memory");
	asm volatile ("mov %%cr8,%%" R "ax \n"
				  "mov %%" R "ax," "%0"
				  : "=m"(reg_dump->cr8):: "memory");

	asm volatile ("mov %0, %%" R "ax" : : "m"(temp) : "memory");

}
/**
 * @brief Case name: IDLE_expose_PAUSE_support_001
 * Summary: In the 64bit mode, this instruction does not change the architectural state of the
 *        processor before excute PAUSE instruction and after excute PAUSE instruction.
 */
static void idle_rqmid_24318_expose_pause_support_001(void)
{
	int ret = 0;
	gen_reg_dump_t gen_reg, gen_reg1;

	/*dump registers*/
	save_gen_reg(&gen_reg);
	pause();
	save_gen_reg(&gen_reg1);

	ret = memcmp(&gen_reg, &gen_reg1, sizeof(gen_reg_dump_t));
	report("%s", ret == 0, __FUNCTION__);
}
/**
 * @brief Case name: IDLE expose HLT support_003
 * Summary: When the current privilege level is 3, excute HLT
 *        cause #GP exception.
 */
static void idle_rqmid_24321_expose_hlt_support_003(void)
{
	/*expect #GP*/
	do_at_ring3(test_hlt_exception, "");
	report("%s", ring3_ret, __FUNCTION__);
	ring3_ret = false;
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
 * @brief case name: IDLE IA32_MISC_ENABLE initial state following start up 001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 18] at BP start-up, the result shall be 0H.
 */
static void idle_rqmid_24185_ia32_misc_enable_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)IA32_MISC_ENABLE_STARTUP_ADDR;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);

	report("%s", (ia32_init_first & ENABLE_MONITOR_FSM_BIT) == 0UL,
		   __FUNCTION__);
}

/**
 * @brief case name: IDLE IA32_MISC_ENABLE initial state following start-up_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 18] at AP entry, the result shall be 0H.
 */
static void idle_rqmid_24111_ia32_misc_enable_following_init_001(void)
{
	volatile u32 *ptr = (volatile u32 *)IA32_MISC_ENABLE_INIT1_ADDR;
	u64 ia32_feature_control;

	ia32_feature_control = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", (ia32_feature_control & ENABLE_MONITOR_FSM_BIT) == 0UL, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("idle && block feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 24109, "IDLE IA32_MONITOR_FILTER_SIZE ignore access_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24111, "IDLE IA32_MISC_ENABLE initial state following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24185, "IDLE IA32_MISC_ENABLE initial state following start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24190, "IDLE hide CPUID leaf 05H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24275, "IDLE IA32_MISC_ENABLE disable MONITOR_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24318, "IDLE expose PAUSE support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24306, "IDLE hide MONITOR/MWAIT extensions support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 24321, "IDLE expose HLT support_003");
}
static void test_idbl(void)
{
	idle_rqmid_24109_test_filter_size_ignore_access_001();

	idle_rqmid_24111_ia32_misc_enable_following_init_001();
	idle_rqmid_24185_ia32_misc_enable_following_start_up_001();

	idle_rqmid_24190_test_hide_cpuid_leaf_05H_001();
	idle_rqmid_24275_test_disable_monitor_001();
	idle_rqmid_24306_test_hide_monitor_extensions_support_002();
	idle_rqmid_24318_expose_pause_support_001();
	idle_rqmid_24321_expose_hlt_support_003();
}

int main(void)
{
	extern unsigned char kernel_entry;

	setup_idt();
	set_idt_entry(0x23, &kernel_entry, 3);

	print_case_list();
	test_idbl();

	return report_summary();
}
