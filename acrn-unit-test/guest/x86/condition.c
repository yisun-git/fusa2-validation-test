#include "libcflat.h"
#include "desc.h"
#include "apic.h"
#include "processor.h"
#include "atomic.h"
#include "asm/barrier.h"
#include "asm/spinlock.h"
#include "condition.h"
#include "instruction_common.h"
#include "instruction_common.c"
#include "interrupt.h"
#include "segmentation.h"
#include "mpx.h"

/* commmon function */

void condition_CPUID_AVX2_1(void)
{
	unsigned long check_bit = 0;

	//printf("****** Check CPUID.(EAX=07H,ECX=0):EBX[bit 5] **********\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_07, EXTENDED_STATE_SUBLEAF_0).b;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05);
}

void condition_CPUID_F16C_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Check CPUID.(EAX=01H):ECX[bit 29] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29);
}

void condition_CPUID_FMA_1(void)
{
	unsigned long check_bit = 0;
	//printf("***** Check CPUID.(EAX=01H):ECX[bit 12] *****\n");

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12);
}

void condition_CPUID_MMX_1(void)
{
	unsigned long check_bit = 0;
	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_23);
}

void condition_LOCK_not_used(void)
{
}

void condition_VEX_used(void)
{
}

bool condition_st_comp_enabled(void)
{
	u64 value = STATE_X87 | STATE_SSE | STATE_AVX;
	u32 eax = value;
	u32 edx = value >> 32;
	u32 index = 0;

	asm volatile(ASM_TRY("1f")
				"xsetbv\n\t" /* xsetbv */
				"1:"
				: : "a" (eax), "d" (edx), "c" (index));
	return exception_vector();
}

int condition_write_cr4_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
				"mov %0,%%cr4\n\t" "1:"
				: : "r" (val));
	return exception_vector();
}

bool condition_CR4_OSXSAVE_1(void)
{
	u32 cr4;

	cr4 = read_cr4();
	return condition_write_cr4_checking(cr4 | X86_CR4_OSXSAVE);
}

void condition_CR4_OSXSAVE_0(void)
{
	u32 cr4;
	cr4 = read_cr4();
	cr4 &= ~X86_CR4_OSXSAVE;
	condition_write_cr4_checking(cr4);
}

void condition_st_comp_not_enabled(void)
{
	u64 value = STATE_X87;
	u32 eax = value;
	u32 edx = value >> 32;
	u32 index = 0;
	asm volatile(ASM_TRY("1f")
				"xsetbv\n\t" /* xsetbv */
				"1:"
				: : "a" (eax), "d" (edx), "c" (index));

}

void condition_Inv_AVX2_hold(void)
{
}

void condition_VEX_4v_1s_not_hold(void)
{
}

void condition_VEX_4v_1s_hold(void)
{
}

void condition_Inv_Prf_not_hold(void)
{
}

void condition_Inv_Prf_hold(void)
{
}

void condition_pgfault_not_occur(void)
{
}

void condition_pgfault_occur(void)
{
}

void condition_Ad_Cann_non_stack(void)
{
}

void condition_Ad_Cann_non_mem(void)
{
}

void condition_Ad_Cann_cann(void)
{
}

void condition_unmasked_SIMD_hold_AVX(void)
{
	u32 r_a;
	asm volatile("vstmxcsr %0\n" : "=m" (r_a) : );
	r_a = r_a & 0x7f;
	r_a = r_a | 0x3f;
	asm volatile("vldmxcsr %0\n" : : "m" (r_a));
}

void condition_unmasked_SIMD_hold(void)
{
	 u32 mxcsr = 0x1f;
	 asm volatile("ldmxcsr %0" : : "m"(mxcsr));
}

void condition_unmasked_SIMD_not_hold(void)
{
}

void condition_CR4_OSXMMEXCPT_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR4.OSXMMEXCPT[bit 10] to 0 *****\n");

	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10)));

	write_cr4(check_bit);
}

void condition_CR4_OSXMMEXCPT_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR4.OSXMMEXCPT[bit 10] to 1 *****\n");

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_10));

	write_cr4(check_bit);
}

