
u8 random()
{
	u8 value;
	long long time;
	time = rdtsc();

	value = (time & 0xff);
	return value;
}

/**
 * @brief case name:Execute Disable support_001
 *
 * Summary:When IA32_EFER.NXE is 0, execute RET instruction shall be no page fault
 */
static void paging_rqmid_25249_execute_disable_support()
{
	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	const char *temp = "\xC3";
	void *p = malloc(4);
	if (p == NULL) {
		printf("malloc error!\n");
		return;
	}

	wrmsr(X86_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);
	memcpy(p, temp, sizeof(temp));
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	report("paging_rqmid_25249_execute_disable_support", test_instruction_fetch(p) == PASS);

	free_gva((void *)p);
}

/**
 * @brief case name:32-Bit Paging Support_001
 *
 * Summary: In 32-bit paging mode with 4K page, the mapping between GVA and GPA shall be
 * 	    correct by checking if the value reading directly from GVA and GPA is same.
 */
static void paging_rqmid_24415_32_bit_paging_support()
{
	u8 random_value[PAGE_SIZE] = {0};
	u8 *gva = malloc(PAGE_SIZE);
	ulong cr0 = read_cr0();
	u8 *gva_old = gva;
	u32 i;

	if (gva == NULL) {
		printf("malloc error!\n");
		return;
	}

	/* get this page start address offset */
	u32 addr_start = (u32)gva & MAX_OFFSET_OF_4K_PAGING;

	/* get gpa through cr3 */
	u32 gpa_add = (u32)virt_to_pte_phys((pgd_t *)read_cr3(), (void *)gva);
	u8 *gpa = (u8 *)gpa_add;

	/* setup no paging mode, then set random value to 4k pages pointed by the gpa */
	write_cr0(cr0 & ~X86_CR0_PG);
	for (i = addr_start; i <= MAX_OFFSET_OF_4K_PAGING; i++) {
		random_value[i] = random();
		*gpa++ = random_value[i];
	}

	/* setup 32-bit paging mode then compare the value in gva with gpa in whole 4k pages */
	write_cr0(cr0 | X86_CR0_PG);
	for (i = addr_start; i <= MAX_OFFSET_OF_4K_PAGING; i++) {
		if (*gva++ != random_value[i]) {
			break;
		}
	}

	i--;
	report("paging_rqmid_24415_32_bit_paging_support", (i == MAX_OFFSET_OF_4K_PAGING));

	free_gva((void *)gva_old);
}

void test_paging_32bit_mode()
{
	paging_rqmid_24415_32_bit_paging_support();
	paging_rqmid_25249_execute_disable_support();
}

