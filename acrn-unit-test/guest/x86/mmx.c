#include "vm.h"
#include "libcflat.h"
#include "desc.h"
#include "types.h"
#include "processor.h"
#include "alloc.h"
#include "asm/page.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "misc.h"
#include "instruction_common.h"

#define CPUID_1_EDX_MMX_SUPPROT	(1UL << 23)

#define RING1_CS32_GDT_DESC (0x00cfbb000000ffffULL)
#define RING1_CS64_GDT_DESC	(0x00afbb000000ffffULL)
#define RING1_DS_GDT_DESC	(0x00cfb3000000ffffULL)

#define RING2_CS32_GDT_DESC (0x00cfdb000000ffffULL)
#define RING2_CS64_GDT_DESC (0x00afdb000000ffffULL)
#define RING2_DS_GDT_DESC	(0x00cfd3000000ffffULL)

int do_at_ring1(void (*fn)(void), const char *arg)
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
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry1: \n\t"
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
				  : [user_ds] "i" (OSSERVISE1_DS),
				  [user_cs] "i" (OSSERVISE1_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x21)
				  : "rcx", "rdx");
	return ret;
}

int do_at_ring2(void (*fn)(void), const char *arg)
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
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry2: \n\t"
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
				  : [user_ds] "i" (OSSERVISE2_DS),
				  [user_cs] "i" (OSSERVISE2_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x22)
				  : "rcx", "rdx");
	return ret;
}

volatile bool ret_ring3 = false;
int do_at_ring3(void (*fn)(void), const char *arg)
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
				  "iret" W "\n"
				  "1: \n\t"
				  /* save kernel SP */
				  "push %%" R "cx\n\t"

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
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
				  [kernel_entry_vector]"i"(0x20)
				  : "rcx", "rdx");
	return ret;
}
#ifdef __x86_64__
/*
 * @brief case name: General support_64 bit Mode_PADDSW_#PF_001
 *
 *	Summary:
 *   ACRN hypervisor shall expose MMX Capability existence to any VM,in compliance with Chapter 9.6, Vol. 1, SDM.
 *	 Under 64 bit Mode , Rebulid the paging structure, to create the page fault(pgfault: occur),
 *	executing PADDSW shall generate #PF .
 */

static void mmx_rqmid_30966_acrn_general_support_64bit_mode_paddsw_pf_001(void)
{
	int add2 = -8;
	int *add1;

	add1 = malloc(sizeof(int));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);

	asm volatile("movq %1, %%mm0\n"
				 ASM_TRY("1f")
				 "paddsw %0, %%mm0\n"
				 "1:"
				 : : "m"(*add1), "m"(add2)
				);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);
}
/*
 * @brief case name: General support_64 bit Mode_MOVD_#GP_002
 *
 *Summary:
 *	 ACRN hypervisor shall expose MMX Capability existence to any VM, in compliance with Chapter 9.6, Vol. 1, SDM.
 *	 Under 64 bit Mode and CPL1 , The address of memory is Non-canonical(Ad. Cann.: non mem),
 *	 executing MOVD shall generate #GP .
 */
static void  mmx_rqmid_31041_acrn_general_support_64bit_mode_movd_gp_002(void)
{
	int *op1, op2 = -8;
	int level;
	bool ret;

	op1 = malloc(sizeof(int));
	/*clear any of high 16bit to make Non-canonical address*/
	op1 = (int *)((u64)op1 & ~(3Ul << 60));
	op1 = (int *)((u64)op1 | (1Ul << 60));/*to make sure the address is Non-canonical*/

	asm volatile(ASM_TRY("1f")
				 "movd %1, %0\n"
				 "1:"
				 : "=m"(*op1)
				 : "y"(op2));
	ret = (exception_vector() == GP_VECTOR) && (exception_error_code() == 0);

	level = read_cs() & 0x3;
	report("%s", (ret == true) && (level == 1), __FUNCTION__);
}
/*
 * @brief case name: General support_64 bit Mode_PMULHW_#NM_004
 *
 *Summary:
 *	 ACRN hypervisor shall expose MMX Capability existence to any VM, in compliance with Chapter 9.6, Vol. 1, SDM.
 *	 Under 64 bit Mode and CPL3 , Sets Task Switched flag on every task switch
 *	 and tests it when executing x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions(CR0.TS: 1),
 *	 executing PMULHW shall generate #NM .
 */
