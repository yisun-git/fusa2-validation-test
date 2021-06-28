#include "segmentation.h"
#include "interrupt.h"
#include "instruction_common.h"
#include "delay.h"
#include "debug_print.h"
#include "memory_type.h"
#include "fwcfg.h"

static volatile unsigned int g_irqcounter[256] = { 0 };

int INC_COUNT = 0;
static inline void irqcounter_initialize(void)
{
	/* init g_irqcounter and INC_COUNT to 0
	 * (INC_COUNT is the value added to the corresponding vector every
	 * time an exception is generated. It has been used to determine
	 * the sequence of the exceptions when multiple exceptions are generated)
	 */
	INC_COUNT = 0;
	memset((void *)g_irqcounter, 0, sizeof(g_irqcounter));
}
static inline void irqcounter_incre(unsigned int vector)
{
	INC_COUNT += 1;
	g_irqcounter[vector]  += INC_COUNT;
}

static inline unsigned int irqcounter_query(unsigned int vector)
{
	return g_irqcounter[vector];
}

struct ex_record {
	unsigned long rip;
	unsigned long handler;
};

extern struct ex_record exception_table_start, exception_table_end;

/* The length of instruction to skip when the exception handler returns
 * (equal to the length of instruction in which the exception occurred)
 */
static volatile int execption_inc_len = 0;
/* Used for exception handling function to return error code
 */
ulong save_error_code = 0;
/* save_rflags1 is used to save the rflag of  the stack when an exception is generated.
 * save_rflags2 is used to save the rflag at the
 * beginning of the exception handling function.
 */
ulong save_rflags1 = 0;
ulong save_rflags2 = 0;

void set_idt_present(int vec, int p)
{
	idt_entry_t *e = &boot_idt[vec];
	e->p = p;
}

void set_idt_type(int vec, int type)
{
	idt_entry_t *e = &boot_idt[vec];
	e->type = type;
}

void set_idt_addr(int vec, void *addr)
{
	idt_entry_t *e = &boot_idt[vec];
	e->offset0 = (unsigned long)addr;
	e->offset1 = (unsigned long)addr >> 16;
#ifdef __x86_64__
	e->offset2 = (unsigned long)addr >> 32;
#endif
}

void *get_idt_addr(int vec)
{
	unsigned long addr;
	idt_entry_t *e = &boot_idt[vec];

#ifdef __x86_64__
	addr = ((unsigned long)e->offset0) | ((unsigned long)e->offset1 << 16)
		| ((unsigned long)e->offset2 << 32);
#else
	addr = ((unsigned long)e->offset0) | ((unsigned long)e->offset1 << 16);
#endif
	return (void *)addr;
}

void handled_interrupt_external_e0(isr_regs_t *regs)
{
	irqcounter_incre(EXTEND_INTERRUPT_E0);
	eoi();
}

void handled_interrupt_external_e0_not_eoi(isr_regs_t *regs)
{
	irqcounter_incre(EXTEND_INTERRUPT_E0);
	//eoi();
}

void handled_interrupt_beyond_limit_f0(isr_regs_t *regs)
{
	irqcounter_incre(IDT_BEYOND_LIMIT_INDEX);
	eoi();
}

void handled_interrupt_external_80(isr_regs_t *regs)
{
	save_rflags1 = regs->rflags;
	save_rflags2 = read_rflags();
	irqcounter_incre(EXTEND_INTERRUPT_80);
	eoi();
}

int check_rflags()
{
	int check = 0;
	/* TF[bit 8], NT[bit 14], RF[bit 16], VM[bit 17] */
	if ((save_rflags2 & RFLAG_TF_BIT) || (save_rflags2 & RFLAG_NT_BIT)
		|| (save_rflags2 & RFLAG_RF_BIT) || (save_rflags2 & RFLAG_VM_BIT)) {
		check = 1;
	}
	return check;
}

asm (".pushsection .text \n\t"
		"__handle_exception: \n\t"
#ifdef __x86_64__
		"push %r15; push %r14; push %r13; push %r12 \n\t"
		"push %r11; push %r10; push %r9; push %r8 \n\t"
#endif
		"push %"R "di; push %"R "si; push %"R "bp; sub $"S", %"R "sp \n\t"
		"push %"R "bx; push %"R "dx; push %"R "cx; push %"R "ax \n\t"
#ifdef __x86_64__
		"mov %"R "sp, %"R "di \n\t"
#else
		"mov %"R "sp, %"R "ax \n\t"
#endif
		"call do_handle_exception \n\t"
		"pop %"R "ax; pop %"R "cx; pop %"R "dx; pop %"R "bx \n\t"
		"add $"S", %"R "sp; pop %"R "bp; pop %"R "si; pop %"R "di \n\t"
#ifdef __x86_64__
		"pop %r8; pop %r9; pop %r10; pop %r11 \n\t"
		"pop %r12; pop %r13; pop %r14; pop %r15 \n\t"
#endif
		"add $"S", %"R "sp \n\t"
		"add $"S", %"R "sp \n\t"
		"iret"W" \n\t"
		".popsection");

#define SOFT_EX(NAME, N) extern char NAME##_fault_soft;	\
	asm (".pushsection .text \n\t"		\
	     #NAME"_fault_soft: \n\t"		\
	     "push"W" $0 \n\t"			\
	     "push"W" $"#N" \n\t"		\
	     "jmp __handle_exception \n\t"	\
	     ".popsection")
SOFT_EX(df, 8);
SOFT_EX(ts, 10);


#ifdef __x86_64__
/*test case which should run under 64bit  */
#include "64/interrupt_fn.c"
#elif __i386__
/*test case which should run  under 32bit  */
#include "32/interrupt_fn.c"
#endif

__unused static void print_case_list(void)
{
	printf("Case ID:%d case name:%s\n\r", 38001,
		"Page fault while handling #DF in non-safety VM_001");
}

__unused static void test_interrupt(void)
{
	interrupt_rqmid_38001_df_pf();
}

int main(void)
{
	setup_vm();
	setup_idt();
#ifdef IN_NON_SAFETY_VM
	print_case_list();

	test_interrupt();
#endif

	return report_summary();
}

