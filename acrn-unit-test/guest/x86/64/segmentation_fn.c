#define USE_GDT_FUN 1

u32 seg_selector = (0x40<<3);
u32 code_selector = (0x50<<3);
u32 data_selector = (0x60<<3);

void call_gate_function(void)
{
	printf("call_gate_function ok\n");
}

asm("call_gate_ent: \n\t"
	"mov %rsp, %rdi \n\t"
	"call call_gate_function\n\t"
	"lretq");
void call_gate_ent(void);

extern gdt_entry_t gdt64[];
static void init_call_gate_64
	(u32 index, u32 code_selector, u8 type, u8 dpl, u8 p, bool is_canon, call_gate_fun fun)
{
	int num = index>>3;
	uint64_t call_base = (uint64_t)fun;
	struct gate_descriptor *call_gate_entry = NULL;

	call_gate_entry = (struct gate_descriptor *)&gdt64[num];
	call_gate_entry->gd_looffset = call_base&0xFFFF;

	if (is_canon == 0) {
		if (BIT_IS(call_base, 48)) {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0xFFFF00000000);
		} else {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0x000000000000);
		}
	}
	else {
		if (BIT_IS(call_base, 48)) {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0x000000000000);
		} else {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0xFFFF00000000);
		}
	}

	call_gate_entry->gd_selector = code_selector;
	call_gate_entry->gd_ist = IST_TABLE_INDEX;
	call_gate_entry->gd_xx = UNUSED;
	call_gate_entry->gd_type = type;
	call_gate_entry->gd_dpl = dpl;
	call_gate_entry->gd_p = p;
	call_gate_entry->sd_xx1 = 0x0;

	u64 *point = (u64 *)&gdt64[num];
	printf("64 call gate %lx %lx\n", (u64)*point, (u64)*(point+1));
}

/* Table 1 */
void lds_p(void *data)
{
	struct lseg_st lds;

	lds.offset = 0xffffffff;
	/*index =2 32/64-bit data segment*/
	lds.selector = 0x10;

	asm volatile(
		//"lds  %0, %%eax\t\n"
		".byte 0xc5, 0x45, 0xf8"
		::"m"(lds)
	);
}

/**
 * @brief case name:segmentation_exception_check_lds_ud_table1_001
 *
 * Summary: Execute LDS in 64-bit mode,will generate #UD exception.
 */
static void segmentation_rqmid_35235_lds_ud_table1_01()
{
	int ret1;
	trigger_func fun1;

	fun1 = lds_p;
	ret1 = test_for_exception(UD_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 2 */
void pop_ds_p(void *data)
{
	asm volatile(
		/* "push %ds\n\t" */
		".byte 0x1e\n\t"
		/* "pop %ds\n\t" */
		".byte 0x1f\n\t"
	);
}

/**
 * @brief case name:segmentation_exception_check_lds_ud_table2_001
 *
 * Summary: Execute POP DS in 64-bit mode,will generate #UD exception.
 */
static void segmentation_rqmid_35236_lds_ud_table2_02()
{
	int ret1;
	trigger_func fun1;

	fun1 = pop_ds_p;
	ret1 = test_for_exception(UD_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 3 */
/**
 * @brief case name:segmentation_exception_check_lfs_gp_table3_001
 *
 * Summary: Execute LFS in 64-bit mode,the segment selector index is not
 * within descriptor table limits, will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35237_lfs_gp_table3_01()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lfs_index_1024;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_2000H)), __FUNCTION__);
}

void lfs_non_canonical(void *data)
{
	u64 *address;
	struct lseg_st64 lfs;

	lfs.offset = 0xFFFFFFFF;
	/* index =0x2 */
	lfs.selector = SELECTOR_INDEX_10H;

	address = creat_non_canon_add((u64)&lfs);
	//printf("%s %d %p 0x%lx\n", __FUNCTION__, __LINE__, &lfs, (u64)address);

	asm volatile(
		"REX\n\t" "lfs  %0, %%rax\t\n"
		::"m"(address)
	);
}

/**
 * @brief case name:segmentation_exception_check_fs_gp_table3_002
 *
 * Summary: Execute LFS in 64-bit mode, the memory address of the descriptor
 * is non-canonical, will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35238_fs_gp_table3_02()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lfs_non_canonical;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	/*non_safety:error_code = 0x5670
	 *safety:error_code = 0x56f0
	 */
	printf("error_code = 0x%x\n", exception_error_code());
	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_10H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_gp_table3_003
 *
 * Summary: In 64-bit mode, the segment is neither a data nor a readable
 * code segment, execute LFS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35239_lfs_gp_table3_03()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*the segment is neither a data nor a readable code segment*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	fun1 = lfs_index_80;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_gp_table3_004
 *
 * Summary: In 64-bit mode,  the segment is a data segment and both RPL and
 * CPL are greater than DPL, execute LFS will generate #GP(Segmentation Selector)
 */
static void test_lfs_gp_table3_04(const char *fun)
{
	int ret = 0;
	trigger_func fun1;

	fun1 = lfs_rpl_3_index_80;
	ret = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), fun);
}

static void segmentation_rqmid_35240_lfs_gp_table3_04()
{

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	do_at_ring3(test_lfs_gp_table3_04, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_np_table3_005
 *
 * Summary: In 64-bit mode, the segment is marked not present,
 * execute LFS will generate #NP(Segmentation Selector)
 */
static void segmentation_rqmid_35241_lfs_np_table3_05()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lfs_index_80;
	ret1 = test_for_exception(NP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/* Table 4 */
/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_001
 *
 * Summary: In 64-bit mode, a NULL selector is attempted to be loaded
 * into the SS register in CPL3 and 64-bit mode will generate #GP(0)
 */
static void test_ss_gp_table4_01(const char *fun)
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lss_index_0;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == 0)), fun);
}

