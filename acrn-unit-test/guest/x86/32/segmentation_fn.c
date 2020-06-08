__attribute__((aligned(4096))) static char ring1_stack[4096];

#define CALL_GATE_SEL (0x31<<3)
#define CODE_SEL	(0x30<<3)
#define DATA_SEL	(0x32<<3)
#define STACK_SEL	(0x33<<3)

static void init_call_gate
	(u32 index, u32 code_selector, u8 type, u8 dpl, u8 p, int copy, call_gate_fun fun)
{
	int num = index>>3;
	u32 call_base = (u32)fun;
	struct gate_descriptor *call_gate_entry = NULL;

	call_gate_entry = (struct gate_descriptor *)&gdt32[num];
	call_gate_entry->gd_looffset = call_base&0xFFFF;
	call_gate_entry->gd_hioffset = (call_base >> 16)&0xFFFF;
	call_gate_entry->gd_stkcpy = copy;
	call_gate_entry->gd_selector = code_selector;
	call_gate_entry->gd_xx = UNUSED;
	call_gate_entry->gd_type = type;
	call_gate_entry->gd_dpl = dpl;
	call_gate_entry->gd_p = p;

	u64 *point = (u64 *)&gdt32[num];
	printf("32 bit call gate %llx\n", (u64)*point);
}

static inline u32 read_esp(void)
{
	u32 val;
	asm volatile ("mov %%esp, %0" : "=mr"(val));
	return val;
}

void call_gate_function(void)
{
	debug_print("ss=0x%x ss0=0x%x esp0=0x%x esp=0x%x\n",
		read_ss(), tss.ss0, tss.esp0, read_esp());
	printf("call_gate_function ok\n");
}

asm("call_gate_ent: \n\t"
	"mov %esp, %edi \n\t"
	"call call_gate_function\n\t"
	"lret");
void call_gate_ent(void);

/* Table 16 */
/**
 * @brief case name:segmentation_exception_check_lds_ud_table1_001
 *
 * Summary: In protected mode, if segment selector is NULL,
 * execute LFS will normal execution.
 */
