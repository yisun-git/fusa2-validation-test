#define USE_GDT_FUN 1

#define CALL_GATE_SEL (0x40<<3)
#define CODE_SEL     (0x50<<3)
#define DATA_SEL     (0x54<<3)

#define CODE_2ND     (0x58<<3)

static u16 orig_user_ss;

static void call_gate_function(void)
{
	printf("call_gate_function ok\n");
}

asm("call_gate_ent: \n\t"
	"mov %rsp, %rdi \n\t"
	"call call_gate_function\n\t"
	"lretq");
void call_gate_ent(void);

extern gdt_entry_t gdt64[];
static void init_call_gate_64(u32 index, u32 code_selector, u8 type, u8 dpl, u8 p, bool is_non_canonical,
	call_gate_fun fun)
{
	int num = index>>3;
	uint64_t call_base = (uint64_t)fun;
	struct gate_descriptor *call_gate_entry = NULL;

	call_gate_entry = (struct gate_descriptor *)&gdt64[num];
	call_gate_entry->gd_looffset = call_base&0xFFFF;

	if (is_non_canonical == 0) {
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

static u64 *create_non_canonical_addr(u64 addr)
{
	u64 address = addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address|(1UL<<63));
	}
	return (u64 *)address;
}

/* Table 1 */
static void lds_p(void *data)
{
	struct lseg_st64 lds;

	lds.offset = 0xffffffff;
	/*index =2 32/64-bit data segment*/
	lds.selector = SELECTOR_INDEX_10H;

	//lds instruction not support in 64 bit, can't build
	asm volatile(
		//"lds  %0, %%eax\t\n"
		".byte 0xc5, 0x45, 0xf8"
		::"m"(lds)
	);
}

static void gs_normal_test(void)
{
	struct lseg_st64 seg;
	uint64_t val = 0;

	seg.offset = 0x55aa55aadeadbeaf;
	/*index =1 64-bit code segment*/
	seg.selector = SELECTOR_INDEX_8H;

	uint16_t orig = read_gs();

	asm volatile(
		"lgs  %0, %%r11\t\n"
		::"m"(seg)
	);

	asm volatile ("mov %%r11, %0" : "=r"(val) : : "memory");

	asm volatile(
		"push %gs\n\t"
		"pop %gs\n\t"
	);

	uint16_t changed = read_gs();
	printf("LGS gs = before:0x%x, after: 0x%x, target=0x%lx\n", orig, changed, val);

}

/**
 * @brief case name:segmentation_exception_check_lds_ud_table1_001
 *
 * Summary: Execute LDS in 64-bit mode,will generate #UD exception.
 */