static void segmentation_rqmid_35266_ss_gp_table4_01()
{
	do_at_ring3(test_ss_gp_table4_01, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_002
 *
 * Summary: In 64-bit mode, a NULL selector is attempted to be loaded
 * into the SS register in non-CPL3 and 64-bit mode where its RPL is
 * not equal to CPL will generate #GP(0)
 */
static void segmentation_rqmid_35267_ss_gp_table4_02()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lss_rpl_2_index_0;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_003
 *
 * Summary: In 64-bit mode, the segment selector index is not within the
 * descriptor table limit, execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35269_ss_gp_table4_03()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lss_index_1024;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_2000H)), __FUNCTION__);
}

void lss_non_canonical(void *data)
{
	u64 *address;
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/* index =0x2 */
	lss.selector = SELECTOR_INDEX_10H;

	address = creat_non_canon_add((u64)&lss);

	asm volatile(
		"REX\n\t" "lss  %0, %%rax\t\n"
		::"m"(address)
	);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_004
 *
 * Summary: In 64-bit mode, the segment selector index is not within the
 * descriptor table limit, execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35270_ss_gp_table4_04()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lss_non_canonical;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	/*non_safety:error_code = 0x5670
	 *safety:error_code = 0x56f0
	 */
	printf("error_code = 0x%x\n", exception_error_code());
	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_10H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_005
 *
 * Summary: In 64-bit mode,the segment selector RPL is not equal to CPL,
 * execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35271_ss_gp_table4_05()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lss_index_rpl_1_80;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_006
 *
 * Summary: In 64-bit mode,the segment is a read only data segment,
 * execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35272_ss_gp_table4_06()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*the segment is a nonwritable data segment*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lss_index_80;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_007
 *
 * Summary: In 64-bit mode, DPL is 3 and  CPL is 0,
 * execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35273_ss_gp_table4_07()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	fun1 = lss_index_80;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_008
 *
 * Summary: In 64-bit mode, the segment is marked not present,
 * execute LSS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35274_ss_ss_table4_08()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*not present*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lss_index_80;
	ret1 = test_for_exception(SS_VECTOR, fun1, NULL);

	report("\t\t %s", ((ret1 == true)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/* Table 5 */
static void segmentation_rqmid_64_ss_call_ss_table5_01()
{
#if 0
	int ret1 = 0;
	trigger_func fun1;

#ifndef USE_GDT_FUN
		struct segment_desc *gdt_table;
		struct segment_desc *new_gdt_entry;
#endif
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

#if USE_GDT_FUN
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
#else
	gdt_table		= (struct segment_desc *) old_gdt_desc.base;
	new_gdt_entry		= &gdt_table[0x50];
	/*limit is 0xF*/
	new_gdt_entry->limit1	= SEGMENT_LIMIT;
	new_gdt_entry->base1	= BASE1;
	new_gdt_entry->base2	= BASE2;
	new_gdt_entry->type	= SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED;
	new_gdt_entry->s	= DESCRIPTOR_TYPE_CODE_DATE_SEG;
	new_gdt_entry->dpl	= DESCRIPTOR_PRIVILEGE_LEVEL_0;
	new_gdt_entry->p	= SEGMENT_PRESENT;
	new_gdt_entry->limit	= SEGMENT_LIMIT2;
	new_gdt_entry->avl	= AVL0;
	new_gdt_entry->l	= 0;
	new_gdt_entry->db	= DEFAULT_OPERtion_32_BIT_SEG;
	new_gdt_entry->g	= GRANULARITY;
	new_gdt_entry->base3	= BASE3;

	u64 *point = (u64 *)new_gdt_entry;
	printf("%lx\n", *point);
#endif
	lgdt(&old_gdt_desc);

	//asm volatile("mov $0x280, %ax \n\t"
	//		"mov %ax, %ss\n\t"
	//		);

	fun1 = lss_index_80;
	fun1(NULL);
	ret1 = test_for_exception(SS_VECTOR, fun1, NULL);
	report("\t\t %s", (ret1 == true), __FUNCTION__);
#endif
}


/* Table 6 */
void call_jmp_tese_fun()
{
	printf("%s %d\n", __FUNCTION__, __LINE__);
	return;
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_001
 *
 * Summary: In 64-bit mode, memory operand effective address is non-canonical,
 * execute JMP will generate #GP(0)
 */
static void segmentation_rqmid_35276_cs_jmp_GP_table6_01()
{
	int ret1 = 0;
	u64 fun_p = (u64)call_jmp_tese_fun;

	fun_p = (u64)creat_non_canon_add(fun_p);

	asm volatile(ASM_TRY("1f")
		"jmp *%0\n\t"
		"1:"
		::"m"(fun_p));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_002
 *
 * Summary: In 64-bit mode, if the segment descriptor pointed to by the segment
 * selector in the destination operand is write read access data segment,
 * execute JMP will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35277_cs_jmp_GP_table6_02()
{
	int ret1 = 0;
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*not for a conforming-code segment, nonconforming-code segment, 64-bit call gate */
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	lcs.selector = 0x50 << 3;

	//".byte 0xff, 0x6d, 0xd0\n\t"
	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/*Table 7*/
/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_001
 *
 * Summary: In 64-bit mode, memory operand effective address is non-canonical,
 * execute CALL will generate #GP(0)
 */
static void segmentation_rqmid_35279_cs_call_GP_table7_01()
{
	int ret1 = 0;
	u64 fun_p = (u64)call_jmp_tese_fun;

	fun_p = (u64)creat_non_canon_add(fun_p);

	asm volatile(ASM_TRY("1f")
		"call *%0\n\t"
		"1:"
		::"m"(fun_p));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_002
 *
 * Summary: In 64-bit mode, segment selector index is not within descriptor
 * table limits, execute CALL will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35280_cs_call_GP_table7_02()
{
	//u16 oldlimit;
	int ret1 = 0;
	struct lseg_st64 lcs;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	//oldlimit = old_gdt_desc.limit;
	//set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
	//	DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
	//	GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	//old_gdt_desc.limit = 0x50*8;

	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	lcs.selector = 0x400 << 3;

	//".byte 0xff, 0x6d, 0xd0\n\t"
	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	/* resume environment */
	//old_gdt_desc.limit = oldlimit;
	//lgdt(&old_gdt_desc);

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_2000H)), __FUNCTION__);
}

/*Table 8*/
/**
 * @brief case name:segmentation_exception_check_call_GP_table8_001
 *
 * Summary: In 64-bit mode, the DPL for a conforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35283_call_GP_table8_01()
{
	int ret1 = 0;
	struct lseg_st64 lcs;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment DPL =3 CPL=0
	 * The DPL for a conforming-code segment is greater than the CPL)
	 */
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	lcs.selector = 0x50 << 3;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_call_GP_table8_002
 *
 * Summary: In 64-bit mode, Segment is not present, execute CALL
 * will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35284_call_NP_table8_02()
{
	int ret1 = 0;
	struct lseg_st64 lcs;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment is not present*/
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	lcs.selector = 0x50 << 3;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == NP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/* Table 9*/
/**
 * @brief case name:segmentation_exception_check_call_GP_table9_001
 *
 * Summary: In 64-bit mode, the RPL for a non-conforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35286_call_GP_table9_01()
{
	int ret1 = 0;
	struct lseg_st64 lcs;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* non_conforming-code segment */
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	/*RPL=3 CPL=0
	 * the RPL for the segment’s segment selector is greater than the CPL
	 */
	lcs.selector = (0x50 << 3) + 3;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_call_GP_table9_002
 *
 * Summary: In 64-bit mode, the DPL for a nonconforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35288_call_GP_table9_02()
{
	int ret1 = 0;
	struct lseg_st64 lcs;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* non_conforming-code segment DPL=3 CPL=0
	 * the DPL for a nonconforming-code segment is not equal to the CPL
	 */
	set_gdt_entry((0x50<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_tese_fun;
	lcs.selector = 0x50 << 3;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	ret1 = exception_vector();
	report("\t\t %s", ((ret1 == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_280H)), __FUNCTION__);
}

/* Table 10*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_001
 *
 * Summary: In 64-bit mode, the DPL from a 64-bit call gate is 0 and RPL of the
 * 64-bit call gate is 3, CALL this call gate will generate #GP(Call Gate Selector)
 */
static void segmentation_rqmid_35322_call_gate_call_gp_table10_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	/*RPL = 3 CPL=0 DPL=0
	 * The DPL from a 64-bit call gate is less than the CPL
	 * or than the RPL of the 64-bit call gate.
	 */
	target_sel = ((seg_selector + 3) << 16);

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_200H)), __FUNCTION__);

}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_NP_table10_002
 *
 * Summary: In 64-bit mode, 64-bit call gate not present,
 * CALL this call gate will generate #NP(Call Gate Selector)
 */
static void segmentation_rqmid_35323_call_gate_call_np_table10_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);

	/*64-bit call gate not present*/
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 0, 0, call_gate_ent);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = seg_selector << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == NP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_200H)), __FUNCTION__);

}

