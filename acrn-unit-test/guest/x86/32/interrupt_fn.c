
void set_idt_s_flag(int vec, int value)
{
	unsigned char *p;
	idt_entry_t *e = &boot_idt[vec];

	p = (unsigned char *)e;
	p += 5;

	printf("e = %llx\n", *(u64 *)e);
	if (value) {
		*p = *p | 0x10;
	}
	else {
		*p = *p & 0xEF;
	}
	printf("e = %llx\n", *(u64 *)e);
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

static u32 current_eip;
static int eip_index = 0;
static u32 eip[10] = {0, };

void get_current_eip_function(void)
{
	u32 esp_32 = 0;
	u64 *esp_val;
	asm volatile("mov %%edi, %0\n\t"
		: "=m"(esp_32)
		);

	esp_val = (u64 *)esp_32;

	current_eip = esp_val[0];
}

asm("get_current_eip:\n"
	"mov %esp, %edi\n"
	"call get_current_eip_function\n"
	"ret\n"
);
void get_current_eip(void);

void handled_32_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	save_error_code = regs->error_code;
	save_rflags1 = regs->rflags;
	save_rflags2 = read_rflags();
	ex_val = regs->vector | (regs->error_code << 16) |
		(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));

	irqcounter_incre(regs->vector);

	if (eip_index < 10) {
		eip[eip_index++] = regs->rip;
	}

	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	if (execption_inc_len >= 0) {
		regs->rip += execption_inc_len;
	}
	else {
		unhandled_exception(regs, false);
	}
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

/**
 * @brief case name: Contributory exception while handling #DF in safety VM_001
 *
 * Summary: At protected mode, when a vCPU of the safety VM detected a second
 * contributory exception(#TS) while calling the exception handler for a prior #DF,
 * ACRN hypervisor shall call bsp_fatal_error().
 */
__attribute__((unused)) static void interrupt_rqmid_36132_df_ts(void)
{
	interru_df_ts(__FUNCTION__);
}

void ring3_ss(const char *arg)
{
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/*index =0*/
	lss.selector = 0x83;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(lss)
	);
}

/**
 * @brief case name: Contributory exception while handling a prior contributory exception_001
 *
 * Summary: At protected mode, when a vCPU detected a second contributory exception(#GP)
 * while calling an exception handler for a prior contributory exception(#SS),
 * ACRN hypervisor shall guarantee that the vCPU receives #DF(0).
 */