void condition_CR0_TS_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR0.TS[bit 3] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03));

	write_cr0(check_bit);
}

void condition_CR0_TS_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR0.TS[bit 3] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_03)));

	write_cr0(check_bit);
}

void condition_CR0_NE_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR0.NE[bit 5] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr0(check_bit);
}

//Modified manually: reconstruct this condition totally.
#define condition_cs_cpl_0() \
	if (0 != (read_cs() & 0x03)) { \
		report("%s: CPL is not 0", 0, __func__); \
		return; \
	}

void condition_cs_cpl_1(void)
{
}

void condition_cs_cpl_2(void)
{
}

void condition_cs_cpl_3(void)
{
}

void condition_CR0_AC_1(void)
{
	unsigned long check_bit = 0;

	//printf("***** Set CR0.AM[bit 18] to 1 *****\n");

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr0(check_bit);
}

void condition_CR0_AC_0(void)
{
	unsigned long check_bit = 0;

	//printf("***** Clear CR0.AM[bit 18] to 0 *****\n");

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr0(check_bit);
}

void condition_Alignment_aligned(void)
{
	condition_EFLAGS_AC_set_to_0();
}

void condition_Alignment_unaligned(void)
{
	condition_EFLAGS_AC_set_to_1();
}

void condition_CR0_PG_0(void)
{
	//unsigned long check_bit = 0;
	//check_bit = read_cr0();
	//check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));
	//write_cr0(check_bit);
}

void condition_CR0_PG_1(void)
{
	unsigned long check_bit = 0;

	/* Clear PE(CR0[bit 0]) to 1 */
	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));
	write_cr0(check_bit);

	/* Set PG(CR0[bit 31]) to 1 */
	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31));
	write_cr0(check_bit);
}

void condition_CR4_R_W_1(void)
{
	//gp_trigger_func fun;
	//bool ret;
	//unsigned long check_bit = 0;
	//check_bit = read_cr4();
	//check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));
	//check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR4_RESEVED_BIT_23, FEATURE_INFORMATION_23));
	//CR4_R_W_1_write_cr4_checking(check_bit);
}

void condition_CR4_PAE_0(void)
{
	//unsigned long check_bit = 0;
	//check_bit = read_cr4();
	//check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)));
	//write_cr4(check_bit);
}

void condition_CR4_PAE_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05));

	write_cr4(check_bit);
}

void condition_CR4_PCIDE_UP_true(void)
{
//        unsigned long check_bit = 0;
//
//        check_bit = read_cr4();
//        check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
}

void condition_CR4_PCIDE_UP_false(void)
{
//        unsigned long check_bit = 0;
//        check_bit = read_cr4();
//        check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17)));
//
//        write_cr4(check_bit);
}

void condition_DR6_1(void)
{

//        unsigned long check_bit = 0;
//
//        check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);
//
//        DR6_1_write_dr6_checking(check_bit);
}

void condition_DR7_1(void)
{
//        unsigned long check_bit = 0;
//
//        check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);
//	asm volatile("mov %0, %%dr7\n\t"
//			: : "r"(check_bit)
//			: "memory");
}

void condition_DR7_GD_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_dr7();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)));

	write_dr7(check_bit);
}

void condition_DR7_GD_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_dr7();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));

	write_dr7(check_bit);
}

void condition_CSLoad_true(void)
{
	asm volatile(ASM_TRY("1f")
			"mov %0, %%cs \n"
			"1:"
			: : "r" (0x8));
}