/* Table 11 */
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table11_003
 *
 * Summary: In 64-bit mode, if target offset in 64-bit call gate is non-canonical,
 * DPL of this call gate is 3 and CPL is 0,  CALL this call gate will generate #GP(0)
 */
static void segmentation_rqmid_35324_call_gate_call_gp_table11_03()
{
	int ret;
	struct lseg_st64 call_st;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	 /*non-canonical address*/
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent);

	/* DPL of the nonconforming destination code segment
	 * pointed by call gate is less than the CPL.
	 * DPL =3 nonconforming
	 */
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	call_st.selector = seg_selector;
	call_st.offset = (u64)creat_non_canon_add(0);

	asm volatile(ASM_TRY("1f")
		"REX\n\t" "lcallw *%0\n\t"
		"1:"
		::"m"(call_st));

	ret = exception_vector();

	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

/* Table 12 */
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table12_002
 *
 * Summary: In 64-bit mode, if target offset in 64-bit call gate is non-canonical,
 * DPL of this call gate is 0 and CPL is 0,  CALL this call gate will generate #GP(0)
 */
static void segmentation_rqmid_35325_call_gate_call_gp_table12_02()
{
	int target_sel, ret;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* 64-bit call gate non-canonical address*/
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 1, call_gate_ent);

	/* call gate & DPL of the destination code segment pointed by
	 * call gate is equal to the CPL
	 */
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = seg_selector << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();

	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}


