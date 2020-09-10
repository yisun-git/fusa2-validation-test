
#define CALL_GATE_SEL (0x31<<3)
#define CODE_SEL	(0x30<<3)
#define DATA_SEL	(0x32<<3)
#define STACK_SEL	(0x33<<3)

#define CODE_2ND	(0x36<<3)
#define DATA_2ND	(0x38<<3)
#define STACK_2ND	(0x39<<3)

static u32 esp_32;
static u32 *esp_val;
static u16 orig_user_cs;
static u16 orig_user_ss;

static char ring1_stack[4096] __attribute__((aligned(4096)));

static inline u32 read_esp(void)
{
	u32 val;
	asm volatile ("mov %%esp, %0" : "=mr"(val));
	return val;
}

void call_gate_function(void)
{
	printf("normal call gate func: cs=0x%x ss=0x%x esp=0x%x\n", read_cs(), read_ss(), read_esp());
}

asm("call_gate_ent: \n\t"
	"mov %esp, %edi \n\t"
	"call call_gate_function\n\t"
	"lret");
void call_gate_ent(void);


void default_gate_function(void)
{
	printf("this function shall be replaced by your target func, if necessary!\n");
}

void (*call_gate_tigger_exception)(void) = default_gate_function;
void (*call_gate_restore)(void) = default_gate_function;

asm("call_gate_exception:\n"
	"mov %esp, %edi \n\t"
	"call *call_gate_tigger_exception\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:\n"
	/*resume code segment selector on stack top*/
	"call *call_gate_restore\n"
	"lret\n"
#endif
	);

void call_gate_exception(void);

static void set_call_gate_funcs(void (*trigger)(void), void (*restore)(void))
{
	call_gate_tigger_exception = trigger;
	call_gate_restore = restore;
}

static void restore_call_gate_funcs(void)
{
	call_gate_tigger_exception = default_gate_function;
	call_gate_restore = default_gate_function;
}

//it call int ring1 kernel_entry_vector, same code with do_at_ring1, not need write here.
//it is different with 64bits code
static int switch_to_ring1(void (*fn)(const char *), const char *arg, void (*recovery)(void))
{
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "mov %[user_ss], %%" R "dx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
			#ifdef USE_HAND_EXECEPTION
				/*ASM_TRY("1f") macro expansion*/
				"movl $0, %%gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
				".pushsection .data.ex \n"
				".long 1111f,  111f \n"
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
			#ifdef USE_HAND_EXECEPTION
				"jmp  2f\n"
				"111:\n"
				/*resume code segment selector or stack */
				"call *%[recovery]\n"
				"2:\n"
			#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (OSSERVISE1_DS),
				  [user_cs] "i" (OSSERVISE1_CS32),
				  [user_ss] "i" (STACK_SEL | 1),
				  [user_stack_top]"m"(ring1_stack[sizeof ring1_stack]),
				  [fn]"r"(fn),
				  [recovery]"r"(recovery),
				  [arg]"D"(arg),
				  [kernel_entry_vector]"i"(0x21)
				  : "rcx", "rdx");

	return ret;
}

//for call gate init
static void init_call_gate(u32 index, u32 code_selector, u8 type, u8 dpl,
	u8 p, int copy, call_gate_fun fun)
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
	printf("init 32 bit call gate: %p, func entry: %p\n", point, fun);
}

static void setup_tss_ring1(u32 stack_base)
{
	struct descriptor_table_ptr gdt;
	struct segment_desc *gdt_table;
	struct segment_desc *tss_entry;

	u32 p = (u32)(ring1_stack + 4096) - stack_base;
	u16 tr = 0;
	tss32_t *tss;
	u32 tss_base;

	sgdt(&gdt);
	tr = str();
	gdt_table = (struct segment_desc *) gdt.base;
	tss_entry = &gdt_table[tr / sizeof(struct segment_desc)];
	tss_base = (u32) tss_entry->base1 | ((u32) tss_entry->base2 << 16) |
			((u32) tss_entry->base3 << 24);
	tss = (tss32_t *)tss_base;

	printf("orig tr=0x%x, tss esp0=0x%x, esp1=0x%x, ss1=0x%x\n", tr, tss->esp0, tss->esp1, tss->ss1);
	tss->esp1 = p;
	tss->ss1 = STACK_SEL | 1;
	printf("tss=%p fixed to tss->esp1=0x%x, ss1=0x%x\n", tss, tss->esp1, tss->ss1);

}


/* Table 16 */

//table16-1: LES/LFS/LGS can load NULL selector, LDS/LSS can't, it will cause execeptions
void test_table16_1(const char *args)
{
	struct lseg_st lfs;

	printf("ds=0x%x, ss=0x%x\n", read_ds(), read_ss());

	lfs.offset = 0xffffffff;
	/*index =0*/
	lfs.selector = SELECTOR_INDEX_0H;

	asm volatile(
		"lds  %0, %%eax\t\n"
		::"m"(lfs)
	);

	printf("over\n");
}

/**
 * @brief case name:segmentation_exception_check_lfs_normal_table16_001
 *
 * Summary: In protected mode, if segment selector is NULL,
 * execute LFS will normal execution.
 */
