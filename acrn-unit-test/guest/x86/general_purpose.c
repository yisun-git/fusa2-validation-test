#include "processor.h"
#include "instruction_common.h"
#include "instruction_common.c"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "misc.c"

#ifdef __x86_64__

/**Some common function **/
static u16 *creat_non_aligned_add(void)
{
	__attribute__ ((aligned(16))) u64 addr;
	u64 *aligned_addr = &addr;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

u64 non_canon_addr = 0;
static u64 *creat_non_canon_add(void)
{
	u64 address = (unsigned long)&non_canon_addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address|(1UL<<63));
	}
	return (u64 *)address;
}

static u64 *non_canon_add(u64 addr)
{
	u64 address = addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address|(1UL<<63));
	}
	return (u64 *)address;
}

/** GDT **/
extern gdt_entry_t gdt64[];
void set_gdt64_entry(int sel, u32 base,  u32 limit, u8 access, u8 gran)
{
	int num = sel >> 3;

	/* Setup the descriptor base address */
	gdt64[num].base_low = (base & 0xFFFF);
	gdt64[num].base_middle = (base >> 16) & 0xFF;
	gdt64[num].base_high = (base >> 24) & 0xFF;

	/* Setup the descriptor limits */
	gdt64[num].limit_low = (limit & 0xFFFF);
	gdt64[num].granularity = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	gdt64[num].granularity |= (gran & 0xF0);
	gdt64[num].access = access;
}

void set_gdt64_entry_null(int sel, u32 base,  u32 limit, u8 access, u8 gran)
{
	int num = sel >> 3;

	/* Setup the descriptor base address */
	gdt64[num].base_low = (base & 0xFFFF);
	gdt64[num].base_middle = (base >> 16) & 0xFF;
	gdt64[num].base_high = (base >> 24) & 0xFF;

	/* Setup the descriptor limits */
	gdt64[num].limit_low = (limit & 0xFFFF);
	gdt64[num].granularity = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	gdt64[num].granularity |= (gran & 0x70);
	gdt64[num].access = access;
}

/* load the GDT to*/
void load_gdt_and_set_segment_rigster(void)
{
	asm volatile("lgdt gdt64_desc\n"
		"mov $0x10, %ax\n"
		"mov %ax, %ds\n"
		"mov %ax, %es\n"
		"mov %ax, %fs\n"
		"mov %ax, %gs\n"
		"mov %ax, %ss\n"
	);
}
#endif

