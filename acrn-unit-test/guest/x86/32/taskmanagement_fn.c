
gdt_entry_t *new_gdt;
struct descriptor_table_ptr old_gdt_desc;
struct descriptor_table_ptr new_gdt_desc;

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;
static volatile int esp = 0;
u16 tr_selector;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		if (modify_init_func != NULL) {
			modify_init_func();
		}
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_cr0_ts_init_value()
{
	ulong cr0;
	cr0 = read_cr0();
	write_cr0(cr0 | X86_CR0_TS);
}

__unused static void modify_eflags_nt_init_value()
{
	eflags_nt_to_1();
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 23827:
		fp = modify_cr0_ts_init_value;
		ap_init_value_process(fp);
		break;
	case 23830:
		fp = modify_eflags_nt_init_value;
		ap_init_value_process(fp);
		break;
	case 26024:
	/*In Cstart64.S , load_tss modified this value, here we need not modify it*/
		ap_init_value_process(NULL);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}


#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...) \
	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif

bool eflags_nt_to_1(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	check_bit = read_rflags();

	/***** Set current TSS's EFLAGS.NT[bit 14] to 1 ******/
	check_bit |= X86_EFLAGS_NT;
	write_rflags(check_bit);

	if (check_bit == read_rflags())
		result = true;
	else
		result = false;

	return result;
}

bool eflags_nt_to_0(void)
{
	unsigned long check_bit = 0;
	bool result = false;

	check_bit = read_rflags();
	/***** Set current TSS's EFLAGS.NT[bit 14] to 0 *****/
	check_bit &= (~(X86_EFLAGS_NT));

	write_rflags(check_bit);

	if (check_bit == read_rflags())
		result = true;
	else
		result = false;

	return result;
}

void init_do_less_privilege(void)
{
	setup_ring_env();
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


static __unused void test_tr_print(const char *msg)
{
	test_cs = read_cs();
}

static __unused void test_tr_init(void)
{
	tss32_t *tss_ap1;
	u16 desc_size = 0;
	u16 ss_val = 0;

	setup_vm();
	setup_idt();
	setup_tss32();
	init_do_less_privilege();

	tss_ap1 = &tss;
	tss_ap1 += get_cpu_id();

	tss_ap1->ss0 = (GDT_NEWTSS_INDEX << 3);

	desc_size = sizeof(tss32_t);
	memset((void *)0, 0, desc_size);
	memcpy((void *)0, (void *)tss_ap1, desc_size);

	do_at_ring3(test_tr_print, "TR test");

	ss_val = read_ss();

	report("taskm_id_24027_tr_init", ss_val == tss_ap1->ss0);

	//resume environment
	tss_ap1->ss0 = 0x10;
}


/* cond 1 */
static __unused void cond_1_gate_present(void)
{
	gate.p = 0;
}

/* cond 3 */
static __unused void cond_3_gate_null_sel(void)
{
	gate.selector = 0;
}

/* cond 10 */
static __unused void cond_10_desc_busy(void)
{
	gdt32[TSS_INTR>>3].access |= 0x2;/*busy = 1*/
}

static __unused void tss_desc_not_present(void)
{
	gdt32[TSS_INTR>>3].access &= (~0x80);
}

/* cond 13 */
void __unused cond_13_desc_tss_not_page(void)
{
	unsigned char *linear_addr;

	tss_intr.eip = (u32)irq_tss;

	gate.selector = 0x1008;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);

	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE),
		(void *)(old_gdt_desc.base), 0x200);
	new_gdt_desc.base = (u32)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)),
		PAGE_PTE, 0, 0, true);
}


static __unused void cond_3_iret_desc_type(void)
{
	gdt32[TSS_INTR>>3].access &= 0xf0;
	gdt32[TSS_INTR>>3].access |= 0x09;/*type = 1001B*/
}

static inline u32 read_esp(void)
{
	u32 val;

	asm volatile ("mov %%esp, %0" : "=mr"(val));

	return val;
}

#if defined(IN_NON_SAFETY_VM) || defined(IN_SAFETY_VM)
static bool set_UMIP(bool set)
{
	#define CPUID_7_ECX_UMIP (1 << 2)

	int cpuid_7_ecx;
	u32 cr4 = 0;

	cpuid_7_ecx = cpuid(7).c;
	if (!(cpuid_7_ecx & CPUID_7_ECX_UMIP))
		return false;

	cr4 = read_cr4();
	if (set)
		cr4 |= X86_CR4_UMIP;
	else
		cr4 &= ~X86_CR4_UMIP;

	write_cr4(cr4);

	return true;
}

static void ltr_from_general_purpose_register(void *data)
{
	asm volatile("mov $" xstr(TSS_INTR)", %%ax\n\t"
			"ltr %%ax\n\t"
			"1:":::);
}

static void test_ltr_for_exception(const char *data)
{
	bool ret = false;

	ret = test_for_exception(GP_VECTOR,
		ltr_from_general_purpose_register, NULL);

	report("%s", ret, data);
}

static void str_to_general_purpose_register(const char *data)
{
	u16 val = 0;

	asm volatile(ASM_TRY("1f")
		"str %%ax\n\t"
		"mov %%ax,%0\n\t":"=m"(val)::);

	report("%s", ((val == TSS_MAIN) || (val == TSS_INTR)), data);
}

static u16 tr_val;
static void str_to_memory(const char *data)
{
	asm volatile("str %0\n\t":"=m"(tr_val)::);
}

static void str_to_memory_exception(void *data)
{
	u16 val = 0;

	asm volatile("str %0\n\t":"=m"(val)::);
}

static void test_str_mem_for_exception(const char *data)
{
	bool ret = false;

	ret = test_for_exception(GP_VECTOR, str_to_memory_exception, NULL);

	report("%s", ret, data);
}

static void trigger_NMI(void *data)
{
	u32 val = APIC_DEST_PHYSICAL | APIC_DM_NMI | APIC_INT_ASSERT;
	u32 dest = 0;

	asm volatile("wrmsr\n\t"::"a"(val),\
		"d"(dest), "c"(APIC_BASE_MSR + APIC_ICR/16));
}


/**
 * @brief case name:
 *	Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_001
 *
 * Summary: Present bit of task gate is 0, it would
 *          generate #NP(Error code = (vector << 3H) | 3H )
 *          when switching to a task gate in IDT by NMI
 */
static void taskm_id_25205_nmi_gate_p_bit(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_1_gate_present();

	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(gate));

	test_for_exception(NP_VECTOR, trigger_NMI, NULL);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((NMI_VECTOR << 3) | 3)),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_002
 *
 * Summary: TSS selector in the task gate is a null selector,
 *	it would gernerate #GP(Error code == 1H )
 *	when switching to a task gate in IDT by NMI
 */
static void taskm_id_25206_nmi_null_selector(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = 0; /*Set null selector*/
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate), sizeof(gate));

	test_for_exception(GP_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == 0x1), __func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_003
 *
 * Summary: TSS selector in the task gate has TI flag set,
 *	it would gernerate #GP(Error code == (TSS selector & F8H) | 05H )
 *	when switching to a task gate in IDT by NMI.
 */
static void taskm_id_25207_nmi_TI_flag_set(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = (TSS_INTR + 4); /*Set TI flag*/
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(GP_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x5)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}
/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_004
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,
 *	it would gernerate #GP(Error code == (TSS selector & F8H) | 01H )
 *	when switching to a task gate in IDT by NMI.
 */
static void taskm_id_25208_nmi_gdt_limit_overflow(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	sgdt(&old_gdt_desc);

	gate.selector = old_gdt_desc.limit + 1;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(GP_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((gate.selector & 0xFFF8) | 0x1)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_005
 *
 * Summary: Page fault occurs when read the descriptor at index of TSS
 *	    selector, it would gernerate #PF when switching to a task gate
 *	    in IDT by NMI.
 */
static void taskm_id_26173_nmi_tss_desc_page_fault(void)
{
	unsigned char *linear_addr;
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));
	gate.selector = 0x1008;//TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);

	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE),
		(void *)(old_gdt_desc.base), 0x200);
	new_gdt_desc.base = (u32)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)),
		PAGE_PTE, 0, 0, true);

	test_for_exception(PF_VECTOR, trigger_NMI, NULL);

	report("%s", (exception_vector() == PF_VECTOR), __func__);

	/*Restore environment*/
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(new_gdt_desc.base + PAGE_SIZE)),
		PAGE_PTE, 0, 1, true);
	free((void *)new_gdt_desc.base);
}
/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_006
 *
 * Summary: The descriptor at index of TSS selector is not a non-busy TSS
 *	descriptor (i.e. [bit 44:40] != 01001B), it would gernerate
 *	#GP(Error code == (TSS selector & F8H) | 01H )
 *	when switching to a task gate in IDT by NMI.
 */
