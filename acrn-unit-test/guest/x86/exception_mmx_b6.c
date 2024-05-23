#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "fwcfg.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"

// define the data type
__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(64))) static u32 unsigned_32 = 0;
__unused __attribute__ ((aligned(64))) static u64 unsigned_64 = 0;
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

// unaligned address
static __unused u16 *unaligned_address_16(void)
{
	static u16  __attribute__ ((aligned(16))) addr_unalign_16;
	u16 *aligned_addr = &addr_unalign_16;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(void)
{
	static u32  __attribute__ ((aligned(32))) addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(void)
{
	static u64  __attribute__ ((aligned(64))) addr_unalign_64;
	u64 *aligned_addr = &addr_unalign_64;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	static union_unsigned_128  __attribute__ ((aligned(64))) addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_256 *unaligned_address_256(void)
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

// non canon mem aligned address
__attribute__ ((aligned(8))) static u8 addr_8 = 0;
static __unused u64 *non_canon_align_8(void)
{
	u64 address = (unsigned long)&addr_8;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(16))) static u16 addr_16 = 0;
static __unused u64 *non_canon_align_16(void)
{
	u64 address = (unsigned long)&addr_16;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(32))) static u32 addr_32 = 0;
static __unused u64 *non_canon_align_32(void)
{
	u64 address = (unsigned long)&addr_32;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}


__attribute__ ((aligned(64))) static u64 addr_64 = 0;
static __unused u64 *non_canon_align_64(void)
{
	u64 address = (unsigned long)&addr_64;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(64))) static union_unsigned_128 addr_128;
static __unused u64 *non_canon_align_128(void)
{
	u64 address = (unsigned long)&addr_128;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(64))) static union_unsigned_256 addr_256;
static __unused u64 *non_canon_align_256(void)
{
	u64 address = (unsigned long)&addr_256;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

// non canon stack aligned address
__unused static u64 *non_canon_align_stack(u64 addr)
{
	u64 address = addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

// trigger_pgfault
static __unused u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (u64 *)add1;
}

static __unused void mmx_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRLD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_1(const char *msg)
{
	condition_EFLAGS_AC_set_to_1();
	asm volatile(ASM_TRY("1f") "MOVQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	condition_EFLAGS_AC_set_to_0();
	report("%s", (exception_vector() == AC_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PACKSSWB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRLQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRAD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDSB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMADDWD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMULHW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSRAW $0x1, %%mm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PACKUSWB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PACKSSDW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBSB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMULLW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PAND %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PADDUSB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_b6_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_b6_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_b6_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLD $0x1, %%mm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_b6_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PANDN %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_b6_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mmx_b6_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mmx_b6_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(mmx_b6_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(mmx_b6_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(mmx_b6_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mmx_b6_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mmx_b6_instruction_6, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(mmx_b6_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(mmx_b6_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(mmx_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(mmx_b6_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(mmx_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(mmx_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(mmx_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(mmx_b6_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(mmx_b6_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(mmx_b6_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(mmx_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(mmx_b6_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(mmx_b6_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(mmx_b6_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(mmx_b6_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_b6_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}

static __unused void mmx_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(mmx_b6_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	mmx_b6_0();
	mmx_b6_1();
	mmx_b6_2();
	mmx_b6_3();
	mmx_b6_4();
	mmx_b6_5();
	mmx_b6_6();
	mmx_b6_7();
	mmx_b6_8();
	mmx_b6_9();
	mmx_b6_10();
	mmx_b6_11();
	mmx_b6_12();
	mmx_b6_13();
	mmx_b6_14();
	mmx_b6_15();
	mmx_b6_16();
	mmx_b6_17();
	mmx_b6_18();
	mmx_b6_19();
	mmx_b6_20();
	mmx_b6_21();
	mmx_b6_22();
	mmx_b6_23();
	mmx_b6_24();
	mmx_b6_25();
	mmx_b6_26();

	return report_summary();
}