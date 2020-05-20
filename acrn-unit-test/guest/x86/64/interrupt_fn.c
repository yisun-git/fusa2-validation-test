/* used to save rip every time an exception occurs continuously
 */
static int rip_index = 0;
static u64 rip[10] = {0, };

void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	save_error_code = regs->error_code;
	save_rflags1 = regs->rflags;
	save_rflags2 = read_rflags();
	ex_val = regs->vector | (regs->error_code << 16) |
		(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:4" : : "r"(ex_val));

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
	regs->rip += execption_inc_len;
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


/**
 * @brief case name: Page fault while handling a prior page fault_001
 *
 * Summary: The processor in #PF meeting a second #PF will be trigger a #DF.
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
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base), 0x200);

	/* step 5 load NEWGDT */
	new_gdt_desc.base = (ulong)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	/* step 6 prepare the interrupt-gate descriptor of #DF.*/
	/* IDT has been initialized. Only segment selector and handler are set here*/
	set_idt_sel(DF_VECTOR, 0x8);

	/* step 7 prepare the interrupt-gate descriptor of #PF.*/
	/* IDT has been initialized. Only segment selector is set here*/
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

}

/**
 * @brief case name: Interrupt to non-present descriptor_001
 *
 * Summary: When a vCPU attempts to trigger  #BP exception
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
}

struct check_regs {
	unsigned long CR0, CR2, CR3, CR4, CR8;
	//unsigned long RSP, RIP;
	//unsigned long RFLAGS;
	unsigned long RAX, RBX, RCX, RDX;
	unsigned long RDI, RSI;
	unsigned long RBP, RSP;
	unsigned long R8, R9, R10, R11, R12, R13, R14, R15;
	unsigned long CS, DS, ES, FS, GS, SS;
};

struct check_regs regs_check[2];

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
	asm volatile ("mov %%r8, %0" : "=m"(p_regs.R8) : : "memory");		\
	asm volatile ("mov %%r9, %0" : "=m"(p_regs.R9) : : "memory");		\
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

void print_all_regs(struct check_regs *p_regs)
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

	ret = memcmp(&regs_check[0], &regs_check[1], sizeof(struct check_regs));
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
 * Summary:Writing Cr4 reserved bit will trigger GP exception and call
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

	/* step 5 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");

	/*step 6 get 'mov %0, %%cr4' instruction address*/
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 3 is 'mov %rdx,%cr4' instruction len*/
	current_rip -= (5 + 3);

	/* step 7 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/* step 8 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "d"(cr4_value) : "memory");
	/* step 9 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 10 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 11 generate #GP*/
	asm volatile ("mov %0, %%cr4" : : "a"(cr4_value) : "memory");
	/* step 12 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	debug_print("%lx %lx %lx %lx %lx\n", save_rflags1, save_rflags2,
		save_error_code, current_rip, rip[0]);

	/* step 13 check #GP exception, program state and error code*/
	check = check_all_regs();
	report("%s", ((irqcounter_query(GP_VECTOR) == 6) && (check == 0)
		&& (save_error_code == 0) && (current_rip == rip[0])), __FUNCTION__);
}

/**
 * @brief case name:Expose exception and interrupt handling_GP_002
 *
 * Summary: Writing Cr4 reserved bit will trigger GP exception and
 * call the #GP handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_36255_expose_exception_gp_002(void)
{
	ulong cr4_value;
	int check = 0;

	/* step 1 init g_irqcounter */
	irqcounter_initialize();

	/*length of the instruction that generated the exception */
	execption_inc_len = 3;

	/* step 2 prepare the interrupt handler of #GP. */
	handle_exception(GP_VECTOR, &handled_exception);

	/* step 3 init by setup_idt(prepare the interrupt-gate descriptor of #GP)*/

	/* step 4 wrirte to cr4 reserved bit*/
	cr4_value = read_cr4();
	cr4_value |= CR4_RESERVED_BIT23;
	write_cr4(cr4_value);

	/* TF[bit 8], NT[bit 14], RF[bit 16], VM[bit 17] */
	if ((save_rflags2 & RFLAG_TF_BIT) || (save_rflags2 & RFLAG_NT_BIT)
		|| (save_rflags2 & RFLAG_RF_BIT) || (save_rflags2 & RFLAG_VM_BIT)) {
		check = 1;
	}
	/* step 5 check #GP exception and rflag*/
	report("%s", ((irqcounter_query(GP_VECTOR) == 1) && (check == 0)), __FUNCTION__);
}