static void taskm_id_25209_nmi_tss_desc_busy(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	cond_10_desc_busy();

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(GP_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x1)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_007
 *
 * Summary: Present bit of the TSS descriptor is 0,
 *	it would gernerate #NP(Error code == (TSS selector & F8H) | 01H )
 *	when switching to a task gate in IDT by NMI.
 */
static void taskm_id_25210_nmi_tss_desc_not_present(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	tss_desc_not_present();

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(NP_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x1)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}
/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_008
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	   32-bit TSS (i.e. [bit 44:40] is 01001B) and the limit of the TSS
 *	   descriptor is lees than 67H, it would gernerate
 *	   #TS(Error code == (TSS selector & F8H) | 01H ) when switching to a
 *	   task gate in IDT by NMI.
 */
static void taskm_id_25211_nmi_tss_desc_limit_67H(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;
	gdt32[TSS_INTR>>3].limit_low = 0x66;

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(TS_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x1)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode
 *	Exception_Checking_on_IDT_by_NMI_009
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    16-bit TSS (i.e. [bit 44:40] is 0001B) and the limit of the TSS
 *	    descriptor is less than 2BH, it would gernerate
 *	    #TS(Error code == (TSS selector & F8H) | 01H )
 *	    when switching to a task gate in IDT by NMI.
 */
static void taskm_id_26093_nmi_tss_desc_limit_2BH(void)
{
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[2];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;
	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));
	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[2]), (void *)(&gate),	sizeof(gate));

	test_for_exception(TS_VECTOR, trigger_NMI, NULL);
	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x1)),
		__func__);

	//resume environment
	memcpy((void *)(&boot_idt[2]), (void *)&bak, sizeof(idt_entry_t));
}

static int ret_val_tc24564;
static int error_code_tc24564;
static void call_ring3_tc24564(const char *data)
{
	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	ret_val_tc24564 = exception_vector();
	error_code_tc24564 = exception_error_code();
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_001
 *
 * Summary: CPL > task gate DPL, it would gernerate
 *	    #GP(Error code == (vector << 3H) | 02H) when switching to a task
 *	    gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_24564_int_CPL_over_DPL(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	do_at_ring3(call_ring3_tc24564, __func__);

	report("%s", ((ret_val_tc24564 == GP_VECTOR) &&
		(error_code_tc24564 == ((6 << 3) | 0x2))), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_002
 *
 * Summary: Present bit of task gate is 0, it would gernerate
 *	    #NP(Error code == (vector << 3H) | 02H) when switching to a task
 *	    gate in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25087_int_gate_present_clear(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 0; /*Clear present bit*/
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR) &&
		(exception_error_code() == ((6 << 3) | 0x2))), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_003
 *
 * Summary: TSS selector in the task gate is a null selector,
 *          it would generate #GP(Error code = 0) when switching to
 *          a task gate in IDT by a software interrupt
 *	    (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25089_int_gate_selector_null(void)
{
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	cond_3_gate_null_sel();

	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == 0), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_004
 *
 * Summary: TSS selector in the task gate has TI flag set, it would gernerate
 *	    #GP(Error code == (TSS selector & F8H) | 04H) when switching to a
 *	    task gate in IDT by a software interrupt(i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25092_int_gate_TI_flag_set(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));
	gate.selector = (TSS_INTR + 4); /*Set TI bit*/
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x4))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_005
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,
 *	it would gernerate #GP(Error code ==TSS selector & F8H)
 *	when switching to a task gate in IDT by a software interrupt
 *	(i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25093_int_gate_selector_index_overflow(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));

	/*the selector = GDT.limits + 1*/
	sgdt(&old_gdt_desc);

	gate.selector = old_gdt_desc.limit + 1;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (gate.selector & 0xFFF8))),
		__func__);
}
/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_006
 *
 * Summary:Page fault occurs when read the descriptor at index of TSS selector,
 *	it would gernerate #PF when switching to a task gate in IDT by a
 *	software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_26172_int_pagefault(void)
{
	unsigned char *linear_addr;
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	memset(e, 0, sizeof(*e));
	gate.selector = 0x1008;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);

	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE),
		(void *)(old_gdt_desc.base), 0x200);
	new_gdt_desc.base = (u32)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);

	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)),
		PAGE_PTE, 0, 0, true);

		asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", (exception_vector() == PF_VECTOR), __func__);

	/*Restore environment*/
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(new_gdt_desc.base + PAGE_SIZE)),
		PAGE_PTE, 0, 1, true);
	free((void *)new_gdt_desc.base);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_007
 *
 * Summary: The descriptor at index of TSS selector is not a non-busy TSS
 *	    descriptor (i.e. [bit 44:40] != 01001B), it would gernerate
 *	    #GP(Error code ==TSS selector & F8H) when switching to a task gate
 *	    in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25195_int_tss_desc_busy(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	cond_10_desc_busy();

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_008
 *
 * Summary: Present bit of the TSS descriptor is 0,
 *	it would gernerate #NP(Error code ==TSS selector & F8H)
 *	when switching to a task gate in IDT by a software interrupt
 *	(i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25196_int_tss_desc_present_clear(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	tss_desc_not_present();

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8))), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_009
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    32-bit TSS (i.e. [bit 44:40] is 01001B) and the limit of the TSS
 *	    descriptor is lees than 67H, it would gernerate #TS
 *	    (Error code ==TSS selector & F8H) when switching to a task gate
 *	    in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_25197_int_tss_desc_limit_67H(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;
	gdt32[TSS_INTR>>3].limit_low = 0x66;

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8))), __func__);

}
/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_IDT_by_Software_interrupt_010
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    16-bit TSS (i.e. [bit 44:40] is 0001B) and the limit of the TSS
 *	    descriptor is less than 2BH, it would gernerate
 *	    #TS(Error code == TSS selector & F8H) when switching to a task gate
 *	    in IDT by a software interrupt (i.e. INT n, INT3 or INTO).
 */
static void taskm_id_26091_int_tss_desc_limit_2BH(void)
{
	idt_entry_t *e = &boot_idt[6];

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"int $6\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8))), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_001
 *
 * Summary: The previous task link is a null selector,
 *	    it would gernerate #TS(Error code =0)
 *	    when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_25212_iret_prelink_null_sel(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;
	cond_10_desc_busy();

	cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = 0;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == 0), __func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_002
 *
 * Summary: The previous task link has TI flag set,
 *	it would gernerate #TS(Error code == (previous task link & F8H) | 04H)
 *	when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_25219_iret_prelink_TI_flag_set(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	cond_10_desc_busy();

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = (TSS_INTR + 4); /*Set TI*/
	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == ((TSS_INTR & 0xF8) | 0x04)),
		__func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_003
 *
 * Summary: Index of the previous task link is out of GDT limits,
 *	it would gernerate #TS(Error code == previous task link & FFF8H)
 *	when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_25221_iret_prelink_gdt_overflow(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	cond_10_desc_busy();

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	sgdt(&old_gdt_desc);
	tss.prev = old_gdt_desc.limit + 1;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (tss.prev & 0xFFF8)), __func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_004
 *
 * Summary: Page fault occurs when read the descriptor at index of the
 *	    previous task link, it would gernerate #PF when executing IRET
 *	    with EFLAGS.NT = 1.
 */
static void taskm_id_26174_iret_prelink_pagefault(void)
{
	unsigned char *linear_addr;
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	cond_10_desc_busy();

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = 0x1008;

	linear_addr = (unsigned char *)malloc(PAGE_SIZE * 2);
	sgdt(&old_gdt_desc);
	memcpy((void *)linear_addr, (void *)(old_gdt_desc.base), 0x200);
	memcpy((void *)(linear_addr+PAGE_SIZE), (void *)(old_gdt_desc.base),
		0x200);
	new_gdt_desc.base = (u32)linear_addr;
	new_gdt_desc.limit = PAGE_SIZE * 2;
	lgdt(&new_gdt_desc);
	set_page_control_bit((void *)((ulong)(linear_addr + PAGE_SIZE)),
		PAGE_PTE, 0, 0, true);

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == PF_VECTOR), __func__);

	//resume environment
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(new_gdt_desc.base + PAGE_SIZE)),
		PAGE_PTE, 0, 1, true);
	free((void *)new_gdt_desc.base);

	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_005
 *
 * Summary: The descriptor at index of the previous task link is not a busy
 *	TSS descriptor (i.e. [bit 44:40] == 01001B), it would generate
 *	#TS(Error code == previous task link & F8H) when executing IRET
 *	with EFLAGS.NT = 1.
 */
