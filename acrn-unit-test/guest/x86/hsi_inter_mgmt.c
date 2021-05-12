/*
 * Copyright (C) 2020 Intel Corporation. All rights reserved.
 *
 * Author:              hexiangx.li@intel.com
 * Date :               2020/11/10
 * Description:         Test case/code for FuSa HSI (Hardware Software Interface Test)
 *                      interrupt management featrues
 *
 */
#define USE_DEBUG
#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include "xsave.h"
#include "debug_print.h"
#include "hsi.h"
#include "segmentation.h"
#include "delay.h"

#define CR4_RESERVED_BIT_23				(1 << 23)

#define IDT_TABLE_SIZE (sizeof(idt_entry_t) * 256)
#define TEST_EXTEND_INTERRUPT 0x31
#define INTERRUPT_EXE_TIME 1
static u32 soft_interrupt_cout;
static u32 exception_cout;
static u32 g_instruction_len;
static idt_entry_t *new_idt_entry = NULL;

tss64_t tss64_intr;
#define STACK_SIZE 4096
static char mc_stack[STACK_SIZE];
static char df_stack[STACK_SIZE];
static char ss_stack[STACK_SIZE];
#define TSS_64BIT_SEL 0x58
enum {
	STACK_TYPE_MC = 1,
	STACK_TYPE_DF,
	STACK_TYPE_SS,
};
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_64BIT_TSS 0x9
struct descriptor_table_ptr old_idt;
#define CMP_RSP_SPACE(rsp, stack_space) (((rsp) > (u64)(stack_space)) && ((rsp) < ((u64)(stack_space) + STACK_SIZE)))

static u64 g_rsp;
typedef struct {
	int case_id;
	const char *case_name;
	void (*case_fun)(void);
} st_case_suit_inter;

void set_local_idt_entry(idt_entry_t *boot_idt, int vec, void *addr, int dpl, u16 ist)
{
	idt_entry_t *e = &boot_idt[vec];
	memset(e, 0, sizeof *e);
	e->offset0 = (unsigned long)addr;
	e->selector = read_cs();
	e->ist = (unsigned short)ist;
	e->type = 14;
	e->dpl = dpl;
	e->p = 1;
	e->offset1 = (unsigned long)addr >> 16;
#ifdef __x86_64__
	e->offset2 = (unsigned long)addr >> 32;
#endif
}

void handled_extend_int_0x31(void)
{
	soft_interrupt_cout++;
	eoi();
}

void handled_exception_save_rsp(void)
{
	asm volatile("mov %%rsp, %0" : "+m"(g_rsp) : : "memory");
	eoi();
}

static inline void int_ins(u16 vector)
{
	asm volatile("int %[test_entry_vector]\n\t"
		:
		: [test_entry_vector]"i"(vector)
		:);
}

struct ex_record {
	unsigned long rip;
	unsigned long handler;
};
extern struct ex_record exception_table_start, exception_table_end;
void handle_gp_exception(struct ex_regs *regs)
{
	/* mark gp exception handle is executed */
	exception_cout++;
	/* skip cause GP exception instruction */
	regs->rip += g_instruction_len;
	eoi();
}

static u8 lidt_checking(const struct descriptor_table_ptr *ptr)
{
	asm volatile(ASM_TRY("1f")
		"lidt %0\n"
		"1:"
		: : "m" (*ptr) :
	);
	return exception_vector();
}

__unused static int sidt_checking(struct descriptor_table_ptr *ptr)
{
	asm volatile(ASM_TRY("1f")
		"sidt %0\n"
		"1:"
		: "=m" (*ptr) : : "memory"
	);
	return exception_vector();
}
__unused static void setup_tss64_entry(u16 sel)
{
	u16 desc_size = sizeof(tss64_t);
	struct descriptor_table_ptr gdt64_desc;

	memset((void *)&tss64_intr, 0, desc_size);
	tss64_intr.ist1 = (u64)mc_stack + STACK_SIZE;
	tss64_intr.ist2 = (u64)df_stack + STACK_SIZE;
	tss64_intr.ist3 = (u64)ss_stack + STACK_SIZE;
	tss64_intr.ist4 = 0;
	tss64_intr.iomap_base = (u16)desc_size;

	sgdt(&gdt64_desc);
	set_gdt64_entry(sel, (u64)&tss64_intr, desc_size - 1,
		SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_SYS|SYS_SEGMENT_AND_GATE_DESCRIPTOR_64BIT_TSS, 0x0f);
	lgdt(&gdt64_desc);
}

