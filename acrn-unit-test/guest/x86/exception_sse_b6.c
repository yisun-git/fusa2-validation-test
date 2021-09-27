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
#include "pci_util.h"

// define the data type
__attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
//__attribute__ ((aligned(64))) static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) static u64 unsigned_64 = 0;
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
	__attribute__ ((aligned(16))) u16 addr_unalign_16 = 0;
	u16 *aligned_addr = &addr_unalign_16;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32 = 0;
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

// non canon stack aligned address
static __unused u64 *non_canon_align_stack(u64 addr)
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
	u64 *add1 = NULL;
	add1 = malloc(sizeof(u64));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return (u64 *)add1;
}

static __unused void sse_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTTPS2DQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTPS2DQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_2(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CMPPD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_3(const char *msg)
{
	float f = 1.7976e308;
	float f1 = 1.6e308;
	memcpy(&unsigned_128, &f, (sizeof(unsigned_128) >> 1));
	asm volatile(ASM_TRY("1f") "ADDPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memcpy(&unsigned_128, &f1, (sizeof(unsigned_128) >> 1));
	asm volatile(ASM_TRY("1f") "ADDPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTDQ2PS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_5(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "CVTPD2DQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVUPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVUPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_8(const char *msg)
{
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "DIVPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_9(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CMPPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ADDPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_11(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MAXPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSD %0, %%xmm1\n" "1:"
				:  : "m"(*non_canon_align_128()));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PEXTRW $0x1, %%xmm1, %[output]\n" "1:"
				: [output] "=r" (*(non_canon_align_64())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(non_canon_align_16())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_19(const char *msg)
{
	u64 val = 0xFFFFFFFFFFFFFFFFUL;
	/*1.build a SNAN to generate float invalid operation.*/
	val &= ~shift_umask(62, 51);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_20(const char *msg)
{
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "MAXSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_21(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CVTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_22(const char *msg)
{
	u64 val = 0xFFFFFFFFFFFFFFFFUL;
	/*1.build a SNAN to generate float invalid operation.*/
	val &= ~shift_umask(62, 51);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_23(const char *msg)
{
	unsigned_32 = 0x1U;
	asm volatile(ASM_TRY("1f") "CVTSS2SD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_32 = 0UL;
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_24(const char *msg)
{
	unsigned_32 = -1;
	unsigned_64 = -1;
	asm volatile(ASM_TRY("1f") "CVTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_25(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ADDSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_26(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "MULSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_27(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_28(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CVTTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTTSD2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CMPSD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SQRTSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTPS2PD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTSD2SS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_34(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "COMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_35(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_36(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "DIVSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_37(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "SUBSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_38(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MINSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_43(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_46(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CVTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_47(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_48(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CVTTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_49(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "COMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_50(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_51(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "CMPSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_52(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "MAXSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_53(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_54(const char *msg)
{
	double f = 1.7976e308;
	asm volatile(ASM_TRY("1f") "CVTSI2SS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (f));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_55(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "DIVSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_64) : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SQRTSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ADDSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_61(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "CVTTSS2SI %[input_1], %[output]\n" "1:"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_62(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_63(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "SUBSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_64(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_65(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "CVTSI2SS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_66(const char *msg)
{
	asm volatile(ASM_TRY("1f") "HSUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_67(const char *msg)
{
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_68(const char *msg)
{
	u64 val = 0x1;
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "HADDPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_69(const char *msg)
{
	u64 val = 0x1;
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "HADDPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_70(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ADDSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_71(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ADDSUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_72(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ANDPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_73(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ANDNPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_74(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ANDPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_75(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ANDNPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f") "BLENDVPS %%xmm0, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PACKUSDW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f") "BLENDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_79(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "BLENDPD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_80(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVHPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTSI2SD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTSI2SD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVLPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f") "CVTDQ2PD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_85(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_86(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_87(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVHPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_88(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVLPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_89(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVQ %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_90(const char *msg)
{
	u64 val = 0x1;
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "ROUNDPD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_91(const char *msg)
{
	u64 val = 0x1;
	memcpy(&unsigned_128, &val, sizeof(val));
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_92(const char *msg)
{
	asm volatile(ASM_TRY("1f") "DPPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_93(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "DPPD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_94(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_95(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_96(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXBW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_97(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXBD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_32())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_98(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXBQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_16())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_99(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVSXBW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_100(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVSXDQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_101(const char *msg)
{
	asm volatile(ASM_TRY("1f") "EXTRACTPS $0x1, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_102(const char *msg)
{
	asm volatile(ASM_TRY("1f") "INSERTPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_103(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PEXTRQ $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_104(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRQ $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_105(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_106(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PMOVSXBD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_107(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PINSRB $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_108(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PEXTRW $0x1, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_16) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_109(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PEXTRB $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_110(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PEXTRD $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_111(const char *msg)
{
	asm volatile(ASM_TRY("1f") "LDDQU %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_112(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVSHDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_113(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVLPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_114(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVLPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_115(const char *msg)
{
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_116(const char *msg)
{
	asm volatile(ASM_TRY("1f") "RSQRTSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_117(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVHPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_118(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVHPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_119(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVSS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_120(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVSS %%xmm2, %%xmm1\n" "1:"
				:  : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_121(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "RCPSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_122(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_123(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDQA %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_124(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVAPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_125(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVAPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_126(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_127(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(non_canon_align_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_128(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_129(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVAPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_130(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_131(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_132(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_133(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_134(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_135(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_136(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_137(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_138(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_139(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_140(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_141(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_142(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_143(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_144(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_145(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_146(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_147(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_148(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_149(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_150(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_151(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_152(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_153(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_154(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_155(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_156(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PHADDSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_157(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_158(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_159(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_160(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PHADDD %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_161(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_162(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_163(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMULHRSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_164(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_165(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PALIGNR $0x1, %[input_2], %%mm1\n" "1:"
				:  : [input_2] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_166(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_167(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_168(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_169(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_170(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_171(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_172(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_173(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_174(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_175(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PHSUBSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_176(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_177(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_178(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PHADDW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_179(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PALIGNR $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_180(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PHADDW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_181(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PHADDD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_182(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PHSUBD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_183(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMULHUW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_184(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_185(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_186(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_187(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PAVGB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_188(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_189(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_190(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%mm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_191(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PAVGW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_192(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXSW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_193(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_194(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_195(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_196(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_197(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_198(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_199(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMINUB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_200(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_201(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_202(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_203(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_204(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_205(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_206(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_207(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_208(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_209(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_210(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_211(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(non_canon_align_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_212(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPGTQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_213(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_214(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_215(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_b6_instruction_216(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_217(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_218(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_219(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_220(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(non_canon_align_64())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_221(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_222(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_223(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_224(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_225(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_226(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_227(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_228(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_229(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_230(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_231(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_232(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_233(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_234(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_235(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_236(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PMULUDQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_237(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_238(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_239(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_240(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_241(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_242(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_243(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_244(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_245(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_246(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_247(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSD $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_64));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_248(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_249(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_250(const char *msg)
{
	unsigned_64 = 0x1;
	unsigned_32 = 0x1;
	memset(&unsigned_128, 0xFF, sizeof(unsigned_128));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	unsigned_32 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_251(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_252(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_253(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_254(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_255(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_b6_instruction_256(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_257(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_258(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_259(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_instruction_260(const char *msg)
{
	double f = 3.14159;
	memcpy(&unsigned_64, &f, sizeof(f));
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	unsigned_64 = 0;
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_0, "");
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

static __unused void sse_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_1, "");
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

static __unused void sse_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_6, "");
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

static __unused void sse_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_7, "");
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

static __unused void sse_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_12, "");
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

static __unused void sse_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_13, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_14, "");
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

static __unused void sse_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_15, "");
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

static __unused void sse_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_16, "");
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

static __unused void sse_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_17, "");
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

static __unused void sse_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_18, "");
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

static __unused void sse_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_21, "");
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

static __unused void sse_b6_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_24, "");
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

static __unused void sse_b6_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_b6_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_b6_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_b6_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_28, "");
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

static __unused void sse_b6_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_29, "");
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

static __unused void sse_b6_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_34, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_37, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_38, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_39, "");
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

static __unused void sse_b6_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_40, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_41, "");
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

static __unused void sse_b6_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_42, "");
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

static __unused void sse_b6_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_43, "");
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

static __unused void sse_b6_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_44, "");
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

static __unused void sse_b6_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_45, "");
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

static __unused void sse_b6_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_46, "");
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

static __unused void sse_b6_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_b6_instruction_47, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_48, "");
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

static __unused void sse_b6_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_49, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_50, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_51, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_b6_instruction_52, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_b6_instruction_53, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_b6_instruction_54, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_b6_instruction_55, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_56, "");
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

static __unused void sse_b6_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_57, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_58, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_59, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_60, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_61, "");
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

static __unused void sse_b6_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_62, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_63, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_64, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_65, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_66, "");
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

static __unused void sse_b6_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_67, "");
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

static __unused void sse_b6_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_68, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_69, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_70, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_71, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_72, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_73, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_74(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_74, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_75, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_76, "");
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

static __unused void sse_b6_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_77, "");
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

static __unused void sse_b6_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_78, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_79(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_79, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_80(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_80, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_81(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_81, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_82(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_82, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_83(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_83, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_84, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_85, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_86, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_87, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_88(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_88, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_89(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_89, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_90(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_90, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_91(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_91, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_92(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_92, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_93(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_93, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_94(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_94, "");
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

static __unused void sse_b6_95(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_95, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_96(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_96, "");
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

static __unused void sse_b6_97(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_97, "");
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

static __unused void sse_b6_98(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_98, "");
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

static __unused void sse_b6_99(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_99, "");
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

static __unused void sse_b6_100(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_100, "");
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

static __unused void sse_b6_101(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_101, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_102(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_102, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_103(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_103, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_104(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_104, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_105(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_105, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_106(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_106, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_107(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_107, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_108(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_108, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_109(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_109, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_110(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_110, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_111(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_111, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_112(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_112, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_113(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_113, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_114(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_114, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_115(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_115, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_116(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_116, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_117(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_117, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_118(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_118, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_119(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_119, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_120(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_120, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_121(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_121, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_122(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_122, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_123(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_123, "");
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

static __unused void sse_b6_124(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_124, "");
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

static __unused void sse_b6_125(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_125, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_126(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_126, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_127(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_127, "");
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

static __unused void sse_b6_128(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_128, "");
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

static __unused void sse_b6_129(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_129, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_130(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_130, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_131(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_131, "");
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

static __unused void sse_b6_132(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_132, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_133(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_133, "");
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

static __unused void sse_b6_134(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_134, "");
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

static __unused void sse_b6_135(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_135, "");
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

static __unused void sse_b6_136(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_136, "");
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

static __unused void sse_b6_137(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_137, "");
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

static __unused void sse_b6_138(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_138, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_139(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_139, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_140(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_140, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_141(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_141, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_142(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_142, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_143(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_143, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_144(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_144, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_145(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_145, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_146(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_146, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_147(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_147, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_148(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_148, "");
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

static __unused void sse_b6_149(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_149, "");
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

static __unused void sse_b6_150(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_150, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_151(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_151, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_152(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_152, "");
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

static __unused void sse_b6_153(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_153, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_154(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_154, "");
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

static __unused void sse_b6_155(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_155, "");
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

static __unused void sse_b6_156(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_156, "");
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

static __unused void sse_b6_157(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_157, "");
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

static __unused void sse_b6_158(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_158, "");
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

static __unused void sse_b6_159(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_b6_instruction_159, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_160(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_b6_instruction_160, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_161(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_b6_instruction_161, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_162(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_162, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_163(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_163, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_164(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_b6_instruction_164, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_165(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_b6_instruction_165, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_166(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_b6_instruction_166, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_167(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_167, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_168(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_168, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_169(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_169, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_170(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_170, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_171(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_171, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_172(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_172, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_173(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_173, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_174(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_174, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_175(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_175, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_176(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_176, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_177(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_177, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_178(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_178, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_179(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_179, "");
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

static __unused void sse_b6_180(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_180, "");
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

static __unused void sse_b6_181(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_181, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_182(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_182, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_183(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_183, "");
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

static __unused void sse_b6_184(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_184, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_185(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_185, "");
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

static __unused void sse_b6_186(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_186, "");
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

static __unused void sse_b6_187(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_187, "");
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

static __unused void sse_b6_188(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_188, "");
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

static __unused void sse_b6_189(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_189, "");
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

static __unused void sse_b6_190(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_b6_instruction_190, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_191(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_b6_instruction_191, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_192(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_b6_instruction_192, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_193(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_193, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_194(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_194, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_195(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_b6_instruction_195, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_196(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_b6_instruction_196, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_197(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_b6_instruction_197, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_198(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_198, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_199(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_199, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_200(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_200, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_201(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_201, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_202(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_202, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_203(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_203, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_204(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_204, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_205(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_205, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_206(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_206, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_207(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_207, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_208(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_208, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_209(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_209, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_210(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_210, "");
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

static __unused void sse_b6_211(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_211, "");
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

static __unused void sse_b6_212(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_212, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_213(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_Alignment_aligned();
	condition_pgfault_not_occur();
	condition_Ad_Cann_cann();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_213, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_214(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_214, "");
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

static __unused void sse_b6_215(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_215, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_216(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(sse_b6_instruction_216, "");
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

static __unused void sse_b6_217(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring1(sse_b6_instruction_217, "");
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

static __unused void sse_b6_218(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring2(sse_b6_instruction_218, "");
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

static __unused void sse_b6_219(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_219, "");
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

static __unused void sse_b6_220(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring3(sse_b6_instruction_220, "");
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

static __unused void sse_b6_221(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_b6_instruction_221, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_222(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_b6_instruction_222, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_223(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_b6_instruction_223, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_224(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_224, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_225(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_b6_instruction_225, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_226(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_b6_instruction_226, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_227(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_b6_instruction_227, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_228(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_b6_instruction_228, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_229(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_229, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_230(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_b6_instruction_230, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_231(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_231, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_232(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_232, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_233(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_233, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_234(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_234, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_235(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_235, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_236(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_236, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_237(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_237, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_238(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_238, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_239(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_239, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_240(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_240, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_241(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_b6_instruction_241, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_242(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_b6_instruction_242, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_243(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(sse_b6_instruction_243, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_244(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_244, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_245(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_b6_instruction_245, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_246(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_b6_instruction_246, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_247(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_b6_instruction_247, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_248(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_b6_instruction_248, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_249(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_b6_instruction_249, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_250(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_b6_instruction_250, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_251(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_b6_instruction_251, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_252(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_b6_instruction_252, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_253(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_b6_instruction_253, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_254(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_254, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_255(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_b6_instruction_255, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_256(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_b6_instruction_256, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_257(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_b6_instruction_257, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_258(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_b6_instruction_258, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_259(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_259, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_b6_260(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_Ad_Cann_cann();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_b6_instruction_260, "");
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
	asm volatile("fninit");
	sse_b6_0();
	sse_b6_1();
	sse_b6_2();
	sse_b6_3();
	sse_b6_4();
	sse_b6_5();
	sse_b6_6();
	sse_b6_7();
	sse_b6_8();
	sse_b6_9();
	sse_b6_10();
	sse_b6_11();
	sse_b6_12();
	sse_b6_13();
	sse_b6_14();
	sse_b6_15();
	sse_b6_16();
	sse_b6_17();
	sse_b6_18();
	sse_b6_19();
	sse_b6_20();
	sse_b6_21();
	sse_b6_22();
	sse_b6_23();
	sse_b6_24();
	sse_b6_25();
	sse_b6_26();
	sse_b6_27();
	sse_b6_28();
	sse_b6_29();
	sse_b6_30();
	sse_b6_31();
	sse_b6_32();
	sse_b6_33();
	sse_b6_34();
	sse_b6_35();
	sse_b6_36();
	sse_b6_37();
	sse_b6_38();
	sse_b6_39();
	sse_b6_40();
	sse_b6_41();
	sse_b6_42();
	sse_b6_43();
	sse_b6_44();
	sse_b6_45();
	sse_b6_46();
	sse_b6_47();
	sse_b6_48();
	sse_b6_49();
	sse_b6_50();
	sse_b6_51();
	sse_b6_52();
	sse_b6_53();
	sse_b6_54();
	sse_b6_55();
	sse_b6_56();
	sse_b6_57();
	sse_b6_58();
	sse_b6_59();
	sse_b6_60();
	sse_b6_61();
	sse_b6_62();
	sse_b6_63();
	sse_b6_64();
	sse_b6_65();
	sse_b6_66();
	sse_b6_67();
	sse_b6_68();
	sse_b6_69();
	sse_b6_70();
	sse_b6_71();
	sse_b6_72();
	sse_b6_73();
	sse_b6_74();
	sse_b6_75();
	sse_b6_76();
	sse_b6_77();
	sse_b6_78();
	sse_b6_79();
	sse_b6_80();
	sse_b6_81();
	sse_b6_82();
	sse_b6_83();
	sse_b6_84();
	sse_b6_85();
	sse_b6_86();
	sse_b6_87();
	sse_b6_88();
	sse_b6_89();
	sse_b6_90();
	sse_b6_91();
	sse_b6_92();
	sse_b6_93();
	sse_b6_94();
	sse_b6_95();
	sse_b6_96();
	sse_b6_97();
	sse_b6_98();
	sse_b6_99();
	sse_b6_100();
	sse_b6_101();
	sse_b6_102();
	sse_b6_103();
	sse_b6_104();
	sse_b6_105();
	sse_b6_106();
	sse_b6_107();
	sse_b6_108();
	sse_b6_109();
	sse_b6_110();
	sse_b6_111();
	sse_b6_112();
	sse_b6_113();
	sse_b6_114();
	sse_b6_115();
	sse_b6_116();
	sse_b6_117();
	sse_b6_118();
	sse_b6_119();
	sse_b6_120();
	sse_b6_121();
	sse_b6_122();
	sse_b6_123();
	sse_b6_124();
	sse_b6_125();
	sse_b6_126();
	sse_b6_127();
	sse_b6_128();
	sse_b6_129();
	sse_b6_130();
	sse_b6_131();
	sse_b6_132();
	sse_b6_133();
	sse_b6_134();
	sse_b6_135();
	sse_b6_136();
	sse_b6_137();
	sse_b6_138();
	sse_b6_139();
	sse_b6_140();
	sse_b6_141();
	sse_b6_142();
	sse_b6_143();
	sse_b6_144();
	sse_b6_145();
	sse_b6_146();
	sse_b6_147();
	sse_b6_148();
	sse_b6_149();
	sse_b6_150();
	sse_b6_151();
	sse_b6_152();
	sse_b6_153();
	sse_b6_154();
	sse_b6_155();
	sse_b6_156();
	sse_b6_157();
	sse_b6_158();
	sse_b6_159();
	sse_b6_160();
	sse_b6_161();
	sse_b6_162();
	sse_b6_163();
	sse_b6_164();
	sse_b6_165();
	sse_b6_166();
	sse_b6_167();
	sse_b6_168();
	sse_b6_169();
	sse_b6_170();
	sse_b6_171();
	sse_b6_172();
	sse_b6_173();
	sse_b6_174();
	sse_b6_175();
	sse_b6_176();
	sse_b6_177();
	sse_b6_178();
	sse_b6_179();
	sse_b6_180();
	sse_b6_181();
	sse_b6_182();
	sse_b6_183();
	sse_b6_184();
	sse_b6_185();
	sse_b6_186();
	sse_b6_187();
	sse_b6_188();
	sse_b6_189();
	sse_b6_190();
	sse_b6_191();
	sse_b6_192();
	sse_b6_193();
	sse_b6_194();
	sse_b6_195();
	sse_b6_196();
	sse_b6_197();
	sse_b6_198();
	sse_b6_199();
	sse_b6_200();
	sse_b6_201();
	sse_b6_202();
	sse_b6_203();
	sse_b6_204();
	sse_b6_205();
	sse_b6_206();
	sse_b6_207();
	sse_b6_208();
	sse_b6_209();
	sse_b6_210();
	sse_b6_211();
	sse_b6_212();
	sse_b6_213();
	sse_b6_214();
	sse_b6_215();
	sse_b6_216();
	sse_b6_217();
	sse_b6_218();
	sse_b6_219();
	sse_b6_220();
	sse_b6_221();
	sse_b6_222();
	sse_b6_223();
	sse_b6_224();
	sse_b6_225();
	sse_b6_226();
	sse_b6_227();
	sse_b6_228();
	sse_b6_229();
	sse_b6_230();
	sse_b6_231();
	sse_b6_232();
	sse_b6_233();
	sse_b6_234();
	sse_b6_235();
	sse_b6_236();
	sse_b6_237();
	sse_b6_238();
	sse_b6_239();
	sse_b6_240();
	sse_b6_241();
	sse_b6_242();
	sse_b6_243();
	sse_b6_244();
	sse_b6_245();
	sse_b6_246();
	sse_b6_247();
	sse_b6_248();
	sse_b6_249();
	sse_b6_250();
	sse_b6_251();
	sse_b6_252();
	sse_b6_253();
	sse_b6_254();
	sse_b6_255();
	sse_b6_256();
	sse_b6_257();
	sse_b6_258();
	sse_b6_259();
	sse_b6_260();

	return report_summary();
}