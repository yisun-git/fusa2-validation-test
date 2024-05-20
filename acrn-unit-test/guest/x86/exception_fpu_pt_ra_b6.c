#define PT_B6
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
#include "fwcfg.h"
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
#endif

void test_FCLEX_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"FCLEX\n" "1:\n" : : );
}

static __unused void fpu_pt_ra_b6_instruction_0(const char *msg)
{
	test_for_exception(UD_VECTOR, test_FCLEX_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"FCHS\n" "1:\n" : : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"FABS\n" "1:\n" : : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"F2XM1\n" "1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_4(const char *msg)
{
	#ifdef __x86_64__
	u64 a = (u64)&unsigned_64;
	a ^= 0x8000000000000000;
	#endif
	asm volatile(ASM_TRY("1f")
	#ifdef PT_B6
	#ifdef __x86_64__
				"FICOMP %[output]\n" "1:\n"
				: [output] "=m" (*(u32 *)a) : );
	#else
				"FICOMP %%ds:0xFFFF0000\n" "1:\n"
				: [output] "=m" (unsigned_16) : );
	#endif
	#else
				"FICOMP %%ds:0xFFFF\n" "1:\n"
				: [output] "=m" (unsigned_16) : );
	#endif
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"FCOMI %%st(1), %%st(0)\n" "1:\n"
				:  : );
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"FCOMIP %%st(1), %%st(0)\n" "1:\n"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"WAIT\n" "1:\n" : : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"FWAIT\n" "1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"WAIT\n" "1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_pt_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	do_at_ring0(fpu_pt_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_1();
	do_at_ring0(fpu_pt_ra_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_2(void)
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
	condition_CR0_TS_1();
	do_at_ring0(fpu_pt_ra_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_3(void)
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
	do_at_ring0(fpu_pt_ra_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_4(void)
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
	condition_D_segfault_occur();
	do_at_ring0(fpu_pt_ra_b6_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_5(void)
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
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(fpu_pt_ra_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_6(void)
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
	condition_FPU_excp_not_hold();
	do_at_ring0(fpu_pt_ra_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_MP_1();
	condition_CR0_TS_1();
	do_at_ring0(fpu_pt_ra_b6_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_MP_0();
	do_at_ring0(fpu_pt_ra_b6_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_pt_ra_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_MP_1();
	condition_CR0_TS_0();
	do_at_ring0(fpu_pt_ra_b6_instruction_9, "");
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
	fpu_pt_ra_b6_0();
	fpu_pt_ra_b6_1();
	fpu_pt_ra_b6_2();
	fpu_pt_ra_b6_3();
	fpu_pt_ra_b6_4();
	fpu_pt_ra_b6_5();
	fpu_pt_ra_b6_6();
	fpu_pt_ra_b6_7();
	fpu_pt_ra_b6_8();
	fpu_pt_ra_b6_9();

	return report_summary();
}