static void segmentation_rqmid_35338_lfs_normal_table16_01()
{
	struct lseg_st seg;

	seg.offset = 0xffffffff;
	/*index =0*/
	seg.selector = SELECTOR_INDEX_0H;

	asm volatile(
		"lfs  %0, %%eax\t\n"
		::"m"(seg)
	);

	report("\t\t %s", true, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_lfs_gp_table16_02
 *
 * Summary: In protected mode, the segment selector index is not within the
 * descriptor table limit, execute LFS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35339_lfs_gp_table16_02()
{
	bool ret1 = test_for_exception(GP_VECTOR, lfs_index_1024, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, err_code);
}

static void test_table16_3(void *args)
{
	struct descriptor_table_ptr old_gdt_desc;

	//normal type: DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,

	sgdt(&old_gdt_desc);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = CODE_SEL;

	asm volatile(
		"lgs  %0, %%eax\t\n"
		::"m"(seg)
	);
}

/**
 * @brief case name:segmentation_exception_check_lgs_gp_table16_03
 *
 * Summary: In protected mode, the segment is neither a data nor a readable code segment,
 * execute LGS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_38761_lgs_gp_table16_03()
{
	bool ret1 = test_for_exception(GP_VECTOR, test_table16_3, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == CODE_SEL),
		__FUNCTION__, err_code);
}

//for table16-4, do at ring1
static void test_table16_4(void *args)
{
	printf("%s, current cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = DATA_SEL | 1;

	asm volatile(
		"les  %0, %%eax\t\n"
		::"m"(seg)
	);
}

static void test_table16_4_2(const char *args)
{
	bool ret1 = test_for_exception(GP_VECTOR, test_table16_4, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == DATA_SEL),
		args, err_code);
}

/**
 * @brief case name:segmentation_exception_check_les_gp_table16_04
 *
 * Summary: In protected mode, the segment is a data or nonconforming-code
 * segment and both RPL and CPL are greater than DPL,
 * execute LES will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_38762_les_gp_table16_04()
{
	struct descriptor_table_ptr old_gdt_desc;
	//CPL = 1, RPL = 1, DPL = 0; if DESCRIPTOR_PRIVILEGE_LEVEL_1 normal
	sgdt(&old_gdt_desc);
	set_gdt_entry(DATA_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	do_at_ring1(test_table16_4_2, __FUNCTION__);
}

static void test_table16_5(void *args)
{
	printf("%s, current cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	struct descriptor_table_ptr old_gdt_desc;

	//normal: SEGMENT_PRESENT_SET
	sgdt(&old_gdt_desc);
	set_gdt_entry(DATA_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = DATA_SEL;

	asm volatile(
		"lds  %0, %%eax\t\n"
		::"m"(seg)
	);
}

/**
 * @brief case name:segmentation_exception_check_lds_gp_table16_05
 *
 * Summary: In protected mode, the segment is marked not present,
 * execute LDS will generate #NP(Segmentation Selector).
 */
static void segmentation_rqmid_38763_lds_np_table16_05()
{
	bool ret1 = test_for_exception(NP_VECTOR, test_table16_5, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == DATA_SEL),
		__FUNCTION__, err_code);

}

/* Table 17 */
static void test_table17_1(void *args)
{
	printf("%s, current cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	struct descriptor_table_ptr old_gdt_desc;

	//CPL = 0, DPL = 0, RPL = 3
	sgdt(&old_gdt_desc);
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = STACK_SEL | 3;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(seg)
	);
}

/**
 * @brief case name:segmentation_exception_check_lss_GP_table17_001
 *
 * Summary: In protected mode, the segment selector RPL is 3 and CPL is 0,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35340_lss_gp_table17_01()
{
	bool ret1 = test_for_exception(GP_VECTOR, test_table17_1, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == STACK_SEL),
		__FUNCTION__, err_code);
}

static void test_table17_2(void *args)
{
	printf("%s, current cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	struct descriptor_table_ptr old_gdt_desc;

	//normal: SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED
	sgdt(&old_gdt_desc);
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = STACK_SEL;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(seg)
	);
}

/**
 * @brief case name:segmentation_exception_check_lss_GP_table17_002
 *
 * Summary: In protected mode, the segment is a non-writable data segment,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_35341_lss_gp_table17_02()
{
	bool ret1 = test_for_exception(GP_VECTOR, test_table17_2, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == STACK_SEL),
		__FUNCTION__, err_code);
}

static void test_table17_3(void *args)
{
	printf("%s, current cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	struct descriptor_table_ptr old_gdt_desc;

	//CPL = 0, DPL = 1
	sgdt(&old_gdt_desc);
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	struct lseg_st seg;
	seg.offset = 0xffffffff;
	seg.selector = STACK_SEL;

	asm volatile(
		"lss  %0, %%eax\t\n"
		::"m"(seg)
	);
}

/**
 * @brief case name:segmentation_exception_check_lss_GP_table17_003
 *
 * Summary: In protected mode, the segment selector DPL is 1 and CPL is 0,
 * execute LSS will generate #GP(Segmentation Selector).
 */
static void segmentation_rqmid_38766_lss_gp_table17_03()
{
	bool ret1 = test_for_exception(GP_VECTOR, test_table17_3, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == STACK_SEL),
		__FUNCTION__, err_code);
}

/* Table 18 */
static void mov_to_lfs_offset(void *data)
{
	asm volatile("mov %eax, %fs:0xffff\n\t");
}

/**
 * @brief case name:segmentation_exception_check_mov_GP_table18_001
 *
 * Summary: In protected mode, a memory operand effective address is
 * outside the FS segment limit, execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_35342_mov_gp_table18_01()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* limit set 0 */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lfs_index_48(NULL);

	fun1 = mov_to_lfs_offset;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == 0),
		__FUNCTION__, err_code);
}

/**
 * @brief case name:segmentation_exception_check_mov_GP_table18_002
 *
 * Summary: In protected mode, write a read-only data segment,
 * execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_35343_mov_gp_table18_02()
{
	int ret1 = 0;
	trigger_func fun1;

	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* read only data segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	lfs_index_48(NULL);

	fun1 = mov_to_lfs_offset;
	ret1 = test_for_exception(GP_VECTOR, fun1, NULL);

	u16 err_code = exception_error_code();
	report("\t\t %s, err_code:0x%x", (ret1 == true) && (err_code == 0),
		__FUNCTION__, err_code);
}

//for table-19

void test_table19_1(void)
{
#if 1 // for exception trigger, pop to out its limit
//	u8 stack[4090]; // if use non-zero SS base, it can use this way
//	printf("stack: %p\n", stack);

	//for POP to operate invalid sp
	u32 stack_limit = ((u32)ring1_stack >> 12) + 3;
	u32 out_limit = ((stack_limit << 12) + 0x100 - (u32)ring1_stack) / 4;

	//use pop to operate stack to out of limit
	u32 i = 0;
	for (i = 0; i < out_limit; i++) {
		asm volatile("pop %ecx\n"); //if not pop until stack out of limit, it work normal
	}
#else // for normal code
//	asm("push %esi\n");
	printf("normal function in ring1, cs=0x%x, ss=0x%x, esp=0x%x\n", read_cs(),
		read_ss(), read_esp());
//	asm("pop %esi\n");
#endif
}


asm("call_gate_exception_19_1:\n"
	/*save stack*/
	"mov %esp, %esi\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f, 1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"mov 0x100(%esp), %edi\n"  //it is used to operate an addr out of esp limit
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"mov %esi, %esp\n"
	"lret\n"
#endif
	);

void call_gate_exception_19_1(void);

static void call_gate_in_ring3(const char *args)
{
	asm volatile("lcall $" xstr(CALL_GATE_SEL + 3) ", $0x0\n\t");
}

static void test_table19_1_2(const char *args)
{
	printf("to call gate: %s cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	asm volatile("lcall $" xstr(CALL_GATE_SEL + 3) ", $0x0\n\t");

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_call_ss_table19_001
 *
 * Summary: In protected mode, a memory operand effective address is outside
 * the SS segment limit, call with stack switch will generate #SS(0).
 */
static void segmentation_rqmid_35344_ss_call_ss_table19_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	//normal: call gate: DPL=3, cs: DPL=1, RPL=3
	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_exception_19_1);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack, stack_limit + 1 is normal
	u32 stack_base = (u32)(ring1_stack); // use  non-0 base, the printf can't work in its func
	u32 stack_limit = 4096; //

	/* ring 1 stack segment, size-limit unit byte, not 4KB normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	printf("set ss base 0x%x, limit: %d * 4KB\n", stack_base, stack_limit);
	setup_tss_ring1(stack_base);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table19_1_2, __FUNCTION__);
}

//for table19-2
static void test_table19_2(const char *args)
{
	printf("to call gate: %s, cs=0x%x, ss=0x%x\n", __FUNCTION__, read_cs(), read_ss());

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:\n\t"
		::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == (STACK_SEL & 0xFF8)),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_call_ss_table19_02
 *
 * Summary: In protected mode, stack segment does not have room for the return
 * address, parameters, or stack segment pointer, when stack switch occurs,
 * call with stack switch will generate #SS(selector).
 */
static void segmentation_rqmid_38774_ss_call_ss_table19_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	//normal: call gate: DPL=3, cs: DPL=1, RPL=3
	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack, stack_limit + 1 is normal, -1 is small space, no space to save return/params
	u32 stack_base = 0; //(u32)(ring1_stack);
	u32 stack_limit = ((u32)ring1_stack >> 12) - 1;

	/* ring 1 stack segment, normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(stack_base);

	lgdt(&old_gdt_desc);

	do_at_ring3(test_table19_2, __FUNCTION__);
}

//for table19-3
asm("call_gate_exception_19_3:\n"
	/*save stack*/
	"mov %esp, %esi\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f, 1f \n"
	".popsection \n"
	"1111:\n"
#endif
	//"call call_gate_function\n" // a normal function
	"mov 0x2000(%esp), %edi\n"  //it is used to operate an addr out of esp limit
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"mov %esi, %esp\n"
	"lret\n"
#endif
	);

