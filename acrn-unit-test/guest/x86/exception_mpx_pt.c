#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "fwcfg.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "mpx.h"

// define the data type
__unused __attribute__ ((aligned(32))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(32))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(32))) static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(32))) static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(32))) static u32 unsigned_32 = 0;
//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused __attribute__ ((aligned(32))) static u32 unsigned_64 = 0;
u8 exception_vector_bak = 0xFF;
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

__unused static u32 *unaligned_address_32(void)
{
	static u32 __attribute__ ((aligned(32))) addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused static u64 *unaligned_address_64(void)
{
	static u64 __attribute__ ((aligned(64))) addr_unalign_64;
	u64 *aligned_addr = &addr_unalign_64;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

__unused static union_unsigned_128 * unaligned_address_128(void)
{
	static union_unsigned_128 __attribute__ ((aligned(64))) addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

__unused static union_unsigned_256 * unaligned_address_256(void)
{
	static union_unsigned_256 __attribute__ ((aligned(64))) addr_unalign_256;
	addr_unalign_256.m[0] = 1;
	addr_unalign_256.m[1] = 1;
	addr_unalign_256.m[2] = 1;
	addr_unalign_256.m[3] = 1;
	addr_unalign_256.m[4] = 1;
	addr_unalign_256.m[5] = 1;
	addr_unalign_256.m[6] = 1;
	addr_unalign_256.m[7] = 1;
	union_unsigned_256 *addr = (union_unsigned_256 *)&addr_unalign_256;
	union_unsigned_256 *non_aligned_addr = (union_unsigned_256 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

// trigger_pgfault
__unused static u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (u64 *)add1;
}


static bool mpx_precondition(void)
{
	unsigned long cr4;
	uint64_t test_bits;
	int i = 0;

	test_bits = STATE_X87 | STATE_SSE | STATE_AVX;
	if (xsetbv_checking(XCR0_MASK, test_bits) == PASS) {
		i++;
	}

	printf("CPUID.1:ECX.XSAVE[bit 26] : %d\n", check_cpuid_1_ecx(CPUID_1_ECX_XSAVE));
	if (check_cpuid_1_ecx(CPUID_1_ECX_XSAVE) == 0) {
		return false;
	}

	/*enable osxsave*/
	cr4 = read_cr4();
	write_cr4_checking(cr4 | X86_CR4_OSXSAVE);

	printf("CPUID.1:ECX.OSXSAVE[bit 27] : %d\n", check_cpuid_1_ecx(CPUID_1_ECX_OSXSAVE));
	if (check_cpuid_1_ecx(CPUID_1_ECX_OSXSAVE) == 0) {
		return false;
	}

	printf("CPUID.(EAX=07H,ECX=0H):EBX.MPX[bit 14] : %d\n", (raw_cpuid(7, 0).b & (1<<14))>>14);
	/*the processor supports Intel MPX*/
	if (((raw_cpuid(7, 0).b & (1<<14)) >> 14) == 0) {
		return false;
	}

	printf("CPUID.(EAX=0DH,ECX=0H):EAX[3] : %d (indicates XCR0.BNDREGS[bit 3] is supported or not)\n",
		(raw_cpuid(0x0d, 0).a & (1<<3)) >> 3);
	if (((raw_cpuid(0x0d, 0).a & (1<<3)) >> 3) == 0) {
		return false;
	}

	printf("CPUID.(EAX=0DH,ECX=0H):EAX[4] : %d (indicates XCR0.BNDCSR[bit 4] is supported or not)\n",
	(raw_cpuid(0x0d, 0).a & (1<<4))>>4);
	if (((raw_cpuid(0x0d, 0).a & (1<<4)) >> 4) == 0) {
		return false;
	}
	enable_xsave();
	return true;
}

static __unused void mpx_pt_instruction_0(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0xf3, 0x0f, 0x1b, 0x65, 0xf8 \n\t"
				".code32\n"
				"2:\n"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xf3, 0x0f, 0x1b, 0x65, 0xf8\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_2(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMK %[input_1], %%bnd0\n"
				".code32\n"
				"2:\n"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_3(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMK %[input_1], %%bnd0\n"
				".code32\n"
				"2:\n"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_4(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_5(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_6(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_7(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_8(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_9(const char *msg)
{
	condition_Seg_Limit_DSeg();
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%ds:0x10000000, %%bnd1 \n"
				"1:\n"
				: : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_14(const char *msg)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (*non_aligned_addr) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_64())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_16(const char *msg)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (*non_aligned_addr) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_64())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_18(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_19(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_20(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_21(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_22(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_23(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_27(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_30(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_31(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_32(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_33(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_34(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_35(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				".byte 0x66, 0x0f, 0x1b, 0x65, 0xf4 \n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_42(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_43(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_44(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_45(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_46(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_47(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_48(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_49(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_50(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_51(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_52(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_53(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_54(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_62(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_instruction_64(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_65(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_66(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_67(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				"ljmpl $0x28, $1f\n\t"
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				"ljmpl $0x8, $2f\n\t"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_68(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_69(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_70(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_71(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_72(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_73(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_74(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_75(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_76(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_77(const char *msg)
{
	int a = 0;
	asm volatile(ASM_TRY("2f")
				".code16\n"
				"1: \n\t"
				"lock\n"
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n"
				".code32\n"
				"2:\n"
				: [output] "=m" (a) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_79(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_80(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_85(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_86(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_87(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_pt_instruction_88(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_instruction_89(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_pt_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_6, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_1();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_9, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_11, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_12, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_13, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_14, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_15, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_16, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_17, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_18, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_19, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_20, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_21, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_22, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_23, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_24, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_25, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_26, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_27, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_28, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_29, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_30, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_31, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_32, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_33, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_34, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_35, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_36, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_37, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_38, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_39, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_40, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_41, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_42, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_43, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_44, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_45, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_46, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_47, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_48, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_49, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_50, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_51, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_52, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_53, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_54, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_55, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_56, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_57, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_58, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_59, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_60, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_61, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_62, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_63, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_64, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_65, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == NO_EXCEPTION), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_66, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_67, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_68, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_69, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_70, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_71, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_72, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_73, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_74(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_74, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_75, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_76, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_used();
	condition_CS_D_0();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_77, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_78, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_79(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_instruction_79, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_80(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_80, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_81(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_pt_instruction_81, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_82(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_82, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_83(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_pt_instruction_83, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_84, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_85, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_86, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_87, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_88(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_88, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void mpx_pt_89(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_Prefix_67H_not_used();
	condition_CS_D_1();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_pt_instruction_89, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == UD_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	mpx_precondition();
	asm volatile("fninit");
	mpx_pt_0();
	mpx_pt_1();
	mpx_pt_2();
	mpx_pt_3();
	mpx_pt_4();
	mpx_pt_5();
	mpx_pt_6();
	mpx_pt_7();
	mpx_pt_8();
	mpx_pt_9();
	mpx_pt_10();
	mpx_pt_11();
	mpx_pt_12();
	mpx_pt_13();
	mpx_pt_14();
	mpx_pt_15();
	mpx_pt_16();
	mpx_pt_17();
	mpx_pt_18();
	mpx_pt_19();
	mpx_pt_20();
	mpx_pt_21();
	mpx_pt_22();
	mpx_pt_23();
	mpx_pt_24();
	mpx_pt_25();
	mpx_pt_26();
	mpx_pt_27();
	mpx_pt_28();
	mpx_pt_29();
	mpx_pt_30();
	mpx_pt_31();
	mpx_pt_32();
	mpx_pt_33();
	mpx_pt_34();
	mpx_pt_35();
	mpx_pt_36();
	mpx_pt_37();
	mpx_pt_38();
	mpx_pt_39();
	mpx_pt_40();
	mpx_pt_41();
	mpx_pt_42();
	mpx_pt_43();
	mpx_pt_44();
	mpx_pt_45();
	mpx_pt_46();
	mpx_pt_47();
	mpx_pt_48();
	mpx_pt_49();
	mpx_pt_50();
	mpx_pt_51();
	mpx_pt_52();
	mpx_pt_53();
	mpx_pt_54();
	mpx_pt_55();
	mpx_pt_56();
	mpx_pt_57();
	mpx_pt_58();
	mpx_pt_59();
	mpx_pt_60();
	mpx_pt_61();
	mpx_pt_62();
	mpx_pt_63();
	mpx_pt_64();
	mpx_pt_65();
	mpx_pt_66();
	mpx_pt_67();
	mpx_pt_68();
	mpx_pt_69();
	mpx_pt_70();
	mpx_pt_71();
	mpx_pt_72();
	mpx_pt_73();
	mpx_pt_74();
	mpx_pt_75();
	mpx_pt_76();
	mpx_pt_77();
	mpx_pt_78();
	mpx_pt_79();
	mpx_pt_80();
	mpx_pt_81();
	mpx_pt_82();
	mpx_pt_83();
	mpx_pt_84();
	mpx_pt_85();
	mpx_pt_86();
	mpx_pt_87();
	mpx_pt_88();
	mpx_pt_89();

	return report_summary();
}