static void segmentation_rqmid_35338_lfs_normal_table16_01()
{

	struct lseg_st lfs;

	lfs.offset = 0xffffffff;
	/*index =0*/
	lfs.selector = 0x0;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(lfs)
	);
	report("\t\t %s", true, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_ss_gp_table4_003
 *
 * Summary: In protected mode, the segment selector index is not within the
 * descriptor table limit, execute LFS will generate #GP(Segmentation Selector)
 */
static void segmentation_rqmid_35339_lfs_gp_table16_02()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = lfs_index_1024;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 17 */
/**
 * @brief case name:segmentation_exception_check_lss_GP_table17_001
 *
 * Summary: In protected mode, the segment selector RPL is 3 and CPL is 0,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35340_lss_gp_table17_01()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	set_gdt_entry((0x30<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lfs_rpl_3_index_48;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lss_GP_table17_002
 *
 * Summary: In protected mode, the segment is a non-writable data segment,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35341_lss_gp_table17_02()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* non-writeable data segment */
	set_gdt_entry((0x30<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	fun1 = lss_index_48;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 18 */
void mov_to_lfs_offset(void *data)
{
	asm volatile("mov %eax, %fs:0xffff\n\t");
}

/**
 * @brief case name:segmentation_exception_check_mov_GP_table18_001
 *
 * Summary: In protected mode, a memory operand effective address is
 * outside the FS segment limit, execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_35342_mov_gp_table18_01()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* limit set 0 */
	set_gdt_entry((0x30<<3), 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lfs_index_48(NULL);

	fun1 = mov_to_lfs_offset;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_mov_GP_table18_002
 *
 * Summary: In protected mode, write a read-only data segment,
 * execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_35343_mov_gp_table18_02()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* read only data segment */
	set_gdt_entry((0x30<<3), 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lfs_index_48(NULL);

	fun1 = mov_to_lfs_offset;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 20 */
void call_gate_function20_1(void *data)
{
	/* Move doubleword at (cs:64M) to eax*/
	asm volatile("mov %cs:0x4000000, %eax\n\t");
}

int test_ret_20 = 0;
void call_gate_function20(void)
{
	trigger_func fun1;

	fun1 = call_gate_function20_1;
	test_ret_20 = test_for_exception(GP_VECTOR, fun1, NULL);
}

/**
 * @brief case name:segmentation_exception_check_cs_mov_GP_table20_001
 *
 * Summary: In protected mode, a memory operand effective address
 * is outside the CS segment limit, execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_35348_cs_mov_gp_table20_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_function20);
	/*limit set to 32M*/
	set_gdt_entry(CODE_SEL, 0, 0x1FFF, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");

	report("\t\t %s", (test_ret_20 == true), __FUNCTION__);
}

void mov_to_cs_16M(void *data)
{
	/* Move eax to (cs:16M)*/
	asm volatile("mov %eax, %cs:0x1000000\n\t");
}
/**
 * @brief case name:segmentation_exception_check_cs_mov_GP_table20_002
 *
 * Summary: In protected mode, use MOV writing to a code segment, will generate #GP(0).
 */
static void segmentation_rqmid_35349_cs_mov_gp_table20_02()
{
	int ret1 = 0;
	trigger_func fun1;

	fun1 = mov_to_cs_16M;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	report("\t\t %s", (ret1 == true), __FUNCTION__);
}

/* Table 21 */
/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table21_001
 *
 * Summary: In protected mode, a memory operand effective address
 * is outside the CS segment limit, execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_35350_cs_jmp_gp_table21_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);
	/*limit set to 0*/
	set_gdt_entry(CODE_SEL, 0, 0x0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $0xf4f4f4f4\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table21_002
 *
 * Summary: In protected mode, segment selector in target operand is NULL,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_35351_cs_jmp_gp_table21_02()
{
	int ret1;

	/*segment selector in target operand is NULL*/
	asm volatile(ASM_TRY("1f")
		"ljmp $0, $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/* Table 22 */
/**
 * @brief case name:segmentation_exception_check_conforming_jmp_GP_table22_001
 *
 * Summary: In protected mode, the DPL for a conforming-code segment is 3 and CPL is 0,
 * execute JMP will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_35353_conforming_jmp_gp_table22_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);

	/*conforming-code segment DPL = 3*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_NP_table22_002
 *
 * Summary: In protected mode, segment is not present,
 * execute JMP will generate #NP(Segment selector in target operand).
 */
static void segmentation_rqmid_35354_conforming_jmp_np_table22_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);

	/*conforming-code segment, not present*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == NP_VECTOR), __FUNCTION__);
}

/* Table 23 */
/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_001
 *
 * Summary: In protected mode, a memory operand effective address
 * is outside the CS segment limit, execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_35362_cs_call_gp_table23_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);
	/*limit set to 0*/
	set_gdt_entry(CODE_SEL, 0, 0x0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $0xf4f4f4f4\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_002
 *
 * Summary: In protected mode, segment selector in target operand is NULL,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_35363_cs_call_gp_table23_02()
{
	int ret1;

	/*segment selector in target operand is NULL*/
	asm volatile(ASM_TRY("1f")
		"lcall $0, $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/* Table 24 */
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table24_001
 *
 * Summary: In protected mode, the DPL from a call-gate is 0 and RPL is 3,
 * execute CALL with this call-gate will generate #GP(call gate selector).
 */
static void segmentation_rqmid_35364_call_gate_call_gp_table24_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);
	/*  DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == GP_VECTOR), __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_NP_table24_002
 *
 * Summary: In protected mode, a call gate is not present,
 * execute call with this call gate will generate #NP(call gate selector).
 */
static void segmentation_rqmid_35365_call_gate_call_np_table24_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret1;

	sgdt(&old_gdt_desc);
	/*not present*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 0, 0, call_gate_ent);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	ret1 = exception_vector();
	report("\t\t %s", (ret1 == NP_VECTOR), __FUNCTION__);
}

/* Table 25 */
int test_ret_25 = 0;
void call_ring3_function25_1(const char *data)
{
	//debug_print("ring3 curr_ss=0x%x ss=0x%x ss0=0x%x ss1=0x%x ss2=0x%x\n", read_ss(),
	//	tss.ss, tss.ss0, tss.ss1, tss.ss2);
	//debug_print("ring3 esp=0x%x esp0=0x%x esp1=0x%x esp2=0x%x\n",
	//	tss.esp, tss.esp0, tss.esp1, tss.esp2);

	/* index = 1024 */
	tss.ss1 = 0x2001;

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	test_ret_25 = exception_vector();
}
/*#TS(error_code=0x2000)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_001
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1
 * and CPL is 3, the new stack segment selector and ESP are beyond the end of the TSS,
 * execute call with this call gate will generate #NP(call gate selector).
 */
static void segmentation_rqmid_35395_call_gate_call_ts_table25_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function25_1, "");

	report("\t\t %s", (test_ret_25 == TS_VECTOR), __FUNCTION__);
}

void call_ring3_function25_2(const char *data)
{
	/* ring1 ss1 = NULL selector */
	tss.ss1 = 0x0;

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	test_ret_25 = exception_vector();
}
/*TS(error_code=0x0000)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1
 * CPL is 3, the new stack segment selector is NULL,
 * execute call with this call gate will generate #NP(New SS).
 */
static void segmentation_rqmid_35396_call_gate_call_ts_table25_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function25_2, "");

	report("\t\t %s", (test_ret_25 == TS_VECTOR), __FUNCTION__);
}

/* Table 26 */
int test_ret_26 = 0;
void call_ring3_function26_1_2(const char *data)
{
	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	test_ret_26 = exception_vector();
}

/*#SS(error_code=0x0198)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_SS_table26_001
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1
 * and CPL is 3, stack segment does not have room for the return address,
 * parameters, or stack segment pointer, when stack switch occurs,
 * execute call with this call gate will generate #SS(New SS)
 */
static void segmentation_rqmid_35399_call_gate_call_ss_table26_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment limit set to 0(0-4095 Bytes)*/
	set_gdt_entry(STACK_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function26_1_2, "");

	report("\t\t %s", (test_ret_26 == SS_VECTOR), __FUNCTION__);
}

/*#GP(0)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table26_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1 and
 * CPL is 3, if target offset in a call gate is beyond the destination code segment limit,
 * execute call with this call gate will generate #GP(0)
 */
static void segmentation_rqmid_35400_call_gate_call_gp_table26_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 limit set to 0(0-4095 Bytes) */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function26_1_2, "");

	report("\t\t %s", (test_ret_26 == GP_VECTOR), __FUNCTION__);
}