void call_gate_exception_19_3(void);

static void test_table19_3(const char *args)
{
	printf("to call gate: %s, current cs=0x%x, ss=0x%x, esp=0x%x\n", __FUNCTION__,
		read_cs(), read_ss(), read_esp());

	asm volatile("lcall $" xstr(CALL_GATE_SEL + 1) ", $0x0\n\t");

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_call_ss_table19_03
 *
 * Summary: In protected mode, a memory operand effective address is
 * outside the SS segment limit; call without stack switch,
 * call gate without stack switch it will generate #SS(0).
 */
static void segmentation_rqmid_38778_ss_call_ss_table19_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	//normal: call gate: DPL=3, cs: DPL=1, RPL=3
	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 1, 1, 0, call_gate_exception_19_3);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack
	u32 stack_base = 0; //(u32)(ring1_stack);
	u32 stack_limit = ((u32)ring1_stack >> 12) + 1;

	printf("set ring1 ds:0x%x, base:0x%x, limit=%d * 4KB\n", STACK_SEL, stack_base, stack_limit);
	/* ring 1 stack segment, normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(stack_base);

	lgdt(&old_gdt_desc);

	//switch_to_ring1, for stack setting
	switch_to_ring1(test_table19_3, __FUNCTION__, default_gate_function);
}

//for table19-4
static void test_table19_4(const char *args)
{
	printf("to call gate: %s, current cs=0x%x, ss=0x%x, esp=0x%x\n", __FUNCTION__,
		read_cs(), read_ss(), read_esp());

	asm volatile("mov %esp, %esi\n"
		"mov $0x04, %esp\n"  //make esp no space to use
		);
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+1) ", $0x0\n"
		"1:"::);
	asm volatile("mov %esi, %esp\n");

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_ss_call_ss_table19_04
 *
 * Summary: In protected mode, stack segment does not have room for the
 * return address, parameters, or stack segment pointer, when stack switch occurs,
 * call gate without stack switch it will generate #SS(0).
 */
static void segmentation_rqmid_38779_ss_call_ss_table19_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	//normal: call gate: DPL=3, cs: DPL=1, RPL=3
	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 1, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack
	u32 stack_base = 0; //(u32)(ring1_stack);
	u32 stack_limit = ((u32)ring1_stack >> 12) + 1;

	printf("set ring1 ds:0x%x, base:0x%x, limit=%d * 4KB\n", STACK_SEL, stack_base, stack_limit);
	/* ring 1 stack segment, normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	switch_to_ring1(test_table19_4, __FUNCTION__, default_gate_function);
}

/* Table 20 */
asm("call_gate_exception_20_1:\n"
	/*save stack*/
	"mov %esp, %esi\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f, 1f \n"
	".popsection \n"
	"1111:\n"
#endif
	//"call call_gate_function\n" // a normal function
	"mov %cs:0x4000000, %eax\n"  //it is used to out of CS limit
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"mov %esi, %esp\n"
	"lret\n"
#endif
	);

void call_gate_exception_20_1(void);

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
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_exception_20_1);
	/*limit set to 32M*/
	set_gdt_entry(CODE_SEL, 0, 0x1FFF, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	printf("befroe call gate, cs=0x%x\n", read_cs());
	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");
	printf("after call gate, cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void mov_to_cs_16M(void *data)
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
	test_for_exception(GP_VECTOR, mov_to_cs_16M, NULL);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

//for table20-3
asm("call_gate_exception_20_3:\n"
	/*save stack*/
	"mov %esp, %esi\n"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f, 1f \n"
	".popsection \n"
	"1111:\n"
#endif
	//"call call_gate_function\n" // a normal function
	"mov %cs:0x100000, %eax\n"  //it is used to access not-readonly code
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"mov %esi, %esp\n"
	"lret\n"
#endif
	);

void call_gate_exception_20_3(void);

/**
 * @brief case name:segmentation_exception_check_cs_mov_GP_table20_003
 *
 * Summary: In protected mode, reading from an execute-only code segment,
 * execute MOV will generate #GP(0).
 */
static void segmentation_rqmid_38780_cs_mov_gp_table20_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_exception_20_3);

	//set code to access/exe, can't be read, normal: SEGMENT_TYPE_CODE_EXE_READ_ACCESSED
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	printf("befroe call gate, cs=0x%x\n", read_cs());
	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");
	printf("after call gate, cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
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

	sgdt(&old_gdt_desc);
	/*limit set to 0, if SEGMENT_LIMIT_ALL, it will jump target func */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table21_002
 *
 * Summary: In protected mode, segment selector in target operand is NULL,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_35351_cs_jmp_gp_table21_02()
{
	/*segment selector in target operand is NULL*/
	asm volatile(ASM_TRY("1f")
		"ljmp $0, $call_gate_function\n\t"
		"1:"::);

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table21_003
 *
 * Summary: In protected mode, segment selector index is not within descriptor table limits,
 * execute JMP will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_38797_cs_jmp_gp_table21_03()
{
	/*segment selector in target operand is not within descriptor table limits*/
	asm volatile(ASM_TRY("1f")
		"ljmp $0x2000, $call_gate_function\n\t"
		"1:"::);

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_jmp_GP_table21_004
 *
 * Summary: In protected mode, segment type is neither a code segment nor call gate,
 * execute JMP will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_38801_cs_jmp_gp_table21_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* segment type is neither a code segment nor call gate,
	 * normal DESCRIPTOR_TYPE_CODE_OR_DATA, it will jump into target.
	 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_SYS|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/* Table 22 */
/**
 * @brief case name:segmentation_exception_check_conforming_jmp_GP_table22_001
 *
 * Summary: In protected mode, the DPL for a conforming-code segment is 3 and CPL is 0,
 * execute JMP will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_35353_conforming_jmp_gp_table22_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment DPL = 3 or it will jump into the target func, if DPL=0, or no-conforming code */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_NP_table22_002
 *
 * Summary: In protected mode, segment selector for a conforming code segment; segment is not present,
 * execute JMP will generate #NP(Segment selector in target operand).
 */
static void segmentation_rqmid_35354_conforming_jmp_np_table22_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment, not present*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_GP_table22_003
 *
 * Summary: In protected mode, segment selector for a conforming code segment;
 * offset in target operand is beyond the code segment limits,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_38802_conforming_jmp_gp_table22_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/*conforming-code segment, limit to 0, normal: SEGMENT_LIMIT_ALL*/
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_GP_table22_005
 *
 * Summary: In protected mode, segment selector for a non-conforming code segment;
 * the RPL for the segment's segment selector is greater than the CPL,
 * execute JMP will generate #GP(Target Segment selector).
 */
static void segmentation_rqmid_38803_non_conforming_jmp_gp_table22_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* RPL is greater than the CPL */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL + 1) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_non-conforming_jmp_GP_table22_006
 *
 * Summary: In protected mode, segment selector for a non-conforming code segment;
 * the DPL for a nonconforming-code segment is not equal to the CPL,
 * execute JMP will generate #GP(Target Segment selector).
 */
static void segmentation_rqmid_38806_non_conforming_jmp_gp_table22_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* DPL is greater than the CPL */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_NP_table22_007
 *
 * Summary: In protected mode, segment selector for a non-conforming code segment;
 * Segment is not present, execute JMP will generate #NP(Target Segment selector).
 */
static void segmentation_rqmid_38807_non_conforming_jmp_np_table22_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* Segment is not present */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_conforming_jmp_GP_table22_008
 *
 * Summary: In protected mode, segment selector for a non-conforming code segment,
 * offset in target operand is beyond the code segment limits,
 * execute JMP will generate #GP(0).
 */
static void segmentation_rqmid_38808_non_conforming_jmp_gp_table22_08()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* limit to 0, normal: SEGMENT_LIMIT_ALL */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
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

	sgdt(&old_gdt_desc);
	/* limit to 0, normal: SEGMENT_LIMIT_ALL */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);

}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_002
 *
 * Summary: In protected mode, segment selector in target operand is NULL,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_35363_cs_call_gp_table23_02()
{
	/*segment selector in target operand is NULL*/
	asm volatile(ASM_TRY("1f")
		"lcall $0, $call_gate_function\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_003
 *
 * Summary: In protected mode, segment selector after a conforming code segment;
 * if the DPL for a conforming-code segment is greater than the CPL,
 * execute CALL will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_38809_cs_call_gp_table23_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	// the DPL for a conforming-code segment is greater than the CPL
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);

}

/**
 * @brief case name:segmentation_exception_check_cs_call_NP_table23_004
 *
 * Summary: In protected mode, segment selector after a conforming code segment and
 * Segment is not present, execute CALL will generate #NP(Segment selector in target operand).
 */
static void segmentation_rqmid_38811_cs_call_np_table23_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	// Segment is not present
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x\n", read_cs());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);

}