static void taskm_id_25222_iret_type_not_busy(void)
{
	u64 cr0_val = read_cr0();
	u64 rflags = read_rflags();

	write_cr0(cr0_val | X86_CR0_TS);
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
		(exception_error_code() == (TSS_INTR & 0xf8)), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_006
 *
 * Summary: Present bit of the TSS descriptor is 0,
 *	it would gernerate #NP(Error code == previous task link & F8H)
 *	when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_25223_iret_tss_desc_not_present(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	cond_10_desc_busy();
	tss_desc_not_present();

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8)), __func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_007
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	32-bit TSS(i.e. [bit 44:40] is 01001B) and the limit of the TSS
 *	descriptor is lees than 67H,it would gernerate
 *	#TS(Error code ==previous task link & F8H)
 *	when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_25224_iret_tss_desc_limit_67H(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
		/*Set present bit of TSS descriptor to 0*/
	gdt32[TSS_INTR>>3].limit_low = 0x66;
	gdt32[TSS_INTR>>3].access = 0x8B; /*p=1, type:1011b*/

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8)), __func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_IRET_008
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	16-bit TSS (i.e. [bit 44:40] is 00001B) and limit of the TSS descriptor
 *	is less than 2BH,it would gernerate #TS(Error code == previous task
 *	link & F8H) when executing IRET with EFLAGS.NT = 1.
 */
static void taskm_id_26094_iret_tss_desc_limit_2BH(void)
{
	u64 cr0_val = 0;
	u64 rflags = 0;

	setup_tss32();
	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	cr0_val = read_cr0();
	write_cr0(cr0_val | X86_CR0_TS);

	rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_NT);

	tss.prev = TSS_INTR;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == TS_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xF8)), __func__);

	//resume environment
	write_cr0(cr0_val);
	write_rflags(rflags);
	tss.prev = 0;
	gdt32[TSS_INTR>>3].access = 0x89;
}

static int ret_val_tc24561;
static int error_code_tc24561;
static void call_ring3_tc24561(const char *data)
{
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	ret_val_tc24561 = exception_vector();
	error_code_tc24561 = exception_error_code();
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_001
 *
 * Summary: CPL > task gate DPL, it would gernerate #GP(Error code == gate
 *	    selector & F8H) when executing a CALL or JMP instruction to
 *	    a task gate.
 */
static void taskm_id_24561_task_gate_CPL_check(void)
{
	u8 gdt_access = 0;

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*Update desriptor of TSS in GDT table*/
	gdt_access = gdt32[TSS_INTR >> 3].access;
	gdt32[TSS_INTR >> 3].access = 0xE9;

	/*construct task gate in GDT for TSS.*/
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	do_at_ring3(call_ring3_tc24561, __func__);

	report("%s", ((ret_val_tc24561 == GP_VECTOR)
		&& (error_code_tc24561 == (TASK_GATE_SEL & 0xF8))),
		__func__);

	/*Restore enviroment*/
	gdt32[TSS_INTR >> 3].access = gdt_access;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_002
 *
 * Summary: Task gate selector RPL > task gate DPL, it would gernerate
 *	    #GP(Error code == gate selector & F8H)
 *	    when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_24613_task_gate_RPL_check(void)
{
	u8 gdt_access = 0;

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*Update desriptor of TSS in GDT table*/
	gdt_access = gdt32[TSS_INTR >> 3].access;
	gdt32[TSS_INTR >> 3].access = 0xE9;

	/*construct task gate in GDT for TSS.*/
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]), (void *)(&gate),
		sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL+3) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR)
		&& (exception_error_code() == (TASK_GATE_SEL & 0xF8))),
		__func__);

	/*Restore enviroment*/
	gdt32[TSS_INTR >> 3].access = gdt_access;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_003
 *
 * Summary: Present bit of task gate is 0,it would gernerate
 *	    #NP(Error code == gate selector & F8H)
 *	when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_24562_task_gate_present_bit_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*construct task gate in GDT for TSS.*/
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 0;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR)
		&& (exception_error_code() == (TASK_GATE_SEL & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_004
 *
 * Summary:TSS selector in the task gate is a null selector,
 *	it would gernerate #NP(Error code == gate.selector & 0xF8)
 *	when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_24566_task_gate_selector_null_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*construct task gate in GDT for TSS.*/
	gate.selector = 0;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 0;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TASK_GATE_SEL & 0xF8))),
		__func__);

	/*Restore environment*/
	gate.selector = TSS_INTR;
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_005
 *
 * Summary:TSS selector in the task gate has TI flag set,it would gernerate
 * #GP(Error code = (TSS selector & F8H) | 04H) when executing a CALL or JMP
 * instruction to a task gate.
 */
static void taskm_id_25023_task_gate_selector_TI_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*construct task gate in GDT for TSS.*/
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL+4) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR)
		&& (exception_error_code() == (0x04 | (TASK_GATE_SEL & 0xF8)))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_006
 *
 * Summary: Index of TSS selector in the task gate is out of GDT limits,
 *	    it would gernerate #GP when executing a CALL or JMP instruction to
 *	    a task gate.
 */
static void taskm_id_25025_task_gate_out_of_gdt_limit_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*construct task gate in GDT for TSS.*/
	sgdt(&old_gdt_desc);
	gate.selector = old_gdt_desc.limit + 1;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_007
 *
 * Summary: Page fault occurs when read the descriptor
 *          at index of TSS selector,it would generate #PF
 *          when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_26171_task_gate_page_fault(void)
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

	report("%s", (exception_vector() == PF_VECTOR), __func__);

	/* resume environment */
	lgdt(&old_gdt_desc);
	set_page_control_bit((void *)((ulong)(new_gdt_desc.base + PAGE_SIZE)),
		PAGE_PTE, 0, 1, true);
	free((void *)new_gdt_desc.base);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008
 *
 * Summary:The descriptor at index of TSS selector is a busy TSS descriptor
 *	(i.e. [bit 44:40] != 01001B),it would gernerate #GP(Error code =
 *	TSS selector & F8H) when executing a CALL or JMP instruction to
 *	a task gate.
 */