static void save_environment()
{
	sidt(&old_idt);
}

__unused static void resume_interrupt_environment()
{
	lidt(&old_idt);
}

#define TSC_TIMER_VECTOR               0x0E0U
#define LVT_TIMER_PERIODIC          (1 << 17)

/* Following are interrupt handlers */
static atomic_t lapic_timer_isr_count;
static void lapic_timer_isr(isr_regs_t *regs)
{
	(void) regs;
	atomic_inc(&lapic_timer_isr_count);

	eoi();
}

/*
 * @brief case name: HSI_Interrupt_Management_Features_EFLAGS_Interrupt_Flag_001
 *
 * Summary: Under 64 bit mode on native board, arm apic timer,
 * when set EFLAGS.IF, apic timer interrupt is delivered;
 * when clear EFLAGS.IF, apic timer interrupt is not delivered.
 */
static void hsi_rqmid_35949_interrupt_management_features_eflags_interrupt_flag_001(void)
{
	u32 chk = 0;
	u32 lvtt;
	const unsigned int vec = TSC_TIMER_VECTOR;
	u32 initial_value = 1000;

	irq_disable();
	atomic_set(&lapic_timer_isr_count, 0);
	handle_irq(vec, lapic_timer_isr);

	/* TSC periodic timer with IRQ enabled */
	irq_enable();
	/* Enable TSC periodic timer mode */
	lvtt = LVT_TIMER_PERIODIC | vec;
	apic_write(APIC_LVTT, lvtt);
	/* Arm the timer */
	apic_write(APIC_TMICT, initial_value);
	test_delay(5);

	while (atomic_read(&lapic_timer_isr_count) < 1) {
		initial_value--;
		if (initial_value == 0) {
			break;
		}
	}
	apic_write(APIC_TMICT, 0);

	if (atomic_read(&lapic_timer_isr_count) >= 1) {
		chk++;
		debug_print("Test set EFLAGS.IF flag pass. LAPIC timer interrupt is delivered\n");
	} else {
		debug_print("Test set EFLAGS.IF flag fail.\n");
	}

	/* TSC periodic timer with IRQ disabled */
	irq_disable();
	initial_value = 1000;
	atomic_set(&lapic_timer_isr_count, 0);
	apic_write(APIC_TMICT, initial_value);
	test_delay(5);

	while (atomic_read(&lapic_timer_isr_count) < 1) {
		initial_value--;
		if (initial_value == 0) {
			break;
		}
	}
	apic_write(APIC_TMICT, 0);

	if (atomic_read(&lapic_timer_isr_count) == 0) {
		chk++;
		debug_print("Test clear EFLAGS.IF flag pass. LAPIC timer interrupt is not delivered\n");
	} else {
		debug_print("Test clear EFLAGS.IF flag fail.\n");
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: HSI_Interrupt_Management_Features_Interrupt_Stack_Table_MC_001
 *
 * Summary: Under 64 bit mode on native board, create a new IDT table,
 * set the IST field in the IDT descriptor for #MC to 1, allocate 1 stack spaces for #MC,
 * configure the IST1 fields of the TSS to point to the mc stack spaces allocated in GDT table,
 * trigger #MC, verify that the RSP value obtained in the interrupt handler is in the allocated stack address space.
 */
static void hsi_rqmid_40126_interrupt_management_features_inter_stack_table_mc_001(void)
{
	u32 chk = 0;
	struct descriptor_table_ptr new_idt;
	g_rsp = 0;

	save_environment();

	/* config tss data, put the 64bit-TSS descriptor in GDT and the corresponding entry index is 58H */
	setup_tss64_entry(TSS_64BIT_SEL);

	/* load tss */
	ltr(TSS_64BIT_SEL);

	/* prepare the exception interrupt hander of #MC */
	set_local_idt_entry(new_idt_entry, MC_VECTOR, handled_exception_save_rsp, 0, STACK_TYPE_MC);

	new_idt.limit = IDT_TABLE_SIZE - 1;
	new_idt.base = (ulong)new_idt_entry;

	/* load idt descriptor */
	lidt(&new_idt);

	int_ins(MC_VECTOR);

	/* Check global variable g_rsp is in the allocated stack address space */
	if (CMP_RSP_SPACE(g_rsp, mc_stack)) {
		chk++;
	}
	debug_print(" g_rsp:0x%lx mc_stack_rbp:0x%lx mc_stack_rsp:0x%lx chk:%d\n",
		g_rsp, (u64)mc_stack, (u64)mc_stack + STACK_SIZE, chk);
	report("%s", (chk == 1), __FUNCTION__);
	resume_interrupt_environment();
}

/*
 * @brief case name: HSI_Interrupt_Management_Features_Interrupt_Stack_Table_DF_001
 *
 * Summary: Under 64 bit mode on native board, create a new IDT table,
 * set the IST field in the IDT descriptor for #DF to 2, allocate 1 stack spaces for #DF,
 * configure the IST2 fields of the TSS to point to the df stack spaces allocated in GDT table,
 * trigger #DF, verify that the RSP value obtained in the interrupt handler is in the allocated stack address space.
 */
__unused static void hsi_rqmid_40135_interrupt_management_features_inter_stack_table_df_001(void)
{
	u32 chk = 0;
	struct descriptor_table_ptr new_idt;
	g_rsp = 0;

	save_environment();

	/* config tss data, put the 64bit-TSS descriptor in GDT and the corresponding entry index is 58H */
	setup_tss64_entry(TSS_64BIT_SEL);

	/* load tss */
	ltr(TSS_64BIT_SEL);

	/* prepare the exception interrupt hander of #DF */
	set_local_idt_entry(new_idt_entry, DF_VECTOR, handled_exception_save_rsp, 0, STACK_TYPE_DF);

	new_idt.limit = IDT_TABLE_SIZE - 1;
	new_idt.base = (ulong)new_idt_entry;

	/* load idt descriptor */
	lidt(&new_idt);

	int_ins(DF_VECTOR);

	/* Check global variable g_rsp is in the allocated stack address space */
	if (CMP_RSP_SPACE(g_rsp, df_stack)) {
		chk++;
	}
	debug_print(" g_rsp:0x%lx df_stack_rbp:0x%lx df_stack_rsp:0x%lx chk:%d\n",
		g_rsp, (u64)df_stack, (u64)df_stack + STACK_SIZE, chk);
	report("%s", (chk == 1), __FUNCTION__);
	resume_interrupt_environment();
}


/*
 * @brief case name: HSI_Interrupt_Management_Features_Interrupt_Stack_Table_SS_001
 *
 * Summary: Under 64 bit mode on native board, create a new IDT table,
 * set the IST field in the IDT descriptor for #SS to 3, allocate 1 stack spaces for #SS,
 * configure the IST3 fields of the TSS to point to the SS stack spaces allocated in GDT table,
 * trigger #SS, verify that the RSP value obtained in the interrupt handler is in the allocated stack address space.
 */
__unused static void hsi_rqmid_40147_interrupt_management_features_inter_stack_table_ss_001(void)
{
	u32 chk = 0;
	struct descriptor_table_ptr new_idt;
	g_rsp = 0;

	save_environment();

	/* config tss data, put the 64bit-TSS descriptor in GDT and the corresponding entry index is 58H */
	setup_tss64_entry(TSS_64BIT_SEL);

	/* load tss */
	ltr(TSS_64BIT_SEL);

	/* prepare the exception interrupt hander of #SS */
	set_local_idt_entry(new_idt_entry, SS_VECTOR, handled_exception_save_rsp, 0, STACK_TYPE_SS);

	new_idt.limit = IDT_TABLE_SIZE - 1;
	new_idt.base = (ulong)new_idt_entry;

	/* load idt descriptor */
	lidt(&new_idt);

	int_ins(SS_VECTOR);

	/* Check global variable g_rsp is in the allocated stack address space */
	if (CMP_RSP_SPACE(g_rsp, ss_stack)) {
		chk++;
	}
	debug_print(" g_rsp:0x%lx ss_stack_rbp:0x%lx ss_stack_rsp:0x%lx chk:%d\n",
		g_rsp, (u64)ss_stack, (u64)ss_stack + STACK_SIZE, chk);
	report("%s", (chk == 1), __FUNCTION__);
	resume_interrupt_environment();
}

/*
 * @brief case name: HSI_Interrupt_Management_Features_IDT_001
 *
 * Summary: Under 64 bit mode on native board, create a new IDT table,
 * execute LIDT instruction to install new IDT and execute SIDT instruction to save IDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
static void hsi_rqmid_40088_interrupt_management_features_idt_001(void)
{
	u32 chk = 0;
	u32 ret1 = 1;
	u32 ret2 = 2;
	struct descriptor_table_ptr new_idt;
	struct descriptor_table_ptr old_idt;
	struct descriptor_table_ptr save_idt;
	ulong cr4_value;
	exception_cout = 0;
	soft_interrupt_cout = 0;

	sidt(&old_idt);

	/* prepare the soft interrupt hander of 0x31 */
	set_local_idt_entry(new_idt_entry, TEST_EXTEND_INTERRUPT, handled_extend_int_0x31, 0, 0);
	/* prepare the interrupt handler of #GP. */
	handle_exception(GP_VECTOR, &handle_gp_exception);

	new_idt.limit = IDT_TABLE_SIZE - 1;
	new_idt.base = (ulong)new_idt_entry;

	ret1 = lidt_checking(&new_idt);

	/* Trigger soft interrupt */
	int_ins(TEST_EXTEND_INTERRUPT);

	cr4_value = read_cr4() | CR4_RESERVED_BIT_23;
	/* skip gp exception instruction */
	g_instruction_len = 3;
	/* generate #GP */
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");

	if (ret1 == NO_EXCEPTION) {
		chk++;
	}

	ret2 = sidt_checking(&save_idt);
	if (ret2 == NO_EXCEPTION) {
		chk++;
	}

	if ((soft_interrupt_cout == INTERRUPT_EXE_TIME) &&
		(exception_cout == INTERRUPT_EXE_TIME)) {
		chk++;
	}

	if ((save_idt.limit == new_idt.limit) &&
		(save_idt.base == new_idt.base)) {
		chk++;
	}
	debug_print(" lidt ret1:%d sidt ret2:%d handled_interrupt:%d\n", ret1, ret2, soft_interrupt_cout);
	report("%s", (chk == 4), __FUNCTION__);

	/* resume environment */
	lidt(&old_idt);
}

static st_case_suit_inter case_suit_inter[] = {
	{
		.case_fun = hsi_rqmid_35949_interrupt_management_features_eflags_interrupt_flag_001,
		.case_id = 35949,
		.case_name = "HSI_Interrupt_Management_Features_EFLAGS_Interrupt_Flag_001",
	},

	{
		.case_fun = hsi_rqmid_40088_interrupt_management_features_idt_001,
		.case_id = 40088,
		.case_name = "HSI_Interrupt_Management_Features_IDT_001",
	},

	{
		.case_fun = hsi_rqmid_40126_interrupt_management_features_inter_stack_table_mc_001,
		.case_id = 40126,
		.case_name = "HSI_Interrupt_Management_Features_Interrupt_Stack_Table_MC_001",
	},

	{
		.case_fun = hsi_rqmid_40135_interrupt_management_features_inter_stack_table_df_001,
		.case_id = 40135,
		.case_name = "HSI_Interrupt_Management_Features_Interrupt_Stack_Table_DF_001",
	},

	{
		.case_fun = hsi_rqmid_40147_interrupt_management_features_inter_stack_table_ss_001,
		.case_id = 40147,
		.case_name = "HSI_Interrupt_Management_Features_Interrupt_Stack_Table_SS_001",
	},
};
static void print_case_list(void)
{
	/*_x86_64__*/
	printf("HSI interrupt management feature case list:\n\r");
#ifdef IN_NATIVE
	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_inter); i++) {
		printf("\t Case ID: %d Case name: %s\n\r", case_suit_inter[i].case_id, case_suit_inter[i].case_name);
	}
#else
#endif
	printf("\t \n\r \n\r");
}



int main(void)
{
	setup_idt();
	asm volatile("fninit");

	setup_vm();
	print_case_list();

#ifdef IN_NATIVE
	/* create a new IDT table */
	new_idt_entry = (idt_entry_t *)malloc(IDT_TABLE_SIZE);
	if (new_idt_entry == NULL) {
		printf("malloc IDT table error.\n");
		return 0;
	}
	/* copy the old IDT table to new IDT table */
	memcpy((void *)new_idt_entry, (void *)&boot_idt[0], IDT_TABLE_SIZE);

	int i;
	for (i = 0; i < ARRAY_SIZE(case_suit_inter); i++) {
		case_suit_inter[i].case_fun();
	}

	/* free memory */
	free(new_idt_entry);
	new_idt_entry = NULL;
#endif
	return report_summary();
}

