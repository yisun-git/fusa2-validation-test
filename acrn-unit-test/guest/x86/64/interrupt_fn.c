/* used to save rip every time an exception occurs continuously
 */
static int rip_index = 0;
static u64 rip[10] = {0, };
static volatile int current_case_id = 0;

void handled_exception(struct ex_regs *regs)
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

	if (rip_index < 10) {
		rip[rip_index++] = regs->rip;
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

static u64 current_rip;
void get_current_rip_function(void)
{
	u64 rsp_64 = 0;
	u64 *rsp_val;
	asm volatile("mov %%rdi, %0\n\t"
		: "=m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;

	current_rip = rsp_val[0];
}

asm("get_current_rip:\n"
	"mov %rsp, %rdi\n"
	"call get_current_rip_function\n"
	"ret\n"
);
void get_current_rip(void);

/**
 * @brief case name: Page fault while handling a prior page fault_001
 *
 * Summary: At 64bit mode, the processor in #PF meeting a second #PF will be trigger a #DF.
 */
static void interrupt_rqmid_24211_pf_pf(void)
{
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov (%%rax), %%rbx' instructions len
	 */
	execption_inc_len = 3;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(DF_VECTOR, &handled_exception);

	/* step 4 prepare NEWGDT*/
	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), PAGE_SIZE);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), PAGE_SIZE);

	/* step 5 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 6 prepare the interrupt-gate descriptor of #DF.*/
	/* IDT has been initialized. Only segment selector and handler are set here*/
	set_idt_sel(DF_VECTOR, 0x8);

	/* step 7 prepare the interrupt-gate descriptor of #PF.
	 * the selector of the #PF is on the second page((0x201<<3) = x01008).
	 * IDT has been initialized. Only segment selector is set here
	 */
	set_idt_sel(PF_VECTOR, 0x1008);

	/* step 8 prepare the not present page.*/
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);

	/* setp 9 trigger #PF exception*/
	asm volatile(
			"mov %0, %%rax\n\t"
			"add $0x1000, %%rax\n\t"
			"mov (%%rax), %%rbx\n\t"
			::"m"(linear_addr));

	/* step 10 confirm #DF exception has been generated*/
	report("%s", ((irqcounter_query(DF_VECTOR) == 1) && (save_error_code == 0)), __FUNCTION__);

	/* resume environment */
	set_idt_sel(DF_VECTOR, read_cs());
	set_idt_sel(PF_VECTOR, read_cs());
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 1, true);
	free(linear_addr);
	linear_addr = NULL;
	execption_inc_len = 0;
}

/**
 * @brief case name: Interrupt to non-present descriptor_001
 *
 * Summary: At 64bit mode, when a vCPU attempts to trigger  #BP exception
 * (type of the IDT descriptor is interrupt-gate) via INT3,
 * if CPL(0) < DPL(3) and P is not present, ACRN hypervisor shall
 * guarantee that the vCPU receives the #NP(0x1A) (index =3, IDT=1, EXT=0 )
 */
static void interrupt_rqmid_36241_no_present_descriptor_001(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'INT3' instructions len
	 */
	execption_inc_len = 1;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(NP_VECTOR, &handled_exception);

	/* step 4 prepare the interrupt-gate descriptor of #BP with DPL set to 0x3,
	 * P bit set to 0(not present)
	 */
	set_idt_present(BP_VECTOR, 0);
	set_idt_dpl(BP_VECTOR, 3);

	/* step 6 execute INT3 to trigger #BP*/
	asm volatile("INT3\n\t");

	/* step 7 confirm #NP exception has been generated*/
	report("%s", ((irqcounter_query(NP_VECTOR) == 1)
		&& (save_error_code == ((BP_VECTOR*8)+2))), __FUNCTION__);

	/* resume environment */
	set_idt_present(BP_VECTOR, 1);
	set_idt_dpl(BP_VECTOR, 0);
	execption_inc_len = 0;
}

struct check_regs_64 {
	unsigned long CR0, CR2, CR3, CR4, CR8;
	//unsigned long RIP;
	//unsigned long RFLAGS;
	unsigned long RAX, RBX, RCX, RDX;
	unsigned long RDI, RSI;
	unsigned long RBP, RSP;
	unsigned long R8, R9, R10, R11, R12, R13, R14, R15;
	unsigned long CS, DS, ES, FS, GS, SS;
};

struct check_regs_64 regs_check[2];

#define DUMP_REGS1(p_regs)	\
{					\
	asm volatile ("mov %%cr0, %0" : "=d"(p_regs.CR0) : : "memory");	\
	asm volatile ("mov %%cr2, %0" : "=d"(p_regs.CR2) : : "memory");	\
	asm volatile ("mov %%cr3, %0" : "=d"(p_regs.CR3) : : "memory");	\
	asm volatile ("mov %%cr4, %0" : "=d"(p_regs.CR4) : : "memory");	\
	asm volatile ("mov %%cr8, %0" : "=d"(p_regs.CR8) : : "memory");	\
	asm volatile ("mov %%rax, %0" : "=m"(p_regs.RAX) : : "memory");	\
	asm volatile ("mov %%rbx, %0" : "=m"(p_regs.RBX) : : "memory");	\
}

#define DUMP_REGS2(p_regs)	\
{					\
	asm volatile ("mov %%rcx, %0" : "=m"(p_regs.RCX) : : "memory");	\
	asm volatile ("mov %%rdx, %0" : "=m"(p_regs.RDX) : : "memory");	\
	asm volatile ("mov %%rdi, %0" : "=m"(p_regs.RDI) : : "memory");	\
	asm volatile ("mov %%rsi, %0" : "=m"(p_regs.RSI) : : "memory");	\
	asm volatile ("mov %%rbp, %0" : "=m"(p_regs.RBP) : : "memory");	\
	asm volatile ("mov %%rsp, %0" : "=m"(p_regs.RSP) : : "memory");	\
	asm volatile ("mov %%r8, %0" : "=m"(p_regs.R8) : : "memory");	\
	asm volatile ("mov %%r9, %0" : "=m"(p_regs.R9) : : "memory");	\
	asm volatile ("mov %%r10, %0" : "=m"(p_regs.R10) : : "memory");	\
	asm volatile ("mov %%r11, %0" : "=m"(p_regs.R11) : : "memory");	\
	asm volatile ("mov %%r12, %0" : "=m"(p_regs.R12) : : "memory");	\
	asm volatile ("mov %%r13, %0" : "=m"(p_regs.R13) : : "memory");	\
	asm volatile ("mov %%r14, %0" : "=m"(p_regs.R14) : : "memory");	\
	asm volatile ("mov %%r15, %0" : "=m"(p_regs.R15) : : "memory");	\
	asm volatile ("mov %%cs, %0" : "=m"(p_regs.CS));				\
	asm volatile ("mov %%ds, %0" : "=m"(p_regs.DS));				\
	asm volatile ("mov %%es, %0" : "=m"(p_regs.ES));				\
	asm volatile ("mov %%fs, %0" : "=m"(p_regs.FS));				\
	asm volatile ("mov %%gs, %0" : "=m"(p_regs.GS));				\
	asm volatile ("mov %%ss, %0" : "=m"(p_regs.SS));				\
}

void print_all_regs(struct check_regs_64 *p_regs)
{
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CR0, p_regs->CR2, p_regs->CR3, p_regs->CR4, p_regs->CR8);
	printf("0x%lx 0x%lx 0x%lx 0x%lx\n", p_regs->RAX, p_regs->RBX, p_regs->RCX, p_regs->RDX);
	printf("0x%lx 0x%lx\n", p_regs->RDI, p_regs->RSI);
	printf("0x%lx 0x%lx\n", p_regs->RBP, p_regs->RSP);
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->R8, p_regs->R9, p_regs->R10, p_regs->R11,
		p_regs->R12, p_regs->R13, p_regs->R14, p_regs->R15);
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CS, p_regs->DS, p_regs->ES, p_regs->FS, p_regs->GS, p_regs->SS);
}

int check_all_regs()
{
	int ret;

	ret = memcmp(&regs_check[0], &regs_check[1], sizeof(struct check_regs_64));
	if (ret == 0) {
		return 0;
	}

	print_all_regs(&regs_check[0]);
	print_all_regs(&regs_check[1]);
	return 1;
}

/**
 * @brief case name: Expose exception and interrupt handling_GP_001
 *
 * Summary:At 64bit mode, writing Cr4 reserved bit will trigger #GP exception and call
 * the #GP handler with error code 0, program state should be unchanged,
 * the saved contents of EIP registers point to the instruction
 * that generated the exception.
 */
