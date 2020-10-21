
#define MEM_WR_HIGH_ADDR_START     (0x10000000UL)
#define MEM_WR_HIGH_ADDR_END       (0x20000000UL)

u8 *g_addr;
/* record the access memory at ring3 result */
int g_vector;
/*P must be a non-null pointer */
static int hsi_read_memory_checking(void *p)
{
	u64 value = 1;
	asm volatile(ASM_TRY("1f")
		"mov (%[p]), %[value]\n\t"
		"1:"
		: : [value]"r"(value), [p]"r"(p));
	return exception_vector();
}

static inline int hsi_test_instruction_fetch(void *p)
{
	asm volatile(ASM_TRY("1f")
		"call *%[addr]\n\t"
		"1:"
		: : [addr]"r"(p));

	return exception_vector();
}

static void map_first_32m_supervisor_pages()
{
	/* Map first 32MB as supervisor pages */
	unsigned long i;
	for (i = 0; i < USER_BASE; i += PAGE_SIZE) {
		*get_pte(phys_to_virt(read_cr3()), phys_to_virt(i)) &= ~PT_USER_MASK;
		invlpg((void *)i);
	}

}

static bool check_pages_p_and_rw_flag(u64 start_addr, u64 end_addr)
{
	/* Check all address for p r/w flag */
	unsigned long i;
	pteval_t *pte = NULL;
	for (i = start_addr; i < end_addr; i += PAGE_SIZE) {
		pte = get_pte(phys_to_virt(read_cr3()), phys_to_virt(i));
		if (pte == NULL) {
			debug_print("address 0x%lx pte is null.", i);
			return false;
		}
		if (((*pte & (1ULL << PAGE_P_FLAG)) == 0) ||
			((*pte & (1ULL << PAGE_WRITE_READ_FLAG)) == 0)) {
			debug_print("address 0x%lx pte:0x%lx p or r/w flag is 0.", i, *pte);
			return false;
		}
	}

	return true;
}

static void resume_paging_environment()
{
	unsigned long i;

	write_cr4(read_cr4() & ~X86_CR4_SMAP);
	write_cr4(read_cr4() & ~X86_CR4_SMEP);

	/* Map first 32MB as user pages */
	for (i = 0; i < USER_BASE; i += PAGE_SIZE) {
		*get_pte(phys_to_virt(read_cr3()), phys_to_virt(i)) |= PT_USER_MASK;
		invlpg((void *)i);
	}
}

__unused static void access_memory_at_ring3()
{
	g_vector = hsi_read_memory_checking((void *)g_addr);
}

/**
 * @brief case name: HSI_Memory_Management_Features_Segmentation_001
 *
 * Summary: Configure GDT and segment registers for a code segment, a stack segment and a data segment. change to ring3
 * mode execute, verify that the configuration is successful, execute have no exception.
 *
 */
__unused static void hsi_rqmid_39846_memory_management_features_segmentation_001(void)
{
	do_change_seg = false;

	do_at_ring3(test_segmemt_config_at_ring3, "test GDT config successful");

	report("%s", (do_change_seg == true), __FUNCTION__);
}

/**
 * @brief case name:HSI_Memory_Management_Features_Segmentation_002
 *
 * Summary: Under 64 bit mode on native board, create a new gdt table,
 * execute LGDT instruction to install new GDT and execute SGDT instruction to save GDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
__unused static void hsi_rqmid_39867_memory_management_features_segmentation_002()
{
	segment_lgdt_sgdt_test();
}

/**
 * @brief case name:hsi_rqmid_40002_memory_management_features_segmentation_003
 *
 * Summary: Under 64 bit mode on native board, create a new gdt table,
 * set IDT entry in GDT table, Execute LLDT instruction to load new LDT and
 * execute SLDT intructions to save LDT to memory.
 * Verify that instructions execution with no exception and install/save operation is successful.
 */
static __unused void hsi_rqmid_40002_memory_management_features_segmentation_003()
{
	segment_lldt_sldt_test();
}

/**
 * @brief case name:HSI_Memory_Management_Features_Paging_Address_translation_001
 *
 * Summary: Under 64 bit mode on native board, In 4-level paging mode with 4K page,
 * the mapping between GVA and GPA shall be correct by checking if the value reading
 * directly from GVA and GPA is same.
 */