//for table23-5
static void test_table23_5(const char *args)
{
	printf("%s: current cs=0x%x, ss=0x%x, esp=0x%x\n", __FUNCTION__,
		read_cs(), read_ss(), read_esp());

	asm volatile("mov %esp, %esi\n"
		"mov $0x04, %esp\n"  //make esp no space to use
		);
	asm volatile(ASM_TRY("1f")
		"call $" xstr(CODE_SEL+1) ", $call_gate_ent\n\t"
		"1:"::);
	asm volatile("mov %esi, %esp\n");

	printf("after exception, cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_SS_table23_005
 *
 * Summary: In protected mode, segment selector after a conforming code segment;
 * stack segment does not have room for the return address, parameters, or stack
 * segment pointer, when stack switch occurs, execute CALL will generate #SS(0).
 */
static void segmentation_rqmid_38116_cs_call_ss_table23_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 1, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack
	u32 stack_base = 0; //(u32)(ring1_stack);
	u32 stack_limit = ((u32)ring1_stack >> 12) + 2;

	printf("set ring1 ds:0x%x, base:0x%x, limit=%d * 4KB\n", STACK_SEL, stack_base, stack_limit);
	/* ring 1 stack segment, normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	switch_to_ring1(test_table23_5, __FUNCTION__, default_gate_function);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_006
 *
 * Summary: In protected mode, segment selector after a conforming code segment;
 * offset in target operand is beyond the code segment limits,
 * execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_38823_cs_call_gp_table23_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	//code segment limits set to 0, normal: SEGMENT_LIMIT_ALL
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_007
 *
 * Summary: In protected mode, segment selector after a non-conforming code segment;
 * the RPL for the segment's segment selector is greater than the CPL,
 * execute CALL will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_38824_cs_call_gp_table23_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	//RPL > CPL
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL + 1) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_008
 *
 * Summary: In protected mode, segment selector after a non-conforming code segment;
 * the DPL for a nonconforming-code segment is not equal to the CPL,
 * execute CALL will generate #GP(Segment selector in target operand).
 */
static void segmentation_rqmid_38825_cs_call_gp_table23_08()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	//DPL > CPL
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_NP_table23_009
 *
 * Summary: In protected mode, segment selector after a non-conforming code segment;
 * Segment is not present, execute CALL will generate #NP(Segment selector in target operand).
 */
static void segmentation_rqmid_38826_cs_call_np_table23_09()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	//Segment is not present.
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_SS_table23_010
 *
 * Summary: In protected mode, segment selector after a non-conforming code segment;
 * stack segment does not have room for the return address, parameters, or stack
 * segment pointer, when stack switch occurs, execute CALL will generate #SS(0).
 */
static void segmentation_rqmid_38828_cs_call_ss_table23_10()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//calc ring1 stack
	u32 stack_base = 0; //(u32)(ring1_stack);
	u32 stack_limit = ((u32)ring1_stack >> 12) + 2;

	printf("set ring1 ds:0x%x, base:0x%x, limit=%d * 4KB\n", STACK_SEL, stack_base, stack_limit);
	/* ring 1 stack segment, normal: 0 -- SEGMENT_LIMIT_ALL */
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	switch_to_ring1(test_table23_5, __FUNCTION__, default_gate_function);
}

/**
 * @brief case name:segmentation_exception_check_cs_call_GP_table23_011
 *
 * Summary: In protected mode, segment selector after a non-conforming code segment; offset
 * in target operand is beyond the code segment limits, execute CALL will generate #GP(0).
 */
static void segmentation_rqmid_38829_cs_call_gp_table23_11()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	//limit set 0, normal SEGMENT_LIMIT_ALL
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CODE_SEL) ", $call_gate_function\n\t"
		"1:\n\t" :: );

	printf("current cs=0x%x, ss=0x%x\n", read_cs(), read_ss());

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
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

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CALL_GATE_SEL),
		__FUNCTION__, vector, err_code);
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

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CALL_GATE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table24_003
 *
 * Summary: In protected mode, Call gate code-segment selector is NULL,
 * execute call with this call gate will generate #GP(0).
 */
static void segmentation_rqmid_38832_call_gate_call_gp_table24_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* no code selector */
	init_call_gate(CALL_GATE_SEL, 0, 0xC, 0, 1, 0, call_gate_ent);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table24_004
 *
 * Summary: In protected mode, Call gate code-segment selector index outside descriptor table
 * limits, execute call with this call gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38833_call_gate_call_gp_table24_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* code-segment selector index outside descriptor table limits */
	init_call_gate(CALL_GATE_SEL, SELECTOR_INDEX_2000H, 0xC, 0, 1, 0, call_gate_ent);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table24_005
 *
 * Summary: In protected mode, the segment descriptor for selector
 * in a call gate does not indicate it is a code segment, execute CALL
 * with this call-gate will generate #GP(destination code segment selector).
 */
static void segmentation_rqmid_38834_call_gate_call_gp_table24_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* type: DESCRIPTOR_TYPE_SYS */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_SYS|SEGMENT_TYPE_DATE_READ_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_NP_table24_006
 *
 * Summary: In protected mode, Destination code segment is not present,
 * execute CALL with this call-gate will generate #NP(destination code segment selector).
 */