static void taskm_id_25026_task_gate_tss_busy_check(void)
{
	/*construct tss_intr*/
	setup_tss32();

	cond_10_desc_busy();

	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_009
 *
 * Summary:Present bit of the TSS descriptor is 0,it would gernerate
 *	   #NP(Error code = TSS selector & F8H) when executing a CALL or
 *	   JMP instruction to a task gate.
 */
static void taskm_id_25024_task_gate_tss_present_check(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*p=0, dpl=0, system=0, type:1001b,tss(avail)*/
	gdt32[TSS_INTR >> 3].access = 0x09;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 1;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_010
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    32-bit TSS(i.e. [bit 44:40] is 01001B) and the limit of the TSS
 *	    descriptor is lees than 67H, it would gernerate
 *	    #TS(Error code = TSS selector & F8H)
 *  when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_25028_task_gate_tss_desc_limit_67H_check(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x66;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 1;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected mode_Exception_Checking_on_Task_gate_011
 *
 * Summary:G bit of the TSS descriptor is 0 and The TSS descriptor
 *	points to 16-bit TSS (i.e. [bit 44:40] is 0001B) and the
 *	limit of the TSS descriptor is less than 2BH,
 *	it would gernerate #TS(Error code = TSS selector & F8H)
 *	when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_26061_task_gate_tss_desc_limit_2BH(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 1;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

static int ret_val_tc24544;
static int error_code_tc24544;
static void call_ring3_tc24544(const char *data)
{
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	ret_val_tc24544 = exception_vector();
	error_code_tc24544 = exception_error_code();
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_TSS_selector_001
 *
 * Summary: CPL > TSS descriptor DPL, it would gernerate #GP(Error code = TSS
 *	    selector & F8H) when executing a CALL or JMP instruction with a
 *	    TSS selector to a TSS descriptor.
 */
static void taskm_id_24544_tss_CPL_over_DPL(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	do_at_ring3(call_ring3_tc24544, __func__);

	report("%s", ((ret_val_tc24544 == GP_VECTOR)
		&& (error_code_tc24544 == (TSS_INTR & 0xF8))), __func__);
}

/**
 * @brief case name:
 *  Invalid task switch in protected
 *  mode_Exception_Checking_on_TSS_selector_002
 *
 * Summary: TSS selector RPL > TSS descriptor DPL,
 *	it would gernerate #GP(Error code == TSS selector & F8H)
 *	when executing a CALL or JMP instruction with a TSS selector to
 *	a TSS descriptor.
 */
static void taskm_id_24546_tss_RPL_over_DPL(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR + 3) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 * Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_003
 *
 * Summary: The TSS selector has TI flag set,
 *	it would gernerate #GP(Error code == (TSS selector & F8H) | 04H)
 *	when executing a CALL or JMP instruction with a TSS selector
 *	to a TSS descriptor.
 */
static void taskm_id_24545_tss_TI_flag_set(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR + 4) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR)
		&& ((exception_error_code() & 0xF8) == TSS_INTR)),
		__func__);
}

/**
 * @brief case name:
 * Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_004
 *
 * Summary: The TSS descriptor is busy (i.e. [bit 41] = 1),
 *          it would generate #GP(Error code = TSS selector & F8H)
 *          when executing a CALL or JMP instruction with a
 *          TSS selector to a TSS descriptor.
 */
static void taskm_id_25030_tss_desc_busy(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	cond_10_desc_busy();

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __func__);
}

/**
 * @brief case name:
 * Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_005
 *
 * Summary: Present bit of the TSS descriptor is 0,
 *	it would gernerate #NP(Error code == TSS selector & F8H)
 *	when executing a CALL or JMP instruction with a TSS selector to
 *	a TSS descriptor.
 */
static void taskm_id_25032_tss_desc_present_bit_clear(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*p=0, dpl=0, system=0, type:1001b,tss(avail)*/
	gdt32[TSS_INTR >> 3].access = 0x09;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == NP_VECTOR) &&
		(exception_error_code() == (TSS_INTR & 0xf8)), __func__);
}
/**
 * @brief case name:
 * Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_006
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    32-bit TSS (i.e. [bit 44:40] is 01001B) and the limit of the TSS
 *	    descriptor is lees than 67H, it would gernerate
 *	    #TS(Error code = TSS selector & F8H) when executing a CALL or
 *	    JMP instruction with a TSS selector to a TSS descriptor.
 */
static void taskm_id_25034_tss_limit_67H(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x66;

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 * Invalid task switch in protected mode_Exception_Checking_on_TSS_selector_007
 *
 * Summary: G bit of the TSS descriptor is 0 and The TSS descriptor points to
 *	    16-bit TSS(i.e. [bit 44:40] is 00001B) and limit of the TSS
 *	    descriptor is less than 2BH, it would gernerate
 *	    #TS(Error code = TSS selector & F8H) when executing a CALL
 *	    or JMP instruction with a TSS selector to a TSS descriptor.
 */
static void taskm_id_26088_tss_limit_2BH(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*Set present bit of TSS descriptor to 0*/
	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}


/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_001
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3]
 *          unchanged,while switch task by CALL
 */
static void taskm_id_23803_cr0_ts_unchanged_call(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ((ts_val1 == ts_val2) && (ts_val1 == 0) &&
		(exception_vector() == GP_VECTOR)), __func__);
}

/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_002
 *
 * Summary: ACRN hypervisor shall keep guest CR0.TS[bit 3] unchanged,
 *	while switch task by JMP
 */
static void taskm_id_23835_cr0_ts_unchanged_call(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	ts_val1 = (read_cr0()>>3) & 1;

	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"ljmp $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ((ts_val1 == ts_val2) && (ts_val1 == 0) &&
		(exception_vector() == GP_VECTOR)), __func__);

}

/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_003
 *
 * Summary:ACRN hypervisor shall keep guest CR0.TS[bit 3] unchanged,
 *		while switch task by interrupt
 */
static void taskm_id_23836_cr0_ts_unchanged_call(void)
{
#define INTR_VEC 35
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;

	/*construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	/*construct task gate and install it to IDT*/
	idt_entry_t bak;
	idt_entry_t *e = &boot_idt[INTR_VEC];

	memcpy((void *)&bak, (void *)e, sizeof(idt_entry_t));


	memset(e, 0, sizeof(*e));
	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&boot_idt[INTR_VEC]),
		(void *)(&gate), sizeof(gate));

	ts_val1 = (read_cr0()>>3) & 1;

	asm volatile(ASM_TRY("1f")
			"int $35\n\t"
			"1:":::);

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ((ts_val1 == ts_val2) && (ts_val1 == 0) &&
		(exception_vector() == GP_VECTOR)), __func__);

	//resume environment
	memcpy((void *)(&boot_idt[INTR_VEC]), (void *)&bak,
		sizeof(idt_entry_t));
}

/**
 * @brief case name:guest CR0.TS keeps unchanged in task switch attempts_004
 *
 * Summary:ACRN hypervisor shall keep guest CR0.TS[bit 3] unchanged,
 *	while switch task by IRET
 */
static void taskm_id_24008_cr0_ts_unchanged_call(void)
{
	ulong ts_val1 = 0;
	ulong ts_val2 = 0;
	u16 sel_bak;

	/*construct tss_intr*/
	setup_tss32();

	cond_10_desc_busy();

	/*set eflags.NT bit*/
	if (!eflags_nt_to_1())
		debug_print("Err: fail to set eflags.NT bit.\n");

	/*set CR0.TS bit*/
	write_cr0_ts(1);

	/*set prelink to TSS_INTR*/
	sel_bak = tss.prev;
	tss.prev = TSS_INTR;

	ts_val1 = (read_cr0()>>3) & 1;

	asm volatile(ASM_TRY("1f")
		"iret\n\t"
			"1:":::);

	ts_val2 = (read_cr0()>>3) & 1;

	report("%s", ((ts_val1 == ts_val2) && (ts_val1 == 1) &&
		(exception_vector() == GP_VECTOR)), __func__);

	/*resume environment*/
	tss.prev = sel_bak;
	write_cr0_ts(0);
	eflags_nt_to_0();
}

/**
 * @brief case name:Task Management expose CR0.TS_001
 *
 * Summary: Set TS and clear EM, execute SSE instruction shall generate #NM.
 */
static void taskm_id_23821_cr0_ts_expose(void)
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
	report("%s", (exception_vector() == NM_VECTOR), __func__);
}


/**
 * @brief case name:Task Management expose CR0.TS_002
 *
 * Summary: Set CR0.TS and clear CR0.EM, execute PAUSE instruction
 *	    would get no exception.
 */
static void taskm_id_23890_cr0_ts_expose(void)
{
	u32 cr0_bak;
	u32 cr4_bak;

	cr0_bak = read_cr0();
	cr4_bak = read_cr4();

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	write_cr0(read_cr0() & ~4); /* clear EM */

	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0_ts(1);

	asm volatile(ASM_TRY("1f")
			"pause\n\t"
			"1:":::);

	//resume environment
	write_cr0(cr0_bak);
	write_cr4(cr4_bak);
	report("%s", (exception_vector() == NO_EXCEPTION), __func__);
}

/**
 * @brief case name:Task Management expose CR0.TS_003
 *
 * Summary: Set TS, clear CR0.EM, CR0.MP, execute WAIT instruction
 *	    would not get #NM exception
 */
static void taskm_id_23892_cr0_ts_expose(void)
{
	u32 cr0_bak;
	u32 cr4_bak;

	cr0_bak = read_cr0();
	cr4_bak = read_cr4();

	write_cr0_ts(0);
	write_cr4_osxsave(1);

	write_cr0(read_cr0() & ~6); /* clear EM(bit2) and MP(bit1)*/

	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0_ts(1);

	asm volatile(ASM_TRY("1f")
			"wait\n\t"
			"1:":::);

	//resume environment
	write_cr0(cr0_bak);
	write_cr4(cr4_bak);
	report("%s", (exception_vector() == NO_EXCEPTION), __func__);
}


