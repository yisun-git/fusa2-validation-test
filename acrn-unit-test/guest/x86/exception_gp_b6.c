#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "misc.c"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "segmentation.h"
#include "interrupt.h"
#include "delay.h"
#include "pci_util.h"

#define CALL_GATE_SEL (0x40<<3)
#define CODE_SEL     (0x50<<3)
#define DATA_SEL     (0x54<<3)
#define CODE_2ND     (0x58<<3)
#define NON_CANINCAL_FACTOR	(0x8000000000000000UL)
#define USE_HAND_EXECEPTION
#define BIT_IS(NUMBER, N) ((((uint64_t)NUMBER) >> N) & (0x1))

typedef void *call_gate_fun;
extern gdt_entry_t gdt64[];

// define the data type
struct addr_m16_16_type {
	u16 offset;
	u16 selector;
};

struct addr_m16_32_type {
	u32 offset;
	u16 selector;
};

struct addr_m16_64_type {
	u64 offset;
	u16 selector;
};

struct addr_m16_16_type addr_m16_16;

struct addr_m16_32_type addr_m16_32;

struct addr_m16_64_type addr_m16_64;

__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(32))) __unused static union_unsigned_128 unsigned_128;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;

/** it is used for cases contain "asm volatile( "MOV %[input_1], %[output]"
 * "1:" :[output] "=r" (*(non_canon_align_mem((u64)&unsigned_16))):[input_1] "m" (unsigned_16));" like case 2
 * using method:
 * execption_inc_len = 3;
 * do_at_ring2(gp_b6_instruction_2, "");
 * report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
 * execption_inc_len = 0;
 */
static volatile u32 execption_inc_len = 0;
void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	if (execption_inc_len == 0) {
		unhandled_exception(regs, false);
	} else {
		regs->rip += execption_inc_len;
	}
}

void set_handle_exception(void)
{
	handle_exception(DE_VECTOR, &handled_exception);
	handle_exception(DB_VECTOR, &handled_exception);
	handle_exception(NMI_VECTOR, &handled_exception);
	handle_exception(BP_VECTOR, &handled_exception);
	handle_exception(OF_VECTOR, &handled_exception);
	handle_exception(BR_VECTOR, &handled_exception);
	handle_exception(UD_VECTOR, &handled_exception);
	handle_exception(NM_VECTOR, &handled_exception);
	handle_exception(DF_VECTOR, &handled_exception);
	handle_exception(TS_VECTOR, &handled_exception);
	handle_exception(NP_VECTOR, &handled_exception);
	handle_exception(SS_VECTOR, &handled_exception);
	handle_exception(GP_VECTOR, &handled_exception);
	handle_exception(PF_VECTOR, &handled_exception);
	handle_exception(MF_VECTOR, &handled_exception);
	handle_exception(AC_VECTOR, &handled_exception);
	handle_exception(MC_VECTOR, &handled_exception);
	handle_exception(XM_VECTOR, &handled_exception);
}

// unaligned address
static __unused u16 *unaligned_address_16(u64 *aligned_addr)
{
	u16 *non_aligned_addr = (u16 *)(((size_t)(aligned_addr)) | 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(u64 *aligned_addr)
{
	u32 *non_aligned_addr = (u32 *)(((size_t)(aligned_addr)) | 1);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(u64 *aligned_addr)
{
	u64 *non_aligned_addr = (u64 *)(((size_t)(aligned_addr)) | 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(union_unsigned_128 *addr)
{
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)(((size_t)(addr)) | 1);
	return non_aligned_addr;
}

// non canon mem aligned address
//__attribute__ ((aligned(16))) static u64 addr_64 = 0;
static __unused u64 *non_canon_align_mem(u64 addr)
{
	u64 address = 0;
	if ((addr >> 63) & 1) {
		address = (addr & (~(1ull << 63)));
	} else {
		address = (addr | (1UL<<63));
	}
	return (u64 *)address;
}

// trigger_pgfault
static __unused u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (add1 == NULL) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return add1;
}

// define called function
void called_func()
{
	return;
}

// define jmp function
void jmp_func()
{
	return;
}

static void init_call_gate_64(u32 index, u32 code_selector, u8 type, u8 dpl, u8 p, bool is_non_canonical,
	call_gate_fun fun)
{
	int num = index>>3;
	uint64_t call_base = (uint64_t)fun;
	struct gate_descriptor *call_gate_entry = NULL;

	call_gate_entry = (struct gate_descriptor *)&gdt64[num];
	call_gate_entry->gd_looffset = call_base&0xFFFF;

	if (is_non_canonical == 0) {
		if (BIT_IS(call_base, 47)) { //Modified manually: 48 -> 47
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF) | (0xFFFF00000000);
		} else {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF) | (0x000000000000);
		}
	} else {
		if (BIT_IS(call_base, 47)) { //Modified manually: 48 -> 47
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF) | (0x000000000000);
		} else {
			call_gate_entry->gd_hioffset = ((call_base >> 16)&0xFFFFFFFF) | (0xFFFF00000000);
		}
	}

	call_gate_entry->gd_selector = code_selector;
	call_gate_entry->gd_ist = IST_TABLE_INDEX;
	call_gate_entry->gd_xx = UNUSED;
	call_gate_entry->gd_type = type;
	call_gate_entry->gd_dpl = dpl;
	call_gate_entry->gd_p = p;
	call_gate_entry->sd_xx1 = 0x0;
}

void call_gate_function(void)
{
	int a = 0;
	a++;
}

asm("call_gate_entry_no_canonical: \n"
	"mov %rsp, %rdi\n"
	"mov %rsp, %rsi\n"
	"pushq %rax \n"
	"mov $0x8000000000000000, %rax \n"
	"xor %rax, %rdi \n"
	"popq %rax \n"
	"mov %rdi, %rsp \n"

#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $0, %gs:"xstr(EXCEPTION_ADDR)"\n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
#endif
	"call call_gate_function\n"
#ifdef USE_HAND_EXECEPTION
	"1:"
	// resume stacks
	"mov %rsi, %rsp\n"
	/*return to normal code stream*/
	"lretq\n"
#endif
	);

void call_gate_entry_no_canonical(void);

void dpl_call_gate(int ring, void *func)
{
	struct descriptor_table_ptr old_gdt_desc;
	u8 dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, ring, 1, 0, func);
	switch (ring) {
	case 0:
	dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	break;
	case 1:
	dpl = DESCRIPTOR_PRIVILEGE_LEVEL_1;
	break;
	case 2:
	dpl = DESCRIPTOR_PRIVILEGE_LEVEL_2;
	break;
	case 3:
	dpl = DESCRIPTOR_PRIVILEGE_LEVEL_3;
	break;
	default:
	dpl = DESCRIPTOR_PRIVILEGE_LEVEL_0;
	break;
	}
	/*config code segment*/
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|dpl|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);
}

void call_absolute_addr_m64(void)
{
	asm volatile(".byte 0x9a, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
}

void call_absolute_addr_rel32(void)
{
	asm volatile(".byte 0x9a, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
}

void test_call_absolute_addr_for_ud_r_m64(void)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)call_absolute_addr_m64;
	test_for_exception(UD_VECTOR, fun, NULL);
}

void test_call_absolute_addr_for_ud_rel32(void)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)call_absolute_addr_rel32;
	test_for_exception(UD_VECTOR, fun, NULL);
}

static u64 current_rip;
void make_stack_rip_non_canonical(void)
{
	u64 rsp_64 = 0;
	u64 *rsp_val;
	asm volatile("mov %%rdi, %0\n\t"
		: "=m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	current_rip = rsp_val[0];
	rsp_val[0] ^= NON_CANINCAL_FACTOR;
}

asm("ret_imm16_gp_entry:\n"
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call make_stack_rip_non_canonical\n"
	"ret\n"
);
void ret_imm16_gp_entry(void);

void called_func_ad_no_mem(void)
{
	u64 rsp_64 = 0;
	u64 *rsp_val;
	u64 a = 0;
	u64 b = 1;
	a++;
	b++;
	asm volatile("mov %%rsp, %0\n\t"
		: "=m"(rsp_64)
		);
	rsp_val = (u64 *)rsp_64;
	rsp_val[0] ^= NON_CANINCAL_FACTOR;
}

int switch_to_ring3(void (*fn)(const char *), const char *arg, void (*modify)(void), void (*recovery)(void))
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

void normal_ring3_call_func(const char *arg)
{
}

void normal_modify_func(void)
{
}

void normal_recovery_func(void)
{
}

void set_ring3_cs_desc(void)
{
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_3|
			DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
			GRANULARITY_SET | L_64_BIT_CODE_SEGMENT | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
	lgdt(&old_gdt_desc);
}

#define BASE_IRET_OFFSET (0)
u64 orig_cs = 0;
u64 *rsp_val = NULL;
u64 rsp_64 = 0;
static void modify_cs_segment(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		: "=m"(rsp_64)
		:
		);
	rsp_val = (u64 *)rsp_64;
	orig_cs = rsp_val[1 + BASE_IRET_OFFSET];
	rsp_val[1 + BASE_IRET_OFFSET] = CODE_SEL + 3;
}

static void recovery_user_cs(void)
{
	rsp_val[1 + BASE_IRET_OFFSET] = orig_cs;
}

