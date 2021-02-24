#include "segmentation.h"
#include "interrupt.h"
#include "debug_print.h"

static volatile unsigned int g_irqcounter[256] = { 0 };
/* The length of instruction to skip when the exception handler returns
 * (equal to the length of instruction in which the exception occurred)
 */
static volatile int execption_inc_len = 0;
int INC_COUNT = 0;

extern tss32_t tss_intr;
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

static volatile int test_count;
static void irq_tss(void)
{
start:
	printf("IRQ task is running\n");
	print_current_tss_info();
	test_count++;
	asm volatile ("iret");
	test_count++;
	printf("IRQ task restarts after iret.\n");
	goto start;
}

static void interru_df_ts(const char *fun)
{
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov (%%eax), %%ebx' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 and 3 is define irqcounter_incre */

	/* step 4 initialize NEWTSS */
	setup_tss32();

	/* step 5 construct a segment descriptor in 4th entry (selector 0x20)
	 * of OLDGDTR with P bit set to 1, Type set to 0x9, S bit set to 0,
	 * BASE set to NEWTSS address, LIMIT set to 0x66(less than sizeof(NEWTSS) - 1).
	 */
	gdt32[TSS_INTR>>3].limit_low = 0x66;
	tss_intr.eip = (u32)irq_tss;

	/* step 6 prepare NEWGDT */
	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), PAGE_SIZE);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), PAGE_SIZE);

	/* step 7 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 8 prepare the task-gate descriptor of #DF */
	set_idt_task_gate(DF_VECTOR, TSS_INTR);

	/* step 9 prepare the interrupt-gate descriptor of #PF,
	 * the selector of the #PF is on the second page((0x201<<3) = x01008).
	 * IDT has been initialized. Only segment selector is set here
	 */
	set_idt_sel(PF_VECTOR, 0x1008);

	/* step 10 prepare the not present page.*/
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);

	/* setp 11 trigger #PF exception */
	asm volatile(
			"mov %0, %%eax\n\t"
			"add $0x1000, %%eax\n\t"
			"mov (%%eax), %%ebx\n\t"
			::"m"(linear_addr));

	/* step 12  report shutdown vm, Control should not come here */
	report("%s ", (0), fun);

	/* resume environment, just set, but not come here */
	set_idt_sel(DF_VECTOR, read_cs());
	set_idt_sel(PF_VECTOR, read_cs());
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 1, true);
	free(linear_addr);
	linear_addr = NULL;
	execption_inc_len = 0;
}

/**
 * @brief case name: Contributory exception while handling #DF in non-safety VM_001
 *
 * Summary: At protected mode, when a vCPU of the non-safety VM detected a second contributory
 * exception(#TS) while calling the exception handler for a prior #DF, ACRN hypervisor shall
 * guarantee that any vCPU of the non-safety VM stops executing any instructions(VM shutdowns).
 */
__attribute__((unused)) static void interrupt_rqmid_36128_df_ts(void)
{
	interru_df_ts(__FUNCTION__);
}

static void print_case_list(void)
{
	printf("Case ID:%d case name:%s\n\r", 36128,
		"Contributory exception while handling #DF in non-safety VM_001");
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();

#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_36128_df_ts();
#endif

	return report_summary();
}