#define LAST_SPARE_SEL 0x78
void condition_CSLoad_false(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
#ifdef __x86_64__
	//0x00af9b000000ffff // 64-bit code segment
	set_gdt64_entry(LAST_SPARE_SEL, 0, SEGMENT_LIMIT_ALL,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT);
#else
	set_gdt_entry(LAST_SPARE_SEL, 0, SEGMENT_LIMIT_ALL,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
#endif
	lgdt(&old_gdt_desc);

#ifdef __x86_64__
	struct lseg_st64 fptr;
	fptr.offset = (u64)&&end;
	fptr.selector = LAST_SPARE_SEL;
	asm goto("rex.W\n" "ljmp *%0\n" : : "m"(fptr) : : end);
#else
	struct lseg_st fptr;
	fptr.offset = (u32)&&end;
	fptr.selector = LAST_SPARE_SEL;
	asm goto("ljmp *%0\n" : : "m"(fptr) : : end);
#endif

end:
	printf(" "); //add this line for fixing compile error
}

void condition_REXR_hold(void)
{
}

void condition_CR8_not_used(void)
{
}

void condition_REXR_not_hold(void)
{
}

void condition_CR8_used(void)
{
}

void condition_CR4_R_W_0(void)
{
}

void condition_D_segfault_not_occur(void)
{
	condition_Seg_Limit_No();
}

void condition_AccessCR1567_true(void)
{
}

void condition_AccessCR1567_false(void)
{
}

void condition_WCR0Invalid_true(void)
{
//	u32 cr0 = read_cr0();
//	cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));
//	write_cr0(cr0);
}

void condition_WCR0Invalid_false(void)
{
}

void condition_CR3NOT0_true(void)
{
//	u32 cr3 = read_cr3();
//	cr3 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_08));
//	write_cr3(cr3);
}

void condition_CR3NOT0_false(void)
{
//	u32 cr3 = read_cr3();
//	cr3 &= 0xFFFFF000;
//	write_cr3(cr3);
}

void condition_DR6_0(void)
{
}

void condition_DR7_0(void)
{
}

void condition_DeAc_true(void)
{
}

void condition_DeAc_false(void)
{
}

void condition_Mem_Dest_hold(void)
{
}

void condition_CMPXCHG16B_Aligned_false(void)
{
}

void condition_CMPXCHG16B_Aligned_true(void)
{
}

void condition_MemLocat_false(void)
{
}

void condition_MemLocat_true(void)
{
}

void condition_Push_Seg_hold(void)
{
}

void condition_Push_Seg_not_hold(void)
{
}

void condition_Descriptor_limit_inside(void)
{
}

void condition_Descriptor_limit_outside(void)
{
//        struct descriptor_table_ptr gdt_descriptor_table;
//        sgdt(&gdt_descriptor_table);
//        gdt_descriptor_table.limit = 0x10;
//        lgdt(&gdt_descriptor_table);
}

void condition_Divisor_0_true(void)
{
}

void condition_Divisor_0_false(void)
{
}

void condition_Quot_large_true(void)
{
}

void condition_Quot_large_false(void)
{
}

void condition_JMP_Abs_address_true(void)
{
}

void condition_JMP_Abs_address_false(void)
{
}

void condition_CALL_Abs_address_true(void)
{
}

void condition_CALL_Abs_address_false(void)
{
}

void condition_D_segfault_occur(void)
{
	condition_Seg_Limit_DSeg();
}

void condition_StackPushAlignment_aligned(void)
{
}

void condition_StackPushAlignment_unaligned(void)
{
}

void condition_cs_dpl_0(void)
{
}

void condition_cs_dpl_1(void)
{
}

void condition_cs_dpl_2(void)
{
}

void condition_DesOutIDT_true(void)
{
	struct descriptor_table_ptr old_idt;
	struct descriptor_table_ptr new_idt;
	sidt(&old_idt);
	new_idt.limit = (16 * 100) - 1;
	new_idt.base = old_idt.base;
	lidt(&new_idt);
}

void condition_DesOutIDT_false(void)
{
	struct descriptor_table_ptr old_idt;
	struct descriptor_table_ptr new_idt;
	sidt(&old_idt);
	new_idt.limit = 16 * 255;
	new_idt.base = old_idt.base;
	lidt(&new_idt);
}

void condition_CPUID_LAHF_SAHF_0(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);
}

void condition_S_segfault_occur(void)
{
}

void condition_RPL_CPL_true(void)
{
}

void condition_RPL_CPL_false(void)
{
}

void condition_S_segfault_not_occur(void)
{
}

void condition_Sou_Not_mem_location_true(void)
{
}

void condition_Sou_Not_mem_location_false(void)
{
}

void condition_CR8_R_W_1(void)
{
//        unsigned long check_bit = 0;
//        check_bit = read_cr8();
//        check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR8_RESEVED_BIT_4, FEATURE_INFORMATION_04));
//		write_cr8(check_bit);
}

