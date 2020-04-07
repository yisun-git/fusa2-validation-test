gdt_entry_t *new_gdt = NULL;
struct descriptor_table_ptr old_gdt_desc;
struct descriptor_table_ptr *new_gdt_desc;

void init_do_less_privilege(void)
{
	extern unsigned char kernel_entry;

	set_idt_entry(0x21, &kernel_entry, 3);
}

int do_at_ring3(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
		  "mov %%dx, %%ds\n\t"
		  "mov %%dx, %%es\n\t"
		  "mov %%dx, %%fs\n\t"
		  "mov %%dx, %%gs\n\t"
		  "mov %%" R "sp, %%" R "cx\n\t"
		  "push" W " %%" R "dx \n\t"
		  "lea %[user_stack_top], %%" R "dx \n\t"
		  "push" W " %%" R "dx \n\t"
		  "pushf" W "\n\t"
		  "push" W " %[user_cs] \n\t"
		  "push" W " $1f \n\t"
		  "iret" W "\n"
		  "1: \n\t"
		  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
		  "push %[arg]\n\t"
#endif
		  "call *%[fn]\n\t"
#ifndef __x86_64__
		  "pop %%ecx\n\t"
#endif

		  "pop %%" R "cx\n\t"
		  "mov $1f, %%" R "dx\n\t"
		  "int %[kernel_entry_vector]\n\t"
		  ".section .text.entry \n\t"
		  "kernel_entry: \n\t"
		  "mov %%" R "cx, %%" R "sp \n\t"
		  "mov %[kernel_ds], %%cx\n\t"
		  "mov %%cx, %%ds\n\t"
		  "mov %%cx, %%es\n\t"
		  "mov %%cx, %%fs\n\t"
		  "mov %%cx, %%gs\n\t"
		  "jmp *%%" R "dx \n\t"
		  ".section .text\n\t"
		  "1:\n\t"
		  : [ret] "=&a" (ret)
		  : [user_ds] "i" (USER_DS),
		    [user_cs] "i" (USER_CS),
		    [user_stack_top]"m"(user_stack[sizeof user_stack]),
		    [fn]"r"(fn),
		    [arg]"D"(arg),
		    [kernel_ds]"i"(KERNEL_DS),
		    [kernel_entry_vector]"i"(0x21)
		  : "rcx", "rdx");
	return ret;
}

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


static void test_tr_print(void)
{
	test_cs = read_cs();
}

static void test_tr_init(void)
{
	tss32_t *tss_ap1;
	u16 desc_size = 0;
	u16 ss_val = 0;

	setup_vm();
	setup_idt();
	setup_tss32();
	init_do_less_privilege();

	tss_ap1 = &tss;
	tss_ap1 += get_lapic_id();

	tss_ap1->ss0 = 0x58;

	desc_size = sizeof(tss32_t);
	memset((void *)0, 0, desc_size);
	memcpy((void *)0, (void *)tss_ap1, desc_size);

	do_at_ring3(test_tr_print, "TR test");

	ss_val = read_ss();

	report("task_management_rqmid_24027_tr_init", ss_val == tss_ap1->ss0);

	//resume environment
	tss_ap1->ss0 = 0x10;
}


/* cond 1 */
static void cond_1_gate_present(void)
{
	gate.p = 0;
}

/* cond 2 */
static void cond_2_gate_dpl(void)
{
	gate.dpl = 0;
}

/* cond 3 */
static void cond_3_gate_null_sel(void)
{
	gate.selector = 0;
}

/* cond 4 */
static void cond_4_gate_ti(void)
{
	gate.selector |= 0x4;
}

static void cond_4_1_gate_RPL(void)
{
	gate.selector |= 0x3;
}

static void cond_4_2_gate_selector_out_gdt_limit(void)
{
	struct descriptor_table_ptr gdt_desc;

	sgdt(&gdt_desc);

	gate.selector = (gdt_desc.limit << 3) + 1;
}

/* cond 7 */
static void cond_7_desc_present(void)
{


	gdt32[TSS_INTR>>3].access &= 0x7f;/*p = 0*/

}

/* cond 10 */
static void cond_10_desc_busy(void)
{


	gdt32[TSS_INTR>>3].access |= 0x2;/*busy = 1*/

}