static void  mmx_rqmid_31066_acrn_general_support_64bit_mode_pmulhw_nm_004(void)
{
	int op1 = -1;
	int level;

	asm volatile(ASM_TRY("1f")
				 "pmulhw %0, %%mm0\n"
				 "1:"
				 ::"m"(op1));

	level = read_cs() & 0x3;
	report("%s", ((exception_vector() == NM_VECTOR)) && (level == 3), __FUNCTION__);
}
/*
 * @brief case name: General support_64 bit Mode_MOVQ_UD_003
 *
 *Summary: Under 64 bit Mode and CPL2 ,
 *	Set Emulation flag(CR0.EM: 1),
 *	executing MOVQ shall generate #UD .
 *
 */
static void mmx_rqmid_31185_acrn_general_support_64bit_mode_movq_ud_003(void)
{
	int op1 = -8;
	int level;

	asm volatile(ASM_TRY("1f")
				 "movq %0, %%mm0\n"
				 "1:"
				 ::"m"(op1));

	level = read_cs() & 0x3;
	report("%s", (exception_vector() == UD_VECTOR) && (level == 2),  __FUNCTION__);
}
/*
 * @brief case name: General support_64 Bit Mode_EMMS_MF_001
 *
 * Summary: Under Protected/Real Address/64 Bit Mode,
 *	FPU exception(FPU excp: hold),executing EMMS shall generate #MF.
 *
 */
static void mmx_rqmid_31859_acrn_general_support_64bit_mode_emms_mf_001(void)
{

	float op1 = 1.2;
	float op2 = 0;
	unsigned short cw = 0x37b;

	write_cr0(read_cr0() & ~0x8); /*clear TS*/
	write_cr0(read_cr0() | (1 << 5));/*set cr0.ne*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 ASM_TRY("1f")
				 "emms\n"
				 "1:"
				 :: "m"(cw), "m"(op1), "m"(op2));

	report("%s", (exception_vector() == MF_VECTOR),  __FUNCTION__);
}
#endif

#ifdef __i386__
/*
 * @brief case name: General support_Protected Mode_MOVQ_#PF_001
 *
 *Summary:
 *   ACRN hypervisor shall expose MMX Capability existence to any VM, in compliance with Chapter 9.6, Vol. 1, SDM.
 *	 Under Protected Mode, Rebulid the paging structure, to create the page fault(pgfault: occur),
 *	 executing MOVQ shall generate #PF
 */
static void mmx_rqmid_31094_acrn_general_support_protected_mode_movq_pf_001(void)
{
	int *op1;

	op1 = malloc(sizeof(int));
	set_page_control_bit((void *)op1, PAGE_PTE, PAGE_P_FLAG, 0, true);

	asm volatile(ASM_TRY("1f")
				 "movq %0, %%mm0\n"
				 "1:"
				 ::"m"(*op1));

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);
}
static void  mmx_movq_ac_at_ring3()
{
	int *op1;
	u8 level;

	op1 = malloc(sizeof(u64) + 3);
	op1 = (int *)((u8 *)(op1) + 1);

	asm volatile(ASM_TRY("1f")
				 "movq %0, %%mm0\n"
				 "1:"
				 ::"m"(*op1));

	level = read_cs() & 0x3;
	ret_ring3 = (exception_vector() == AC_VECTOR) && (level == 3);
}
/*
 * @brief case name: General support_Protected Mode_MOVQ_#AC_001
 *
 *Summary:
 *  ACRN hypervisor shall expose MMX Capability existence to any VM, in compliance with Chapter 9.6, Vol. 1, SDM.
 *	Under Protected Mode and CPL3 , Memory address is Non-alignment(Alignment: unaligned, CR0.AC: 1, CS.CPL: 3),
 *	executing MOVQ shall generate #AC .
 */
