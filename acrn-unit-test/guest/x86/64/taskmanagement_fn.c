static volatile int test_count;
tss64_t tss64_intr;
static char intr_alt_stack[4096];

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