static void segmentation_rqmid_35235_lds_ud_table1_01()
{
	gs_normal_test();

	bool ret1 = test_for_exception(UD_VECTOR, lds_p, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 2 */
static void pop_ds_p(void *data)
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
 * Summary: Execute POP DS in 64-bit mode,will generate #UD exception.
 */
static void segmentation_rqmid_35236_lds_ud_table2_01()
{
	bool ret1 = test_for_exception(UD_VECTOR, pop_ds_p, NULL);

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
	bool ret1 = test_for_exception(GP_VECTOR, lfs_index_1024, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

static void lfs_non_canonical(void *data)
{
	u64 *address;
	struct lseg_st64 lfs;

	lfs.offset = 0xFFFFFFFF;
	/* index =0x2 */
	lfs.selector = SELECTOR_INDEX_10H;

	address = create_non_canonical_addr((u64)&lfs);
	printf("%s: seg addr orig=%p changed to: %p\n", __FUNCTION__, &lfs, address);

	asm volatile(
		"REX\n\t" "lfs  %0, %%rax\t\n"
		::"m"(address)
	);
}

/**
 * @brief case name:segmentation_exception_check_fs_gp_table3_002
 *
 * Summary: Execute LFS in 64-bit mode, the memory address of the descriptor
 * is non-canonical, will generate #GP(N/A).
 */
static void segmentation_rqmid_35238_fs_gp_table3_02()
{
	bool ret1 = test_for_exception(GP_VECTOR, lfs_non_canonical, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_gp_table3_003
 *
 * Summary: In 64-bit mode, the segment is neither a data nor a readable
 * code segment, execute LFS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35239_lfs_gp_table3_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	printf("GDT: base=0x%lx, limit=0x%x\n", old_gdt_desc.base, old_gdt_desc.limit);
	uint64_t *gdt = (uint64_t *)old_gdt_desc.base;
	printf("GDT item: 0x%x, orig: 0x%016lx\n", 0x50, gdt[0x50]);

	/*the segment is neither a data nor a readable code segment*/
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(GP_VECTOR, lfs_index_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_lfs_gp_table3_004
 *
 * Summary: In 64-bit mode, the segment is a data segment and both RPL and
 * CPL are greater than DPL, execute LFS will generate #GP(Segmentation Selector).
 */
static void test_lfs_gp_table3_04(const char *fun)
{
	bool ret = test_for_exception(GP_VECTOR, lfs_rpl_3_index_80, NULL);

	//it is the original FS, not the target FS; //(err_code == SELECTOR_INDEX_280H)
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret == true) && (err_code == (SELECTOR_INDEX_280H & 0xFF8)),
		fun, err_code);
}

static void segmentation_rqmid_35240_lfs_gp_table3_04()
{

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	do_at_ring3(test_lfs_gp_table3_04, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_np_table3_005
 *
 * Summary: In 64-bit mode, the segment is marked not present,
 * execute LFS will generate #NP(Segmentation Selector).
 */
static void segmentation_rqmid_35241_lfs_np_table3_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(NP_VECTOR, lfs_index_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/* Table 4 */
static void test_ss_gp_table4_01(const char *fun)
{
	bool ret1 = test_for_exception(GP_VECTOR, lss_index_0, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == 0),
		fun, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_001
 *
 * Summary: In 64-bit mode, a NULL selector is attempted to be loaded
 * into the SS register in CPL3 and 64-bit mode will generate #GP(0).
 */
static void segmentation_rqmid_35266_ss_gp_table4_01()
{
	do_at_ring3(test_ss_gp_table4_01, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_002
 *
 * Summary: In 64-bit mode, a NULL selector is attempted to be loaded
 * into the SS register in non-CPL3 and 64-bit mode where its RPL is
 * not equal to CPL will generate #GP(0).
 */
static void segmentation_rqmid_35267_ss_gp_table4_02()
{
	bool ret1 = test_for_exception(GP_VECTOR, lss_rpl_2_index_0, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == 0),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_003
 *
 * Summary: In 64-bit mode, the segment selector index is not within the
 * descriptor table limit, execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35269_ss_gp_table4_03()
{
	bool ret1 = test_for_exception(GP_VECTOR, lss_index_1024, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

static void lss_non_canonical(void *data)
{
	u64 *address;
	struct lseg_st lss;

	lss.offset = 0xffffffff;
	/* index =0x2 */
	lss.selector = SELECTOR_INDEX_10H;

	address = create_non_canonical_addr((u64)&lss);

	asm volatile(
		"REX\n\t" "lss  %0, %%rax\t\n"
		::"m"(address)
	);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_004
 *
 * Summary: In 64-bit mode, the memory address of the descriptor is
 * non-canonical, execute LSS will generate #GP(N/A).
 */
static void segmentation_rqmid_35270_ss_gp_table4_04()
{
	bool ret1 = test_for_exception(GP_VECTOR, lss_non_canonical, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_005
 *
 * Summary: In 64-bit mode, the segment selector RPL is not equal to CPL,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35271_ss_gp_table4_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(GP_VECTOR, lss_index_rpl_1_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_006
 *
 * Summary: In 64-bit mode, the segment is a read only data segment,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35272_ss_gp_table4_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*the segment is a read only data segment*/
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(GP_VECTOR, lss_index_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_007
 *
 * Summary: In 64-bit mode, DPL is 3 and CPL is 0,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35273_ss_gp_table4_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(GP_VECTOR, lss_index_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_SS_table4_008
 *
 * Summary: In 64-bit mode, the segment is marked not present,
 * execute LSS will generate #SS(Segmentation Selector).
 */
static void segmentation_rqmid_35274_ss_ss_table4_08()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*not present*/
	set_gdt_entry(SELECTOR_INDEX_280H, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	bool ret1 = test_for_exception(SS_VECTOR, lss_index_80, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_280H),
		__FUNCTION__, err_code);
}

/* Table 5 */
static char ring1_stack[4096] __attribute__((aligned(4096)));
#define MAP_NORMAL_ADDR   (u64)(ring1_stack + 4096)   // it is normal case
#define MAP_NON_CANONICAL_ADDR 0x800000000000  // it will case SS
static void setup_tss(bool normal_stack)
{
	struct descriptor_table_ptr gdt;
	struct segment_desc64 *gdt_table;
	struct segment_desc64 *tss_entry;

	u64 p = MAP_NORMAL_ADDR;
	u16 tr = 0;
	tss64_t *tss;
	uint64_t tss_base;

	sgdt(&gdt);
	tr = str();
	gdt_table = (struct segment_desc64 *) gdt.base;
	tss_entry = &gdt_table[tr / sizeof(struct segment_desc64)];
	tss_base = ((uint64_t) tss_entry->base1 |
			((uint64_t) tss_entry->base2 << 16) |
			((uint64_t) tss_entry->base3 << 24) |
			((uint64_t) tss_entry->base4 << 32));
	tss = (tss64_t *)tss_base;

	if (!normal_stack) {
		p = MAP_NON_CANONICAL_ADDR;
	}

	if (BIT_IS(p, 47)) {
		p = p | 0xFFFF000000000000;
	}

	printf("orig  tr=0x%x, tss rsp0=0x%lx, rsp1=0x%lx, rsp2=0x%lx\n", tr, tss->rsp0, tss->rsp1, tss->rsp2);
	tss->rsp1 = p;
	printf("tss=%p fix tss->rsp1=0x%lx\n", tss, tss->rsp1);

//	ltr(tr); //not need reload TR, and can't load for it is in use
}

static void setup_callgate(void)
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_function);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

static void test_ss_table5_01(const char *args)
{
	int target_sel = (CALL_GATE_SEL + 3) << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, err_code);
}

/**
 * @brief case name:segmentation_exception_check_SS_table5_001
 *
 * Summary: In 64-bit mode, if pushing the old values of SS selector, stack pointer,
 * EFLAGS, CS selector, offset, or error code onto the stack violates the canonical
 * boundary when a stack switch occurs execute CALL will generate #SS.
 */
//ring0 --> swith to ring3 (do_at_ring3), then call gate to ring1 (use ring1 stack)
static void segmentation_rqmid_64_38553_ss_call_ss_table5_01()
{
	setup_tss(false); //if not set rsp1 in TSS, exeception will be PF
	setup_callgate();
	do_at_ring3(test_ss_table5_01, __FUNCTION__);
}


/* Table 6 */
static void call_jmp_test_fun()
{
	printf("%s %d\n", __FUNCTION__, __LINE__);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_001
 *
 * Summary: In 64-bit mode, segment selector in target operand is NULL,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_38645_cs_jmp_GP_table6_01()
{
	struct lseg_st64 lcs;

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = 0;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_002
 *
 * Summary: In 64-bit mode, memory operand effective address is non-canonical,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_35276_cs_jmp_GP_table6_02()
{
	u64 fun_p = (u64)call_jmp_test_fun;

	fun_p = (u64)create_non_canonical_addr(fun_p); //if not change, it can jump

	asm volatile(ASM_TRY("1f")
		"jmp *%0\n\t"
		"1:"
		::"m"(fun_p));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

//used to to compare
void normal_lcall_cs_test(void)
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*not for a conforming-code segment, nonconforming-code segment, 64-bit call gate */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	printf("a normal_lcall_cs_test\n");

	//".byte 0xff, 0x6d, 0xd0\n\t"
	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"    //for ljmp, it can work but, it will not return so use lcall
		"1:"
		::"m"(lcs));

	printf("normal test over\n");
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_003
 *
 * Summary: In 64-bit mode, segment selector index is not within descriptor table limits,
 * execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38650_cs_jmp_GP_table6_03()
{
	struct lseg_st64 lcs;

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = 0x2000;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0x2000),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_004
 *
 * Summary: In 64-bit mode, if the segment descriptor pointed to by the segment
 * selector in the destination operand is write read access data segment,
 * (If the segment descriptor pointed to by the segment selector in the destination operand
 * is not for a conforming-code segment, nonconforming-code segment, 64-bit call gate.)
 * execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_35277_cs_jmp_GP_table6_04()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	//normal case, use same CODE_SEL, can't open when test cases running.
	//normal_lcall_cs_test();

	sgdt(&old_gdt_desc);
	/* not for a conforming-code segment, nonconforming-code segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_005
 *
 * Summary: In 64-bit mode, the segment descriptor pointed to by the segment selector in
 * the destination operand is a code segment and has both the D-bit and the L-bit set,
 * execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38651_cs_jmp_GP_table6_05()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_006
 *
 * Summary: In 64-bit mode, the DPL for a conforming-code segment is greater than the CPL,
 * execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38652_cs_jmp_GP_table6_06()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*for a conforming-code segment, DPL is 3*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_CONFORMING,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_NP_table6_007
 *
 * Summary: In 64-bit mode, segment is not present for conforming segment
 * execute JMP will generate #NP(Destination code segment selector).
 */
static void segmentation_rqmid_38653_cs_jmp_NP_table6_07()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a conforming segment: SEGMENT_PRESENT_CLEAR */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_CONFORMING,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_008
 *
 * Summary: In 64-bit mode, target offset in destination operand is non-canonical
 * for a comforming segment, execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_38654_cs_jmp_GP_table6_08()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a conforming segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_CONFORMING,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)create_non_canonical_addr((u64)call_jmp_test_fun);
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_009
 *
 * Summary: In 64-bit mode, the RPL for the segment's segment selector is greater than
 * the CPL for non-conforming code, execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38655_cs_jmp_GP_table6_09()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a non-conforming segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL + 3; // RPL is 3, CPL is 0

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_010
 *
 * Summary: In 64-bit mode, the DPL for a nonconforming-code segment is not equal to the CPL,
 * for non-conforming code, execute JMP will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38656_cs_jmp_GP_table6_10()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a non-conforming segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL; // DPL is 3, CPL is 0

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_NP_table6_011
 *
 * Summary: In 64-bit mode, destination segment is not present for non-conforming code,
 * execute JMP will generate #NP(Destination code segment selector)
 */
static void segmentation_rqmid_38657_cs_jmp_NP_table6_11()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a non-conforming segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table6_12
 *
 * Summary: In 64-bit mode, target offset in destination operand is non-canonical
 * for non-conforming code, execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_38658_cs_jmp_GP_table6_12()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* for a non-conforming segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)create_non_canonical_addr((u64)call_jmp_test_fun);
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "ljmp *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/*Table 7*/
/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_001
 *
 * Summary: In 64-bit mode, memory operand effective address is non-canonical,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_35279_cs_call_GP_table7_01()
{
	u64 fun_p = (u64)call_jmp_test_fun;

	fun_p = (u64)create_non_canonical_addr(fun_p);

	asm volatile(ASM_TRY("1f")
		"call *%0\n\t"
		"1:"
		::"m"(fun_p));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_002
 *
 * Summary: In 64-bit mode, segment selector index is not within descriptor
 * table limits, execute CALL will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_35280_cs_call_GP_table7_02()
{
	struct lseg_st64 lcs;

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = SELECTOR_INDEX_2000H;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_03
 *
 * Summary: In 64-bit mode, if the segment descriptor pointed to by the segment selector
 * in the destination operand is not for a conforming-code segment, nonconforming-code segment,
 * 64-bit call gate , execute CALL will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38661_cs_call_GP_table7_03()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*not for a conforming-code segment, nonconforming-code segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table7_04
 *
 * Summary: In 64-bit mode, the segment descriptor pointed to by the segment selector
 * in the destination operand is a code segment and has both the D-bit and the L-bit set,
 * execute CALL will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38662_cs_call_GP_table7_04()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/*for a nonconforming-code segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/*Table 8*/
/**
 * @brief case name:segmentation_exception_check_call_GP_table8_001
 *
 * Summary: In 64-bit mode, the DPL for a conforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Destination code segment selector);
 */
static void segmentation_rqmid_35283_call_GP_table8_01()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment DPL =3 CPL=0
	 * The DPL for a conforming-code segment is greater than the CPL)
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_NP_table8_002
 *
 * Summary: In 64-bit mode, segment is not present, execute CALL
 * will generate #NP(Destination code segment selector).
 */
static void segmentation_rqmid_35284_call_NP_table8_02()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment is not present*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_GP_table8_003
 *
 * Summary: In 64-bit mode, target offset in destination operand is non-canonical,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_38664_call_GP_table8_03()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment is not present*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)create_non_canonical_addr((u64)call_jmp_test_fun);
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/* Table 9*/
/**
 * @brief case name:segmentation_exception_check_call_GP_table9_001
 *
 * Summary: In 64-bit mode, the RPL for a non-conforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_35286_call_GP_table9_01()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* non_conforming-code segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	/*RPL=3 CPL=0
	 * the RPL for the segment’s segment selector is greater than the CPL
	 */
	lcs.selector = (CODE_SEL) + 3;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_GP_table9_002
 *
 * Summary: In 64-bit mode, the DPL for a nonconforming-code segment is 3
 * and the CPL is 0, execute CALL will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_35288_call_GP_table9_02()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* non_conforming-code segment DPL=3 CPL=0
	 * the DPL for a nonconforming-code segment is not equal to the CPL
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_NP_table9_003
 *
 * Summary: In 64-bit mode, segment is not present,
 * execute CALL will generate #NP(Destination code segment selector).
 */
static void segmentation_rqmid_38668_call_NP_table9_03()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	//the segment is not present for a nonconforming-code segment
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)call_jmp_test_fun;
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_GP_table9_004
 *
 * Summary: In 64-bit mode, target offset in destination operand is non-canonical,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_38669_call_GP_table9_04()
{
	struct lseg_st64 lcs;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	lcs.offset = (u64)create_non_canonical_addr((u64)call_jmp_test_fun);
	lcs.selector = CODE_SEL;

	asm volatile(ASM_TRY("1f")
		"rex.W\n\t" "lcall *%0\n\t"
		"1:"
		::"m"(lcs));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}

/* Table 10*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_001
 *
 * Summary: In 64-bit mode, the DPL from a 64-bit call gate is 0 and RPL of the
 * 64-bit call gate is 3, CALL this call gate will generate #GP(Call Gate Selector).
 */
static void segmentation_rqmid_35322_call_gate_call_gp_table10_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	/*RPL = 3 CPL=0 DPL=0
	 * The DPL from a 64-bit call gate is less than the CPL
	 * or than the RPL of the 64-bit call gate.
	 */
	target_sel = ((CALL_GATE_SEL + 3) << 16);

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));


	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CALL_GATE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_NP_table10_002
 *
 * Summary: In 64-bit mode, 64-bit call gate not present,
 * CALL this call gate will generate #NP(Call Gate Selector).
 */
static void segmentation_rqmid_35323_call_gate_call_np_table10_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	/*64-bit call gate not present*/
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 0, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CALL_GATE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_003
 *
 * Summary: In 64-bit mode, call gate code-segment selector is NULL,
 * CALL this call gate will generate #GP(0).
 */
static void segmentation_rqmid_38671_call_gate_GP_table10_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	/*64-bit call gate code-segment selector is NULL*/
	init_call_gate_64(CALL_GATE_SEL, 0, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}


/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_004
 *
 * Summary: In 64-bit mode, call gate code-segment selector index outside descriptor table limits,
 * CALL this call gate will generate #GP(Call Gate Selector).
 */
static void segmentation_rqmid_38674_call_gate_GP_table10_04()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, SELECTOR_INDEX_2000H, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_005
 *
 * Summary: In 64-bit mode, the DPL for a code segment is greater than the CPL,
 * CALL this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38675_call_gate_GP_table10_05()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table10_006
 *
 * Summary: In 64-bit mode, the code segment descriptor pointed to by the selector
 * in the 64-bit gate doesn't have the L-bit set and the D-bit clear,
 * CALL this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38676_call_gate_GP_table10_06()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

/* Table 11 */


/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table11_003
 *
 * Summary: In 64-bit mode, if target offset in 64-bit call gate is non-canonical,
 * DPL of this call gate's CS is 0 and CPL is 3,
 * CALL this call gate will generate #GP(0).
 */
static void call_gate_table11_03(const char *args)
{
	int target_sel;
	target_sel = (CALL_GATE_SEL + 3) << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		args, err_code);
}

static void segmentation_rqmid_35324_call_gate_call_gp_table11_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	 //Note: if canonical address, and DPL = CPL, it is a normal case; if non-canonical address, it will GP
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 1, call_gate_ent);

	/* DPL of the nonconforming destination code segment
	 * pointed by call gate is less than the CPL.
	 * DPL =3 nonconforming
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	do_at_ring3(call_gate_table11_03, __FUNCTION__);
}

/* Table 12 */

#if 0 // it cann't be constructed
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table12_001
 *
 * Summary: In 64-bit mode, if the stack address is in a non-canonical form;
 * call gate & DPL of the destination code segment pointed by call gate is equal to the CPL,
 * CALL this call gate will generate #SS(0).
 */

void call_gate_table12_01(const char *args)
{
	int target_sel, ret;
	target_sel = (CALL_GATE_SEL + 1) << 16;

	asm volatile(ASM_TRY("1f")
		"lcall *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s ret=%d", (ret == SS_VECTOR), args, ret);
}

static void segmentation_rqmid_35324_call_gate_SS_table12_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	//setup a stack in ring1 is non-canical
	setup_tss(false);

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 1, 1, 0, call_gate_ent);

	/* DPL of the nonconforming destination code segment
	 * pointed by call gate equals the CPL. DPL = 1 nonconforming
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	//do_at_ring1(call_gate_table12_01, __FUNCTION__);

#if 1
	target_sel = (CALL_GATE_SEL + 1) << 16;
	asm volatile(ASM_TRY("1f")
		"lcall *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t %s ret=%d", (ret == SS_VECTOR), __FUNCTION__, ret);
#endif
}
#endif
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table12_002
 *
 * Summary: In 64-bit mode, if target offset in 64-bit call gate is non-canonical,
 * DPL of this call gate is 0 and CPL is 0,  CALL this call gate will generate #GP(0).
 */
static void segmentation_rqmid_35325_call_gate_call_gp_table12_02()
{
	int target_sel;
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* 64-bit call gate non-canonical address*/
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 1, call_gate_ent);

	/* call gate & DPL of the destination code segment pointed by
	 * call gate is equal to the CPL
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_16BIT_SEGMENT|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, err_code);
}


/* Table 13 */
/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_001
 *
 * Summary: In 64-bit mode, DPL of the 64-bit call gate is 0 and RPL is 3,
 * execute JMP with this call gate will generate #GP(call gate selector).
 */
static void segmentation_rqmid_35326_call_gate_jmp_gp_table13_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	/*RPL = 3 CPL=0 DPL=0
	 * The DPL from a 64-bit call gate is less than the CPL
	 * or than the RPL of the 64-bit call gate.
	 */
	target_sel = ((CALL_GATE_SEL + 3) << 16);

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CALL_GATE_SEL), __FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table13_002
 *
 * Summary: In 64-bit mode, 64-bit call gate not present,
 * execute JMP with this call gate will generate #NP(call gate selector).
 */
static void segmentation_rqmid_35328_call_gate_jmp_np_table13_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	/*64-bit call gate not present*/
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 0, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CALL_GATE_SEL), __FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_003
 *
 * Summary: In 64-bit mode, 64-bit call gate code-segment selector is NULL,
 * execute JMP with this call gate will generate #GP(0).
 */
static void segmentation_rqmid_38721_call_gate_jmp_gp_table13_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	/*0 CS -- NULL code selector*/
	init_call_gate_64(CALL_GATE_SEL, 0, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0), __FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_004
 *
 * Summary: In 64-bit mode, 64-bit call gate code segment selector index outside descriptor table limits,
 * execute JMP with this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38723_call_gate_jmp_gp_table13_04()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	/*0x500 out of GDT */
	init_call_gate_64(CALL_GATE_SEL, SELECTOR_INDEX_2000H, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_005
 *
 * Summary: In 64-bit mode, the DPL for a conforming-code segment is greater than the CPL,
 * execute JMP with this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38728_call_gate_jmp_gp_table13_05()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL), __FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_006
 *
 * Summary: In 64-bit mode, the DPL for a nonconforming-code segment is not equal to the CPL,
 * execute JMP with this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38729_call_gate_jmp_gp_table13_06()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL), __FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table13_007
 *
 * Summary: In 64-bit mode, the code segment descriptor pointed to by the selector
 * in the 64-bit gate doesn't have the L-bit set and the D-bit clear,
 * execute JMP with this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38730_call_gate_jmp_gp_table13_07()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL), __FUNCTION__, err_code);
}


/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table13_008
 *
 * Summary: In 64-bit mode, Destination code segment is not present,
 * execute JMP with this call gate will generate #NP(destination code segment selector).
 */
static void segmentation_rqmid_38731_call_gate_jmp_np_table13_08()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel;

	sgdt(&old_gdt_desc);

	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = CALL_GATE_SEL << 16;

	asm volatile(ASM_TRY("1f")
		"ljmpw *%0\n\t"
		"1:"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL), __FUNCTION__, err_code);
}

/* Table 14 */
static inline u64 read_esp(void)
{
	u64 val;
	asm volatile ("mov %%rsp, %0" : "=mr"(val));
	return val;
}

static u64 rsp_64;
static u64 *rsp_val;

void call_gate_function2(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	u64 new_rsp = (u64)create_non_canonical_addr(rsp_64);

	asm volatile("mov %0, %%rdi\n\t"
		::"m"(new_rsp)
		);
}

asm("call_gate_ent2: \n"
	"mov %rsp, %rdi\n"
	"mov %rsp, %rsi\n"

	"call call_gate_function2\n"
	"mov %rdi, %rsp\n"
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
	// resume stacks
	"mov %rsi, %rsp\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);
void call_gate_ent2(void);

static void test_ss_table14_02(const char *args)
{
	int target_sel, ret;
	target_sel = (CALL_GATE_SEL + 3) << 16;

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:%d", (ret == SS_VECTOR) && (err_code == 0), args, ret, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_ss_table14_002
 *
 * Summary: In 64-bit mode, an attempt to pop a value (Calling CS and Calling EIP)
 * off the stack causes a non-canonical address to be referenced,
 * execute IRET will generate #SS(0).
 */
static void segmentation_rqmid_38733_cs_ret_ss_table14_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent2);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_ss_table14_02, __FUNCTION__);
}

void call_gate_function3(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		rsp_64, rsp_val[1], rsp_val[0]);

	/*set the code segment selector at the top fo stack to NULL*/
	rsp_val[1] = 0;

	printf("changed: rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		rsp_64, rsp_val[1], rsp_val[0]);
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
 * Summary: In 64-bit mode, call to a function(name is call_gate_ent) by call gate,
 * if you modify the return code segment selector to NULL,
 * execute IRET will generate #GP(0).
 */
static void segmentation_rqmid_35329_cs_ret_gp_table14_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent3);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (CALL_GATE_SEL << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:%d", (ret == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, ret, err_code);
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
 * descriptor table limit, execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_35330_cs_ret_gp_table14_04()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent4);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (CALL_GATE_SEL << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == 0x2000),
		__FUNCTION__, ret, err_code);

}