void condition_CR8_R_W_0(void)
{
}

void condition_LOCK_used(void)
{
}
void condition_CR3_R_W_1(void)
{
//	unsigned long check_bit = 0;
//	check_bit = read_cr3();
//	check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR3_RESEVED_BIT_2, FEATURE_INFORMATION_00));
//	check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR3_RESEVED_BIT_5, FEATURE_INFORMATION_05));
//	write_cr3(check_bit);
}

void condition_CR3_R_W_0(void)
{
}

void condition_CPUID_CMPXCHG16B_set_to_0(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)));
}

void condition_CPUID_CMPXCHG16B_set_to_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13);
}

void condition_EFLAGS_NT_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_14));

	write_rflags(check_bit);
}

void condition_EFLAGS_NT_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_14)));

	write_rflags(check_bit);
}

void condition_CPUID_LAHF_SAHF_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_EXTERN_INFORMATION_801, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00);
}

void condition_EFLAGS_AC_set_to_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_rflags(check_bit);
}

void condition_EFLAGS_AC_set_to_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_rflags();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_rflags(check_bit);
}

void condition_CR0_AM_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18)));

	write_cr0(check_bit);
}

void condition_CR0_AM_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_18));

	write_cr0(check_bit);
}


void condition_Inv_Prt_not_hold(void)
{
}

void condition_Seg_Limit_SSeg(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt_entry(KERNEL_DS, 0, SEGMENT_LIMIT, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
	DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
	GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);
	asm volatile("mov %0, %%eax\n"
				"mov %%ax, %%ss\n"
				:
				: "i"(KERNEL_DS)
				: "memory");

}

void condition_Seg_Limit_DSeg(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt_entry(KERNEL_DS, 0, SEGMENT_LIMIT, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
	DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
	GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);
	asm volatile("mov %0, %%eax\n"
				"mov %%ax, %%ds\n"
				:
				: "i"(KERNEL_DS)
				: "memory");
}

void condition_Seg_Limit_No(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt_entry(KERNEL_DS, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
	DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
	GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);
	asm volatile("mov %0, %%eax\n"
				"mov %%ax, %%ds\n"
				"mov %%ax, %%ss\n"
				"mov %%ax, %%es\n"
				"mov %%ax, %%fs\n"
				"mov %%ax, %%gs\n"
				:
				: "i"(KERNEL_DS)
				: "memory");
}

void condition_VEX_not_used(void)
{
}

void condition_CPUID_SSE2_0(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);
}

void condition_CPUID_SSE2_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_26);
}

void condition_CR0_EM_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02)));
	write_cr0(check_bit);
}

void condition_CR0_EM_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_02));

	write_cr0(check_bit);
}


void condition_CR4_OSFXSR_1(void)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09));
	write_cr4(check_bit);
}

void condition_CR4_OSFXSR_0(void)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09)));
	write_cr4(check_bit);
}

void condition_CPUID_SSE_0(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_25);
}

void condition_CPUID_SSE_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).d;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_25);
}

void condition_CPUID_SSE3_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09);
}

void condition_CPUID_SSE4_1_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_19);
}

void condition_CPUID_SSSE3_0(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09);
}

void condition_CPUID_SSSE3_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_09);
}

void condition_FPU_excp_hold(void)
{
	float op1 = 1.2;
	int op2 = 0;
	unsigned short cw;

	cw = 0x37b;/*bit2==0,report div0 exception*/
	asm volatile("fninit;fldcw %0;fld %1\n"
			 "fdiv %2\n"
			 : : "m"(cw), "m"(op1), "m"(op2));
}

void condition_FPU_excp_not_hold(void)
{
}

void condition_CPUID_SSE4_2_1(void)
{
	unsigned long check_bit = 0;

	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_20);
}

void condition_CR4_PCIDE_1(void)
{
}

void condition_CR4_PCIDE_0(void)
{
}

void condition_PDPT_RB_1(void)
{
}

void condition_PDPT_RB_0(void)
{
}

void condition_PDPTLoad_true(void)
{
}

void condition_PDPTLoad_false(void)
{
}