static int switch_to_ring3_pf(void (*fn)(const char *), const char *arg, void (*modify)(void), void (*recovery)(void))
{
	unsigned char *user_stack = (unsigned char *)malloc(4096);
	int ret;
	set_page_control_bit((void *)user_stack, PAGE_PTE, PAGE_P_FLAG, 0, true);

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
				[user_stack_top]"m"(user_stack[4095]),
				[modify]"m"(modify),
				[fn]"m"(fn),
				[recovery]"m"(recovery),
				[arg]"D"(arg),
				[kernel_entry_vector]"i"(0x23)
				: "rcx", "rdx");
	set_page_control_bit((void *)user_stack, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(user_stack);
	return ret;
}

int switch_to_ring3_ac(void (*fn)(const char *), const char *arg, void (*modify)(void), void (*recovery)(void))
{
	int ret;
	static char user_stack[4096] __attribute__((aligned(16)));
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
				[user_stack_top]"m"(user_stack[4095]),
				[modify]"m"(modify),
				[fn]"m"(fn),
				[recovery]"m"(recovery),
				[arg]"D"(arg),
				[kernel_entry_vector]"i"(0x23)
				: "rcx", "rdx");
	return ret;
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
u64 ori_rip;
void intr_gate_function15_3(void)
{
	asm volatile("mov %%rdi, %0\n\t"
		: "=m"(rsp_64)
		:
		);
	rsp_val = (u64 *)rsp_64;
	ori_rip = rsp_val[0];
	rsp_val[0] ^= NON_CANINCAL_FACTOR;
}

void intr_gate_function_end_15_3(void)
{
	rsp_val[0] = ori_rip;
}

asm("intr_gate_normal:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call normal_modify_func\n"
	#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $"xstr(NO_EXCEPTION)", %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
	#endif
	"iretq\n"
	#ifdef USE_HAND_EXECEPTION
	"1:\n"
	/*resume code segment selector or stack*/
	"call normal_recovery_func\n"
	"iretq\n"
	#endif
	);

void intr_gate_normal(void);
void non_mem_modify_func(void)
{
	volatile u64 a = 0;
	a += 1;
	asm volatile("mov %%rsp, %0 \n\t"
		: "=m"(rsp_64)
		:
	);
	rsp_64 ^= NON_CANINCAL_FACTOR;
	asm volatile("mov %0, %%rsp \n\t"
		:
		: "m"(rsp_64)
	);
}

void non_mem_recovery_func(void)
{
}

asm("intr_gate_non_mem_canonical:\n"
	/*save stack*/
	"mov %rsp, %rdi\n"
	"mov %rbp, %rsi\n"
	"call non_mem_modify_func\n"
	#ifdef USE_HAND_EXECEPTION
	/*ASM_TRY("1f") macro expansion*/
	"movl $"xstr(NO_EXCEPTION)", %gs:"xstr(EXCEPTION_ADDR)" \n"
	".pushsection .data.ex \n"
	".quad 1111f,  1f \n"
	".popsection \n"
	"1111:\n"
	#endif
	"iretq\n"
	#ifdef USE_HAND_EXECEPTION
	"1:\n"
	/*resume code segment selector or stack*/
	"call non_mem_recovery_func\n"
	"iretq\n"
	#endif
	);
void intr_gate_non_mem_canonical(void);

void test_call_offset_operand_nc(void)
{
	struct addr_m16_64_type ct;
	uint64_t offset = (uint64_t)called_func;
	offset ^= NON_CANINCAL_FACTOR;
	ct.offset = offset;
	ct.selector = CODE_SEL;
	asm volatile(ASM_TRY("1f") "call *%0 \n" "1:" : : "m"(ct));
}

//Added manually
//--------------------------------------------
#define NON_CANO_PREFIX_MASK (0xFFFFUL << 48) /*bits [63:48] */
#define EXCEPTION_IST 1
#define OFFSET_IST 5
#define AVL_SET 0x1
static char exception_stack[0x1000] __attribute__((aligned(0x10)));
static char rspx_stack[0x1000] __attribute__((aligned(0x10)));

tss64_t *set_ist(ulong non_cano_rsp)
{
	u64 ex_rsp = (u64)(exception_stack + sizeof(exception_stack));
	u64 rspx = ((non_cano_rsp > 0) ? non_cano_rsp : (u64)(rspx_stack + sizeof(rspx_stack)));
	tss64_t *tss64 = &tss +  OFFSET_IST;
	tss64->ist1 = ex_rsp;
	tss64->ist2 = 0;
	tss64->ist3 = 0;
	tss64->ist4 = 0;
	tss64->ist5 = 0;
	tss64->ist6 = 0;
	tss64->ist7 = 0;
	tss64->rsp0 = rspx;
	tss64->rsp1 = rspx;
	tss64->rsp2 = rspx;
	tss64->res1 = 0;
	tss64->res2 = 0;
	tss64->res3 = 0;
	tss64->res4 = 0;
	tss64->iomap_base = (u16)sizeof(tss64_t);
	return tss64;
}

void setup_tss64(int dpl, ulong non_cano_rsp)
{
	tss64_t *tss64 = set_ist(non_cano_rsp);
	int tss_ist_sel = TSS_MAIN + (sizeof(struct segment_desc64)) * OFFSET_IST; //tss64
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt64_entry(tss_ist_sel, (u64)tss64, sizeof(tss64_t) - 1,
		SEGMENT_PRESENT_SET | DESCRIPTOR_TYPE_SYS | (dpl << 5) | SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT | AVL_SET);
	lgdt(&old_gdt_desc);
	ltr(tss_ist_sel);
}

int stack_will_switch;
ulong old_rsp;
ulong ret_rip;
static jmp_buf jmpbuf;
static u8 test_vector = NO_EXCEPTION;

void change_rsp_by_dpl(ulong dpl)
{
	if (stack_will_switch) {
		setup_tss64(dpl, 0);
	}
}