void call_gate_function6(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	/*set the code segment selector to DS */
	rsp_val[1] = 0x10;
}

void call_gate_function6_1(void)
{
	/*resume code segment selecor at stack top*/
	rsp_val[1] = 0x8;
}

asm("call_gate_ent6:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call call_gate_function6\n"
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
	"call call_gate_function6_1\n"
	"lretq\n"
#endif
	);

void call_gate_ent6(void);

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table14_006
 *
 * Summary: In 64-bit mode, return code segment descriptor is not a code segment;
 * execute LRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38734_cs_ret_gp_table14_06()
{
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent6);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	target_sel = (CALL_GATE_SEL << 16);

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, ret:%d, err-code:0x%x", (ret == GP_VECTOR) && (err_code == 0x10),
		__FUNCTION__, ret, err_code);
}

//for table14-07
static u16 orig_cs;
void call_gate_function7(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		rsp_64, rsp_val[1], rsp_val[0]);

	orig_cs = rsp_val[1];
	/* USER_CS: 0x4b; set its RPL to 1, lower than CPL */
	rsp_val[1] = (rsp_val[1] & 0xFFF8) | 0x01;
}

void call_gate_function7_1(void)
{
	rsp_val[1] = orig_cs;
}

asm("call_gate_ent7: \n"
	"mov %rsp, %rdi\n"

	"call call_gate_function7\n"

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
	"1:\n"
	// resume CS
	"call call_gate_function7_1\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);
void call_gate_ent7(void);

static void test_ss_table14_07(const char *args)
{
	int target_sel;
	target_sel = (CALL_GATE_SEL + 3) << 16;

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == (orig_cs & 0xFFF8)),
		args, ret, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_gp_table14_007
 *
 * Summary: In 64-bit mode, the RPL of the return code segment selector is less than the CPL,
 * execute IRET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_38735_cs_ret_gp_table14_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent7);

	/*config code segment*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_ss_table14_07, __FUNCTION__);
}

//it call int ring3 kernel_entry_vector, same code with do_at_ring3, not need write here.
// before call modify func, it need save some registers
#define SAVE_REGS  \
			"push" W " %%rcx \n" \
			"push" W " %%r8 \n" \
			"push" W " %%r9 \n" \
			"push" W " %%r10 \n" \
			"push" W " %%rsi \n" \
			"push" W " %%rdi \n"

#define RESTORE_REGS  \
			"pop" W " %%rdi \n"  \
			"pop" W " %%rsi \n" \
			"pop" W " %%r10 \n" \
			"pop" W " %%r9 \n" \
			"pop" W " %%r8 \n" \
			"pop" W " %%rcx \n"

//if use save/restore, need add offset, if no, just set it 0
#define BASE_IRET_OFFSET 0

static int switch_to_ring3(void (*fn)(const char *), const char *arg, void (*modify)(void), void (*recovery)(void))
{
	static unsigned char user_stack[4096] __attribute__((aligned(16)));
	int ret;

	asm volatile("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
			//insert for modify something to do test, it need to save them
				  "mov %%rsp, %%rdi \n"
				  "call *%[modify] \n"
			//insert modfiy end
			#ifdef USE_HAND_EXECEPTION
				/*ASM_TRY("1f") macro expansion*/
				"movl $0, %%gs:"xstr(EXCEPTION_ADDR)" \n"
				".pushsection .data.ex \n"
				".quad 1111f,  111f \n"
				".popsection \n"
				"1111:\n"
			#endif
				  "iret" W "\n"
				  "1: \n\t"
				  /* save kernel SP */
				  "push %%" R "cx\n\t"