static void segmentation_rqmid_38835_call_gate_call_np_table24_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_ent);

	//SEGMENT_PRESENT_CLEAR
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/* Table 25 */
static void call_ring3_function25_1(const char *args)
{
	debug_print("ring3 curr_ss=0x%x ss=0x%x ss0=0x%x ss1=0x%x ss2=0x%x\n", read_ss(),
		tss.ss, tss.ss0, tss.ss1, tss.ss2);

	/* index = 1024 */
	tss.ss1 = 0x2001;

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == TS_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_001
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1
 * and CPL is 3, the new stack segment selector and ESP are beyond the end of the TSS,
 * execute call with this call gate will generate #TS(call gate selector).
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
	do_at_ring3(call_ring3_function25_1, __FUNCTION__);
}

static void call_ring3_function25_2(const char *args)
{
	/* ring1 ss1 = NULL selector */
	tss.ss1 = 0x0;

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == TS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1
 * CPL is 3, the new stack segment selector is NULL,
 * execute call with this call gate will generate #TS(New SS).
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
	do_at_ring3(call_ring3_function25_2, __FUNCTION__);
}

static void call_ring3_function25_3(const char *args)
{
	tss.ss1 = STACK_SEL+3;

	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == TS_VECTOR) && (err_code == STACK_SEL),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_003
 *
 * Summary: In protected mode, Call gate & DPL of the nonconforming destination code
 * segment is less than the CPL; the RPL of the new stack segment selector in the TSS
 * is not equal to the DPL of the code segment being accessed,
 * execute call with this call gate will generate #TS(New SS).
 */
static void segmentation_rqmid_38839_call_gate_call_ts_table25_03()
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
	do_at_ring3(call_ring3_function25_3, __FUNCTION__);
}

static void call_ring3_function25_4(const char *args)
{
	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == TS_VECTOR) && (err_code == STACK_SEL),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_004
 *
 * Summary: In protected mode, Call gate & DPL of the nonconforming destination
 * code segment is less than the CPL; DPL of the stack segment descriptor for the
 * new stack segment is not equal to the DPL of the code segment descriptor,
 * execute call with this call gate will generate #TS(New SS).
 */
static void segmentation_rqmid_38840_call_gate_call_ts_table25_04()
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
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function25_4, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_TS_table25_005
 *
 * Summary: In protected mode, Call gate & DPL of the nonconforming destination code
 * segment is less than the CPL; the new stack segment is not a writable data segment,
 * execute call with this call gate will generate #TS(New SS).
 */
static void segmentation_rqmid_38841_call_gate_call_ts_table25_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment: SEGMENT_TYPE_DATE_READ_ONLY, normal: SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function25_4, __FUNCTION__);
}

static void call_ring3_function25_6(const char *args)
{
	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == STACK_SEL),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_SS_table25_006
 *
 * Summary: In protected mode, Call gate & DPL of the nonconforming destination
 * code segment is less than the CPL; The new stack segment is not present,
 * execute call with this call gate will generate #SS(New SS).
 */
static void segmentation_rqmid_38842_call_gate_call_ss_table25_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment: SEGMENT_PRESENT_CLEAR, normal: SEGMENT_PRESENT_SET*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	/*ring1 stack selector*/
	tss.ss1 = STACK_SEL+1;
	tss.esp1 = (u32)ring1_stack + 4096;

	/* CPL = 3 */
	do_at_ring3(call_ring3_function25_6, __FUNCTION__);
}

/* Table 26 */
static void call_ring3_function26_1(const char *args)
{
	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == STACK_SEL),
		args, vector, err_code);
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
	do_at_ring3(call_ring3_function26_1, __FUNCTION__);
}

static void call_ring3_function26_2(const char *args)
{
	/* RPL = 3 */
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table26_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 1 and
 * CPL is 3, if target offset in a call gate is beyond the destination code segment limit,
 * execute call with this call gate will generate #GP(0).
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
	do_at_ring3(call_ring3_function26_2, __FUNCTION__);
}

/* Table 27 */
static void call_ring3_function27_1(const char *args)
{
	asm volatile("mov $" xstr(STACK_SEL+3) ",%ax\n\t"
		"mov %ax, %ss\n\t"
	);

	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:\n\t"
		"mov $" xstr(USER_DS)", %%ax\n\t"
		"mov %%ax, %%ss\n\t"
		::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/*#SS(0)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_call_SS_table27_001
 *
 * Summary: In protected mode, DPL of the nonconforming destination code segment is 3
 * and CPL is 3, stack segment does not have room for the return address,
 * parameters, or stack segment pointer, when stack switch occurs,
 * execute call with this call gate will generate #SS(0).
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

	/* ring 3 stack segment limit set to 0(0-4095 Bytes)  */
	set_gdt_entry(STACK_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	debug_print("TSS: ss=0x%x ss0=0x%x esp0=0x%x esp=0x%x\n",
		read_ss(), tss.ss0, tss.esp0, read_esp());

	do_at_ring3(call_ring3_function27_1, __FUNCTION__);
}

static void call_ring3_function27_2(const char *args)
{
	asm volatile(ASM_TRY("1f")
		"lcall $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_call_GP_table27_002
 *
 * Summary: In protected mode, DPL of the nonconforming destination
 * code segment is 3 and CPL is 3, if target offset in a call
 * gate is beyond the destination code segment limit,
 * execute call with this call gate will generate #GP(0).
 */
static void segmentation_rqmid_35402_call_gate_call_gp_table27_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);

	/*code segment DPL = 1 limit set to 0(0-4095 Bytes) */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	do_at_ring3(call_ring3_function27_2, __FUNCTION__);
}

/* Table 28 */
static void jmp_ring3_function28_1(const char *args)
{
	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CALL_GATE_SEL),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_001
 *
 * Summary: In protected mode, the DPL from a call-gate is 0 and CPL 3,
 * execute jmp with this call gate will generate #GP(Call gate selector).
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

	do_at_ring3(jmp_ring3_function28_1, __FUNCTION__);
}

/*#NP(error_code=0x0188)*/
/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table28_002
 *
 * Summary: In protected mode, call gate not present,
 * execute jmp with this call gate will generate #NP(Call gate selector).
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

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CALL_GATE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_003
 *
 * Summary: In protected mode, call gate code-segment selector is NULL,
 * execute jmp with this call gate will generate #GP(0).
 */
static void segmentation_rqmid_38843_call_gate_jmp_gp_table28_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0, code selector = 0 */
	init_call_gate(CALL_GATE_SEL, 0, 0xC, 0x0, 1, 0, call_gate_ent);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_004
 *
 * Summary: In protected mode, call gate code-segment selector index
 * outside descriptor table limits, execute jmp with this call
 * gate will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38844_call_gate_jmp_gp_table28_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0, code selector = 0x2000 */
	init_call_gate(CALL_GATE_SEL, SELECTOR_INDEX_2000H, 0xC, 0x0, 1, 0, call_gate_ent);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_005
 *
 * Summary: In protected mode, the segment descriptor for selector in a call gate
 * does not indicate it is a code segment, execute jmp with this call gate
 * will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38845_call_gate_jmp_gp_table28_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 1, 0, call_gate_ent);
	/*code segment DPL = 0, it is not a code-segment, set it data segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_006
 *
 * Summary: In protected mode, the DPL for a conforming-code segment is greater than the CPL,
 * execute jmp with this call gate will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38846_call_gate_jmp_gp_table28_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 1, 0, call_gate_ent);
	/*code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

static void jmp_ring3_function28_7(const char *args)
{
	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL+3) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == CODE_SEL),
		args, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_007
 *
 * Summary: In protected mode, the DPL for a nonconforming-code segment is not equal to the CPL,
 * execute jmp with this call gate will generate #GP(Destination code segment selector).
 */
