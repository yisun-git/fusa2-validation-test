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
__unused __attribute__ ((aligned(64))) static fpu_sse_t fpu_sse_t_obj;
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

static __unused void fpu_pt_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FMUL %%st(1), %%st(0)\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FMULP \n" "1:" :  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FISUBR %[input_1]\n" "1:"
				:  : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FLDENV %[input_1]\n" "1:"
				:  : [input_1] "m" (fpu_sse_t_obj));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FLD %%st(1)\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FLDCW %[input_1]\n" "1:"
				:  : [input_1] "m" (unsigned_16));
	exception_vector_bak = exception_vector();
}

static __unused void fpu_pt_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	do_at_ring0(fpu_pt_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(fpu_pt_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(fpu_pt_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(fpu_pt_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	do_at_ring3(fpu_pt_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	do_at_ring3(fpu_pt_instruction_5, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	asm volatile("fninit");
	fpu_pt_0();
	fpu_pt_1();
	fpu_pt_2();
	fpu_pt_3();
	fpu_pt_4();
	fpu_pt_5();

	return report_summary();
}