#include "segmentation.h"

/* bit test */
#define BIT_IS(NUMBER, N) ((((uint64_t)NUMBER) >> N) & (0x1))

#ifndef __x86_64__
asm ("boot_ldt = 0x400\n\t");

static int do_at_ring3(void (*fn)(const char *), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
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
		"mov $1f, %%" R "dx\n\t"
		"int %[kernel_entry_vector]\n\t"
		".section .text.entry \n\t"
		"kernel_entry: \n\t"
		"mov %%" R "cx, %%" R "sp \n\t"
		"mov %[kernel_ds], %%cx\n\t"
		"mov %%cx, %%ds\n\t"
		"mov %%cx, %%es\n\t"
		"mov %%cx, %%fs\n\t"
		"mov %%cx, %%gs\n\t"
		"jmp *%%" R "dx \n\t"
		".section .text\n\t"
		"1:\n\t"
		: [ret] "=&a" (ret)
		: [user_ds] "i" (USER_DS),
		[user_cs] "i" (USER_CS),
		[user_stack_top]"m"(user_stack[sizeof user_stack]),
		[fn]"r"(fn),
		[arg]"D"(arg),
		[kernel_ds]"i"(KERNEL_DS),
		[kernel_entry_vector]"i"(0x20)
		: "rcx", "rdx");
	return ret;
}

void save_unchanged_reg(void)
{
	asm volatile ("sldt (0x8100)\n\t");

	/*segment base*/
	/*Read the magic(0xdeadbeef), in the ds segment and save to memory*/
	asm volatile ("mov %ds:0x100 ,%eax\n\t"
		"mov %eax,(0x9100)\n\t");
	/*Read the magic(0xdeadbeef), in the es segment and save to memory*/
	asm volatile ("mov %es:0x100 ,%eax\n\t"
		"mov %eax,(0x9100 + 0x4)\n\t");
	/*Read the magic(0xdeadbeef), in the fs segment and save to memory*/
	asm volatile("mov %fs:0x100 ,%eax\n\t"
		"mov %eax,(0x9100 + 0x8)\n\t");
	/*Read the magic(0xdeadbeef), in the gs segment and save to memory*/
	asm volatile("mov %gs:0x100 ,%eax\n\t"
		"mov %eax,(0x9100 + 0xc)\n\t");
	/*Read the magic(0xdeadbeef), in the ss segment and save to memory*/
	asm volatile("mov %ss:0x100 ,%eax\n\t"
		"mov %eax,(0x9100 + 0x10)\n\t");
	/*Read the memory, in the cs segment and save to memory*/
	asm volatile("mov %cs:0x100 ,%eax\n\t"
		"mov %eax,(0x9100 + 0x14)\n\t");
}

void common_function(void)
{
	printf("common_function ok\n");
}

asm ("common_callgate_funct: \n"
	"mov %esp, %edi \n"
	"call common_function\n"
	"lret");

static void set_ldt_entry(int sel, u32 base,  u32 limit, u8 access, u8 gran)
{
	int num = sel >> 3;

	/* Setup the descriptor base address */
	boot_ldt[num].base_low = (base & 0xFFFF);
	boot_ldt[num].base_middle = (base >> 16) & 0xFF;
	boot_ldt[num].base_high = (base >> 24) & 0xFF;

	/* Setup the descriptor limits */
	boot_ldt[num].limit_low = (limit & 0xFFFF);
	boot_ldt[num].granularity = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	boot_ldt[num].granularity |= (gran & 0xF0);
	boot_ldt[num].access = access;
}

static void init_all_ldt_of_null(void)
{
	short i;
	for (i = 0; i < 50; i++) {
		set_ldt_entry(i*8, 0,  0, 0, 0);
	}
}

static void enable_init(void)
{
	init_all_ldt_of_null();
}

/**
 * @brief Case name:Segmentation ACRN hypervisor shall set initial guest SS to Selector_001
 *
 * Summary: Power on/Reset DUT: BP Start, AP Reads SS Register Value while Waiting for Initialization .
 *
 */