/*--------------------------------Test case------------------------------------*/
#ifdef __x86_64__
/*
 * @brief case name: 31315: Data transfer instructions Support_64 bit Mode_MOV_#GP_007.
 *
 * Summary:  Under 64 bit Mode and CPL0,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_007(void)
{
	u64 r_a;
	asm volatile(
		"mov %1, %0\n"
		: "=r" (r_a)
		: "m" (*(creat_non_canon_add())));
}

static void  gp_rqmid_31315_data_transfer_mov_gp_007(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_007;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31318: Data transfer instructions Support_64 bit Mode_MOV_#GP_008.
 *
 * Summary:  Under 64 bit Mode and CPL1,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_008(void)
{
	u64 r_a = 1;
	asm volatile("mov %1, %0\n"
		: "=m"(*(creat_non_canon_add()))
		: "r"(r_a));
}

static void gp_rqmid_31318_data_transfer_mov_gp_008(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_008;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31321: Data transfer instructions Support_64 bit Mode_MOV_#GP_009.
 *
 * Summary:  Under 64 bit Mode and CPL2,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_009(void)
{
	u32 r_a = 1;
	asm volatile("mov %1, %0\n"
		: "=m"(*(creat_non_canon_add()))
		: "r"(r_a));
}

static void  gp_rqmid_31321_data_transfer_mov_gp_009(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_009;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31324: Data transfer instructions Support_64 bit Mode_MOV_#GP_010.
 *
 * Summary:  Under 64 bit Mode and CPL3,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_010(void)
{
	u16 r_a = 1;

	eflags_ac_to_0();
	asm volatile("mov %1, %0 \n"
		: "=r"(r_a)
		: "m"(*(creat_non_canon_add())));
}

static void  gp_rqmid_31324_data_transfer_mov_gp_010(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_010;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31327: Data transfer instructions Support_64 bit Mode_MOV_#GP_011.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_011(void)
{
	u32 r_a = 1;
	asm volatile("mov %1, %0 \n"
		: "=m"(*(creat_non_canon_add()))
		: "r"(r_a));
}

static void ring3_mov_gp_011(const char *fun_name)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_011;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, fun_name);
}
static void  gp_rqmid_31327_data_transfer_mov_gp_011(void)
{
	cr0_am_to_0();

	do_at_ring3(ring3_mov_gp_011, __FUNCTION__);
}

/*
 * @brief case name: 31330: Data transfer instructions Support_64 bit Mode_MOV_#GP_012.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_012(void)
{
	u32 r_a = 1;
	asm volatile("mov %1, %0 \n"
		: "=r"(r_a)
		: "m"(*(creat_non_canon_add())));
}

static void  ring3_mov_gp_012(const char *fun_name)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_012;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, fun_name);
}

static void  gp_rqmid_31330_data_transfer_mov_gp_012(void)
{
	cr0_am_to_1();
	eflags_ac_to_1();

	do_at_ring3(ring3_mov_gp_012, __FUNCTION__);
}

void gp_data_transfer_mov_ud(const char *fun_name)
{
	asm volatile(ASM_TRY("1f")
		"mov %0, %%cs \n"
		"1:"
		: : "r"(0x8) : );
	report("%s", (exception_vector() == UD_VECTOR), fun_name);
}
/*
 * @brief case name: 31333: Data transfer instructions Support_64 bit Mode_MOV_#UD_001.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31333_data_transfer_mov_ud_001(void)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31336: Data transfer instructions Support_64 bit Mode_MOV_#UD_002.
 *
 * Summary: Under 64 bit Mode and CPL1,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31336_data_transfer_mov_ud_002(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31339: Data transfer instructions Support_64 bit Mode_MOV_#UD_003.
 *
 * Summary: Under 64 bit Mode and CPL2,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31339_data_transfer_mov_ud_003(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31342: Data transfer instructions Support_64 bit Mode_MOV_#UD_004.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31342_data_transfer_mov_ud_004(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31370: Data transfer instructions Support_64 bit Mode_MOV_#GP_013.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to clear CR0PG[bit 31](CR0.PG: 0),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_013(void)
{
	u64 check_bit = 0;
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));
	asm volatile(
		"mov %0, %%cr0\n"
		: : "r"(check_bit) : "memory");
}

static void  gp_rqmid_31370_data_transfer_mov_gp_013(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_013;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31378: Data transfer instructions Support_64 bit Mode_MOV_#GP_015.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to write a 1 to any reserved bit in CR4(CR4.R.W: 1),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_31378_data_transfer_mov_gp_015(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31385: Data transfer instructions Support_64 bit Mode_MOV_#GP_017.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to write a 1 to any reserved bit in CR8(CR8.R.W: 1),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_31385_data_transfer_mov_gp_017(void)
{
	cr8_r_w_to_1(__FUNCTION__);
}

/*
 * @brief case name: 31398: Data transfer instructions Support_64 bit Mode_MOV_#GP_019.
 *
 * Summary: Under 64 bit Mode,
 *   If an attempt is made to write a 1 to any reserved bit in CR3(CR3.R.W: 1),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_31398_data_transfer_mov_gp_019(void)
{
	cr3_r_w_to_1(__FUNCTION__);
}

/*
 * @brief case name: 32284: Data transfer instructions Support_64 bit Mode_MOV_#GP_021.
 *
 * Summary: Under 64 bit Mode,
 *   If an attempt is made to leave IA-32e mode by clearing CR4PAE[bit 5](CR4.PAE: 0),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_32284_data_transfer_mov_gp_021(void)
{
	report("%s #GP exception", cr4_pae_to_0() == true, __FUNCTION__);
}

/*
 * @brief case name: 31609: Data transfer instructions Support_64 bit Mode_MOV_#DB_001.
 *
 * Summary: Under 64 bit Mode,
 *  If any debug register is accessed while the DR7GD[bit 13] = 1(DeAc: true, DR7.GD: 1),
 *  executing MOV shall generate #DB.
 */
