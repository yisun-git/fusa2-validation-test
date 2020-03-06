u8 *g_gva = NULL;
/* test at ring3, return the result to this global value */
bool g_ring3_is_pass = false;

static void map_first_16m_supervisor_pages()
{
	/* Map first 16MB as supervisor pages */
	unsigned long i;
	for (i = 0; i < USER_BASE; i += PAGE_SIZE) {
		*get_pte(phys_to_virt(read_cr3()), phys_to_virt(i)) &= ~PT_USER_MASK;
		invlpg((void *)i);
	}

}

static void resume_environment()
{
	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	unsigned long i;

	wrmsr(PAGING_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);
	write_cr4(read_cr4() & ~X86_CR4_SMAP);
	write_cr4(read_cr4() & ~X86_CR4_SMEP);
	write_cr4(read_cr4() | X86_CR4_PSE);
	write_cr4(read_cr4() & ~X86_CR4_PGE);
	write_cr0(read_cr0() | X86_CR0_WP);
	write_cr0(read_cr0() & ~X86_CR0_CD);
	clac();
	/* Map first 16MB as user pages */
	for (i = 0; i < USER_BASE; i += PAGE_SIZE) {
		*get_pte(phys_to_virt(read_cr3()), phys_to_virt(i)) |= PT_USER_MASK;
		invlpg((void *)i);
	}
}

/*P must be a non-null pointer */
static int read_memory_checking(void *p)
{
	u64 value = 1;
	asm volatile(ASM_TRY("1f")
		     "mov (%[p]), %[value]\n\t"
		     "1:"
		     : : [value]"r"(value), [p]"r"(p));
	return exception_vector();
}

static void get_vector_error_code_by_read_memory(void *gva, u8 *vector, u16 *error_code)
{
	u64 value = 1;
	asm volatile(ASM_TRY("1f")
		     "mov (%[gva]), %[value]\n\t"
		     "1:"
		     : : [value]"r"(value), [gva]"r"(gva));
	*vector = exception_vector();
	*error_code = exception_error_code();
}

static int write_memory_checking(void *p)
{
	assert(p != NULL);
	u8 value = WRITE_INITIAL_VALUE;
	asm volatile(ASM_TRY("1f")
		     "mov %[value], (%[p])\n\t"
		     "1:"
		     : : [value]"r"(value), [p]"r"(p));
	return exception_vector();
}

static bool write_memory_error_code_checking(void *p, u32 bit)
{
	bool flag = false;
	assert(p != NULL);
	u8 value = WRITE_INITIAL_VALUE;
	asm volatile(ASM_TRY("1f")
		     "mov %[value], (%[p])\n\t"
		     "1:"
		     : : [value]"r"(value), [p]"r"(p));
	if ((exception_vector() == PF_VECTOR) && ((exception_error_code() & bit) != 0)) {
		flag = true;
	}

	return flag;
}

/**
 * @brief case name:Write Protect Support_001
 *
 * Summary: when write CR0.WP is 0, writing value to supervisor-mode address shall
 * be successful for supervisor access.The case is pass when write data success
 */
static void paging_rqmid_23918_write_protect_support()
{
	u8 *p = malloc(sizeof(u8));
	if (p == NULL) {
		printf("malloc error!\n");
		return;
	}

	write_cr0(read_cr0() & ~X86_CR0_WP);
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	*p = 2;
	report("paging_rqmid_23918_write_protect_support", (*p == 2));

	free_gva((void *)p);
}

/**
 * @brief case name:TLB Support_001
 *
 * Summary:Config 4-level paging structure, then write value to gva1, after clear P bit in PTE related with gva1.
 *         we still can access gva1 normally for paging frame information is cached in TLB.
 *         The case is pass when write value to 1B memory pointed by GVA
 */
static void paging_rqmid_24522_tlb_support()
{
	u8 *p = malloc(sizeof(u8));
	if (p == NULL) {
		printf("malloc error!\n");
		return;
	}
	*p = 1;

	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 0, false);

	*p = 2;
	report("paging_rqmid_24522_tlb_support", (*p == 2));

	free_gva((void *)p);
}