static void hsi_rqmid_40031_memory_management_features_paging_address_translation_001()
{
	u32 chk = 0;
	struct cpuid r;
	u64 check_bit;

	/* test 4-level paging support */
	r = cpuid(1);
	if ((r.d & CPUID_1_EDX_PAE) != 0) {
		chk++;
	}
	r = cpuid(CPUID_EXTERN_INFORMATION_801);
	if ((r.d & CPUID_801_EDX_LM) != 0) {
		chk++;
	}

	/* test 1-GByte pages are supported with 4-level paging */
	if ((r.d & CPUID_801_EDX_Page1GB) != 0) {
		chk++;
	}

	/* test 4-level paging is enabled */
	/* CR0.PG = 1 */
	check_bit = read_cr0();
	if ((check_bit & (BIT_SHIFT(CR0_BIT_PG))) != 0) {
		chk++;
	}
	/* CR4. PAE = 1 */
	check_bit = read_cr4();
	if ((check_bit & (BIT_SHIFT(CR4_BIT_PAE))) != 0) {
		chk++;
	}
	/* IA32_EFER.LME = 1 */
	check_bit = rdmsr(MSR_EFER);
	if ((check_bit & EFER_LME) != 0) {
		chk++;
	}

	/* get processor MAXPHYADDR value */
	r = cpuid(CPUID_EXTERN_INFORMATION_808);
	if ((r.a & 0xff) == PHYSICAL_ADDRESS_WIDTH) {
		chk++;
	}

	debug_print("chk=%d\n", chk);

	report("%s", (chk == 7), __func__);

}

/**
 * @brief case name:HSI_Memory_Management_Features_Paging_Address_translation_4k_001
 *
 * Summary: Under 64 bit mode on native board, In 4-level paging mode with 4K page,
 * the mapping between GVA and GPA shall be correct by checking if the value reading
 * directly from GVA and GPA is same.
 */
static void hsi_rqmid_40032_memory_management_features_paging_address_translation_4K_001()
{
	volatile u8 *gva = malloc(PAGE_SIZE);
	volatile u8 *gpa;
	pgd_t *pgd = (pgd_t *)read_cr3();
	u32 pte = *get_pte(pgd, (void *)gva);
	u32 i;

	assert(gva);
	assert_msg(((pte>>7)&1) == 0, "pte: %#x", pte);

	for (i = 0; i < PAGE_SIZE; i++) {
		gpa = (volatile u8 *)(uintptr_t)virt_to_pte_phys(pgd, (void *)(gva + i));
		*gpa = i & 0xff;
	}

	for (i = 0; i < PAGE_SIZE; i++) {
		u8 got = gva[i];
		if (got != (i & 0xff)) {
			printf("%d: %x != %x\n", i, got, i & 0xff);
			break;
		}
	}

	report("%s", (i == PAGE_SIZE), __func__);

	free((void *)gva);
}

/**
 * @brief case name:hsi_rqmid_40033_memory_management_features_paging_address_translation_2M_001
 *
 * Summary: Under 64 bit mode on native board, in 4-level paging mode with 2M page,
 * the mapping between GVA and GPA shall be correct by checking
 * if the value reading directly from GVA and GPA is same. 
 */
__unused static void hsi_rqmid_40033_memory_management_features_paging_address_translation_2M_001()
{
	// 2M / 4K = 512 = 2**9
	volatile u8 *gva = alloc_pages(9);
	volatile u8 *gpa = (volatile u8 *)(uintptr_t)virt_to_phys((void *)gva);
	u32 pte = *get_pte((pgd_t *)read_cr3(), (void *)gva);
	u32 i = 0;

	assert(gva);
	assert_msg(((pte>>7)&1) == 1, "pte: %#x", pte);

	for (i = 0; i < LARGE_PAGE_SIZE; i++) {
		gpa[i] = i & 0xff;
	}

	for (i = 0; i < LARGE_PAGE_SIZE; i++) {
		u8 got = gva[i];
		if (got != (i & 0xff)) {
			printf("%d: %x != %x\n", i, got, i & 0xff);
			break;
		}
	}

	report("%s", (i == LARGE_PAGE_SIZE), __func__);

	free_pages((void *)gva, LARGE_PAGE_SIZE);
}

/**
 * @brief case name:HSI_Memory_Management_Features_Paging_Access_Control_001
 *
 * Summary: Under 64 bit mode on native board, in 4-level paging mode with 1G page,
 * the mapping between GVA and GPA shall be correct by checking
 * if the value reading directly from GVA and GPA is same. 
 */