static void interrupt_rqmid_36133_ss_gp(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'lss  %0, %%eax' instructions len
	 */
	execption_inc_len = 4;

	/* step 2, 3 and 4 is define irqcounter_incre */
	handle_exception(DF_VECTOR, &handled_32_exception);

	/* step 5 prepare the interrupt-gate descriptor of #SS*/
	set_idt_s_flag(SS_VECTOR, 1);

	/* step 6 init by setup_idt (prepare the interrupt-gate descriptor of #DF)*/
	/* step 7 init by setup_idt (prepare the interrupt-gate descriptor of #GP)*/

	/* setp 8 construct a data segment descriptor in 16th entry (selector 0x80) of GDT*/
	sgdt(&old_gdt_desc);
	set_gdt_entry(0x80, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* setp 9 reload  GDTR*/
	lgdt(&old_gdt_desc);


	/* setp 10 and step 11 switch to ring3, execute LSS to load the segment of not present constructed in step 6 */
	do_at_ring3(ring3_ss, "");

	/* step 12 confirm #DF exception has been generated*/
	report("%s", ((irqcounter_query(DF_VECTOR) == 1) && (save_error_code == 0)), __FUNCTION__);

	/* resume environment */
	set_idt_s_flag(SS_VECTOR, 0);
	execption_inc_len = 0;
}

static void ring3_ac_add()
{
	u64 value = 0;
	unsigned char *p = (unsigned char *)&value;
	p = p + 2;

	printf("%p %p \n", &value, p);
	asm volatile("incl %0" ::"m"(*(u32 *)p));

}
/**
 * @brief case name: Interrupt to non-present descriptor_002
 *
 * Summary: At protected mode, when a vCPU attempts to trigger  #AC exception
 * via interrupt-gate, if  P is not present, ACRN hypervisor shall guarantee
 * that the vCPU receives the #NP(0x8B) (index =17, IDT=1, EXT=1 )
 */
static void interrupt_rqmid_36246_no_present_descriptor_002(void)
{
	ulong set_value = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'incl %0' instructions len
	 */
	execption_inc_len = 3;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(NP_VECTOR, &handled_32_exception);

	/* step 4 prepare the interrupt-gate descriptor of
	 * #AC with P bit set to 0(not present)
	 */
	set_idt_present(AC_VECTOR, 0);

	/* step 5 init by setup_idt (prepare the interrupt-gate descriptor of #NP)*/

	/* step 6  set cr0.AM */
	set_value = read_cr0();
	set_value |= CR0_AM_BIT;
	write_cr0(set_value);

	/* step 7 set EFLAGS.AC */
	set_value = read_rflags();
	set_value |= RFLAG_AC_BIT;
	write_rflags(set_value);

	/* step 8 and 9 switch to ring3 execute INC*/
	do_at_ring3(ring3_ac_add, "");

	/* step 10 confirm #NP exception has been generated*/
	report("%s", ((irqcounter_query(NP_VECTOR) == 1)
		& (save_error_code == ((AC_VECTOR*8)+3))), __FUNCTION__);

	/* resume environment */
	set_idt_present(AC_VECTOR, 1);
	execption_inc_len = 0;
}

/**
 * @brief case name: Contributory exception while handling a prior contributory exception_002
 *
 * Summary: At protected mode, when a vCPU detected a second contributory exception(#NP)
 * while calling an exception handler for a prior contributory exception(#PF),
 * ACRN hypervisor shall guarantee that the vCPU receives #DF(0).
 */
static void interrupt_rqmid_38046_pf_np(void)
{
	u8 *p;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov %al,(%ebx)' instructions len
	 */
	execption_inc_len = 2;

	/* step 2, 3 and 4 is define irqcounter_incre */
	handle_exception(DF_VECTOR, &handled_32_exception);

	/* step 5 prepare the IDT entry of #PF, with its P flag as 0*/
	set_idt_present(PF_VECTOR, 0);

	/* step 6 init by setup_idt (prepare the interrupt-gate descriptor of #NP)*/
	/* step 7 init by setup_idt (prepare the interrupt-gate descriptor of #DF)*/

	/* setp 8 define one GVA1 points to one 1B memory block in the 4K page */
	p = malloc(sizeof(u8));
	if (p == NULL) {
		printf("malloc error!\n");
		return;
	}

	/* setp 9 set the 4k page PTE entry is not present */
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 0, true);

	/*setp 10 access the memory block to trigger a PF.*/
	u8 value = 0x1;
	asm volatile("mov %[value], (%[p])\n\t"
			: : [value]"r"(value), [p]"r"(p));

	/* step 11 confirm #DF exception has been generated*/
	report("%s", ((irqcounter_query(DF_VECTOR) == 1) && (save_error_code == 0)), __FUNCTION__);

	/* resume environment */
	set_idt_present(PF_VECTOR, 1);
	execption_inc_len = 0;
}