static void Segmentation_rqmid_27186_ap_read_ss_init_value(void)
{
	short initial_SS = 0;

	asm ("mov (0x8000 + 0x14) ,%%ax\n\t"
		"mov %%ax,%0\n\t"
		: "=q"(initial_SS));

	report("\t\t Segmentation_rqmid_27186_ap_read_ss_init_value", initial_SS == 0x0);
}

/**
 * @brief Case name:Segmentation ACRN hypervisor shall set initial
 * guest IA32_KERNEL_GS_BASE to 0H following start-up_001
 *
 * Summary: Power on/reset, read IA32_KERNEL_GS_BASE register at start-up
 *
 */
static void Segmentation_rqmid_27133_bp_startup_read_i32_kernel_gs_base(void)
{
	u32 initial_start_i32_kernel_gs_base_h, initial_start_i32_kernel_gs_base_l;
	u64 kernel_gs_base_h, kernel_gs_base_l, kernel_gs_base;

	asm ("mov (0x6000 + 0x3c) ,%%eax\n\t"
		"mov %%eax,%0\n\t"
		: "=q"(initial_start_i32_kernel_gs_base_h));

	asm ("mov (0x6000 + 0x40) ,%%eax\n\t"
		"mov %%eax,%0\n\t"
		: "=q"(initial_start_i32_kernel_gs_base_l));

	kernel_gs_base_h = initial_start_i32_kernel_gs_base_h;
	kernel_gs_base_l = initial_start_i32_kernel_gs_base_l;
	kernel_gs_base = kernel_gs_base_h << 32 | kernel_gs_base_l;

	report("\t\t Segmentation_rqmid_27133_bp_startup_read_i32_kernel_gs_base", kernel_gs_base == 0x0);
}

/*RPL=0*/
int cpl_entry(void)
{
	asm volatile("mov $0x50,%%ax\n\t"
		ASM_TRY("1f")
		"mov %%ax,%%ss\n\t"
		"1:"
		:::);
	return exception_vector();
}

static int call_fun(void)
{
	int ret;

	/*CPL=0*/
	asm volatile ("call cpl_entry\n\t" : "=a"(ret));

	return ret;
}

/**
 * @brief Case name:Segmentation When any vCPU loads SS register and the CPL_001
 *
 * Summary: chaeck RPL of stack segment is 0, DPL of stack segment descriptor is 3,
 *          and running in ring0；Generating #GP (selector) at runtime
 */