__unused static void hsi_rqmid_40034_memory_management_features_paging_address_translation_1G_001()
{
	// 64M / 4K = 512 = 2**14
	const unsigned test_size = 64ul << 20;
	volatile u8 *gva = alloc_pages(14);
	u32 i = 0;
	u8 support_1g = (cpuid(0x80000001).d >> 26) & 1;
	volatile u8 *p = (volatile u8 *)(3ul << 30);
	unsigned long gpa = virt_to_phys((void *)gva);
	pgd_t *cr3 = (pgd_t *)read_cr3();

	assert(gva);
	assert(support_1g);

	for (i = 0; i < test_size; i++) {
		gva[i] = i & 0xff;
	}

	assert_msg(hsi_read_memory_checking((void *)p) != PF_VECTOR, "read non memory backed address, no #PF?");
	assert_msg(hsi_read_memory_checking((void *)((4ul << 30)-16)) != PF_VECTOR,
				"read non memory backed address(4G), no #PF?");

	pteval_t *pdpte = get_pte_level(cr3, (void *)p, 3);
	pteval_t old_val = *pdpte;
	*pdpte = PT_PAGE_SIZE_MASK | PT_PRESENT_MASK | PT_WRITABLE_MASK | PT_USER_MASK;
	mb();
	write_cr0(read_cr0());

	for (i = 0; i < test_size; i++) {
		u8 got = p[i+gpa];
		if (got != (i&0xff)) {
			printf("%d: got[%x] != expected[%x]\n", i, got, i&0xff);
			break;
		}
	}

	if (hsi_read_memory_checking((void *)((4ul << 30)-16)) != PASS) {
		printf("read mapped address(4G), #PF?");
		i = -1;
	}

	report("%s", i == test_size, __func__);

	free_pages((void *)gva, 64ul << 20);
	*pdpte = old_val;
	mb();
	write_cr0(read_cr0());
}


/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_001
 *
 * Summary: On native board, run on ring 0, set CR4.SMEP to 1, execute RET
 * instruction with a user-mode address should generate #PF.
 * Then, disable the CR4.SMEP, executing should be successful.
 */
static __unused void hsi_rqmid_36537_memory_management_features_paging_access_control_001()
{
	u32 chk = 0;

	gp_trigger_func fun;
	bool ret1;
	int ret2 = 1;

	/* a user-mode address stored the opcode of RET instruction */
	u8 *p = (u8 *)(USER_BASE + 0x100);
	assert(p != NULL);

	const char *temp = "\xC3";
	memcpy(p, temp, strlen(temp));

	map_first_32m_supervisor_pages();

	/* config the Page-Table entry P(bit 0) to 1, U/S(bit 2) to 1(User-mode) */
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 1, true);
	set_page_control_bit(phys_to_virt((u64)p), PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	/* enable SMEP */
	write_cr4(read_cr4() | BIT_SHIFT(CR4_BIT_SMEP));

	/* execute the user-mode address should generate #PF in the supervisor mode */
	fun = (gp_trigger_func)hsi_test_instruction_fetch;
	ret1 = test_for_exception(PF_VECTOR, fun, (void *)p);
	if (ret1 == true) {
		chk++;
	}

	/* disable SMEP */
	write_cr4(read_cr4() & ~BIT_SHIFT(CR4_BIT_SMEP));
	ret2 = hsi_test_instruction_fetch((void *)p);
	if (ret2 == PASS) {
		chk++;
	}

	debug_print("smep enable ret1:%d, smep disable ret2:%d\n", ret1, ret2);
	report("%s", (chk == 2), __FUNCTION__);
	resume_paging_environment();
}

/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_002
 *
 * Summary: On native board, run on ring 0, set CR4.SMAP to 1,
 * read from a user-mode address should generate #PF.
 * Then, disable the CR4.SMAP, reading should be successful.
 */