/* Table 13 */
/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_001
 *
 * Summary: In 64-bit mode, DPL of the 64-bit call gate is 0 and RPL is 3,
 * execute JMP with this call gate will generate #GP(call gate selector)
 */
static void segmentation_rqmid_35326_call_gate_jmp_gp_table13_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	/*RPL = 3 CPL=0 DPL=0
	 * The DPL from a 64-bit call gate is less than the CPL
	 * or than the RPL of the 64-bit call gate.
	 */
	target_sel = ((seg_selector + 3) << 16);

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_200H)), __FUNCTION__);

}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table13_002
 *
 * Summary: In 64-bit mode, 64-bit call gate not present,
 * execute JMP with this call gate will generate #NP(call gate selector)
 */
static void segmentation_rqmid_35328_call_gate_jmp_np_table13_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);

	/*64-bit call gate not present*/
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 0, 0, call_gate_ent);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = seg_selector << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == NP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_200H)), __FUNCTION__);
}

/* Table 14 */
void call_gate_function1(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);

	printf("%s %d ok\n", __FUNCTION__, __LINE__);

	/*config data segment for ss*/
	set_gdt64_entry(data_selector, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*init ss to data_selector*/
	asm volatile("mov $0x300, %ax\n"
		"mov %ax, %ss");
}

