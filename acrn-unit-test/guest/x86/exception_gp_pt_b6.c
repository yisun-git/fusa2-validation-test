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
#include "regdump.h"

#define CALL_GATE_SEL (0x40<<3)
#define CODE_SEL     (0x50<<3)
#define DATA_SEL     (0x54<<3)
#define CODE_2ND     (0x58<<3)
#define NON_CANINCAL_FACTOR	(0x8000000000000000UL)
#define USE_HAND_EXECEPTION
#define BIT_IS(NUMBER, N) ((((uint64_t)NUMBER) >> N) & (0x1))
#define MSR_IA32_TSC_DEADLINE   (0x000006E0U)

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
	size_t a = (size_t)aligned_addr;
	a |= 1;
	u16 *non_aligned_addr = (u16 *)(a);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(u64 *aligned_addr)
{
	size_t a = (size_t)aligned_addr;
	a |= 1;
	u32 *non_aligned_addr = (u32 *)(a);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(u64 *aligned_addr)
{
	size_t a = (size_t)aligned_addr;
	a |= 1;
	u64 *non_aligned_addr = (u64 *)(a);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(union_unsigned_128 *addr)
{
	size_t a = (size_t)addr;
	a |= 1;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)(a);
	return non_aligned_addr;
}

#ifdef __x86_64__
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
#endif

#ifdef __i386__
static __unused u32 *trigger_pgfault(void)
{
	u32 *add1 = NULL;
	add1 = malloc(sizeof(u32));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return add1;
}
#else
static __unused u64 *trigger_pgfault(void)
{
	u64 *add1 = NULL;
	add1 = malloc(sizeof(u64));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return add1;
}
#endif

// define called function
void called_func()
{
	return;
}

int rdmsr_checking(u32 MSR_ADDR, u64 *result)
{
	u32 eax;
	u32 edx;

	asm volatile(ASM_TRY("1f")
		"rdmsr \n\t"
		"1:"
		: "=a"(eax), "=d"(edx) : "c"(MSR_ADDR));
	if (result) {
		*result = eax + ((u64)edx << 32);
	}
	return exception_vector();
}

int wrmsr_checking(u32 MSR_ADDR, u64 value)
{
	u32 edx = value >> 32;
	u32 eax = value;

	asm volatile(ASM_TRY("1f")
		"wrmsr \n\t"
		"1:"
		: : "c"(MSR_ADDR), "a"(eax), "d"(edx));
	return exception_vector();
}

void test_LMSW_GP(void *data)
{
	u32 cr0 = read_cr0();
	asm volatile(ASM_TRY("1f")
				"LMSW %0\n" "1:\n"
				: : "m"(cr0) : "memory");
}

void test_SMSW_GP(void *data)
{
	#ifdef __x86_64__
	u64 a = (u64)&unsigned_64;
	a ^= 0x8000000000000000;
	asm volatile(ASM_TRY("1f")
				"SMSW %0\n" "1:\n"
				: "=m"(*(u32 *)a) : : "memory");
	#else
	condition_Seg_Limit_DSeg();
	asm volatile(ASM_TRY("1f")
				"SMSW %%ds:0xFFFF0000 \n" "1:\n"
				: "=m"(unsigned_32) : : "memory");
	#endif
}

void test_SBB_AC(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"SBB $1, %%al\n" "1:\n"
				: "=a"((*(unaligned_address_16((u64 *)&unsigned_64)))) : );
}

void test_TZCNT_AC(void *data)
{
	asm volatile(ASM_TRY("1f")
			"TZCNT %[input_1], %[output]\n" "1:\n"
			: [output] "=r" (*(unaligned_address_32((u64 *)&unsigned_64))) : [input_1] "m" (unsigned_32));
}

static void *rsp = NULL;
static jmp_buf jmpbuf;
void handled_exception_AC(struct ex_regs *regs)
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
		asm volatile("mov %0, %%" R "sp \n" : : "r"(rsp));
		longjmp(jmpbuf, 1);
		unhandled_exception(regs, false);
	} else {
		regs->rip += execption_inc_len;
	}
}

void test_int_AC(void)
{
	int ret = 0;
	#ifdef __i386__
	set_idt_entry(0x40, called_func, 3);
	#endif
	handle_exception(AC_VECTOR, &handled_exception_AC);
	ret = set_exception_jmpbuf(jmpbuf);
	if (ret == 0) {
		asm volatile("mov %%" R "sp, %0 \n" : "=r"(rsp) : );
		size_t rsp_1 = (size_t)rsp | 0x1;
		asm volatile("mov %0, %%" R "sp \n" : : "r"(rsp_1));
		asm volatile(ASM_TRY("1f")
		"int $0x40\n"
		"1:\n" : : );
	} else if (ret == 1) {
		set_handle_exception();
	}
}