#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif
				  "pop %%" R "cx\n\t"
			#ifdef USE_HAND_EXECEPTION
				"jmp  2f\n"
				"111:\n"
				/*resume code segment selector or stack */
				"call *%[recovery]\n"
				"jmp  1f\n"		/* iret exception not need execution INT N*/
				"2:\n"
			#endif
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (USER_DS),
				  [user_cs] "i" (USER_CS),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [modify]"m"(modify),
				  [fn]"m"(fn),
				  [recovery]"m"(recovery),
				  [arg]"D"(arg),
				  [kernel_entry_vector]"i"(0x23)
				  : "rcx", "rdx");
	return ret;
}

/*Table 15*/
//Note: it can't be too complex, or it need save/recovery registers in ASM.
static void normal_ring3_call_func(const char *args)
{
	printf("Normal called in Ring3, CS=0x%x, SS=0x%x\n", read_cs(), read_ss());
}

static void modify_table15_1(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x, cs:0x%x\n", __FUNCTION__, read_ss(), read_cs());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	printf("set over %s to cs set D-bit\n", __FUNCTION__);
	orig_cs = rsp_val[1 + BASE_IRET_OFFSET];
	rsp_val[1 + BASE_IRET_OFFSET] = CODE_SEL + 3;
}

static void recovery_user_cs(void)
{
	//NOTE: if print in this function, call it need save some registers before call it.
	printf("recovery user-cs, called in Ring0: CS=0x%x\n", read_cs());

	rsp_val[1 + BASE_IRET_OFFSET] = orig_cs;
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_001
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is
 * larger than the CPL; return code segment descriptor has L-bit = 1 and D-bit = 1,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_35332_rpl_ret_gp_table15_01()
{
	//if not modify the USER_CS, the ring3 function will be called, and recovery will not be called.
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_1, recovery_user_cs);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, ret, err_code);
}