/**
 * @brief case name:Task Management expose CR0.TS_004
 *
 * Summary: Set CR0.EM, CR0.TS set and executes SSE instruction MOVAPS.
 *         Set CR0.EM, Clear TS not set and executes SSE instruction MOVAPS.
 */
static void taskm_id_23893_cr0_ts_expose(void)
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

	write_cr4(read_cr4() | 0x200); /* OSFXSR */

	write_cr0(read_cr0() | (1<<2)); /*Set EM(bit2)*/

	asm volatile(ASM_TRY("1f")
			"movaps %1, %%xmm1\n\t"
			"1:":"=m"(v):"m"(add):"memory");

	if (exception_vector() == UD_VECTOR) {
		write_cr0_ts(1);

		asm volatile(ASM_TRY("1f")
			"movaps %1, %%xmm1\n\t"
			"1:":"=m"(v):"m"(add):"memory");

		report("%s", (exception_vector() == UD_VECTOR), __func__);
	} else {
		report("%s", false, __func__);
	}

	//resume environment
	write_cr0(cr0_bak);
	write_cr4(cr4_bak);
}


/**
 * @brief case name:Task Management expose TR_001
 *
 * Summary: Configure TSS and use LTR instruction to load TSS
 */
static void taskm_id_23674_tr_expose(void)
{
	/*Backup TSS*/
	unsigned int tr0 = str();

	setup_tss32();
	asm volatile(ASM_TRY("1f")
			"mov $" xstr(TSS_INTR)", %%ax\n\t"
			"ltr %%ax\n\t"
			"1:":::);

	report("%s", (exception_vector() == NO_EXCEPTION), __func__);

	/*Restore TSS*/
	gdt32[tr0 >> 3].access &= (~0x2); /*clear busy*/
	ltr(tr0);
}


/**
 * @brief case name:Task Management expose TR_002
 *
 * Summary: Configure TSS and use LTR instruction to load TSS in Ring3
 */
static void taskm_id_23680_tr_expose(void)
{
	setup_tss32();
	do_at_ring3(test_ltr_for_exception, __func__);
}

/**
 * @brief case name:Task Management expose TR_003
 *
 * Summary: Configure TSS and use STR instruction to store
 *	TR to general purpose register with privilege
 *	level 0 and 3
 */
static void taskm_id_23687_tr_expose(void)
{
	u16 val;

	setup_tss32();

	/* pre-condition:cr4.umip = 0 */
	set_UMIP(false);

	/*Use STR to store TSS selector to general register EAX.*/
	asm volatile(ASM_TRY("1f")
			"str %%ax\n\t"
			"mov %%ax,%0\n\t":"=m"(val)::);

	/*Use STR to store TSS selector to general register EAX in Ring3*/
	if ((val == TSS_MAIN) || (val == TSS_INTR))
		do_at_ring3(str_to_general_purpose_register, __func__);
	else
		report("%s", false, __func__);
}

/**
 * @brief case name:Task Management expose TR_004
 *
 * Summary: Configure TSS and use STR instruction to store
 *	TR to memory with privilege level 0 and 3
 */
static void taskm_id_23688_tr_expose(void)
{
	u16 val = 0;

	setup_tss32();

	/* pre-condition:cr4.umip = 0 */
	set_UMIP(false);

	/*Use STR to store TSS selector to memory.*/
	asm volatile(ASM_TRY("1f")
		"str %0\n\t":"=m"(val)::);

	/*Use STR to store TSS selector to memory in Ring3*/
	if (val != 0)
		do_at_ring3(str_to_memory, __func__);


	report("%s", ((val != 0) && (tr_val != 0)), __func__);
}

/**
 * @brief case name:Task Management expose TR_005
 *
 * Summary: Set CR4.UMIP = 1, execute STR in ring0 and ring3,
 *	it will gernerate #GP with ring3.
 */
static void taskm_id_23689_tr_expose(void)
{
	u16 val = 0;

	if (!set_UMIP(true)) {
		printf("%s, UMIP is NOT supported.\n", __func__);
		report("%s", true, __func__);
		return;
	}
	/*Ring 0: Use STR to store TSS selector to memory.*/
	asm volatile("str %0\n\t":"=m"(val)::);

	/*Ring 3: Use STR to store TSS selector to memory.*/
	if (val != 0)
		do_at_ring3(test_str_mem_for_exception, __func__);
	else
		report("%s", false, __func__);

	//restore enviornment
	set_UMIP(false);
}

/**
 * @brief case name:Task switch in protect mode_001
 *
 * Summary: Configure TSS and use CALL instruction with the TSS
 *          segment selector for the new task as the operand.
 */
static void taskm_id_23761_protect(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __func__);
}

/**
 * @brief case name:Task switch in protect mode_002
 *
 * Summary: Configure TSS and use CALL instruction with
 *	the task gate selector for the new task as the operand.
 */
static void taskm_id_23762_protect(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);
	report("%s", (exception_vector() == GP_VECTOR), __func__);
}

/**
 * @brief case name:Task switch in protect mode_003
 *
 * Summary: Configure TSS and use INT instruction with the vector
 * points to a task-gate descriptor in the IDT as the operand
 */
static void taskm_id_23763_protect(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	idt_entry_t *e = &boot_idt[6];

	memset(e, 0, sizeof(*e));

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;

	memcpy((void *)(&boot_idt[6]), (void *)(&gate),	sizeof(gate));

	asm volatile(ASM_TRY("1f")
			"int $6\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __func__);
}

/**
 * @brief case name:Task switch in protect mode_004
 *
 * Summary: Configure TSS and use IRET with the EFLAGS.NT set
 *	and the previous task link field configured.
 */
static void taskm_id_24006_protect(void)
{
	u16 sel_bak;

	/*construct tss_intr*/
	setup_tss32();

	cond_10_desc_busy();

	/*set eflags.NT bit*/
	if (!eflags_nt_to_1())
		debug_print("Err: fail to set eflags.NT bit.\n");

	/*set CR0.TS bit*/
	write_cr0_ts(1);

	/*set prelink to TSS_INTR*/
	sel_bak = tss.prev;
	tss.prev = TSS_INTR;

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __func__);

	/*resume environment*/
	tss.prev = sel_bak;
	write_cr0_ts(0);
	eflags_nt_to_0();
}

/**
 * @brief case name:TR Initial value following start-up_001
 *
 * Summary: Check the TR invisible part in start up
 */
static void taskm_id_24026_tr_start_up(void)
{
	struct descriptor_table_ptr gdt_ptr;
	u16 desc_size = 0;
	u16 ss_val = 0;
	u16 ss_val_old = 0;

	tss.ss0 = (GDT_NEWTSS_INDEX << 3);

	sgdt(&gdt_ptr);
	memcpy(&(gdt32[GDT_NEWTSS_INDEX]), &(gdt32[2]), sizeof(gdt_entry_t));
	lgdt(&gdt_ptr);

	ss_val_old = read_ss();

	desc_size = sizeof(tss32_t);
	memset((void *)0, 0, desc_size);
	memcpy((void *)0, (void *)&tss, desc_size);

	do_at_ring3(test_tr_print, "TR test");

	ss_val = read_ss();


	report("%s", ((ss_val == tss.ss0)
		&& (ss_val == (GDT_NEWTSS_INDEX << 3))
		&& (ss_val_old == 0x10)), __func__);

	//resume environment
	tss.ss0 = 0x10;
	asm volatile("mov $0x10, %ax\n\t"
		"mov %ax, %ss\n\t");
}

/**
 * @brief case name: CR0.TS state following STARTUP_001
 *
 * Summary:  Get CR0.TS at BP startup, the bit shall be 0 and same
 *	     with SDM definition.
 */
static void taskm_id_23826_cr0_ts_startup(void)
{
	u32 cr0 = *((u32 *)STARTUP_CR0_ADDR);

	report("%s", (cr0 & X86_CR0_TS) == 0, __func__);
}

/**
 * @brief case name: EFLAGS.NT state following STARTUP_001
 *
 * Summary: Get EFLAGS.NT at BP start-up, the bit shall be 0
 *	    and same with SDM definition.
 */
