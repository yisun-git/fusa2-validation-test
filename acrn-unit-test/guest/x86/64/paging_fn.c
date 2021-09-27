static u8 *g_gva = NULL;
/* test at ring3, return the result to this global value */
static bool g_ring3_is_pass = false;

/**
 * @brief case name:Encoding of CPUID Leaf 2 Descriptors_001
 *
 * Summary: check TLB information in CPUID.02H. Because the order of descriptors
 *          in the EAX, EBX, ECX, and EDX registers is not defined,
 *          the descriptors may appear in any order. While checking CPUID:02 registers,
 *          the return value may be disordered while comparing with default value.
 *          So we will check each different "8bit value" if exists
 *          in EAX/EBX/ECX/EDX, and not care it's order.
 */
static void paging_rqmid_23896_check_tlb_info()
{
	struct cpuid r = cpuid(2);
	u8 tlb_info[8] = {TYPE_TLB_01, TYPE_TLB_03, TYPE_TLB_63, TYPE_TLB_76,
			TYPE_TLB_B6, TYPE_STLB_C3,
			TYPE_PREFECTH_F0, TYPE_GENEAL_FF
		};
	u32 cpuid_value[4] = {r.a, r.b, r.c, r.d};
	u32 exist_num = 0;
	u32 i;
	u32 j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 4; i++) {
			if (check_value_is_exist(cpuid_value[i], tlb_info[j])) {
				exist_num++;
				break;
			}
		}
	}

	report("paging_rqmid_23896_check_tlb_info", (exist_num == 8));
}

/**
 * @brief case name:Hide Processor Context Identifiers_001
 *
 * Summary: When process-context identifiers are hidden, CPUID.01H:ECX.
 *		PCID [bit 17] shall be 0, and changing CR4.PCIDE from 0 to 1,shall generate #GP.
 */
static void paging_rqmid_23897_hide_processor_context_identifiers()
{
	unsigned long cr4 = read_cr4();
	bool is_pass = false;

	if ((cpuid(1).c & (1 << 17)) == 0) {
		if (write_cr4_checking(cr4 | X86_CR4_PCIDE) == GP_VECTOR) {
			is_pass = true;
		}
	}

	report("paging_rqmid_23897_hide_processor_context_identifiers", is_pass);
}

/**
 * @brief case name: Global Pages Support_001
 *
 * Summary: Execute CPUID.01H:EDX.PGE [bit 13] shall be 1, set CR4.PGE to enable
 *		global-page feature shall have no exception
 */