static void mmx_rqmid_31096_acrn_general_support_protected_mode_movq_ac_001(void)
{
	u32 eflags;
	ulong cr0;

	/*enable eflags AC check eflags[bit18]*/
	asm volatile(
		"pushf\n\t"
		"pop %%eax\n\t"
		"mov %%eax,%0 \n\t"
		"or $(1<<18),%%eax\n\t"
		"push %%eax\n\t"
		"popf\n\t"
		: "=m"(eflags)
	);

	/*enable cr0.AM*/
	cr0 = read_cr0();
	write_cr0(cr0 | (1 << 18));

	do_at_ring3(mmx_movq_ac_at_ring3, "");

	/*restore flags back*/
	asm volatile(
		"mov %0, %%eax \n\t"
		"push %%eax \n\t"
		"popf \n\t"
		:: "m"(eflags)
	);

	report("%s", ret_ring3, __FUNCTION__);
	write_cr0(cr0);
}
/*
 * @brief case name: General support_Protected Mode_MOVD_#NM_002
 *
 *Summary:
 *	ACRN hypervisor shall expose MMX Capability existence to any VM, in compliance with Chapter 9.6, Vol. 1, SDM.
 *	Under Protected Mode and CPL1 ,
 *	Sets Task Switched flag on every task switch and
 *	tests it when executing x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions(CR0.TS: 1),
 *	executing MOVD shall generate #NM .
 */
static void mmx_rqmid_31117_acrn_general_support_protected_mode_movd_nm_001(void)
{
	int op1 = -8;
	int level;

	level = read_cs() & 0x3;

	asm volatile(ASM_TRY("1f")
				 "movd %0, %%mm0\n"
				 "1:"
				 :: "m"(op1));
	report("%s", (exception_vector() == NM_VECTOR) && (level == 1), __FUNCTION__);

}
#endif
/*
 *	@brief setup_ring
 *	Summary:setup ring environment:gdt entry and idt
 *		which will be used for CPU  switching to ring1, ring2,ring3
 */
void setup_ring_env()
{
	/*for setup ring1 ring2 ring3 environment*/
	extern unsigned char kernel_entry;
	set_idt_entry(0x20, &kernel_entry, 3);
	extern unsigned char kernel_entry1;
	set_idt_entry(0x21, &kernel_entry1, 1);
	extern unsigned char kernel_entry2;
	set_idt_entry(0x22, &kernel_entry2, 2);

#ifdef __x86_64__
	extern gdt_entry_t gdt64[];
	*(u64 *)&gdt64[11] = RING1_CS64_GDT_DESC;
	*(u64 *)&gdt64[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt64[13] = RING2_CS64_GDT_DESC;
	*(u64 *)&gdt64[14] = RING2_DS_GDT_DESC;
#elif __i386__
	extern gdt_entry_t gdt32[];
	*(u64 *)&gdt32[11] = RING1_CS32_GDT_DESC;
	*(u64 *)&gdt32[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt32[13] = RING2_CS32_GDT_DESC;
	*(u64 *)&gdt32[14] = RING2_DS_GDT_DESC;
#endif

}

int main()
{
	setup_vm();
	setup_idt();
	setup_ring_env();

	if (!(cpuid(1).d & CPUID_1_EDX_MMX_SUPPROT)) {
		report_skip("%s", "MMX feature not supported\n\r");
		return -1;
	}

	write_cr0(read_cr0() & ~0x4);/*clear EM*/

#ifdef __x86_64__
	mmx_rqmid_30966_acrn_general_support_64bit_mode_paddsw_pf_001();
	do_at_ring1(mmx_rqmid_31041_acrn_general_support_64bit_mode_movd_gp_002, "");

	write_cr0(read_cr0() | 0x8);/*set TS*/
	do_at_ring3(mmx_rqmid_31066_acrn_general_support_64bit_mode_pmulhw_nm_004, "");
	write_cr0(read_cr0() & ~0x8);/*clear TS to restore environment*/

	write_cr0(read_cr0() | 0x4); /*set EM*/
	do_at_ring2(mmx_rqmid_31185_acrn_general_support_64bit_mode_movq_ud_003, "");
	write_cr0(read_cr0() & ~0x4);/*clear EM*/

	mmx_rqmid_31859_acrn_general_support_64bit_mode_emms_mf_001();
#elif __i386__
	mmx_rqmid_31094_acrn_general_support_protected_mode_movq_pf_001();
	mmx_rqmid_31096_acrn_general_support_protected_mode_movq_ac_001();

	write_cr0(read_cr0() | 0x8);/*set TS*/
	do_at_ring1(mmx_rqmid_31117_acrn_general_support_protected_mode_movd_nm_001, "");
	write_cr0(read_cr0() & ~0x8);/*clear TS*/
#endif
	return report_summary();
}

