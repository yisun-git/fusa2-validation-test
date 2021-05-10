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
#include "isr.h"

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
 * "1:" : [output] "=r" (*(non_canon_align_mem((u64)&unsigned_16))):[input_1] "m" (unsigned_16));" like case 2
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
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(u64 *aligned_addr)
{
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(u64 *aligned_addr)
{
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(union_unsigned_128 *addr)
{
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
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

void bound_32bit_br_001(void)
{
	int t[2] = {0, 10};
	u32 r_a = 0xff;
	asm volatile ("bound %1, %[add]\n"
		: [add]"=m"(t[0])
		: "r"(r_a)
		: "memory");
}

void bound_32bit_ud_001(void)
{
	int t[2] = {0, 10};
	u32 r_a = 0xff;
	asm volatile (".byte 0x62, 0xD0\n"
		: [add]"=m"(t[0])
		: "r"(r_a)
		: "memory");
}

void irq_func(isr_regs_t *regs)
{
}

void irq_func_stack_unaligned(isr_regs_t *regs)
{
	volatile int a = 0;
	a += 1;
	asm volatile("sub $5, %esp\n");
}

static __unused void gp_pt_instruction_0(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_1(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_2(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_3(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_4(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_5(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_6(const char *msg)
{
	unsigned long check_bit = 0;
	asm volatile(ASM_TRY("1f") "mov %%cr0, %0\n" "1:" : "=r"(check_bit) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_7(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	check_bit &= ~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_00));
	check_bit |= FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31);
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_8(const char *msg)
{
	unsigned long check_bit = 0;
	check_bit = read_cr0();
	asm volatile(ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_9(const char *msg)
{
	u32 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr5\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_10(const char *msg)
{
	u32 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr5\n" "1:" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_11(const char *msg)
{
	u32 cr0 = read_cr0();
	cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(cr0) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_12(const char *msg)
{
	u32 cr0 = read_cr0();
	cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:" : : "r"(cr0) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %%cr0, %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOV %%cr0, %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_15(const char *msg)
{
	unsigned long check_bit = read_dr7();
	u32 r_a = 1;
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));
	asm volatile ("mov %0, %%dr7\n" ASM_TRY("1f") "mov %1, %%dr6\n" "1:" : : "r"(check_bit), "r"(r_a) : "memory");
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNG %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNE %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNB %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNC %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNAE %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CMOVNBE %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC %[input_1], %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : [input_1] "r" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_27(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"ADC $1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %%cs:4 \n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_64((u64 *)&unsigned_64))) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_38(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_39(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_40(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_41(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_42(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_43(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_44(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_45(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_46(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_47(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_48(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_49(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_50(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_51(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_52(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_53(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_54(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_55(const char *msg)
{
	asm volatile("mov $0x00, %" R "dx");
	asm volatile("mov $0x00, %" R "ax");
	unsigned_8 = 1;
	asm volatile(ASM_TRY("1f") "DIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	unsigned_8 = 0;
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_56(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_57(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_58(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_59(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_60(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_61(const char *msg)
{
	unsigned_8 = 0;
	asm volatile(ASM_TRY("1f") "IDIVB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_62(const char *msg)
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

static __unused void gp_pt_instruction_63(const char *msg)
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

static __unused void gp_pt_instruction_64(const char *msg)
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

static __unused void gp_pt_instruction_65(const char *msg)
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

static __unused void gp_pt_instruction_66(const char *msg)
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

static __unused void gp_pt_instruction_67(const char *msg)
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
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_68(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SETB %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_69(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_70(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_71(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_72(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_73(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_74(const char *msg)
{
	unsigned_32 = 0xFFFFFF00U;
	asm volatile(ASM_TRY("1f") "TZCNT %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_75(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_79(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_80(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $100\n" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_85(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_86(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_87(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_88(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_89(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_90(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_91(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_92(const char *msg)
{
	asm volatile(ASM_TRY("1f") "int $64\n" "1:" : : );
	test_delay(1);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_93(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_94(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_95(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_96(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_97(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_98(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_br_001;
	test_for_exception(BR_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_99(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_100(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_101(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_102(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_103(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_104(const char *msg)
{
	gp_trigger_func fun;
	fun = (gp_trigger_func)bound_32bit_ud_001;
	test_for_exception(UD_VECTOR, fun, NULL);
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_105(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_106(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_107(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_108(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_109(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_110(const char *msg)
{
	static int t[2] = {0, 10};
	static u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_111(const char *msg)
{
	addr_m16_32.selector = 0x11;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_112(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_113(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_114(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0xC4, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_115(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_116(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_instruction_117(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:"
				: : "m" (addr_m16_32));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_118(const char *msg)
{
	addr_m16_32.selector = KERNEL_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_119(const char *msg)
{
	addr_m16_32.selector = OSSERVISE1_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_120(const char *msg)
{
	addr_m16_32.selector = OSSERVISE2_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lds %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_121(const char *msg)
{
	addr_m16_32.selector = USER_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_122(const char *msg)
{
	addr_m16_32.selector = USER_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_123(const char *msg)
{
	addr_m16_32.selector = USER_DS;
	addr_m16_32.offset = (u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%eax \n" "1:" : : "m" (addr_m16_32));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_124(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_125(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_126(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_127(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_128(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_instruction_129(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	exception_vector_bak = exception_vector();
}

static __unused void gp_pt_instruction_130(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SMSW %[input_1]\n" "1:"
				: : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_1();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_3();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_0();
	condition_AccessCR1567_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_9, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_0();
	condition_AccessCR1567_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_11, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_12, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_0();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_13, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_PDPT_RB_1();
	condition_PDPTLoad_false();
	condition_cs_cpl_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_14, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_cs_cpl_0();
	condition_DeAc_true();
	condition_DR7_GD_1();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_15, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_16, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_17, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_18, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_19, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_20(void)
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
	condition_CR0_AM_0();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_20, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_21(void)
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
	condition_Alignment_aligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_21, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	do_at_ring0(gp_pt_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_25(void)
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
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(gp_pt_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_26(void)
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
	condition_CR0_AM_0();
	do_at_ring3(gp_pt_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_27(void)
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
	condition_Alignment_aligned();
	do_at_ring3(gp_pt_instruction_27, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_not_hold();
	do_at_ring0(gp_pt_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_occur();
	do_at_ring0(gp_pt_instruction_29, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_30, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_31, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	do_at_ring0(gp_pt_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_instruction_34, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(gp_pt_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	do_at_ring3(gp_pt_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	do_at_ring3(gp_pt_instruction_37, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Divisor_0_true();
	do_at_ring0(gp_pt_instruction_38, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Divisor_0_true();
	do_at_ring1(gp_pt_instruction_39, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Divisor_0_true();
	do_at_ring2(gp_pt_instruction_40, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Divisor_0_true();
	do_at_ring3(gp_pt_instruction_41, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_42(void)
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
	condition_CR0_AM_0();
	condition_Divisor_0_true();
	do_at_ring3(gp_pt_instruction_42, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_43(void)
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
	condition_Alignment_aligned();
	condition_Divisor_0_true();
	do_at_ring3(gp_pt_instruction_43, "");
	report("%s", (exception_vector_bak == DE_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring0(gp_pt_instruction_44, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring1(gp_pt_instruction_45, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring2(gp_pt_instruction_46, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_pt_instruction_47, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_48(void)
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
	condition_CR0_AM_0();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_pt_instruction_48, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_49(void)
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
	condition_Alignment_aligned();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring3(gp_pt_instruction_49, "");
	report("%s", (exception_vector_bak == DE_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring0(gp_pt_instruction_50, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring1(gp_pt_instruction_51, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring2(gp_pt_instruction_52, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_pt_instruction_53, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_54(void)
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
	condition_CR0_AM_0();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_pt_instruction_54, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_55(void)
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
	condition_Alignment_aligned();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring3(gp_pt_instruction_55, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring0(gp_pt_instruction_56, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring1(gp_pt_instruction_57, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring2(gp_pt_instruction_58, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring3(gp_pt_instruction_59, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_60(void)
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
	condition_CR0_AM_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring3(gp_pt_instruction_60, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_61(void)
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
	condition_Alignment_aligned();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring3(gp_pt_instruction_61, "");
	report("%s", (exception_vector_bak == DE_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring0(gp_pt_instruction_62, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring1(gp_pt_instruction_63, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring2(gp_pt_instruction_64, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring3(gp_pt_instruction_65, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_66(void)
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
	condition_CR0_AM_0();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring3(gp_pt_instruction_66, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_67(void)
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
	condition_Alignment_aligned();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring3(gp_pt_instruction_67, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	do_at_ring0(gp_pt_instruction_68, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_69, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_70, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_71, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_72, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_73, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_74(void)
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
	condition_Alignment_aligned();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_74, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_DesOutIDT_true();
	do_at_ring0(gp_pt_instruction_75, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_DesOutIDT_true();
	do_at_ring1(gp_pt_instruction_76, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_DesOutIDT_true();
	do_at_ring2(gp_pt_instruction_77, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_78, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_79(void)
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
	condition_CR0_AM_0();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_79, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_80(void)
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
	condition_StackPushAlignment_aligned();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_80, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_81(void)
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
	condition_cs_dpl_0();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_81, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_82(void)
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
	condition_cs_dpl_1();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_82, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_83(void)
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
	condition_cs_dpl_2();
	condition_DesOutIDT_true();
	do_at_ring3(gp_pt_instruction_83, "");
	report("%s", (exception_vector_bak == GP_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 0);
	do_at_ring0(gp_pt_instruction_84, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 1);
	do_at_ring1(gp_pt_instruction_85, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 2);
	do_at_ring2(gp_pt_instruction_86, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 3);
	do_at_ring3(gp_pt_instruction_87, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_88(void)
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
	condition_CR0_AM_0();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 3);
	do_at_ring3(gp_pt_instruction_88, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_89(void)
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
	condition_StackPushAlignment_aligned();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func, 3);
	do_at_ring3(gp_pt_instruction_89, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_90(void)
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
	condition_cs_dpl_0();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func_stack_unaligned, 3);
	do_at_ring3(gp_pt_instruction_90, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_91(void)
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
	condition_cs_dpl_1();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func_stack_unaligned, 3);
	do_at_ring3(gp_pt_instruction_91, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_92(void)
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
	condition_cs_dpl_2();
	condition_DesOutIDT_false();
	set_idt_entry(0x40, irq_func_stack_unaligned, 3);
	do_at_ring3(gp_pt_instruction_92, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_93(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_93, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_94(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_94, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_95(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_95, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_96(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_96, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_97(void)
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
	condition_CR0_AM_0();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_97, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_98(void)
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
	condition_Alignment_aligned();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_98, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == BR_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_99(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_99, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_100(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_100, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_101(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_101, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_102(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_102, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_103(void)
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
	condition_CR0_AM_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_103, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_104(void)
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
	condition_Alignment_aligned();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_104, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_105(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_105, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_106(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_106, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_107(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_107, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_108(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_108, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_109(void)
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
	condition_CR0_AM_0();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_109, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_110(void)
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
	condition_Alignment_aligned();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_110, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_111(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_occur();
	//Added manually
	condition_set_ss_null();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_111, "");
	execption_inc_len = 0;
	//Added manually
	condition_restore_ss();
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_112(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_112, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_113(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_113, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_114(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_114, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_115(void)
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
	condition_EFLAGS_AC_set_to_0();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_115, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_116(void)
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
	condition_CR0_AM_0();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_116, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_117(void)
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
	condition_Alignment_aligned();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_117, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_118(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_instruction_118, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_119(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring1(gp_pt_instruction_119, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_120(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_S_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring2(gp_pt_instruction_120, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_121(void)
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
	condition_EFLAGS_AC_set_to_0();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_121, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_122(void)
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
	condition_CR0_AM_0();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_122, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_123(void)
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
	condition_Alignment_aligned();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring3(gp_pt_instruction_123, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_124(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	do_at_ring0(gp_pt_instruction_124, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_125(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	do_at_ring1(gp_pt_instruction_125, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_126(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	do_at_ring2(gp_pt_instruction_126, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_127(void)
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
	condition_EFLAGS_AC_set_to_0();
	do_at_ring3(gp_pt_instruction_127, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_128(void)
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
	condition_CR0_AM_0();
	do_at_ring3(gp_pt_instruction_128, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_129(void)
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
	condition_Alignment_aligned();
	do_at_ring3(gp_pt_instruction_129, "");
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_130(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_UMIP_1();
	condition_cs_cpl_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	do_at_ring0(gp_pt_instruction_130, "");
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
	gp_pt_0();
	gp_pt_1();
	gp_pt_2();
	gp_pt_3();
	gp_pt_4();
	gp_pt_5();
	gp_pt_6();
	gp_pt_7();
	gp_pt_8();
	gp_pt_9();
	gp_pt_10();
	gp_pt_11();
	gp_pt_12();
	gp_pt_13();
	gp_pt_14();
	gp_pt_15();
	gp_pt_16();
	gp_pt_17();
	gp_pt_18();
	gp_pt_19();
	gp_pt_20();
	gp_pt_21();
	gp_pt_22();
	gp_pt_23();
	gp_pt_24();
	gp_pt_25();
	gp_pt_26();
	gp_pt_27();
	gp_pt_28();
	gp_pt_29();
	gp_pt_30();
	gp_pt_31();
	gp_pt_32();
	gp_pt_33();
	gp_pt_34();
	gp_pt_35();
	gp_pt_36();
	gp_pt_37();
	gp_pt_38();
	gp_pt_39();
	gp_pt_40();
	gp_pt_41();
	gp_pt_42();
	gp_pt_43();
	gp_pt_44();
	gp_pt_45();
	gp_pt_46();
	gp_pt_47();
	gp_pt_48();
	gp_pt_49();
	gp_pt_50();
	gp_pt_51();
	gp_pt_52();
	gp_pt_53();
	gp_pt_54();
	gp_pt_55();
	gp_pt_56();
	gp_pt_57();
	gp_pt_58();
	gp_pt_59();
	gp_pt_60();
	gp_pt_61();
	gp_pt_62();
	gp_pt_63();
	gp_pt_64();
	gp_pt_65();
	gp_pt_66();
	gp_pt_67();
	gp_pt_68();
	gp_pt_69();
	gp_pt_70();
	gp_pt_71();
	gp_pt_72();
	gp_pt_73();
	gp_pt_74();
	gp_pt_75();
	gp_pt_76();
	gp_pt_77();
	gp_pt_78();
	gp_pt_79();
	gp_pt_80();
	gp_pt_81();
	gp_pt_82();
	gp_pt_83();
	gp_pt_84();
	gp_pt_85();
	gp_pt_86();
	gp_pt_87();
	gp_pt_88();
	gp_pt_89();
	gp_pt_90();
	gp_pt_91();
	gp_pt_92();
	gp_pt_93();
	gp_pt_94();
	gp_pt_95();
	gp_pt_96();
	gp_pt_97();
	gp_pt_98();
	gp_pt_99();
	gp_pt_100();
	gp_pt_101();
	gp_pt_102();
	gp_pt_103();
	gp_pt_104();
	gp_pt_105();
	gp_pt_106();
	gp_pt_107();
	gp_pt_108();
	gp_pt_109();
	gp_pt_110();
	gp_pt_111();
	gp_pt_112();
	gp_pt_113();
	gp_pt_114();
	gp_pt_115();
	gp_pt_116();
	gp_pt_117();
	gp_pt_118();
	gp_pt_119();
	gp_pt_120();
	gp_pt_121();
	gp_pt_122();
	gp_pt_123();
	gp_pt_124();
	gp_pt_125();
	gp_pt_126();
	gp_pt_127();
	gp_pt_128();
	gp_pt_129();
	gp_pt_130();

	return report_summary();
}