//For table15-2
static void modify_table15_2(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x, cs:0x%x\n", __FUNCTION__, read_ss(), read_cs());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	printf("set over %s to cs set D-bit\n", __FUNCTION__);
	orig_cs = rsp_val[1 + BASE_IRET_OFFSET];
	rsp_val[1 + BASE_IRET_OFFSET] = CODE_SEL + 1;
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_002
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is larger than the CPL;
 * the return code segment is conforming and the segment selectors DPL greater than the RPL of the
 * code segment segment selector, execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_35334_rpl_ret_gp_table15_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_2, recovery_user_cs);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, ret, err_code);
}

//for table15-3, test iret
void intr_gate_function15_3(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	/*set the code segment selector RPL at the top of stack to 2*/
	printf("original: rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		rsp_64, rsp_val[1], rsp_val[0]);

	orig_cs = rsp_val[1];

	rsp_val[1] = orig_cs | 0x3; //change the RPL not equal with its DPL
}

void intr_gate_function_end_15_3(void)
{
	/*resume code segment selecor at stack top*/
	rsp_val[1] = orig_cs;
}

asm("intr_gate_ent15_3:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call intr_gate_function15_3\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"iretq\n"
#ifdef USE_HAND_EXECEPTION
	"1:\n"
	/*resume code segment selector or stack*/
	"call intr_gate_function_end_15_3\n"
	"iretq\n"
#endif
	);

void intr_gate_ent15_3(void);

static void intr_gate_ring2_table15_03(const char *args)
{
	asm volatile("int $0x35\n\t");
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_003
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is
 * larger than the CPL; the return code segment is non-conforming and the segment
 * selector’s DPL is not equal to the RPL of the code segment’s segment selector,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38973_rpl_ret_gp_table15_03()
{
	set_idt_entry(0x35, intr_gate_ent15_3, 2);

	do_at_ring2(intr_gate_ring2_table15_03, NULL);
	/*ring2 CS register equal OSSERVISE2_CS32,
	 *init by setup_ring_env(non-conformning code segment)
	 */
	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == (orig_cs & 0xFF8)),
		__FUNCTION__, ret, err_code);
}

