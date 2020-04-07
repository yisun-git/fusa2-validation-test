static volatile int test_count;
tss64_t tss64_intr;
static char intr_alt_stack[4096];

extern struct descriptor_table_ptr gdt64_desc;

struct segment_desc64 gdt64[20];

static void set_gdt64_entry(int sel, u64 base,  u32 limit, u8 access, u8 gran)
{
	int num = sel >> 3;

	/* Setup the descriptor base address */
	gdt64[num].base1 = (base & 0xFFFF);
	gdt64[num].base2 = (base >> 16) & 0xFF;
	gdt64[num].base3 = (base >> 24) & 0xFF;
	gdt64[num].base4 = (base >> 32) & 0xFFFFFFFF;

	/* Setup the descriptor limits */
	gdt64[num].limit1 = (limit & 0xFFFF);
	gdt64[num].limit = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	gdt64[num].g |= (gran & 0x80);
	gdt64[num].avl |= (gran & 0x10);
	gdt64[num].p |= (access & 0x80);
	gdt64[num].dpl |= (access & 0x60);
	gdt64[num].type |= (access & 0x0F);

	gdt64[num].zero = 0;
}

static void setup_tss64(void)
{
	u16 desc_size = sizeof(tss64_t);

	tss64_intr.ist1 = (u64)intr_alt_stack + 4096;
	tss64_intr.iomap_base = (u16)desc_size;

	set_gdt64_entry(0x58, (u64)&tss64_intr, desc_size - 1, 0x89, 0x0f);
}
static void set_gdt64_task_gate(int vec, u16 sel)
{

	idt_entry_t *e = (idt_entry_t *)(gdt64_desc.base);

	e += 12;/*0x60*/
	memset(e, 0, sizeof *e);

	e->selector = sel;
	e->ist = 0;
	e->type = 5;
	e->dpl = 0;
	e->p = 1;
}

static void set_idt_task_gate(int vec, u16 sel)
{
	idt_entry_t *e = &boot_idt[vec];

	memset(e, 0, sizeof *e);

	e->selector = sel;
	e->ist = 0;
	e->type = 5;
	e->dpl = 0;
	e->p = 1;
}

/**
 * @brief case name:Task_Switch_in_IA-32e_mode_001
 *
 * Summary: Configure TSS and CALL instruction with the TSS segment selector
 *          for the new task as the operand.
 */
void task_management_rqmid_26076_ia_32e(void)
{

	int target_sel = 0x58 << 16;

	//printf("%s\n", __FUNCTION__);

	asm volatile(ASM_TRY("1f")
			"lcallw *%0\n\t"
			"1:"::"m"(target_sel) : );

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);

}

/**
 * @brief case name:Task_Switch_in_IA-32e_mode_002
 *
 * Summary: Configure TSS and use CALL instruction with the task gate
 *          selector for the new task as the operand.
 */
void task_management_rqmid_26073_ia_32e(void)
{

	int target_sel = 0x60 << 16;

	set_gdt64_task_gate(0x60, 0x58);

	gate.selector = 0x58;
	gate.type = 5;
	gate.system = 0;
	gate.dpl = 3;
	gate.p = 1;

	memcpy((void *)(&gdt64[0x60>>3]), (void *)(&gate), sizeof(struct task_gate));

	asm volatile(ASM_TRY("1f")
			"lcallw *%0\n\t"
			"1:"::"m"(target_sel) : );

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}
/**
 * @brief case name:Task_Switch_in_IA-32e_mode_003
 *
 * Summary: Configure TSS and use INT instruction with the vector points to a
 *          task-gate descriptor in the IDT as the operand.
 */
void task_management_rqmid_24011_ia_32e(void)
{
	set_idt_task_gate(35, USER_CS64);

	asm volatile(ASM_TRY("1f")
			"int $35\n\t"
			"1:":::);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}
/**
 * @brief case name:Task_Switch_in_IA-32e_mode_004
 *
 * Summary: Configure TSS and use IRET with the EFLAGS.NT set and the previous
 *          task link field configured.
 */
void task_management_rqmid_26256_ia_32e(void)
{
	int target_sel = 0x58 << 16;

	u64 cr0_val = read_cr0();

	write_cr0(cr0_val | X86_CR0_TS);

	u64 rflags = read_rflags();

	write_rflags(rflags | X86_EFLAGS_NT);

	asm volatile(ASM_TRY("1f")
			"iret\n\t"
			"1:"::"m"(target_sel) : );

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

__attribute__((unused)) static void test_task_management_64_normal(void)
{
	task_management_rqmid_26073_ia_32e();
	task_management_rqmid_24011_ia_32e();
	task_management_rqmid_26256_ia_32e();
}
static void test_task_management_64(void)
{
	task_management_rqmid_26076_ia_32e();
}