/* Table 27 */
int test_ret_27 = 0;
void call_ring3_function27_1(const char *data)
{
	asm volatile("mov $" xstr(STACK_SEL+3) ",%ax\n\t"
		"mov %ax, %ss\n\t"
	);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		//"1:\n\t"
		//"mov $0x43, %ax\n\t"
		//"mov %ax, %ss\n\t"
		"1:"::);

	test_ret_27 = exception_vector();
}

/*#SS(0)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_SS_table27_001
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 3
 * and CPL is 3, stack segment does not have room for the return address,
 * parameters, or stack segment pointer, when stack switch occurs,
 * execute call with this call gate will generate #SS(0)
 */
static void segmentation_rqmid_35401_call_gate_call_ss_table27_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	debug_print("ss=0x%x ss0=0x%x esp0=0x%x esp=0x%x\n",
		read_ss(), tss.ss0, tss.esp0, read_esp());

	/* ring 3 stack segment limit set to 0(0-4095 Bytes)  */
	set_gdt_entry(STACK_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(call_ring3_function27_1, "");

	report("\t\t %s", (test_ret_27 == SS_VECTOR), __FUNCTION__);
}

void call_ring3_function27_2(const char *data)
{
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	test_ret_27 = exception_vector();
}

/*#GP(0)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table27_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination
 * code segment is 3 and CPL is 3, If target offset in a call
 * gate is beyond the destination code segment limit
 * execute call with this call gate will generate #GP(0)
 */