//for table15-4 to change its return CS
void call_func_table15_04(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;

	orig_cs = rsp_val[1];

	printf("to change cs\n");

	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	gdt_entry_t *gdt = (gdt_entry_t *)old_gdt_desc.base;
	gdt_entry_t *desc = gdt + (orig_cs >> 3);

	printf("current cs=0x%x returning-cs=0x%x, orig-access=0x%x\n", read_cs(), orig_cs, desc->access);

	desc->access &= 0x7F; //remove present bit
	lgdt(&old_gdt_desc);
}

void recovery_cs_table15_4(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	gdt_entry_t *gdt = (gdt_entry_t *)old_gdt_desc.base;
	gdt_entry_t *desc = gdt + (orig_cs >> 3);

	printf("recovery the returning CS\n");

	desc->access |= 0x80; //set present bit

	lgdt(&old_gdt_desc);
}

asm("call_gate_ent15: \n"
	"mov %rsp, %rdi\n"
	"mov %rsp, %rsi\n"
	"call call_func_table15_04\n"
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
	"1:\n"
	// resume code segment or stack
	"call recovery_cs_table15_4\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);
void call_gate_ent15(void);

static void test_table15_04(const char *args)
{
	int target_sel, ret;
	target_sel = (CALL_GATE_SEL + 3) << 16;

	printf("called in ring3, cs=0x%x\n", read_cs());

	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == NP_VECTOR) && (err_code == (orig_cs & 0xFF8)),
		args, ret, err_code);
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_np_table15_04
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is
 * larger than the CPL, return code segment descriptor is not present,
 * execute LRET wiill generate #NP(Return code segment selector).
 */
static void segmentation_rqmid_38974_rpl_ret_np_table15_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent15);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table15_04, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_ss_table15_06
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is larger than the CPL,
 * an attempt to pop a value (Calling CS and Calling EIP) off the stack causes a
 * non-canonical address to be referenced, execute LRET will generate #SS(0).
 */
static void segmentation_rqmid_38982_rpl_ret_ss_table15_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	setup_tss(true);

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent2);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	//reuse the table14-02, just the dest CS DPL is 1 for pre-condition
	do_at_ring3(test_ss_table14_02, __FUNCTION__);
}

//for table 15-7
static void modify_table15_7(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed to 0x00\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];
	rsp_val[4 + BASE_IRET_OFFSET] = 0x00;

	printf("set over %s to cs clear L-bit\n", __FUNCTION__);
	orig_cs = rsp_val[1 + BASE_IRET_OFFSET];
	rsp_val[1 + BASE_IRET_OFFSET] = CODE_SEL + 3;

	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);
}

static void recovery_table15_7(void)
{
	printf("call %s\n", __FUNCTION__);

	rsp_val[4 + BASE_IRET_OFFSET] = orig_user_ss;
	rsp_val[1 + BASE_IRET_OFFSET] = orig_cs;
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_007
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is larger
 * than the CPL; stack segment selector is NULL and new CS descriptor L-bit = 0,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38986_rpl_ret_gp_table15_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);

	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_7, recovery_table15_7);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, ret, err_code);
}

void modify_table15_9(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed to 0x03\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];
	rsp_val[4 + BASE_IRET_OFFSET] = 0x03;

}

static void not_need_recovery(void)
{
	printf("recovery nothing called in ring0: cs=0x%x ss=0x%x\n", read_cs(), read_ss());
}
/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_009
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is larger
 * than the CPL; stack segment selector is NULL and stack segment selector RPL = 3,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38987_rpl_ret_gp_table15_09(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_9, not_need_recovery);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, ret, err_code);
}