/* cond 13 */
void cond_13_desc_tss_not_page(void)
{
	u32 *linear_addr;

	tss_intr.eip = (u32)irq_tss;

	gate.selector = 0x1008;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));



	linear_addr = (u32 *)malloc(PAGE_SIZE * 2);

	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), 0x200);
	old_gdt_desc.base = (u32)linear_addr;
	old_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&old_gdt_desc);

	set_page_control_bit((void *)(phys_to_virt((unsigned long)(linear_addr + PAGE_SIZE))), 1, 0, 0, true);

}

/* cond 14 */
void cond_14_desc_g_limit(void)
{


	gdt32[TSS_INTR>>3].access &= 0xf0;
	gdt32[TSS_INTR>>3].access |= 0x9;/**type = 1001b*/

	gdt32[TSS_INTR>>3].limit_low = 0x66;
	gdt32[TSS_INTR>>3].granularity &= 0x0F;
	gdt32[TSS_INTR>>3].granularity &= ~0x80;/**G bit is 0*/

}

/* cond 15 */
void cond_15_desc_g_limit(void)
{


	gdt32[TSS_INTR>>3].access &= 0xf0;
	gdt32[TSS_INTR>>3].access |= 0x1;/*type = 0001b*/

	gdt32[TSS_INTR>>3].limit_low = 0x66;
	gdt32[TSS_INTR>>3].granularity &= 0x0F;
	gdt32[TSS_INTR>>3].granularity &= ~0x80;/*G bit is 0*/

}

static void cond_0_iret_prelink_null(void)
{


	tss.prev = 0;
}

static void cond_1_iret_prelink_ti(void)
{


	tss.prev = TSS_INTR | 0x4;
}

static void cond_2_iret_prelink_out_limit(void)
{


	struct descriptor_table_ptr gdt_desc;

	sgdt(&gdt_desc);

	tss.prev = (gdt_desc.limit << 3) + 1;
}

static void cond_3_iret_desc_type(void)
{


	gdt32[TSS_INTR>>3].access &= 0xf0;
	gdt32[TSS_INTR>>3].access |= 0x09;/*type = 1001B*/
}

static void far_call_task_gate(void)
{
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TASK_GATE_SEL & 0xf8)), __FUNCTION__);
}

static void far_call_task_desc(void)
{
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}