asm("call_gate_ent1: \n"
	"call call_gate_function1\n"
	"lretq"
	);
void call_gate_ent1(void);

/* fail */
static void segmentation_rqmid_64_cs_ret_ss_table14_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent1);

	/*config code segment*/
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

static inline u64 read_esp(void)
{
	u64 val;
	asm volatile ("mov %%rsp, %0" : "=mr"(val));
	return val;
}

u64 rsp_64;
u64 *rsp_val;
void call_gate_function2(void)
{
	//u64 val = 0x4567;
	//u64 esp_val = 0x1234;
	//u64 *address;
	//u64 *non_address;

	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	debug_print("rsp_64=0x%lx cs=0x%lx 0x%lx offset=0x%lx\n",
		rsp_64, rsp_val[1], (u64)&rsp_val[1], rsp_val[0]);

	/*set the code segment selector at the top fo stack to NULL*/
	rsp_val[0] = (u64)creat_non_canon_add(rsp_val[0]);
	//rsp_val[1] = creat_non_canon_add(rsp_val[1]);
	debug_print("rsp_64=0x%lx cs=0x%lx 0x%lx offset=0x%lx\n",
		rsp_64, rsp_val[1], (u64)&rsp_val[1], rsp_val[0]);
#if 0
	address = (u64)&esp_val;
	non_address = creat_non_canon_add((u64)&esp_val);
	printf("address=0x%lx esp_val=0x%lx esp=0x%lx\n", (u64)address, esp_val, read_esp());

	asm volatile("pop %0\n\t"
		::"m"(*address));
	printf("address=0x%lx esp_val=0x%lx esp=0x%lx\n", (u64)address, esp_val, read_esp());

	asm volatile("pop %0\n\t"
		::"m"(*address));
	printf("address=0x%lx esp_val=0x%lx esp=0x%lx\n", (u64)address, esp_val, read_esp());

	asm volatile("pop %0\n\t"
		::"m"(*address));
	printf("address=0x%lx esp_val=0x%lx esp=0x%lx\n", (u64)address, esp_val, read_esp());

	asm volatile("pop %0\n\t"
		::"m"(*address));
	printf("address=0x%lx esp_val=0x%lx esp=0x%lx\n", (u64)address, esp_val, read_esp());
#endif
}

asm("call_gate_ent2: \n"
	"mov %rsp, %rdi\n"
	"call call_gate_function2\n"
	"lretq"
	);
void call_gate_ent2(void);

//fail
static void segmentation_rqmid_64_cs_ret_ss_table14_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent2);

	/*config code segment*/
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

void call_gate_function3(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	//debug_print("rsp_64=0x%lx cs=0x%lx 0x%lx offset=0x%lx\n",
	//	rsp_64, rsp_val[1], (u64)&rsp_val[1], rsp_val[0]);

	/*set the code segment selector at the top fo stack to NULL*/
	rsp_val[1] = 0;

	//debug_print("rsp_64=0x%lx cs=0x%lx 0x%lx offset=0x%lx\n",
	//	rsp_64, rsp_val[1], (u64)&rsp_val[1], rsp_val[0]);
}

void call_gate_function3_1(void)
{
	/*resume code segment selecor at stack top*/
	rsp_val[1] = 0x8;
}

asm("call_gate_ent3:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call call_gate_function3\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lretq\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function3_1\n"
	/* resume stacks
	 * Note that if RSI RDI is used in the code from the last time the stack
	 * address was saved to now, it may cause recovery to the wrong address
	 * If you have not manually modified RBP, or the value of RSP,
	 * may not need to restore it
	 */
	/*
	 *"mov %rdi, %rsp\n"
	 *"mov %rsi, %rbp\n"
	 */
	/*return to normal code stream*/
	"lretq\n"
#endif
	);

void call_gate_ent3(void);

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table14_003
 *
 * Summary: In 64-bit mode, call to a function(name is call_gate_ent) by call gate,
 * if you modify the return code segment selector to NULL,
 * execute RET will generate #GP(0)
 */