static void segmentation_rqmid_35402_call_gate_call_gp_table27_02()
{
	gdt_entry_t ring3_code_bak = gdt32[7];
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 , destination offset is 3G*/
	init_call_gate(CALL_GATE_SEL, (7<<3), 0xC, 0x3, 1, 0, (call_gate_fun)0xC0000000);

	/*ring 3 code segment DPL = 3 limit is 0x7FFFF [0-2G)*/
	set_gdt_entry((7<<3), 0, 0x3FFFF, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	debug_print("ss=0x%x ss0=0x%x esp0=0x%x esp=0x%x\n",
		read_ss(), tss.ss0, tss.esp0, read_esp());

	lgdt(&old_gdt_desc);

	do_at_ring3(call_ring3_function27_2, "");

	/* resume environment */
	gdt32[7] = ring3_code_bak;
	lgdt(&old_gdt_desc);

	report("\t\t %s", (test_ret_27 == GP_VECTOR), __FUNCTION__);
}

/* Table 28 */
int test_ret_28 = 0;
void jmp_ring3_function28_1(const char *data)
{
	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	test_ret_28 = exception_vector();
}

/* Table 28 */
/*#GP(error_code=0x0188)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_001
 *
 * Summary: In protected mode, the DPL from a call-gate is 0 and CPL 3,
 * execute call with this call gate will generate #GP(Call gate selector)
 */
static void segmentation_rqmid_35403_call_gate_jmp_gp_table28_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 1, 0, call_gate_ent);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(jmp_ring3_function28_1, "");

	report("\t\t %s", (test_ret_28 == GP_VECTOR), __FUNCTION__);
}

/*#NP(error_code=0x0188)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table28_002
 *
 * Summary: In protected mode, call gate not present,
 * execute call with this call gate will generate #NP(Call gate selector)
 */
static void segmentation_rqmid_35539_call_gate_jmp_np_table28_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 not present*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 0, 0, call_gate_ent);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	test_ret_28 = exception_vector();
	report("\t\t %s", (test_ret_28 == NP_VECTOR), __FUNCTION__);
}

u32 esp_32;
u32 *esp_val;
u32 esp_val_bak;

/* Table 29*/
void call_gate_function29_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);

	/*set the code segment selector at the top fo stack to NULL*/
	esp_val[1] = 0;

	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);
}

void call_gate_function29_2_end(void)
{
	/*resume code segment selecor at stack top*/
	esp_val[1] = 0x8;
}

asm("call_gate_ent29_2:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function29_2\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function29_2_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent29_2(void);

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table29_002
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * if you modify the return code segment selector to NULL,
 * execute RET will generate #GP(0)
 */
static void segmentation_rqmid_35540_cs_ret_gp_table29_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 0*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent29_2);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/* remove exception vector */
	asm volatile("movl $0, %gs:4 \n");

	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

void call_gate_function29_3(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);
	esp_val = (u32 *)esp_32;
	/*set the code segment selector at the top of stack to 1k*/
	esp_val[1] = 0x2000;
}

void call_gate_function29_3_end(void)
{
	/*resume code segment selecor at stack top*/
	esp_val[1] = 0x8;
}

asm("call_gate_ent29_3:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function29_3\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function29_3_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent29_3(void);

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table29_003
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * if you modify the return code segment selector addresses descriptor beyond
 * descriptor table limit, execute RET will generate #GP(return code segment selector)
 */
static void segmentation_rqmid_35541_cs_ret_gp_table29_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 0*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent29_3);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/* remove exception vector */
	asm volatile("movl $0, %gs:4 \n");

	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