static void taskm_id_23829_eflags_nt_start_up(void)
{
	u32 eflags = *((u32 *)STARTUP_EFLAGS_ADDR);

	report("%s", (eflags & X86_EFLAGS_NT) == 0, __func__);
}

/**
 * @brief case name: TR selector following start-up_001
 *
 * Summary: TR selector should be 0H following start-up.
 */
static void taskm_id_26023_tr_selector_start_up(void)
{
	u32 tr_sel = *((u32 *)STARTUP_TR_SEL_ADDR);

	report("%s", tr_sel == 0, __func__);
}

#endif /* defined(IN_NON_SAFETY_VM) || defined(IN_SAFETY_VM) */

#if defined(IN_NON_SAFETY_VM)
/**
 * @brief case name: CR0.TS state following INIT_001
 *
 * Summary: Get CR0.TS at AP init, the bit shall be 0
 *	     and same with SDM definition.
 */
static void taskm_id_23827_cr0_ts_init(void)
{
	volatile u32 cr0 = *((volatile u32 *)INIT_CR0_ADDR);
	bool is_pass = true;

	if ((cr0 & X86_CR0_TS) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(23827);
	cr0 = *((volatile u32 *)INIT_CR0_ADDR);
	if ((cr0 & X86_CR0_TS) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __func__);
}

/**
 * @brief case name: EFLAGS.NT state following INIT_001
 *
 * Summary: Get EFLAGS.NT at AP start-up,
 *	the bit shall be 0 and same with SDM definition.
 */
static void taskm_id_23830_eflags_nt_init(void)
{
	volatile u32 eflags = *((u32 *)INIT_EFLAGS_ADDR);
	bool is_pass = true;

	if ((eflags & X86_EFLAGS_NT) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(23830);
	eflags = *((u32 *)INIT_EFLAGS_ADDR);
	if ((eflags & X86_EFLAGS_NT) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __func__);
}

/**
 * @brief case name: TR selector following INIT_001
 *
 * Summary: TR selector should be 0H following INIT.
 */
static void taskm_id_26024_tr_selector_init(void)
{
	volatile u32 tr_sel = *((u32 *)INIT_TR_SEL_ADDR);
	bool is_pass = true;

	if (tr_sel != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(26024);
	tr_sel = *((u32 *)INIT_TR_SEL_ADDR);
	if (tr_sel != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __func__);
}

/**
 * @brief case name:TR base following INIT_001
 *
 * Summary: Check the TR invisible part in INIT
 */
static void taskm_id_24027_tr_init(void)
{
	struct descriptor_table_ptr gdt_ptr;

	sgdt(&gdt_ptr);

	memcpy(&(gdt32[GDT_NEWTSS_INDEX]), &(gdt32[2]), sizeof(gdt_entry_t));

	lgdt(&gdt_ptr);

	smp_init();

	on_cpu(1, (void *)test_tr_init, NULL);
}

#endif /* #if defined(IN_NON_SAFETY_VM) */

#if defined(IN_NATIVE)

/**
 * @brief case name:
 *   Limit check for 16-bit TSS on the physical platform
 *
 * Summary:G bit of the TSS descriptor is 0 and The TSS descriptor
 *	points to 16-bit TSS (i.e. [bit 44:40] is 0001B) and the
 *	limit of the TSS descriptor is less than 2BH,
 *	it would gernerate #TS(Error code = TSS selector & F8H)
 *	when executing a CALL or JMP instruction to a task gate.
 */

static void taskm_id_38864_limit_check_tss_desc_limit_2BH(void)
{
	/*Construct tss_intr*/
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x2A;
	gdt32[TSS_INTR>>3].access = 0x81; /*p=1, type:0001b*/

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 1;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == TS_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

/**
 * @brief case name:
 *	Order of checks on TSS descriptor during a task switch_001
 *
 * Summary: When a vCPU attempts to do task switch, the physical platform
 *	shall guarantee that checks on the new TSS descriptor include the
 *	following in order.
 *	1) Check S bit and gate type of the TSS descriptor
	   (depends on type of task switch).
 *	2) Check P bit of the TSS descriptor.
 *	3) Check G bit and limit of the TSS descriptor.
 *
 *	This test case set S bit and clear P bit of the TSS descriptor,
 *	also set limit of TSS descriptor to 0x66 (G = 0),
 *	it would generate #NP (Error code = TSS_INTR & 0xF8)
 *	when executing a CALL to TSS_INTR
 *
 *	Note:  Check S bit before P bit of the TSS descriptor and G bit and
 *	limit of the TSS descriptor.
 */
static void taskm_id_37691_checks_on_TSS_descriptor_S_bit_set(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x66;
	gdt32[TSS_INTR>>3].access = 0x19; /*p=0, dpl = 0, s = 1, type:1001b*/

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
		"1:":::);

	/**
	 * Note: from native test,
	 * below exception is 11(NP) and 0x20, instead of 13(GP) and 0
	 */
	report("%s", ((exception_vector() == NP_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))), __func__);
}

/**
 * @brief case name: Order of checks on TSS descriptor during a task switch_002
 *
 * Summary: When a vCPU attempts to do task switch, the physical platform
 *	shall guarantee that checks on the new TSS descriptor include the
 *	following in order.
 *	1) Check S bit and gate type of the TSS descriptor
 *	  (depends on type of task switch).
 *	2) Check P bit of the TSS descriptor.
 *	3) Check G bit and limit of the TSS descriptor.
 *
 *	This test case clear P bit of the TSS descriptor,
 *	also set limit of TSS descriptor to 0x66 (G = 0),
 *	it would generate #NP (Error code == TSS selector & F8H)
 *	when executing a CALL to TSS_INTR.
 *
 *	Note: Check P bit of the TSS descriptor before G bit
 *	      and limit of the TSS descriptor.
 */
static void taskm_id_37692_checks_on_TSS_descriptor_P_bit_clear(void)
{

	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x66;
	gdt32[TSS_INTR>>3].access = 0x09;  /*p=0, dpl = 0, s = 0, type:1001b */

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TSS_INTR) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR)
		&& (exception_error_code() == (TSS_INTR & 0xF8))),
		__func__);
}

static int ret_val_tc37686;
static int error_code_tc37686;
static void call_ring3_tc37686(const char *data)
{
	asm volatile(ASM_TRY("1f")
			"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
			"1:":::);

	ret_val_tc37686 = exception_vector();
	error_code_tc37686 = exception_error_code();
}

/**
 * @brief case name:
 *  Order of checks on task gate P bit check during a task switch_001
 *
 * Summary: When a vCPU attempts to do task switch and a task gate is used
 *	    to specify the TSS selector, the physical platform shall check
 *	    the present bit of the task gate after privilege checks and
 *	    before checking the new TSS selector.
 *
 *	With below conditions, the physical platform shall check the privilege
 *	first and generate #GP(Error code = gate selector & F8H)
 *	when executing a CALL to a task gate.
 *
 *	1)CPL > task gate DPL(set CPL to 3 and DPL of task gate to 0)
 *	2) present bit of the task gate is 0;
 *	3)Set G bit to 0 and limit to 0x66;
 */
static void taskm_id_37686_task_switch_privilege_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gdt32[TSS_INTR>>3].limit_low = 0x66;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 0;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
	(void *)(&gate), sizeof(gate));

	do_at_ring3(call_ring3_tc37686, __func__);

	report("%s", ((ret_val_tc37686 == GP_VECTOR) &&
		(error_code_tc37686 == (TASK_GATE_SEL & 0xF8))), __func__);
}


/**
 * @brief case name:
 *  Order of checks on task gate P bit check during a task switch_002
 *
 * Summary: When a vCPU attempts to do task switch and a task gate is used to
 *	    specify the TSS selector, the physical platform shall check the
 *	    present bit of the task gate after privilege checks and before
 *	    checking the new TSS selector.
 *
 *	Given below conditions, the physical platform shall check the present
 *	bit and generate #NP(Error code = gate selector & F8H)
 *	when executing a CALL  to a task gate.
 *
 *	1) present bit of the task gate is 0;
 *	2) Set G bit to 0 and limit to 0x66;
 */