static void segmentation_rqmid_35329_cs_ret_gp_table14_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent3);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	/* remove exception vector */
	asm volatile("movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n");

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

void call_gate_function4(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	/*set the code segment selector at the top of stack to 1k*/
	rsp_val[1] = 0x2000;
}

void call_gate_function4_1(void)
{
	/*resume code segment selecor at stack top*/
	rsp_val[1] = 0x8;
}

asm("call_gate_ent4:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call call_gate_function4\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lretq\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function4_1\n"
	/* resume stacks
	 * Note that if RSI RDI is used in the code from the last time the stack
	 * address was saved to now, it may cause recovery to the wrong address
	 * If you have not manually modified RBP, or the value of RSP,
	 * may not need to restore it
	 */
	/*
	 *"mov %rdi, %rsp\n"
	 *"mov %rsi, %rbp\n"
	 */
	/*return to normal code stream*/
	"lretq\n"
#endif
	);

void call_gate_ent4(void);

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table14_004
 *
 * Summary: In 64-bit mode, call to a function(name is call_gate_ent) by call gate,
 * if you modify the return code segment selector addresses descriptor beyond
 * descriptor table limit, execute RET will generate #GP(return code segment selector)
 */
static void segmentation_rqmid_35330_cs_ret_gp_table14_04()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent4);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	/* remove exception vector */
	asm volatile("movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n");

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_2000H)), __FUNCTION__);
}

