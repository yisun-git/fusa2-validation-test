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

// this part is for sse protected mode test
// define the data type
__attribute__ ((aligned(32))) static union_unsigned_128 unsigned_128;
//__attribute__ ((aligned(32))) static u8 unsigned_8 = 0;
__attribute__ ((aligned(32))) static u16 unsigned_16 = 0;
__attribute__ ((aligned(32))) static u32 unsigned_32 = 0;
//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__attribute__ ((aligned(32))) static u32 unsigned_64 = 0;
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

//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
static __unused u32 *unaligned_address_64(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32 = 0;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	__attribute__ ((aligned(32))) union_unsigned_128 addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

// trigger_pgfault
static __unused union_unsigned_128 *trigger_pgfault(void)
{
	static union_unsigned_128 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(union_unsigned_128));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (union_unsigned_128 *)ADDR_ALIGN((u32)add1, 16);
}
static __unused void sse_pt_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVUPD %%xmm1, %%ss:0xFFFF0000 \n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ORPD %%ds:0xFFFF0000, %%xmm1 \n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVUPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_3(const char *msg)
{
	u64 val = 0x01;
	memcpy(&unsigned_128, &val, sizeof(u64));
	asm volatile(ASM_TRY("1f") "SUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_4(const char *msg)
{
	u64 val = 0x01;
	memcpy(&unsigned_128, &val, sizeof(u64));
	asm volatile(ASM_TRY("1f") "SQRTPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MULPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UNPCKHPS %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UNPCKLPS %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVUPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_10(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "SQRTPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_11(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "SUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_13(const char *msg)
{
	float f = 3.1415;
	char *p = (char *)&unsigned_128;
	memcpy(p, &f, sizeof(f));
	memcpy(p+4, &f, sizeof(f));
	memcpy(p+8, &f, sizeof(f));
	memcpy(p+12, &f, sizeof(f));
	asm volatile(ASM_TRY("1f")
				"LDDQU %[input_1], %%xmm1 \n"
				"SUBPS %[input_1], %%xmm1 \n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_18(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_19(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_20(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_21(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_22(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_23(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_24(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_25(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_26(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_27(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_39(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_42(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_43(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_44(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_45(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_46(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_47(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_48(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_49(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_50(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_51(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_52(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_53(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_54(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_62(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSLDUP %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSLDUP %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_64(const char *msg)
{
	asm volatile(ASM_TRY("1f") "LDDQU %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_65(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_66(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_67(const char *msg)
{
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_68(const char *msg)
{
	float f = 3.1415;
	char *p = (char *)&unsigned_128;
	memcpy(p, &f, sizeof(f));
	memcpy(p+4, &f, sizeof(f));
	memcpy(p+8, &f, sizeof(f));
	memcpy(p+12, &f, sizeof(f));
	asm volatile(ASM_TRY("1f")
				"LDDQU %[input_1], %%xmm1 \n"
				"SUBPS %[input_1], %%xmm1 \n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_69(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDQU %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_70(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDQU %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_71(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SHUFPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_72(const char *msg)
{
	asm volatile(ASM_TRY("1f") "RSQRTPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_73(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMINSB %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_74(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXUD %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_75(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXUW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXSB %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_78(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_79(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_80(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_81(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_82(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_83(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_84(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_85(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_86(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_87(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_88(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_89(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_90(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_91(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_92(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_93(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_94(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_95(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_96(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_97(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_98(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_99(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_100(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_101(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_102(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_103(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_104(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_105(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_106(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_107(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_108(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_109(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_110(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_111(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_112(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_113(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_114(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_115(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_116(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_117(const char *msg)
{
	u32 mxcsr = 0x1f;
	asm volatile(ASM_TRY("1f") "STMXCSR %0\n" "1:"
				: "=m"(mxcsr) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_118(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_119(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_120(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_121(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_122(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_123(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_124(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_125(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_126(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_127(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_128(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_129(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_130(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_131(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_132(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_133(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_134(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_135(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_136(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_137(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_138(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_139(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_140(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVNTDQA %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_141(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %%ss:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_142(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %%ds:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_143(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_144(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_145(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_146(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_147(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_148(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_149(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_150(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_151(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_152(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_153(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_154(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_155(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_156(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_157(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_158(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_159(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_160(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_161(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_162(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_163(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_164(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_165(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGNW %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_166(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGNW %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_167(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGNB %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_168(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGND %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_169(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGNW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_170(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %%ss:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_171(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %%ds:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_172(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_173(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_174(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_175(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_176(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_177(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_178(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_179(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_180(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_181(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_182(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_183(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_184(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_185(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_186(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_187(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_188(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_189(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_190(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_191(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_192(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_193(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_194(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %%ss:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_195(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %%ds:0xFFFF0000, %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_196(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_197(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_198(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_199(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %%ss:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_200(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %%ds:0xFFFF0000, %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_201(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_202(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_pt_instruction_203(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_204(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_205(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_206(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_207(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_208(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_209(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_210(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_211(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_212(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_213(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_214(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_215(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_216(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_217(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_218(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_219(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_220(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_221(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_222(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_223(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_224(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_225(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_226(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_227(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_228(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_229(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_230(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_231(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_232(const char *msg)
{
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
	unsigned_64 = 0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_233(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_234(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_235(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_236(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_237(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_instruction_238(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_239(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_240(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_241(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_instruction_242(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_0(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_0, "");
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

static __unused void sse_pt_1(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_1, "");
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

static __unused void sse_pt_2(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_2, "");
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

static __unused void sse_pt_3(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_4(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_5(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_6(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_7(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_7, "");
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

static __unused void sse_pt_8(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_8, "");
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

static __unused void sse_pt_9(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_9, "");
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

static __unused void sse_pt_10(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_11(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_12(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_13(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_14(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_14, "");
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

static __unused void sse_pt_15(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_15, "");
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

static __unused void sse_pt_16(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_16, "");
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

static __unused void sse_pt_17(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_17, "");
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

static __unused void sse_pt_18(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_19(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_pt_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_20(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(sse_pt_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_21(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_22(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_23(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_24(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_pt_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_25(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_pt_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_26(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_27(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_28(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_29(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_29, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_30(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_31(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_32(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_33(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_34(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_34, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_35(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_36(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_37(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_37, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_38(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_38, "");
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

static __unused void sse_pt_39(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_39, "");
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

static __unused void sse_pt_40(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_40, "");
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

static __unused void sse_pt_41(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_41, "");
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

static __unused void sse_pt_42(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_42, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_43(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_pt_instruction_43, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_44(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(sse_pt_instruction_44, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_45(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_45, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_46(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_46, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_47(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_47, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_48(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_pt_instruction_48, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_49(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_pt_instruction_49, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_50(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_50, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_51(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_51, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_52(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_52, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_53(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_53, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_54(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_54, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_55(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_55, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_56(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_56, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_57(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_57, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_58(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_58, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_59(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_59, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_60(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_60, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_61(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_61, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_62(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_62, "");
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

static __unused void sse_pt_63(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_63, "");
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

static __unused void sse_pt_64(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_64, "");
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

static __unused void sse_pt_65(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_65, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_66(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_66, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_67(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_67, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_68(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_68, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_69(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_69, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_70(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_70, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_71(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_71, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_72(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_72, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_73(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_73, "");
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

static __unused void sse_pt_74(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_74, "");
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

static __unused void sse_pt_75(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_75, "");
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

static __unused void sse_pt_76(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_76, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_77(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_77, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_78(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_78, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_79(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_79, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_80(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_80, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_81(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_81, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_82(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_82, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_83(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_83, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_84(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_84, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_85(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_85, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_86(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_86, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_87(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_87, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_88(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_88, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_89(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_89, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_90(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_90, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_91(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_91, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_92(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_92, "");
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

static __unused void sse_pt_93(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_93, "");
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

static __unused void sse_pt_94(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_94, "");
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

static __unused void sse_pt_95(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_95, "");
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

static __unused void sse_pt_96(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_96, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_97(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_97, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_98(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_98, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_99(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_99, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_100(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_100, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_101(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_101, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_102(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_102, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_103(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_103, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_104(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_104, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_105(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_105, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_106(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_106, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_107(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_107, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_108(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_108, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_109(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_109, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_110(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_110, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_111(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_111, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_112(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_112, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_113(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_113, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_114(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_114, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_115(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_115, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_116(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_116, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_117(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_117, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_118(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_118, "");
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

static __unused void sse_pt_119(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_119, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_120(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_120, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_121(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_121, "");
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

static __unused void sse_pt_122(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_122, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_123(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_123, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_124(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_124, "");
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

static __unused void sse_pt_125(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_125, "");
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

static __unused void sse_pt_126(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_126, "");
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

static __unused void sse_pt_127(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_127, "");
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

static __unused void sse_pt_128(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_128, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_129(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_129, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_130(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_130, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_131(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_131, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_132(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_132, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_133(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_133, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_134(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_134, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_135(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_135, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_136(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_136, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_137(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_137, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_138(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_138, "");
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

static __unused void sse_pt_139(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_139, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_140(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_140, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_141(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_141, "");
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

static __unused void sse_pt_142(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_142, "");
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

static __unused void sse_pt_143(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_143, "");
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

static __unused void sse_pt_144(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_144, "");
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

static __unused void sse_pt_145(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_pt_instruction_145, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_146(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_pt_instruction_146, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_147(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_pt_instruction_147, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_148(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_148, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_149(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_149, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_150(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_instruction_150, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_151(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_pt_instruction_151, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_152(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_pt_instruction_152, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_153(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_153, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_154(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_154, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_155(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_155, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_156(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_156, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_157(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_157, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_158(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_158, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_159(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_159, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_160(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_160, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_161(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_161, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_162(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_162, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_163(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_163, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_164(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_164, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_165(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_165, "");
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

static __unused void sse_pt_166(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_166, "");
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

static __unused void sse_pt_167(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_167, "");
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

static __unused void sse_pt_168(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_168, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_169(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_169, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_170(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_170, "");
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

static __unused void sse_pt_171(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_171, "");
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

static __unused void sse_pt_172(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_172, "");
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

static __unused void sse_pt_173(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_173, "");
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

static __unused void sse_pt_174(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_pt_instruction_174, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_175(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_pt_instruction_175, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_176(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_pt_instruction_176, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_177(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_177, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_178(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_178, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_179(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_instruction_179, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_180(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_pt_instruction_180, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_181(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_pt_instruction_181, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_182(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_182, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_183(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_183, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_184(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_184, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_185(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_185, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_186(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_186, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_187(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_187, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_188(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_188, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_189(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_189, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_190(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_190, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_191(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_191, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_192(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_192, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_193(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_193, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_194(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_194, "");
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

static __unused void sse_pt_195(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_195, "");
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

static __unused void sse_pt_196(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_196, "");
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

static __unused void sse_pt_197(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_197, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_198(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_198, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_199(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_199, "");
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

static __unused void sse_pt_200(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_200, "");
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

static __unused void sse_pt_201(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_instruction_201, "");
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

static __unused void sse_pt_202(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_pt_instruction_202, "");
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

static __unused void sse_pt_203(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_pt_instruction_203, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_204(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring1(sse_pt_instruction_204, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_205(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring2(sse_pt_instruction_205, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_206(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_206, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_207(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring3(sse_pt_instruction_207, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_208(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_instruction_208, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_209(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring1(sse_pt_instruction_209, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_210(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring2(sse_pt_instruction_210, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_211(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_211, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_212(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring3(sse_pt_instruction_212, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_213(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_213, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_214(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_214, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_215(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_215, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_216(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_216, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_217(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_217, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_218(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_218, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_219(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_219, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_220(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_220, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_221(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_221, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_222(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_222, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_223(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_pt_instruction_223, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_224(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(sse_pt_instruction_224, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_225(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(sse_pt_instruction_225, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_226(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_226, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_227(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(sse_pt_instruction_227, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_228(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_pt_instruction_228, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_229(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(sse_pt_instruction_229, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_230(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(sse_pt_instruction_230, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_231(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_231, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_232(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(sse_pt_instruction_232, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_233(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_instruction_233, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_234(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring1(sse_pt_instruction_234, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_235(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring2(sse_pt_instruction_235, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_236(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_236, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_237(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring3(sse_pt_instruction_237, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_238(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_instruction_238, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_239(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(sse_pt_instruction_239, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_240(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(sse_pt_instruction_240, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_241(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_241, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_242(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(sse_pt_instruction_242, "");
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
	sse_pt_0();
	sse_pt_1();
	sse_pt_2();
	sse_pt_3();
	sse_pt_4();
	sse_pt_5();
	sse_pt_6();
	sse_pt_7();
	sse_pt_8();
	sse_pt_9();
	sse_pt_10();
	sse_pt_11();
	sse_pt_12();
	sse_pt_13();
	sse_pt_14();
	sse_pt_15();
	sse_pt_16();
	sse_pt_17();
	sse_pt_18();
	sse_pt_19();
	sse_pt_20();
	sse_pt_21();
	sse_pt_22();
	sse_pt_23();
	sse_pt_24();
	sse_pt_25();
	sse_pt_26();
	sse_pt_27();
	sse_pt_28();
	sse_pt_29();
	sse_pt_30();
	sse_pt_31();
	sse_pt_32();
	sse_pt_33();
	sse_pt_34();
	sse_pt_35();
	sse_pt_36();
	sse_pt_37();
	sse_pt_38();
	sse_pt_39();
	sse_pt_40();
	sse_pt_41();
	sse_pt_42();
	sse_pt_43();
	sse_pt_44();
	sse_pt_45();
	sse_pt_46();
	sse_pt_47();
	sse_pt_48();
	sse_pt_49();
	sse_pt_50();
	sse_pt_51();
	sse_pt_52();
	sse_pt_53();
	sse_pt_54();
	sse_pt_55();
	sse_pt_56();
	sse_pt_57();
	sse_pt_58();
	sse_pt_59();
	sse_pt_60();
	sse_pt_61();
	sse_pt_62();
	sse_pt_63();
	sse_pt_64();
	sse_pt_65();
	sse_pt_66();
	sse_pt_67();
	sse_pt_68();
	sse_pt_69();
	sse_pt_70();
	sse_pt_71();
	sse_pt_72();
	sse_pt_73();
	sse_pt_74();
	sse_pt_75();
	sse_pt_76();
	sse_pt_77();
	sse_pt_78();
	sse_pt_79();
	sse_pt_80();
	sse_pt_81();
	sse_pt_82();
	sse_pt_83();
	sse_pt_84();
	sse_pt_85();
	sse_pt_86();
	sse_pt_87();
	sse_pt_88();
	sse_pt_89();
	sse_pt_90();
	sse_pt_91();
	sse_pt_92();
	sse_pt_93();
	sse_pt_94();
	sse_pt_95();
	sse_pt_96();
	sse_pt_97();
	sse_pt_98();
	sse_pt_99();
	sse_pt_100();
	sse_pt_101();
	sse_pt_102();
	sse_pt_103();
	sse_pt_104();
	sse_pt_105();
	sse_pt_106();
	sse_pt_107();
	sse_pt_108();
	sse_pt_109();
	sse_pt_110();
	sse_pt_111();
	sse_pt_112();
	sse_pt_113();
	sse_pt_114();
	sse_pt_115();
	sse_pt_116();
	sse_pt_117();
	sse_pt_118();
	sse_pt_119();
	sse_pt_120();
	sse_pt_121();
	sse_pt_122();
	sse_pt_123();
	sse_pt_124();
	sse_pt_125();
	sse_pt_126();
	sse_pt_127();
	sse_pt_128();
	sse_pt_129();
	sse_pt_130();
	sse_pt_131();
	sse_pt_132();
	sse_pt_133();
	sse_pt_134();
	sse_pt_135();
	sse_pt_136();
	sse_pt_137();
	sse_pt_138();
	sse_pt_139();
	sse_pt_140();
	sse_pt_141();
	sse_pt_142();
	sse_pt_143();
	sse_pt_144();
	sse_pt_145();
	sse_pt_146();
	sse_pt_147();
	sse_pt_148();
	sse_pt_149();
	sse_pt_150();
	sse_pt_151();
	sse_pt_152();
	sse_pt_153();
	sse_pt_154();
	sse_pt_155();
	sse_pt_156();
	sse_pt_157();
	sse_pt_158();
	sse_pt_159();
	sse_pt_160();
	sse_pt_161();
	sse_pt_162();
	sse_pt_163();
	sse_pt_164();
	sse_pt_165();
	sse_pt_166();
	sse_pt_167();
	sse_pt_168();
	sse_pt_169();
	sse_pt_170();
	sse_pt_171();
	sse_pt_172();
	sse_pt_173();
	sse_pt_174();
	sse_pt_175();
	sse_pt_176();
	sse_pt_177();
	sse_pt_178();
	sse_pt_179();
	sse_pt_180();
	sse_pt_181();
	sse_pt_182();
	sse_pt_183();
	sse_pt_184();
	sse_pt_185();
	sse_pt_186();
	sse_pt_187();
	sse_pt_188();
	sse_pt_189();
	sse_pt_190();
	sse_pt_191();
	sse_pt_192();
	sse_pt_193();
	sse_pt_194();
	sse_pt_195();
	sse_pt_196();
	sse_pt_197();
	sse_pt_198();
	sse_pt_199();
	sse_pt_200();
	sse_pt_201();
	sse_pt_202();
	sse_pt_203();
	sse_pt_204();
	sse_pt_205();
	sse_pt_206();
	sse_pt_207();
	sse_pt_208();
	sse_pt_209();
	sse_pt_210();
	sse_pt_211();
	sse_pt_212();
	sse_pt_213();
	sse_pt_214();
	sse_pt_215();
	sse_pt_216();
	sse_pt_217();
	sse_pt_218();
	sse_pt_219();
	sse_pt_220();
	sse_pt_221();
	sse_pt_222();
	sse_pt_223();
	sse_pt_224();
	sse_pt_225();
	sse_pt_226();
	sse_pt_227();
	sse_pt_228();
	sse_pt_229();
	sse_pt_230();
	sse_pt_231();
	sse_pt_232();
	sse_pt_233();
	sse_pt_234();
	sse_pt_235();
	sse_pt_236();
	sse_pt_237();
	sse_pt_238();
	sse_pt_239();
	sse_pt_240();
	sse_pt_241();
	sse_pt_242();

	return report_summary();
}