/**
 * @brief case name: Expose exception and interrupt handling_MF_001
 *
 * Summary:FPU exception(FPU excp: hold), executing EMMS shall generate #MF
 * exception and call the #MF handler with error code 0,
 * program state should be unchanged, the saved contents of EIP registers
 * point to the instruction that generated the exception.
 */
static void interrupt_rqmid_36701_expose_exception_mf_001(void)
{
	float op1 = 1.2;
	float op2 = 0;
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

	/* step 7 and 8 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));

	/*step 9 get 'mov %0, %%cr4' instruction address*/
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len, 2 is 'emms' instruction len*/
	current_rip -= (5 + 2);

	/* step 10 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/* step 11 and 12 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));
	/* step 13 dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);

	/* step 14 dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);
	/* step 15 and 16 build the FPU exception generate #MF*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 "emms\n"
				 :: "m"(cw), "m"(op1), "m"(op2));
	/* step 17 dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	debug_print("%lx %lx %lx %lx %lx\n", save_rflags1, save_rflags2,
		save_error_code, current_rip, rip[0]);

	/* step 18 check #MF, program state register, error code */
	check = check_all_regs();
	report("%s", ((irqcounter_query(MF_VECTOR) == 6) && (check == 0)
		&& (save_error_code == 0) && (current_rip == rip[0])), __FUNCTION__);
}

/**
 * @brief case name:Expose exception and interrupt handling_mf_002
 *
 * Summary: FPU exception(FPU excp: hold), executing EMMS will trigger GP exception
 * and call the #MF handler, EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT
 * is cleared after EFLAGS is saved on the stack.
 */
static void interrupt_rqmid_36702_expose_exception_mf_002(void)
{
	int check = 0;
	float op1 = 1.2;
	float op2 = 0;
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
}

void handled_interrupt_external(isr_regs_t *regs)
{
	irqcounter_incre(EXTEND_INTERRUPT_E0);
	eoi();
}

void test_delay(int time)
{
	__unused int count = 0;
	u64 tsc;
	tsc = rdtsc() + ((u64)time * 1000000000);

	while (rdtsc() < tsc) {
		;
	}
}

unsigned long long asm_read_tsc(void)
{
	long long r;
	unsigned a, d;

	asm volatile ("rdtsc" : "=a"(a), "=d"(d));
	r = a | ((long long)d << 32);
	return r;
}

/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P4_P7_001
 *
 * Summary: Arm a TSC deadline timer with 0x500 periods increament.
 * Immediately execute RDPMC instruction to trigger a VM-exit.
 * This will generate an interrupt and #UD simultaneously after VM-exit finished.
 * #UD should be serviced first, and the specified interrupt should be serviced later.
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

		/* step 4 is define handled_interrupt_external*/
		/* setp 5 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external);

		/* step 6 enable interrupt*/
		irq_enable();

		/* step 7 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24)) {
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);
			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x500);
		}

		/* step 8 trigger VM-exit and generate #UD*/
		t[0] = asm_read_tsc();
		asm volatile ("rdpmc" : "=a"(a), "=d"(d) : "c"(index));
		t[1] = asm_read_tsc();

		/* step 9 enable interrupt*/
		irq_disable();
		debug_print("%s %d %d %d %d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(UD_VECTOR), t[1] - t[0]);

		test_delay(1);
	}

	/* step 10  the test result is: first #UD, second interrupt*/
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_E0) == 1)
		&& (irqcounter_query(UD_VECTOR) == 2)), __FUNCTION__);
}