void condition_Mem_Dest_not_hold(void)
{
}

void condition_Sign_Quot_Large_true(void)
{
}

void condition_Sign_Quot_Large_false(void)
{
}

void condition_Bound_test_fail(void)
{
}

void condition_Bound_test_pass(void)
{
}

void condition_Sec_Not_mem_location_true(void)
{
}

void condition_Sec_Not_mem_location_false(void)
{
}

int set_UMIP(int set)
{
	#define CPUID_7_ECX_UMIP (1 << 2)

	int cpuid_7_ecx;
	u32 cr4 = 0;

	cpuid_7_ecx = cpuid(7).c;
	if (!(cpuid_7_ecx & CPUID_7_ECX_UMIP)) {
		printf("\nCPUID.UMIP is not supported\n");
		return 0;
	}
	cr4 = read_cr4();
	if (set) {
		cr4 |= X86_CR4_UMIP;
	} else {
		cr4 &= ~X86_CR4_UMIP;
	}

	write_cr4(cr4);

	return 1;
}

void condition_CR4_UMIP_0(void)
{
	set_UMIP(0);
}

void condition_CR4_UMIP_1(void)
{
	set_UMIP(1);
}

void condition_write_prot_not_hold(void)
{
}

void condition_write_prot_hold(void)
{
}

void condition_Prefix_67H_used(void)
{
}

void condition_Prefix_67H_not_used(void)
{
}

void condition_CS_D_1(void)
{

}

void condition_CS_D_0(void)
{

}

void condition_RIP_relative_hold(void)
{
}

void condition_RIP_relative_not_hold(void)
{
}

void condition_BNDCFGS_EN_1(void)
{
	wrmsr_checking(MSR_IA32_BNDCFGS, 1);

}

void condition_BNDCFGS_EN_0(void)
{
	wrmsr_checking(MSR_IA32_BNDCFGS, 0);
}

void condition_BND4_15_used(void)
{
}

void condition_BND4_15_not_used(void)
{
}

void condition_Bound_Lower(void)
{
}

void condition_Bound_Upper(void)
{
}

void condition_imm8_0_hold(void)
{
}

void condition_imm8_0_not_hold(void)
{
}

void condition_cs_dpl_3(void)
{
}

void condition_ECX_MSR_Reserved_true(void)
{
}

void condition_ECX_MSR_Reserved_false(void)
{
}

void condition_ECX_MSR_Unimplement_true(void)
{
}

void condition_register_nc_hold(void)
{
}

void condition_register_nc_not_hold(void)
{
}

void condition_EDX_EAX_set_reserved_true(void)
{
}

void condition_EDX_EAX_set_reserved_false(void)
{
}

void condition_ECX_MSR_Unimplement_false(void)
{
}

void condition_CPUID_MMX_0(void)
{
}

#define CHECK_CPUID_0(func, index, reg, bit, msg) \
	if (0 != (cpuid_indexed((func), (index)).reg & (1 << (bit)))) { \
		report("%s: Ignore this case, because the processor supports " msg, \
			1, __func__); \
		return; \
	} \
	printf("%s: the processor does not supports " msg, __func__)

#define CHECK_CPUID_1(func, index, reg, bit, msg) \
	if (0 == (cpuid_indexed((func), (index)).reg & (1 << (bit)))) { \
		report("%s: The processor does not support " msg, \
			0, __func__); \
		return; \
	} \
	printf("%s: CPUID = 0x%x", __func__, cpuid_indexed((func), (index)).reg)

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H,ECX=0):ECX[bit 28]
#define condition_CPUID_AVX_0() \
	CHECK_CPUID_0(0x01, 0, c, 28, "AVX instruction extensions.")