/* Table 30*/
void call_gate_function30_1(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	esp_val[1] = (esp_val[1]&0xFFFC);

	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);
}

void call_gate_function30_1_end(void)
{
	/*resume the return code segment selecor RPL to 3*/
	esp_val[1] = (esp_val[1]&0xFFFC) + 0x3;
}

asm("call_gate_ent30_1:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function30_1\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function30_1_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent30_1(void);

void call_ring3_function30_1(const char *data)
{
	asm volatile("lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t");
}

/* #GP(error_code=0x0038)*/
/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table30_001
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * the RPL of the return code segment selector is 0 and the the CPL is 3,
 * execute RET will generate #GP(Return code segment selector)
 */
static void segmentation_rqmid_35542_cs_ret_gp_table30_01()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent30_1);

	/*code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(call_ring3_function30_1, "");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

void call_gate_function30_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	esp_val[1] = (esp_val[1]&0xFFFC);

	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);
}

void call_gate_function30_2_end(void)
{
	/*resume code segment selecor RPL to 3*/
	esp_val[1] = (esp_val[1]&0xFFFC) + 0x3;
}

asm("call_gate_ent30_2:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function30_2\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function30_2_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent30_2(void);

void call_ring3_function30_2(const char *data)
{
	asm volatile("lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t");
}

/* #GP(error_code=0x0038)*/
/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table30_001
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * the return code segment is conforming and the segment selector’s DPL is 3
 * and the RPL of the code segment’s segment selector is 0,
 * execute RET will generate #GP(Return code segment selector)
 */
static void segmentation_rqmid_35543_cs_ret_gp_table30_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent30_2);

	/*target code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* The return code segment is ring3 code segment(index = 7)
	 * set this segment to conforming(DPL default is 3)
	 */
	set_gdt_entry(0x38, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(call_ring3_function30_2, "");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

/* Table 31*/
void call_gate_function31_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x esp_val[2]=0x%x esp_val[3]=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0], esp_val[2], esp_val[3]);

	/*set the stack selector to NULL*/
	esp_val_bak = esp_val[3];
	esp_val[3] = 0;

	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x esp_val[2]=0x%x esp_val[3]=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0], esp_val[2], esp_val[3]);
}

void call_gate_function31_2_end(void)
{
	/*resume the return code segment selecor RPL to 1*/
	esp_val[3] = esp_val_bak;
}

asm("call_gate_ent31_2:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function31_2\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function31_2_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent31_2(void);

void call_ring3_function31_2(const char *data)
{
	asm volatile("lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t");
	printf(" ring 1 exec success\n");
}

/* #GP(0)*/
/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_002
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * RPL of the return code segment selector is 3 and CPL is 1,
 * set Stack segment selector is NULL,
 * execute RET will generate #GP(0)
 */
static void segmentation_rqmid_35544_rpl_ret_gp_table31_02()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent31_2);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	do_at_ring3(call_ring3_function31_2, "");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

void call_gate_function31_3(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x esp_val[2]=0x%x esp_val[3]=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0], esp_val[2], esp_val[3]);

	/*set the stack selector index to 1024*/
	esp_val_bak = esp_val[3];
	esp_val[3] = 0x2000 + 3;

	debug_print("esp_32=0x%x cs=0x%x 0x%x offset=0x%x esp_val[2]=0x%x esp_val[3]=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0], esp_val[2], esp_val[3]);
}

void call_gate_function31_3_end(void)
{
	/*resume the return code segment selecor RPL to 1*/
	esp_val[3] = esp_val_bak;
}

asm("call_gate_ent31_3:\n"
	/*save stack*/
	"mov %esp, %edi\n"
	"mov %ebp, %esi\n"
	"call call_gate_function31_3\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:4 \n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"call call_gate_function31_3_end\n"
	/* resume stacks */
	"mov %edi, %esp\n"
	"mov %esi, %ebp\n"
	/*return to normal code stream*/
	"lret\n"
#endif
	);

