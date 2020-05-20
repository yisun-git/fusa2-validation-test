#include "interrupt.h"

//#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

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
u32 execption_inc_len = 0;
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

#ifdef __x86_64__
/*test case which should run under 64bit  */
#include "64/interrupt_fn.c"
#elif __i386__
/*test case which should run  under 32bit  */
#include "32/interrupt_fn.c"
#endif

static void print_case_list(void)
{
#ifndef __x86_64__	/*386*/
	print_case_list_32();
#else			/*_x86_64__*/
	print_case_list_64();
#endif
}

static void test_interrupt(void)
{
#ifndef __x86_64__	/*386*/
	test_interrupt_32();
#else				/*x86_64__*/
	test_interrupt_64();
#endif
}

void ap_main(void)
{
#ifndef __x86_64__	/*386*/
		//test_ap_interrupt_32();
#else				/*x86_64__*/
		test_ap_interrupt_64();
#endif
}

int main(void)
{
	setup_vm();
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);
	setup_idt();

	print_case_list();

	test_interrupt();

	return report_summary();
}

