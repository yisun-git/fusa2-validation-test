#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "fwcfg.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "mpx.h"

// define the data type
__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(64))) static u32 unsigned_32 = 0;
__unused __attribute__ ((aligned(64))) static u64 unsigned_64 = 0;
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

// unaligned address
static __unused u16 *unaligned_address_16(void)
{
	static u16  __attribute__ ((aligned(16))) addr_unalign_16;
	u16 *aligned_addr = &addr_unalign_16;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(void)
{
	static u32  __attribute__ ((aligned(32))) addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(void)
{
	static u64  __attribute__ ((aligned(64))) addr_unalign_64;
	u64 *aligned_addr = &addr_unalign_64;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	static union_unsigned_128  __attribute__ ((aligned(64))) addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_256 *unaligned_address_256(void)
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

#ifdef __x86_64__
// non canon mem aligned address
__attribute__ ((aligned(8))) static u8 addr_8 = 0;
static __unused u64 *non_canon_align_8(void)
{
	u64 address = (unsigned long)&addr_8;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(16))) static u16 addr_16 = 0;
static __unused u64 *non_canon_align_16(void)
{
	u64 address = (unsigned long)&addr_16;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(32))) static u32 addr_32 = 0;
static __unused u64 *non_canon_align_32(void)
{
	u64 address = (unsigned long)&addr_32;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(64))) static u64 addr_64 = 0;
static __unused u64 *non_canon_align_64(void)
{
	u64 address = (unsigned long)&addr_64;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(64))) static union_unsigned_128 addr_128;
static __unused u64 *non_canon_align_128(void)
{
	u64 address = (unsigned long)&addr_128;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

__attribute__ ((aligned(64))) static union_unsigned_256 addr_256;
static __unused u64 *non_canon_align_256(void)
{
	u64 address = (unsigned long)&addr_256;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

// non canon stack aligned address
__unused static u64 *non_canon_align_stack(u64 addr)
{
	u64 address = addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}
#endif

// trigger_pgfault
static __unused u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (u64 *)add1;
}

#ifdef __x86_64__
void test_rip_relative_1(unsigned *mem, char *insn_ram)
{
	insn_ram[0] = 0xf3;
	insn_ram[1] = 0x0f;
	insn_ram[2] = 0x1b;
	insn_ram[3] = 0x05;
	*(unsigned *)&insn_ram[4] = 4 + (char *)mem - (insn_ram + 9);
	insn_ram[8] = 0x01;
	/* ret */
	insn_ram[9] = 0xc3;

	asm volatile(ASM_TRY("1f")
				"callq *%1\n"
				"1:\n"
				: "+m"(*mem) : "r"(insn_ram));
}

void test_rel_rip(void)
{
	void *mem = alloc_page();
	void *mem1 = alloc_page();
	test_rip_relative_1(mem, mem1);
	free_page(mem);
	free_page(mem1);
}
#endif


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

static __unused void mpx_b6_instruction_0(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(*non_canon_align_stack((u64)&a)));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_1(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(*non_canon_align_stack((u64)&a)));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				".byte 0xf3, 0x0f, 0x1b, 0x05, 0xdd, 0xe1, 0x05\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xf3, 0x0f, 0x1b, 0x05, 0xdd, 0xe1, 0x05\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				".byte 0xf3, 0x0f, 0x1b, 0x65, 0xf8\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xf3, 0x0f, 0x1b, 0x65, 0xf8\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_6(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_7(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_8(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_9(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_27(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_43(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_46(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_47(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_b6_instruction_48(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_49(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_50(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_51(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_52(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_53(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_54(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_b6_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	exception_vector_bak = exception_vector();
}

static __unused void mpx_b6_0(void)
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
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_0, "");
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

static __unused void mpx_b6_1(void)
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
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_1, "");
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

static __unused void mpx_b6_2(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_2, "");
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

static __unused void mpx_b6_3(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_3, "");
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

static __unused void mpx_b6_4(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_4, "");
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

static __unused void mpx_b6_5(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_5, "");
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

static __unused void mpx_b6_6(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_6, "");
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

static __unused void mpx_b6_7(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_7, "");
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

static __unused void mpx_b6_8(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_8, "");
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

static __unused void mpx_b6_9(void)
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
	condition_Ad_Cann_cann();
	condition_RIP_relative_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_9, "");
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

static __unused void mpx_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_10, "");
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

static __unused void mpx_b6_11(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_11, "");
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

static __unused void mpx_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_12, "");
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

static __unused void mpx_b6_13(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_13, "");
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

static __unused void mpx_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_14, "");
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

static __unused void mpx_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_15, "");
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

static __unused void mpx_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_16, "");
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

static __unused void mpx_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_17, "");
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

static __unused void mpx_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_18, "");
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

static __unused void mpx_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_19, "");
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

static __unused void mpx_b6_20(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_20, "");
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

static __unused void mpx_b6_21(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_21, "");
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

static __unused void mpx_b6_22(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_22, "");
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

static __unused void mpx_b6_23(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_23, "");
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

static __unused void mpx_b6_24(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_24, "");
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

static __unused void mpx_b6_25(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_25, "");
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

static __unused void mpx_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_26, "");
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

static __unused void mpx_b6_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_27, "");
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

static __unused void mpx_b6_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_28, "");
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

static __unused void mpx_b6_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_29, "");
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