/*Table 15*/
void call_gate_function15_1(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	/*set the code segment selector RPL at the top of stack to 3*/
	rsp_val[1] = 0x8 + 3;

	sgdt(&old_gdt_desc);
	/*Set all D and L bits of the return code segment to 1 */
	set_gdt_entry((0x8<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

void call_gate_function_end_15_1(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	/*resume code segment selecor at stack top*/
	rsp_val[1] = 0x8;

	sgdt(&old_gdt_desc);
	/*resume the code segment D bit to 0*/
	set_gdt_entry((0x8<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

asm("call_gate_ent15_1:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call call_gate_function15_1\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lretq\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function_end_15_1\n"
	/* resume stacks
	 * Note that if RSI RDI is used in the code from the last time the stack
	 * address was saved to now, it may cause recovery to the wrong address
	 * If you have not manually modified RBP, or the value of RSP,
	 * may not need to restore it
	 */
	/*
	 *"mov %rdi, %rsp\n"
	 *"mov %rsi, %rbp\n"
	 */
	/*return to normal code stream*/
	"lretq\n"
#endif
	);

void call_gate_ent15_1(void);

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_001
 *
 * Summary: In 64-bit ring 0 mode, call to a function(name is call_gate_ent)
 * by call gate, if you modify the RPL of the return code segment selector
 * to 3 and modify code segment descriptor L-bit to 1, D-bit to 1,
 * execute RET will generate #GP(return code segment selector)
 */
static void segmentation_rqmid_35332_rpl_ret_gp_table15_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent15_1);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_8H)), __FUNCTION__);
}

void call_gate_function15_2(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	/*set the code segment selector RPL at the top of stack to 2*/
	rsp_val[1] = 0x8 + 2;

	/*set the code segment DPL to 3 and conforming bit to 1*/
	set_gdt_entry((0x8<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

void call_gate_function_end_15_2(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	/*resume code segment selecor at stack top*/
	rsp_val[1] = 0x8;

	sgdt(&old_gdt_desc);
	/*resume code segment*/
	set_gdt_entry((0x8<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

asm("call_gate_ent15_2:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call call_gate_function15_2\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lretq\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function_end_15_2\n"
	/* resume stacks
	 * Note that if RSI RDI is used in the code from the last time the stack
	 * address was saved to now, it may cause recovery to the wrong address
	 * If you have not manually modified RBP, or the value of RSP,
	 * may not need to restore it
	 */
	/*
	 *"mov %rdi, %rsp\n"
	 *"mov %rsi, %rbp\n"
	 */
	/*return to normal code stream*/
	"lretq\n"
#endif
	);

void call_gate_ent15_2(void);

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_002
 *
 * Summary: In 64-bit ring 0 mode, call to a function(name is call_gate_ent)
 * by call gate, if you modify the RPL of the return code segment selector
 * to 2 and modify return code segment descriptor DPL to 3,
 * execute RET will generate #GP(return code segment selector)
 */
static void segmentation_rqmid_35334_rpl_ret_gp_table15_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(seg_selector, code_selector, 0xC, 0, 1, 0, call_gate_ent15_2);
	set_gdt64_entry(code_selector, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (seg_selector << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == SELECTOR_INDEX_8H)), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_020
 *
 * Summary: In 64-bit mode, to construct a instruction address as a
 * non-canonical address, push the non-canonical address into the stack,
 * execute RET will generate #GP(0).
 */
void segmentation_rqmid_35335_rpl_ret_gp_table15_20(void)
{
	int ret;
	u64 esp_val;
	u64 *address;

	address = creat_non_canon_add(0x1001006);
	//address = 0x1001006;
	asm volatile("push %0\n\t"
		::"m"(address));

	asm volatile(ASM_TRY("1f")
		"ret\n\t"
		"1:"
		::);

	ret = exception_vector();

	/* resume environment */
	asm volatile("pop %0\n\t"
		::"m"(esp_val));

	report("\t\t %s", ((ret == GP_VECTOR)
		&& (exception_error_code() == 0)), __FUNCTION__);
}

static void print_case_list_64(void)
{
	printf("\t\t Segmentation feature 64-Bits Mode case list:\n\r");

}

static void test_segmentation_64(void)
{
	/*x86_64__*/
	int branch = 0;
	switch (branch) {
	case 0:
		/* Table 1 */
		segmentation_rqmid_35235_lds_ud_table1_01();

		/* Table 2 */
		segmentation_rqmid_35236_lds_ud_table2_02();

		/* Table 3 */
		segmentation_rqmid_35237_lfs_gp_table3_01();
		segmentation_rqmid_35238_fs_gp_table3_02();
		segmentation_rqmid_35239_lfs_gp_table3_03();
		segmentation_rqmid_35240_lfs_gp_table3_04();
		segmentation_rqmid_35241_lfs_np_table3_05();

		/* Table 4 */
		segmentation_rqmid_35266_ss_gp_table4_01();
		segmentation_rqmid_35267_ss_gp_table4_02();
		segmentation_rqmid_35269_ss_gp_table4_03();
		segmentation_rqmid_35270_ss_gp_table4_04();
		segmentation_rqmid_35271_ss_gp_table4_05();
		segmentation_rqmid_35272_ss_gp_table4_06();
		segmentation_rqmid_35273_ss_gp_table4_07();
		segmentation_rqmid_35274_ss_ss_table4_08();

		/* Table 5 */
		segmentation_rqmid_64_ss_call_ss_table5_01();

		/* Table 6 */
		segmentation_rqmid_35276_cs_jmp_GP_table6_01();
		segmentation_rqmid_35277_cs_jmp_GP_table6_02();

		/* Table 7 */
		segmentation_rqmid_35279_cs_call_GP_table7_01();
		segmentation_rqmid_35280_cs_call_GP_table7_02();

		/* Table 8 */
		segmentation_rqmid_35283_call_GP_table8_01();
		segmentation_rqmid_35284_call_NP_table8_02();

		/* Table 9 */
		segmentation_rqmid_35286_call_GP_table9_01();
		segmentation_rqmid_35288_call_GP_table9_02();

		/* Table 10 */
		segmentation_rqmid_35322_call_gate_call_gp_table10_01();
		segmentation_rqmid_35323_call_gate_call_np_table10_02();

		/* Table 11 */
		segmentation_rqmid_35324_call_gate_call_gp_table11_03();

		/* Table 12 */
		segmentation_rqmid_35325_call_gate_call_gp_table12_02();

		/* Table 13 */
		segmentation_rqmid_35326_call_gate_jmp_gp_table13_01();
		segmentation_rqmid_35328_call_gate_jmp_np_table13_02();

		/* Table 14 */
		segmentation_rqmid_35329_cs_ret_gp_table14_03();
		segmentation_rqmid_35330_cs_ret_gp_table14_04();

		/*Table 15*/
		segmentation_rqmid_35332_rpl_ret_gp_table15_01();
		segmentation_rqmid_35334_rpl_ret_gp_table15_02();
		segmentation_rqmid_35335_rpl_ret_gp_table15_20();
		break;

	case 1:
		/*Table 14*/
		segmentation_rqmid_64_cs_ret_ss_table14_01();	//fail
		segmentation_rqmid_64_cs_ret_ss_table14_02(); //fail
		break;

	default:
		break;
		}

}