static void Segmentation_rqmid_28300_chaeck_DPL_is_3_of_stack_segment(void)
{
	u16 index;
	u32 base;
	struct descriptor_table_ptr gdt_descriptor_table;

	index = 0x50;
	base = 0x0;

	sgdt(&gdt_descriptor_table);

	/*DPL =3*/
	set_gdt_entry(index, base, SEGMENT_LIMIT, SEGMENT_PRESENT_SET|DESCRIPTOR_TYPE_CODE_OR_DATA|
		SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED|DESCRIPTOR_PRIVILEGE_LEVEL_3,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	lgdt(&gdt_descriptor_table);

	report("\t\t Segmentation_rqmid_28300_chaeck_DPL_is_3_of_stack_segment", call_fun() == GP_VECTOR);
}

/**
 * @brief Case name:Segmentation When any vCPU accesses data segment exclusive stack segment and P bit in data segment
 *	     exclusive stack segment descriptor is clear_001
 *
 * Summary: Checking DS load a non-existent data segment description table will generate #NP
 *
 */
static void Segmentation_rqmid_27165_generate_NP_exclusive_stack(void)
{
	u16 index;
	u32 base;
	int ret;

	base = 0x0;
	index = 0x50;

	set_gdt_entry(index, base, SEGMENT_LIMIT, DESCRIPTOR_TYPE_CODE_OR_DATA|
		SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	asm volatile("mov %0, %%ax\n\t" :: "r"(index));
	asm volatile(ASM_TRY("1f")
		"mov %%ax, %%ds\n\t"
		"1:"
		:::);

	ret = exception_vector();
	report("\t\t Segmentation_rqmid_27165_generate_NP_exclusive_stack", ret == NP_VECTOR);

}

typedef void (*trigger_func)(void *data);
void ljmp_p(void *data)
{
	asm volatile("ljmpl $0xc, $0\n\t"
		:::);
}

/**
 * @brief Case name: Segmentation When any vCPU load LDT and base address in the segment descriptor
 *        is missing in memory_001
 *
 * Summary: When any vCPU load LDT and base address in the segment descriptor is missing in memory,
 * shall generate  #PF(selector)
 *
 */
static void Segmentation_rqmid_28340_loading_ldt_miss_base_address_generates_pf(void)
{
	u32 *base, ldt_linear;
	u16 index, led_index;
	u32 ldt_base, ret;
	struct descriptor_table_ptr old_gdt_desc;
	unsigned char *linear = malloc(PAGE_SIZE);

	if (linear == NULL) {
		printf("malloc %lu bytes failed.\n", PAGE_SIZE);
		return;
	}

	led_index = 0x8;
	base = (u32 *)(linear);
	ldt_linear = (u32)(*(&base));
	sgdt(&old_gdt_desc);

	set_ldt_entry(led_index, ldt_linear,  SEGMENT_LIMIT,
		SEGMENT_PRESENT_SET|DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_ONLY,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);

	index = 0x50;
	base = (u32 *)(boot_ldt);
	ldt_base = (u32)(*(&base));

	set_gdt_entry(index, ldt_base, SEGMENT_LIMIT,  SEGMENT_PRESENT_SET|SEGMENT_TYPE_DATE_READ_WRITE,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	set_page_control_bit(phys_to_virt((unsigned long)linear), PAGE_PTE, PAGE_P_FLAG, 0, 1);

	lgdt(&old_gdt_desc);
	asm volatile("mov $0x50,%ax");
	asm volatile("lldt %ax");

	trigger_func fun;
	fun = ljmp_p;
	ret = test_for_exception(PF_VECTOR, fun, NULL);
	report("\t\t Segmentation_rqmid_28340_loading_ldt_miss_base_address_generates_pf", ret == true);
}

static void jmp_rpl_than_dlp(const char *msg)
{
	int ret;
	asm volatile(ASM_TRY("1f")
		"ljmpl $0x60, $0\n\t"
		"1:"
		:::);

	ret = exception_vector();
	report("\t\t Segmentation_rqmid_28056_call_gate_with_jmp_accesses_nonconforming_codesegment", ret == GP_VECTOR);
}

/**
 * @brief Case name:Segmentation When any vCPU accesses a nonconforming code segment using a call gate
 *	     with JMP instruction and any following conditions is met_001
 *
 * Summary: When any vCPU accesses a nonconforming code segment using a call gate with JMP instruction and CPL
 *	    is greater than call gate DPL,shall generate #GP(selector)
 */
static void Segmentation_rqmid_28056_call_gate_with_jmp_accesses_nonconforming_codesegment(void)
{
	struct gate_descriptor *callgate = NULL;
	struct descriptor_table_ptr old_gdt_desc;

	u32 index, base, call_base;
	void common_callgate_funct(void);

	call_base = (u32)common_callgate_funct;

	/*index = 0x60*/
	callgate = (struct gate_descriptor *)&gdt32[12];
	/*call_entry_base*/
	callgate->gd_looffset = call_base;
	callgate->gd_hioffset = (call_base >> 16)&0xFFFF;
	callgate->gd_selector = 0x68;
	callgate->gd_stkcpy	= STACK_CPY_NUMBER;
	callgate->gd_xx = UNUSED;
	callgate->gd_type = SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING;
	/*code segment DPL to be 0*/
	callgate->gd_dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	callgate->gd_p = SEGMENT_PRESENT;

	index = 0x68;
	base = 0;

	sgdt(&old_gdt_desc);
	/*set the call gate DPL to be 0*/
	set_gdt_entry(index, base, SEGMENT_LIMIT, SEGMENT_PRESENT_SET|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_RAED,
		GRANULARITY_SET|DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);

	/*ring3*/
	do_at_ring3(jmp_rpl_than_dlp, "RPL > DLP");
}
#else  /*__x86_64__*/
void save_unchanged_reg(void)
{

}

void call_gate_function(void)
{
	printf("call_gate_function ok\n");
}

asm ("call_gate_ent: \n"
	"	mov %esp, %edi \n"
	"	call call_gate_function\n"
	"	iretq");

/**
 *  @brief Case name:Segmentation When any vCPU accesses segment in IA-32e mode and memory address accessed by
 *        the selector is non-canonical_001
 *
 * Summary: non-canonical memory addresses accessed in IA-32E mode shall generate  #GP
 *
 */
static void Segmentation_rqmid_27772_memory_address_accessed_non_canonical(void)
{
	struct segment_desc64 *gdt_table;
	struct segment_desc64 *new_gdt_entry;
	struct descriptor_table_ptr old_gdt_desc;
	int target_sel, ret;

	sgdt(&old_gdt_desc);

	gdt_table		= (struct segment_desc64 *) old_gdt_desc.base;
	new_gdt_entry		= &gdt_table[0x50];
	new_gdt_entry->limit1	= SEGMENT_LIMIT;
	new_gdt_entry->base1	= BASE1;
	new_gdt_entry->base2	= BASE2;
	new_gdt_entry->type	= SEGMENT_TYPE_DATE_READ_WRITE;
	new_gdt_entry->s	= DESCRIPTOR_TYPE_SYS_SEGMENT;
	new_gdt_entry->dpl	= DESCRIPTOR_PRIVILEGE_LEVEL_0;
	new_gdt_entry->p	= SEGMENT_PRESENT;
	new_gdt_entry->limit	= SEGMENT_LIMIT2;
	new_gdt_entry->avl	= AVL0;
	new_gdt_entry->l	= BIT64_CODE_SEGMENT;
	new_gdt_entry->db	= DEFAULT_OPERtion_32_BIT_SEG;
	new_gdt_entry->g	= GRANULARITY;
	new_gdt_entry->base3	= 0x20;
	new_gdt_entry->base4 = 0xFFFF7FF1;

	lgdt(&old_gdt_desc);

	target_sel = 0x50 << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t Segmentation_rqmid_27772_memory_address_accessed_non_canonical", ret == GP_VECTOR);
}

/**
 * @brief Case name:Segmentation When any vCPU accesses code segment in IA-32e
 * mode and upper type field of the call gate descriptor is greater than zero_001
 *
 * Summary: When any vCPU accesses code segment in IA-32e mode and upper type
 * field of the call gate descriptor is greater than zero, shall generate #GP(selector)
 *
 */
static void Segmentation_rqmid_27794_set_non_zero_upper_type_in_call_gate_descriptor(void)
{
	struct descriptor_table_ptr gdt;
	struct segment_desc64  *callgate_code_segment_entry = NULL;
	struct segment_desc64  *gdt_table;
	struct gate_descriptor *call_gate_entry = NULL;
	int target_sel, ret;

	void call_gate_ent(void);
	uint64_t call_base = (uint64_t)call_gate_ent;

	sgdt(&gdt);

	gdt_table = (struct segment_desc64 *) gdt.base;

	/*call gate*/
	/*index = 5x2*/
	call_gate_entry = (struct gate_descriptor *)&gdt_table[5];
	/* call_entry_base;*/
	call_gate_entry->gd_looffset = call_base&0xFFFF;

	if (BIT_IS(call_base, 48)) {
		call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0xFFFF00000000);
	} else {
		call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0x000000000000);
	}

	/* index=12,0x60*/
	call_gate_entry->gd_selector = 0x60;
	call_gate_entry->gd_ist = IST_TABLE_INDEX;
	call_gate_entry->gd_xx = UNUSED;
	call_gate_entry->gd_type = SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING;
	call_gate_entry->gd_dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	call_gate_entry->gd_p = SEGMENT_PRESENT;
	/* type = 00010b*/
	call_gate_entry->sd_xx1 = 0x200;
	/*code segment*/
	/*index=6x2*/
	callgate_code_segment_entry = &gdt_table[6];
	callgate_code_segment_entry->limit1 = SEGMENT_LIMIT;
	callgate_code_segment_entry->base1 = BASE1;
	callgate_code_segment_entry->base2 = BASE2;
	callgate_code_segment_entry->type = SEGMENT_TYPE_CODE_EXE_RAED_ACCESSED;
	callgate_code_segment_entry->s = DESCRIPTOR_TYPE_SYS_SEGMENT;
	callgate_code_segment_entry->dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	callgate_code_segment_entry->p = SEGMENT_PRESENT;
	callgate_code_segment_entry->limit = SEGMENT_LIMIT2;
	callgate_code_segment_entry->avl = AVL0;
	callgate_code_segment_entry->l = BIT64_CODE_SEGMENT;
	callgate_code_segment_entry->db = DEFAULT_OPERtion_16_BIT_SEG;
	callgate_code_segment_entry->g = GRANULARITY;
	callgate_code_segment_entry->base3 = BASE3;
	callgate_code_segment_entry->base4	= BASE4;

	lgdt(&gdt);

	/*index = 5 x 2 x 8 =80 , 0x50*/
	target_sel = 0x50 << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t Segmentation_rqmid_27794_set_non_zero_upper_type_in_call_gate_descriptor", ret == GP_VECTOR);
}

/**
 * @brief Case name:Segmentation When any vCPU accesses segment in IA-32e
 * mode using gate and any of following condition is violated_001
 *
 * Summary: When any vCPU accesses segment in IA-32e mode using gate  and  L bit of target segment is set,
 *	    shall generate #GP(selector)
 *
 */
static void Segmentation_rqmid_28134_set_d_and_l_bits_in_segment_descriptor(void)
{
	struct descriptor_table_ptr gdt;
	struct segment_desc64  *callgate_code_segment_entry = NULL;
	struct segment_desc64  *gdt_table;
	struct gate_descriptor *call_gate_entry = NULL;
	u32 index, callgate_selecter;
	int target_sel, ret;

	index = 5;
	callgate_selecter = 0x60;

	void call_gate_ent(void);
	uint64_t call_base = (uint64_t)call_gate_ent;

	sgdt(&gdt);

	gdt_table = (struct segment_desc64 *) gdt.base;

	/*call gate*/
	/*index = 5x2*/
	call_gate_entry = (struct gate_descriptor *)&gdt_table[index];
	/*call_entry_base*/
	call_gate_entry->gd_looffset = call_base&0xFFFF;

	if (BIT_IS(call_base, 48)) {
		call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0xFFFF00000000);
	} else {
		call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF)|(0x000000000000);
	}

	/* index=12,0x60*/
	call_gate_entry->gd_selector	= callgate_selecter;
	call_gate_entry->gd_ist		= IST_TABLE_INDEX;
	call_gate_entry->gd_xx		= UNUSED;
	call_gate_entry->gd_type	= SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING;
	call_gate_entry->gd_dpl	= DESCRIPTOR_PRIVILEGE_LEVEL_0;
	call_gate_entry->gd_p		= SEGMENT_PRESENT;
	/* type = 00000b*/
	call_gate_entry->sd_xx1		= 0x0;

	/*code segment*/
	/*index=6x2*/
	callgate_code_segment_entry		= &gdt_table[6];
	callgate_code_segment_entry->limit1 = SEGMENT_LIMIT;
	callgate_code_segment_entry->base1 = BASE1;
	callgate_code_segment_entry->base2 = BASE2;
	callgate_code_segment_entry->type = SEGMENT_TYPE_CODE_EXE_RAED_ACCESSED;
	callgate_code_segment_entry->s = DESCRIPTOR_TYPE_SYS_SEGMENT;
	callgate_code_segment_entry->dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	callgate_code_segment_entry->p = SEGMENT_PRESENT;
	callgate_code_segment_entry->limit = SEGMENT_LIMIT2;
	callgate_code_segment_entry->avl = AVL0;
	/*f CS.L = 1 and CS.D = 0, the processor is running in 64-bit mode*/
	callgate_code_segment_entry->l = BIT64_CODE_SEGMENT;
	callgate_code_segment_entry->db = DEFAULT_OPERtion_32_BIT_SEG;
	callgate_code_segment_entry->g = GRANULARITY;
	callgate_code_segment_entry->base3 = BASE3;
	callgate_code_segment_entry->base4 = BASE4;

	lgdt(&gdt);

	target_sel = 0x50 << 16;

	asm volatile(ASM_TRY("1f")
		"lcallw *%0\n\t"
		"1:"
		::"m"(target_sel));

	ret = exception_vector();
	report("\t\t Segmentation_rqmid_28134_set_d_and_l_bits_in_segment_descriptor", ret == GP_VECTOR);
}

