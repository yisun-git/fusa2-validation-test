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
#endif

void test_PSRLW_ud(void *data)
{
	asm volatile(
				"lock\n"
				"PSRLW $0x1, %%mm1\n"
				: : );
}

static __unused void mmx_pt_ra_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"PADDUSW %[input_1], %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_ra_b6_instruction_1(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PSRLW_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_pt_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CR0_EM_1();
	//Added manually: fix no output issue
	printf(" ");
	do_at_ring0(mmx_pt_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_pt_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_used();
	//Added manually: fix no output issue
	printf(" ");
	do_at_ring0(mmx_pt_ra_b6_instruction_1, "");
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
	//Added manually: init floating-point unit
	asm volatile("fninit");
	#ifdef PHASE_0
	mmx_pt_ra_b6_0();
	mmx_pt_ra_b6_1();
	#endif

	return report_summary();
}
