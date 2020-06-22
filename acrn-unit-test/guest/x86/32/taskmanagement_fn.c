gdt_entry_t *new_gdt = NULL;
struct descriptor_table_ptr old_gdt_desc;
struct descriptor_table_ptr new_gdt_desc;

void init_do_less_privilege(void)
{
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);
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


static void test_tr_print(const char *msg)
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

/* cond 3 */
static void cond_3_gate_null_sel(void)
{
	gate.selector = 0;
}

/* cond 10 */
static void cond_10_desc_busy(void)
{
	gdt32[TSS_INTR>>3].access |= 0x2;/*busy = 1*/
}

/* cond 13 */
void cond_13_desc_tss_not_page(void)
{
	unsigned char *linear_addr;

	tss_intr.eip = (u32)irq_tss;

	gate.selector = 0x1008;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate), sizeof(struct task_gate));

	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);

	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), 0x200);
	new_gdt_desc.base = (u32)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);
}

static void cond_3_iret_desc_type(void)
{
	gdt32[TSS_INTR>>3].access &= 0xf0;
	gdt32[TSS_INTR>>3].access |= 0x09;/*type = 1001B*/
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

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(new_gdt_desc.base + PAGE_SIZE)), PAGE_PTE, 0, 1, true);
	free((void *)new_gdt_desc.base);
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

	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
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
 * @brief case name:TR base following INIT_001
 *
 * Summary: Check the TR invisible part in INIT
 */
static __unused void task_management_rqmid_24027_tr_init(void)
{
	struct descriptor_table_ptr gdt_ptr;

	sgdt(&gdt_ptr);

	memcpy(&(gdt32[11]), &(gdt32[2]), sizeof(gdt_entry_t));

	lgdt(&gdt_ptr);

	smp_init();

	on_cpu(1, (void *)test_tr_init, NULL);
}

static void print_case_list_32(void)
{
	printf("Task Management feature protecte mode case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 23821,
		"Task Management expose CR0.TS_001");
	printf("\t Case ID:%d case name:%s\n\r", 24026,
		"TR Initial value following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 23803,
		"guest CR0.TS keeps unchanged in task switch attempts_001");
	printf("\t Case ID:%d case name:%s\n\r", 25205,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_001");
	printf("\t Case ID:%d case name:%s\n\r", 25089,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_003");
	printf("\t Case ID:%d case name:%s\n\r", 25222,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_005");
	printf("\t Case ID:%d case name:%s\n\r", 25030,
		"Exception_Checking_on_TSS_selector_004");
	printf("\t Case ID:%d case name:%s\n\r", 23674,
		"Task Management expose TR_001");
	printf("\t Case ID:%d case name:%s\n\r", 23761,
		"Task switch in protect mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 24027,
		"TR Initial value following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 26171,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_007");
}

static void test_task_management_32(void)
{
	task_management_rqmid_23821_cr0_ts_expose();
	task_management_rqmid_24026_tr_start_up();
	task_management_rqmid_23803_cr0_ts_unchanged_call();
	task_management_rqmid_25205_nmi_gate_p_bit();
	task_management_rqmid_25089_int_gate_selector_null();
	task_management_rqmid_25222_iret_type_not_busy();
	task_management_rqmid_25030_tss_desc_busy();
	task_management_rqmid_23674_tr_expose();
	task_management_rqmid_23761_protect();
#ifdef IN_NON_SAFETY_VM
	task_management_rqmid_24027_tr_init();
#endif
	task_management_rqmid_26171_task_gate_page_fault();

}