static __unused void gp_pt_b6_instruction_0(const char *msg)
{
	u64 val = 0;
	rdmsr_checking(0x571, &val);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_1(const char *msg)
{
	u64 val = 0;
	rdmsr_checking(0x571, &val);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_2(const char *msg)
{
	wrmsr_checking(0x571, 0x0);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVLE %[input_1], %[output]\n" "1:\n"
				: [output] "=r" (*(trigger_pgfault())) : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BTC $1, %[output]\n" "1:\n"
				: [output] "=m" (*(unaligned_address_16((u64 *)&unsigned_16))) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"AND $1, %%al\n" "1:\n"
				: "=a"(*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_6(const char *msg)
{
	test_for_exception(AC_VECTOR, test_SBB_AC, NULL);
	exception_vector_bak = exception_vector();
}

//Modified manually: reconstruct this case totally.
#define NON_CANO_PREFIX_MASK (0xFFFFUL << 48) /*bits [63:48] */
static __unused void ret_test(void *data)
{
	asm volatile(
		"push" W " %0\n" //push ip
		ASM_TRY("1f")
		"ret" W " $4\n"
		"1:"
		:
		: "r"(data)
	);
}

static __unused void gp_pt_b6_instruction_7(const char *msg)
{
	void *gva = malloc(sizeof(long));
	/* clear p flag in PTE table */
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 0, true);
	test_for_exception(PF_VECTOR, ret_test, gva);
	/* resume environment */
	set_page_control_bit(gva, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(gva);
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}
//end

static __unused void gp_pt_b6_instruction_8(const char *msg)
{
	test_for_exception(AC_VECTOR, test_TZCNT_AC, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_b6_instruction_9(const char *msg)
{
	test_int_AC();
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"LFS %[input_1], %[output]\n" "1:\n"
				: [output] "=r" (*(trigger_pgfault())) : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
		"LFS %[input_1], %[output]\n" "1:\n"
		: [output] "=r" (*(unaligned_address_16((u64 *)&unsigned_16)))
		: [input_1] "m" (*(unaligned_address_64((u64 *)&addr_m16_16))));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_b6_instruction_12(const char *msg)
{
	u64 val = 0;
	rdmsr_checking(0x571, &val);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_13(const char *msg)
{
	u64 val = 0;
	rdmsr_checking(0x571, &val);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_14(const char *msg)
{
	u64 val = 0;
	rdmsr_checking(MSR_IA32_TSC_DEADLINE, &val);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_15(const char *msg)
{
	wrmsr_checking(0x571, 0x0);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_16(const char *msg)
{
	wrmsr_checking(0x571, 0x0);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_17(const char *msg)
{
	wrmsr_checking(0x571, 0x0);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_18(const char *msg)
{
	wrmsr_checking(0x571, 0x0);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_19(const char *msg)
{
	wrmsr_checking(MSR_IA32_TSC_DEADLINE, 0);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_20(const char *msg)
{
	test_for_exception(GP_VECTOR, test_LMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_21(const char *msg)
{
	test_for_exception(GP_VECTOR, test_LMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_22(const char *msg)
{
	test_for_exception(GP_VECTOR, test_LMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_23(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_24(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_25(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_26(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_27(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SMSW_GP, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"SMSW %0\n" "1:\n"
				: "=m"(*(trigger_pgfault())) : : "memory");
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"SMSW %0\n" "1:\n"
				: "=m"(*(trigger_pgfault())) : : "memory");
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"SMSW %0\n" "1:\n"
				: "=m"(*(unaligned_address_32(((u64 *)&unsigned_32)))) : : "memory");
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_b6_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"LZCNT %[input_1], %[output]\n" "1:\n"
				: [output] "=r" (*(trigger_pgfault())) : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_3();
	do_at_ring3(gp_pt_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_b6_instruction_4, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_b6_instruction_6, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_b6_instruction_8, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_StackPushAlignment_unaligned();
	condition_cs_dpl_3();
	do_at_ring3(gp_pt_b6_instruction_9, "");
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_b6_instruction_11, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_ECX_MSR_Reserved_true();
	do_at_ring0(gp_pt_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_true();
	do_at_ring0(gp_pt_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	do_at_ring0(gp_pt_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_register_nc_hold();
	do_at_ring0(gp_pt_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_true();
	do_at_ring0(gp_pt_b6_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_true();
	do_at_ring0(gp_pt_b6_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	condition_EDX_EAX_set_reserved_true();
	do_at_ring0(gp_pt_b6_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	condition_EDX_EAX_set_reserved_false();
	do_at_ring0(gp_pt_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_b6_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	do_at_ring3(gp_pt_b6_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_b6_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_b6_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_3();
	do_at_ring3(gp_pt_b6_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_occur();
	do_at_ring0(gp_pt_b6_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_0();
	condition_D_segfault_occur();
	do_at_ring0(gp_pt_b6_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_28, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_0();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_29, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_b6_instruction_30, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_b6_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_b6_instruction_31, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	gp_pt_b6_0();
	gp_pt_b6_1();
	gp_pt_b6_2();
	gp_pt_b6_3();
	gp_pt_b6_4();
	gp_pt_b6_5();
	gp_pt_b6_6();
	gp_pt_b6_7();
	gp_pt_b6_8();
	gp_pt_b6_9();
	gp_pt_b6_10();
	gp_pt_b6_11();
	gp_pt_b6_12();
	gp_pt_b6_13();
	gp_pt_b6_14();
	gp_pt_b6_15();
	gp_pt_b6_16();
	gp_pt_b6_17();
	gp_pt_b6_18();
	gp_pt_b6_19();
	gp_pt_b6_20();
	gp_pt_b6_21();
	gp_pt_b6_22();
	gp_pt_b6_23();
	gp_pt_b6_24();
	gp_pt_b6_25();
	gp_pt_b6_26();
	gp_pt_b6_27();
	gp_pt_b6_28();
	gp_pt_b6_29();
	gp_pt_b6_30();
	gp_pt_b6_31();

	return report_summary();
}