void longjmp_func(void)
{
	asm("movb %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(test_vector));
	longjmp(jmpbuf, 0xbeef);
}

void return_kernel_entry(void);
void ring0_non_cano_esp_handler(void);
void ringx_non_cano_esp_handler(void);
void ringx_int_handler(void);
asm (
"ring0_non_cano_esp_handler:\n"
	"movq old_rsp, %rax\n"
	"pop %rcx\n" //pop error code
	"mov %rsp, %rcx\n"
	"mov %rax, 24(%rcx)\n" //change rsp
	"movq $longjmp_func, (%rcx)\n" //change rip
	"iretq\n"
"ringx_non_cano_esp_handler:\n"
	"pop %rbx\n"
	"movb $" xstr(SS_VECTOR) ", %gs:" xstr(EXCEPTION_VECTOR_ADDR) "\n"
	"mov %bx, %gs:" xstr(EXCEPTION_ECODE_ADDR) "\n"
"ringx_int_handler:\n"
	"call change_rsp_by_dpl\n"
	"movq old_rsp, %rax\n"
	"mov %rsp, %rcx\n"
	"mov %rax, 24(%rcx)\n" //change rsp
	"mov ret_rip, %rax\n"
	"mov %rax, (%rcx)\n" //change rip
	"iretq\n"
);

extern gdt_entry_t gdt64[];
static char  ringx_user_stack[0x1000] __attribute__((aligned(0x10)));
static u16 cses[] = { KERNEL_CS, OSSERVISE1_CS32, OSSERVISE2_CS32, USER_CS };
static u16 dses[] = { KERNEL_DS, OSSERVISE1_DS, OSSERVISE2_DS, USER_DS };

void ringx_int_with_non_cano_rsp(u16 cpl, u16 dpl)
{
	#define KERNEL_INT 0x30
	#define RING_INT 0x40

	assert((cpl <= 3) && (dpl <= 3));

	ulong non_cano_rsp = 0, kernel_rsp = 0;
	ulong udpl = dpl;
	u16 ring_cs = cses[cpl];
	u16 ring_ds = dses[cpl];
	//u16 old_lr = str();
	stack_will_switch = (cpl != dpl);
	old_rsp = (ulong)(ringx_user_stack + sizeof(ringx_user_stack));
	asm volatile ("mov %%rsp, %[kernel_rsp]\n"	: [kernel_rsp]"=r"(kernel_rsp));
	// construct a non-canonical address
	non_cano_rsp =  old_rsp ^ NON_CANO_PREFIX_MASK;

	test_vector = SS_VECTOR;

	set_idt_entry_ex(RING_INT, ringx_int_handler, cses[dpl], cpl, 0);
	set_idt_entry(KERNEL_INT, return_kernel_entry, cpl);
	set_idt_entry_ex(test_vector, ringx_non_cano_esp_handler,
		(stack_will_switch ? cses[0] : cses[dpl]), dpl, EXCEPTION_IST);

	setup_tss64(cpl, (stack_will_switch ? non_cano_rsp : 0));

	// Enter ringx(ring1, 2, 3)
	asm volatile (
		"mov %[ring_ds], %%rdx\n"
		"mov %%dx, %%ds\n"
		"mov %%dx, %%es\n"
		"mov %%dx, %%fs\n"
		"mov %%dx, %%gs\n"
		"pushq %%rdx\n"
		"lea %[user_stack], %%rdx\n"
		"pushq %%rdx\n"
		"pushfq\n"
		"pushq %[ring_cs]\n"
		"pushq $1f\n"
		"iretq\n"
		"1: \n"
		:
		:  [ring_cs]"m"(ring_cs), [ring_ds]"m"(ring_ds), [user_stack]"m"(old_rsp)
		: "rdx"
	);

	// Test under ringx
	asm volatile (
		"movb $" xstr(NO_EXCEPTION) ", %%gs:"xstr(EXCEPTION_ADDR)"\n"
		"mov %[dpl], %%rdi\n"
		"mov $1f, %%rdx\n"
		"mov %%rdx, ret_rip\n"
		"movq %[non_cano_user_stack], %%rsp\n"
		"int %[ring_vector]\n"
		"1:\n"
		:
		:  [non_cano_user_stack]"r"(non_cano_rsp), [dpl]"m"(udpl), [ring_vector]"i"(RING_INT)
		: "rcx", "rdx", "memory"
		);

	// back to ring0
	asm volatile (
		"movq %[kernel_rsp], %%rcx\n"
		"mov $1f, %%rdx\n"
		"int %[kernel_vector]\n"
		".section .text.entry \n"
		".globl return_kernel_entry\n"
		"return_kernel_entry:\n"
		"mov %%rcx, %%rsp\n"
		"mov %[kernel_ds], %%cx\n"
		"mov %%cx, %%ds\n"
		"mov %%cx, %%es\n"
		"mov %%cx, %%fs\n"
		"mov %%cx, %%gs\n"
		"jmp *%%rdx \n"
		".section .text\n"
		"1:\n"
		:
		: [kernel_rsp]"m"(kernel_rsp), [kernel_ds]"i"(KERNEL_DS),  [kernel_vector]"i"(KERNEL_INT)
		: "rcx", "rdx", "memory"
		);

	//ltr(old_lr);//restore lr
}

u8 ring0_run_with_non_cano_rsp(void (*func)(void))
{
	int ret = 0;
	u64 non_cano_rsp = 0;
	//u16 old_lr = str();
	test_vector = SS_VECTOR;
	setup_tss64(0, 0);
	set_idt_entry_ist(test_vector, ring0_non_cano_esp_handler, 0, EXCEPTION_IST);
	ret = setjmp(jmpbuf);
	if (ret == 0) {
		asm("movb $" xstr(NO_EXCEPTION) ", %%gs:"xstr(EXCEPTION_ADDR)"\n" : );
		asm volatile("mov %%rsp, %0\n" : "=r"(old_rsp) : );
		non_cano_rsp = old_rsp ^ NON_CANO_PREFIX_MASK; // construct a non-canonical address
		asm volatile("movq %0, %%rsp\n" : : "r"(non_cano_rsp) : "memory");
		func();
		asm volatile("movq old_rsp, %rsp\n");
	}
	u8 vector = exception_vector();
	setup_idt(); //restore idt
	//ltr(old_lr); //restore lr
	return vector;
}

static void test_int(void)
{
	asm volatile("int $0x40\n");
}

static void test_call(void)
{
	int target_sel = (CALL_GATE_SEL + 0) << 16;
	asm volatile("lcallw *%0\n1:" : : "m"(target_sel));
}

static void test_enter(void)
{
	asm volatile(
		"enter $16, $0\n"
		"leave\n"
	);
}
//--------------------------------------------
//end

void test_gp_b6_instruction_6(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movq (%%rax), %%rax\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_gp_b6_instruction_7(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movb (%%rax), %%al\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_gp_b6_instruction_8(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movl %%eax, (%%rax)\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_gp_b6_instruction_9(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movw (%%rax), %%ax\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_gp_b6_instruction_10(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movl (%%rax), %%eax\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_gp_b6_instruction_11(void)
{
	u64 addr64 = 0UL;
	asm volatile("mov %%gs, %%rdx\n " : "=d"(addr64));
	u64 index = addr64 >> 3;
	addr64 = 0UL;
	addr64 = (gdt64[index].base_low) \
	| ((gdt64[index].base_middle) << 16) \
	| ((gdt64[index].base_high) << 24);
	addr64 += 0x3000;
	addr64 ^= NON_CANINCAL_FACTOR;
	asm volatile(ASM_TRY("1f") "movl $0x1, (%%rax)\n"
				:
				: "a"(addr64)
				: "memory");
}

void test_jmp_abs_addr_ud_can(void)
{
	asm volatile(".byte 0xea, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0x7e\n");
}

void test_jmp_abs_addr_ud_non_can(void)
{
	asm volatile(".byte 0xea, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe\n");
}

void test_int1_db(void)
{
	asm volatile(ASM_TRY("1f") "int $1\n" "1:" : : );
}

void make_descriptor_selector_nc(int vec, int dpl)
{
	set_gdt64_entry(CODE_SEL, NON_CANINCAL_FACTOR, -1, 0x93, 0);
	set_idt_entry(vec, called_func, dpl);
	set_idt_sel(vec, CODE_SEL);
}

void test_leave_AC(void)
{
	asm volatile("enter $4096, $0\n");
	asm volatile("sub $9, %rbp\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
}

void test_leave_SS(void)
{
	asm volatile("enter $4096, $0\n");
	asm volatile("mov $0x8000000000000000, %rdi\n"
				"xor %rdi, %rbp\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
}

static __unused void gp_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %[input_1], %[output]\n" "1:"
		: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "r" (unsigned_8));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVB $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_16)))
		: [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVB $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV $1, %[output]\n" "1:"
				: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_16))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %[input_1], %[output]\n" "1:"
		: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "r" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_6(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_6;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_7(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_7;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_8(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_8;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_9(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_9;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_10(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_10;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_11(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_gp_b6_instruction_11;
	test_for_exception(GP_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "mov %0, %%cs \n" "1:" : : "r" (0x8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVW $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVB %[input_1], %%gs:0x3000\n" "1:"
				: : [input_1] "a" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %%ss, %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_21(const char *msg)
{
	unsigned_64 = USER_DS;
	asm volatile(ASM_TRY("1f") "MOV %[input_1], %%ss\n" "1:"
				: : [input_1] "r" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVL $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : [input_1] "r" (unsigned_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_24(const char *msg)
{
	u64 r_b = 1;
	asm volatile ("mov %%cr3, %0\n" : : "r" (r_b));
	asm volatile (ASM_TRY("1f") "REX.R mov %0, %%cr3\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_25(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_26(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_27(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_28(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_29(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr8();
	check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR8_RESEVED_BIT_4, FEATURE_INFORMATION_04));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr8\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_30(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr8();
	check_bit |= (FEATURE_INFORMATION_BIT_RANGE(CR8_RESEVED_BIT_4, FEATURE_INFORMATION_04));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr8\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

/**
 * Modified manually: Set the reserved bit in CR3, such as CR3[0:2] or [5:11]
 * Covered the removed case: 31398
 */
static __unused void gp_b6_instruction_31(const char *msg)
{
	ulong old_cr3 = read_cr3();
	ulong cr3 = old_cr3 | 0x02;
	//printf("old_cr3=0x%lx, cr3=0x%lx\n", old_cr3, cr3);
	asm volatile (ASM_TRY("1f") "mov %0, %%cr3\n" "1:" : : "r"(cr3) : "memory");
	//printf("new_cr3=0x%lx\n", read_cr3());
	write_cr3(old_cr3); //restore
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_32(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr3();
	check_bit |= 1UL << 63;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr3\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_33(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_34(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_05)));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_35(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_36(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_37(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr3\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_38(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr3\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_39(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_40(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_41(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr5\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_42(const char *msg)
{
	u64 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr5\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_43(const char *msg)
{
	u32 cr0 = read_cr0();
	cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(cr0) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_44(const char *msg)
{
	u32 cr0 = read_cr0();
	cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(cr0) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_45(const char *msg)
{
	u32 cr3 = read_cr3();
	cr3 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_08));
	write_cr3(cr3);
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_46(const char *msg)
{
	u32 cr3 = read_cr3();
	cr3 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_08));
	write_cr3(cr3);
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_47(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit &= (~FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_48(const char *msg)
{
	u32 c = cpuid(1).c;
	if (!(c & SHIFT_LEFT(1U, 17))) {
		printf("\nCR4.PCIDE is not supported\n");
	} else {
		u32 cr3 = read_cr3();
		cr3 &= SHIFT_UMASK(11, 0);
		write_cr3(cr3);
		unsigned long check_bit = 0;
		check_bit = read_cr4();
		check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
		asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	}
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_49(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit &= (~FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_50(const char *msg)
{
	u32 c = cpuid(1).c;
	if (!(c & SHIFT_LEFT(1U, 17))) {
		printf("\nCR4.PCIDE is not supported\n");
	} else {
		u32 cr3 = read_cr3();
		cr3 &= SHIFT_UMASK(11, 0);
		write_cr3(cr3);
		unsigned long check_bit = 0;
		check_bit = read_cr4();
		check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
		asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	}
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_51(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "REX.R mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_52(const char *msg)
{
	unsigned long check_bit = read_dr6();
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);
	asm volatile(ASM_TRY("1f") "mov %0, %%dr6\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_53(const char *msg)
{
	unsigned long check_bit = read_dr7();
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_32);
	asm volatile(ASM_TRY("1f") "mov %0, %%dr7\n" "1:" : : "r"(check_bit) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_54(const char *msg)
{
	unsigned long check_bit = read_dr7();
	u64 r_a = 1;
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));
	asm volatile ("mov %0, %%dr7\n" ASM_TRY("1f") "mov %1, %%dr6\n" "1:" : : "r"(check_bit), "r"(r_a) : "memory");
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_55(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_56(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_57(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_58(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_59(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_60(const char *msg)
{
	u64 r_b = 0;
	asm volatile(ASM_TRY("1f") "mov %%dr0, %[output] \n" "1:" : [output] "=r" (r_b) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"AND $1, %%ax\n" "1:"
				: [output] "=a" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_62(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (*(unaligned_address_64(array_64))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"AND $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_64(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"ADD %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_32)))
		: [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_65(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"ADD %[input_1], %[output]\n" "1:"
		: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "r" (unsigned_8));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_66(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"ADD %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_16)))
		: [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_67(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"ADD %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_68(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"ADD %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_69(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : [input_1] "r" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_70(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_8) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_71(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_8) : [input_1] "r" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_72(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_8) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_73(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_74(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADD $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_75(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC %[input_1], %[output]\n" "1:"
				: [output] "=r" (*(trigger_pgfault())) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADD $1, %%ax\n" "1:"
				: [output] "=a" (*(unaligned_address_16((u64 *)&unsigned_16))) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADD $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_16))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_79(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_32))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_80(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_16))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"lock\n"
		"ADC %[input_1], %[output]\n" "1:"
		: [output] "=r" (*(non_canon_align_mem((u64)&unsigned_8)))
		: [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_8))) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_8) : [input_1] "r" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_85(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : [input_1] "r" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_86(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_87(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_88(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : [input_1] "r" (unsigned_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_89(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_90(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_91(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_64((u64 *)&unsigned_64))) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_92(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_64))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_93(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_64))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_94(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_64))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_95(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_64))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_96(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_mem((u64)&unsigned_64))) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_97(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_128))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_98(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_99(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_100(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_101(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_102(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_103(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG16B %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_128((union_unsigned_128 *)&unsigned_128))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_104(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_105(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_106(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_107(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_108(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_109(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_110(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_111(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_112(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_113(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_114(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_115(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_116(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "r" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_117(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH $1\n" "1:"
				: : );
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_118(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %%gs\n" "1:"
				: : );
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_119(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSHW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	asm volatile("POPW %[output]\n" : [output] "=r" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_120(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "r" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_121(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "r" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_122(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "r" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_123(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_124(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH %[input_1]\n" "1:"
				: : [input_1] "r" (unsigned_64));
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_125(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH $1\n" "1:"
				: : );
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_126(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH $1\n" "1:"
				: : );
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_127(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PUSH $1\n" "1:"
				: : );
	asm volatile("POP %[output]\n" : [output] "=r" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_128(const char *msg)
{
	u16 r_a = 0x2000;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_129(const char *msg)
{
	u16 r_a = 0x2000;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_130(const char *msg)
{
	u16 r_a = 0x2001;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_131(const char *msg)
{
	u16 r_a = 0x2001;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_132(const char *msg)
{
	u16 r_a = 0x2002;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_133(const char *msg)
{
	u16 r_a = 0x2002;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_134(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_135(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_136(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_137(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_138(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_139(const char *msg)
{
	u16 r_a = 0x2003;
	asm volatile("MOV %0, %%dx\n" "PUSH %%dx\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : "m" (r_a));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_140(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_141(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_142(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_143(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_144(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_145(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_146(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_147(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_148(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_149(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_150(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_151(const char *msg)
{
	asm volatile("PUSH %%fs\n" ASM_TRY("1f") "POP %%fs\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_152(const char *msg)
{
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_153(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULQ %[input_1]\n" "1:"
				: : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_154(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULW %[input_1]\n" "1:"
				: : [input_1] "m" (*(unaligned_address_16((u64 *)&unsigned_16))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_155(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_156(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_157(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_158(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_159(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_160(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_161(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_162(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_163(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_164(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_165(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_166(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_167(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_168(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_169(const char *msg)
{
	unsigned_64 = 0;
	asm volatile(ASM_TRY("1f") "DIVQ %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_170(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_171(const char *msg)
{
	unsigned_32 = 0;
	asm volatile(ASM_TRY("1f") "IDIVL %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_172(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_173(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_174(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_175(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_176(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_177(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_178(const char *msg)
{
	unsigned_32 = 0;
	asm volatile(ASM_TRY("1f") "DIVL %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_32));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_179(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	size_t val = 0;
	size_t val1 = 0;
	asm volatile("mov $0, %0\n mov $0, %1 \n" : "=a" (val), "=d"(val1));
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_180(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	size_t val = 0;
	size_t val1 = 0;
	asm volatile("mov $0, %0\n mov $0, %1 \n" : "=a" (val), "=d"(val1));
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_181(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_16 = 1;
	asm volatile(ASM_TRY("1f") "IDIVW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	unsigned_16 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_182(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	size_t val = 0;
	size_t val1 = 0;
	asm volatile("mov $0, %0\n mov $0, %1 \n" : "=a" (val), "=d"(val1));
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_183(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_64 = 1;
	asm volatile(ASM_TRY("1f") "IDIVQ %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_184(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_16 = 1;
	asm volatile(ASM_TRY("1f") "DIVW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	unsigned_16 = 0;
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_185(const char *msg)
{
	asm volatile(ASM_TRY("1f") "insw %%dx, %%es:(%%rdi) \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_186(const char *msg)
{
	asm volatile(ASM_TRY("1f") "INSW\n" "1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_187(const char *msg)
{
	asm volatile(ASM_TRY("1f") "insw %%dx, %%es:(%%rdi) \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_188(const char *msg)
{
	asm volatile(ASM_TRY("1f") "OUTSW\n" "1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_189(const char *msg)
{
	asm volatile(ASM_TRY("1f") "insw %%dx, %%es:(%%rdi) \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_190(const char *msg)
{
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_191(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SETA %[input_1]\n" "1:"
				: : [input_1] "m" (*(non_canon_align_mem((u64)&unsigned_8))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_192(const char *msg)
{
	asm volatile(ASM_TRY("1f") "enter $4096, $1\n" "1:" : : );
	asm volatile("leave \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_193(const char *msg)
{
	test_call_offset_operand_nc();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_194(const char *msg)
{
	test_call_offset_operand_nc();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_195(const char *msg)
{
	test_call_offset_operand_nc();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_196(const char *msg)
{
	test_call_offset_operand_nc();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_197(const char *msg)
{
	test_call_offset_operand_nc();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_198(const char *msg)
{
	test_call_offset_operand_nc();
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_199(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_200(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_201(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_202(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_203(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_204(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_205(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_206(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_207(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_208(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_209(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_can, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_210(const char *msg)
{
	//Modified manually
	test_for_exception(UD_VECTOR, (gp_trigger_func)test_jmp_abs_addr_ud_non_can, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_211(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_212(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_213(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_214(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_215(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_216(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_217(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_218(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_219(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_220(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_221(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_222(const char *msg)
{
	u64 *ptr = (u64 *)jmp_func;
	asm volatile("push $1f\n" ASM_TRY("1f") "rex.W\n\t" "jmp *%0 \n" "1:\n" : : "m"(ptr));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_223(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	u8 vector = ring0_run_with_non_cano_rsp(test_call);
	report("%s", (vector == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_224(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 1) << 16;
	asm volatile("lcallw *%0\n\t" : : "m"(target_sel));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_225(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 2) << 16;
	asm volatile("lcallw *%0\n\t" : : "m"(target_sel));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_226(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t" : : "m"(target_sel));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_227(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t" : : "m"(target_sel));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_228(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile("lcallw *%0\n\t" : : "m"(target_sel));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_229(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_r_m64();
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_230(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_rel32();
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_231(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_r_m64();
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_232(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_r_m64();
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_233(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_rel32();
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_234(const char *msg)
{
	//Modified manually
	test_call_absolute_addr_for_ud_r_m64();
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_235(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_236(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_237(const char *msg)
{
	struct addr_m16_64_type call_test;
	call_test.offset = (uint64_t)called_func;
	call_test.selector = 0x1 << 3;
	asm volatile(ASM_TRY("1f") "call *%0 \n" "1:" : : "m"(call_test));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_238(const char *msg)
{
	int target_sel = (CALL_GATE_SEL + 3) << 16;
	asm volatile(ASM_TRY("1f") "lcallw *%0\n\t" "1:" : : "m"(target_sel));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_239(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_240(const char *msg)
{
	uint64_t offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "call *%0 \n" : : "m"(offset));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_241(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_242(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_243(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_244(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_245(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_246(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)ret_imm16_gp_entry;
	test_for_exception(GP_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_247(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_248(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_249(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_250(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_251(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_252(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_253(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_254(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_255(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_256(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_257(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func \n" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_258(const char *msg)
{
	asm volatile(ASM_TRY("1f") "call called_func_ad_no_mem \n" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_259(const char *msg)
{
	switch_to_ring3(normal_ring3_call_func, NULL, normal_modify_func, normal_recovery_func);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_260(const char *msg)
{
	switch_to_ring3(normal_ring3_call_func, NULL, modify_cs_segment, recovery_user_cs);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_261(const char *msg)
{
	switch_to_ring3_pf(normal_ring3_call_func, NULL, normal_modify_func, normal_recovery_func);
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_262(const char *msg)
{
	switch_to_ring3_ac(normal_ring3_call_func, NULL, normal_modify_func, normal_recovery_func);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_263(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_264(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_265(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_266(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_267(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_268(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_269(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_270(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_271(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_272(const char *msg)
{
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_273(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_274(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_275(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_276(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_277(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_278(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_279(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_280(const char *msg)
{
	asm volatile("int $0x40 \n\t");
	test_delay(1);
	exception_vector_bak = exception_vector();
}

//Modified manually: reconstruct the case totally.
struct far_pointer {
	ulong offset;
	u16 selector;
} __attribute__((packed));

static __unused void gp_b6_instruction_281(const char *msg)
{
		ulong offset = (uintptr_t)&&end;
		offset ^= NON_CANO_PREFIX_MASK; // construct a non-canonical address

		struct far_pointer fp = {
			.offset = offset,
			.selector = KERNEL_CS,
		};

		asm goto(
			"mov $0xFF, %%ax\n"
			 "cmp $0, %%ax\n"
			 ASM_TRY("1f")
			 "jmp *%0\n"
			 "1:\n"
			 : : "m" (fp) : "ax" : end);

end:
	printf("vector=%d\n", exception_vector());
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}
//end

static __unused void gp_b6_instruction_282(const char *msg)
{
	asm volatile("mov $3, %%rax\n" "cmp $5, %%rax\n" "ja_tag220: nop\n" : : );
	asm volatile(ASM_TRY("1f") "ja ja_tag220\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_283(const char *msg)
{
	asm volatile("mov $3, %%rax\n" "cmp $5, %%rax\n" "ja_tag65: nop\n" : : );
	asm volatile(ASM_TRY("1f") "ja ja_tag65\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

//Modified manually: reconstruct the case totally.
void create_func_given_addr(u8 *dest)
{
	u8 *dst = dest;
	u8 func_ins[] = "\x48\xc7\xc1\x02\x00\x00\x00" //mov    $0x2,%rcx
	"\x65\xc7\x04\x25\x04\x10\x00\x00\xff\x00\x00\x00" //movl   $0xff,%gs:0x1004
	"\xe0\x7f" //loopne <tag> -- f2
	"\xc3";	 //retq
	int len = sizeof(func_ins);
	u8 *src = func_ins;
	while (len > 0) {
		*dst++ = *src++;
		len--;
	}
}

void virt_addr_page_map(void *vir_addr) {
	void *va = (void *)((u64)vir_addr & (-PAGE_SIZE));
	phys_addr_t pa = virt_to_phys(alloc_page());
	install_page(phys_to_virt(read_cr3()), pa, va);
}

void trigger_func(void *data) {
	int target_sel = (int)(u64)data;
	asm volatile("lcallw *%0\n"	::"m"(target_sel));
}

static __unused void gp_b6_instruction_284(const char *msg)
{
	int target_sel = CALL_GATE_SEL << 16;
	struct descriptor_table_ptr old_gdt_desc;

	u8 *func_addr = (u8 *)0x00007fffffffffe0;
	virt_addr_page_map(func_addr);

	sgdt(&old_gdt_desc);
	init_call_gate_64(CALL_GATE_SEL, CODE_SEL, 0xC, 0, 1, 0,  func_addr);
	/*not for a conforming-code segment, nonconforming-code segment, 64-bit call gate */
	set_gdt_entry(CODE_SEL, 0, SEGMENT_LIMIT_ALL, SEGMENT_PRESENT_SET|DESCRIPTOR_PRIVILEGE_LEVEL_0|
		DESCRIPTOR_TYPE_CODE_OR_DATA|SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET|L_64_BIT_CODE_SEGMENT);
	lgdt(&old_gdt_desc);

	create_func_given_addr((u8 *)func_addr);
	test_for_exception(GP_VECTOR, trigger_func, (void *)(u64)target_sel);

	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_285(const char *msg)
{
	asm volatile("mov $1, %%rcx\n" "mov $2, %%rax\n" "loop_tag965: add %%rax, %%rax\n" : : );
	asm volatile(ASM_TRY("1f") "loop loop_tag965\n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_286(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_287(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_288(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_289(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_290(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_291(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_292(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_293(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_294(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_295(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	u8 vector = ring0_run_with_non_cano_rsp(test_int);
	report("%s", (vector == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_296(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(1, 1);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_297(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(2, 2);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_298(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 3);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_299(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 3);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_300(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 3);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_301(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 0);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_302(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 1);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_303(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	ringx_int_with_non_cano_rsp(3, 2);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_304(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_305(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_306(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_307(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_308(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_309(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_310(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_311(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_312(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_313(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_314(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_315(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_316(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_317(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_318(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_319(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_320(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_321(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_int1_db;
	test_for_exception(DB_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_322(const char *msg)
{
	asm volatile(ASM_TRY("1f") "lea %[input_1], %%rdi\n" ".byte 0x48, 0xAF \n" "1:"
				: : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_323(const char *msg)
{
	asm volatile(ASM_TRY("1f") "pushf \n" "1:" : : );
	asm volatile("popf \n");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_324(const char *msg)
{
	asm volatile("pushf \n");
	asm volatile(ASM_TRY("1f") "popf \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_325(const char *msg)
{
	asm volatile(ASM_TRY("1f") "pushfq \n" "1:" : : );
	asm volatile("popfq \n");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_326(const char *msg)
{
	asm volatile("pushfq \n");
	asm volatile(ASM_TRY("1f") "popfq \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_327(const char *msg)
{
	asm volatile(ASM_TRY("1f") "lea %[input_1], %%rdi\n" ".byte 0x48, 0xAF \n" "1:"
				: : [input_1] "m" (unsigned_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_328(const char *msg)
{
	//Modified manually: reconstruct the case totally.
	u8 vector = ring0_run_with_non_cano_rsp(test_enter);
	report("%s", (vector == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_329(const char *msg)
{
	asm volatile(ASM_TRY("1f") "enter $4096, $0\n" "1:" : : );
	asm volatile("leave \n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_330(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_AC;
	test_for_exception(AC_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_331(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_332(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_333(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_334(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_335(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_336(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)test_leave_SS;
	test_for_exception(SS_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_337(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_338(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_339(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_340(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_341(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_342(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_343(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_344(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_345(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_346(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_347(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_348(const char *msg)
{
	asm volatile("enter $4096, $0\n");
	asm volatile(ASM_TRY("1f") "leave \n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_349(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SAHF\n" "1:"
				: : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_350(const char *msg)
{
	asm volatile(ASM_TRY("1f")  " .byte 0xF0, 0x9F \n" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_351(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SAHF\n" "1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_352(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_353(const char *msg)
{
	addr_m16_64.selector = 0x11;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_354(const char *msg)
{
	addr_m16_64.selector = 0x62;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_355(const char *msg)
{
	addr_m16_64.selector = 0x73;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_356(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_357(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_358(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_359(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_360(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_361(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_362(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_363(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_364(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_365(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_366(const char *msg)
{
	asm volatile(ASM_TRY("1f") "lss %0, %%ax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_16))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_367(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:"
		: : "m" (*(non_canon_align_mem((u64)&addr_m16_64))));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_368(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_369(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_370(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_371(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_372(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_373(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_374(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_375(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_376(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lgs %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_377(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_378(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_379(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_380(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_381(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_382(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_383(const char *msg)
{
	//Modified manually: Use the opcode of 'LFS r64,m16:64'
	// to construct a non-memory location.
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB4, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_384(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_385(const char *msg)
{
	asm volatile(ASM_TRY("1f") "REX.W\n\t" ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_b6_instruction_386(const char *msg)
{
	addr_m16_64.selector = 0x10;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_387(const char *msg)
{
	addr_m16_64.selector = 0x61;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_388(const char *msg)
{
	addr_m16_64.selector = 0x72;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_389(const char *msg)
{
	addr_m16_64.selector = 0x43;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_390(const char *msg)
{
	addr_m16_64.selector = 0x43;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_391(const char *msg)
{
	addr_m16_64.selector = 0x43;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_b6_instruction_392(const char *msg)
{
	addr_m16_64.selector = 0x10;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_393(const char *msg)
{
	addr_m16_64.selector = 0x61;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_instruction_394(const char *msg)
{
	addr_m16_64.selector = 0x72;
	addr_m16_64.offset = (uint64_t)called_func;
	asm volatile(ASM_TRY("1f") "REX.W\n\t" "lss %0, %%rax \n" "1:" : : "m" (addr_m16_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_5, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_mem_descriptor_nc_hold();
	do_at_ring0(gp_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_mem_descriptor_nc_hold();
	do_at_ring1(gp_b6_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_mem_descriptor_nc_hold();
	do_at_ring2(gp_b6_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_mem_descriptor_nc_hold();
	do_at_ring3(gp_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_mem_descriptor_nc_hold();
	do_at_ring3(gp_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_mem_descriptor_nc_hold();
	do_at_ring3(gp_b6_instruction_11, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring0(gp_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring1(gp_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring2(gp_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring3(gp_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring3(gp_b6_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CSLoad_true();
	do_at_ring3(gp_b6_instruction_17, "");
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring0(gp_b6_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring1(gp_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring2(gp_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring3(gp_b6_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring3(gp_b6_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CSLoad_false();
	do_at_ring3(gp_b6_instruction_23, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_not_used();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_24, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_0();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_25, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_0();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_26, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_27, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_28, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_29, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_30, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_31, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_32, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_0();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_33, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_0();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_34, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_35, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_36, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_37, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_38, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_39, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_40, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_41, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_42, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_43, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_44, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_true();
	condition_CR3NOT0_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_45, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_true();
	condition_CR3NOT0_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_46, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_false();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_47, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_true();
	condition_CR3NOT0_false();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_48, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_false();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_49, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	condition_CR8_used();
	condition_CR0_PG_1();
	condition_CR4_R_W_0();
	condition_CR8_R_W_0();
	condition_CR3_R_W_0();
	condition_CR4_PAE_1();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	condition_CR4_PCIDE_UP_true();
	condition_CR3NOT0_false();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_50, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_hold();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_51, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_1();
	do_at_ring0(gp_b6_instruction_52, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_53, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_true();
	condition_DR7_GD_1();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_54, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_false();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_55, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_true();
	condition_DR7_GD_0();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_56, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_false();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_57, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_true();
	condition_DR7_GD_0();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_58, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_false();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_59, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_REXR_not_hold();
	condition_DR6_0();
	condition_DR7_0();
	condition_DeAc_true();
	condition_DR7_GD_0();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_60, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_61, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_62, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_63, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_64, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	do_at_ring2(gp_b6_instruction_65, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_66, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_67, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_68, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_69, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_70, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_71, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_72, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_73, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_74(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_74, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_75, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_76, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_77, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_78, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_79(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	do_at_ring2(gp_b6_instruction_79, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_80(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_80, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_81(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_81, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_82(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_82, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_83(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_83, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_84, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_85, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_86, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_87, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_88(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_88, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_89(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_0();
	do_at_ring0(gp_b6_instruction_89, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_90(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_90, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_91(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_91, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_92(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_92, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_93(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_93, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_94(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	do_at_ring2(gp_b6_instruction_94, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_95(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_95, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_96(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_96, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_97(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_97, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_98(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring0(gp_b6_instruction_98, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_99(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring1(gp_b6_instruction_99, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_100(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring2(gp_b6_instruction_100, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_101(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring3(gp_b6_instruction_101, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_102(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring3(gp_b6_instruction_102, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_103(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_false();
	do_at_ring3(gp_b6_instruction_103, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_104(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring0(gp_b6_instruction_104, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_105(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring1(gp_b6_instruction_105, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_106(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring2(gp_b6_instruction_106, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_107(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring3(gp_b6_instruction_107, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_108(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring3(gp_b6_instruction_108, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_109(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_false();
	do_at_ring3(gp_b6_instruction_109, "");
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_110(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring0(gp_b6_instruction_110, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_111(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring1(gp_b6_instruction_111, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_112(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring2(gp_b6_instruction_112, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_113(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring3(gp_b6_instruction_113, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_114(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring3(gp_b6_instruction_114, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_115(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_CMPXCHG16B_set_to_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CMPXCHG16B_Aligned_true();
	condition_MemLocat_true();
	do_at_ring3(gp_b6_instruction_115, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_116(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring0(gp_b6_instruction_116, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_117(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Push_Seg_not_hold();
	do_at_ring0(gp_b6_instruction_117, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_118(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring1(gp_b6_instruction_118, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_119(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Push_Seg_not_hold();
	do_at_ring1(gp_b6_instruction_119, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_120(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring2(gp_b6_instruction_120, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_121(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Push_Seg_not_hold();
	do_at_ring2(gp_b6_instruction_121, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_122(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_122, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_123(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_123, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_124(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_124, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_125(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_125, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_126(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_126, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_127(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Push_Seg_not_hold();
	do_at_ring3(gp_b6_instruction_127, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_128(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring0(gp_b6_instruction_128, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_129(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Descriptor_limit_outside();
	do_at_ring0(gp_b6_instruction_129, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_130(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring1(gp_b6_instruction_130, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_131(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Descriptor_limit_outside();
	do_at_ring1(gp_b6_instruction_131, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_132(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring2(gp_b6_instruction_132, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_133(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Descriptor_limit_outside();
	do_at_ring2(gp_b6_instruction_133, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_134(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_134, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_135(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_135, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_136(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_136, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_137(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_137, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_138(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_138, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_139(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Descriptor_limit_outside();
	do_at_ring3(gp_b6_instruction_139, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_140(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring0(gp_b6_instruction_140, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_141(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Descriptor_limit_inside();
	do_at_ring0(gp_b6_instruction_141, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_142(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring1(gp_b6_instruction_142, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_143(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Descriptor_limit_inside();
	do_at_ring1(gp_b6_instruction_143, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_144(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring2(gp_b6_instruction_144, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_145(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Descriptor_limit_inside();
	do_at_ring2(gp_b6_instruction_145, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_146(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_146, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_147(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_147, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_148(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_148, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_149(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_149, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_150(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_150, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_151(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Descriptor_limit_inside();
	do_at_ring3(gp_b6_instruction_151, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_152(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	do_at_ring0(gp_b6_instruction_152, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_153(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_153, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_154(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_154, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_155(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_155, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_156(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_156, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_157(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	do_at_ring2(gp_b6_instruction_157, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_158(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_158, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_159(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_159, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_160(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_160, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_161(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_161, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_162(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_162, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_163(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_163, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_164(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_164, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_165(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_165, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_166(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_166, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_167(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring0(gp_b6_instruction_167, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_168(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring1(gp_b6_instruction_168, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_169(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring2(gp_b6_instruction_169, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_170(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring3(gp_b6_instruction_170, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_171(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring3(gp_b6_instruction_171, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_172(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Divisor_0_true();
	do_at_ring3(gp_b6_instruction_172, "");
	report("%s", (exception_vector_bak == DE_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_173(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring0(gp_b6_instruction_173, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_174(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring1(gp_b6_instruction_174, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_175(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring2(gp_b6_instruction_175, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_176(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_b6_instruction_176, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_177(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_b6_instruction_177, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_178(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_b6_instruction_178, "");
	report("%s", (exception_vector_bak == DE_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_179(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring0(gp_b6_instruction_179, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_180(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring1(gp_b6_instruction_180, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_181(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring2(gp_b6_instruction_181, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_182(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_b6_instruction_182, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_183(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_b6_instruction_183, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_184(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_b6_instruction_184, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_185(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_185, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_186(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_186, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_187(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_187, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_188(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_188, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_189(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_189, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_190(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_190, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_191(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_191, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_192(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_192, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_193(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_offset_operand_nc_hold();
	dpl_call_gate(0, called_func);
	do_at_ring0(gp_b6_instruction_193, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_194(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_offset_operand_nc_hold();
	dpl_call_gate(1, called_func);
	do_at_ring1(gp_b6_instruction_194, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_195(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_offset_operand_nc_hold();
	dpl_call_gate(2, called_func);
	do_at_ring2(gp_b6_instruction_195, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_196(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_offset_operand_nc_hold();
	dpl_call_gate(3, called_func);
	do_at_ring3(gp_b6_instruction_196, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_197(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_offset_operand_nc_hold();
	dpl_call_gate(3, called_func);
	do_at_ring3(gp_b6_instruction_197, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_198(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_offset_operand_nc_hold();
	dpl_call_gate(3, called_func);
	do_at_ring3(gp_b6_instruction_198, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_199(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring0(gp_b6_instruction_199, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_200(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring0(gp_b6_instruction_200, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_201(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring1(gp_b6_instruction_201, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_202(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring1(gp_b6_instruction_202, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_203(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring2(gp_b6_instruction_203, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_204(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring2(gp_b6_instruction_204, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_205(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_205, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_206(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_206, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_207(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_207, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_208(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_208, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_209(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_209, "");
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_210(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_true();
	do_at_ring3(gp_b6_instruction_210, "");
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_211(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring0(gp_b6_instruction_211, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_212(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring0(gp_b6_instruction_212, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_213(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring1(gp_b6_instruction_213, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_214(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring1(gp_b6_instruction_214, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_215(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring2(gp_b6_instruction_215, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_216(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring2(gp_b6_instruction_216, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_217(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_217, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_218(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_218, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_219(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_219, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_220(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_220, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_221(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_221, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_222(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	condition_JMP_Abs_address_false();
	do_at_ring3(gp_b6_instruction_222, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_223(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(0, called_func);
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_223, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_224(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(1, call_gate_entry_no_canonical);
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_224, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_225(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(2, call_gate_entry_no_canonical);
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_225, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_226(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(3, call_gate_entry_no_canonical);
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_226, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_227(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(3, call_gate_entry_no_canonical);
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_227, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_228(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	dpl_call_gate(3, call_gate_entry_no_canonical);
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_228, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == SS_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_229(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring0(gp_b6_instruction_229, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_230(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring1(gp_b6_instruction_230, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_231(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring2(gp_b6_instruction_231, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_232(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring3(gp_b6_instruction_232, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_233(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring3(gp_b6_instruction_233, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_234(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_true();
	do_at_ring3(gp_b6_instruction_234, "");
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_235(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	do_at_ring0(gp_b6_instruction_235, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_236(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	do_at_ring1(gp_b6_instruction_236, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_237(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	do_at_ring2(gp_b6_instruction_237, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_238(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	dpl_call_gate(3, called_func);
	do_at_ring3(gp_b6_instruction_238, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_239(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	do_at_ring3(gp_b6_instruction_239, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_240(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CALL_Abs_address_false();
	do_at_ring3(gp_b6_instruction_240, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_241(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_inst_nc_hold();
	do_at_ring0(gp_b6_instruction_241, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_242(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_inst_nc_hold();
	do_at_ring1(gp_b6_instruction_242, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_243(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_inst_nc_hold();
	do_at_ring2(gp_b6_instruction_243, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_244(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_inst_nc_hold();
	do_at_ring3(gp_b6_instruction_244, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_245(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_inst_nc_hold();
	do_at_ring3(gp_b6_instruction_245, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_246(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_inst_nc_hold();
	do_at_ring3(gp_b6_instruction_246, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_247(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_247, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_248(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_248, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_249(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_249, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_250(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_250, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_251(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_251, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_252(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	do_at_ring2(gp_b6_instruction_252, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_253(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_253, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_254(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_254, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_255(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_255, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_256(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_256, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_257(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_257, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_258(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	do_at_ring3(gp_b6_instruction_258, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_259(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_1();
	set_ring3_cs_desc();
	do_at_ring0(gp_b6_instruction_259, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_260(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_occur();
	set_ring3_cs_desc();
	do_at_ring0(gp_b6_instruction_260, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_261(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	set_ring3_cs_desc();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_261, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_262(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 4;
	do_at_ring0(gp_b6_instruction_262, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_263(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 0);
	do_at_ring0(gp_b6_instruction_263, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_264(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 1);
	do_at_ring1(gp_b6_instruction_264, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_265(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 2);
	do_at_ring2(gp_b6_instruction_265, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_266(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 3);
	do_at_ring3(gp_b6_instruction_266, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_267(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 3);
	do_at_ring3(gp_b6_instruction_267, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_268(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_inst_nc_hold();
	set_idt_entry(0x40, intr_gate_ent15_3, 3);
	do_at_ring3(gp_b6_instruction_268, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_269(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 0);
	do_at_ring0(gp_b6_instruction_269, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_270(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	set_idt_entry(0x40, intr_gate_non_mem_canonical, 0);
	do_at_ring0(gp_b6_instruction_270, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_271(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 1);
	do_at_ring1(gp_b6_instruction_271, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_272(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	do_at_ring1(gp_b6_instruction_272, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_273(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 2);
	do_at_ring2(gp_b6_instruction_273, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_274(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	set_idt_entry(0x40, intr_gate_non_mem_canonical, 2);
	do_at_ring2(gp_b6_instruction_274, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_275(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 3);
	do_at_ring3(gp_b6_instruction_275, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_276(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	set_idt_entry(0x40, intr_gate_non_mem_canonical, 3);
	do_at_ring3(gp_b6_instruction_276, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_277(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 3);
	do_at_ring3(gp_b6_instruction_277, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_278(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	set_idt_entry(0x40, intr_gate_non_mem_canonical, 3);
	do_at_ring3(gp_b6_instruction_278, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_279(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	set_idt_entry(0x40, intr_gate_normal, 3);
	do_at_ring3(gp_b6_instruction_279, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_280(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_EFLAGS_NT_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	set_idt_entry(0x40, intr_gate_non_mem_canonical, 3);
	do_at_ring3(gp_b6_instruction_280, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_281(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_281, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_282(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_282, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_283(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	do_at_ring0(gp_b6_instruction_283, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_284(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_offset_jump_nc_hold();
	do_at_ring0(gp_b6_instruction_284, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_285(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_285, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_286(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 0);
	do_at_ring0(gp_b6_instruction_286, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_287(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 1);
	do_at_ring1(gp_b6_instruction_287, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_288(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 2);
	do_at_ring2(gp_b6_instruction_288, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_289(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_289, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_290(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_290, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_291(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_aligned();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_291, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_292(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_0();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_292, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_293(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_1();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_293, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_294(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_2();
	condition_descriptor_selector_nc_hold();
	make_descriptor_selector_nc(100, 3);
	do_at_ring3(gp_b6_instruction_294, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_295(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	do_at_ring0(gp_b6_instruction_295, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_296(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_296(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_297(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_297(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_298(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_298(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_299(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_299(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_300(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_aligned();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_300(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_301(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_0();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_301(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_302(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_1();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_302(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_303(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_2();
	condition_Ad_Cann_non_mem();
	//Modified manually: reconstruct the case totally.
	gp_b6_instruction_303(0);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_304(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring0(gp_b6_instruction_304, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_305(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring1(gp_b6_instruction_305, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_306(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring2(gp_b6_instruction_306, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_307(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_307, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_308(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_308, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_309(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_aligned();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_309, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_310(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_310, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_311(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_1();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_311, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_312(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_2();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_true();
	do_at_ring3(gp_b6_instruction_312, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_313(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 0);
	do_at_ring0(gp_b6_instruction_313, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_314(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 1);
	do_at_ring1(gp_b6_instruction_314, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_315(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 2);
	do_at_ring2(gp_b6_instruction_315, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_316(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_316, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_317(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_317, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_318(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_aligned();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_318, "");
	report("%s", (exception_vector_bak == DB_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_319(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_0();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_319, "");
	report("%s", (exception_vector_bak == DB_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_320(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_1();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_320, "");
	report("%s", (exception_vector_bak == DB_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_321(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_2();
	condition_Ad_Cann_cann();
	condition_DesOutIDT_false();
	set_idt_dpl(DB_VECTOR, 3);
	do_at_ring3(gp_b6_instruction_321, "");
	report("%s", (exception_vector_bak == DB_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_322(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	do_at_ring0(gp_b6_instruction_322, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_323(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(gp_b6_instruction_323, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_324(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(gp_b6_instruction_324, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_325(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(gp_b6_instruction_325, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_326(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	do_at_ring3(gp_b6_instruction_326, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_327(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	do_at_ring3(gp_b6_instruction_327, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_328(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_328, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_329(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	do_at_ring0(gp_b6_instruction_329, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_330(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_330, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_331(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_331, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_332(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_332, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_333(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_333, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_334(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_334, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_335(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_335, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_336(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_336, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == SS_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_337(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	do_at_ring0(gp_b6_instruction_337, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_338(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	do_at_ring0(gp_b6_instruction_338, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_339(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	do_at_ring1(gp_b6_instruction_339, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_340(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_1();
	do_at_ring1(gp_b6_instruction_340, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_341(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	do_at_ring2(gp_b6_instruction_341, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_342(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_2();
	do_at_ring2(gp_b6_instruction_342, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_343(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_343, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_344(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(gp_b6_instruction_344, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_345(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_345, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_346(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	do_at_ring3(gp_b6_instruction_346, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_347(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	do_at_ring3(gp_b6_instruction_347, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_348(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	do_at_ring3(gp_b6_instruction_348, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_349(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_LAHF_SAHF_0();
	do_at_ring0(gp_b6_instruction_349, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_350(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_LAHF_SAHF_1();
	condition_LOCK_used();
	do_at_ring0(gp_b6_instruction_350, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_351(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_CPUID_LAHF_SAHF_1();
	condition_LOCK_not_used();
	do_at_ring0(gp_b6_instruction_351, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_352(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_3();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_352, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_353(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	//Added manually
	condition_set_ss_null();
	condition_RPL_CPL_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_353, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_354(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_true();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_354, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_355(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_true();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_355, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_356(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	condition_RPL_CPL_false();
	condition_pgfault_occur();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_356, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_357(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_false();
	condition_pgfault_occur();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_357, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_358(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_false();
	condition_pgfault_occur();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_358, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_359(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_359, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_360(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_360, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_361(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_361, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_362(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_362, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_363(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_363, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_364(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_364, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_365(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_365, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_366(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_366, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_367(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_367, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_368(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_368, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_369(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_369, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_370(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_370, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_371(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_371, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_372(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_372, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_373(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_complex_nc_hold();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_373, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_374(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_complex_nc_hold();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_374, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_375(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_complex_nc_hold();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_375, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_376(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_complex_nc_hold();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_376, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_377(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_377, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_378(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_378, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_379(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_379, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_380(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_380, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_381(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_381, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_382(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_382, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_383(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_383, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_384(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_384, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_385(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_true();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_385, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_386(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_386, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_387(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_387, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_388(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_388, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_389(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_389, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_390(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_390, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_391(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_b6_instruction_391, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_392(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_0();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_b6_instruction_392, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_393(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_1();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring1(gp_b6_instruction_393, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static __unused void gp_b6_394(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	condition_cs_cpl_2();
	condition_RPL_CPL_false();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Sou_Not_mem_location_false();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring2(gp_b6_instruction_394, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	gp_b6_0();
	gp_b6_1();
	gp_b6_2();
	gp_b6_3();
	gp_b6_4();
	gp_b6_5();
	gp_b6_6();
	gp_b6_7();
	gp_b6_8();
	gp_b6_9();
	gp_b6_10();
	gp_b6_11();
	gp_b6_12();
	gp_b6_13();
	gp_b6_14();
	gp_b6_15();
	gp_b6_16();
	gp_b6_17();
	gp_b6_18();
	gp_b6_19();
	gp_b6_20();
	gp_b6_21();
	gp_b6_22();
	gp_b6_23();
	gp_b6_24();
	gp_b6_25();
	gp_b6_26();
	gp_b6_27();
	gp_b6_28();
	gp_b6_29();
	gp_b6_30();
	gp_b6_31();
	gp_b6_32();
	gp_b6_33();
	gp_b6_34();
	gp_b6_35();
	gp_b6_36();
	gp_b6_37();
	gp_b6_38();
	gp_b6_39();
	gp_b6_40();
	gp_b6_41();
	gp_b6_42();
	gp_b6_43();
	gp_b6_44();
	gp_b6_45();
	gp_b6_46();
	gp_b6_47();
	gp_b6_48();
	gp_b6_49();
	gp_b6_50();
	gp_b6_51();
	gp_b6_52();
	gp_b6_53();
	gp_b6_54();
	gp_b6_55();
	gp_b6_56();
	gp_b6_57();
	gp_b6_58();
	gp_b6_59();
	gp_b6_60();
	gp_b6_61();
	gp_b6_62();
	gp_b6_63();
	gp_b6_64();
	gp_b6_65();
	gp_b6_66();
	gp_b6_67();
	gp_b6_68();
	gp_b6_69();
	gp_b6_70();
	gp_b6_71();
	gp_b6_72();
	gp_b6_73();
	gp_b6_74();
	gp_b6_75();
	gp_b6_76();
	gp_b6_77();
	gp_b6_78();
	gp_b6_79();
	gp_b6_80();
	gp_b6_81();
	gp_b6_82();
	gp_b6_83();
	gp_b6_84();
	gp_b6_85();
	gp_b6_86();
	gp_b6_87();
	gp_b6_88();
	gp_b6_89();
	gp_b6_90();
	gp_b6_91();
	gp_b6_92();
	gp_b6_93();
	gp_b6_94();
	gp_b6_95();
	gp_b6_96();
	gp_b6_97();
	gp_b6_98();
	gp_b6_99();
	gp_b6_100();
	gp_b6_101();
	gp_b6_102();
	gp_b6_103();
	gp_b6_104();
	gp_b6_105();
	gp_b6_106();
	gp_b6_107();
	gp_b6_108();
	gp_b6_109();
	gp_b6_110();
	gp_b6_111();
	gp_b6_112();
	gp_b6_113();
	gp_b6_114();
	gp_b6_115();
	gp_b6_116();
	gp_b6_117();
	gp_b6_118();
	gp_b6_119();
	gp_b6_120();
	gp_b6_121();
	gp_b6_122();
	gp_b6_123();
	gp_b6_124();
	gp_b6_125();
	gp_b6_126();
	gp_b6_127();
	gp_b6_128();
	gp_b6_129();
	gp_b6_130();
	gp_b6_131();
	gp_b6_132();
	gp_b6_133();
	gp_b6_134();
	gp_b6_135();
	gp_b6_136();
	gp_b6_137();
	gp_b6_138();
	gp_b6_139();
	gp_b6_140();
	gp_b6_141();
	gp_b6_142();
	gp_b6_143();
	gp_b6_144();
	gp_b6_145();
	gp_b6_146();
	gp_b6_147();
	gp_b6_148();
	gp_b6_149();
	gp_b6_150();
	gp_b6_151();
	gp_b6_152();
	gp_b6_153();
	gp_b6_154();
	gp_b6_155();
	gp_b6_156();
	gp_b6_157();
	gp_b6_158();
	gp_b6_159();
	gp_b6_160();
	gp_b6_161();
	gp_b6_162();
	gp_b6_163();
	gp_b6_164();
	gp_b6_165();
	gp_b6_166();
	gp_b6_167();
	gp_b6_168();
	gp_b6_169();
	gp_b6_170();
	gp_b6_171();
	gp_b6_172();
	gp_b6_173();
	gp_b6_174();
	gp_b6_175();
	gp_b6_176();
	gp_b6_177();
	gp_b6_178();
	gp_b6_179();
	gp_b6_180();
	gp_b6_181();
	gp_b6_182();
	gp_b6_183();
	gp_b6_184();
	gp_b6_185();
	gp_b6_186();
	gp_b6_187();
	gp_b6_188();
	gp_b6_189();
	gp_b6_190();
	gp_b6_191();
	gp_b6_192();
	gp_b6_193();
	gp_b6_194();
	gp_b6_195();
	gp_b6_196();
	gp_b6_197();
	gp_b6_198();
	gp_b6_199();
	gp_b6_200();
	gp_b6_201();
	gp_b6_202();
	gp_b6_203();
	gp_b6_204();
	gp_b6_205();
	gp_b6_206();
	gp_b6_207();
	gp_b6_208();
	gp_b6_209();
	gp_b6_210();
	gp_b6_211();
	gp_b6_212();
	gp_b6_213();
	gp_b6_214();
	gp_b6_215();
	gp_b6_216();
	gp_b6_217();
	gp_b6_218();
	gp_b6_219();
	gp_b6_220();
	gp_b6_221();
	gp_b6_222();
	//Modified manually: the following 3 cases were fully reconstructed
	//gp_b6_223();
	gp_b6_224();
	gp_b6_225();
	gp_b6_226();
	gp_b6_227();
	gp_b6_228();
	gp_b6_229();
	gp_b6_230();
	gp_b6_231();
	gp_b6_232();
	gp_b6_233();
	gp_b6_234();
	gp_b6_235();
	gp_b6_236();
	gp_b6_237();
	gp_b6_238();
	gp_b6_239();
	gp_b6_240();
	gp_b6_241();
	gp_b6_242();
	gp_b6_243();
	gp_b6_244();
	gp_b6_245();
	gp_b6_246();
	gp_b6_247();
	gp_b6_248();
	gp_b6_249();
	gp_b6_250();
	gp_b6_251();
	gp_b6_252();
	gp_b6_253();
	gp_b6_254();
	gp_b6_255();
	gp_b6_256();
	gp_b6_257();
	gp_b6_258();
	gp_b6_259();
	gp_b6_260();
	gp_b6_261();
	gp_b6_262();
	gp_b6_263();
	gp_b6_264();
	gp_b6_265();
	gp_b6_266();
	gp_b6_267();
	gp_b6_268();
	gp_b6_269();
	gp_b6_270();
	gp_b6_271();
	gp_b6_272();
	gp_b6_273();
	gp_b6_274();
	gp_b6_275();
	gp_b6_276();
	gp_b6_277();
	gp_b6_278();
	gp_b6_279();
	gp_b6_280();
	//Modified manually: reconstruct the case totally.
	//gp_b6_281();
	gp_b6_282();
	gp_b6_283();
	//Modified manually: the following 3 cases were fully reconstructed
	//gp_b6_284();
	gp_b6_285();
	gp_b6_286();
	gp_b6_287();
	gp_b6_288();
	gp_b6_289();
	gp_b6_290();
	gp_b6_291();
	gp_b6_292();
	gp_b6_293();
	gp_b6_294();
	//Modified manually: the following 3 cases were fully reconstructed
	//gp_b6_295();
	//gp_b6_296();
	//gp_b6_297();
	//gp_b6_298();
	//gp_b6_299();
	//gp_b6_300();
	//gp_b6_301();
	//gp_b6_302();
	//gp_b6_303();
	gp_b6_304();
	gp_b6_305();
	gp_b6_306();
	gp_b6_307();
	gp_b6_308();
	gp_b6_309();
	gp_b6_310();
	gp_b6_311();
	gp_b6_312();
	gp_b6_313();
	gp_b6_314();
	gp_b6_315();
	gp_b6_316();
	gp_b6_317();
	gp_b6_318();
	gp_b6_319();
	gp_b6_320();
	gp_b6_321();
	gp_b6_322();
	gp_b6_323();
	gp_b6_324();
	gp_b6_325();
	gp_b6_326();
	gp_b6_327();
	//Modified manually: the following 3 cases were fully reconstructed
	//gp_b6_328();
	gp_b6_329();
	gp_b6_330();
	gp_b6_331();
	gp_b6_332();
	gp_b6_333();
	gp_b6_334();
	gp_b6_335();
	gp_b6_336();
	gp_b6_337();
	gp_b6_338();
	gp_b6_339();
	gp_b6_340();
	gp_b6_341();
	gp_b6_342();
	gp_b6_343();
	gp_b6_344();
	gp_b6_345();
	gp_b6_346();
	gp_b6_347();
	gp_b6_348();
	gp_b6_349();
	gp_b6_350();
	gp_b6_351();
	gp_b6_352();
	gp_b6_353();
	gp_b6_354();
	gp_b6_355();
	gp_b6_356();
	gp_b6_357();
	gp_b6_358();
	gp_b6_359();
	gp_b6_360();
	gp_b6_361();
	gp_b6_362();
	gp_b6_363();
	gp_b6_364();
	gp_b6_365();
	gp_b6_366();
	gp_b6_367();
	gp_b6_368();
	gp_b6_369();
	gp_b6_370();
	gp_b6_371();
	gp_b6_372();
	gp_b6_373();
	gp_b6_374();
	gp_b6_375();
	gp_b6_376();
	gp_b6_377();
	gp_b6_378();
	gp_b6_379();
	gp_b6_380();
	gp_b6_381();
	gp_b6_382();
	gp_b6_383();
	gp_b6_384();
	gp_b6_385();
	gp_b6_386();
	gp_b6_387();
	gp_b6_388();
	gp_b6_389();
	gp_b6_390();
	gp_b6_391();
	gp_b6_392();
	gp_b6_393();
	gp_b6_394();
	//Modified manually: the following 3 cases were fully reconstructed.
	gp_b6_223();
	gp_b6_281();
	gp_b6_284();
	gp_b6_295();
	gp_b6_296();
	gp_b6_297();
	gp_b6_298();
	gp_b6_299();
	gp_b6_300();
	gp_b6_301();
	gp_b6_302();
	gp_b6_303();
	gp_b6_328();

	return report_summary();
}