void ring3_trigger_interrupt_n(const char *arg)
{
	/* step 9 Make ESP not aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"add $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");

	/* step 10 Execute INT 224*/
	asm volatile(
		"INT $"xstr(EXTEND_INTERRUPT_E0)"\n\t");

	/* step 11 Make ESP aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"sub $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");
}

/**
 * @brief case name: Benign exception while handling a prior exception_intn_ac_001
 *
 * Summary: At protected mode, the processor in INT n meeting a second #AC will be trigger a #AC.
 */
static void interrupt_rqmid_38071_intn_ac(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'int $0xe0' instructions len
	 */
	execption_inc_len = 2;

	/* step 2, 3 is define irqcounter_incre */

	/* step 4 init by setup_idt (prepare the interrupt-gate descriptor of #AC)*/
	handle_exception(AC_VECTOR, &handled_32_exception);

	/* step 5 Construct interrupt gate descriptor of  224th interrupt(224th entry in IDT)
	 * with the Selector set to be 0x4b, DPL set to 3,
	 * the Offset point to interrupt 0xE0 handler function
	 */
	handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0_not_eoi);
	set_idt_dpl(EXTEND_INTERRUPT_E0, 3);
	set_idt_sel(EXTEND_INTERRUPT_E0, USER_CS);

	/* step 6 Set CR0.AC[bit 18] to 1.*/
	cr0_am_to_1();
	/* step 7 Set RFLAGS.AC[bit 18] bit to 1*/
	eflags_ac_to_1();

	/*step 8 Switch cpl to ring3*/
	do_at_ring3(ring3_trigger_interrupt_n, "");

	/* step 12 Switch to ring 0*/

	/* step 13 Send EOI*/
	eoi();

	/* step 14 Confirm only #AC exception has been generated*/
	report("%s %x %x", (irqcounter_query(AC_VECTOR) == 1),
		__FUNCTION__, irqcounter_query(AC_VECTOR), irqcounter_query(EXTEND_INTERRUPT_E0));

	/* resume environment */
	set_idt_dpl(EXTEND_INTERRUPT_E0, 0);
	set_idt_sel(EXTEND_INTERRUPT_E0, read_cs());
	execption_inc_len = 0;
}

