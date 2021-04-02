#ifdef PT_B6
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
#include "segmentation.h"
#include "interrupt.h"
#include "delay.h"
#include "isr.h"
#else
asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
#endif
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;
#ifdef PT_B6
u16 execption_inc_len = 0;
#else
extern u16 execption_inc_len;
#endif

#ifdef PT_B6
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
	if (execption_inc_len == 0)
		unhandled_exception(regs, false);
	else
		regs->rip += execption_inc_len;
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

__attribute__ ((aligned(16))) static u16 addr_16 = 0;
static __unused u16 *non_canon_align_16(void)
{
	u64 address = (unsigned long)&addr_16;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u16 *)address;
}


void test_FNSTSW_GP_b6(void *data)
{
	asm volatile(ASM_TRY("1f") "FNSTSW %[output]\n" "1:"
				: [output] "=a" (*non_canon_align_16()) : );
}

#endif

void test_fstsw_nm(void *data)
{
	asm volatile(ASM_TRY("1f") "FSTSW %%ax\n" "1:"
				: [output] "=a" (unsigned_16) : );
}

static __unused void fpu_ra_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "FNSTSW %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void fpu_ra_b6_instruction_1(const char *msg)
{
	test_for_exception(NM_VECTOR, test_fstsw_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void fpu_ra_b6_instruction_2(const char *msg)
{
	#ifdef PT_B6
	test_for_exception(GP_VECTOR, test_FNSTSW_GP_b6, NULL);
	#else
	asm volatile(ASM_TRY("1f") "FNSTSW %%ds:0xFFFF\n" "1:"
				: [output] "=a" (unsigned_16) : );
	#endif
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void fpu_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CR0_EM_1();
	do_at_ring0(fpu_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(fpu_ra_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_ra_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_occur();
	do_at_ring0(fpu_ra_b6_instruction_2, "");
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
	#ifdef PHASE_0
	fpu_ra_b6_0();
	fpu_ra_b6_1();
	fpu_ra_b6_2();
	#endif

	return report_summary();
}
