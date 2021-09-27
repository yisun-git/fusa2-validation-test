/* SEFFE: Structured Extended Feature Flags Enumeration Leaf */
#define CPUID_LEAF_SEFFE  0x07
/* PKU: Protection keys for user-mode pages */
#define CPUID_FEATURE_SEFFE_PKU  3

static void
paging_rqmid_35133_page_size_extension_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>3)&1, __func__);
}

static void
paging_rqmid_35134_page_1gb_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(0x80000001).d)>>26)&1, __func__);
}

static void
paging_rqmid_35136_supervisor_mode_access_prevention_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(7).b)>>20)&1, __func__);
}

static void
paging_rqmid_35137_execute_disable_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(0x80000001).d)>>20)&1, __func__);
}

static void
paging_rqmid_35140_4level_paging_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>16)&1, __func__);
}

static void
paging_rqmid_35141_supervisor_mode_execution_prevention_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(7).b)>>7)&1, __func__);
}

static void
paging_rqmid_35152_32bit_paging_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>17)&1, __func__);
}

static void
paging_rqmid_35153_pae_paging_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>6)&1, __func__);
}

static void
paging_rqmid_35154_global_pages_support_physical_platform_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>13)&1, __func__);
}

static void
paging_rqmid_35156_4way_set_associative_data_tlb_32entries_2M4M_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	/*
	 * 0x63:Data TLB: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries and a separate array
	 * with 1 GByte pages,4-way set associative, 4 entries
	 */
	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0x63)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_35157_4way_set_associative_data_tlb_4entries_1GB_page_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	/*
	 * 0x63:Data TLB: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries and a separate array
	 * with 1 GByte pages,4-way set associative, 4 entries
	 */
	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0x63)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_35158_6way_set_associative_shared_level2_tlb_1536entries_4K2M_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0xc3)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_35159_4way_set_associative_level1_data_tlb_64entries_4K_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	/*
	 * 0x03: Data TLB: 4 KByte pages, 4-way set associative, 64 entries
	 */
	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0x03)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_35160_64byte_prefetching_support_physical_platform_constraint_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0xf0)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_35161_full_associative_level1_instruction_tlb_8entries_2M4M_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0x76)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_36259_8_way_set_associative_level_1_instruction_TLB_128_entries_4K_byte_pages_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0xb6)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_36260_Physical_physical_address_width_support_AC_001()
{
	report("%s", ((cpuid(0x80000008).a) & 0xff) == 0x27, __func__);
}

static void
paging_rqmid_36257_TLB_and_other_address_translation_parameters_via_CPUID_2H_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0xff)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_36262_Physical_CPUID_80000008H_EAX_7_0_AC_001()
{
	report("%s", ((cpuid(0x80000008).a) & 0xff) == 0x27, __func__);
}

static void
paging_rqmid_36258_4way_set_associative_shared_level_2_TLB_16_entries_1G_byte_pages_AC_001()
{
	struct cpuid r = cpuid(2);
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 i;
	int found = 0;

	for (i = 0; i < 4; i++) {
		if (check_value_is_exist(cpuid_value[i], 0x63)) {
			found = 1;
			break;
		}
	}

	report("%s", found, __func__);
}

static void
paging_rqmid_36263_Physical_CPUID_80000008H_EAX_15_8_AC_001()
{
	report("%s", (((cpuid(0x80000008).a)>>8) & 0xff) == 0x30, __func__);
}

static void
paging_rqmid_36261_Physical_Linear_address_Width_Support_AC_001()
{
	report("%s", (((cpuid(0x80000008).a)>>8) & 0xff) == 0x30, __func__);
}

static void paging_rqmid_35155_tlb_support_physical_platform_constraint_AC_001()
{
	u8 *p = malloc(sizeof(u8));
	assert(p);

	*p = 1;

	set_page_control_bit((void *)p, PAGE_PTE, PAGE_P_FLAG, 0, false);

	report("%s", (*p == 1), __func__);

	free((void *)p);
}

static void paging_rqmid_35064_write_protect_support_physical_platform_constraint_AC_001()
{
	u8 *p = malloc(sizeof(u8));
	assert(p);

	set_page_control_bit((void *)p, PAGE_PTE, PAGE_WRITE_READ_FLAG, 0, false);
	set_page_control_bit((void *)p, PAGE_PTE, PAGE_USER_SUPER_FLAG, 1, true);

	asm volatile(ASM_TRY("1f")
		"mov %%al, %0\n"
		"1:"
		::"m"(*p)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	free((void *)p);
}


/* Executes CPUID.7H:ECX.PKU == 0 */
static void
paging_rqmid_40363_protection_keys_not_support_hysical_platform_constraint_AC_001()
{
	report("%s", 0 == ((cpuid(CPUID_LEAF_SEFFE).c >> CPUID_FEATURE_SEFFE_PKU) & 1), __func__);
}

/**
 * @brief case name:CPUID.80000008H:EAX[7:0]_001
 *
 * Summary: Execute CPUID.80000008H:EAX[7:0] to get the physical-address width
 *		supported by the processor, it shall be 39.
 */
static void paging_rqmid_23895_check_physical_address_width()
{
	bool is_pass = false;

	if ((cpuid(0x80000008).a & 0xff) == PHYSICAL_ADDRESS_WIDTH) {
		is_pass = true;
	}

	report("paging_rqmid_23895_check_physical_address_width", is_pass);
}

static void paging_rqmid_32347_4level_4K_page_support()
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