#define condition_CPUID_AVX_1() \
	CHECK_CPUID_1(0x01, 0, c, 28, "AVX instruction extensions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=07H,ECX=0):EBX[bit 5]
#define condition_CPUID_AVX2_0() \
	CHECK_CPUID_0(0x07, 0, b, 5, "AVX2 instruction extensions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 29]
#define condition_CPUID_F16C_0() \
	CHECK_CPUID_0(0x01, 0, c, 29, "16-bit floating-point conversion instructions.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 12]
#define condition_CPUID_FMA_0() \
	CHECK_CPUID_0(0x01, 0, c, 12, "FMA extensions using YMM state.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 0]
#define condition_CPUID_SSE3_0() \
	CHECK_CPUID_0(0x01, 0, c, 0, "Streaming SIMD Extensions 3 (SSE3).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 19]
#define condition_CPUID_SSE4_1_0() \
	CHECK_CPUID_0(0x01, 0, c, 19, "Streaming SIMD Extensions 4.1 (SSE4.1).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX[bit 20]
#define condition_CPUID_SSE4_2_0() \
	CHECK_CPUID_0(0x01, 0, c, 20, "Streaming SIMD Extensions 4.2 (SSE4.2).")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=07H, ECX=0H):EBX.RDSEED[bit 18]
#define condition_CPUID_RDSEED_0() \
	CHECK_CPUID_0(0x07, 0, b, 18, "RDSEED instruction.")

//Modified manually: reconstruct this condition totally.
//Check CPUID.(EAX=01H):ECX.RDRAND[bit 30]
#define condition_CPUID_RDRAND_0() \
	CHECK_CPUID_0(0x01, 0, c, 30, "RDRAND instruction.")

void condition_CR0_MP_1(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01));

	write_cr0(check_bit);

}

void condition_CR0_MP_0(void)
{
	unsigned long check_bit = 0;

	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_01));

	write_cr0(check_bit);
}

void condition_CPUID_RDRAND_1(void)
{
}

void condition_CPUID_RDSEED_1(void)
{
}

void condition_F2HorF3H_hold(void)
{
}

void condition_F2HorF3H_not_hold(void)
{
}

void condition_OpcodeExcp_false(void)
{
}

void condition_OpcodeExcp_true(void)
{
}

//Added manually
u16 global_ss;
void condition_set_ss_null(void)
{
	global_ss = read_ss();
	write_ss(0);
}

void condition_restore_ss(void)
{
	write_ss(global_ss);
}
//end

/*----------------ring0-------------------*/
void do_at_ring0(void (*fn)(const char *), const char *arg)
{
	fn(arg);
}

#ifdef __i386__
void init_call_gate_32(u32 index, u32 code_selector, u8 type, u8 dpl, u8 p, bool is_non_canonical,
	void *fun)
{
	int num = index >> 3;
	u32 call_base = (u32)fun;
	struct gate_descriptor *call_gate_entry = NULL;

	call_gate_entry = (struct gate_descriptor *)&gdt32[num];

	call_gate_entry->gd_looffset = call_base & 0xFFFF;
	call_gate_entry->gd_hioffset = (call_base >> 16) & 0xFFFF;

	call_gate_entry->gd_selector = code_selector;
	call_gate_entry->gd_stkcpy	= 0;
	call_gate_entry->gd_xx = 0;
	call_gate_entry->gd_type = type;
	call_gate_entry->gd_dpl = dpl;
	call_gate_entry->gd_p = p;
}
#endif

void condition_mem_descriptor_nc_hold(void){}
void condition_mem_descriptor_nc_not_hold(void){}
void condition_offset_operand_nc_hold(void){}
void condition_offset_operand_nc_not_hold(void){}
void condition_inst_nc_hold(void){}
void condition_inst_nc_not_hold(void){}
void condition_offset_jump_nc_hold(void){}
void condition_offset_jump_nc_not_hold(void){}
void condition_descriptor_selector_nc_hold(void){}
void condition_descriptor_selector_nc_not_hold(void){}
void condition_complex_nc_hold(void){}
void condition_complex_nc_not_hold(void){}
void condition_Inv_AVX2_not_hold(void){}

#define RING1_CS32_GDT_DESC (0x00cfbb000000ffffULL)
#define RING1_CS64_GDT_DESC (0x00afbb000000ffffULL)
#define RING1_DS_GDT_DESC   (0x00cfb3000000ffffULL)

#define RING2_CS32_GDT_DESC (0x00cfdb000000ffffULL)
#define RING2_CS64_GDT_DESC (0x00afdb000000ffffULL)
#define RING2_DS_GDT_DESC   (0x00cfd3000000ffffULL)

