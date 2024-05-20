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

static __unused void mpx_pt_b6_instruction_0(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_1(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_2(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_3(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				#ifdef __x86_64__
				".byte 0xf3, 0x0f, 0x1a, 0x25, 0x6d, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf3, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				#ifdef __x86_64__
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0xd1, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf2, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				#ifdef __x86_64__
				".byte 0xf3, 0x0f, 0x1a, 0x25, 0x6d, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf3, 0x0f, 0x1a, 0x25, 0x80, 0xd4, 0x05, 0x01\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				#ifdef __x86_64__
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0xd1, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0x80, 0xd4, 0x05, 0x01\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_8(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u32 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_9(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_10(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_11(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_12(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_13(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_14(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_15(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_16(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u32 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_17(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x67\n"
				#ifdef __x86_64__
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0xd1, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf2, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_19(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x00UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u32 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCN %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_20(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x00UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCN %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_21(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_22(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_23(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_24(const char *msg)
{
	condition_Seg_Limit_DSeg();
	asm volatile(ASM_TRY("1f")
				".byte 0x67 \n"
				"BNDMOV %%ds:0x10000000, %%bnd1 \n"
				"1:\n"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_instruction_25(const char *msg)
{
	condition_Seg_Limit_DSeg();
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%ds:0x10000000, %%bnd1 \n"
				"1:\n"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_pt_b6_0(void)
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
	condition_CS_D_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_0, "");
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

static __unused void mpx_pt_b6_1(void)
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
	condition_CS_D_1();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_1, "");
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

static __unused void mpx_pt_b6_2(void)
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
	condition_Bound_Lower();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_2, "");
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

static __unused void mpx_pt_b6_3(void)
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
	condition_Bound_Lower();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_3, "");
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

static __unused void mpx_pt_b6_4(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_4, "");
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

static __unused void mpx_pt_b6_5(void)
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
	do_at_ring0(mpx_pt_b6_instruction_5, "");
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

static __unused void mpx_pt_b6_6(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_6, "");
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

static __unused void mpx_pt_b6_7(void)
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
	do_at_ring0(mpx_pt_b6_instruction_7, "");
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

static __unused void mpx_pt_b6_8(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_8, "");
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

static __unused void mpx_pt_b6_9(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_9, "");
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

static __unused void mpx_pt_b6_10(void)
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
	do_at_ring0(mpx_pt_b6_instruction_10, "");
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

static __unused void mpx_pt_b6_11(void)
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
	do_at_ring0(mpx_pt_b6_instruction_11, "");
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

static __unused void mpx_pt_b6_12(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_12, "");
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

static __unused void mpx_pt_b6_13(void)
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
	condition_Bound_Upper();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_13, "");
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

static __unused void mpx_pt_b6_14(void)
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
	do_at_ring0(mpx_pt_b6_instruction_14, "");
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

static __unused void mpx_pt_b6_15(void)
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
	do_at_ring0(mpx_pt_b6_instruction_15, "");
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

static __unused void mpx_pt_b6_16(void)
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
	condition_Bound_Upper();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_16, "");
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

static __unused void mpx_pt_b6_17(void)
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
	condition_Bound_Upper();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_17, "");
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

static __unused void mpx_pt_b6_18(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_18, "");
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

static __unused void mpx_pt_b6_19(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_19, "");
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

static __unused void mpx_pt_b6_20(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_20, "");
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

static __unused void mpx_pt_b6_21(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_21, "");
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

static __unused void mpx_pt_b6_22(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_22, "");
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

static __unused void mpx_pt_b6_23(void)
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
	condition_Bound_Lower();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_23, "");
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

static __unused void mpx_pt_b6_24(void)
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
	condition_D_segfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_24, "");
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

static __unused void mpx_pt_b6_25(void)
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
	condition_D_segfault_occur();
	execption_inc_len = 3;
	do_at_ring0(mpx_pt_b6_instruction_25, "");
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


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	mpx_precondition();
	asm volatile("fninit");
	mpx_pt_b6_0();
	mpx_pt_b6_1();
	mpx_pt_b6_2();
	mpx_pt_b6_3();
	mpx_pt_b6_4();
	mpx_pt_b6_5();
	mpx_pt_b6_6();
	mpx_pt_b6_7();
	mpx_pt_b6_8();
	mpx_pt_b6_9();
	mpx_pt_b6_10();
	mpx_pt_b6_11();
	mpx_pt_b6_12();
	mpx_pt_b6_13();
	mpx_pt_b6_14();
	mpx_pt_b6_15();
	mpx_pt_b6_16();
	mpx_pt_b6_17();
	mpx_pt_b6_18();
	mpx_pt_b6_19();
	mpx_pt_b6_20();
	mpx_pt_b6_21();
	mpx_pt_b6_22();
	mpx_pt_b6_23();
	mpx_pt_b6_24();
	mpx_pt_b6_25();

	return report_summary();
}