static void paging_rqmid_23901_global_pages_support()
{
	unsigned long cr4 = read_cr4();
	bool is_pass = false;

	if ((cpuid(1).c & (1 << 13)) != 0) {
		if (write_cr4_checking(cr4 | X86_CR4_PGE) == PASS) {
			if ((read_cr4() & X86_CR4_PGE) != 0) {
				is_pass = true;
			}
		}
	}

	report("paging_rqmid_23901_global_pages_support", is_pass);
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

/**
 * @brief case name:CPUID.80000008H:EAX[15:8]_001
 *
 * Summary: execute CPUID.80000008H:EAX[15:8] to get the linear-address width shall be 48
 */
static void paging_rqmid_32346_check_linear_address_width()
{
	bool is_pass = false;

	if (((cpuid(0x80000008).a >> 8) & 0xff) == LINEAR_ADDRESS_WIDTH) {
		is_pass = true;
	}

	report("%s", is_pass, __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name: IA32 EFER.NXE state following INIT_001
 *
 * Summary: Get IA32_EFER.NXE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32265_ia32_efer_nxe_init()
{
	bool is_pass = true;
	volatile u64 ia32_init = *(volatile u64 *)INIT_IA32_EFER_LOW_ADDR;

	if ((ia32_init & X86_IA32_EFER_NXE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32265);
	ia32_init = *(volatile u64 *)INIT_IA32_EFER_LOW_ADDR;
	if ((ia32_init & X86_IA32_EFER_NXE) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: EFLAGS.AC state following INIT_001
 *
 * Summary: Get EFLAGS.AC at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32270_eflags_ac_init()
{
	bool is_pass = true;

	volatile u32 eflags_ac_init = *((volatile u32 *)INIT_EFLAGS_ADDR);

	if ((eflags_ac_init & X86_EFLAGS_AC) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32270);
	eflags_ac_init = *((volatile u32 *)INIT_EFLAGS_ADDR);
	if ((eflags_ac_init & X86_EFLAGS_AC) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMEP state following INIT_001
 *
 * Summary: Get CR4.SMEP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32272_cr4_smep_init()
{
	bool is_pass = true;
	u32 cr4 = get_init_cr4();

	if ((cr4 & X86_CR4_SMEP) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32272);

	cr4 = get_init_cr4();
	if ((cr4 & X86_CR4_SMEP) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMAP state following INIT_001
 *
 * Summary: Get CR4.SMAP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32274_cr4_smap_init()
{
	bool is_pass = true;
	u32 cr4 = get_init_cr4();

	if ((cr4 & X86_CR4_SMAP) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32274);

	cr4 = get_init_cr4();
	if ((cr4 & X86_CR4_SMAP) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.PSE state following INIT_001
 *
 * Summary:  Get CR4.PSE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32276_cr4_pse_init()
{
	bool is_pass = true;
	u32 cr4 = get_init_cr4();

	if ((cr4 & X86_CR4_PSE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32276);

	cr4 = get_init_cr4();
	if ((cr4 & X86_CR4_PSE) != 0) {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.PKE state following INIT_001
 *
 * Summary:  Get CR4.PKE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32278_cr4_pke_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PKE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PGE state following INIT_001
 *
 * Summary:  Get CR4.PGE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32280_cr4_pge_init()
{
	bool is_pass = true;
	u32 cr4 = get_init_cr4();

	if ((cr4 & X86_CR4_PGE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32280);

	cr4 = get_init_cr4();
	if ((cr4 & X86_CR4_PGE) != 0) {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.PAE state following INIT_001
 *
 * Summary:  Get CR4.PAE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32297_cr4_pae_init()
{
	bool is_pass = true;
	u32 cr4 = get_init_cr4();

	if ((cr4 & X86_CR4_PAE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32297);

	cr4 = get_init_cr4();
	if ((cr4 & X86_CR4_PAE) != 0) {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR4.PCIDE state following INIT_001
 *
 * Summary:  Get CR4.PCIDE at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32290_cr4_pcide_init()
{
	u32 cr4 = get_init_cr4();

	report("%s", (cr4 & X86_CR4_PCIDE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR0.WP state following INIT_001
 *
 * Summary:  Get CR0.WP at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32304_cr0_wp_init()
{
	bool is_pass = true;
	u32 cr0 = get_init_cr0();

	if ((cr0 & X86_CR0_WP) != 0) {
		is_pass = false;
	}
	notify_modify_and_read_init_value(32304);
	cr0 = get_init_cr0();
	if ((cr0 & X86_CR0_WP) != 0) {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR0.PG state following INIT_001
 *
 * Summary:  Get CR0.PG at AP init, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32306_cr0_pg_init()
{
	bool is_pass = true;
	u32 cr0 = get_init_cr0();

	if ((cr0 & X86_CR0_PG) != 0) {
		is_pass = false;
	}
	notify_modify_and_read_init_value(32306);
	cr0 = get_init_cr0();
	if ((cr0 & X86_CR0_PG) != 0) {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR2 state following INIT_001
 *
 * Summary: Get CR2 at AP init,shall be 00000000H and same with SDM definition.
 */
static void paging_rqmid_32302_cr2_init()
{
	bool is_pass = true;
	volatile u32 cr2 = *((volatile u32 *)INIT_CR2_ADDR);

	if (cr2 != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32306);

	cr2 = *((volatile u32 *)INIT_CR2_ADDR);
	if (cr2 != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: CR3[bit 63:12] state following INIT_001
 *
 * Summary: Get CR3.[bit 63:12] at AP init,shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32300_cr3_init()
{
	volatile u32 cr3_l;
	volatile u64 cr3_64bit;
	u64 cr3_h = 0;
	u64 cr3;
	bool is_pass = true;

	cr3_l = *((volatile u32 *)INIT_CR3_ADDR);
	cr3_64bit = *((volatile u64 *)INIT_64BIT_CR3_ADDR);
	cr3_h = cr3_64bit >> 32;
	cr3 = (cr3_h << 32) | cr3_l;

	if ((cr3 & (~(0xfffull))) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(32300);

	cr3_l = *((volatile u32 *)INIT_CR3_ADDR);
	cr3_64bit = *((volatile u64 *)INIT_64BIT_CR3_ADDR);
	cr3_h = cr3_64bit >> 32;
	cr3 = (cr3_h << 32) | cr3_l;

	if ((cr3 & (~(0xfffull))) != 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}
#endif

/**
 * @brief case name: IA32 EFER.NXE state following start-up_001
 *
 * Summary: Get IA32_EFER.NXE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32264_ia32_efer_nxe_start_up()
{
	volatile u32 *ptr = (volatile u32 *)STARTUP_IA32_EFER_LOW_ADDR;
	u64 ia32_startup = *ptr + ((u64)(*(ptr + 1)) << 32);

	report("%s", (ia32_startup & X86_IA32_EFER_NXE) == 0, __FUNCTION__);
}

/**
 * @brief case name: EFLAGS.AC state following start-up_001
 *
 * Summary: Get EFLAGS.AC at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32269_eflags_ac_start_up()
{
	volatile u32 eflags = *((volatile u32 *)STARTUP_EFLAGS_ADDR);

	report("%s", (eflags & X86_EFLAGS_AC) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMAP state following start-up_001
 *
 * Summary: Get CR4.SMAP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32271_cr4_smap_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_SMAP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.SMEP state following start-up_001
 *
 * Summary: Get CR4.SMEP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32273_cr4_smep_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_SMEP) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PSE state following start-up_001
 *
 * Summary: Get CR4.PSE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32275_cr4_pse_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PSE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PKE state following start-up_001
 *
 * Summary: Get CR4.PKE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32277_cr4_pke_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PKE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PGE state following start-up_001
 *
 * Summary: Get CR4.PGE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32279_cr4_pge_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PGE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PCIDE state following start-up_001
 *
 * Summary: Get CR4.PCIDE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32285_cr4_pcide_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PCIDE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR4.PAE state following start-up_001
 *
 * Summary: Get CR4.PAE at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32295_cr4_pae_start_up()
{
	volatile u32 cr4 = get_startup_cr4();

	report("%s", (cr4 & X86_CR4_PAE) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR3[bit 63:12] state following start-up_001
 *
 * Summary: Get CR3.[bit 63:12] value at BP start-up,  shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32298_cr3_start_up()
{
	u64 cr3;
	volatile u32 cr3_l = *((volatile u32 *)STARTUP_CR3_ADDR);
	volatile u64 cr3_64bit = *((volatile u64 *)STARTUP_64BIT_CR3_ADDR);
	volatile u64 cr3_h = 0;

	cr3_h = cr3_64bit >> 32;
	cr3 = (cr3_h << 32) | cr3_l;

#ifdef IN_NON_SAFETY_VM
	cr3_l = *((volatile u32 *)STARTUP_32BIT_CR3_ADDR_IN_NON_SAFETY);
	cr3 = (cr3_h << 32) | cr3_l;
#endif
	report("%s", (cr3 & (~0xfffull)) == 0, __FUNCTION__);
}

/**
 * @brief case name: CR2 state following start-up_001
 *
 * Summary: Get CR2 value at BP start-up,  shall be 00000000H and same with SDM definition.
 */
static void paging_rqmid_32301_cr2_start_up()
{
	volatile u32 cr2 = *((volatile u32 *)STARTUP_CR2_ADDR);

	report("%s", (cr2 == 0), __FUNCTION__);
}

/**
 * @brief case name: CR0.WP state following start-up_001
 *
 * Summary: Get CR0.WP at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32303_cr0_wp_start_up()
{
	volatile u32 cr0 = get_startup_cr0();

	report("%s", ((cr0 & X86_CR0_WP) == 0), __FUNCTION__);
}

/**
 * @brief case name: CR0.PG state following start-up_001
 *
 * Summary: Get CR0.PG at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void paging_rqmid_32305_cr0_pg_start_up()
{
	volatile u32 cr0 = get_startup_cr0();

	report("%s", ((cr0 & X86_CR0_PG) == 0), __FUNCTION__);
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
	/* Map first 32MB as user pages */
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
		if (write_cr4_checking(cr4 | X86_CR4_PKE) == GP_VECTOR) {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr3(read_cr3());
	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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

	do_at_ring3(write_super_pages_at_ring3, "write memory at ring3");

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
	do_at_ring3(write_user_pages_at_ring3, "write memory at ring3");
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

	map_first_32m_supervisor_pages();
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

	map_first_32m_supervisor_pages();
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

	map_first_32m_supervisor_pages();
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
	map_first_32m_supervisor_pages();
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
	map_first_32m_supervisor_pages();
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
 * Summary: When supervisor-mode access, set CR0.WP to be 1 and set read only pages
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
	map_first_32m_supervisor_pages();
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
	map_first_32m_supervisor_pages();
	write_cr4(read_cr4() | X86_CR4_SMAP);

	stac();

	report("%s", (*gva == WRITE_INITIAL_VALUE), __FUNCTION__);

	/* resume environment */
	resume_environment();
}

/**
 * @brief case name:Supervisor Mode Access Prevention Support_001
 *
 * Summary:  When supervisor-mode access, clear CR4.SMAP, clear CR0.WP,
 *	     read user-mode pages shall successful
 */
static void paging_rqmid_25233_smap_001()
{
	u8 *gva = (u8 *)(USER_BASE + 0x100);
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	write_cr4(read_cr4() & ~X86_CR4_SMAP);
	write_cr0(read_cr0() & ~X86_CR0_WP);

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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_PTE_GLOBAL_PAGE_FLAG, 1, true);
	if (*gva == 0x12) {
		result++;
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	write_cr3(read_cr3());
	if (*gva == 0x12) {
		result++;
	} else {
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

	map_first_32m_supervisor_pages();
	*gva = 0x12;

	set_page_control_bit((void *)gva, PAGE_PTE, PAGE_P_FLAG, 0, false);
	if (*gva == 0x12) {
		result++;
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 | X86_CR4_SMAP);
	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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
	} else {
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
	} else {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	/* for set CR4.SMEP, set first 16M to supervisor-mode pages */
	map_first_32m_supervisor_pages();
	write_cr4(cr4 | X86_CR4_SMEP);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 & ~X86_CR4_PSE);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr4(cr4 | X86_CR4_PGE);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr0(cr0 & ~X86_CR0_WP);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
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
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	write_cr0(cr0 | X86_CR0_CD);

	if (read_memory_checking((void *)gva) == PF_VECTOR) {
		result++;
	} else {
		printf("%s %d test fail\n", __FUNCTION__, __LINE__);
	}

	report("%s", (result == 2), __FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR4.PGE_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR4.PGE, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_24246_changes_cr4_pge_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	write_cr4(read_cr4() ^ X86_CR4_PGE);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR4.SMEP_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR4.SMEP, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37207_changes_cr4_smep_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	map_first_32m_supervisor_pages();
	write_cr4(read_cr4() ^ X86_CR4_SMEP);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR0.WP_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR0.WP, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37206_changes_cr0_wp_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	write_cr0(read_cr0() ^ X86_CR0_WP);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR4.PSE_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR4.PSE, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37205_changes_cr4_pse_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	write_cr4(read_cr4() ^ X86_CR4_PSE);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR4.SMAP_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR4.SMAP, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37204_changes_cr4_smap_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	map_first_32m_supervisor_pages();
	write_cr4(read_cr4() ^ X86_CR4_SMAP);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR0.CD_001
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR0.CD, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37202_changes_cr0_cd_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	write_cr0(read_cr0() ^ X86_CR0_CD);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes IA32_EFER.NXE
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing IA32_EFER.NXE, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_37201_changes_ia32_efer_nxe_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	wrmsr(PAGING_IA32_EFER, ia32_efer ^ X86_IA32_EFER_NXE);

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name:Invalidate Paging Structure Cache When vCPU changes CR3
 *
 * Summary:Config 4-level paging structure, define one GVA1 points to one 1B memory
 * block in the 4K page. Write a random value to gva1 and clear P bit in PDE related
 * with GVA1. we still can access GVA1 normally for paging-structure information is
 * cached in paging-structure cache. After changing CR3, we will get #PF because
 * paging-structure cache is invalidated and get PDE directly from memory.
 */
static void paging_rqmid_32244_changes_cr3_001()
{
	volatile u8 *gva = (volatile u8 *)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 0, false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got %#x != %#x", got, WRITE_INITIAL_VALUE);

	write_cr3(read_cr3());

	asm volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE, PAGE_P_FLAG, 1, true);
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

	do_at_ring3(test_instruction_fetch_at_ring3, "test instruction fetch");

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

	do_at_ring3(test_instruction_fetch_at_ring3, "test instruction fetch");

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

	int i = register_runtime_exception((unsigned long)gva, (unsigned long)&&out);
	asm volatile("jmp *%[addr]\n\t" : : [addr]"r"(gva));
out:
	unregister_runtime_exception(i);

	report("%s", exception_vector() == PF_VECTOR,
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

	memcpy(gva, temp, strlen(temp));

	report("%s", (test_instruction_fetch(gva) == PASS),
		__FUNCTION__);

	/* resume environment */
	resume_environment();
	free_gva((void *)gva);
}

/**
 * @brief case name: Execute Disable support_001
 *
 * Summary: When supervisor-mode access,set IA32_EFER.NXE to be 0, clear U/S flag in PTE table,
 *	    execute RET instruction with supervisor-mode pages shall have no page fault
 */
static void paging_rqmid_25249_execute_disable_support_001()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	wrmsr(PAGING_IA32_EFER, ia32_efer & ~X86_IA32_EFER_NXE);

	memcpy(gva, temp, strlen(temp));

	set_page_control_bit(gva, PAGE_PTE, PAGE_USER_SUPER_FLAG, 0, true);

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

	do_at_ring3(test_error_code_us_bit_at_ring3, "test error code us bit");
	report("%s", g_ring3_is_pass, __FUNCTION__);

	/* resume environment */
	set_page_control_bit(g_gva, PAGE_PTE, PAGE_WRITE_READ_FLAG, 1, true);
	free_gva((void *)g_gva);
}

/**
 * @brief case name: 4-level Paging Support_001
 *
 * Summary: When supervisor-mode access set IA32_EFER.NXE to 1, XD to 0,
 * clear SMEP execute RET shall generate #PF and error code bit4 shall be 1.
 */
static void paging_rqmid_32354_check_error_code_id_bit()
{
	u64 ia32_efer = rdmsr(PAGING_IA32_EFER);
	const char *temp = "\xC3";
	void *gva = malloc(sizeof(u32));
	assert(gva != NULL);

	wrmsr(PAGING_IA32_EFER, ia32_efer | X86_IA32_EFER_NXE);
	write_cr4(read_cr4() & ~X86_CR4_SMEP);

	memcpy(gva, temp, strlen(temp));

	set_page_control_bit(gva, PAGE_PTE, PAGE_XD_FLAG, 1, true);

	int i = register_runtime_exception((unsigned long)gva, (unsigned long)&&out);
	asm volatile("jmp *%[addr]\n\t" : : [addr]"r"(gva));
out:
	unregister_runtime_exception(i);

	report("%s", exception_vector() == PF_VECTOR,
		__FUNCTION__);

	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_XD_FLAG, 0, true);
	wrmsr(PAGING_IA32_EFER, ia32_efer);
	resume_environment();
	free_gva((void *)gva);
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

static void paging_rqmid_32348_4level_2M_page_support()
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

static void paging_rqmid_32349_4level_1G_page_support()
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

	assert_msg(read_memory_checking((void *)p) == PF_VECTOR, "read non memory backed address, no #PF?");
	assert_msg(read_memory_checking((void *)((4ul << 30)-16)) == PF_VECTOR,
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

	if (read_memory_checking((void *)((4ul << 30)-16)) != PASS) {
		printf("read mapped address(4G), #PF?");
		i = -1;
	}

	report("%s", i == test_size, __func__);

	free_pages((void *)gva, 64ul << 20);
	*pdpte = old_val;
	mb();
	write_cr0(read_cr0());
}

static void paging_rqmid_35133_page_size_extension_support_constraint_AC_001()
{
	report("%s", ((cpuid(1).d)>>3)&1, __func__);
}
