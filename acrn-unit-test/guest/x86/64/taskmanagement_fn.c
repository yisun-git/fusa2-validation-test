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

static void print_case_list_64(void)
{
	printf("Task Management 64bit mode feature case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 26076,
		"Task_Switch_in_IA-32e_mode_001");
}

static void test_task_management_64(void)
{
	task_management_rqmid_26076_ia_32e();
}