/**
 * @brief Case name: Segmentation ACRN hypervisor shall expose IA32_FS_BASE_001
 *
 * Summary: Execute WRMSR/RDMSR instruction writeread /IA32_FS_BASE  register shall have no exception
 *
 */
static void Segmentation_rqmid_28327_expose_fs_gs_msrs_ia32_fs_base(void)
{
	u32 index;
	u64 val, r_val;

	index = 0xC0000100;
	val = 0xdeadbeef;
	wrmsr(index, val);

	r_val = rdmsr(index);

	report("\t\t Segmentation_rqmid_28327_expose_fs_gs_msrs_ia32_fs_base", r_val == 0xdeadbeef);
}
#endif  /*__x86_64__*/

static void print_case_list(void)
{
	printf("\t\t Segmentation feature case list:\n\r");
#ifndef __x86_64__	/*386*/
	printf("\t\t Segmentation 32-Bits Mode:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 27133u, "Segmentation ACRN hypervisor \
		shall set initial guest IA32_KERNEL_GS_BASE to 0H following start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27186u, "Segmentation ACRN hypervisor \
		shall set initial guest SS to Selector_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27165u, "Segmentation When any vCPU \
		accesses data segment exclusive stack segment and P bit in data segment \
		exclusive stack segment descriptor is clear_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28300u, "Segmentation When any vCPU \
		loads SS register and the CPL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28056u, "Segmentation When any vCPU \
		accesses a nonconforming code segment using a call gatewith JMP \
		instruction and any following conditions is met_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28340u, "Segmentation When any vCPU \
		load LDT and base address in the segment descriptor is missing in memory_001");
#else			/*_x86_64__*/
	printf("\t\t Segmentation 64-Bits Mode:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 27772u, "Segmentation When any vCPU \
		accesses segment in IA-32e mode and memory address accessed bythe selector is non-canonical_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27794u, "Segmentation When any vCPU \
		accesses code segment in IA-32e mode and upper type field of the call \
		gate descriptor is greater than zero_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28134u, "Segmentation When any vCPU \
		accesses segment in IA-32e mode using gate and any of following condition is violated_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28327u, "Segmentation ACRN hypervisor \
		shall expose IA32_FS_BASE_001");
	printf("\t\t \n\r \n\r");
#endif
}

