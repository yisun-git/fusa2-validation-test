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
#endif

#ifdef PT_B6
#ifdef __x86_64__
static __unused u64 *non_canon_align_mem(u64 addr)
{
	u64 address = 0;
	if ((addr >> 63) & 1) {
		address = (addr & (~(1ull << 63)));
	} else {
		address = (addr | (1UL<<63));
	}
	return (u64 *)address;
}
#else
#undef u64
#define u64 size_t
#define non_canon_align_mem(x) ((u32 *)(size_t)x)
#endif
#else
#undef u64
#define u64 size_t
#define non_canon_align_mem(x) ((u16 *)(size_t)x)
#endif

static __unused void test_SUB_instruction_3(void *data)
{
	asm volatile(ASM_TRY("1f")
		"lock\n"
	#ifdef PT_B6
	#ifdef __x86_64__
		"SUB $1, %[output]\n"
	#else
		"SUB $1, %%ds:0xFFFF0000\n"
	#endif
	#else
		"SUB $1, %%ds:0xFFFF\n"
	#endif
		"1:\n" : [output] "=a" (*(non_canon_align_mem((u64)&unsigned_8))) : );
}


static __unused void gp_pt_ra_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "AAM %0\n" "1:\n" : : "i"(0) : );
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "AAM %0\n" "1:\n" : : "i"(0xA) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_imm8_0_hold();
	do_at_ring0(gp_pt_ra_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_imm8_0_not_hold();
	do_at_ring0(gp_pt_ra_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	#ifdef PHASE_0
	gp_pt_ra_0();
	gp_pt_ra_1();
	#endif

	return report_summary();
}