/**
 * @brief case name:Supervisor Mode Execution Prevention Support_001
 *
 * Summary: When CR4.SMEP,IA32_EFER.NXE are 0 and use user-mode pages,execute RET instruction
 *                shall have no exception.
 */
static void paging_rqmid_26017_smep_support()
{
	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	const char *temp = "\xC3";
	void *p = malloc(sizeof(u32));
	if (p == NULL) {
		printf("malloc error!\n");
		return;
	}

	wrmsr(X86_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);
	memcpy(p, temp, sizeof(temp));

	report("paging_rqmid_26017_smep_support", test_instruction_fetch(p) == PASS);

	free_gva((void *)p);
}

/**
 * @brief case name:Protection Keys Hide_001
 *
 * Summary: changing CR4.PKE from 0 to 1 shall generate #GP.
 *		  If test results are same with expected result, the test case pass, otherwise, the test case fail.
 */
static void paging_rqmid_23917_protection_keys_hide()
{
	unsigned long cr4 = read_cr4();
	bool is_pass = false;

	if ((cpuid(7).c & (1 << 3)) == 0) {
		if (write_cr4_exception_checking(cr4 | X86_CR4_PKE) == GP_VECTOR) {
			is_pass = true;
		}
	}

	report("paging_rqmid_23917_protection_keys_hide", is_pass);
}

/**
 * @brief case name:Invalidate TLB When vCPU writes CR3_disable global paging_001
 *
 * Summary: Writing CR3 will invalidate all TLB entries while disabling
 *		  global page and process-context identifiers. Read the 1B memory
 *		  pointed by GVA shall success.
 *		  If test results are same with expected result, the test case pass, otherwise, the test case fail.
 */
static void paging_rqmid_24519_disable_global_paging()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	u8 result = 0;
	if (gva == NULL) {
		printf("malloc error!\n");
		return;
	}
	*gva = 0x12;

	write_cr4(cr4 & ~X86_CR4_PCIDE);
	write_cr4(read_cr4() & ~X86_CR4_PGE);

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == 0x12) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr3(read_cr3());
	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("paging_rqmid_24519_disable_global_paging", (result == 2));

	free_gva((void *)gva);
}

static void write_super_pages_at_ring3()
{
	report("paging_rqmid_32224_ring3_access_super_address",
		(write_memory_checking(g_gva) == PF_VECTOR));
}

/**
 * @brief case name:Write Protect Support_013
 *
 * Summary: When user-mode access environment,
 *	    wirte value to the supervisor-mode pages shall generate #PF exception
 */
static void paging_rqmid_32224_ring3_access_super_address()
{
	g_gva = malloc(sizeof(u8));
	if (g_gva == NULL) {
		printf("malloc error!\n");
		return;
	}

	set_page_control_bit((void *)g_gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	page_test_at_ring3(write_super_pages_at_ring3, "write memory at ring3");

	free_gva((void *)g_gva);
}

static void write_user_pages_at_ring3()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);
	report("paging_rqmid_32227_ring3_access_user_address",
		(write_memory_checking((void *)gva) == PASS));

	free((void *)gva);
	gva = NULL;
}

/**
 * @brief case name:Write Protect Support_012
 *
 * Summary: When user-mode access and set pages can be write,
 *	    write value to user-mode pages shall have no exception
 */
static void paging_rqmid_32227_ring3_access_user_address()
{
	page_test_at_ring3(write_user_pages_at_ring3, "write memory at ring3");
}

/**
 * @brief case name:Write Protect Support_011
 *
 * Summary: When supervisor-mode access, set CR0.WP,CR4.SMAP to 1, clear EFLAGS.AC to 0,
 *	    write value to the user-mode pages shall generate #PF exception
 */