static void segmentation_rqmid_38847_call_gate_jmp_gp_table28_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_ent);
	/*code segment DPL = 1 not equal CPL3*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);
	setup_tss_ring1(0);

	do_at_ring3(jmp_ring3_function28_7, __FUNCTION__);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_NP_table28_008
 *
 * Summary: In protected mode, destination code segment is not present,
 * execute jmp with this call gate will generate #NP(Destination code segment selector).
 */
static void segmentation_rqmid_38849_call_gate_jmp_np_table28_08()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 1, 0, call_gate_ent);
	/*code segment DPL = 0, not present */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == CODE_SEL),
		__FUNCTION__, vector, err_code);
}

/**
 * @brief case name:segmentation_exception_check_call_gate_jmp_GP_table28_009
 *
 * Summary: In protected mode, if target offset in a call gate is beyond the destination
 * code segment limit, execute jmp with this call gate will generate #GP(0).
 */
static void segmentation_rqmid_38850_call_gate_jmp_gp_table28_09()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 0 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x0, 1, 0, call_gate_ent);
	/*code segment DPL = 0, limit set 0:  4096 bytes */
	set_gdt_entry(CODE_SEL, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	asm volatile(ASM_TRY("1f")
		"ljmp $" xstr(CALL_GATE_SEL) ", $0x0\n\t"
		"1:"::);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

/* Table 29*/
asm("call_gate_exception_29_1:\n"
	/*save stack*/
	"mov %ss, %si\n"
	"mov $" xstr(STACK_2ND + 1) ",%ax\n\t"  //change a more limited SS
	"mov %ax, %ss\n\t"
#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n"
	".pushsection .data.ex \n"
	".long 1111f, 1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"lret\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	/*resume code segment selector on stack top*/
	"mov %si, %ss\n"
	"lret\n"
#endif
	);
void call_gate_exception_29_1(void);

static void test_table29_1(const char *args)
{
	//if SS base is not 0, in qemu it can't print out
	printf("%s called in ring1, CS=0x%x, SS=0x%x, stack-base:0x%x\n", __FUNCTION__,
		read_cs(), read_ss(), (u32)ring1_stack);

	asm volatile("lcall $" xstr(CALL_GATE_SEL + 3) ", $0x0\n\t");
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_SS_table29_001
 *
 * Summary: In protected mode, call to a function(name is call_gate_exception) by call gate,
 * the top bytes (Calling CS and Calling EIP) of the called procedure's stack are not
 * within stack limits, execute RET will generate #SS(0).
 */
static void segmentation_rqmid_38854_cs_ret_ss_table29_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 3*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 3, 1, 0, call_gate_exception_29_1);

	/*code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//set stack limit, as 1 byte unit, not 4KB unit
	u32 stack_base = (u32)ring1_stack;
	u32 stack_limit = 4096;

	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	stack_limit -= 0x28; //lcall gate: esp: 0xFE8, so the limit shall be -0x18 , it should work normal
	set_gdt_entry(STACK_2ND, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	setup_tss_ring1(stack_base);

	printf("%s called in ring3, CS=0x%x, SS=0x%x, stack-base:0x%x\n", __FUNCTION__,
		read_cs(), read_ss(), (u32)ring1_stack);

	do_at_ring3(test_table29_1, NULL);

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function29_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	debug_print("orig: esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);

	/*set the code segment selector at the top fo stack to NULL*/
	orig_user_cs = esp_val[1];
	esp_val[1] = 0;

	debug_print("changed to: esp_32=0x%x cs=0x%x 0x%x offset=0x%x\n",
		esp_32, esp_val[1], (u32)&esp_val[1], esp_val[0]);
}

static void call_gate_recovery_user_cs(void)
{
	/*resume code segment selecor at stack top*/
	esp_val[1] = orig_user_cs;
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table29_002
 *
 * Summary: In protected mode, call to a function(name is call_gate_exception) by call gate,
 * if you modify the return code segment selector to NULL, execute RET will generate #GP(0).
 */
static void segmentation_rqmid_35540_cs_ret_gp_table29_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 0*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_exception);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function29_2, call_gate_recovery_user_cs);

	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");

	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function29_3(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);
	esp_val = (u32 *)esp_32;
	/*set the code segment selector at the top of stack to 1k*/
	orig_user_cs = esp_val[1];
	esp_val[1] = SELECTOR_INDEX_2000H;
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table29_003
 *
 * Summary: In protected mode, call to a function(name is call_gate_exception) by call gate,
 * if you modify the return code segment selector addresses descriptor beyond descriptor table limit,
 * execute RET will generate #GP(return code segment selector).
 */
static void segmentation_rqmid_35541_cs_ret_gp_table29_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);

	/* call gate DPL = 0*/
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0, call_gate_exception);

	/*code segment DPL = 0 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function29_3, call_gate_recovery_user_cs);

	/* remove exception vector */
	asm volatile("movl $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)"\n");
	asm volatile("lcall $" xstr(CALL_GATE_SEL) ", $0x0\n\t");

	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, vector, err_code);
}

/* Table 30*/
static void call_gate_function30_1(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	orig_user_cs = esp_val[1];
	esp_val[1] = (orig_user_cs & 0xFFFC);

	printf("changed to: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table30_001
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * the RPL of the return code segment selector is 0 and the the CPL is 3,
 * execute RET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_35542_cs_ret_gp_table30_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function30_1, call_gate_recovery_user_cs);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == (orig_user_cs & 0xFF8)),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function30_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	orig_user_cs = esp_val[1];
	esp_val[1] = (CODE_2ND & 0xFFFC);

	printf("changed to: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table30_001
 *
 * Summary: In protected mode, call to a function(name is call_gate_ent) by call gate,
 * the return code segment is conforming and the segment selector DPL is 3
 * and the RPL of the code segmentation segment selector is 0,
 * execute RET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_35543_cs_ret_gp_table30_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	set_gdt_entry(CODE_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function30_2, call_gate_recovery_user_cs);

	//ring3 call ring3's call gate, its CS/SS not switch
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == (CODE_2ND & 0xFF8)),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function30_3(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	orig_user_cs = esp_val[1];
	esp_val[1] = (orig_user_cs & 0xFFFC);

	printf("changed to: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table30_003
 *
 * Summary: In protected mode, the return code segment is non-conforming and
 * the segment selector's DPL is not equal to the RPL of the code segment's
 * segment selector, execute RET will generate #GP(Return code segment selector).
 */
static void segmentation_rqmid_38855_cs_ret_gp_table30_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	setup_tss_ring1(0);

	set_call_gate_funcs(call_gate_function30_3, call_gate_recovery_user_cs);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == (orig_user_cs & 0xFF8)),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function30_4(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0]);

	/*set the return code segment selector RPL to 0*/
	orig_user_cs = esp_val[1];
	esp_val[1] = CODE_2ND + 3;

	printf("changed to: esp_32=0x%x cs=0x%x eip=0x%x\n", esp_32, esp_val[1], esp_val[0]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_NP_table30_004
 *
 * Summary: In protected mode, Return code segment descriptor is not present,
 * execute RET will generate #NP(Return code segment selector).
 */
static void segmentation_rqmid_38888_cs_ret_np_table30_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/* target code segment */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	// set SEGMENT_PRESENT_CLEAR
	set_gdt_entry(CODE_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function30_4, call_gate_recovery_user_cs);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == NP_VECTOR) && (err_code == (CODE_2ND & 0xFF8)),
		__FUNCTION__, vector, err_code);
}

