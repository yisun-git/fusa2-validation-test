#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "misc.c"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"

// define the data type
__unused __attribute__ ((aligned(32))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(32))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(32))) static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(32))) static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(32))) static u32 unsigned_32 = 0;
//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused __attribute__ ((aligned(32))) static u32 unsigned_64 = 0;
u8 exception_vector_bak = 0xFF;
static volatile u32 execption_inc_len = 0;

void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	if (execption_inc_len == 0) {
		unhandled_exception(regs, false);
	} else {
		regs->rip += execption_inc_len;
	}
}

void set_handle_exception(void)
{
	handle_exception(DE_VECTOR, &handled_exception);
	handle_exception(DB_VECTOR, &handled_exception);
	handle_exception(NMI_VECTOR, &handled_exception);
	handle_exception(BP_VECTOR, &handled_exception);
	handle_exception(OF_VECTOR, &handled_exception);
	handle_exception(BR_VECTOR, &handled_exception);
	handle_exception(UD_VECTOR, &handled_exception);
	handle_exception(NM_VECTOR, &handled_exception);
	handle_exception(DF_VECTOR, &handled_exception);
	handle_exception(TS_VECTOR, &handled_exception);
	handle_exception(NP_VECTOR, &handled_exception);
	handle_exception(SS_VECTOR, &handled_exception);
	handle_exception(GP_VECTOR, &handled_exception);
	handle_exception(PF_VECTOR, &handled_exception);
	handle_exception(MF_VECTOR, &handled_exception);
	handle_exception(AC_VECTOR, &handled_exception);
	handle_exception(MC_VECTOR, &handled_exception);
	handle_exception(XM_VECTOR, &handled_exception);
}

__unused static u32 *unaligned_address_32(void)
{
	static u32 __attribute__ ((aligned(32))) addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused static u64 *unaligned_address_64(void)
{
	static u64 __attribute__ ((aligned(64))) addr_unalign_64;
	u64 *aligned_addr = &addr_unalign_64;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

__unused static union_unsigned_128 * unaligned_address_128(void)
{
	static union_unsigned_128 __attribute__ ((aligned(64))) addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

__unused static union_unsigned_256 * unaligned_address_256(void)
{
	static union_unsigned_256 __attribute__ ((aligned(64))) addr_unalign_256;
	addr_unalign_256.m[0] = 1;
	addr_unalign_256.m[1] = 1;
	addr_unalign_256.m[2] = 1;
	addr_unalign_256.m[3] = 1;
	addr_unalign_256.m[4] = 1;
	addr_unalign_256.m[5] = 1;
	addr_unalign_256.m[6] = 1;
	addr_unalign_256.m[7] = 1;
	union_unsigned_256 *addr = (union_unsigned_256 *)&addr_unalign_256;
	union_unsigned_256 *non_aligned_addr = (union_unsigned_256 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

// trigger_pgfault
__unused static u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (u64 *)add1;
}

static __unused void mmx_pt_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"PXOR %%ss:0xFFFF0000, %%mm1\n"
				"1:\n"
				: : : "memory");
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"PXOR %%ds:0xFFFF0000, %%mm1\n"
				"1:\n"
				: : : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %%cs:0 \n" "1:" : [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_4(const char *msg)
{
	condition_EFLAGS_AC_set_to_1();
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_64())) : );
	condition_EFLAGS_AC_set_to_0();
	report("%s", (exception_vector() == AC_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVD %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVQ %%mm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBUSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_26(const char *msg)
{
	condition_EFLAGS_AC_set_to_1();
	asm volatile(ASM_TRY("1f") "PSUBSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	condition_EFLAGS_AC_set_to_0();
	report("%s", (exception_vector() == AC_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_27(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRAW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRLD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRAD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRLW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLW $0x1, %%mm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_43(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRLQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBUSB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_instruction_46(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_pt_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(mmx_pt_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(mmx_pt_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_hold();
	execption_inc_len = 3;
	do_at_ring0(mmx_pt_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mmx_pt_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mmx_pt_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(mmx_pt_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(mmx_pt_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(mmx_pt_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_pt_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_pt_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(mmx_pt_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(mmx_pt_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(mmx_pt_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_pt_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_pt_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(mmx_pt_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(mmx_pt_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(mmx_pt_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_pt_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_pt_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(mmx_pt_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(mmx_pt_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(mmx_pt_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_pt_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_write_prot_not_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_pt_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mmx_pt_instruction_25, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mmx_pt_instruction_26, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(mmx_pt_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(mmx_pt_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(mmx_pt_instruction_29, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_pt_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_pt_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(mmx_pt_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(mmx_pt_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(mmx_pt_instruction_34, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_pt_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_pt_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(mmx_pt_instruction_37, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(mmx_pt_instruction_38, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(mmx_pt_instruction_39, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_pt_instruction_40, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_pt_instruction_41, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(mmx_pt_instruction_42, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(mmx_pt_instruction_43, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(mmx_pt_instruction_44, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_pt_instruction_45, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_pt_instruction_46, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	mmx_pt_0();
	mmx_pt_1();
	mmx_pt_2();
	mmx_pt_3();
	mmx_pt_4();
	mmx_pt_5();
	mmx_pt_6();
	mmx_pt_7();
	mmx_pt_8();
	mmx_pt_9();
	mmx_pt_10();
	mmx_pt_11();
	mmx_pt_12();
	mmx_pt_13();
	mmx_pt_14();
	mmx_pt_15();
	mmx_pt_16();
	mmx_pt_17();
	mmx_pt_18();
	mmx_pt_19();
	mmx_pt_20();
	mmx_pt_21();
	mmx_pt_22();
	mmx_pt_23();
	mmx_pt_24();
	mmx_pt_25();
	mmx_pt_26();
	mmx_pt_27();
	mmx_pt_28();
	mmx_pt_29();
	mmx_pt_30();
	mmx_pt_31();
	mmx_pt_32();
	mmx_pt_33();
	mmx_pt_34();
	mmx_pt_35();
	mmx_pt_36();
	mmx_pt_37();
	mmx_pt_38();
	mmx_pt_39();
	mmx_pt_40();
	mmx_pt_41();
	mmx_pt_42();
	mmx_pt_43();
	mmx_pt_44();
	mmx_pt_45();
	mmx_pt_46();

	return report_summary();
}