static void paging_rqmid_32228_write_protect_support_011()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	clac();

	report("%s", (write_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	resume_environment();
}

/**
 * @brief case name:Write Protect Support_010
 *
 * Summary: When supervisor-mode access, set CR0.WP,CR4.SMAP,EFLAGS.AC to be 1 and set only read pages,
 *	    write value to  user-mode pages shall generate #PF
 */
static void paging_rqmid_32229_write_protect_support_010()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	stac();

	/* set R/w flag to 0, then this pages only read */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	report("%s", (write_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Write Protect Support_009
 *
 * Summary: When supervisor-mode access, set CR0.WP,CR4.SMAP,EFLAGS.AC to be 1 and
 *	    set page-structure R/W flag to be 1, write value to user-mode pages shall have no exception
 */
static void paging_rqmid_32230_write_protect_support_009()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	stac();

	report("%s", (write_memory_checking((void *)gva) == PASS) && (*gva == WRITE_INITIAL_VALUE),
		__FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Write Protect Support_008
 *
 * Summary: When supervisor-mode access, set CR0.WP to be 1 and clear CR4.SMAP to be 0 set read only pages,
 *	    write value to the user-mode pages shall generate #PF exception
 */
static void paging_rqmid_32231_write_protect_support_008()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	write_cr4(read_cr4() & ~X86_CR4_SMAP);

	/* set R/w flag to 0, then this pages only read */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	report("%s", (write_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	free_gva((void *)gva);
}

/**
 * @brief case name:Write Protect Support_007
 *
 * Summary: When supervisor-mode access, set CR0.WP to be 1 and clear CR4.SMAP,set writable pages,
 *	    write value to the user-mode pages shall have no exception
 */
static void paging_rqmid_32232_write_protect_support_007()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	write_cr4(read_cr4() & ~X86_CR4_SMAP);

	report("%s", (write_memory_checking((void *)gva) == PASS) && (*gva == WRITE_INITIAL_VALUE),
		__FUNCTION__);

	/* resume environment */
	free_gva((void *)gva);
}

/**
 * @brief case name:Write Protect Support_006
 *
 * Summary: When supervisor-mode access, clear CR0.WP, EFLAGS.AC and set CR4.SMAP to be 1 ,
 *	    write value to the user-mode pages shall generate #PF exception
 */
static void paging_rqmid_32233_write_protect_support_006()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	write_cr0(read_cr0() & ~X86_CR0_WP);

	/* for set CR4.SMAP, set first 16M to supervisor-mode pages */
	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	clac();

	report("%s", (write_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Write Protect Support_005
 *
 * Summary: When supervisor-mode access, clear CR0.WP and set CR4.SMAP,EFLAGS.AC to be 1,
 *	    write value to the user-mode pages shall have no exception
 */
static void paging_rqmid_32307_write_protect_support_005()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	write_cr0(read_cr0() & ~X86_CR0_WP);

	/* for set CR4.SMAP, set first 16M to supervisor-mode pages */
	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	stac();

	report("%s", (write_memory_checking((void *)gva) == PASS) && (*gva == WRITE_INITIAL_VALUE),
		__FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Write Protect Support_004
 *
 * Summary:  When supervisor-mode access, clear CR0.WP and CR4.SMAP,
 *	     write value to the user-mode pages shall have no exception
 */
static void paging_rqmid_32308_write_protect_support_004()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() & ~X86_CR0_WP);

	write_cr4(read_cr4() & ~X86_CR4_SMAP);

	report("%s", (write_memory_checking((void *)gva) == PASS) && (*gva == WRITE_INITIAL_VALUE),
		__FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Write Protect Support_003
 *
 * Summary: When supervisor-mode access, set CR0.WP to be 1 and set read only pages£¬
 *	    data wirte to supervisor-mode address shall generate #PF
 */
static void paging_rqmid_32309_write_protect_support_003()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	/* set R/w flag to 0, then this pages only read */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	/* set supervisor-mode pages */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	report("%s", (write_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);
	free_gva((void *)gva);
}

/**
 * @brief case name:Write Protect Support_002
 *
 * Summary:  When supervisor-mode access, set CR0.WP to be 1,
 *	     data writes to supervisor-mode address shall be success
 */
static void paging_rqmid_32310_write_protect_support_002()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	write_cr0(read_cr0() | X86_CR0_WP);

	/* set supervisor-mode pages */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	report("%s", (write_memory_checking((void *)gva) == PASS) && (*gva == WRITE_INITIAL_VALUE),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);
	free_gva((void *)gva);
}

/**
 * @brief case name: Supervisor Mode Execution Prevention Support_002
 *
 * Summary: When supervisor-mode access, clear CR4.SMEP and clear XD flag to 0 in every page-structure entry,
 *	    set IA32_EFER.NXE to 1,execute RET instruction with this user-mode pages shall have no exception
 */
static void paging_rqmid_32311_smep_test_002()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	write_cr4(read_cr4() & ~X86_CR4_SMEP);

	wrmsr(PAGING_IA32_EFER, ia32_efer | X86_IA32_EFER_NXE);

	/* clear XD flag */
	set_page_control_bit(gva, PAGE_PTE, PAGE_XD_FLAG, 0, true);

	memcpy(gva, temp, strlen(temp));

	report("%s", (test_instruction_fetch(gva) == PASS),
		__FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Supervisor Mode Access Prevention Support_003
 *
 * Summary:  When supervisor-mode access, set CR4.SMAP to be 1, clear EFLAGS.AC ,
 *	     read user-mode pages shall generate #PF
 */
static void paging_rqmid_32312_smap_003()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	/* for set CR4.SMAP, set first 16M to supervisor-mode pages */
	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	clac();

	report("%s", (read_memory_checking((void *)gva) == PF_VECTOR), __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Supervisor Mode Access Prevention Support_002
 *
 * Summary:  When supervisor-mode access, set CR4.SMAP, EFLAGS.AC to be 1,
 *	     read user-mode pages shall successful
 */
static void paging_rqmid_32326_smap_002()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	/* for set CR4.SMAP, set first 16M to supervisor-mode pages */
	map_first_16m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	stac();

	report("%s", (*gva == WRITE_INITIAL_VALUE), __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Invalidate TLB When vCPU writes CR3_enable global paging_002
 *
 * Summary: Writing CR3 won't invadidate TLB enties related with global page
 *		  hile disabling process-context identifiers. read global pages shall success.
 *		  If test results are same with expected result, the test case pass, otherwise, the test case fail.
 */
static void paging_rqmid_26827_enable_global_paging()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	u8 result = 0;
	if (gva == NULL) {
		printf("malloc error!\n");
		return;
	}
	*gva = 0x12;

	write_cr4(cr4 & ~X86_CR4_PCIDE);

	/* check supported for enable global paging */
	if ((cpuid(1).d & (1u << 13)) != 0) {
		write_cr4(cr4 | X86_CR4_PGE);
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_PTE_GLOBAL_PAGE_FLAG, 1, true);
	if (*gva == 0x12) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	write_cr3(read_cr3());
	if (*gva == 0x12) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("paging_rqmid_26827_write_cr3_global_paging", (result == 3));

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_PTE_GLOBAL_PAGE_FLAG, 0, true);
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR4.SMAP_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR4.SMAP,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_24460_cr4_smap_invalidate_tlb()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	u8 result = 0;
	if (gva == NULL) {
		printf("malloc error!\n");
		return;
	}

	map_first_16m_supervisor_pages();
	*gva = 0x12;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == 0x12) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 | X86_CR4_SMAP);
	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB when vCPU changes IA32 EFER.NXE_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing IA32 EFER.NXE,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32328_ia32_efer_nxe_tlb()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	/* change ia32_efer.nxe value */
	if (ia32_efer & X86_IA32_EFER_NXE) {
		wrmsr(PAGING_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);
	} else {
		wrmsr(PAGING_IA32_EFER, ia32_efer | X86_IA32_EFER_NXE);
	}

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR4.SMEP_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR4.SMEP,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32329_cr4_smep_tlb()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	/* for set CR4.SMEP, set first 16M to supervisor-mode pages */
	map_first_16m_supervisor_pages();
	write_cr4(cr4 | X86_CR4_SMEP);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR4.PSE_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR4.PSE,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32344_cr4_pse_tlb()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 & ~X86_CR4_PSE);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR4.PGE_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR4.PGE,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32406_cr4_pge_tlb()
{
	unsigned long cr4 = read_cr4();
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 | X86_CR4_PGE);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR0.WP_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR0.WP,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32234_cr0_wp_tlb()
{
	unsigned long cr0 = read_cr0();
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr0(cr0 & ~X86_CR0_WP);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate TLB When vCPU changes CR0.CD_001
 *
 * Summary: Config 4-level paging structure, then write value to gva1,
 *          after clear P bit in PTE related with gva1. we still can access gva1
 *          normally for paging frame information is cached in TLB. After changing CR0.CD,
 *          we will get #PF because TLB is invalidated and get PTE directly from memory.
 */
static void paging_rqmid_32236_cr0_cd_tlb()
{
	unsigned long cr0 = read_cr0();
	u8 *gva = malloc(sizeof(u8));
	assert(gva != NULL);
	u8 result = 0;

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == WRITE_INITIAL_VALUE) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr0(cr0 | X86_CR0_CD);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	}
	else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

static void test_instruction_fetch_at_ring3()
{
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	memcpy(gva, temp, strlen(temp));

	if (test_instruction_fetch(gva) == PASS) {
		g_ring3_is_pass = true;
	} else {
		g_ring3_is_pass = false;
	}

	free((void *)gva);
	gva = NULL;
}

/**
 * @brief case name: Execute Disable support_005
 *
 * Summary: When user-mode access, set IA32_EFER.NXE to be 1,clear XD flag in every paging-structure entry,
 *	    execute RET instruction shall have no page fault from  user-mode pages.
 */
static void paging_rqmid_32266_execute_disable_support_005()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);

	wrmsr(PAGING_IA32_EFER, ia32_efer | X86_IA32_EFER_NXE);

	/* All default paging structure entry have cleared the XD flag */

	page_test_at_ring3(test_instruction_fetch_at_ring3, "test instruction fetch");

	report("%s", g_ring3_is_pass, __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name: Execute Disable support_004
 *
 * Summary: When user-mode access, set IA32_EFER.NXE to be 0,clear XD flag in every paging-structure entry,
 *	    execute RET instruction shall have no page fault from user-mode pages.
 */
static void paging_rqmid_32407_execute_disable_support_004()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);

	wrmsr(PAGING_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);

	page_test_at_ring3(test_instruction_fetch_at_ring3, "test instruction fetch");

	report("%s", g_ring3_is_pass, __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name: Execute Disable support_003
 *
 * Summary: When supervisor-mode access,set IA32_EFER.NXE to be 1,set XD flag to 1 in PTE table,
 *	    execute RET instruction with supervisor-mode pages shall generate #PF
 */
static void paging_rqmid_32267_execute_disable_support_003()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	wrmsr(PAGING_IA32_EFER, ia32_efer | X86_IA32_EFER_NXE);

	memcpy(gva, temp, strlen(temp));

	set_page_control_bit(gva, PAGE_PTE, PAGE_XD_FLAG, 1, true);

	report("%s", (test_instruction_fetch(gva) == PF_VECTOR),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_XD_FLAG, 0, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name: Execute Disable support_002
 *
 * Summary: When supervisor-mode access,set IA32_EFER.NXE to be 0,
 *	    execute RET instruction with supervisor-mode pages shall have no page falut
 */
static void paging_rqmid_32268_execute_disable_support_002()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	wrmsr(PAGING_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);

	/* set to supervisor mode pages */
	set_page_control_bit(gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	memcpy(gva, temp, strlen(temp));

	report("%s", (test_instruction_fetch(gva) == PASS),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name: 4-level Paging Support_006
 *
 * Summary: Prepare a linear address GVA, set PTE reserved bit to 1 of GVA,
 *	    write a value to the memory pointed by the GVA, shall generate #PF and
 *	    error code bit 2 shall be 1H .
 *	    The fault was caused by a reserved bit set to 1 in some paging-structure entry
 */
static void paging_rqmid_32350_check_error_code_rsvd_bit()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	/* set reserved bit to 1 in PTE table */
	set_page_control_bit(gva, PAGE_PTE, (page_control_bit)PAGE_PTE_RESERVED_BIT, 1, true);

	report("%s", (write_memory_error_code_checking(gva, ERROR_CODE_RSVD_BIT) == true),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, (page_control_bit)PAGE_PTE_RESERVED_BIT, 0, true);
	free_gva((void *)gva);
}

/**
 * @brief case name: 4-level Paging Support_005
 *
 * Summary: When supervisor-mode access, set read only pages,
 *	    write value to the read only pages shall generate #PF and error code bit1 shall be 1H
 */
static void paging_rqmid_32351_check_error_code_wr_bit()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	/* set only read pages */
	set_page_control_bit(gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	report("%s", (write_memory_error_code_checking(gva, ERROR_CODE_WR_BIT) == true),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	free_gva((void *)gva);
}

/**
 * @brief case name: 4-level Paging Support_004
 *
 * Summary: When supervisor-mode access, set read only pages,
 *	    write value to the read only pages shall generate #PF and error code bit0 shall be 1H
 */
static void paging_rqmid_32352_check_error_code_p_bit()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	/* set only read pages */
	set_page_control_bit(gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	report("%s", (write_memory_error_code_checking(gva, ERROR_CODE_P_BIT) == true),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	free_gva((void *)gva);
}

/**
 * @brief case name: 4-level Paging Support_003
 *
 * Summary: When supervisor-mode access, prepare a linear address GVA, clear P flag of GVA's PTE,
 *	    read the memory pointed by the GVA shall generate #PF exception and
 *	    error code shall be 0000H
 */
static void paging_rqmid_32353_check_error_code_all_bit()
{
	u8 *gva = (u8 *)malloc(sizeof(u8));
	u16 error_code;
	u8 vector;
	assert(gva != NULL);

	*gva = 0x12;
	/* clear p flag in PTE table */
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 0, true);

	get_vector_error_code_by_read_memory(gva, &vector, &error_code);

	report("%s", ((vector == PF_VECTOR) && (error_code == (u16)ERROR_CODE_INIT_VALUE)),
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free_gva((void *)gva);
}

static void test_error_code_us_bit_at_ring3()
{
	assert(g_gva != NULL);

	if (write_memory_error_code_checking(g_gva, ERROR_CODE_US_BIT) == true) {
		g_ring3_is_pass = true;
	} else {
		g_ring3_is_pass = false;
	}
}

/**
 * @brief case name: 4-level Paging Support_002
 *
 * Summary: When supervisor-mode access, prepare a linear address GVA, clear P flag of GVA's PTE,
 *	    read the memory pointed by the GVA shall generate #PF exception and
 *	    error code shall be 0000H
 */
static void paging_rqmid_32558_check_error_code_us_bit()
{
	g_gva = (u8 *)malloc(sizeof(u8));
	assert(g_gva != NULL);

	/* set only read pages */
	set_page_control_bit(g_gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, true);

	page_test_at_ring3(test_error_code_us_bit_at_ring3, "test error code us bit");
	report("%s", g_ring3_is_pass, __FUNCTION__);

	/* resume environment */
	set_page_control_bit(g_gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	free_gva((void *)g_gva);
}

struct page_invpcid_desc {
	unsigned long pcid : 12;
	unsigned long rsv  : 52;
	unsigned long addr : 64;
};

static int page_invpcid_checking(unsigned long type, void *desc)
{
	asm volatile (ASM_TRY("1f")
		  ".byte 0x66,0x0f,0x38,0x82,0x18 \n\t" /* invpcid (%rax), %rbx */
		  "1:" : : "a" (desc), "b" (type));
	return exception_vector();
}

/**
 * @brief case name:Hide Invalidate Process-Context Identifier_001
 *
 * Summary:Execute CPUID.(EAX=7,ECX=0):EBX[bit 10] shall be 0, execute INVPCIDE shall generate #UD.
 */
static void paging_rqmid_23912_hide_invalidate_processor_context_identifiers()
{
	struct page_invpcid_desc desc;
	bool is_pass = false;

	if ((cpuid(7).c & (1 << 10)) == 0) {
		if (page_invpcid_checking(2, &desc) == UD_VECTOR) {
			is_pass = true;
		}
	}

	report("paging_rqmid_23912_hide_invalidate_processor_context_identifiers", is_pass);
}

/*64bit mode sample case code*/
void test_paging_64bit_mode()
{
	paging_rqmid_23912_hide_invalidate_processor_context_identifiers();
	paging_rqmid_23918_write_protect_support();
	paging_rqmid_24522_tlb_support();
	paging_rqmid_24519_disable_global_paging();
	paging_rqmid_26017_smep_support();
	paging_rqmid_23917_protection_keys_hide();
	paging_rqmid_24460_cr4_smap_invalidate_tlb();
	paging_rqmid_26827_enable_global_paging();
}

/*non-sample case code*/
void test_paging_64bit_mode_p2()
{
	/*
	 * The case of test_id1 and the test_id2 cannot run at the same time,
	 * so need to be separated
	 */

	u32 test_id = 1;
	switch (test_id) {
	case 1:
		//paging_rqmid_23912_hide_invalidate_processor_context_identifiers();
		//paging_rqmid_23918_write_protect_support();
		//paging_rqmid_24522_tlb_support();
		//paging_rqmid_24519_disable_global_paging();
		//paging_rqmid_26017_smep_support();
		//paging_rqmid_23917_protection_keys_hide();
		paging_rqmid_32224_ring3_access_super_address();
		paging_rqmid_32227_ring3_access_user_address();
		paging_rqmid_32228_write_protect_support_011();
		paging_rqmid_32229_write_protect_support_010();
		paging_rqmid_32230_write_protect_support_009();
		paging_rqmid_32231_write_protect_support_008();
		paging_rqmid_32232_write_protect_support_007();
		paging_rqmid_32233_write_protect_support_006();
		paging_rqmid_32307_write_protect_support_005();
		paging_rqmid_32308_write_protect_support_004();
		paging_rqmid_32309_write_protect_support_003();
		paging_rqmid_32310_write_protect_support_002();
		paging_rqmid_32311_smep_test_002();
		paging_rqmid_32312_smap_003();
		paging_rqmid_32326_smap_002();

		paging_rqmid_32328_ia32_efer_nxe_tlb();
		paging_rqmid_32329_cr4_smep_tlb();
		paging_rqmid_32344_cr4_pse_tlb();
		paging_rqmid_32406_cr4_pge_tlb();
		paging_rqmid_32234_cr0_wp_tlb();
		paging_rqmid_32236_cr0_cd_tlb();
		//paging_rqmid_24460_cr4_smap_invalidate_tlb();

		paging_rqmid_32266_execute_disable_support_005();
		paging_rqmid_32407_execute_disable_support_004();
		paging_rqmid_32268_execute_disable_support_002();

		paging_rqmid_32350_check_error_code_rsvd_bit();
		paging_rqmid_32351_check_error_code_wr_bit();
		paging_rqmid_32352_check_error_code_p_bit();
		paging_rqmid_32558_check_error_code_us_bit();
		paging_rqmid_32353_check_error_code_all_bit();
		break;
	case 2:
		paging_rqmid_32267_execute_disable_support_003();
		//paging_rqmid_26827_enable_global_paging();
		break;
	}
}