//for table15-10
static void modify_table15_10(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed to 0x0FF3\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];
	rsp_val[4 + BASE_IRET_OFFSET] = SELECTOR_INDEX_2000H + 3;
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_010
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is larger than the CPL;
 * return stack segment selector index is not within its descriptor table limits,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38991_rpl_ret_gp_table15_10(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_10, not_need_recovery);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, ret, err_code);
}

//for table15-12
static void modify_table15_12(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed to non-writable\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];

	struct descriptor_table_ptr old_gdt_desc;

	//set SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED, it is normal
	sgdt(&old_gdt_desc);
	set_gdt_entry(orig_user_ss, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);
}

static void recovery_user_ss(void)
{
	printf("recovery user (ring3) SS\n");

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry(orig_user_ss, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_012
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is
 * larger than the CPL; the return stack segment is not a writable data segment,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38993_rpl_ret_gp_table15_12(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_12, recovery_user_ss);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == (orig_user_ss & 0xFF8)),
		__FUNCTION__, ret, err_code);
}

//for table15-13
static void modify_table15_13(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed RPL to 1\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];

	rsp_val[4 + BASE_IRET_OFFSET] = (orig_user_ss & 0xFF8) | 1;
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_013
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector
 * is larger than the CPL; the return stack segment selector RPL is not
 * equal to the RPL of the return code segment selector,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38994_rpl_ret_gp_table15_13(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_13, not_need_recovery);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == (orig_user_ss & 0xFF8)),
		__FUNCTION__, ret, err_code);
}

//for table15-14
static void modify_table15_14(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed DPL to 1\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];

	struct descriptor_table_ptr old_gdt_desc;

	//set DESCRIPTOR_PRIVILEGE_LEVEL_3 is normal
	sgdt(&old_gdt_desc);
	set_gdt_entry(orig_user_ss, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);

}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_014
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector
 * is larger than the CPL; the return stack segment descriptor DPL is
 * not equal to the RPL of the return code segment selector,
 * execute IRET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_38996_rpl_ret_gp_table15_14(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_14, recovery_user_ss);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == (orig_user_ss & 0xFF8)),
		__FUNCTION__, ret, err_code);
}

//for table15-15
void modify_table15_15(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	printf("call %s, its ss:0x%x\n", __FUNCTION__, read_ss());

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx ss=0x%lx changed DPL to 1\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[4+BASE_IRET_OFFSET]);

	orig_user_ss = rsp_val[4 + BASE_IRET_OFFSET];

	struct descriptor_table_ptr old_gdt_desc;

	//set SEGMENT_PRESENT_SET is normal
	sgdt(&old_gdt_desc);
	set_gdt_entry(orig_user_ss, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET);
	lgdt(&old_gdt_desc);

}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_ss_table15_015
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector
 * is larger than the CPL; the return stack segment is not present,
 * execute IRET will generate #SS(return code segment selector).
 */
static void segmentation_rqmid_38997_rpl_ret_ss_table15_15(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_15, recovery_user_ss);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == SS_VECTOR) && (err_code == (orig_user_ss & 0xFF8)),
		__FUNCTION__, ret, err_code);
}

//for table15-16
static void modify_table15_16(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	printf("original: rsp_64=0x%lx cs=0x%lx EIP=0x%lx changed to non-canonical\n", rsp_64,
		rsp_val[1+BASE_IRET_OFFSET], rsp_val[0+BASE_IRET_OFFSET]);

	rsp_val[0 + BASE_IRET_OFFSET] = (u64)create_non_canonical_addr(rsp_val[0 + BASE_IRET_OFFSET]);
}

/**
 * @brief case name:segmentation_exception_check_rpl_ret_GP_table15_016
 *
 * Summary: In 64-bit ring 0 mode, RPL of the return code segment selector is larger
 * than the CPL; the return instruction pointer is not within canonical address space,
 * execute IRET will generate #GP(0).
 */
static void segmentation_rqmid_38998_rpl_ret_gp_table15_16(void)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_table15_16, not_need_recovery);

	u8 ret = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (ret == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, ret, err_code);
}

void call_gate_function15_17(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	printf("original: current-cs=0x%x, rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		read_cs(), rsp_64, rsp_val[1], rsp_val[0]);

	orig_cs = rsp_val[1];
	//set D/L in returning CS
	rsp_val[1] = CODE_2ND + 3;
}

void call_gate_function15_17_1(void)
{
	rsp_val[1] = orig_cs;
}

asm("call_gate_ent15_17: \n"
	"mov %rsp, %rdi\n"

	"call call_gate_function15_17\n"

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
	"1:\n"
	// resume CS
	"call call_gate_function15_17_1\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);
void call_gate_ent15_17(void);

static void test_table15_17(const char *args)
{
	printf("called in ring3, cs=0x%x\n", read_cs());

	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == (CODE_2ND & 0xFF8)),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_gp_table15_017
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is equal to the CPL,
 * return code segment descriptor has L-bit = 1 and D-bit = 1,
 * execute LRET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_39000_rpl_ret_gp_table15_17()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent15_17);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	//used to generate exception: L-bit = 1 and D-bit = 1
	set_gdt_entry(CODE_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table15_17, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_gp_table15_018
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is equal to the CPL,
 * the return code segment is non-conforming and the segment selector's DPL is
 * not equal to the RPL of the code segment's segment selector,
 * execute LRET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_39002_rpl_ret_gp_table15_18()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent15_17);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	//used to generate exception: set DPL not same with RPL
	set_gdt_entry(CODE_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table15_17, __FUNCTION__);
}


static void test_table15_19(const char *args)
{
	printf("called in ring3, cs=0x%x\n", read_cs());

	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == (CODE_2ND & 0xFF8)),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_np_table15_019
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is equal to the CPL;
 * return code segment descriptor is not present,
 * execute LRET will generate #NP(Return code segment selector).
 */
static void segmentation_rqmid_39003_rpl_ret_np_table15_19()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent15_17);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	//used to generate exception, set SEGMENT_PRESENT_CLEAR
	set_gdt_entry(CODE_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table15_19, __FUNCTION__);
}

static u64 orig_eip = 0;
void call_gate_function15_20(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		::"m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;
	printf("original: current-cs=0x%x, rsp_64=0x%lx cs=0x%lx offset/eip=0x%lx\n",
		read_cs(), rsp_64, rsp_val[1], rsp_val[0]);

	//EIP non-canonical
	orig_eip = rsp_val[0];
	rsp_val[0] = (u64)create_non_canonical_addr(rsp_val[0]);
}