static __unused void hsi_rqmid_36538_memory_management_features_paging_access_control_002()
{
	u32 chk = 0;
	int ret1 = 1;
	int ret2 = 1;

	/* a user-mode address */
	u8 *p = (u8 *)(USER_BASE + 0x100);
	assert(p != NULL);

	map_first_32m_supervisor_pages();

	/* config the Page-Table entry P(bit 0) to 1, U/S(bit 2) to 1(User-mode) */
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 1, true);
	set_page_control_bit(phys_to_virt((u64)p), PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	/* enable SMAP */
	write_cr4(read_cr4() | BIT_SHIFT(CR4_BIT_SMAP));
	/* clear EFLAGS.AC flag */
	clac();

	/* read the user-mode address should generate #PF in the supervisor mode */
	ret1 = hsi_read_memory_checking(p);
	if (ret1 == PF_VECTOR) {
		chk++;
	}

	/* disable SMAP */
	write_cr4(read_cr4() & ~BIT_SHIFT(CR4_BIT_SMAP));

	/* disable SMAP read the user-mode address should successful in the supervisor mode */
	ret2 = hsi_read_memory_checking(p);
	if (ret2 == PASS) {
		chk++;
	}

	debug_print("access ret1:%d ret2:%d\n", ret1, ret2);

	report("%s", (chk == 2), __FUNCTION__);
	resume_paging_environment();
}


/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_003
 *
 * Summary: On native board, run on ring 0, set CR4.SMAP to 1, execute clac reenabling
 * the SMAP protection, read from a user-mode address should generate #PF.
 * Then, execute stac diabling the SMAP protection, reading should be successful.
 */
static __unused void hsi_rqmid_40022_memory_management_features_paging_access_control_003()
{
	u32 chk = 0;
	int ret1 = 1;
	int ret2 = 1;

	/* a user-mode address */
	u8 *p = (u8 *)(USER_BASE + 0x100);
	assert(p != NULL);

	map_first_32m_supervisor_pages();

	/* config the Page-Table entry P(bit 0) to 1, U/S(bit 2) to 1(User-mode) */
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 1, true);
	set_page_control_bit(phys_to_virt((u64)p), PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	/* enable SMAP */
	write_cr4(read_cr4() | BIT_SHIFT(CR4_BIT_SMAP));
	/* clear EFLAGS.AC flag, reenabling the SMAP protection */
	clac();

	/* read the user-mode address should generate #PF in the supervisor mode */
	ret1 = hsi_read_memory_checking(p);
	if (ret1 == PF_VECTOR) {
		chk++;
	}

	/* set EFLAGS.AC flag, disabling the SMAP protection */
	stac();

	/* set EFLAGS.AC flag read the user-mode address should successful in the supervisor mode */
	ret2 = hsi_read_memory_checking(p);
	if (ret2 == PASS) {
		chk++;
	}

	debug_print("CLAC ret1:%d STAC ret2:%d\n", ret1, ret2);

	report("%s", (chk == 2), __FUNCTION__);
	resume_paging_environment();
	clac();
}

/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_004
 *
 * Summary: On native board, run on ring 0, clear Page-Table entry U/S(bit 2) to 0(Supervisor-mode),
 * read from a supervisor-mode address should generate #PF at ring3.
 * Then set Page-Table entry U/S(bit 2) to 1(user-mode),
 * read from a user-mode address at ring3 should successful.
 */
