
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

#if 0
static void task_gate_g_limit_ts(void)
{
	setup_tss32();
	gdt32[TSS_INTR>>3].limit_low = 0x66;
	set_idt_task_gate(DF_VECTOR, TSS_INTR);

	asm volatile(//ASM_TRY("1f")
			"int $"xstr(DF_VECTOR)"\n\t"
			//"1:"
			:::);

	report("%s", (exception_vector() == TS_VECTOR), __FUNCTION__);
}
#endif

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

	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	regs->rip += execption_inc_len;
}

static void interru_df_ts(const char *fun)
{
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov (%%rax), %%rbx' instructions len
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
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), 0x200);

	/* step 7 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 8 prepare the task-gate descriptor of #DF */
	set_idt_task_gate(DF_VECTOR, TSS_INTR);

	/* step 9 prepare the interrupt-gate descriptor of #PF.
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
}

/**
 * @brief case name: Contributory exception while handling #DF in non-safety VM_001
 *
 * Summary: When a vCPU of the non-safety VM detected a second #TS exception while
 * calling the exception handler for a prior #DF, ACRN hypervisor shall guarantee that
 * any vCPU of the non-safety VM stops executing any instructions(VM shutdowns).
 */
__attribute__((unused)) static void interrupt_rqmid_36128_df_ts(void)
{
	interru_df_ts(__FUNCTION__);
}

/**
 * @brief case name: Contributory exception while handling #DF in safety VM_001
 *
 * Summary: When a vCPU of the safety VM detected a second contributory
 * exception(#TS) while calling the exception handler for a prior #DF,
 * ACRN hypervisor shall call bsp_fatal_error().
 */
__attribute__((unused)) static void interrupt_rqmid_36132_df_ts(void)
{
	interru_df_ts(__FUNCTION__);
}

struct lseg_st {
	u32  offset;
	int16_t selector;
};

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
 * Summary: When a vCPU detected a second contributory exception(#GP) while
 * calling an exception handler for a prior contributory exception(#SS),
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

	/* setp 6 construct a data segment descriptor in 16th entry (selector 0x80) of GDT*/
	sgdt(&old_gdt_desc);
	set_gdt_entry(0x80, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* setp 7 reload  GDTR*/
	lgdt(&old_gdt_desc);


	/* setp 8 and step 9 switch to ring3, execute LSS to load the segment of not present constructed in step 6 */
	do_at_ring3(ring3_ss, "");

	/* step 10 confirm #DF exception has been generated*/
	report("%s", ((irqcounter_query(DF_VECTOR) == 1) && (save_error_code == 0)), __FUNCTION__);

	/* resume environment */
	set_idt_s_flag(SS_VECTOR, 0);
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
 * Summary: When a vCPU attempts to trigger  #AC exception via interrupt-gate,
 * if  P is not present, ACRN hypervisor shall guarantee that the vCPU
 * receives the #NP(0x8B) (index =17, IDT=1, EXT=1 )
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

	/* step 5 init by setup_idt */

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
	set_idt_present(DB_VECTOR, 1);
}

static void print_case_list_32(void)
{
	printf("\t\t Interrupt feature 32-Bits Mode case list:\n\r");

	printf("Case ID:%d case name:%s\n\r", 36246,
		"Interrupt to non-present descriptor_002");
	printf("Case ID:%d case name:%s\n\r", 36133,
		"Contributory exception while handling a prior contributory exception_001");
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
	//task_gate_g_limit_ts();
	interrupt_rqmid_36246_no_present_descriptor_002();
	interrupt_rqmid_36133_ss_gp();	//DF

#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_36128_df_ts();	//non-safety VM shutdowns	not ok
#endif
#ifdef IN_SAFETY_VM
	interrupt_rqmid_36132_df_ts();	//safety call bsp_fatal_error()
#endif
}