static void taskm_id_37689_task_switch_present_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;
	gdt32[TSS_INTR>>3].limit_low = 0x66;

	gate.selector = TSS_INTR;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 0;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == NP_VECTOR)
		&& (exception_error_code() == (TASK_GATE_SEL & 0xF8))),
		__func__);
}


/**
 * @brief case name: Order of checks on TSS selector during a task switch_001
 *
 * Summary: When a vCPU attempts to do task switch and a task gate is used to
 *	    specify the TSS selector, the physical platform shall check the
 *	    present bit of the task gate after privilege checks and
 *	    before checking the new TSS selector.
 *
 *	TSS selector in the task gate has TI flag set and selector(index) is 0,
 *	it would generate #GP(Error code = TSS selector )
 *	when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_37694_task_switch_null_selector(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	gate.selector = 4; /*index = 0, TI = 1*/
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR) &&
		(exception_error_code() == gate.selector)), __func__);
}

/**
 * @brief case name: Order of checks on TSS selector during a task switch_002
 *
 * Summary: When a vCPU attempts to do task switch and a task gate is used to
 *	    specify the TSS selector, the physical platform shall check the
 *	    present bit of the task gate after privilege checks and
 *	    before checking the new TSS selector.
 *
 *	TSS selector in the task gate has TI flag set and selector is
 *	gdt.limit +1, it would generate #GP(Error code = TSS selector )
 *	when executing a CALL or JMP instruction to a task gate.
 */
static void taskm_id_37695_task_switch_TI_check(void)
{
	setup_tss32();
	tss_intr.eip = (u32)irq_tss;

	sgdt(&old_gdt_desc);

	gate.selector = old_gdt_desc.limit + 5;

	gate.type = 5;
	gate.system = 0;
	gate.dpl = 0;
	gate.p = 1;
	memcpy((void *)(&gdt32[TASK_GATE_SEL>>3]),
		(void *)(&gate), sizeof(gate));

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", ((exception_vector() == GP_VECTOR) &&
		(exception_error_code() == gate.selector)), __func__);
}

/* cond 14 */
unsigned char *cond_14_tss_not_page(void)
{
	unsigned char *linear_addr;

	linear_addr = (unsigned char *)malloc(PAGE_SIZE);
	memcpy((void *)linear_addr, (void *)(&tss_intr), sizeof(tss_intr));
	set_gdt_entry(TSS_INTR, (u32)linear_addr, sizeof(tss_intr) - 1, 0x89, 0x0f);

	set_page_control_bit((void *)((ulong)(linear_addr)),
		PAGE_PTE, 0, 0, true);

	return linear_addr;
}

/**
 * @brief case name: Exception checking skip target_001
 *
 * Summary: When a vCPU attempts to do task switch and task switch sanity checks pass,
 * the physical platform shall generate a VM exit due to task switch without checking
 * validity of the linear address of the target TSS.
 */
static void taskm_id_38866_task_switch_page_not_present(void)
{
	unsigned char *tmp_tss;

	tmp_tss = cond_14_tss_not_page();

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(TASK_GATE_SEL) ", $0xf4f4f4f4\n\t"
		"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __func__);

	/* resume environment */
	setup_tss32();
	set_page_control_bit((void *)((ulong)(tmp_tss)), PAGE_PTE, 0, 1, true);
	free((void *)tmp_tss);
}
#endif /*IN_NATIVE*/

typedef void (*tskmgr_handler_t)(void);