/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P4_P8_001
 *
 * Summary: Arm a TSC deadline timer with 0x500 periods increament.
 * Immediately execute RDMSR instruction with an invalid MSR to trigger a VM-exit.
 * This will generate an interrupt and #GP simultaneously after VM-exit finished.
 * #GP should be serviced first, and the specified interrupt should be serviced later.
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

		/* step 4 is define handled_interrupt_external*/
		/* setp 5 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external);

		/* step 6 enable interrupt*/
		irq_enable();

		/* step 7 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24))
		{
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);
			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+0x500);
		}

		/* step 8 trigger VM-exit and generate #GP*/
		t[0] = asm_read_tsc();
		asm volatile ("rdmsr" :: "c"(0xD20) : "memory");
		t[1] = asm_read_tsc();

		/* step 9 disable interrupt*/
		irq_disable();
		debug_print("%s %d %d %d %d tsc_delay=0x%lx\n", __FUNCTION__, __LINE__, i,
			irqcounter_query(EXTEND_INTERRUPT_E0), irqcounter_query(GP_VECTOR), t[1]-t[0]);
		test_delay(1);
	}

	/* step 10 the test result is: first #GP, second interrupt*/
	report("%s", ((irqcounter_query(EXTEND_INTERRUPT_E0) == 1)
		&& (irqcounter_query(GP_VECTOR) == 2)), __FUNCTION__);
}

/**
 * @brief case name: Expose priority among simultaneous exceptions and interrupts_P3_P4_001
 *
 * Summary: Sync vCPU1 and vCPU2 so that vCPU1 arm a TSC deadline timer with 0x2500
 * clock periods increament and vCPU2 send NMI IPI to vCPU1 concurrently.
 * On vCPU1, immediately execute an instruction which would trigger a VM-exit.
 * This will generate an NMI and specified interrupt simultaneously
 * after VM-exit finished. #NMI should be serviced first,
 * and the specified interrupt should be serviced later.
 */
volatile int interrupt_wait_bp = 0;
static void interrupt_rqmid_36693_p3_p4_ap_001(void)
{
	int i = 1;
	if (get_lapic_id() != 1) {
		return;
	}

	while (i--) {
		/* setp 1*/
		while (interrupt_wait_bp == 0) {
			asm volatile ("nop\n\t" :::"memory");
		}

		/* setp 10 AP send NMI to BP*/
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT, 0);
		interrupt_wait_bp = 0;
	}
}

static __unused void interrupt_rqmid_36693_p3_p4_001(void)
{
	__unused u64 t[2];
	int i = 1;

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

		/* step 5 is define handled_interrupt_external*/
		/* setp 6 set_idt_entry for irq */
		handle_irq(EXTEND_INTERRUPT_E0, handled_interrupt_external);

		/* step 7 enable interrupt*/
		irq_enable();

		/* step 8 use TSC deadline timer delivery 224 interrupt*/
		if (cpuid(1).c & (1 << 24))
		{
			apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_E0);
			//apic_write(APIC_TDCR, APIC_TDR_DIV_4);
			//apic_write(APIC_TMICT, 0x2000);

			/* step 9 BP sends synchronization message to AP*/
			interrupt_wait_bp = 1;

			wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x2500));
		}

		t[0] = asm_read_tsc();
		/* step 10 BP trigger VM-exit*/
		cpuid_indexed(0x16, 0);
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
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_001
 *
 * Summary: Trigger a fault-class exception #UD,
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
}

/**
 * @brief case name:Expose masking and unmasking Instruction breakpoints_005
 *
 * Summary: Trigger a fault-class exception #UD,
 * check that the value of EFLAGS.RF pushed on the stack is 1.
 * Set EFLAGS.RF, execute INT1 instruction, check that no #GP(0) is captured
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
}

static void print_case_list_64(void)
{
	printf("\t Interrupt feature 64-Bits Mode case list:\n\r");

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
	printf("Case ID:%d case name:%s\n\r", 36693,
		"Expose priority among simultaneous exceptions and interrupts_P3_P4_001");
	printf("Case ID:%d case name:%s\n\r", 36694,
		"Expose masking and unmasking Instruction breakpoints_001");
	printf("Case ID:%d case name:%s\n\r", 36695,
		"Expose masking and unmasking Instruction breakpoints_005");
}

static void test_ap_interrupt_64(void)
{
	interrupt_rqmid_36693_p3_p4_ap_001();
}

static void test_interrupt_64(void)
{
	/* __x86_64__*/
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
}