static __unused void hsi_rqmid_40027_memory_management_features_paging_access_control_004()
{
	u32 chk = 0;
	/* init to a invalid value */
	g_vector = 0xffff;

	g_addr = (u8 *)(USER_BASE + 0x100);
	assert(g_addr != NULL);

	/* config the Page-Table entry P(bit 0) to 1, U/S(bit 2) to 0(Supervisor-mode) */
	set_page_control_bit((void *)g_addr, PAGE_PTE, PAGE_P_FLAG, 1, true);
	set_page_control_bit(phys_to_virt((u64)g_addr), PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

	/* read the supervisor-mode address should generate #PF in the user mode */
	do_at_ring3(access_memory_at_ring3, "access supervisor memory at ring3");
	if (g_vector == PF_VECTOR) {
		chk++;
	}

	debug_print("super address access ret:%d\n", g_vector);
	/* config the Page-Table entry P(bit 0) to 1, U/S(bit 2) to 1(User-mode) */
	set_page_control_bit((void *)g_addr, PAGE_PTE, PAGE_P_FLAG, 1, true);
	set_page_control_bit(phys_to_virt((u64)g_addr), PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	/* read the user-mode address should successful in the user mode */
	do_at_ring3(access_memory_at_ring3, "access user memory at ring3");
	if (g_vector == NO_EXCEPTION) {
		chk++;
	}
	debug_print("user address access ret:%d\n", g_vector);

	report("%s", (chk == 2), __FUNCTION__);
	resume_paging_environment();
}

/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_High_Address_001
 *
 * Summary: On native board, test all available RAM addresses(High address 256~512M) are present and
 * writeable, via checking P flag (bit 0) and R/W flag (bit 1) of page structure entries
 * which reference these RAM addresses.
 * write value to the memory, check value write corrently.
 */
static __unused void hsi_rqmid_40040_memory_management_features_paging_access_control_high_address_001(void)
{
	uint64_t start = MEM_WR_HIGH_ADDR_START;
	uint64_t end = MEM_WR_HIGH_ADDR_END;
	uint64_t size = 0UL;
	bool value_chk = true;
	bool pte_chk = true;
	u32 i;

	size = end - start;

	/* Checking P flag (bit 0) and R/W flag (bit 1) of page structure entries */
	pte_chk = check_pages_p_and_rw_flag(start, end);

	volatile u32 *start_addr = (volatile u32 *)start;
	size = size / sizeof(u32);
	for (i = 0; i < size; i++) {
		start_addr[i] = i;
	}

	delay(1000);

	for (i = 0; i < size; i++) {
		if (start_addr[i] != i) {
			debug_print("check fail num:0x%x write value:%u actual value:%u .\n",
				i, start_addr[i], i);
			value_chk = false;
			break;
		}
	}

	printf("pte_chk:%d value_chk:%d \n", pte_chk, value_chk);
	report("%s", (value_chk && pte_chk), __FUNCTION__);
}


st_case_suit case_suit[] = {
	{
		.case_fun = hsi_rqmid_39846_memory_management_features_segmentation_001,
		.case_id = 39846,
		.case_name = GET_CASE_NAME(hsi_rqmid_39846_memory_management_features_segmentation_001),
	},

	{
		.case_fun = hsi_rqmid_39867_memory_management_features_segmentation_002,
		.case_id = 39867,
		.case_name = GET_CASE_NAME(hsi_rqmid_39867_memory_management_features_segmentation_002),
	},

	{
		.case_fun = hsi_rqmid_40002_memory_management_features_segmentation_003,
		.case_id = 40002,
		.case_name = GET_CASE_NAME(hsi_rqmid_40002_memory_management_features_segmentation_003),
	},

	{
		.case_fun = hsi_rqmid_40031_memory_management_features_paging_address_translation_001,
		.case_id = 40031,
		.case_name = GET_CASE_NAME(hsi_rqmid_40031_memory_management_features_paging_address_translation_001),
	},

	{
		.case_fun = hsi_rqmid_40032_memory_management_features_paging_address_translation_4K_001,
		.case_id = 40032,
		.case_name =
			GET_CASE_NAME(hsi_rqmid_40032_memory_management_features_paging_address_translation_4K_001),
	},

	{
		.case_fun = hsi_rqmid_40033_memory_management_features_paging_address_translation_2M_001,
		.case_id = 40033,
		.case_name =
			GET_CASE_NAME(hsi_rqmid_40033_memory_management_features_paging_address_translation_2M_001),
	},

	{
		.case_fun = hsi_rqmid_40034_memory_management_features_paging_address_translation_1G_001,
		.case_id = 40034,
		.case_name =
			GET_CASE_NAME(hsi_rqmid_40034_memory_management_features_paging_address_translation_1G_001),
	},

	{
		.case_fun = hsi_rqmid_36537_memory_management_features_paging_access_control_001,
		.case_id = 36537,
		.case_name = GET_CASE_NAME(hsi_rqmid_36537_memory_management_features_paging_access_control_001),
	},

	{
		.case_fun = hsi_rqmid_36538_memory_management_features_paging_access_control_002,
		.case_id = 36538,
		.case_name = GET_CASE_NAME(hsi_rqmid_36538_memory_management_features_paging_access_control_002),
	},

	{
		.case_fun = hsi_rqmid_40022_memory_management_features_paging_access_control_003,
		.case_id = 40022,
		.case_name = GET_CASE_NAME(hsi_rqmid_40022_memory_management_features_paging_access_control_003),
	},

	{
		.case_fun = hsi_rqmid_40027_memory_management_features_paging_access_control_004,
		.case_id = 40027,
		.case_name = GET_CASE_NAME(hsi_rqmid_40027_memory_management_features_paging_access_control_004),
	},

	{
		.case_fun = hsi_rqmid_40040_memory_management_features_paging_access_control_high_address_001,
		.case_id = 40040,
		.case_name = GET_CASE_NAME(hsi_rqmid_40040_memory_management_\
			features_paging_access_control_high_address_001),
	},

};