static void far_int_task_gate(void)
{
	asm volatile(ASM_TRY("1f")
			".word 0xffff\n\t"/*#UD*/
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((UD_VECTOR << 3) | 2)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_001
 *
 * Summary: Present bit of task gate is 0, it would
 *          generate #NP(Error code = (vector << 3H) | 3H )
 *          when switching to a task gate in IDT by NMI
 */
static void task_management_rqmid_25205_nmi_gate_p_bit(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_1_gate_present();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((NMI_VECTOR << 3) | 3)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_002
 *
 * Summary: TSS selector in the task gate is a null selector,
 *          it would gernerate #GP(Error code = 1H )when switching to a task gate in IDT by NMI
 */
static void task_management_rqmid_25206_nmi_gate_null_sel(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_3_gate_null_sel();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == 1), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_003
 *
 * Summary: TSS selector in the task gate has TI flag set,
 *          it would gernerate #GP(Error code = (TSS selector & F8H) | 05H )
 *          when switching to a task gate in IDT by NMI.
 */
static void task_management_rqmid_25207_nmi_ti(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_gate_ti();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 5)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_004
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,
 *          it would gernerate #GP(Error code =(TSS selector & F8H) | 01H )
 *          when switching to a task gate in IDT by NMI
 */
static void task_management_rqmid_25208_nmi_gdt_limit(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_2_gate_selector_out_gdt_limit();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 5)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_005
 *
 * Summary: Page fault occurs when read the descriptor at index of TSS selector,
 *          it would gernerate #PF when switching to a task gate in IDT by NMI.
 */
static void task_management_rqmid_26173_nmi_pf(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_13_desc_tss_not_page();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_006
 *
 * Summary: The descriptor at index of TSS selector is not a non-busy TSS descriptor
 *          (i.e. [bit 44:40] != 01001B), it would gernerate #GP
 *          (Error code =(TSS selector & F8H) | 01H )
 *           when switching to a task gate in IDT by NMI
 */
static void task_management_rqmid_25209_nmi_busy(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_10_desc_busy();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 1)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_007
 *
 * Summary: Present bit of the TSS descriptor is 0, it would gernerate #NP
 *          (Error code = (TSS selector & F8H) | 01H )
 *          when switching to a task gate in IDT by NMI.
 */
static void task_management_rqmid_25210_nmi_p_bit(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_7_desc_present();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 1)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_008
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *          32-bit TSS(i.e. [bit 44:40] is 01001B) and the limit of the TSS descriptor
 *          is lees than 67H, it would gernerate #TS
 *          (Error code =(TSS selector & F8H) | 01H )
 *          when switching to a task gate in IDT by NMI.
 */
static void task_management_rqmid_25211_nmi_g_limit_32(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_14_desc_g_limit();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 1)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_009
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points
 *          to 16-bit TSS (i.e. [bit 44:40] is 0001B) and the limit of the
 *          TSS descriptor is less than 2BH, it would gernerate #TS
 *          (Error code =(TSS selector & F8H) | 01H )when switching to a
 *          task gate in IDT by NMI.
 */
static void task_management_rqmid_26093_nmi_g_limit_16(void)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[2];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_15_desc_g_limit();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(
			"movl $0, %%gs:4 \n\t"				    \
			".pushsection .data.ex \n\t"			    \
			".long 1f, " "1f" "\n\t"			    \
			".popsection \n\t"				    \
			"1111:"
			"wrmsr\n\t"			\
			"1:"::"a"(val), "d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 1)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_001
 *
 * Summary: CPL > task gate DPL, it would gernerate #GP(Error code = (vector << 3H) | 02H)
 *          when switching to a task gate in IDT by a software interrupt
 *          (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_24564_int_gate_cpl(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_2_gate_dpl();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	do_at_ring3(far_int_task_gate, "test");
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_002
 *
 * Summary: Present bit of task gate is 0, it would gernerate #NP
 *          (Error code =(vector << 3H) | 02H) when switching to a task gate in
 *          IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25087_int_p_bit(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_1_gate_present();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 3) | 2)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_003
 *
 * Summary: TSS selector in the task gate is a null selector,
 *          it would generate #GP(Error code = 0) when switching to
 *          a task gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25089_int_gate_selector_null(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_3_gate_null_sel();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_004
 *
 * Summary: TSS selector in the task gate has TI flag set, it would
 *          gernerate #GP(Error code =(TSS selector & F8H) | 04H) when switching
 *          to a task gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25092_int_gate_ti(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_gate_ti();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 4)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_005
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,
 *          it would gernerate #GP(Error code =TSS selector & F8H) when switching
 *          to a task gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25093_int_gdt_limit(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_2_gate_selector_out_gdt_limit();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_006
 *
 * Summary: Page fault occurs when read the descriptor at index of TSS selector,
 *          it would gernerate #PF when switching to a task gate in IDT by a software
 *          interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_26172_int_pf(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_13_desc_tss_not_page();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_007
 *
 * Summary: The descriptor at index of TSS selector is not a non-busy TSS descriptor
 *          (i.e. [bit 44:40] != 01001B), it would gernerate #GP
 *          (Error code =TSS selector & F8H) when switching to a task gate in IDT by a
 *          software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25195_int_busy(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_10_desc_busy();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_008
 *
 * Summary: Present bit of the TSS descriptor is 0, it would gernerate #NP
 *          (Error code =TSS selector & F8H) when switching to a task gate in IDT by a
 *          software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25196_int_p_bit(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_7_desc_present();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_009
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 32-bit TSS
 *          (i.e. [bit 44:40] is 01001B) and the limit of the TSS descriptor is lees than 67H,
 *          it would gernerate #TS(Error code =TSS selector & F8H) when switching to a
 *          task gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_25197_int_g_limit_32(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_14_desc_g_limit();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_010
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 16-bit TSS
 *          (i.e. [bit 44:40] is 0001B) and the limit of the TSS descriptor is less than 2BH,
 *          it would gernerate #TS(Error code =?TSS selector & F8H) when switching to a
 *          task gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void task_management_rqmid_26091_int_g_limit_16(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_15_desc_g_limit();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_001
 *
 * Summary: The previous task link is a null selector, it would gernerate #TS
 *          (Error code =0) when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_25212_iret_null_sel(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_0_iret_prelink_null();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_002
 *
 * Summary: The previous task link has TI flag set, it would gernerate #TS
 *          (Error code =(previous task link & F8H) | 04H) ?when executing IRET with
 *          EFLAGS.NT = 1.
 */
static void task_management_rqmid_25219_iret_ti(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_1_iret_prelink_ti();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8)|4)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_003
 *
 * Summary: Index of the previous task link is out of GDT limits, it would gernerate
 *          #TS(Error code =previous task link & F8H) when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_25221_iret_gdt_limit(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_2_iret_prelink_out_limit();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_004
 *
 * Summary: Page fault occurs when read the descriptor at index of the previous task link,
 *          it would gernerate #PF when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_26174_iret_pf(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	tss.prev = 0x1008;
	cond_13_desc_tss_not_page();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_005
 *
 * Summary: The descriptor at index of the previous task link
 *          is not a busy TSS descriptor (i.e. [bit 44:40] != 01011B),
 *          it would generate #TS(Error code =?previous task link & F8H)
 *          when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_25222_iret_type_not_busy(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	cond_3_iret_desc_type();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_006
 *
 * Summary: Present bit of the TSS descriptor is 0, it would gernerate #NP
 *         (Error code =previous task link & F8H) when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_25223_iret_p_bit(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_7_desc_present();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_007
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 32-bit TSS
 *          (i.e. [bit 44:40] is 01001B) and the limit of the TSS descriptor is lees than 67H,
 *          it would gernerate #TS(Error code =previous task link & F8H)
 *          when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_25224_iret_g_limit_32(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_14_desc_g_limit();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_IRET_008
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 16-bit TSS
 *          (i.e. [bit 44:40] is 00001B) and limit of the TSS descriptor is less than 2BH,
 *          it would gernerate #TS(Error code =previous task link & F8H)
 *          when executing IRET with EFLAGS.NT = 1.
 */
static void task_management_rqmid_26094_iret_g_limit_16(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	cond_15_desc_g_limit();

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_001
 *
 * Summary: CPL > task gate DPL, it would gernerate #GP(Error code =gate selector & F8H)
 *          when executing a CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_24561_task_gate_cpl(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_2_gate_dpl();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	do_at_ring3(far_call_task_gate, "test");
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_002
 *
 * Summary: Task gate selector RPL > task gate DPL, it would gernerate #GP
 *          (Error code =gate selector & F8H)when executing a CALL or JMP instruction
 *          to a task gate.
 */
static void task_management_rqmid_24613_task_gate_rpl(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_1_gate_RPL();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TASK_GATE_SEL & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_003
 *
 * Summary: Present bit of task gate is 0,it would gernerate #NP
 *          (Error code =gate selector & F8H)when executing a CALL or JMP instruction
 *          to a task gate.
 */
static void task_management_rqmid_24562_task_gate_p_bit(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_1_gate_present();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TASK_GATE_SEL & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_004
 *
 * Summary: TSS selector in the task gate is a null selector,it would gernerate #GP
 *         (Error code =0)when executing a CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_24566_task_gate_null_sel(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_3_gate_null_sel();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == 0), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_005
 *
 * Summary: TSS selector in the task gate has TI flag set,it would gernerate #GP
 *         (Error code = (TSS selector & F8H) | 04H) ??when executing a CALL or JMP
 *         instruction to a task gate.
 */
static void task_management_rqmid_24562_task_gate_ti(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_gate_ti();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xf8) | 4)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_006
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,it would gernerate
 *          #GP(Error code = TSS selector & F8H) ??when executing a CALL or JMP instruction
 *          to a task gate.
 */
static void task_management_rqmid_25025_task_gate_gdt_limit(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_4_2_gate_selector_out_gdt_limit();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_007
 *
 * Summary: Page fault occurs when read the descriptor
 *          at index of TSS selector,it would generate #PF
 *          when executing a CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_26171_task_gate_page_fault(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_13_desc_tss_not_page();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	printf("%s can't catch this #PF exception\n", __FUNCTION__);
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008
 *
 * Summary: The descriptor at index of TSS selector is not a non-busy TSS descriptor
 *          (i.e. [bit 44:40] != 01001B),it would gernerate #GP
 *          (Error code = TSS selector & F8H) ??when executing a CALL or JMP instruction to
 *          a task gate.
 */
static void task_management_rqmid_25026_task_gate_busy(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_10_desc_busy();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_009
 *
 * Summary: Present bit of the TSS descriptor is 0,it would gernerate #NP(Error code = TSS
 *          selector & F8H) when executing a CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_25024_task_gate_p_tss(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_7_desc_present();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_010
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 32-bit TSS
 *          (i.e. [bit 44:40] is 01001B) and the limit of the TSS descriptor is lees than 67H,
 *          it would gernerate #TS(Error code = TSS selector & F8H) ??when executing a
 *          CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_25028_task_gate_g_limit_32(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_14_desc_g_limit();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_Task_gate_011
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 16-bit TSS
 *          (i.e. [bit 44:40] is 0001B) and the limit of the TSS descriptor is less than 2BH,
 *          it would gernerate #TS(Error code = TSS selector & F8H) ??when executing a
 *          CALL or JMP instruction to a task gate.
 */
static void task_management_rqmid_26061_task_gate_g_limit_16(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_15_desc_g_limit();

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);

}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_001
 *
 * Summary: CPL > TSS descriptor DPL, it would gernerate #GP(Error code =TSS selector & F8H)
 *          when executing a CALL or JMP instruction with a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_24544_tss_cpl(void)
{
	tss_intr.eip = (u32)irq_tss;

	do_at_ring3(far_call_task_desc, "test");
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_002
 *
 * Summary: TSS selector RPL > TSS descriptor DPL, it would gernerate #GP
 *          (Error code =TSS selector & F8H)?when executing a CALL or JMP instruction with
 *          a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_24546_tss_rpl(void)
{
	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR | 0x3) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR | 0x3) & 0xf8)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_003
 *
 * Summary: The TSS selector has TI flag set, it would gernerate #GP
 *          (Error code =(TSS selector & F8H) | 04H)when executing a CALL or JMP
 *          instruction with a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_24545_tss_ti(void)
{
	tss_intr.eip = (u32)irq_tss;

	cond_10_desc_busy();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR | 0x4) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (((TSS_INTR | 0x4) & 0xf8)|4)), __FUNCTION__);
}

/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_004
 *
 * Summary: The TSS descriptor is busy (i.e. [bit 41] = 1),
 *          it would generate #GP(Error code = TSS selector & F8H)
 *          when executing a CALL or JMP instruction with a
 *          TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_25030_tss_desc_busy(void)
{
	tss_intr.eip = (u32)irq_tss;

	cond_10_desc_busy();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_005
 *
 * Summary: Present bit of the TSS descriptor is 0,it would gernerate #NP
 *          (Error code =TSS selector & F8H)when executing a CALL or JMP instruction with
 *          a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_25032_tss_p_bit(void)
{
	tss_intr.eip = (u32)irq_tss;

	cond_7_desc_present();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_006
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 32-bit TSS
 *          (i.e. [bit 44:40] is 01001B) and the limit of the TSS descriptor is lees than 67H,
 *          it would gernerate #TS(Error code =?TSS selector & F8H)?when executing a
 *          CALL or JMP instruction with a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_25034_tss_g_limit_32(void)
{
	tss_intr.eip = (u32)irq_tss;

	cond_14_desc_g_limit();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}
/**
 * @brief case name:Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_007
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to 16-bit TSS
 *          (i.e. [bit 44:40] is 00001B) and limit of the TSS descriptor is less than 2BH,
 *          it would gernerate #TS(Error code =?TSS selector & F8H)?when executing a
 *          CALL or JMP instruction with a TSS selector to a TSS descriptor.
 */
static void task_management_rqmid_26088_tss_g_limit_16(void)
{
	tss_intr.eip = (u32)irq_tss;

	cond_15_desc_g_limit();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __FUNCTION__);
}

/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_001
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3]
 *          unchanged,while switch task by CALL
 */
static void task_management_rqmid_23803_cr0_ts_unchanged_call(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	//printf("exception_vector = %d, error_code = 0x%x\n", exception_vector(),exception_error_code());
	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ((ts_val1 == ts_val2) && (ts_val1 == 0) &&
		(exception_vector() == GP_VECTOR)), __FUNCTION__);
}
/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_002
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3]
 *          unchanged,while switch task by JMP
 */
static void task_management_rqmid_23835_cr0_ts_unchanged_jmp(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"ljmp $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	//report("exception_vector - expect #GP", (exception_vector() == GP_VECTOR));

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ts_val1 == ts_val2, __FUNCTION__);
}
/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_003
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3] unchanged,
 *          while switch task by interrupt
 */
static void task_management_rqmid_23836_cr0_ts_unchanged_int(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	set_intr_task_gate(35, irq_tss);

	asm volatile(ASM_TRY("1f")
			"int $35\n\t"
			"1:":::);

	//report("exception_vector - expect #GP", (exception_vector() == GP_VECTOR));

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ts_val1 == ts_val2, __FUNCTION__);
}

/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_004
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3] unchanged,
 *          while switch task by IRET
 */
static void task_management_rqmid_24008_cr0_ts_unchanged_iret(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	//report("exception_vector - expect #GP", (exception_vector() == GP_VECTOR));

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ts_val1 == ts_val2, __FUNCTION__);
}

/**
 * @brief case name:Task Management expose CR0.TS_001
 *
 * Summary: Set TS and clear EM, execute SSE instruction shall generate #NM.
 */
static void task_management_rqmid_23821_cr0_ts_expose(void)
{
	u32 cr0_bak;
	u32 cr4_bak;
	unsigned long test_bits;
	sse_union v;
	sse_union add;

	cr0_bak = read_cr0();
	cr4_bak = read_cr4();

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	test_bits = STATE_X87 | STATE_SSE;
	xsetbv_checking(XCR_XFEATURE_ENABLED_MASK, test_bits);

	write_cr0(read_cr0() & ~6); /* EM, TS */
	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0((read_cr0() | (1<<3)) & (~(1<<2)));

	asm volatile(ASM_TRY("1f")
			"movaps %1, %%xmm1\n\t"
			"1:":"=m"(v):"m"(add):"memory");

	//resume environment
	write_cr0(cr0_bak);
	write_cr4(cr4_bak);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:Task Management expose CR0.TS_002
 *
 * Summary: Set TS and clear EM, execute PAUSE instruction would get no exception.
 */
static void task_management_rqmid_23890_cr0_ts_expose(void)
{
	unsigned long test_bits;

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	test_bits = STATE_X87 | STATE_SSE;
	xsetbv_checking(XCR_XFEATURE_ENABLED_MASK, test_bits);

	write_cr0(read_cr0() & ~6); /* EM, TS */
	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0((read_cr0() | (1<<3)) & (~(1<<2)));

	asm volatile(ASM_TRY("1f")
			"pause\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);
}
/**
 * @brief case name:Task Management expose CR0.TS_003
 *
 * Summary: Set TS, clear EM, MP, execute WAIT instruction would not get #NM
 */
static void task_management_rqmid_23892_cr0_ts_expose(void)
{
	unsigned long test_bits;

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	test_bits = STATE_X87 | STATE_SSE;
	xsetbv_checking(XCR_XFEATURE_ENABLED_MASK, test_bits);

	write_cr0(read_cr0() & ~6); /* EM, TS */
	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	/**TS = 1 , EM = 0 , MP = 0 expect #NM*/
	write_cr0((read_cr0() | (1<<3)) & (~(1<<2)) & (~(1<<1)));

	asm volatile(ASM_TRY("1f")
			"wait\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);
}
/**
 * @brief case name:Task Management expose CR0.TS_004
 *
 * Summary: Set EM, TS set and executes SSE instruction MOVAPS.
 *          Set EM, TS not set and executes SSE instruction MOVAPS.
 *          The two results are the same.
 */
static void task_management_rqmid_23893_cr0_ts_expose(void)
{
	unsigned long test_bits;
	sse_union v;
	sse_union add;
	unsigned exp1 = 0;

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	test_bits = STATE_X87 | STATE_SSE;
	xsetbv_checking(XCR_XFEATURE_ENABLED_MASK, test_bits);

	write_cr0(read_cr0() & ~6); /* EM, TS */
	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0((read_cr0() | (1<<3)) | (1<<2));

	asm volatile(ASM_TRY("1f")
			"movaps %1, %%xmm1\n\t"
			"1:":"=m"(v):"m"(add):"memory");

	exp1 = exception_vector();

	write_cr0((read_cr0() & (~(1<<3))) | (1<<2));

	asm volatile(ASM_TRY("1f")
			"movaps %1, %%xmm1\n\t"
			"1:":"=m"(v):"m"(add):"memory");

	report("%s", (exception_vector() == exp1), __FUNCTION__);

}



/**
 * @brief case name:Task Management expose TR_001
 *
 * Summary: Configure TSS and use LTR instruction to load TSS
 */
static void task_management_rqmid_23674_tr_expose(void)
{
	setup_tss32();
	asm volatile(ASM_TRY("1f")
			"mov $" xstr(TSS_INTR)", %%ax\n\t"
			"ltr %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);
}

static void ltr_test(void)
{
	asm volatile(ASM_TRY("1f")
			"mov $" xstr(TSS_INTR)", %%ax\n\t"
			"ltr %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:Task Management expose TR_002
 *
 * Summary: Configure TSS and LTR instruction to load TSS with CPL is not 0
 *          would gernerate #GP
 */
static void task_management_rqmid_23680_tr_expose(void)
{
	do_at_ring3(ltr_test, "test");
}

static void str_test(void)
{
	asm volatile(ASM_TRY("1f")
			"str %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);
}

/**
 * @brief case name:Task Management expose TR_003
 *
 * Summary: Use STR instruction stores the visible portion of the task register
 *          in a general-purpose register with privilege level 0 and 3
 */
static void task_management_rqmid_23687_tr_expose(void)
{
	asm volatile(ASM_TRY("1f")
			"str %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);

	do_at_ring3(str_test, "test");
}
static void str_test1(void)
{
	u16 mem = 0;

	asm volatile(ASM_TRY("1f")
			"str %0\n\t"
			"1:":"=a"(mem)::"memory");

	report("%s", (mem != 0), __FUNCTION__);
}

/**
 * @brief case name:Task Management expose TR_004
 *
 * Summary: Use STR instruction stores the visible portion of the task register
 *          in a memory with privilege level 0 and 3
 */
static void task_management_rqmid_23688_tr_expose(void)
{
	u16 mem = 0;

	asm volatile(ASM_TRY("1f")
			"str %0\n\t"
			"1:":"=a"(mem)::"memory");

	report("%s", (mem != 0), __FUNCTION__);


	do_at_ring3(str_test1, "test");

}
static void str_test2(void)
{
	asm volatile(ASM_TRY("1f")
			"str %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:Task Management expose TR_005
 *
 * Summary: Set CR4.UMIP = 1, execute STR in ring0 and ring3,
 *          it will gernerate #GP with ring3.
 */
static void task_management_rqmid_23689_tr_expose(void)
{
	/*Set CR4.UMIP = 1*/
	write_cr4(read_cr4() | (1<<11));

	asm volatile(ASM_TRY("1f")
			"str %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == 0), __FUNCTION__);

	do_at_ring3(str_test2, "test");
}


/**
 * @brief case name:Task switch in protect mode_001
 *
 * Summary: Configure TSS and use CALL instruction with the TSS
 *          segment selector for the new task as the operand.
 */
static void task_management_rqmid_23761_protect(void)
{
	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);
	//printf("exception_vector = %d, error_code = 0x%x\n",
	//	exception_vector(),exception_error_code());

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}
/**
 * @brief case name:Task switch in protect mode_002
 *
 * Summary: Configure TSS and use CALL instruction with the task gate selector
 *          for the new task as the operand.
 */
static void task_management_rqmid_23762_protect(void)
{
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);

}
/**
 * @brief case name:Task switch in protect mode_003
 *
 * Summary: Configure TSS and use INT instruction with the vector points to
 *          a task-gate descriptor in the IDT as the operand.
 */
static void task_management_rqmid_23763_protect(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof *e);

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&boot_idt[6]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);

}
/**
 * @brief case name:Task switch in protect mode_004
 *
 * Summary: Configure TSS and use IRET with the EFLAGS.NT set and the previous
 *          task link field configured.
 */
static void task_management_rqmid_24006_protect(void)
{
	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	gdt32[TSS_INTR>>3].access |= 0x2;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);

}



/**
 * @brief case name:TR Initial value following start-up_001
 *
 * Summary: Check the TR invisible part in start up
 */
static void task_management_rqmid_24026_tr_start_up(void)
{
	struct descriptor_table_ptr gdt_ptr;
	u16 desc_size = 0;
	u16 ss_val = 0;
	u16 ss_val_old = 0;

	tss.ss0 = 0x58;

	sgdt(&gdt_ptr);
	memcpy(&(gdt32[11]), &(gdt32[2]), sizeof(gdt_entry_t));
	lgdt(&gdt_ptr);

	ss_val_old = read_ss();

	desc_size = sizeof(tss32_t);
	memset((void *)0, 0, desc_size);
	memcpy((void *)0, (void *)&tss, desc_size);

	do_at_ring3(test_tr_print, "TR test");

	ss_val = read_ss();


	report("%s", ((ss_val == tss.ss0) && (ss_val == 0x58) && (ss_val_old == 0x10)),
		__FUNCTION__);

	//resume environment
	tss.ss0 = 0x10;
	asm volatile("mov $0x10, %ax\n\t"
		"mov %ax, %ss\n\t");
	//printf("tss.esp0=0x%x esp=0x%x ss=0x%x\n", tss.esp0, read_esp(),read_ss());
}
/**
 * @brief case name:TR Initial value following INIT_001
 *
 * Summary: Check the TR invisible part in INIT
 */
static void task_management_rqmid_24027_tr_init(void)
{
	struct descriptor_table_ptr gdt_ptr;

	sgdt(&gdt_ptr);

	memcpy(&(gdt32[11]), &(gdt32[2]), sizeof(gdt_entry_t));

	lgdt(&gdt_ptr);

	smp_init();

	on_cpu(1, (void *)test_tr_init, NULL);
}

__attribute__((unused)) static void test_task_management_32_normal(void)
{
	/*386*/
	task_management_rqmid_25206_nmi_gate_null_sel();
	task_management_rqmid_25207_nmi_ti();
	task_management_rqmid_25208_nmi_gdt_limit();
	task_management_rqmid_26173_nmi_pf();
	task_management_rqmid_25209_nmi_busy();
	task_management_rqmid_25210_nmi_p_bit();
	task_management_rqmid_25211_nmi_g_limit_32();
	task_management_rqmid_26093_nmi_g_limit_16();
	task_management_rqmid_24564_int_gate_cpl();
	task_management_rqmid_25087_int_p_bit();
	task_management_rqmid_25092_int_gate_ti();
	task_management_rqmid_25093_int_gdt_limit();
	task_management_rqmid_26172_int_pf();
	task_management_rqmid_25195_int_busy();
	task_management_rqmid_25196_int_p_bit();
	task_management_rqmid_25197_int_g_limit_32();
	task_management_rqmid_26091_int_g_limit_16();
	task_management_rqmid_25212_iret_null_sel();
	task_management_rqmid_25219_iret_ti();
	task_management_rqmid_25221_iret_gdt_limit();
	task_management_rqmid_26174_iret_pf();
	task_management_rqmid_25223_iret_p_bit();
	task_management_rqmid_25224_iret_g_limit_32();
	task_management_rqmid_26094_iret_g_limit_16();
	task_management_rqmid_24561_task_gate_cpl();
	task_management_rqmid_24613_task_gate_rpl();
	task_management_rqmid_24562_task_gate_p_bit();
	task_management_rqmid_24566_task_gate_null_sel();
	task_management_rqmid_24562_task_gate_ti();
	task_management_rqmid_25025_task_gate_gdt_limit();
	task_management_rqmid_25026_task_gate_busy();
	task_management_rqmid_25024_task_gate_p_tss();
	task_management_rqmid_25028_task_gate_g_limit_32();
	task_management_rqmid_26061_task_gate_g_limit_16();
	task_management_rqmid_24544_tss_cpl();
	task_management_rqmid_24546_tss_rpl();
	task_management_rqmid_24545_tss_ti();
	task_management_rqmid_25032_tss_p_bit();
	task_management_rqmid_25034_tss_g_limit_32();
	task_management_rqmid_26088_tss_g_limit_16();
	task_management_rqmid_23835_cr0_ts_unchanged_jmp();
	task_management_rqmid_23836_cr0_ts_unchanged_int();
	task_management_rqmid_24008_cr0_ts_unchanged_iret();
	task_management_rqmid_23890_cr0_ts_expose();
	task_management_rqmid_23892_cr0_ts_expose();
	task_management_rqmid_23893_cr0_ts_expose();
	task_management_rqmid_23680_tr_expose();
	task_management_rqmid_23687_tr_expose();
	task_management_rqmid_23688_tr_expose();
	task_management_rqmid_23689_tr_expose();
	task_management_rqmid_23762_protect();
	task_management_rqmid_23763_protect();
	task_management_rqmid_24006_protect();
}

static void test_task_management_32(void)
{
	int branch = 0;
	/*386*/
	switch (branch) {
	case 0:
		task_management_rqmid_23821_cr0_ts_expose();
		task_management_rqmid_24026_tr_start_up();
		task_management_rqmid_23803_cr0_ts_unchanged_call();
		task_management_rqmid_25205_nmi_gate_p_bit();
		task_management_rqmid_25089_int_gate_selector_null();
		task_management_rqmid_25222_iret_type_not_busy();
		task_management_rqmid_25030_tss_desc_busy();
		task_management_rqmid_23674_tr_expose();
		task_management_rqmid_23761_protect();
		task_management_rqmid_24027_tr_init();
		break;

	case 1:
		task_management_rqmid_26171_task_gate_page_fault(); //PF ok ,can't catch
		break;

	default:
		break;
	}
}