void call_gate_function15_20_1(void)
{
	rsp_val[0] = orig_eip;
}

asm("call_gate_ent15_20: \n"
	"mov %rsp, %rdi\n"

	"call call_gate_function15_20\n"

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
	"1:\n"
	// resume CS
	"call call_gate_function15_20_1\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);
void call_gate_ent15_20(void);

static void test_table15_20(const char *args)
{
	printf("called in ring3, cs=0x%x\n", read_cs());

	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t"
		::"m"(target_sel));

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s ret:%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_gp_table15_020
 *
 * Summary: In 64-bit mode, RPL of the return code segment selector is equal to the CPL,
 * the return instruction pointer is not within canonical address space,
 * execute LRET will generate #GP(0).
 */
static void segmentation_rqmid_35335_rpl_ret_gp_table15_20()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent15_20);

	/*config code segment*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table15_20, __FUNCTION__);
}

static void print_case_list_64(void)
{
	printf("\t\t Segmentation feature 64-Bits Mode case list:\n\r");

	uint32_t cr0 = read_cr0();
	uint32_t efer = rdmsr(X86_IA32_EFER);

	printf("CR0: PE=%d, PG=%d; EFER: LMA=%d\n", (cr0 & X86_CR0_PE) ? 1 : 0,
		(cr0 & X86_CR0_PG) ? 1 : 0, (efer & X86_EFER_LMA) ? 1 : 0);
}

static void test_segmentation_64(void)
{
	/*x86_64__*/
	/* Table 1 */
	segmentation_rqmid_35235_lds_ud_table1_01();

	/* Table 2 */
	segmentation_rqmid_35236_lds_ud_table2_01();

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
	segmentation_rqmid_64_38553_ss_call_ss_table5_01();

	/* Table 6 */
	segmentation_rqmid_38645_cs_jmp_GP_table6_01();
	segmentation_rqmid_35276_cs_jmp_GP_table6_02();
	segmentation_rqmid_38650_cs_jmp_GP_table6_03();
	segmentation_rqmid_35277_cs_jmp_GP_table6_04();
	segmentation_rqmid_38651_cs_jmp_GP_table6_05();
	segmentation_rqmid_38652_cs_jmp_GP_table6_06();
	segmentation_rqmid_38653_cs_jmp_NP_table6_07();
	segmentation_rqmid_38654_cs_jmp_GP_table6_08();
	segmentation_rqmid_38655_cs_jmp_GP_table6_09();
	segmentation_rqmid_38656_cs_jmp_GP_table6_10();
	segmentation_rqmid_38657_cs_jmp_NP_table6_11();
	segmentation_rqmid_38658_cs_jmp_GP_table6_12();

	/* Table 7 */
	segmentation_rqmid_35279_cs_call_GP_table7_01();
	segmentation_rqmid_35280_cs_call_GP_table7_02();
	segmentation_rqmid_38661_cs_call_GP_table7_03();
	segmentation_rqmid_38662_cs_call_GP_table7_04();

	/* Table 8 */
	segmentation_rqmid_35283_call_GP_table8_01();
	segmentation_rqmid_35284_call_NP_table8_02();
	segmentation_rqmid_38664_call_GP_table8_03();

	/* Table 9 */
	segmentation_rqmid_35286_call_GP_table9_01();
	segmentation_rqmid_35288_call_GP_table9_02();
	segmentation_rqmid_38668_call_NP_table9_03();
	segmentation_rqmid_38669_call_GP_table9_04();

	/* Table 10 */
	segmentation_rqmid_35322_call_gate_call_gp_table10_01();
	segmentation_rqmid_35323_call_gate_call_np_table10_02();
	segmentation_rqmid_38671_call_gate_GP_table10_03();
	segmentation_rqmid_38674_call_gate_GP_table10_04();
	segmentation_rqmid_38675_call_gate_GP_table10_05();
	segmentation_rqmid_38676_call_gate_GP_table10_06();

	/* Table 11 */
	segmentation_rqmid_35324_call_gate_call_gp_table11_03();

	/* Table 12 */
	//segmentation_rqmid_35324_call_gate_SS_table12_01();
	segmentation_rqmid_35325_call_gate_call_gp_table12_02();

	/* Table 13 */
	segmentation_rqmid_35326_call_gate_jmp_gp_table13_01();
	segmentation_rqmid_35328_call_gate_jmp_np_table13_02();
	segmentation_rqmid_38721_call_gate_jmp_gp_table13_03();
	segmentation_rqmid_38723_call_gate_jmp_gp_table13_04();
	segmentation_rqmid_38728_call_gate_jmp_gp_table13_05();
	segmentation_rqmid_38729_call_gate_jmp_gp_table13_06();
	segmentation_rqmid_38730_call_gate_jmp_gp_table13_07();
	segmentation_rqmid_38731_call_gate_jmp_np_table13_08();

	/* Table 14 */
	segmentation_rqmid_38733_cs_ret_ss_table14_02();
	segmentation_rqmid_35329_cs_ret_gp_table14_03();
	segmentation_rqmid_35330_cs_ret_gp_table14_04();
	segmentation_rqmid_38734_cs_ret_gp_table14_06();
	segmentation_rqmid_38735_cs_ret_gp_table14_07();

	/*Table 15*/
	segmentation_rqmid_35332_rpl_ret_gp_table15_01();
	segmentation_rqmid_35334_rpl_ret_gp_table15_02();
	segmentation_rqmid_38973_rpl_ret_gp_table15_03();
	segmentation_rqmid_38974_rpl_ret_np_table15_04();
	segmentation_rqmid_38982_rpl_ret_ss_table15_06();
	segmentation_rqmid_38986_rpl_ret_gp_table15_07();
	segmentation_rqmid_38987_rpl_ret_gp_table15_09();
	segmentation_rqmid_38991_rpl_ret_gp_table15_10();
	segmentation_rqmid_38993_rpl_ret_gp_table15_12();
	segmentation_rqmid_38994_rpl_ret_gp_table15_13();
	segmentation_rqmid_38996_rpl_ret_gp_table15_14();
	segmentation_rqmid_38997_rpl_ret_ss_table15_15();
	segmentation_rqmid_38998_rpl_ret_gp_table15_16();
	segmentation_rqmid_39000_rpl_ret_gp_table15_17();
	segmentation_rqmid_39002_rpl_ret_gp_table15_18();
	segmentation_rqmid_39003_rpl_ret_np_table15_19();
	segmentation_rqmid_35335_rpl_ret_gp_table15_20();
}

