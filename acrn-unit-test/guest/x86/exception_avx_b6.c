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

// define the data type
__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(64))) static u32 unsigned_32 = 0;
__unused __attribute__ ((aligned(64))) static u64 unsigned_64 = 0;
u8 exception_vector_bak = 0xFF;
u16 execption_inc_len = 0;

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
	__attribute__ ((aligned(16))) u16 addr_unalign_16;
	u16 *aligned_addr = &addr_unalign_16;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u64 *unaligned_address_64(void)
{
	__attribute__ ((aligned(16))) u64 addr_unalign_64;
	u64 *aligned_addr = &addr_unalign_64;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	__attribute__ ((aligned(64))) union_unsigned_128 addr_unalign_128;
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
	__attribute__ ((aligned(64))) union_unsigned_256 addr_unalign_256;
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

__attribute__ ((aligned(16))) static u32 addr_32 = 0;
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


__attribute__ ((aligned(16))) static u64 addr_64 = 0;
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

// trigger_pgfault
static __unused u64 *trigger_pgfault(void)
{
	u64 *add1;
	add1 = malloc(sizeof(u64));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return (u64 *)add1;
}

// execution
static __unused void avx_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VADDSUBPD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VADDSUBPD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_2(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VADDPS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_3(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VADDPD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VADDPD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VADDPS %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void avx_b6_instruction_7(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPS %%xmm2, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_8(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_256())));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_9(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPD %%xmm2, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VDIVSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_11(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_12(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_13(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VADDSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_14(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VCVTSD2SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_15(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VCVTTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_16(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VCVTTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_17(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_32));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_18(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_64));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_19(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_64));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_20(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTPS2PD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_21(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTPS2PD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSS2SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSI2SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VDIVSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_27(const char *msg)
{
	double f = 3.14e10;
	memcpy(&unsigned_64, &f, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTSI2SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSI2SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_29(const char *msg)
{
	float f = 3.14e10;
	memcpy(&unsigned_32, &f, sizeof(unsigned_32));
	asm volatile(ASM_TRY("1f") "VCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_32 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_30(const char *msg)
{
	double f = 3.14e10;
	memcpy(&unsigned_64, &f, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VADDSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_31(const char *msg)
{
	double f = 3.0;
	memcpy(&unsigned_64, &f, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VANDNPD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VANDNPD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VEXTRACTF128 $0x1, %%ymm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTF128 %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTDQ2PD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTDQ2PD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSS %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTSI2SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VEXTRACTPS $0x1, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_43(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "VLDMXCSR %0\n" "1:"
	: : "m"(mxcsr));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_256())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_46(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_47(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPACKUSWB %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_48(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPACKUSDW %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_256())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_49(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_256())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_50(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_51(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_52(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPACKSSWB %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_53(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPACKSSDW %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_54(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSS %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*unaligned_address_32()));
	exception_vector_bak = exception_vector();
}

static __unused void avx_b6_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_16())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VEXTRACTI128 $0x1, %%ymm2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTB %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_8())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVD %%xmm2, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_62(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTB %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_8));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSS %%xmm2, %%xmm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_64(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_65(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTI128 %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_66(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VBROADCASTSD %%xmm2, %%ymm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_67(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_68(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPBROADCASTW %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_69(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVD %%ymm2, %%ymm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_256) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_70(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_71(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_64())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_72(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	u32 f = 0x01;
	char *p = (char *)&unsigned_128;
	memcpy(p, &f, sizeof(f));
	memcpy(p+4, &f, sizeof(f));
	memcpy(p+8, &f, sizeof(f));
	memcpy(p+12, &f, sizeof(f));
	asm volatile("VMOVDQU %[input_1], %%xmm1 \n" : : [input_1] "m" (unsigned_128));
	asm volatile("VMOVDQU %%xmm1, %%xmm2 \n" : : );
	asm volatile(ASM_TRY("1f")
				"VCVTPS2PH $0x0, %%xmm2, %[output] \n" "1:"
				: [output] "=m" (r_a) : );
	memset(p, 0x00, sizeof(unsigned_128));
	asm volatile("VMOVDQU %[input_1], %%xmm1 \n" : : [input_1] "m" (unsigned_128));
	asm volatile("VMOVDQU %%xmm1, %%xmm2 \n" : : );
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_73(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_74(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_75(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPH2PS %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD231PD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD132PD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_79(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADD132PS %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_80(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADD132PS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD213PD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD132PD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMSUB213SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void avx_b6_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMSUB231SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_85(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMADD231SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_86(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD213SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_87(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_88(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD231SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_89(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADD132SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_90(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADD213SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_91(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_92(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB213SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_93(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMSUB132SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_94(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMADD132SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_95(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMADD213SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_96(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_97(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMADD213SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_98(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_99(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB132SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_100(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMSUB213SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_101(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB132SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_102(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_103(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMSUB132SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_104(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD132SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_105(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_106(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMADD231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_107(const char *msg)
{
	double f = 3.0;
	memcpy(&unsigned_64, &f, sizeof(unsigned_64));
	asm volatile("VMOVDQU %0, %%xmm1 \n" : : "m" (unsigned_64));
	asm volatile("VMOVDQU %%xmm1, %%xmm2 \n" : : );
	asm volatile(ASM_TRY("1f") "VFNMSUB213SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_108(const char *msg)
{
	double f = 3.0;
	memcpy(&unsigned_64, &f, sizeof(unsigned_64));
	asm volatile("VMOVDQU %0, %%xmm1 \n" : : "m" (unsigned_64));
	asm volatile("VMOVDQU %%xmm1, %%xmm2 \n" : : );
	asm volatile(ASM_TRY("1f") "VFNMADD132SD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_instruction_109(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VGATHERDPS %%ymm2, (%%eax,%%ymm0,1), %%ymm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_110(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA %[input_2], %%rax\n"   ASM_TRY("1f") "VGATHERQPD %%xmm2, (%%rax,%%xmm0,1), %%xmm1\n" "1:"
				:  : [input_2] "m" (*(non_canon_align_64())));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_111(const char *msg)
{
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VGATHERDPS %%xmm2, (%%eax,%%xmm0,1), %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_b6_instruction_112(const char *msg)
{
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VGATHERDPD %%xmm2, (%%eax,%%xmm0,1), %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_6, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_9, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_11, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(avx_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(avx_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_b6_instruction_16, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_b6_instruction_17, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(avx_b6_instruction_18, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(avx_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_b6_instruction_21, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_22, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_23, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_24, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_25, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_26, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_27, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(avx_b6_instruction_28, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(avx_b6_instruction_29, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_30, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_31, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_32, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_33, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_34, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_35, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_36, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_37, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_38, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_39, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(avx_b6_instruction_40, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(avx_b6_instruction_41, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_42, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_43, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_44, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_45, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_46, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_47, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_48, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_49, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_50, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_51, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_52, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_53, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_54, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_55, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_56, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_57, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_58, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_59, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_60, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_61, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_62, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_63, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_64, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_65, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(avx_b6_instruction_66, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(avx_b6_instruction_67, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_68, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_69, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_70, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_71, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_72, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_73, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_74(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_74, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_75, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_76, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_77, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_78, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_79(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_b6_instruction_79, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_80(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_b6_instruction_80, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_81(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_81, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_82(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_82, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_83(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_83, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_84, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_85, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_86, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_87, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_88(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_88, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_89(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_b6_instruction_89, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_90(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(avx_b6_instruction_90, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_91(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(avx_b6_instruction_91, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_92(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_b6_instruction_92, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_93(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_b6_instruction_93, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_94(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_b6_instruction_94, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_95(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(avx_b6_instruction_95, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_96(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(avx_b6_instruction_96, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_97(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_b6_instruction_97, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_98(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_b6_instruction_98, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_99(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_99, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_100(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_b6_instruction_100, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_101(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_b6_instruction_101, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_102(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_102, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_103(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_b6_instruction_103, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_104(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_104, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_105(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(avx_b6_instruction_105, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_106(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(avx_b6_instruction_106, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_107(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_107, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_108(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_b6_instruction_108, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_109(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Inv_AVX2_not_hold();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_109, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_110(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Inv_AVX2_not_hold();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_110, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_111(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Inv_AVX2_not_hold();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_b6_instruction_111, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_b6_112(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Inv_AVX2_not_hold();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(avx_b6_instruction_112, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	avx_b6_0();
	avx_b6_1();
	avx_b6_2();
	avx_b6_3();
	avx_b6_4();
	avx_b6_5();
	avx_b6_6();
	avx_b6_7();
	avx_b6_8();
	avx_b6_9();
	avx_b6_10();
	avx_b6_11();
	avx_b6_12();
	avx_b6_13();
	avx_b6_14();
	avx_b6_15();
	avx_b6_16();
	avx_b6_17();
	avx_b6_18();
	avx_b6_19();
	avx_b6_20();
	avx_b6_21();
	avx_b6_22();
	avx_b6_23();
	avx_b6_24();
	avx_b6_25();
	avx_b6_26();
	avx_b6_27();
	avx_b6_28();
	avx_b6_29();
	avx_b6_30();
	avx_b6_31();
	avx_b6_32();
	avx_b6_33();
	avx_b6_34();
	avx_b6_35();
	avx_b6_36();
	avx_b6_37();
	avx_b6_38();
	avx_b6_39();
	avx_b6_40();
	avx_b6_41();
	avx_b6_42();
	avx_b6_43();
	avx_b6_44();
	avx_b6_45();
	avx_b6_46();
	avx_b6_47();
	avx_b6_48();
	avx_b6_49();
	avx_b6_50();
	avx_b6_51();
	avx_b6_52();
	avx_b6_53();
	avx_b6_54();
	avx_b6_55();
	avx_b6_56();
	avx_b6_57();
	avx_b6_58();
	avx_b6_59();
	avx_b6_60();
	avx_b6_61();
	avx_b6_62();
	avx_b6_63();
	avx_b6_64();
	avx_b6_65();
	avx_b6_66();
	avx_b6_67();
	avx_b6_68();
	avx_b6_69();
	avx_b6_70();
	avx_b6_71();
	avx_b6_72();
	avx_b6_73();
	avx_b6_74();
	avx_b6_75();
	avx_b6_76();
	avx_b6_77();
	avx_b6_78();
	avx_b6_79();
	avx_b6_80();
	avx_b6_81();
	avx_b6_82();
	avx_b6_83();
	avx_b6_84();
	avx_b6_85();
	avx_b6_86();
	avx_b6_87();
	avx_b6_88();
	avx_b6_89();
	avx_b6_90();
	avx_b6_91();
	avx_b6_92();
	avx_b6_93();
	avx_b6_94();
	avx_b6_95();
	avx_b6_96();
	avx_b6_97();
	avx_b6_98();
	avx_b6_99();
	avx_b6_100();
	avx_b6_101();
	avx_b6_102();
	avx_b6_103();
	avx_b6_104();
	avx_b6_105();
	avx_b6_106();
	avx_b6_107();
	avx_b6_108();
	avx_b6_109();
	avx_b6_110();
	avx_b6_111();
	avx_b6_112();

	return report_summary();
}