static void interrupt_rqmid_36254_expose_exception_gp_001(void)
{
	ulong cr4_value;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #GP. */
	handle_exception(GP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #GP)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	cr4_value = read_cr4();
	cr4_value |= CR4_RESERVED_BIT23;

	/* step 5 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/* step 6 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");
	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 8 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 9 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "a"(cr4_value) : "memory");
	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/* step 11 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");

	/*step 12 get 'mov %0, %%cr4' instruction address*/
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 3 is 'mov %rdx,%cr4' instruction len*/
	current_rip -= (5 + 3);

	debug_print("%lx %lx %lx %lx %lx\n", save_rflags1, save_rflags2,
		save_error_code, current_rip, rip[0]);

	/* step 13 check #GP exception, program state and error code*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(GP_VECTOR) == 6) && (check == 0)
		&& (save_error_code == 0) && (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_GP_002
 *
 * Summary: At 64bit mode, writing Cr4 reserved bit will trigger #GP exception and
 * call the #GP handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_36255_expose_exception_gp_002(void)
{
	ulong cr4_value;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #GP. */
	handle_exception(GP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #GP)*/

	/* step 4 wrirte to cr4 reserved bit*/
	cr4_value = read_cr4();
	cr4_value |= CR4_RESERVED_BIT23;
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");

	check = check_rflags();

	/* step 5 check #GP exception and rflag*/
	report("%s", ((irqcounter_query(GP_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_MF_001
 *
 * Summary:At 64bit mode, FPU exception(FPU excp: hold),
 * executing EMMS shall generate #MF exception and call the #MF
 * handler with error code 0, program state should be unchanged,
 * the saved contents of EIP registers point to the instruction
 * that generated the exception.
 */
static void interrupt_rqmid_36701_expose_exception_mf_001(void)
{
	float op1 = 1.2;
	float op2 = 0;
	ulong cr0 = read_cr0();
	unsigned short cw = 0x37b;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'emms' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #MF. */
	handle_exception(MF_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #MF)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 clear cr0.EM[bit 2]*/
	write_cr0(read_cr0() & ~CR0_EM_BIT);

	/* step 6 set cr0.ne[bit 5]*/
	write_cr0(read_cr0() | CR0_NE_BIT);

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/* step 8 and 9 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));
	/* step 10 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 11 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 12 and 13 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));
	/* step 14 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/* step 15 and 16 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));

	/*step 17 get step 16 'emms' instruction address*/
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 2 is 'emms' instruction len*/
	current_rip -= (5 + 2);

	debug_print("%lx %lx %lx %lx %lx\n", save_rflags1, save_rflags2,
		save_error_code, current_rip, rip[0]);

	/* step 18 check #MF, program state register, error code */
	check = check_all_regs();
	report("%s", ((irqcounter_query(MF_VECTOR) == 6) && (check == 0)
		&& (save_error_code == 0) && (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	write_cr0(cr0);
}

/**
 * @brief case name:Expose exception and interrupt handling_mf_002
 *
 * Summary: At 64bit mode, FPU exception(FPU excp: hold),
 * executing EMMS will trigger #GP exception and call the #MF handler,
 *  EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_36702_expose_exception_mf_002(void)
{
	int check = 0;
	float op1 = 1.2;
	float op2 = 0;
	ulong cr0 = read_cr0();
	unsigned short cw = 0x37b;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'emms' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #MF. */
	handle_exception(MF_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #MF)*/

	/* step 4 clear cr0.EM[bit 2]*/
	write_cr0(read_cr0() & ~CR0_EM_BIT);

	/* step 5 set cr0.ne[bit 5]*/
	write_cr0(read_cr0() | CR0_NE_BIT);

	/* step 6 and 7 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));

	/* TF[bit 8], NT[bit 14], RF[bit 16], VM[bit 17] */
	if ((save_rflags2 & RFLAG_TF_BIT) || (save_rflags2 & RFLAG_NT_BIT)
		|| (save_rflags2 & RFLAG_RF_BIT) || (save_rflags2 & RFLAG_VM_BIT)) {
		check = 1;
	}
	/* step 8  check #MF exception and rflag*/
	report("%s", ((irqcounter_query(MF_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	write_cr0(cr0);
}

//test fail test result: first #UD, second interrupt
/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P4_P7_001
 *
 * Summary: At 64bit mode, arm a TSC deadline timer with 0x1000 periods increament
 * (when the deadline timer expires, an 0xE0 interrupt is generated).
 * Immediately execute RDPMC instruction to trigger a VM-exit.
 * This will generate an interrupt and #UD simultaneously after VM-exit finished.
 * The 0xE0 interrupt should be serviced first, and the #UD should be serviced later.
 */
static void interrupt_rqmid_36689_p4_p7_001(void)
{
	u32 a;
	u32 d;
	u32 index = 9;
	int i = 1;
	__unused u64 t[2];

	while (i--) {
		/* step 1 init g_irqcounter */
		irqcounter_initialize();

		/* length of the instruction that generated the exception
		 * 'rdpmc' instructions len
		 */
		execption_inc_len = 2;

		/* step 2 prepare the interrupt handler of #UD. */
		handle_exception(UD_VECTOR, &handled_exception);

		/* step 3 init by setup_idt (prepare the interrupt-gate descriptor of #UD)*/

		/* step 4 is define handled_interrupt_external_e0*/
		/* setp 5 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

		/* step 6 enable interrupt*/
		irq_enable();

		/* step 7 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24)) {
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);
			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x1000);
		}

		/* step 8 trigger VM-exit and generate #UD*/
		t[0] = asm_read_tsc();
		asm volatile ("rdpmc" : "=a"(a), "=d"(d) : "c"(index));	/*0x1f00 tsc */
		t[1] = asm_read_tsc();

		/* step 9 enable interrupt*/
		irq_disable();
		debug_print("%s %d %d %d %d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(UD_VECTOR), t[1] - t[0]);

		test_delay(1);
	}

	/* step 10  first interrupt, second #UD */
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_E0) == 1)
		&& (irqcounter_query(UD_VECTOR) == 2)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

//test fail test result: first #GP, second interrupt
/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P4_P8_001
 *
 * Summary: At 64bit mode, arm a TSC deadline timer with 0x1000 periods increament
 * (when the deadline timer expires, an 0xE0 interrupt is generated).
 * Immediately execute RDMSR instruction with an invalid MSR to trigger a VM-exit.
 * This will generate an interrupt and #GP simultaneously after VM-exit finished.
 * The 0xE0 interrupt should be serviced first, and the #GP should be serviced later.
 */
static void interrupt_rqmid_36691_p4_p8_001(void)
{
	int i = 1;
	__unused u64 t[2];

	while (i--) {
		/* step 1 init g_irqcounter */
		irqcounter_initialize();

		/* length of the instruction that generated the exception
		 * 'rdmsr' instructions len
		 */
		execption_inc_len = 2;

		/* step 2 prepare the interrupt handler of #GP. */
		handle_exception(GP_VECTOR, &handled_exception);

		/* step 3 init by setup_idt (prepare the interrupt-gate descriptor of #GP)*/

		/* step 4 is define handled_interrupt_external_e0*/
		/* setp 5 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

		/* step 6 enable interrupt*/
		irq_enable();

		/* step 7 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24))
		{
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);
			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x1000);
		}

		/* step 8 trigger VM-exit and generate #GP*/
		t[0] = asm_read_tsc();
		asm volatile ("rdmsr" :: "c"(0xD20) : "memory"); /*0x253a900 tsc*/
		t[1] = asm_read_tsc();

		/* step 9 disable interrupt*/
		irq_disable();
		debug_print("%s %d %d %d %d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(GP_VECTOR), t[1]-t[0]);
		test_delay(1);
	}

	/* step 10 first interrupt, second #GP */
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_E0) == 1)
		&& (irqcounter_query(GP_VECTOR) == 2)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P3_P4_001
 *
 * Summary: At 64bit mode, sync vCPU1 and vCPU2 so that vCPU1 arm a TSC deadline timer
 * with 0x2500 clock periods increament and vCPU2 send NMI IPI to vCPU1 concurrently.
 * On vCPU1, immediately execute an instruction which would trigger a VM-exit.
 * This will generate an NMI and specified interrupt simultaneously
 * after VM-exit finished. #NMI should be serviced first,
 * and the specified interrupt should be serviced later.
 */
volatile int interrupt_wait_bp = 0;
static void interrupt_rqmid_36693_p3_p4_ap_001(void)
{
	u64 t_ap;
	int i = 1;
	if (get_lapic_id() != 1) {
		return;
	}

	while (i--) {
		/* setp 1*/
		while (interrupt_wait_bp == 0) {
			asm volatile ("nop\n\t" :::"memory");
		}

		/* wait bp vm_exit to host */
		t_ap = asm_read_tsc();
		while (asm_read_tsc() < (t_ap + 0x500)) {
			asm volatile ("nop\n\t" :::"memory");
		}
		/* setp 10 AP send NMI to BP*/
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT, 0);	/*0x1A00 tsc*/
		interrupt_wait_bp = 0;
	}
}

static __unused void interrupt_rqmid_36693_p3_p4_001(void)
{
	__unused u64 t[2];
	int i = 1;

	current_case_id = 36693;
	while (i--) {
		/* step 2 init g_irqcounter */
		irqcounter_initialize();

		/* length of the instruction that generated the exception
		 * NMI is ap send to bp, so execption instruction set to 0
		 */
		execption_inc_len = 0;

		/* step 3 prepare the interrupt handler of #NMI)*/
		handle_exception(NMI_VECTOR, &handled_exception);

		/* step 4 init by setup_idt (prepare the interrupt-gate descriptor of #NMI)*/

		/* step 5 is define handled_interrupt_external_e0*/
		/* setp 6 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

		/* step 7 enable interrupt*/
		irq_enable();

		/* step 8 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24))
		{
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);

			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x2500));	/*0x200 tsc*/

			/* step 9 BP sends synchronization message to AP*/
			interrupt_wait_bp = 1;
		}

		t[0] = asm_read_tsc();
		/* step 10 BP trigger VM-exit*/
		cpuid_indexed(0x16, 0);	/*0x5000 tsc*/
		t[1] = asm_read_tsc();

		/* step 11 disable interrupt*/
		irq_disable();
		debug_print("%s %d %d %d %d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(NMI_VECTOR), t[1] - t[0]);
		test_delay(1);
	}

	/* step 12 first #NMI, second interrupt*/
	report("%s", ((irqcounter_query(NMI_VECTOR) == 1)
		&& (irqcounter_query(EXTEND_INTERRUPT_E0) == 2)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_001
 *
 * Summary: At 64bit mode, trigger a fault-class exception #UD,
 * check that the value of EFLAGS.RF pushed on the stack is 1.
 */
static void interrupt_rqmid_36694_expose_instruction_breakpoints_001(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'UD2' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #UD. */
	handle_exception(UD_VECTOR, &handled_exception);

	/* step 3 init by setup_idt (prepare the interrupt-gate descriptor of #UD)*/

	/* step 4 execute UD2 */
	asm volatile("UD2\n");

	debug_print("%lx %lx\n", save_rflags1, save_rflags2);
	/* step 5 confirm #UD exception and EFLAGS.RF */
	if ((save_rflags1 & RFLAG_RF_BIT)) {
		check = 1;
	}
	report("%s", ((irqcounter_query(UD_VECTOR) == 1) && (check == 1)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_005
 *
 * Summary: At 64bit mode,
 * 1. PUSH two EFLAGS values to the stack, the first
 * clear TF flag and the second set TF flag.
 * 2. Set EFLAGS.TF via POPF instruction, entry single-step mode,
 * execute 3 NOP instructions. A first #GP(0) should be generated
 * with the EIP pointing to the 2nd NOP instruction. A second #GP(0)
 * should be generated with the EIP pointing to the 3rd NOP instruction.
 * 3. Clear EFLAGS.TF via POPF instruction, stop singe-step mode,
 * check that a #GP(0) is captured, the rip value at the time of
 * exception is the same as expected
 */
static void interrupt_rqmid_36695_expose_instruction_breakpoints_005(void)
{
	int i;
	u64  rflag = 0;
	u64  rflag_tf = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * single-step trap does not need to handle instruction length
	 */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #GP. */
	handle_exception(GP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt (prepare the interrupt-gate descriptor of #GP)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5  push 2 rflag to stack, first rflag is not set TF, second rflag is set TF*/
	rflag = read_rflags();
	rflag_tf = rflag | RFLAG_TF_BIT;
	debug_print("%s %d rflag = 0x%lx rflag_tf = 0x%lx\n", __FUNCTION__, __LINE__, rflag, rflag_tf);
	debug_print("rip_index=%d\n", rip_index);
	for (i = 0; i < 10; i++) {
		debug_print("rip%d:0x%lx\n", i, rip[i]);
	}
	asm volatile("push %0\n"
		::"m"(rflag));
	asm volatile("push %0\n"
		::"m"(rflag_tf));

	/* step 6  pop rflag entry single-step trap*/
	asm volatile("popf");

	/*step 7 nop */
	asm volatile("nop");
	asm volatile("nop");

	/*step 8 pop rflag*/
	asm volatile("popf");

	/*step 9 nop*/
	asm volatile("nop");

	/*step 10 get second nop instruction address*/
	asm volatile("call get_current_rip");
	/* 5 + 1 + 1 + 1 is call + nop + popf + nop instruction len */
	current_rip -= (5 + 1 + 1 + 1);

	debug_print("rip_index=%d\n", rip_index);
	for (i = 0; i < 10; i++) {
		debug_print("rip%d:0x%lx\n", i, rip[i]);
	}

	debug_print("vector:0x%x current_rip= 0x%lx rip[0]=0x%lx, rip[1]=0x%lx, rip[2]=0x%lx\n",
		irqcounter_query(GP_VECTOR), current_rip, rip[0], rip[1], rip[2]);

	/* step 11 check #GP(3 #GP 1+2+3) and rip array */
	report("%s", ((irqcounter_query(GP_VECTOR) == 6) && (current_rip == rip[0]) &&
		((rip[0]+1) == rip[1]) && ((rip[1]+1) == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

struct descriptor_table_32_ptr {
	u16 limit;
	u32 base;
} __attribute__((packed));

/**
 * @brief case name:IDTR_following_start-up_001
 *
 * Summary: At 64bit mode, execute SIDT instruction to store content in
 * IDTR at BP start-up, the initial value is 00000000FFFFH.
 */
static void interrupt_rqmid_24028_idtr_startup(void)
{
	volatile struct descriptor_table_32_ptr *ptr;

	ptr = (volatile struct descriptor_table_32_ptr *)STARTUP_IDTR_ADDR;

	report("%s base=%x limit=%x", ((ptr->base == 0x0) && (ptr->limit == 0xFFFF)),
		__FUNCTION__, ptr->base, ptr->limit);
}

/**
 * @brief case name:IDTR_following_INIT_001
 *
 * Summary: At 64bit mode, execute SIDT instruction to store content
 * in IDTR at AP init, the initial value is 00000000FFFFH.
 */
static void __unused interrupt_rqmid_23981_idtr_init(void)
{
	volatile struct descriptor_table_32_ptr *ptr;

	ptr = (volatile struct descriptor_table_32_ptr *)INIT_IDTR_ADDR;

	report("%s base=%x limit=%x", ((ptr->base == 0x0) && (ptr->limit == 0xFFFF)),
		__FUNCTION__, ptr->base, ptr->limit);
}

static void interru_df_pf(const char *fun)
{
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov (%%rax), %%rbx' instructions len
	 */
	execption_inc_len = 3;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(DF_VECTOR, &handled_exception);

	/* step 4 prepare NEWGDT*/
	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), PAGE_SIZE);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), PAGE_SIZE);

	/* step 5 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 6 prepare the interrupt-gate descriptor of #DF.*/
	/* IDT has been initialized. Only segment selector are set here*/
	set_idt_sel(DF_VECTOR, 0x1010);

	/* step 7 prepare the interrupt-gate descriptor of #PF.
	 * the selector of the #PF is on the second page((0x201<<3) = x01008).
	 * IDT has been initialized. Only segment selector is set here
	 */
	set_idt_sel(PF_VECTOR, 0x1008);

	/* step 8 prepare the not present page.*/
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);

	/* setp 9 trigger #PF exception*/
	asm volatile(
			"mov %0, %%rax\n\t"
			"add $0x1000, %%rax\n\t"
			"mov (%%rax), %%rbx\n\t"
			::"m"(linear_addr));

	/* step 10  report shutdown vm, Control should not come here */
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
 * @brief case name: Page fault while handling #DF in non-safety VM_001
 *
 * Summary: At 64bit mode, when a vCPU of the non-safety VM detected
 * a second contributory exception(#PF) while calling the exception
 * handler for a prior #DF, ACRN hypervisor shall guarantee that any
 * vCPU of the non-safety VM stops executing any instructions(VM shutdowns).
 */
__attribute__((unused)) static void interrupt_rqmid_38001_df_pf(void)
{
	interru_df_pf(__FUNCTION__);
}

/**
 * @brief case name: Page fault while handling #DF in safety VM_001
 *
 * Summary: At 64bit mode, when a vCPU of the safety VM detected a second
 * contributory exception(#PF) while calling the exception handler for a prior #DF,
 * ACRN hypervisor shall call bsp_fatal_error().
 */
__attribute__((unused)) static void interrupt_rqmid_38002_df_pf(void)
{
	interru_df_pf(__FUNCTION__);
}

/**
 * @brief case name: Contributory exception while handling a prior benign exception_001
 *
 * Summary: At 64bit mode, when a vCPU detected a second contributory exception(#NP) while
 * calling an exception handler for a prior contributory exception(#BP),
 * ACRN hypervisor shall guarantee that the vCPU receives #NP.
 */
static void interrupt_rqmid_38047_bp_np(void)
{
	u64  rflag = 0;
	u64  rflag_rf = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'int3' instructions len
	 */
	execption_inc_len = 1;

	/* step 2, 3 is define irqcounter_incre */
	handle_exception(NP_VECTOR, &handled_exception);

	/* step 4 prepare the IDT entry of #PF, with its P flag as 0*/
	set_idt_present(BP_VECTOR, 0);

	/* step 5 init by setup_idt (prepare the interrupt-gate descriptor of #NP)*/

	/* step 6  push rflag to stack, the rflag is set RF*/
	rflag = read_rflags();
	rflag_rf = rflag | RFLAG_RF_BIT;

	asm volatile("push %0\n"
		::"m"(rflag_rf));

	/* step 7  execute POPF to set EFLAGS.RF to 1*/
	asm volatile("popf");

	/*setp 8 execute INT3 instruction*/
	asm volatile("int3");

	/* step 9 confirm #NP exception has been generated, error code: idt and BP_VECTOR*/
	report("%s", ((irqcounter_query(NP_VECTOR) == 1) && ((save_error_code>>3) == BP_VECTOR)
		&& (((save_error_code>>1) & 0x1) == 0x1)), __FUNCTION__);

	/* resume environment */
	set_idt_present(BP_VECTOR, 1);
	execption_inc_len = 0;
}

/**
 * @brief case name: Page fault while handling benign or contributory exception_002
 *
 * Summary: At 64bit mode, the processor in #GP meeting a second #PF will be trigger a #PF.
 */
static void interrupt_rqmid_38069_gp_pf(void)
{
	unsigned long val;
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'mov %rax, %%cr4' instructions len
	 */
	execption_inc_len = 3;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(PF_VECTOR, &handled_exception);

	/* step 4 prepare NEWGDT*/
	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), PAGE_SIZE);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), PAGE_SIZE);

	/* step 5 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 6 prepare the interrupt-gate descriptor of #GP.
	 * the selector of the #GP is on the second page((0x201<<3) = x01008).
	 * IDT has been initialized. Only segment selector is set here
	 */
	set_idt_sel(GP_VECTOR, 0x1008);

	/* step 7 init by setup_idt (prepare the interrupt-gate descriptor of #PF)*/

	/* step 8 prepare the not present page.*/
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);

	/* setp 9 trigger #GP exception*/
	val = read_cr4();
	val |= (1<<23); //bit 23 is cr4 reserved bit
	asm volatile(
		     "mov %0, %%cr4\n\t"
		     : : "r" (val));

	/* step 10 confirm #PF exception has been generated*/
	report("%s %lx", ((irqcounter_query(PF_VECTOR) == 1)), __FUNCTION__, save_error_code);

	/* resume environment */
	set_idt_sel(GP_VECTOR, read_cs());
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 1, true);
	free(linear_addr);
	linear_addr = NULL;
	execption_inc_len = 0;
}

/**
 * @brief case name: Page fault while handling benign or contributory exception_001
 *
 * Summary: At 64bit mode, the processor in #NMI meeting a second #PF will be trigger a #PF.
 */
static void interrupt_rqmid_38070_nmi_pf(void)
{
	unsigned char *linear_addr;
	struct descriptor_table_ptr old_gdt_desc;
	struct descriptor_table_ptr new_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'wrmsr' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 and 3 is define irqcounter_incre */
	handle_exception(PF_VECTOR, &handled_exception);

	/* step 4 prepare NEWGDT*/
	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), PAGE_SIZE);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), PAGE_SIZE);

	/* step 5 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 6 prepare the interrupt-gate descriptor of #NMI.
	 * the selector of the #NMI is on the second page((0x201<<3) = x01008).
	 * IDT has been initialized. Only segment selector is set here
	 */
	set_idt_sel(NMI_VECTOR, 0x1008);

	/* step 7 init by setup_idt (prepare the interrupt-gate descriptor of #PF)*/

	/* step 8 prepare the not present page.*/
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 0, true);

	/* setp 9 trigger #NMI exception*/
	cli();
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, 0);

	/* step 10 confirm #PF exception has been generated*/
	report("%s %lx", ((irqcounter_query(PF_VECTOR) == 1)), __FUNCTION__, save_error_code);

	/* resume environment */
	set_idt_sel(NMI_VECTOR, read_cs());
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)), PAGE_PTE, 0, 1, true);
	free(linear_addr);
	linear_addr = NULL;
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose NMI handling_001
 *
 * Summary: At 64bit mode, clear EFLAGS.IF, configure local APIC ICR register
 * to send a non-maskable IPI to self, with its delivery mode as 100 (NMI),
 * shall generate #NMI.
 */
static void interrupt_rqmid_38110_nmi_001(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'wrmsr' instructions len
	 */
	execption_inc_len = 2;

	/* step 2 and 3 is define irqcounter_incre and prepare the interrupt-gate descriptor*/
	handle_exception(NMI_VECTOR, &handled_exception);

	/* setp 4 trigger #NMI exception*/
	cli();
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, APIC_ID_BSP);

	/* step 5 confirm #NMI exception has been generated*/
	report("%s %lx", ((irqcounter_query(NMI_VECTOR) == 1)), __FUNCTION__, save_error_code);

	/* resume environment */
	execption_inc_len = 0;
}

int volatile interrupt_38112_sync = 0;
u64 time_tsc[3];
void handled_nmi_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

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
	if (execption_inc_len >= 0) {
		regs->rip += execption_inc_len;
	}
	else {
		unhandled_exception(regs, false);
	}

	/*first nmi*/
	if (g_irqcounter[NMI_VECTOR] == 1) {
		time_tsc[0] = asm_read_tsc();
		interrupt_38112_sync = 1;
		test_delay(NMI_HANDLER_DELAY);
		time_tsc[1] = asm_read_tsc();
	}
	/*second nmi*/
	else if (g_irqcounter[NMI_VECTOR] == 3) {
		time_tsc[2] = asm_read_tsc();
	}
}

static void interrupt_rqmid_38112_nmi_ap_002(void)
{
	if (get_lapic_id() != 1) {
		return;
	}
	interrupt_38112_sync = 0;

	/*step 4 send nmi ipi to bp */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, APIC_ID_BSP);

	/*wait bp in nmi handler*/
	while (interrupt_38112_sync == 0) {
		asm volatile ("nop\n\t" :::"memory");
	}

	/*step 6 send nmi ipi to bp again*/
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, APIC_ID_BSP);
}

/**
 * @brief case name: Expose NMI handling_002
 *
 * Summary: At 64bit mode, AP1 sends the first non-maskable IPI to BP;
 * when BP in NMI_handler(in NMI_handler implement a delay() function),
 * AP1 send the second non-maskable IPI to BP. The second NMI will be
 * processed after the first NMI processing is completed,
 * rather than nested processing.
 */
static __unused void interrupt_rqmid_38112_nmi_002(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* step 2 prepare the interrupt handler of #NMI.*/
	/* step 3 prepare the interrupt-gate descriptor of #NMI*/
	handle_exception(NMI_VECTOR, &handled_nmi_exception);

	/* sync to ap start */
	current_case_id = 38112;

	/*step 4 wait ap ipi*/
	test_delay(NMI_BP_DELAY);

	/*step 7 check NMI exception, check start and end time of each NMI_handler.*/
	report("%s %lx", ((irqcounter_query(NMI_VECTOR) == 3) && (time_tsc[2] > time_tsc[1])),
		__FUNCTION__, save_error_code);
}

/**
 * @brief case name: Expose support for enabling and disabling interrupts_001
 *
 * Summary: At 64bit mode, clear EFLAGS.IF and trigger
 * the maskable interrupt, no interrupt is captured.
 */
static void interrupt_rqmid_38135_mask_interrupt_001(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* step 2 prepare the interrupt handler of INT 0xe0.*/
	/* step 3 prepare the interrupt-gate descriptor of INT 0xe0*/
	handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

	/* step 4 clear EFLAGS.IF*/
	cli();

	/* step 5 use TSC deadline timer delivery 224 interrupt*/
	if (cpuid(1).c & (1 << 24)) {
		apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
		wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x500);
	}

	/* step 6 wait dealline timer expire*/
	test_delay(1);

	/* step 7 check interrupt 0xe0*/
	report("%s %d", (irqcounter_query(EXTEND_INTERRUPT_E0) == 0),
		__FUNCTION__, irqcounter_query(EXTEND_INTERRUPT_E0));

	/* resume environment */
	sti();
	/* wait interrupte done */
	test_delay(1);
}

/**
 * @brief case name: Expose support for enabling and disabling interrupts_002
 *
 * Summary: At 64bit mode, set EFLAGS.IF and trigger
 * the maskable interrupt, interrupt is captured.
 */
static void interrupt_rqmid_38136_mask_interrupt_002(void)
{
	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* step 2 prepare the interrupt handler of INT 0xe0.*/
	/* step 3 prepare the interrupt-gate descriptor of INT 0xe0*/
	handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

	/* step 4 set EFLAGS.IF*/
	sti();

	/* step 5 use TSC deadline timer delivery 224 interrupt*/
	if (cpuid(1).c & (1 << 24)) {
		apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
		wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x500);
	}
	/* step 6 wait dealline timer expire*/
	test_delay(1);

	/* step 7 check interrupt 0xe0*/
	report("%s %d", (irqcounter_query(EXTEND_INTERRUPT_E0) == 1),
		__FUNCTION__, irqcounter_query(EXTEND_INTERRUPT_E0));
}

void ring3_sti_cli_exec(const char *msg)
{
	sti();
	cli();
}
/**
 * @brief case name: Expose support for enabling and disabling interrupts_003
 *
 * Summary: At 64bit mode, set RFLAGS.IOPL to 0, switch to ring3,
 * execute STI/CLI instruction shall generate #GP.
 */
static void interrupt_rqmid_38137_mask_interrupt_003(void)
{
	ulong rflags;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * 'sti'/'cli' instructions len
	 */
	execption_inc_len = 1;
	/* step 2 prepare the interrupt handler of #GP.*/
	handle_exception(GP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #GP)*/

	/* step 4 set EFLAGS.IOPL to 0*/
	rflags = read_rflags();
	rflags &= ~0x3000;
	write_rflags(rflags);

	/* step 5/6/7 switch to ring3, execute sti/cli*/
	do_at_ring3(ring3_sti_cli_exec, "");

	/* step 13 check #GP exception, error code*/
	report("%s %d", ((irqcounter_query(GP_VECTOR) == 3) && (save_error_code == 0)),
		__FUNCTION__, irqcounter_query(GP_VECTOR));

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose interrupt and exception handling data structure_001
 *
 * Summary: At 64bit mode, access a descriptor beyond the limit of IDT,
 * shall generate #GP(0).
 */
static void interrupt_rqmid_38157_data_structure_001(void)
{
	struct descriptor_table_ptr old_idt;
	struct descriptor_table_ptr new_idt;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	execption_inc_len = 2;
	/* step 2 prepare the interrupt handler of #GP.*/
	handle_exception(GP_VECTOR, &handled_exception);
	/* step 3 is define handled_interrupt_external_f0*/

	/* step 4 init by setup_idt(prepare the interrupt-gate descriptor of #GP)*/

	/* setp 5 set_idt_entry for irq */
	handle_irq(IDT_BEYOND_LIMIT_INDEX, handled_interrupt_beyond_limit_f0);

	/* step 6 */
	sidt(&old_idt);

	/* step 7 */
	new_idt.limit = ((IDT_BEYOND_LIMIT_INDEX-1)*8) - 1;
	new_idt.base = old_idt.base;

	/* step 8 */
	lidt(&new_idt);

	/* step 9 */
	asm volatile("INT $"xstr(IDT_BEYOND_LIMIT_INDEX)"\n\t");

	/* step 10 error code is IDT vector number*/
	report("%s %d %lx", ((irqcounter_query(GP_VECTOR) == 1)
		&& (save_error_code == (IDT_BEYOND_LIMIT_INDEX*8)+ERROR_CODE_IDT_FLAG)),
		__FUNCTION__, irqcounter_query(GP_VECTOR), save_error_code);

	/* resume environment */
	execption_inc_len = 0;
	lidt(&old_idt);
}

/**
 * @brief case name: Expose interrupt and exception handling data structure_002
 *
 * Summary: At 64bit mode, set EFLAGS.IF bit, trigger a external interrupt,
 * if this external interrupt entry is an interrupt-gate descriptor,
 * in the interrupt handling function, the RFLAGS.IF shall clear automatically.
 */
static void interrupt_rqmid_38158_data_structure_002(void)
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
 * @brief case name: Expose interrupt and exception handling data structure_003
 *
 * Summary: At 64bit mode, set EFLAGS.IF bit, trigger a external interrupt,
 * if this external interrupt entry is an trap-gate descriptor,
 * in the interrupt handling function, the RFLAGS.IF shall keep set.
 */
static void interrupt_rqmid_38159_data_structure_003(void)
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

/**
 * @brief case name: Expose exception and interrupt handling_DE_001
 *
 * Summary:At 64bit mode, execute divide by zero will trigger #DE exception
 * and call the #DE handler, program state should be unchanged,
 * the saved contents of EIP registers point to the instruction
 * that generated the exception.
 */
static void interrupt_rqmid_38173_expose_exception_de_001(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #DE. */
	handle_exception(DE_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #DE)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/* step 6 generate #DE, use edx*/
	asm volatile ("mov $0x0,%edx\n\t"
		"div    %edx");
	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 8 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 9 generate #DE, , use ebx*/
	asm volatile ("mov $0x0,%ebx\n\t"
		"div    %ebx");
	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/* step 11 generate #DE, use any register*/
	asm volatile ("mov $0x0,%ebx\n\t"
		"div    %ebx");

	/*step 12 get step 11 'div %eax' instruction address*/
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 2 is 'div %eax' instruction len*/
	current_rip -= (5 + 2);

	debug_print("%lx %lx %lx %lx %lx\n", save_rflags1, save_rflags2,
		save_error_code, current_rip, rip[0]);

	/* step 13 check #DE exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(DE_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_de_002
 *
 * Summary: At 64bit mode, execute divide by zero will trigger #DE exception
 * and call the #DE handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38174_expose_exception_de_002(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #DE. */
	handle_exception(DE_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #DE)*/

	/* step 4 execute divide by zero*/
	asm volatile ("mov $0x0,%ebx\n\t"
		"div    %ebx");

	check = check_rflags();

	/* step 5 check #DE exception and rflag*/
	report("%s", ((irqcounter_query(DE_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_NMI_001
 *
 * Summary:At 64bit mode, clear EFLAGS.IF, configure local APIC ICR register to send a
 * non-maskable IPI to self, with its delivery mode as 100 will trigger #NMI exception
 * and call the #NMI handler, program state should be unchanged, the saved contents of
 * EIP registers point to the instruction that generated the exception.
 */
static void interrupt_rqmid_38175_expose_exception_nmi_001(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #NMI. */
	handle_exception(NMI_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #NMI)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 init eax register()*/
	asm volatile("mov %0, %%eax"::"r"(APIC_DEST_PHYSICAL | APIC_DM_NMI));
	/* step 6  clear rflags.if*/
	cli();
	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/* step 8 generate #NMI*/
	asm volatile ("wrmsr"
		::"a"(APIC_DEST_PHYSICAL | APIC_DM_NMI),
		"d"(APIC_ID_BSP),
		"c"(APIC_BASE_MSR + APIC_ICR/16));
	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 11 generate #NMI*/
	asm volatile ("wrmsr"
		::"a"(APIC_DEST_PHYSICAL | APIC_DM_NMI),
		"d"(APIC_ID_BSP),
		"c"(APIC_BASE_MSR + APIC_ICR/16));
	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*step 13 clear RFLAGS.IF bit, generate #NMI */
	asm volatile ("wrmsr"
		::"a"(APIC_DEST_PHYSICAL | APIC_DM_NMI),
		"d"(APIC_ID_BSP),
		"c"(APIC_BASE_MSR + APIC_ICR/16));

	/*step 14 get step 13 'wrmsr' instruction address
	 * If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	/* 5 is call get_current_rip instruction len, 2 is 'wrmsr' instruction len */
	current_rip -= (5 + 2);

	debug_print("%x %x %lx %lx\n", irqcounter_query(NMI_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #NMI exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(NMI_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_NMI_002
 *
 * Summary: At 64bit mode, clear EFLAGS.IF, configure local APIC ICR register to
 * send a non-maskable IPI to self, with its delivery mode as 100 will trigger #NMI
 * exception and call the #NMI handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38176_expose_exception_nmi_002(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #NMI. */
	handle_exception(NMI_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #NMI)*/

	/* step 4 clear RFLAGS.IF bit, generate #NMI */
	cli();
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, APIC_ID_BSP);

	check = check_rflags();

	/* step 5 check #NMI exception and rflag*/
	report("%s", ((irqcounter_query(NMI_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_BP_001
 *
 * Summary:At 64bit mode, set EFLAGS.RF to 1, execute INT3 instruction will trigger
 * #BP exception and call the #BP handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38178_expose_exception_bp_001(void)
{
	ulong rflag;
	ulong rflag_rf;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * #BP is a trap execption, so, not need add instruction len.
	 */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #BP. */
	handle_exception(BP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #BP)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5  push rflag to stack, the rflag is set RF*/
	rflag = read_rflags();
	rflag_rf = rflag | RFLAG_RF_BIT;
	asm volatile("push %0\n"
		::"m"(rflag_rf));

	/* step 6  execute POPF to set EFLAGS.RF to 1*/
	asm volatile("popf");

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 execute INT3 instruction trigger #BP*/
	asm volatile("int3");

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 execute INT3 instruction trigger #BP*/
	asm volatile("int3");

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 execute INT3 instruction trigger #BP*/
	asm volatile("int3");

	/*step 14 get setp 13 'int3' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len*/
	current_rip -= (5 + 0);

	debug_print("%x %x %lx %lx\n", irqcounter_query(BP_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #BP exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(BP_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_BP_002
 *
 * Summary:At 64bit mode, set EFLAGS.RF to 1, execute INT3 instruction will
 * trigger #BP exception and call the #BP handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38179_expose_exception_bp_002(void)
{
	ulong rflag;
	ulong rflag_rf;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #BP. */
	handle_exception(BP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #BP)*/

	/* step 4  push rflag to stack, the rflag is set RF*/
	rflag = read_rflags();
	rflag_rf = rflag | RFLAG_RF_BIT;
	asm volatile("push %0\n"
		::"m"(rflag_rf));

	/* step 5  execute POPF to set EFLAGS.RF to 1*/
	asm volatile("popf");

	/*setp 6 execute INT3 instruction trigger #BP*/
	asm volatile("int3");

	check = check_rflags();

	/* step 7 check #BP exception and rflag*/
	report("%s", ((irqcounter_query(BP_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_UD_001
 *
 * Summary:At 64bit mode, execute UD2 instruction will trigger #UD
 * exception and call the #UD handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38263_expose_exception_ud_001(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #UD. */
	handle_exception(UD_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #UD)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 6 execute UD2 instruction trigger #UD*/
	asm volatile("UD2");

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 8 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 9 execute UD2 instruction trigger #UD*/
	asm volatile("UD2");

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 11 execute UD2 instruction trigger #UD*/
	asm volatile("UD2");

	/*step 12 get setp 11 'UD2' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 2 is UD2 instruction len*/
	current_rip -= (5 + 2);

	debug_print("%x %x %lx %lx\n", irqcounter_query(UD_VECTOR), check,
		current_rip, rip[2]);

	/* step 13 check #UD exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(UD_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_UD_002
 *
 * Summary:At 64bit mode, execute UD2 instruction will trigger #UD
 * exception and call the #UD handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38264_expose_exception_ud_002(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #UD. */
	handle_exception(UD_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #UD)*/

	/*setp 4 execute UD2 instruction trigger #UD*/
	asm volatile("UD2");

	check = check_rflags();

	/* step 5 check #UD exception and rflag*/
	report("%s", ((irqcounter_query(UD_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_NM_001
 *
 * Summary:At 64bit mode, set CR0.EM and CR0.NE to 1
 * (enable native exception and enable software emulate),
 * then execute floating point arithmetic will trigger #NM
 * exception and call the #NM handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38286_expose_exception_nm_001(void)
{
	float op1 = 1.2;
	ulong cr0 = read_cr0();
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #NM. */
	handle_exception(NM_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #NM)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/*step 5 set cr0.em to 1*/
	cr0_em_to_1();
	/*step 6 set cr0.ne to 1*/
	cr0_ne_to_1();

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*step 8 Load Floating Point Value to trigger #NM*/
	asm volatile("fld %0\n\t"::"m"(op1));

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*step 11 Load Floating Point Value to trigger #NM*/
	asm volatile("fld %0\n\t"::"m"(op1));

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*step 13 Load Floating Point Value to trigger #NM*/
	asm volatile("fld %0\n\t"::"m"(op1));

	/*step 14 get setp 13 'fld %0' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 3 is 'fld %0' instruction len*/
	current_rip -= (5 + 3);

	debug_print("%x %x %lx %lx\n", irqcounter_query(NM_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #NM exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(NM_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	write_cr0(cr0);
}

/**
 * @brief case name:Expose exception and interrupt handling_NM_002
 *
 * Summary:At 64bit mode, set CR0.EM and CR0.NE to 1
 * (enable native exception and enable software emulate),
 * then execute floating point arithmetic will trigger #NM
 * exception and call the #NM handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38287_expose_exception_nm_002(void)
{
	float op1 = 1.2;
	int check = 0;
	ulong cr0 = read_cr0();

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #NM. */
	handle_exception(NM_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #NM)*/

	/*step 4 set cr0.em to 1*/
	cr0_em_to_1();
	/*step 5 set cr0.ne to 1*/
	cr0_ne_to_1();
	/*setp 6  trigger #NM*/
	asm volatile("fld %0\n\t"::"m"(op1));

	check = check_rflags();

	/* step 7 check #NM exception and rflag*/
	report("%s", ((irqcounter_query(NM_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	write_cr0(cr0);
}

/**
 * @brief case name: Expose exception and interrupt handling_DF_001
 *
 * Summary:At 64bit mode, execute INT 8 instruction will trigger #DF
 * exception and call the #DF handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38289_expose_exception_df_001(void)
{
	int check = 0;
	void *addr = get_idt_addr(DF_VECTOR);

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #DF. */
	handle_exception(DF_VECTOR, &handled_exception);
	set_idt_addr(DF_VECTOR, &df_fault_soft);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #DF)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 6 execute INT 8 instruction trigger #DF*/
	asm volatile("INT $8");

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 8 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 9 execute INT 8 instruction trigger #DF*/
	asm volatile("INT $8");

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 11 execute INT 8 instruction trigger #DF*/
	asm volatile("INT $8");

	/*step 12 get setp 11 'INT 8' instruction exception address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len*/
	current_rip -= (5 + 0);

	debug_print("%x %x %lx %lx\n", irqcounter_query(DF_VECTOR), check,
		current_rip, rip[2]);

	/* step 13 check #DF exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(DF_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_addr(DF_VECTOR, addr);
}

/**
 * @brief case name:Expose exception and interrupt handling_DF_002
 *
 * Summary:At 64bit mode, execute INT 8 instruction will trigger #DF
 * exception and call the #DF handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38290_expose_exception_df_002(void)
{
	int check = 0;
	void *addr = get_idt_addr(DF_VECTOR);

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #DF. */
	handle_exception(DF_VECTOR, &handled_exception);
	set_idt_addr(DF_VECTOR, &df_fault_soft);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #DF)*/

	/*setp 4 execute INT 8 instruction trigger #DF*/
	asm volatile("INT $8");

	check = check_rflags();

	/* step 5 check #DF exception and rflag*/
	report("%s", ((irqcounter_query(DF_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_addr(DF_VECTOR, addr);
}

/**
 * @brief case name: Expose exception and interrupt handling_NP_001
 *
 * Summary:At 64bit mode, construct IDT entry of #UD with P flag as 0,
 * execute UD2 instruction will trigger #NP exception and
 * call the #NP handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38300_expose_exception_np_001(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #UD. */
	handle_exception(UD_VECTOR, &handled_exception);
	/* step 3 prepare the interrupt handler of #NP. */
	handle_exception(NP_VECTOR, &handled_exception);

	/* step 4 init by setup_idt(prepare the interrupt-gate descriptor of #UD)*/
	set_idt_present(UD_VECTOR, 0);
	/* step 5 init by setup_idt(prepare the interrupt-gate descriptor of #NP)*/

	/* step 6 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 execute UD2 instruction trigger #NP*/
	asm volatile("UD2");

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 execute UD2 instruction trigger #NP*/
	asm volatile("UD2");

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 execute UD2 instruction trigger #NP*/
	asm volatile("UD2");

	/*step 14 get setp 13 'UD2' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 2 is UD2 instruction len*/
	current_rip -= (5 + 2);

	debug_print("%x %x %lx %lx\n", irqcounter_query(NP_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #NP exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(NP_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_present(UD_VECTOR, 0);
}

/**
 * @brief case name:Expose exception and interrupt handling_NP_002
 *
 * Summary:At 64bit mode, execute UD2 instruction will trigger #UD
 * exception and call the #NP handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38301_expose_exception_np_002(void)
{
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #UD. */
	handle_exception(UD_VECTOR, &handled_exception);
	/* step 3 prepare the interrupt handler of #NP. */
	handle_exception(NP_VECTOR, &handled_exception);

	/* step 4 init by setup_idt(prepare the interrupt-gate descriptor of #UD)*/
	set_idt_present(UD_VECTOR, 0);
	/* step 5 init by setup_idt(prepare the interrupt-gate descriptor of #NP)*/

	/*setp 6 execute UD2 instruction trigger #NP*/
	asm volatile("UD2");

	check = check_rflags();

	/* step 7 check #NP exception and rflag*/
	report("%s", ((irqcounter_query(NP_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	set_idt_present(UD_VECTOR, 1);
}

/**
 * @brief case name: Expose exception and interrupt handling_SS_001
 *
 * Summary:At 64bit mode, the segment is marked not present,
 * execute LSS will trigger #SS exception and call the #SS handler,
 * program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38352_expose_exception_ss_001(void)
{
	struct lseg_st lss;
	int check = 0;
	struct descriptor_table_ptr old_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 4;

	/* step 2 prepare the interrupt handler of #SS. */
	handle_exception(SS_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #SS)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 Construct a not present stack segment descriptor in GDT*/
	sgdt(&old_gdt_desc);
	/*not present*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/*step 6 Prepare the LSS source operand*/
	lss.offset = 0xffffffff;
	/* index =0x50 RPL=0*/
	lss.selector = SELECTOR_INDEX_280H;

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 trigger #SS*/
	asm volatile("lss  %0, %%eax\t\n"
		::"m"(lss));

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 trigger #SS*/
	asm volatile("lss  %0, %%eax\t\n"
		::"m"(lss));

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 trigger #SS*/
	asm volatile("lss  %0, %%eax\t\n"
		::"m"(lss));

	/*step 14 get setp 13 'lss -0x14(%rbp),%eax' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 4 is 'lss -0x14(%rbp),%eax' instruction len*/
	current_rip -= (5 + 4);

	debug_print("%x %x %lx %lx\n", irqcounter_query(SS_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #SS exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(SS_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_SS_002
 *
 * Summary:At 64bit mode, the segment is marked not present,
 * execute LSS will trigger #SS exception and call the #SS handler,
 * EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT is cleared
 * after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38353_expose_exception_ss_002(void)
{
	int check = 0;
	struct lseg_st lss;
	struct descriptor_table_ptr old_gdt_desc;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 4;

	/* step 2 prepare the interrupt handler of #SS. */
	handle_exception(SS_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #SS)*/

	/* step 4 Construct a not present stack segment descriptor in GDT*/
	sgdt(&old_gdt_desc);
	/*not present*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/*step 5 Prepare the LSS source operand*/
	lss.offset = 0xffffffff;
	/* index =0x50 RPL=0*/
	lss.selector = SELECTOR_INDEX_280H;

	/*setp 6 trigger #SS*/
	asm volatile("lss  %0, %%eax\t\n"
		::"m"(lss));

	check = check_rflags();

	/* step 7 check #SS exception and rflag*/
	report("%s", ((irqcounter_query(SS_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

// CR2 shoud be set #PF vaddr
/**
 * @brief case name: Expose exception and interrupt handling_PF_001
 *
 * Summary:At 64bit mode, paging is enabled (CR0.PG is set),
 * clear the P bit of PTE which is needed for the address translation,
 * access an address which is mapped to the PTE will trigger #PF
 * exception and call the #PF handler, program state should be unchanged,
 * the saved contents of EIP registers point to the
 * instruction that generated the exception.
 */
static void interrupt_rqmid_38393_expose_exception_pf_001(void)
{
	int check = 0;
	u8 *gva;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception 'mov %rax,(%rbx)'*/
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #PF. */
	handle_exception(PF_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #PF)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 define one GVA1 points to one 1B memory block in the 4K page. */
	gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	/* step 6 set the 4k page PTE entry is not present*/
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, true);

	/* mov gva to rbx*/
	asm volatile("mov %0, %%rbx\n\t"
		"1:" : : "m"(gva));

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 trigger #PF*/
	asm volatile("mov %rax, (%rbx)\n\t");

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 trigger #PF*/
	asm volatile("mov %rax, (%rbx)\n\t");

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 trigger #PF*/
	asm volatile("mov %rax, (%rbx)\n\t");

	/*step 14 get setp 13 'mov %rax,(%rbx)' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 3 is the 'mov %rax,(%rbx)' instruction len */
	current_rip -= (5 + 3);

	debug_print("%x %x %lx %lx\n", irqcounter_query(PF_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #PF exception, program state, not check CR2 */
	regs_check[1].CR2 = regs_check[0].CR2;
	check = check_all_regs();
	report("%s", ((irqcounter_query(PF_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_PF_002
 *
 * Summary:At 64bit mode, paging is enabled (CR0.PG is set),
 * clear the P bit of PTE which is needed for the address translation,
 * access an address which is mapped to the PTE will trigger #PF
 * exception and call the #PF handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38394_expose_exception_pf_002(void)
{
	int check = 0;
	u8 value = 1;
	u8 *gva;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception 'mov %al,(%r12)'*/
	//execption_inc_len = 4;
	/* length of the instruction that generated the exception 'mov %dl,(%rax)'*/
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #PF. */
	handle_exception(PF_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #PF)*/

	/* step 4 define one GVA1 points to one 1B memory block in the 4K page. */
	gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	/* step 5 set the 4k page PTE entry is not present*/
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, true);

	/*setp 6 trigger #PF*/
	asm volatile("mov %[value], (%[p])\n\t"
		"1:" : : [value]"d"(value), [p]"a"(gva));

	check = check_rflags();

	/* step 7 check #PF exception and rflag*/
	report("%s", ((irqcounter_query(PF_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name: Expose exception and interrupt handling_XM_001
 *
 * Summary:At 64bit mode, set CR4.OSXMMEXCPT and CR4.OSFXSR to be 1,
 * clear all mask bit in MXCSR register, Execute ADDPS instruction,
 * will trigger #XM exception and call the #XM handler,
 * program state should be unchanged, the saved contents of EIP registers
 * point to the instruction that generated the exception.
 */
static void interrupt_rqmid_38455_expose_exception_xm_001(void)
{
	int check = 0;
	u32 mxcrs = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception 'divpd  %xmm2,%xmm1'*/
	execption_inc_len = 4;

	/* step 2 prepare the interrupt handler of #XM */
	handle_exception(XM_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #XM)*/

	/* step 4 init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* step 5 set cr4.OSFXSR[bit 9] and set cr4.OSXMMEXCPT[bit 10]*/
	cr4_osfxsr_to_1();
	cr4_osxmmexcpt_to_1();

	/* step 6 clear all mask bit in MXCSR register*/
	asm volatile("LDMXCSR %0"
		::"m"(mxcrs));

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);

	/*setp 8 trigger #XM*/
	asm volatile("DIVPD %xmm2, %xmm1");

	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*setp 11 trigger #XM*/
	asm volatile("DIVPD %xmm2, %xmm1");

	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/*setp 13 trigger #XM*/
	asm volatile("DIVPD %xmm2, %xmm1");

	/*step 14 get setp 13 'DIVPD %xmm2, %xmm1' instruction address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len,  4 is 'DIVPD %xmm2, %xmm1' instruction len */
	current_rip -= (5 + 4);

	debug_print("%x %x %lx %lx\n", irqcounter_query(XM_VECTOR), check,
		current_rip, rip[2]);

	/* step 15 check #XM exception, program state*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(XM_VECTOR) == 6) && (check == 0)
		&& (current_rip == rip[2])), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose exception and interrupt handling_XM_002
 *
 * Summary:At 64bit mode, set CR4.OSXMMEXCPT and CR4.OSFXSR to be 1,
 * clear all mask bit in MXCSR register, Execute ADDPS instruction,
 * will trigger #XM exception and call the #XM handler, EFLAGS.TF, EFLAGS.VM,
 * EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_38456_expose_exception_xm_002(void)
{
	int check = 0;
	u32 mxcrs = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception 'divpd  %xmm2,%xmm1'*/
	execption_inc_len = 4;

	/* step 2 prepare the interrupt handler of #XM. */
	handle_exception(XM_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #XM)*/

	/* step 4 set cr4.OSFXSR[bit 9] and set cr4.OSXMMEXCPT[bit 10]*/
	cr4_osfxsr_to_1();
	cr4_osxmmexcpt_to_1();

	/* step 5 clear all mask bit in MXCSR register*/
	asm volatile("LDMXCSR %0"
		::"m"(mxcrs));

	/*setp 6 trigger #XM*/
	asm volatile("DIVPD %xmm2, %xmm1");

	check = check_rflags();

	/* step 7 check #XM exception and rflag*/
	report("%s", ((irqcounter_query(XM_VECTOR) == 1) && (check == 0)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_002
 *
 * Summary: At 64bit mode, trigger a local APIC timer interrupt during executing
 * OUTS iteration (except the last iteration) of a repeated string instruction,
 * check that the value of EFLAGS.RF pushed on the stack is 1.
 */
static void interrupt_rqmid_38458_expose_instruction_breakpoints_002(void)
{
	int check = 0;
	__unused u64 t[2];
	char *in;
	char *out, *out_ori;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of INT 0x80.*/
	/* step 3 prepare the interrupt-gate descriptor of INT 0x80*/
	handle_irq(EXTEND_INTERRUPT_80, handled_interrupt_external_80);
	//handle_exception(GP_VECTOR, &handled_exception);
	sti();

	in = malloc(INTERRUPT_MOVSB_MAX_SIZE);
	out_ori = out = malloc(INTERRUPT_MOVSB_MAX_SIZE);

	if ((in == NULL) || (out == NULL)) {
		printf("%s malloc error!\n", __FUNCTION__);
		if (in) {
			free(in);
		}
		else if (out) {
			free(out);
		}
		return;
	}

	/* step 4 use TSC deadline timer delivery 0x80 interrupt*/
	if (cpuid(1).c & (1 << 24))
	{
		apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_80);
		wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x1000));
	}

	/* step 5 execute movsb iteration, and out pointer will ++ for each time */
	t[0] = asm_read_tsc();
	asm volatile ("rep/movsb" : "+D"(out) : "S"(in), "c"(INTERRUPT_MOVSB_MAX_SIZE));	//0x125bd8 TSC
	t[1] = asm_read_tsc();

	test_delay(1);

	debug_print("t0=%lx t1=%lx %lx\n", t[0], t[1], t[1]-t[0]);
	debug_print("%lx %lx %d\n", save_rflags1, save_rflags2,
		irqcounter_query(EXTEND_INTERRUPT_80));

	/* step 6 confirm 0x80 interrupt and EFLAGS.RF */
	if ((save_rflags1 & RFLAG_RF_BIT)) {
		check = 1;
	}

	debug_print("%d %d\n", irqcounter_query(EXTEND_INTERRUPT_80), check);
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_80) == 1) && (check == 1)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
	free(in);
	/* out pointer has changed, then need to free original out pointer */
	free(out_ori);
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_004
 *
 * Summary: At 64bit mode, trigger a trap-class exception after an instruction
 * completes, check that the value of EFLAGS.RF pushed on the stack is the
 * value of EFLAGS.RF at the time the exception handler was called.
 */
static void interrupt_rqmid_38460_expose_instruction_breakpoints_004(void)
{
	ulong rflag;
	ulong rflag_rf;
	int check = 0;
	ulong pre_rflags = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception
	 * #BP is a trap execption, so, not need add instruction len.
	 */
	execption_inc_len = 0;

	/* step 2 prepare the interrupt handler of #BP. */
	handle_exception(BP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #BP)*/

	/* step 4  push rflag to stack, the rflag is set RF*/
	rflag = read_rflags();
	rflag_rf = rflag | RFLAG_RF_BIT;
	asm volatile("push %0\n"
		::"m"(rflag_rf));

	/* step 5  execute POPF to set EFLAGS.RF to 1*/
	asm volatile("popf");

	/* step 6 save rflag value*/
	pre_rflags = read_rflags();

	/*setp 7 execute INT3 instruction trigger #BP*/
	asm volatile("int3");

	debug_print("%lx %lx %d\n", save_rflags1, pre_rflags,
		irqcounter_query(BP_VECTOR));

	/* step 8 confirm #BP exception and EFLAGS.RF is not change*/
	if ((save_rflags1 == pre_rflags)) {
		check = 1;
	}
	debug_print("%d %d\n", irqcounter_query(BP_VECTOR), check);
	report("%s", ((irqcounter_query(BP_VECTOR) == 1) && (check == 1)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

/**
 * @brief case name:Set EFLAGS.RF_001
 *
 * Summary: At 64bit mode, when a vCPU attempts to call a handler for a NMI,
 * ACRN hypervisor shall guarantee that the EFLAGS.RF on the current guest stack is is unchanged.
 */
static void interrupt_rqmid_39087_set_eflags_rf_001(void)
{
	bool check = false;
	ulong save_rflags0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/* length of the instruction that generated the exception */
	execption_inc_len = 2;

	/* step 2 prepare the interrupt handler of #NMI. */
	handle_exception(NMI_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #NMI)*/

	/* step 4 generate #NMI */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, APIC_ID_BSP);

	save_rflags0 = read_rflags();

	if ((save_rflags1 & RFLAG_RF_BIT) == (save_rflags0 & RFLAG_RF_BIT)) {
		check = true;
	}

	/* step 5 check #NMI exception and rflag*/
	report("%s", ((irqcounter_query(NMI_VECTOR) == 1) && (check == true)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

//test fail test result: first #PF, second interrupt
/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P4_P6_001
 *
 * Summary: At 64bit mode, arm a TSC deadline timer with 0x1000 periods increament
 * (when the deadline timer expires, an 0xE0 interrupt is generated).
 * Immediately access memory address above 512M to trigger VM exit(EPT violation).
 * This will generate an interrupt and #PF simultaneously after VM-exit finished.
 * The 0xE0 interrupt should be serviced first, and the #PF should be serviced later.
 */
static void interrupt_rqmid_39157_p4_p6_001(void)
{
	u8 *gva = (u8 *)0x40000000;	//1G address space
	int i = 1;
	__unused u64 t[2];

	while (i--) {
		/* step 1 init g_irqcounter */
		irqcounter_initialize();

		/* length of the instruction that generated the exception 'mov %edx,(%rax)'*/
		execption_inc_len = 2;

		/* step 2 prepare the interrupt handler of #PF. */
		handle_exception(PF_VECTOR, &handled_exception);

		/* step 3 init by setup_idt (prepare the interrupt-gate descriptor of #PF)*/

		/* step 4 is define handled_interrupt_external_e0*/
		/* setp 5 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external_e0);

		/* step 6 enable interrupt*/
		irq_enable();

		/* step 7 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24)) {
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x1000);
		}

		/* step 8 trigger VM-exit and generate #PF*/
		t[0] = asm_read_tsc();
		asm volatile("mov %[value], (%[p])\n\t"
			"1:" : : [value]"d"(0), [p]"a"(gva));	//0x2c00 TSC
		t[1] = asm_read_tsc();

		/* step 9 enable interrupt*/
		irq_disable();
		debug_print("%s %d i=%d interrupt=%d PF=%d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(PF_VECTOR), t[1] - t[0]);

		test_delay(1);
	}

	/* step 10 first interrupt, second #PF */
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_E0) == 1)
		&& (irqcounter_query(PF_VECTOR) == 2)), __FUNCTION__);

	/* resume environment */
	execption_inc_len = 0;
}

static void print_case_list_64(void)
{
	printf("\t Interrupt feature 64-Bits Mode case list:\n\r");

	printf("Case ID:%d case name:%s\n\r", 24028, "IDTR_following_start-up_001");
#ifdef IN_NON_SAFETY_VM
	printf("Case ID:%d case name:%s\n\r", 23981, "IDTR_following_INIT_001");
#endif
	printf("Case ID:%d case name:%s\n\r", 24211,
		"Page fault while handling a prior page fault_001");
	printf("Case ID:%d case name:%s\n\r", 36241,
		"Interrupt to non-present descriptor_001");
	printf("Case ID:%d case name:%s\n\r", 36254,
		"Expose exception and interrupt handling_GP_001");

	printf("Case ID:%d case name:%s\n\r", 36255,
		"Expose exception and interrupt handling_GP_002");
	printf("Case ID:%d case name:%s\n\r", 36701,
		"Expose exception and interrupt handling_MF_001");
	printf("Case ID:%d case name:%s\n\r", 36702,
		"Expose exception and interrupt handling_mf_002");
	printf("Case ID:%d case name:%s\n\r", 36689,
		"Expose priority among simultaneous exceptions and interrupts_P4_P7_001");
	printf("Case ID:%d case name:%s\n\r", 36691,
		"Expose priority among simultaneous exceptions and interrupts_P4_P8_001");

#ifdef IN_NON_SAFETY_VM
	printf("Case ID:%d case name:%s\n\r", 36693,
		"Expose priority among simultaneous exceptions and interrupts_P3_P4_001");
#endif
	printf("Case ID:%d case name:%s\n\r", 36694,
		"Expose masking and unmasking Instruction breakpoints_001");
	printf("Case ID:%d case name:%s\n\r", 36695,
		"Expose masking and unmasking Instruction breakpoints_005");
	printf("Case ID:%d case name:%s\n\r", 38047,
		"Contributory exception while handling a prior benign exception_001");
	printf("Case ID:%d case name:%s\n\r", 38069,
		"Page fault while handling benign or contributory exception_002");

	printf("Case ID:%d case name:%s\n\r", 38070,
		"Page fault while handling benign or contributory exception_001");
	printf("Case ID:%d case name:%s\n\r", 38110, "Expose NMI handling_001");
	printf("Case ID:%d case name:%s\n\r", 38112, "Expose NMI handling_002");
	printf("Case ID:%d case name:%s\n\r", 38135,
		"Expose support for enabling and disabling interrupts_001");
	printf("Case ID:%d case name:%s\n\r", 38136,
		"Expose support for enabling and disabling interrupts_002");

	printf("Case ID:%d case name:%s\n\r", 38137,
		"Expose support for enabling and disabling interrupts_003");
	printf("Case ID:%d case name:%s\n\r", 38157,
		"Expose interrupt and exception handling data structure_001");
	printf("Case ID:%d case name:%s\n\r", 38158,
		"Expose interrupt and exception handling data structure_002");
	printf("Case ID:%d case name:%s\n\r", 38159,
		"Expose interrupt and exception handling data structure_003");
	printf("Case ID:%d case name:%s\n\r", 38173,
		"Expose exception and interrupt handling_DE_001");

	printf("Case ID:%d case name:%s\n\r", 38174,
		"Expose exception and interrupt handling_DE_002");
	printf("Case ID:%d case name:%s\n\r", 38175,
		"Expose exception and interrupt handling_NMI_001");
	printf("Case ID:%d case name:%s\n\r", 38176,
		"Expose exception and interrupt handling_NMI_002");
	printf("Case ID:%d case name:%s\n\r", 38178,
		"Expose exception and interrupt handling_BP_001");
	printf("Case ID:%d case name:%s\n\r", 38179,
		"Expose exception and interrupt handling_BP_002");

	printf("Case ID:%d case name:%s\n\r", 38263,
		"Expose exception and interrupt handling_UD_001");
	printf("Case ID:%d case name:%s\n\r", 38264,
		"Expose exception and interrupt handling_UD_002");
	printf("Case ID:%d case name:%s\n\r", 38286,
		"Expose exception and interrupt handling_NM_001");
	printf("Case ID:%d case name:%s\n\r", 38287,
		"Expose exception and interrupt handling_NM_002");
	printf("Case ID:%d case name:%s\n\r", 38289,
		"Expose exception and interrupt handling_DF_001");

	printf("Case ID:%d case name:%s\n\r", 38290,
		"Expose exception and interrupt handling_DF_002");
	printf("Case ID:%d case name:%s\n\r", 38300,
		"Expose exception and interrupt handling_NP_001");
	printf("Case ID:%d case name:%s\n\r", 38301,
		"Expose exception and interrupt handling_NP_002");
	printf("Case ID:%d case name:%s\n\r", 38352,
		"Expose exception and interrupt handling_SS_001");
	printf("Case ID:%d case name:%s\n\r", 38353,
		"Expose exception and interrupt handling_SS_002");

	printf("Case ID:%d case name:%s\n\r", 38393,
		"Expose exception and interrupt handling_PF_001");
	printf("Case ID:%d case name:%s\n\r", 38394,
		"Expose exception and interrupt handling_PF_002");
	printf("Case ID:%d case name:%s\n\r", 38455,
		"Expose exception and interrupt handling_XM_001");
	printf("Case ID:%d case name:%s\n\r", 38456,
		"Expose exception and interrupt handling_XM_002");
	printf("Case ID:%d case name:%s\n\r", 38458,
		"Expose masking and unmasking Instruction breakpoints_002");

	printf("Case ID:%d case name:%s\n\r", 38460,
		"Expose masking and unmasking Instruction breakpoints_004");
	printf("Case ID:%d case name:%s\n\r", 39087, "Set EFLAGS.RF_001");
	printf("Case ID:%d case name:%s\n\r", 39157,
		"Expose priority among simultaneous exceptions and interrupts_P4_P6_001");
#ifdef IN_NON_SAFETY_VM
		printf("Case ID:%d case name:%s\n\r", 38001,
			"Page fault while handling #DF in non-safety VM_001");
#endif
#ifdef IN_SAFETY_VM
		printf("Case ID:%d case name:%s\n\r", 38002,
			"Page fault while handling #DF in safety VM_001");
#endif
}

static void test_ap_interrupt_64(void)
{
	while (1) {
		switch (current_case_id) {
		case 36693:
			interrupt_rqmid_36693_p3_p4_ap_001();
			current_case_id = 0;
			break;

		case 38112:
			interrupt_rqmid_38112_nmi_ap_002();
			current_case_id = 0;
			break;

		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}

static void test_interrupt_64(void)
{
	/* __x86_64__*/
	interrupt_rqmid_24028_idtr_startup();
#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_23981_idtr_init();
#endif
	interrupt_rqmid_24211_pf_pf();
	interrupt_rqmid_36241_no_present_descriptor_001();
	interrupt_rqmid_36254_expose_exception_gp_001();

	interrupt_rqmid_36255_expose_exception_gp_002();
	interrupt_rqmid_36701_expose_exception_mf_001();
	interrupt_rqmid_36702_expose_exception_mf_002();
	interrupt_rqmid_36689_p4_p7_001();
	interrupt_rqmid_36691_p4_p8_001();

#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_36693_p3_p4_001();
#endif
	interrupt_rqmid_36694_expose_instruction_breakpoints_001();
	interrupt_rqmid_36695_expose_instruction_breakpoints_005();
	interrupt_rqmid_38047_bp_np();
	interrupt_rqmid_38069_gp_pf();

	interrupt_rqmid_38070_nmi_pf();
	interrupt_rqmid_38110_nmi_001();
#ifdef IN_NON_SAFETY_VM
	interrupt_rqmid_38112_nmi_002();
#endif
	interrupt_rqmid_38135_mask_interrupt_001();
	interrupt_rqmid_38136_mask_interrupt_002();

	interrupt_rqmid_38137_mask_interrupt_003();
	interrupt_rqmid_38157_data_structure_001();
	interrupt_rqmid_38158_data_structure_002();
	interrupt_rqmid_38159_data_structure_003();
	interrupt_rqmid_38173_expose_exception_de_001();

	interrupt_rqmid_38174_expose_exception_de_002();
	interrupt_rqmid_38175_expose_exception_nmi_001();
	interrupt_rqmid_38176_expose_exception_nmi_002();
	interrupt_rqmid_38178_expose_exception_bp_001();
	interrupt_rqmid_38179_expose_exception_bp_002();

	interrupt_rqmid_38263_expose_exception_ud_001();
	interrupt_rqmid_38264_expose_exception_ud_002();
	interrupt_rqmid_38286_expose_exception_nm_001();
	interrupt_rqmid_38287_expose_exception_nm_002();
	interrupt_rqmid_38289_expose_exception_df_001();

	interrupt_rqmid_38290_expose_exception_df_002();
	interrupt_rqmid_38300_expose_exception_np_001();
	interrupt_rqmid_38301_expose_exception_np_002();
	interrupt_rqmid_38352_expose_exception_ss_001();
	interrupt_rqmid_38353_expose_exception_ss_002();

	interrupt_rqmid_38393_expose_exception_pf_001();
	interrupt_rqmid_38394_expose_exception_pf_002();
	interrupt_rqmid_38455_expose_exception_xm_001();
	interrupt_rqmid_38456_expose_exception_xm_002();
	interrupt_rqmid_38458_expose_instruction_breakpoints_002();

	interrupt_rqmid_38460_expose_instruction_breakpoints_004();
	interrupt_rqmid_39087_set_eflags_rf_001();
	interrupt_rqmid_39157_p4_p6_001();
#ifdef IN_NON_SAFETY_VM
	/* non-safety VM shutdowns, no information can be print */
	//interrupt_rqmid_38001_df_pf();
#endif
#ifdef IN_SAFETY_VM
	interrupt_rqmid_38002_df_pf();	//safety call bsp_fatal_error()
#endif
}