/* Table 31*/
static void call_gate_function31_1(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;

	/* This case calls printf at ring3, will generate #PF */
	//printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
	//	read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	asm volatile("mov $" xstr(STACK_2ND + 1) ",%ax\n\t"
		"mov %ax, %ss\n\t"
	);
}

static void call_gate_function31_1_end(void)
{
	asm volatile("mov $" xstr(STACK_SEL + 1) ",%ax\n\t"
		"mov %ax, %ss\n\t"
	);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_ss_table31_001
 *
 * Summary: In protected mode, RPL of the return code segment selector is larger than the CPL;
 * the top bytes (parameters) of the called procedure's stack are not within stack limits,
 * execute RET will generate #SS(0).
 */
static void segmentation_rqmid_38953_rpl_ret_ss_table31_01()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//set stack limit, as 1 byte unit, not 4KB unit
	u32 stack_base = (u32)ring1_stack;
	u32 stack_limit = 4096;

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	stack_limit -= 16;
	set_gdt_entry(STACK_2ND, stack_base, stack_limit, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(stack_base);

	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_1, call_gate_function31_1_end);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_2(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	/*set the stack selector to NULL*/
	orig_user_ss = esp_val[3];
	esp_val[3] = 0;
	printf("changed calling ss to 0\n");
}

static void call_gate_recovery_user_ss(void)
{
	/*resume the return code segment selecor RPL to 1*/
	esp_val[3] = orig_user_ss;
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_002
 *
 * Summary: In protected mode, RPL of the return code segment selector
 * is larger than the CPL, stack segment selector is NULL,
 * execute RET will generate #GP(0).
 */
static void segmentation_rqmid_35544_rpl_ret_gp_table31_02()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_2, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_3(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	/*set the stack selector index to 0x2000*/
	orig_user_ss = esp_val[3];
	esp_val[3] = SELECTOR_INDEX_2000H + 3;
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_003
 *
 * Summary: In protected mode, RPL of the return code segment selector is larger than the CPL,
 * return stack segment selector index is not within its descriptor table limits,
 * execute RET will generate #GP(Return stack segment selector).
 */
static void segmentation_rqmid_35546_rpl_ret_gp_table31_03()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_3, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == SELECTOR_INDEX_2000H),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_4(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	//set calling SS as a readonly SS
	orig_user_ss = esp_val[3];
	esp_val[3] = STACK_2ND + 3;

	printf("change calling SS to 0x%x\n", esp_val[3]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_004
 *
 * Summary: In protected mode, RPL of the return code segment selector is
 * larger than the CPL, the return stack segment is not a writable data segment,
 * execute RET will generate #GP(Return stack segment selector).
 */
static void segmentation_rqmid_38954_rpl_ret_gp_table31_04()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//Set as SEGMENT_TYPE_DATE_READ_ONLY
	set_gdt_entry(STACK_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_4, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == STACK_2ND),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_5(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	orig_user_ss = esp_val[3];
	esp_val[3] = orig_user_ss & 0xFF9; //RPL = 1

	printf("change calling SS to 0x%x\n", esp_val[3]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_005
 *
 * Summary: In protected mode, RPL of the return code segment selector is larger than the CPL;
 * the stack segment selector RPL is not equal to the RPL of the return code segment selector,
 * execute RET will generate #GP(Return stack segment selector).
 */
static void segmentation_rqmid_38955_rpl_ret_gp_table31_05()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_5, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == (orig_user_ss & 0xFF8)),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_6(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	//set calling SS as DPL = 1
	orig_user_ss = esp_val[3];
	esp_val[3] = STACK_2ND + 1;

	printf("change calling SS to 0x%x\n", esp_val[3]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_006
 *
 * Summary: In protected mode, RPL of the return code segment selector is larger than the CPL,
 * the return stack segment descriptor DPL is not equal to the RPL of the return code
 * segment selector, execute RET will generate #GP(Return stack segment selector).
 */
static void segmentation_rqmid_38956_rpl_ret_gp_table31_06()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//Set as DPL=1
	set_gdt_entry(STACK_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_6, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == STACK_2ND),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_7(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	//set calling SS as a SEGMENT_PRESENT_CLEAR ss
	orig_user_ss = esp_val[3];
	esp_val[3] = STACK_2ND + 3;

	printf("change calling SS to 0x%x\n", esp_val[3]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_SS_table31_007
 *
 * Summary: In protected mode, RPL of the return code segment selector is
 * larger than the CPL, the return stack segment is not present,
 * execute RET will generate #SS(Return stack segment selector).
 */
static void segmentation_rqmid_38957_rpl_ret_ss_table31_07()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//Set as SEGMENT_PRESENT_CLEAR
	set_gdt_entry(STACK_2ND, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_CLEAR|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_7, call_gate_recovery_user_ss);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == SS_VECTOR) && (err_code == STACK_2ND),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_8(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	//set calling CS as limited CS
	orig_user_cs = esp_val[1];
	esp_val[1] = CODE_2ND + 3;

	printf("change calling CS to 0x%x\n", esp_val[1]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_008
 *
 * Summary: In protected mode, RPL of the return code segment selector is larger than the CPL,
 * the return instruction pointer is not within the return code segment limit,
 * execute RET will generate #GP(0).
 */
static void segmentation_rqmid_38959_rpl_ret_gp_table31_08()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 1 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	/* ring 1 stack segment*/
	set_gdt_entry(STACK_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_1|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//set a limited CS, 4KB space
	set_gdt_entry(CODE_2ND, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	setup_tss_ring1(0);
	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_8, call_gate_recovery_user_cs);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void call_gate_function31_9(void)
{
	asm volatile("mov %%edi, %0\n\t"
		::"m"(esp_32)
		);

	esp_val = (u32 *)esp_32;
	printf("orig: current-ss=0x%x, cs=0x%x, esp=0x%x calling cs=0x%x eip=0x%x, ss=0x%x\n",
		read_ss(), read_cs(), esp_32, esp_val[1], esp_val[0], esp_val[3]);

	//set calling CS as limited CS
	orig_user_cs = esp_val[1];
	esp_val[1] = CODE_2ND + 3;

	printf("change calling CS to 0x%x\n", esp_val[1]);
}

/**
 * @brief case name:segmentation_exception_check_cs_ret_GP_table31_009
 *
 * Summary: In protected mode, RPL of the return code segment selector is equal to the CPL,
 * the return instruction pointer is not within the return code segment limit,
 * execute RET will generate #GP(0).
 */
static void segmentation_rqmid_38961_rpl_ret_gp_table31_09()
{
	struct descriptor_table_ptr old_gdt_desc;

	sgdt(&old_gdt_desc);
	/* call gate DPL = 3 */
	init_call_gate(CALL_GATE_SEL, CODE_SEL, 0xC, 0x3, 1, 0, call_gate_exception);

	/*target code segment DPL = 3 */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	//set a limited CS, 4KB space
	set_gdt_entry(CODE_2ND, 0, 0, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&old_gdt_desc);

	set_call_gate_funcs(call_gate_function31_9, call_gate_recovery_user_cs);
	do_at_ring3(call_gate_in_ring3, NULL);
	restore_call_gate_funcs();

	u8 vector = exception_vector();
	u16 err_code = exception_error_code();
	report("\t\t %s, vector=%d, err_code:0x%x", (vector == GP_VECTOR) && (err_code == 0),
		__FUNCTION__, vector, err_code);
}

static void print_case_list_32(void)
{
	printf("\t\t Segmentation feature 32-Bits Mode case list:\n\r");
}

static void test_segmentation_32(void)
{
	/*386*/
	/* Table 16 */
	segmentation_rqmid_35338_lfs_normal_table16_01();
	segmentation_rqmid_35339_lfs_gp_table16_02();
	segmentation_rqmid_38761_lgs_gp_table16_03();
	segmentation_rqmid_38762_les_gp_table16_04();
	segmentation_rqmid_38763_lds_np_table16_05();

	/* Table 17 */
	segmentation_rqmid_35340_lss_gp_table17_01();
	segmentation_rqmid_35341_lss_gp_table17_02();
	segmentation_rqmid_38766_lss_gp_table17_03();

	/* Table 18 */
	segmentation_rqmid_35342_mov_gp_table18_01();
	segmentation_rqmid_35343_mov_gp_table18_02();

	/* Table 19 */
	segmentation_rqmid_35344_ss_call_ss_table19_01();
	segmentation_rqmid_38774_ss_call_ss_table19_02();
	segmentation_rqmid_38778_ss_call_ss_table19_03();
	segmentation_rqmid_38779_ss_call_ss_table19_04();

	/* Table 20 */
	segmentation_rqmid_35348_cs_mov_gp_table20_01();
	segmentation_rqmid_35349_cs_mov_gp_table20_02();
	segmentation_rqmid_38780_cs_mov_gp_table20_03();

	/* Table 21 */
	segmentation_rqmid_35350_cs_jmp_gp_table21_01();
	segmentation_rqmid_35351_cs_jmp_gp_table21_02();
	segmentation_rqmid_38797_cs_jmp_gp_table21_03();
	segmentation_rqmid_38801_cs_jmp_gp_table21_04();

	/* Table 22*/
	segmentation_rqmid_35353_conforming_jmp_gp_table22_01();
	segmentation_rqmid_35354_conforming_jmp_np_table22_02();
	segmentation_rqmid_38802_conforming_jmp_gp_table22_03();
	segmentation_rqmid_38803_non_conforming_jmp_gp_table22_05();
	segmentation_rqmid_38806_non_conforming_jmp_gp_table22_06();
	segmentation_rqmid_38807_non_conforming_jmp_np_table22_07();
	segmentation_rqmid_38808_non_conforming_jmp_gp_table22_08();

	/* Table 23 */
	segmentation_rqmid_35362_cs_call_gp_table23_01();
	segmentation_rqmid_35363_cs_call_gp_table23_02();
	segmentation_rqmid_38809_cs_call_gp_table23_03();
	segmentation_rqmid_38811_cs_call_np_table23_04();
	segmentation_rqmid_38116_cs_call_ss_table23_05();
	segmentation_rqmid_38823_cs_call_gp_table23_06();
	segmentation_rqmid_38824_cs_call_gp_table23_07();
	segmentation_rqmid_38825_cs_call_gp_table23_08();
	segmentation_rqmid_38826_cs_call_np_table23_09();
	segmentation_rqmid_38828_cs_call_ss_table23_10();
	segmentation_rqmid_38829_cs_call_gp_table23_11();

	/* Table 24 */
	segmentation_rqmid_35364_call_gate_call_gp_table24_01();
	segmentation_rqmid_35365_call_gate_call_np_table24_02();
	segmentation_rqmid_38832_call_gate_call_gp_table24_03();
	segmentation_rqmid_38833_call_gate_call_gp_table24_04();
	segmentation_rqmid_38834_call_gate_call_gp_table24_05();
	segmentation_rqmid_38835_call_gate_call_np_table24_06();

	/* Table 25 */
	segmentation_rqmid_35395_call_gate_call_ts_table25_01();
	segmentation_rqmid_35396_call_gate_call_ts_table25_02();
	segmentation_rqmid_38839_call_gate_call_ts_table25_03();
	segmentation_rqmid_38840_call_gate_call_ts_table25_04();
	segmentation_rqmid_38841_call_gate_call_ts_table25_05();
	segmentation_rqmid_38842_call_gate_call_ss_table25_06();

	/* Table 26 */
	segmentation_rqmid_35399_call_gate_call_ss_table26_01();
	segmentation_rqmid_35400_call_gate_call_gp_table26_02();

	/* Table 27 */
	segmentation_rqmid_35401_call_gate_call_ss_table27_01();
	segmentation_rqmid_35402_call_gate_call_gp_table27_02();

	/* Table 28 */
	segmentation_rqmid_35403_call_gate_jmp_gp_table28_01();
	segmentation_rqmid_35539_call_gate_jmp_np_table28_02();
	segmentation_rqmid_38843_call_gate_jmp_gp_table28_03();
	segmentation_rqmid_38844_call_gate_jmp_gp_table28_04();
	segmentation_rqmid_38845_call_gate_jmp_gp_table28_05();
	segmentation_rqmid_38846_call_gate_jmp_gp_table28_06();
	segmentation_rqmid_38847_call_gate_jmp_gp_table28_07();
	segmentation_rqmid_38849_call_gate_jmp_np_table28_08();
	segmentation_rqmid_38850_call_gate_jmp_gp_table28_09();

	/* Table 29 */
	segmentation_rqmid_38854_cs_ret_ss_table29_01();
	segmentation_rqmid_35540_cs_ret_gp_table29_02();
	segmentation_rqmid_35541_cs_ret_gp_table29_03();

	/* Table 30 */
	segmentation_rqmid_35542_cs_ret_gp_table30_01();
	segmentation_rqmid_35543_cs_ret_gp_table30_02();
	segmentation_rqmid_38855_cs_ret_gp_table30_03();
	segmentation_rqmid_38888_cs_ret_np_table30_04();

	/* Table 31 */
	segmentation_rqmid_38953_rpl_ret_ss_table31_01();
	segmentation_rqmid_35544_rpl_ret_gp_table31_02();
	segmentation_rqmid_35546_rpl_ret_gp_table31_03();
	segmentation_rqmid_38954_rpl_ret_gp_table31_04();
	segmentation_rqmid_38955_rpl_ret_gp_table31_05();
	segmentation_rqmid_38956_rpl_ret_gp_table31_06();
	segmentation_rqmid_38957_rpl_ret_ss_table31_07();
	segmentation_rqmid_38959_rpl_ret_gp_table31_08();
	segmentation_rqmid_38961_rpl_ret_gp_table31_09();
}