static __unused void mpx_b6_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_30, "");
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

static __unused void mpx_b6_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_31, "");
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

static __unused void mpx_b6_32(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_32, "");
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

static __unused void mpx_b6_33(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_33, "");
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

static __unused void mpx_b6_34(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_34, "");
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

static __unused void mpx_b6_35(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_35, "");
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

static __unused void mpx_b6_36(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_36, "");
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

static __unused void mpx_b6_37(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_37, "");
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

static __unused void mpx_b6_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_38, "");
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

static __unused void mpx_b6_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_39, "");
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

static __unused void mpx_b6_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_40, "");
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

static __unused void mpx_b6_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_41, "");
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

static __unused void mpx_b6_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_42, "");
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

static __unused void mpx_b6_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_43, "");
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

static __unused void mpx_b6_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_44, "");
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

static __unused void mpx_b6_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_45, "");
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

static __unused void mpx_b6_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_46, "");
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

static __unused void mpx_b6_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_47, "");
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

static __unused void mpx_b6_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_48, "");
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

static __unused void mpx_b6_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_49, "");
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

static __unused void mpx_b6_50(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_50, "");
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

static __unused void mpx_b6_51(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_b6_instruction_51, "");
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

static __unused void mpx_b6_52(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_52, "");
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

static __unused void mpx_b6_53(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring1(mpx_b6_instruction_53, "");
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

static __unused void mpx_b6_54(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_54, "");
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

static __unused void mpx_b6_55(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring2(mpx_b6_instruction_55, "");
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

static __unused void mpx_b6_56(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_56, "");
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

static __unused void mpx_b6_57(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_57, "");
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

static __unused void mpx_b6_58(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_58, "");
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

static __unused void mpx_b6_59(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_0();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_59, "");
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

static __unused void mpx_b6_60(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_60, "");
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

static __unused void mpx_b6_61(void)
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
	condition_D_segfault_not_occur();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring3(mpx_b6_instruction_61, "");
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
	mpx_b6_0();
	mpx_b6_1();
	mpx_b6_2();
	mpx_b6_3();
	mpx_b6_4();
	mpx_b6_5();
	mpx_b6_6();
	mpx_b6_7();
	mpx_b6_8();
	mpx_b6_9();
	mpx_b6_10();
	mpx_b6_11();
	mpx_b6_12();
	mpx_b6_13();
	mpx_b6_14();
	mpx_b6_15();
	mpx_b6_16();
	mpx_b6_17();
	mpx_b6_18();
	mpx_b6_19();
	mpx_b6_20();
	mpx_b6_21();
	mpx_b6_22();
	mpx_b6_23();
	mpx_b6_24();
	mpx_b6_25();
	mpx_b6_26();
	mpx_b6_27();
	mpx_b6_28();
	mpx_b6_29();
	mpx_b6_30();
	mpx_b6_31();
	mpx_b6_32();
	mpx_b6_33();
	mpx_b6_34();
	mpx_b6_35();
	mpx_b6_36();
	mpx_b6_37();
	mpx_b6_38();
	mpx_b6_39();
	mpx_b6_40();
	mpx_b6_41();
	mpx_b6_42();
	mpx_b6_43();
	mpx_b6_44();
	mpx_b6_45();
	mpx_b6_46();
	mpx_b6_47();
	mpx_b6_48();
	mpx_b6_49();
	mpx_b6_50();
	mpx_b6_51();
	mpx_b6_52();
	mpx_b6_53();
	mpx_b6_54();
	mpx_b6_55();
	mpx_b6_56();
	mpx_b6_57();
	mpx_b6_58();
	mpx_b6_59();
	mpx_b6_60();
	mpx_b6_61();

	return report_summary();
}