static ulong cr4_value_gp = 0;
void ring3_trigger_gp(const char *arg)
{
	/* step 9 Make ESP not aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"add $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");

	/* step 10 Set CR4 reserved [bit 23]  to 1 to trigger #GP exception.*/
	asm volatile ("mov %0, %%cr4" : : "r"(cr4_value_gp) : "memory");

	/* step 11 Make ESP aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"sub $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");
}

/**
 * @brief case name: Benign exception while handling a prior exception_gp_ac_002
 *
 * Summary: At protected mode, the processor in #GP meeting a second #AC will be trigger a #AC.
 */
static void interrupt_rqmid_38254_gp_ac(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov    %eax,%cr4' instructions len
	 */
	execption_inc_len = 3;

	/* step 2, 3 is define irqcounter_incre */

	/* step 4 init by setup_idt (prepare the interrupt-gate descriptor of #AC)*/
	handle_exception(AC_VECTOR, &handled_32_exception);

	/* step 5 Construct interrupt gate descriptor of #PF(14th entry in IDT)
	 * with the Selector set to be 0x4b, DPL set to 3,
	 * the Offset point to #GP handler function
	 */
	handle_exception(GP_VECTOR, &handled_32_exception);
	set_idt_dpl(GP_VECTOR, 3);
	set_idt_sel(GP_VECTOR, USER_CS);

	/* step 6 Set CR0.AC[bit 18] to 1.*/
	cr0_am_to_1();
	/* step 7 Set RFLAGS.AC[bit 18] bit to 1*/
	eflags_ac_to_1();

	cr4_value_gp = read_cr4();
	cr4_value_gp |= CR4_RESERVED_BIT23;

	/*step 8 Switch cpl to ring3*/
	do_at_ring3(ring3_trigger_gp, "");

	/* step 12 Switch to ring 0*/
	/* step 13 Confirm only #AC exception has been generated*/
	report("%s %x %x", (irqcounter_query(AC_VECTOR) == 1),
		__FUNCTION__, irqcounter_query(AC_VECTOR), irqcounter_query(GP_VECTOR));

	/* resume environment */
	set_idt_dpl(GP_VECTOR, 0);
	set_idt_sel(GP_VECTOR, read_cs());
	execption_inc_len = 0;
}

static u8 *p_pf;
void ring3_trigger_pf(const char *arg)
{
	/* step 11 Make ESP not aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"add $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");

	/*step 12 access the no preent address trigger #PF*/
	asm volatile("mov %[value], (%[p])\n\t"
			: : [value]"r"(1), [p]"r"(p_pf));

	/* step 13 Make ESP aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"sub $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");
}

/**
 * @brief case name: Benign exception while handling a prior exception_pf_ac_003
 *
 * Summary: At protected mode, the processor in #PF meeting a second #AC will be trigger a #AC.
 */
static void interrupt_rqmid_38257_pf_ac(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov    %eax,(%edx)' instructions len
	 */
	execption_inc_len = 2;

	/* step 2, 3 is define irqcounter_incre */

	/* step 4 init by setup_idt (prepare the interrupt-gate descriptor of #AC)*/
	handle_exception(AC_VECTOR, &handled_32_exception);

	/* step 5 Construct interrupt gate descriptor of #PF(14th entry in IDT)
	 * with the Selector set to be 0x4b, DPL set to 3,
	 * the Offset point to #GP handler function
	 */
	handle_exception(PF_VECTOR, &handled_32_exception);
	set_idt_dpl(PF_VECTOR, 3);
	set_idt_sel(PF_VECTOR, USER_CS);

	/* step 6 Set CR0.AC[bit 18] to 1.*/
	cr0_am_to_1();
	/* step 7 Set RFLAGS.AC[bit 18] bit to 1*/
	eflags_ac_to_1();

	/* setp 8 define one GVA1 points to one 1B memory block in the 4K page */
	p_pf = malloc(sizeof(u8));
	if (p_pf == NULL) {
		printf("malloc error!\n");
		goto ret;
	}

	/* setp 9 set the 4k page PTE entry is not present */
	set_page_control_bit((void *)p_pf, PAGE_PTE, PAGE_P_FLAG, 0, true);

	/*step 10 Switch cpl to ring3*/
	do_at_ring3(ring3_trigger_pf, "");

	/* step 14 Switch to ring 0*/
	/* step 15 Confirm only #AC exception has been generated*/
	report("%s %x %x", (irqcounter_query(AC_VECTOR) == 1),
		__FUNCTION__, irqcounter_query(AC_VECTOR), irqcounter_query(PF_VECTOR));

	ret:
	/* resume environment */
	set_idt_dpl(PF_VECTOR, 0);
	set_idt_sel(PF_VECTOR, read_cs());
	execption_inc_len = 0;
}

void ring3_trigger_df(const char *arg)
{
	/* step 9 Make ESP not aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"add $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");

	/* step 10 Execute INT 8*/
	asm volatile(
		"INT $"xstr(DF_VECTOR)"\n\t");

	/* step 11 Make ESP aligned in memory*/
	asm volatile (
		"mov %%esp, %%eax\n\t"
		"sub $1, %%eax\n\t"
		"mov %%eax, %%esp\n\t"
		:::"eax");
}


/**
 * @brief case name: Benign exception while handling a prior exception_df_ac_004
 *
 * Summary: At protected mode, the processor in #DF meeting a second #AC will be trigger a #AC.
 */
static void interrupt_rqmid_38258_df_ac(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'int $0xe0' instructions len
	 */
	execption_inc_len = 2;

	/* step 2, 3 is define irqcounter_incre */

	/* step 4 init by setup_idt (prepare the interrupt-gate descriptor of #AC)*/
	handle_exception(AC_VECTOR, &handled_32_exception);

	/* step 5 Construct interrupt gate descriptor of #DF
	 * with the Selector set to be 0x4b, DPL set to 3,
	 * the Offset point to interrupt 0xE0 handler function
	 */
	handle_exception(DF_VECTOR, &handled_32_exception);
	set_idt_dpl(DF_VECTOR, 3);
	set_idt_sel(DF_VECTOR, USER_CS);

	/* step 6 Set CR0.AC[bit 18] to 1.*/
	cr0_am_to_1();
	/* step 7 Set RFLAGS.AC[bit 18] bit to 1*/
	eflags_ac_to_1();

	/*step 8 Switch cpl to ring3*/
	do_at_ring3(ring3_trigger_df, "");

	/* step 12 Switch to ring 0*/

	/* step 13 Confirm only #AC exception has been generated*/
	report("%s %x %x", (irqcounter_query(AC_VECTOR) == 1),
		__FUNCTION__, irqcounter_query(AC_VECTOR), irqcounter_query(DF_VECTOR));

	/* resume environment */
	set_idt_dpl(DF_VECTOR, 0);
	set_idt_sel(DF_VECTOR, read_cs());
	execption_inc_len = 0;
}


/**
 * @brief case name: Expose interrupt and exception handling data structure_004
 *
 * Summary: At protected mode, set EFLAGS.IF bit, trigger a external interrupt,
 * if this external interrupt entry is an interrupt-gate descriptor,
 * in the interrupt handling function, the RFLAGS.IF shall clear automatically.
 */
static void interrupt_rqmid_38165_data_structure_004(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* step 2 prepare the interrupt handler of INT 0xe0.*/
	/* step 3 prepare the interrupt-gate descriptor of INT 0xe0*/
	handle_irq(EXTEND_INTERRUPT_80, handled_interrupt_external_80);

	/* step 4*/
	sti();

	/* step 5 */
	asm volatile("INT $"xstr(EXTEND_INTERRUPT_80)"\n\t");

	/* step 6*/
	report("%s %lx %lx %d", ((irqcounter_query(EXTEND_INTERRUPT_80) == 1)
		&& (((save_rflags1>>9) & 0x1) == 0x1) && (((save_rflags2>>9) & 0x1) == 0x0)),
		__FUNCTION__, save_rflags1, save_rflags2, irqcounter_query(EXTEND_INTERRUPT_80));
}

/**
 * @brief case name: Expose interrupt and exception handling data structure_005
 *
 * Summary: At protected mode, set EFLAGS.IF bit, trigger a external interrupt,
 * if this external interrupt entry is an trap-gate descriptor,
 * in the interrupt handling function, the RFLAGS.IF shall keep set.
 */
static void interrupt_rqmid_38166_data_structure_005(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* step 2 prepare the interrupt handler of INT 0xe0.*/
	/* step 3 prepare the trap-gate descriptor of INT 0xe0*/
	handle_irq(EXTEND_INTERRUPT_80, handled_interrupt_external_80);
	set_idt_type(EXTEND_INTERRUPT_80, 0xF);

	/* step 4*/
	sti();

	/* step 5 */
	asm volatile("INT $"xstr(EXTEND_INTERRUPT_80)"\n\t");

	/* step 6*/
	report("%s %lx %lx %d", ((irqcounter_query(EXTEND_INTERRUPT_80) == 1)
		&& (((save_rflags1>>9) & 0x1) == 0x1) && (((save_rflags2>>9) & 0x1) == 0x1)),
		__FUNCTION__, save_rflags1, save_rflags2, irqcounter_query(EXTEND_INTERRUPT_80));
}


struct check_regs_32 {
	unsigned long CR0, CR2, CR3, CR4;
	//unsigned long EIP;
	//unsigned long EFLAGS;
	unsigned long EAX, EBX, ECX, EDX;
	unsigned long EDI, ESI;
	unsigned long EBP, ESP;
	unsigned long CS, DS, ES, FS, GS, SS;
};

struct check_regs_32 regs_check[2];

#define DUMP_REGS1(p_regs)	\
{					\
	asm volatile ("mov %%cr0, %0" : "=d"(p_regs.CR0) : : "memory");	\
	asm volatile ("mov %%cr2, %0" : "=d"(p_regs.CR2) : : "memory");	\
	asm volatile ("mov %%cr3, %0" : "=d"(p_regs.CR3) : : "memory");	\
	asm volatile ("mov %%cr4, %0" : "=d"(p_regs.CR4) : : "memory");	\
	asm volatile ("mov %%eax, %0" : "=m"(p_regs.EAX) : : "memory");	\
	asm volatile ("mov %%ebx, %0" : "=m"(p_regs.EBX) : : "memory");	\
}

#define DUMP_REGS2(p_regs)	\
{					\
	asm volatile ("mov %%ecx, %0" : "=m"(p_regs.ECX) : : "memory");	\
	asm volatile ("mov %%edx, %0" : "=m"(p_regs.EDX) : : "memory");	\
	asm volatile ("mov %%edi, %0" : "=m"(p_regs.EDI) : : "memory");	\
	asm volatile ("mov %%esi, %0" : "=m"(p_regs.ESI) : : "memory");	\
	asm volatile ("mov %%ebp, %0" : "=m"(p_regs.EBP) : : "memory");	\
	asm volatile ("mov %%esp, %0" : "=m"(p_regs.ESP) : : "memory");	\
	asm volatile ("mov %%cs, %0" : "=m"(p_regs.CS));				\
	asm volatile ("mov %%ds, %0" : "=m"(p_regs.DS));				\
	asm volatile ("mov %%es, %0" : "=m"(p_regs.ES));				\
	asm volatile ("mov %%fs, %0" : "=m"(p_regs.FS));				\
	asm volatile ("mov %%gs, %0" : "=m"(p_regs.GS));				\
	asm volatile ("mov %%ss, %0" : "=m"(p_regs.SS));				\
}

void print_all_regs(struct check_regs_32 *p_regs)
{
	printf("0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CR0, p_regs->CR2, p_regs->CR3, p_regs->CR4);
	printf("0x%lx 0x%lx 0x%lx 0x%lx\n", p_regs->EAX, p_regs->EBX, p_regs->ECX, p_regs->EDX);
	printf("0x%lx 0x%lx\n", p_regs->EDI, p_regs->ESI);
	printf("0x%lx 0x%lx\n", p_regs->EBP, p_regs->ESP);
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CS, p_regs->DS, p_regs->ES, p_regs->FS, p_regs->GS, p_regs->SS);
}

int check_all_regs()
{
	int ret;

	ret = memcmp(&regs_check[0], &regs_check[1], sizeof(struct check_regs_32));
	if (ret == 0) {
		return 0;
	}

	print_all_regs(&regs_check[0]);
	print_all_regs(&regs_check[1]);
	return 1;
}

/**
 * @brief case name: Expose exception and interrupt handling_OF_001
 *
 * Summary:At protected mode, set EFLAGS.OF to 1,
 * execute INTO instruction will trigger #OF exception and call
 * the #OF handler, program state should be unchanged, the saved contents of EIP
 * registers point to the instruction that generated the exception.
 */
static void interrupt_rqmid_38259_expose_exception_of_001(void)
{
	ulong eflag;
	ulong eflag_rf;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * #OF is a trap execption, so, not need add instruction len.
	 */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #BP. */
	handle_exception(OF_VECTOR, &handled_32_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #OF)*/

	/* step 4 init rip array and rip_index*/
	memset(eip, 0, sizeof(eip));
	eip_index = 0;

	/* step 5  push eflag to stack, the eflag is set OF*/
	eflag = read_rflags();
	eflag_rf = eflag | RFLAG_OF_BIT;
	asm volatile("push %0\n"
		::"m"(eflag_rf));

	/* step 6  execute POPF to set RFLAGS.OF to 1*/
	asm volatile("popf");

	/* step 7 dump CR and eax ebx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 execute INTO instruction trigger #OF*/
	asm volatile("into");

	/* step 9 dump CR and eax ebx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 execute INTO instruction trigger #OF*/
	asm volatile("into");

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 execute INTO instruction trigger #OF*/
	asm volatile("into");

	/*step 14 get setp 13 'int3' instruction address */
	asm volatile("call get_current_eip");
	/* 5 is call get_current_eip instruction len*/
	current_eip -= 5;


	/* step 15 check #OF exception, program state*/
	check = check_all_regs();

	debug_print("%x %x %x %x\n", irqcounter_query(OF_VECTOR), check,
		current_eip, eip[2]);
	report("%s", ((irqcounter_query(OF_VECTOR) == 6) && (check == 0)
		&& (current_eip == eip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_OF_002
 *
 * Summary:Set EFLAGS.OF to 1, execute INT3 instruction will trigger #OF exception and
 * call the #OF handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38260_expose_exception_of_002(void)
{
	ulong eflag;
	ulong eflag_rf;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/*length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #OF. */
	handle_exception(OF_VECTOR, &handled_32_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #OF)*/

	/* step 4  push eflag to stack, the eflag is set OF*/
	eflag = read_rflags();
	eflag_rf = eflag | RFLAG_OF_BIT;
	asm volatile("push %0\n"
		::"m"(eflag_rf));

	/* step 5  execute POPF to set EFLAGS.OF to 1*/
	asm volatile("popf");

	/*setp 6 execute INTO instruction trigger #OF*/
	asm volatile("into");

	check = check_rflags();

	/* step 7 check #OF exception and eflag*/
	report("%s", ((irqcounter_query(OF_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_TS_001
 *
 * Summary:At 64bit mode, will trigger #TS
 * exception and call the #TS handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38296_expose_exception_ts_001(void)
{
	int check = 0;
	void *addr = get_idt_addr(TS_VECTOR);

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #TS. */
	handle_exception(TS_VECTOR, &handled_32_exception);
	//handle_exception(GP_VECTOR, &handled_32_exception);
	set_idt_addr(TS_VECTOR, &ts_fault_soft);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #TS)*/

	/* step 4 init rip array and rip_index*/
	memset(eip, 0, sizeof(eip));
	eip_index = 0;
#if 0
	/* step 5 initialize NEWTSS */
	/* step 6 construct a segment descriptor in GDT 4th entry (selector 0x20)
	 * of OLDGDTR with P bit set to 1, Type set to 0x9, S bit set to 0,
	 * BASE set to NEWTSS address, LIMIT set to 0x67.
	 */
	setup_tss32();
	/* RPL of the stack segment selector in the TSS is not equal to the DPL of the
	 * code segment being accessed by the interrupt or trap gate
	 */
	tss_intr.cs = 0x8 + 0x3;
	tss_intr.eip = (u32)irq_tss;

	/*step 7 construct a task gate descriptor in IDT 0x80th entry */
	set_idt_task_gate(0x80, TSS_INTR);
#endif
	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
#if 0
	/*step 8 execute INT to trigger #TS*/
	asm volatile("INT $0x80\n\t");
#else
	asm volatile("INT $10");
#endif
	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

#if 0
	/*step 11 execute INT to trigger #TS*/
	asm volatile("INT $0x80\n\t");
#else
	asm volatile("INT $10");
#endif

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

#if 0
	/*step 13 execute INT to trigger #TS*/
	asm volatile("INT $0x80\n\t");
#else
	asm volatile("INT $10");
#endif

	/*step 14 get setp 13 'INT' instruction exception address */
	asm volatile("call get_current_eip");
	/* 5 is call get_current_rip instruction len*/
	current_eip -= 5;

	debug_print("%x %x %x %x\n", irqcounter_query(TS_VECTOR), check,
		current_eip, eip[2]);

	/* step 15 check #TS exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(TS_VECTOR) == 6) && (check == 0)
		&& (current_eip == eip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_addr(TS_VECTOR, addr);
}

/**
 * @brief case name:Expose exception and interrupt handling_TS_002
 *
 * Summary:At 64bit mode, will trigger #TS
 * exception and call the #TS handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38298_expose_exception_ts_002(void)
{
	int check = 0;
	void *addr = get_idt_addr(TS_VECTOR);

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/*length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #TS. */
	handle_exception(TS_VECTOR, &handled_32_exception);
	set_idt_addr(TS_VECTOR, &ts_fault_soft);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #TS)*/
#if 0
	/* step 4 initialize NEWTSS */
	/* step 5 construct a segment descriptor in GDT 4th entry (selector 0x20)
	 * of OLDGDTR with P bit set to 1, Type set to 0x9, S bit set to 0,
	 * BASE set to NEWTSS address, LIMIT set to 0x67.
	 */
	setup_tss32();
	/* RPL of the stack segment selector in the TSS is not equal to the DPL of the
	 * code segment being accessed by the interrupt or trap gate
	 */
	//tss_intr.ss = 0x8;
	tss_intr.cs = 0x2;
	tss_intr.eip = (u32)irq_tss;

	/*step 6 construct a task gate descriptor in IDT 0x80th entry */
	set_idt_task_gate(0x80, TSS_INTR);
#endif

	/*step 7 execute INT to trigger #TS*/
#if 0
	asm volatile("INT $0x80\n\t");
#else
	asm volatile("INT $10");
#endif

	check = check_rflags();

	/* step 8 check #TS exception and rflag*/
	report("%s", ((irqcounter_query(TS_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_addr(TS_VECTOR, addr);
}

static void print_case_list_32(void)
{
	printf("\t\t Interrupt feature 32-Bits Mode case list:\n\r");

	printf("Case ID:%d case name:%s\n\r", 36246,
		"Interrupt to non-present descriptor_002");
	printf("Case ID:%d case name:%s\n\r", 36133,
		"Contributory exception while handling a prior contributory exception_001");
	printf("Case ID:%d case name:%s\n\r", 38046,
		"Contributory exception while handling a prior contributory exception_002");
	printf("Case ID:%d case name:%s\n\r", 38071,
		"Benign exception while handling a prior exception_intn_ac_001");
	printf("Case ID:%d case name:%s\n\r", 38254,
		"Benign exception while handling a prior exception_gp_ac_002");

	printf("Case ID:%d case name:%s\n\r", 38257,
		"Benign exception while handling a prior exception_pf_ac_003");
	printf("Case ID:%d case name:%s\n\r", 38258,
		"Benign exception while handling a prior exception_df_ac_004");
	printf("Case ID:%d case name:%s\n\r", 38165,
		"Expose interrupt and exception handling data structure_004");
	printf("Case ID:%d case name:%s\n\r", 38166,
		"Expose interrupt and exception handling data structure_005");
	printf("Case ID:%d case name:%s\n\r", 38259,
		"Expose exception and interrupt handling_OF_001");

	printf("Case ID:%d case name:%s\n\r", 38260,
		"Expose exception and interrupt handling_OF_002");
	printf("Case ID:%d case name:%s\n\r", 38296,
		"Expose exception and interrupt handling_TS_001");
	printf("Case ID:%d case name:%s\n\r", 38298,
		"Expose exception and interrupt handling_TS_002");
#ifdef IN_NON_SAFETY_VM
	printf("Case ID:%d case name:%s\n\r", 36128,
		"Contributory exception while handling #DF in non-safety VM_001");
#endif
#ifdef IN_NATIVE
	printf("Case ID:%d case name:%s\n\r", 36132,
		"Contributory exception while handling #DF in safety VM_001");
#endif
}

static void test_interrupt_32(void)
{
	/* __i386__ */
	interrupt_rqmid_36246_no_present_descriptor_002();
	interrupt_rqmid_36133_ss_gp();
	interrupt_rqmid_38046_pf_np();
	interrupt_rqmid_38071_intn_ac();
	interrupt_rqmid_38254_gp_ac();

	interrupt_rqmid_38257_pf_ac();
	interrupt_rqmid_38258_df_ac();
	interrupt_rqmid_38165_data_structure_004();
	interrupt_rqmid_38166_data_structure_005();
	interrupt_rqmid_38259_expose_exception_of_001();

	interrupt_rqmid_38260_expose_exception_of_002();
	interrupt_rqmid_38298_expose_exception_ts_002();
	interrupt_rqmid_38296_expose_exception_ts_001();
#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_36128_df_ts();	//non-safety VM shutdowns	not ok
#endif
#ifdef IN_SAFETY_VM
	interrupt_rqmid_36132_df_ts();	//safety call bsp_fatal_error()
#endif

}