static void test_segmentation(void)
{
#ifndef __x86_64__	/*386*/
	enable_init();
	Segmentation_rqmid_27133_bp_startup_read_i32_kernel_gs_base();
	Segmentation_rqmid_27186_ap_read_ss_init_value();
	Segmentation_rqmid_27165_generate_NP_exclusive_stack();
	Segmentation_rqmid_28300_chaeck_DPL_is_3_of_stack_segment();
	Segmentation_rqmid_28056_call_gate_with_jmp_accesses_nonconforming_codesegment();
	Segmentation_rqmid_28340_loading_ldt_miss_base_address_generates_pf();
#else				/*x86_64__*/
	Segmentation_rqmid_27772_memory_address_accessed_non_canonical();
	Segmentation_rqmid_27794_set_non_zero_upper_type_in_call_gate_descriptor();
	Segmentation_rqmid_28134_set_d_and_l_bits_in_segment_descriptor();
	Segmentation_rqmid_28327_expose_fs_gs_msrs_ia32_fs_base();
#endif
}

int main(void)
{
	setup_vm();

#ifndef __x86_64__	/*i386*/
	extern unsigned char kernel_entry;
	set_idt_entry(0x20, &kernel_entry, 3);
#endif
	setup_idt();

	print_case_list();

	test_segmentation();

	return report_summary();
}