const struct {
	bool enable;
	u32 id;
	tskmgr_handler_t handler;
	const char *desc;
} tc_tbl32[] = {

#if defined(IN_NATIVE)
	/**
	 * Task Management Application constraint:
	 * Valid limit check for 16-bit TSS on the physical platform
	 */
	{
		true, 38864, taskm_id_38864_limit_check_tss_desc_limit_2BH,
		"Valid limit check for 16-bit TSS on the physical platform"
	},

	/**
	 * Task Management Application constraint:
	 * Order of checks on TSS descriptor during a task switch
	 */
	{	/*done, test fail.*/
		true, 37691, taskm_id_37691_checks_on_TSS_descriptor_S_bit_set,
		"Order of checks on TSS descriptor during a task switch_001"
	},
	{
		true, 37692,
		taskm_id_37692_checks_on_TSS_descriptor_P_bit_clear,
		"Order of checks on TSS descriptor during a task switch_002"
	},

	/**
	 * Task Management Application constraint:
	 * Order of checks on task gate P bit check during a task switch
	 */
	{
		true, 37686, taskm_id_37686_task_switch_privilege_check,
		"Order of checks on task gate P bit during task switch_001"
	},
	{
		true, 37689, taskm_id_37689_task_switch_present_check,
		"Order of checks on task gate P bit during task switch_002"
	},

	/**
	 * Task Management Application constraint:
	 * Order of checks on TSS selector during a task switch
	 */
	{
		true, 37694, taskm_id_37694_task_switch_null_selector,
		"Order of checks on TSS selector during task switch_001"
	},
	{
		true, 37695, taskm_id_37695_task_switch_TI_check,
		"Order of checks on TSS selector during task switch_001"
	},

	/**
	 * Task Management Application constraint:
	 * Exception checking skip target TSS
	 */
	{
		true, 38866, taskm_id_38866_task_switch_page_not_present,
		"Exception checking skip target TSS_001"
	}
#endif

#ifdef IN_NON_SAFETY_VM
	{
		true, 23827, taskm_id_23827_cr0_ts_init,
		"CR0.TS state following INIT_001"
	},
	{
		true, 23830, taskm_id_23830_eflags_nt_init,
		"EFLAGS.NT state following INIT_001"
	},
	{
		true, 26024, taskm_id_26024_tr_selector_init,
		"TR selector following INIT_001"
	},
	{
		true, 24027, taskm_id_24027_tr_init,
		"TR Initial value following INIT_001"
	},
#endif

#if defined(IN_NON_SAFETY_VM) || defined(IN_SAFETY_VM)
	{
		true, 23826, taskm_id_23826_cr0_ts_startup,
		"CR0.TS state following STARTUP_001"
	},
	{
		true, 23829, taskm_id_23829_eflags_nt_start_up,
		"EFLAGS.NT state following STARTUP_001"
	},
	{
		true, 26023, taskm_id_26023_tr_selector_start_up,
		"TR selector following STARTUP_001"
	},
	{
		true, 24026, taskm_id_24026_tr_start_up,
		"TR Initial value following start-up_001"
	},

	/* Expose Registers: Task Management Expose TR*/
	{
		true, 23674, taskm_id_23674_tr_expose,
		"Task Management expose TR_001"
	},
	{
		true, 23680, taskm_id_23680_tr_expose,
		"Task Management expose TR_002"
	},
	{
		true, 23687, taskm_id_23687_tr_expose,
		"Task Management expose TR_003"
	},
	{
		true, 23688, taskm_id_23688_tr_expose,
		"Task Management expose TR_004"
	},
	{
		true, 23689, taskm_id_23689_tr_expose,
		"Task Management expose TR_005"
	},

	/* Expose Registers: Task Management Expose CR0.TS*/
	{
		true, 23821, taskm_id_23821_cr0_ts_expose,
		"Task Management expose CR0.TS_001"
	},
	{
		true, 23890, taskm_id_23890_cr0_ts_expose,
		"Task Management expose CR0.TS_002"
	},
	{
		true, 23892, taskm_id_23892_cr0_ts_expose,
		"Task Management expose CR0.TS_003"
	},
	{
		true, 23893, taskm_id_23893_cr0_ts_expose,
		"Task Management expose CR0.TS_004"
	},

	/* Expose Registers:
	 * guest CR0.TS keeps unchanged in task switch attempt
	 */
	{
		true, 23803, taskm_id_23803_cr0_ts_unchanged_call,
		"guest CR0.TS keeps unchanged in task switch attempts_001"
	},
	{
		true, 23835, taskm_id_23835_cr0_ts_unchanged_call,
		"guest CR0.TS keeps unchanged in task switch attempts_002"
	},
	{
		true, 23836, taskm_id_23836_cr0_ts_unchanged_call,
		"guest CR0.TS keeps unchanged in task switch attempts_003"
	},
	{
		true, 24008, taskm_id_24008_cr0_ts_unchanged_call,
		"guest CR0.TS keeps unchanged in task switch attempts_004"
	},

	/* Expose Task Switch Support: Task Switch in protect mode */
	{

		true, 23761, taskm_id_23761_protect,
		"Task switch in protect mode_001"
	},
	{
		true, 23762, taskm_id_23762_protect,
		"Task switch in protect mode_002"
	},
	{
		true, 23763, taskm_id_23763_protect,
		"Task switch in protect mode_003"
	},
	{
		true, 24006, taskm_id_24006_protect,
		"Task switch in protect mode_004"
	},

	/**
	 * Expose Exception & Violation:
	 * Switching to a task gate in IDT by NMI,
	 * exception other than #BP and #OF or an external interrupt
	 */
	{
		true, 25205, taskm_id_25205_nmi_gate_p_bit,
		"Exception_Checking_on_IDT_by_NMI_001"
	},
	{
		true, 25206, taskm_id_25206_nmi_null_selector,
		"Exception_Checking_on_IDT_by_NMI_002"
	},
	{
		true, 25207, taskm_id_25207_nmi_TI_flag_set,
		"Exception_Checking_on_IDT_by_NMI_003"
	},
	{
		true, 25208, taskm_id_25208_nmi_gdt_limit_overflow,
		"Exception_Checking_on_IDT_by_NMI_004"
	},
	{
		true, 26173, taskm_id_26173_nmi_tss_desc_page_fault,
		"Exception_Checking_on_IDT_by_NMI_005"
	},
	{
		true, 25209, taskm_id_25209_nmi_tss_desc_busy,
		"Exception_Checking_on_IDT_by_NMI_006"
	},
	{
		true, 25210, taskm_id_25210_nmi_tss_desc_not_present,
		"Exception_Checking_on_IDT_by_NMI_007"
	},
	{
		true, 25211, taskm_id_25211_nmi_tss_desc_limit_67H,
		"Exception_Checking_on_IDT_by_NMI_008"
	},
	{
		true, 26093, taskm_id_26093_nmi_tss_desc_limit_2BH,
		"Exception_Checking_on_IDT_by_NMI_009"
	},

	/**
	 * Expose Exception & Violation:
	 * Switching to a task gate in IDT by a software interrupt
	 * (i.e. INT n, INT3 or INTO)
	 */
	{
		true, 24564, taskm_id_24564_int_CPL_over_DPL,
		"Exception_Checking_on_IDT_by_Software_interrupt_001"
	},
	{
		true, 25087, taskm_id_25087_int_gate_present_clear,
		"Exception_Checking_on_IDT_by_Software_interrupt_002"
	},
	{
		true, 25089, taskm_id_25089_int_gate_selector_null, /*P1*/
		"Exception_Checking_on_IDT_by_Software_interrupt_003"
	},
	{
		true, 25092, taskm_id_25092_int_gate_TI_flag_set,
		"Exception_Checking_on_IDT_by_Software_interrupt_004"
	},
	{
		true, 25093, taskm_id_25093_int_gate_selector_index_overflow,
		"Exception_Checking_on_IDT_by_Software_interrupt_005"
	},
	{
		true, 26172, taskm_id_26172_int_pagefault,
		"Exception_Checking_on_IDT_by_Software_interrupt_006"
	},
	{
		true, 25195, taskm_id_25195_int_tss_desc_busy,
		"Exception_Checking_on_IDT_by_Software_interrupt_007"
	},
	{
		true, 25196, taskm_id_25196_int_tss_desc_present_clear,
		"Exception_Checking_on_IDT_by_Software_interrupt_008"
	},
	{
		true, 25197, taskm_id_25197_int_tss_desc_limit_67H,
		"Exception_Checking_on_IDT_by_Software_interrupt_009"
	},
	{
		true, 26091, taskm_id_26091_int_tss_desc_limit_2BH,
		"Exception_Checking_on_IDT_by_Software_interrupt_010"
	},

	/**
	 * Expose Exception & Violation:
	 * Executing IRET with EFLAGS.NT = 1
	 */
	{
		true, 25212, taskm_id_25212_iret_prelink_null_sel,
		"Exception_Checking_on_IRET_001"
	},
	{
		true, 25219, taskm_id_25219_iret_prelink_TI_flag_set,
		"Exception_Checking_on_IRET_002"
	},
	{
		true, 25221, taskm_id_25221_iret_prelink_gdt_overflow,
		"Exception_Checking_on_IRET_003"
	},
	{
		true, 26174, taskm_id_26174_iret_prelink_pagefault,
		"Exception_Checking_on_IRET_004"
	},
	{
		true, 25222, taskm_id_25222_iret_type_not_busy,
		"Exception_Checking_on_IRET_005"
	},
	{
		true, 25223, taskm_id_25223_iret_tss_desc_not_present,
		"Exception_Checking_on_IRET_006"
	},
	{
		true, 25224, taskm_id_25224_iret_tss_desc_limit_67H,
		"Exception_Checking_on_IRET_007"
	},
	{
		true, 26094, taskm_id_26094_iret_tss_desc_limit_2BH,
		"Exception_Checking_on_IRET_008"
	},

	/**
	 * Expose Exception & Violation:
	 * CALL or JMP instruction with a TSS selector to a TSS descriptor
	 */
	{
		true, 24544, taskm_id_24544_tss_CPL_over_DPL,
		"Exception_Checking_on_TSS_selector_001"
	},
	{
		true, 24546, taskm_id_24546_tss_RPL_over_DPL,
		"Exception_Checking_on_TSS_selector_002"
	},
	{
		true, 24545, taskm_id_24545_tss_TI_flag_set,
		"Exception_Checking_on_TSS_selector_003"
	},
	{
		true, 25030, taskm_id_25030_tss_desc_busy,
		"Exception_Checking_on_TSS_selector_004"
	},
	{
		true, 25032, taskm_id_25032_tss_desc_present_bit_clear,
		"Exception_Checking_on_TSS_selector_005"
	},
	{
		true, 25034, taskm_id_25034_tss_limit_67H,
		"Exception_Checking_on_TSS_selector_006"
	},
	{
		true, 26088, taskm_id_26088_tss_limit_2BH,
		"Exception_Checking_on_TSS_selector_007"
	},

	/**
	 * Expose Exception & Violation:
	 * CALL or JMP instruction to a task gate
	 */
	{
		true, 24561, taskm_id_24561_task_gate_CPL_check,
		"Exception_Checking_on_Task_gate_001"
	},
	{
		true, 24513, taskm_id_24613_task_gate_RPL_check,
		"Exception_Checking_on_Task_gate_002"
	},
	{
		true, 24562, taskm_id_24562_task_gate_present_bit_check,
		"Exception_Checking_on_Task_gate_003"
	},
	{
		true, 24566, taskm_id_24566_task_gate_selector_null_check,
		"Exception_Checking_on_Task_gate_004"
	},
	{
		true, 25023, taskm_id_25023_task_gate_selector_TI_check,
		"Exception_Checking_on_Task_gate_005"
	},
	{
		true, 25025, taskm_id_25025_task_gate_out_of_gdt_limit_check,
		"Exception_Checking_on_Task_gate_006"
	},
	{
		true, 26171, taskm_id_26171_task_gate_page_fault,
		"Exception_Checking_on_Task_gate_007"
	},
	{
		true, 25026, taskm_id_25026_task_gate_tss_busy_check,
		"Exception_Checking_on_Task_gate_008"
	},
	{
		true, 25024, taskm_id_25024_task_gate_tss_present_check,
		"Exception_Checking_on_Task_gate_009"
	},
	{
		true, 25028, taskm_id_25028_task_gate_tss_desc_limit_67H_check,
		"Exception_Checking_on_Task_gate_010"
	},
	{
		true, 26061, taskm_id_26061_task_gate_tss_desc_limit_2BH,
		"Exception_Checking_on_Task_gate_011"
	},
#endif
};

static void print_case_list_32(void)
{
	u32 i;

	printf("Task Management feature protected mode case list:\n\r");
	for (i = 0; i < ARRAY_SIZE(tc_tbl32); i++) {
		if (tc_tbl32[i].enable)
			printf("\t Case ID: %d case name:%s\n\r",
				tc_tbl32[i].id, tc_tbl32[i].desc);
	}
}

static void test_task_management_32(void)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(tc_tbl32); i++) {
		if (tc_tbl32[i].enable)
			tc_tbl32[i].handler();
	}
}