static void mov_write_dr7(ulong val)
{
	asm volatile ("mov %0, %%dr7 \n"
		: : "r"(val) : "memory");
}

static void  gp_rqmid_31609_data_transfer_mov_db_001(void)
{
	gp_trigger_func fun;
	bool ret;
	//dr7_gd_to_1();

	unsigned long check_bit = 0;

	printf("***** Set DR7.GD[bit 13] to 1 *****\n");

	check_bit = read_dr7();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));

	/*set DR7.GD to 1*/
	mov_write_dr7(check_bit);

	fun = (gp_trigger_func)mov_write_dr7;
	ret = test_for_exception(DB_VECTOR, fun, (void *)check_bit);

	report("%s Execute Instruction: MOV, to generate #DB",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31614: Data transfer instructions Support_64 bit Mode_MOV_#GP_035.
 *
 * Summary: Under 64 bit Mode and CPL1,
 *  If the current privilege level is not 0(CS.CPL: 1),
 *  executing MOV shall generate #GP.
 */
ulong dr7_check_bit;
static void  ring1_mov_gp_035(const char *msg)
{
	gp_trigger_func fun;
	int level;
	bool ret;

	level = read_cs() & 0x3;

	fun = (gp_trigger_func)mov_write_dr7;
	ret = test_for_exception(GP_VECTOR, fun, (void *)dr7_check_bit);

	report("gp_rqmid_31614_data_transfer_mov_gp_035 Execute Instruction"
		": MOV, to generate #GP", (ret == true) && (level == 1));
}

static void  gp_rqmid_31614_data_transfer_mov_gp_035(void)
{
	//dr7_gd_to_1();

	unsigned long check_bit = 0;

	printf("***** Set DR7.GD[bit 13] to 0 *****\n");

	check_bit = read_dr7();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13)));
	dr7_check_bit = check_bit;

	do_at_ring1(ring1_mov_gp_035, "");
}

/*
 * @brief case name: 31271: Data transfer instructions Support_64 bit Mode_CMPXCHG16B_#GP_060.
 *
 * Summary: Under 64 bit Mode,
 *   If memory operand for CMPXCHG16B is not aligned on a 16-byte boundary(CMPXCHG16B_Aligned: false),
 *  executing CMPXCHG16B shall generate #GP.
 */
