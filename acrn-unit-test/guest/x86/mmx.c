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
static void  mmx_rqmid_31041_acrn_general_support_64bit_mode_movd_gp_002(const char *msg)
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
static void  mmx_rqmid_31066_acrn_general_support_64bit_mode_pmulhw_nm_004(const char *msg)
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
static void mmx_rqmid_31185_acrn_general_support_64bit_mode_movq_ud_003(const char *msg)
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
	asm volatile("fninit");
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
static void  mmx_movq_ac_at_ring3(const char *msg)
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
	ring3_ret = (exception_vector() == AC_VECTOR) && (level == 3);
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

	report("%s", ring3_ret, __FUNCTION__);
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
static void mmx_rqmid_31117_acrn_general_support_protected_mode_movd_nm_001(const char *msg)
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