void call_gate_ent31_3(void);

void call_ring3_function31_3(const char *data)
{
	asm volatile("lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t");
	printf(" ring 1 exec success\n");
}

/* #GP(error_code=0x2000)*/
/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_003
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * RPL of the return code segment selector is 3 and CPL is 1
 * return stack segment selector index is not within its descriptor table limits
 * execute RET will generate #GP(Return stack segment selector)
 */
static void segmentation_rqmid_35546_rpl_ret_gp_table31_03()
{
	struct descriptor_table_ptr old_gdt_desc;
	int ret;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent31_3);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	do_at_ring3(call_ring3_function31_3, "");

	ret = exception_vector();
	report("\t\t %s", (ret == GP_VECTOR), __FUNCTION__);
}

static void print_case_list_32(void)
{
	printf("\t\t Segmentation feature 32-Bits Mode case list:\n\r");
}

static void test_segmentation_32(void)
{
	int branch = 0;
	/*386*/
	switch (branch) {
	case 0:
		/* Table 16 */
		segmentation_rqmid_35338_lfs_normal_table16_01();
		segmentation_rqmid_35339_lfs_gp_table16_02();

		/* Table 17 */
		segmentation_rqmid_35340_lss_gp_table17_01();
		segmentation_rqmid_35341_lss_gp_table17_02();

		/* Table 18 */
		segmentation_rqmid_35342_mov_gp_table18_01();
		segmentation_rqmid_35343_mov_gp_table18_02();

		/* Table 19 */

		/* Table 20 */
		segmentation_rqmid_35348_cs_mov_gp_table20_01();
		segmentation_rqmid_35349_cs_mov_gp_table20_02();

		/* Table 21 */
		segmentation_rqmid_35350_cs_jmp_gp_table21_01();
		segmentation_rqmid_35351_cs_jmp_gp_table21_02();

		/* Table 22*/
		segmentation_rqmid_35353_conforming_jmp_gp_table22_01();
		segmentation_rqmid_35354_conforming_jmp_np_table22_02();

		/* Table 23 */
		segmentation_rqmid_35362_cs_call_gp_table23_01();
		segmentation_rqmid_35363_cs_call_gp_table23_02();

		/* Table 24 */
		segmentation_rqmid_35364_call_gate_call_gp_table24_01();
		segmentation_rqmid_35365_call_gate_call_np_table24_02();

		/* Table 25 */
		segmentation_rqmid_35395_call_gate_call_ts_table25_01();
		segmentation_rqmid_35396_call_gate_call_ts_table25_02();

		/* Table 26 */
		segmentation_rqmid_35399_call_gate_call_ss_table26_01();
		segmentation_rqmid_35400_call_gate_call_gp_table26_02();

		/* Table 27 */
		segmentation_rqmid_35402_call_gate_call_gp_table27_02();

		/* Table 28 */
		segmentation_rqmid_35403_call_gate_jmp_gp_table28_01();
		segmentation_rqmid_35539_call_gate_jmp_np_table28_02();

		/* Table 29 */
		segmentation_rqmid_35540_cs_ret_gp_table29_02();
		segmentation_rqmid_35541_cs_ret_gp_table29_03();

		/* Table 30 */
		segmentation_rqmid_35542_cs_ret_gp_table30_01();
		segmentation_rqmid_35543_cs_ret_gp_table30_02();

		/* Table 31 */
		segmentation_rqmid_35544_rpl_ret_gp_table31_02();
		segmentation_rqmid_35546_rpl_ret_gp_table31_03();
		break;

	case 1:
		/* Table 27*/
		segmentation_rqmid_35401_call_gate_call_ss_table27_01();	//can't catch
		break;

	default:
		break;
	}
	printf("___________test over__________________\n");
}