__unused static void cmpxchg16b_gp_060(void)
{
	asm volatile ("cmpxchg16b %[add] \n"
		: [add]"=m"(*(creat_non_aligned_add())) : : "memory");
}
static void  gp_rqmid_31271_data_transfer_cmpxchg16b_gp_060(void)
{
	int is_support;
	gp_trigger_func fun;
	bool ret;

	is_support = ((cpuid(0x1).c)>>13)&1;
	if (is_support) {
		fun = (gp_trigger_func)cmpxchg16b_gp_060;
		ret = test_for_exception(GP_VECTOR, fun, NULL);
	} else {
		ret = false;
	}

#if 0
	CHECK_INSTRUCTION_INFO(__FUNCTION__, cpuid_cmpxchg16b_to_1);
	asm volatile ("CMPXCHG16B %[add] \n" : [add]"=m"(*(creat_non_aligned_add())) : : "memory");
#endif
	/* Need modify unit-test framework, CMPXCHG16B can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31323: Data transfer instructions Support_64 bit Mode_PUSH_#UD_017.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the PUSH is of CS, SS, DS, or ES(Push Seg: hold),
 *  executing PUSH shall generate #UD.
 */
__unused static void push_ud_017(void)
{
	asm volatile (".byte 0x0e \n");	  /* "push %cs \n" */
}

static void  gp_rqmid_31323_data_transfer_push_ud_017(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)push_ud_017;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, PUSH %%cs can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32288: Data transfer instructions Support_64 bit Mode_POP_#GP_066.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the descriptor is outside the descriptor table limit(Descriptor limit: outside),
 *  executing POP shall generate #GP.
 */
__unused static void pop_gp_066(void)
{
	asm volatile ("pop %fs \n");
}

static void  gp_rqmid_32288_data_transfer_pop_ud_066(void)
{
	gp_trigger_func fun;
	bool ret;
	set_gdt64_entry(DS_SEL, 0, 0xc000f, 0x93, 0);

	load_gdt_and_set_segment_rigster();
	//asm volatile("pop %fs \n");

	fun = (gp_trigger_func)pop_gp_066;
	ret = test_for_exception(GP_VECTOR, fun, NULL);
	/* Need modify unit-test framework, PUSH %%cs can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31495: Binary arithmetic instructions Support_64 bit Mode_IDIV_#DE_007.
 *
 * Summary:Under 64 bit Mode and CPL0,
 * If the quotient is too large for the designated register(Quot large: true),
 * executing IDIV shall generate #DE.
 */
__unused static void idiv_de_007(void)
{
	long int a = -4294967296;
	long int b = -1;
	asm volatile ("mov %[dividend], %%eax \n"
		"cdq  \n"
		"idiv %[divisor] \n"
		: : [dividend]"r"((int)a), [divisor]"r"((int)b) : );
}

static void  gp_rqmid_31495_binary_arithmetic_idiv_de_007(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)idiv_de_007;
	ret = test_for_exception(DE_VECTOR, fun, NULL);
	/* Need modify unit-test framework, idiv will trigger double-fault in ASM_TRY handler. */
	report("%s", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31522: Binary arithmetic instructions Support_64 bit Mode_DIV_normal_execution_059.
 *
 * Summary:Under 64 bit Mode and CPL0,
 * If the quotient is too large for the designated register(Quot large: true),
 * executing IDIV shall generate #DE.
 */
__unused static void div_execute(void)
{
	int a = 100;
	int b = 2;

	asm volatile ("mov %[dividend], %%eax \n"
		"div %[divisor] \n"
		"1:"
		: : [dividend]"r"(a), [divisor]"r"(b) : );
}

static void  gp_rqmid_31522_binary_arithmetic_div_normal_059(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)div_execute;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31655: Control transfer instructions Support_64 bit Mode_JMP_#UD_030.
 *
 * Summary: Under 64 bit Mode and CPL0 ,(64-bit mode only),
 *  If a far jump is direct to an absolute address in memory(JMP Abs address: true),
 *  executing JMP shall generate #UD.
 */
__unused static void jmp_ud_030(void)
{
	asm volatile(".byte 0xea, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
}

static void  gp_rqmid_31655_control_transfer_jmp_ud_030(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)jmp_ud_030;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: JMP", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31286: Control transfer instructions Support_64 bit Mode_CALL_#UD_042.
 *
 * Summary: Under 64 bit Mode and CPL0 ,(64-bit mode only),
 *  If a far call is direct to an absolute address in memory(CALL Abs address: true),
 *  executing CALL shall generate #UD.
 */
__unused static void call_ud_042(void)
{
	/* "call (absolute address in memory) \n" */
	//u64 *data_add = (u64 *)0x000000000048e520;
	asm volatile(".byte 0x9a, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
	/*
	 *asm volatile("call %[addr]\n\t"
	 *	: : [addr]"m"(data_add)
	 *	: "memory");
	 */
}

static void  gp_rqmid_31286_control_transfer_call_ud_042(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)call_ud_042;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31379: Control transfer instructions Support_64 bit Mode_IRET_#GP_097.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If EFLAGSNT[bit 14] = 1(EFLAGS.NT: 1),  executing IRET shall generate #GP.
 */
__unused static void iret_gp_097(void)
{
	/* "call (absolute address in memory) \n" */
	/* u64 *data_add = (u64 *)0x000000000048e520; */
	asm volatile("iret \n");
}

static void  gp_rqmid_31379_control_transfer_iret_gp_097(void)
{
	gp_trigger_func fun;
	bool ret;

	CHECK_INSTRUCTION_INFO(__FUNCTION__, eflags_nt_to_1);
	fun = (gp_trigger_func)iret_gp_097;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31652: Control transfer instructions Support_64 bit Mode_INT1_#GP_115.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the vector selects a descriptor outside the IDT limits(DesOutIDT: true),
 *  executing INT1 shall generate #GP.
 */
__unused static void int_gp_115(void)
{
	//asm volatile(".byte 0xf1");
	asm volatile("INT $14");
}

static __unused void  gp_rqmid_31652_control_transfer_int_gp_115(void)
{
	struct descriptor_table old_idt;
	struct descriptor_table new_idt;
	gp_trigger_func fun;
	bool ret;

	old_idt = stor_idt();
	printf("idt limit: %#x \n", old_idt.limit);

	new_idt.limit = (16*14) - 1;
	new_idt.base = old_idt.base;

	set_idt(new_idt);
	printf("modify idt limit to: %#x \n", new_idt.limit);

	fun = (gp_trigger_func)int_gp_115;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);

	/* resume environment */
	set_idt(old_idt);
}

/*
 * @brief case name: 31901: Segment Register instructions Support_64 bit Mode_LFS_#GP_125.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *   If the memory address is in a non-canonical form(CS.CPL: 0, RPL?CPL: true, S. segfault: occur),
 *   executing LFS shall generate #GP.
 */
static void lfs_gp_125(void)
{
	u64 *address;
	struct lseg_st lfss;
	lfss.a = 0xffffffff;
	lfss.b = 0x280;

	address = non_canon_add((u64)&lfss);
	asm volatile("REX\n\t"
		"lfs %0, %%rax\n\t"
		: : "m"(address));
}

static void  gp_rqmid_31901_segment_instruction_lfs_gp_125(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf(" Set gdt64 segment not present\n ");
	set_gdt64_entry(0x50, 0, 0xc000f, 0x93, 0);

	//printf(" Load gdt to lgdtr\n ");
	asm volatile("lgdt gdt64_desc\n");

	fun = (gp_trigger_func)lfs_gp_125;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction: LFS", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31751: Segment Register instructions Support_64 bit Mode_LSS_#GP_132.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory address is in a non-canonical form(Ad. Cann.: non mem),
 *  executing LSS shall generate #GP.
 */
__unused static void lss_gp_132(void)
{
	eflags_ac_to_1();
	asm volatile("lss (%0), %%eax  \n"
		: : "r"(creat_non_canon_add()));
}

static void  ring3_lss_gp_132(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)lss_gp_132;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	/* Need modify unit-test framework, can't handle exception. */
	report("%s Execute Instruction: LFS", ret == true, msg);
}

static void  gp_rqmid_31751_segment_instruction_lss_gp_132()
{
	cr0_am_to_0();

	do_at_ring3(ring3_lss_gp_132, __FUNCTION__);
}

/*
 * @brief case name: 31937: MSR access instructions_64 Bit Mode_RDMSR_#GP_004.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a reserved  MSR address(ECX_MSR_Reserved: true),
 *  executing RDMSR shall generate #GP.
 */
static void  gp_rqmid_31937_msr_access_rdsmr_gp_004(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*D10H - D4FH: Reserved MSR Address Space for L2 CAT Mask Registers*/
	report("%s Execute Instruction: reserved",
		rdmsr_checking(0x00000D20, aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31939: MSR access instructions_64 Bit Mode_RDMSR_#GP_005.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a unimplemented MSR address(ECX_MSR_Reserved: true),
 *  executing RDMSR shall generate #GP.
 */
static void  gp_rqmid_31939_msr_access_rdsmr_gp_005(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*4000_0000H-4000_00FFH:
	 *All existing and future processors will not implement MSRs in this range.
	 */
	report("%s Execute Instruction: unimplement",
		rdmsr_checking(0x40000000, aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31949: MSR access instructions_64 Bit Mode_WRMSR_#GP_009.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a unimplemented MSR address(ECX_MSR_Reserved: true),
 *  executing wrmsr shall generate #GP.
 */
static void  gp_rqmid_31949_msr_access_wrmsr_gp_009(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*4000_0000H-4000_00FFH:
	 *All existing and future processors will not implement MSRs in this range.
	 */
	report("%s Execute Instruction: unimplement",
		wrmsr_checking(0x40000000, *aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31953: MSR access instructions_64 Bit Mode_WRMSR_#GP_011.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a reserved MSR address(ECX_MSR_Reserved: true),
 *  executing wrmsr shall generate #GP.
 */
static void  gp_rqmid_31953_msr_access_wrmsr_gp_011(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*D10H - D4FH: Reserved MSR Address Space for L2 CAT Mask Registers*/
	report("%s Execute Instruction: unimplement",
		wrmsr_checking(0x00000D20, *aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 32163: Random number generator instructions Support_64 Bit Mode_RDRAND_#UD_006.
 *
 * Summary: Under 64 Bit Mode,
 *  If the F2H or F3H prefix is used(F2HorF3H: hold),
 *  executing RDRAND shall generate #UD.
 */
__unused static void rdrand_ud_006(void)
{
	asm volatile(".byte 0xf3, 0x0f, 0xc7, 0xf0 \n");
}

static void  gp_rqmid_32163_random_number_rdrand_ud_006(void)
{
	gp_trigger_func fun;
	bool ret;
	int is_support;

	is_support = ((cpuid(0x1).c)>>30)&1;

	if (is_support) {
		fun = (gp_trigger_func)rdrand_ud_006;
		ret = test_for_exception(UD_VECTOR, fun, NULL);
	} else {
		ret = false;
	}

	/* Need modify unit-test framework, can't handle eception. */
	report("%s Execute Instruction: rdrand", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32191: Undefined instruction Support_64 Bit Mode_UD1_#UD_010.
 *
 * Summary: Under Protected/Real Address/64 Bit Mode,
 *  Raises an invalid opcode exception in all operating modes(OpcodeExcp: true),
 *  executing UD1 shall generate #UD.
 */
static void ud1_ud_010(void)
{
	asm volatile(
		/* "ud1 %%eax, %%edx \n" */
		".byte 0x0f, 0xb9 \n"
		: : :);
}

static void  gp_rqmid_32191_ud_instruction_ud1_ud_010(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)ud1_ud_010;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	report("%s Execute Instruction: UD1", ret == true, __FUNCTION__);
}

#else  /* #ifndef __x86_64__*/
/*
 * @brief case name: 32003: Data transfer instructions Support_Protected Mode_MOV_#GP_001.
 *
 * Summary: Under Protected Mode,
 * If an attempt is made to write a 1 to any reserved bit in CR4(CR4.R.W: 1),
 * executing MOV shall generate #GP.
 */
static void  gp_rqmid_32003_data_transfer_mov_gp_001(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf("***** Set the reserved bit in CR4, such as CR4[12] *****\n");
	unsigned long check_bit = 0;

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32005: Data transfer instructions Support_Protected Mode_MOV_#GP_002.
 *
 * Summary: Under 64 bit Mode,
 *   If an attempt is made to leave IA-32e mode by clearing CR4PAE[bit 5](CR4.PAE: 0),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_32005_data_transfer_mov_gp_002(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf("***** Using MOV instruction to set CR4.PCIDE[bit 17] to 1  *****\n");
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31737: Binary arithmetic instructions Support_Protected Mode_IDIV_#DE_001.
 *
 * Summary: Under Protected Mode and CPL0,
 *   If the source operand (divisor) is 0(Divisor = 0: true),
 *   executing IDIV shall generate #DE.
 */
__unused static void idiv_de_32bit_001(void)
{
	int a = 1;
	int b = 0;
	asm volatile ("mov %[dividend], %%eax \n"
		"idiv %[divisor] \n"
		: : [dividend]"r"(a), [divisor]"r"(b) : );
}

static void  gp_rqmid_31737_binary_arithmetic_idiv_de_32bit_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)idiv_de_32bit_001;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31956: Control transfer instructions Support_Protected Mode_BOUND_#BR_001.
 *
 * Summary:Under Protected Mode and CPL0,
 *  If the bounds test fails with BOUND range exceeded(Bound test: fail),
 *  executing BOUND shall generate #BR.
 */
static void bound_32bit_br_001(void)
{
	int t[2] = {0, 10};
	u32 r_a = 0xff;
	asm volatile ("bound %1, %[add]\n"
		: [add]"=m"(t[0])
		: "r"(r_a)
		: "memory");
}

static void  gp_rqmid_31956_data_transfer_bound_32bit_br_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)bound_32bit_br_001;
	ret = test_for_exception(BR_VECTOR, fun, NULL);

	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32179: Decimal arithmetic instructions Support_Protected Mode_AAM_#DE_001.
 *
 * Summary: Under Protected/Real Address Mode ,
 *  If an immediate value of 0 is used(imm8 = 0: hold),
 *  executing AAM shall generate #DE.
 */
static void aam_32bit_de_001(void)
{
	asm volatile("aam $0\n"
		: : :);
}

static void  gp_rqmid_32179_decimal_arithmetic_aam_32bit_de_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)aam_32bit_de_001;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s Execute Instruction: AAM", ret == true, __FUNCTION__);
}

#endif   /* #ifndef __x86_64__*/

/*------------------------------Test case End!---------------------------------*/
int main(void)
{
	printf("----------eflag = %lx\n", read_rflags());

	setup_vm();
	setup_idt();

	extern unsigned char kernel_entry1;
	set_idt_entry(0x21, &kernel_entry1, 1);
	extern unsigned char kernel_entry2;
	set_idt_entry(0x22, &kernel_entry2, 2);
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);

#ifdef __x86_64__
	init_gdt_description();

	/*--------------------------64 bit--------------------------*/
	gp_rqmid_31522_binary_arithmetic_div_normal_059();
	gp_rqmid_31315_data_transfer_mov_gp_007();
	do_at_ring1(gp_rqmid_31318_data_transfer_mov_gp_008, "");
	do_at_ring2(gp_rqmid_31321_data_transfer_mov_gp_009, "");
	do_at_ring3(gp_rqmid_31324_data_transfer_mov_gp_010, "");
	do_at_ring1(gp_rqmid_31336_data_transfer_mov_ud_002, "");
	do_at_ring2(gp_rqmid_31339_data_transfer_mov_ud_003, "");
	do_at_ring3(gp_rqmid_31342_data_transfer_mov_ud_004, "");
	gp_rqmid_31751_segment_instruction_lss_gp_132();
	gp_rqmid_31327_data_transfer_mov_gp_011();
	gp_rqmid_31330_data_transfer_mov_gp_012();
	gp_rqmid_31333_data_transfer_mov_ud_001();
	gp_rqmid_31378_data_transfer_mov_gp_015();
	gp_rqmid_32284_data_transfer_mov_gp_021();
	gp_rqmid_31614_data_transfer_mov_gp_035();//hv bug
	gp_rqmid_31271_data_transfer_cmpxchg16b_gp_060();
	gp_rqmid_31323_data_transfer_push_ud_017();
	gp_rqmid_32288_data_transfer_pop_ud_066();
	gp_rqmid_31495_binary_arithmetic_idiv_de_007();
	gp_rqmid_31655_control_transfer_jmp_ud_030();
	gp_rqmid_31286_control_transfer_call_ud_042();
	gp_rqmid_31379_control_transfer_iret_gp_097();
	gp_rqmid_32163_random_number_rdrand_ud_006();
	gp_rqmid_32191_ud_instruction_ud1_ud_010();
	gp_rqmid_31370_data_transfer_mov_gp_013();
	gp_rqmid_31385_data_transfer_mov_gp_017();
	gp_rqmid_31398_data_transfer_mov_gp_019();//native/hv all fail
	gp_rqmid_31609_data_transfer_mov_db_001();//native/hv all fail
	gp_rqmid_31652_control_transfer_int_gp_115();
	gp_rqmid_31937_msr_access_rdsmr_gp_004();
	gp_rqmid_31939_msr_access_rdsmr_gp_005();
	gp_rqmid_31949_msr_access_wrmsr_gp_009();
	gp_rqmid_31953_msr_access_wrmsr_gp_011();
	gp_rqmid_31901_segment_instruction_lfs_gp_125();
	/*---64 bit end----*/
#else /* #ifndef __x86_64__ */
	/*--------------------------32 bit--------------------------*/
	gp_rqmid_32003_data_transfer_mov_gp_001();
	gp_rqmid_32005_data_transfer_mov_gp_002();
	gp_rqmid_31737_binary_arithmetic_idiv_de_32bit_001();
	gp_rqmid_31956_data_transfer_bound_32bit_br_001();
	gp_rqmid_32179_decimal_arithmetic_aam_32bit_de_001();
	/*--------------------------32 bit end ----------------------*/
#endif

	return report_summary();
}

