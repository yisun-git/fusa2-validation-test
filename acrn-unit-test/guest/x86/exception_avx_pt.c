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

// define the data type
__unused __attribute__ ((aligned(32))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(32))) static union_unsigned_256 unsigned_256;
__unused __attribute__ ((aligned(32))) static u8 unsigned_8 = 0;
__unused __attribute__ ((aligned(32))) static u16 unsigned_16 = 0;
__unused __attribute__ ((aligned(32))) static u32 unsigned_32 = 0;
//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused __attribute__ ((aligned(32))) static u32 unsigned_64 = 0;
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

__unused static u32 *unaligned_address_32(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
__unused static u32 *unaligned_address_64(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

__unused static union_unsigned_128 * unaligned_address_128(void)
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

__unused static union_unsigned_256 * unaligned_address_256(void)
{
	__attribute__ ((aligned(32))) union_unsigned_256 addr_unalign_256;
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
__unused static u32 *trigger_pgfault(void)
{
	u32 *add1;
	add1 = malloc(sizeof(u32));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return (u32 *)add1;
}

// execution
static __unused void avx_pt_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VSUBSS %%ss:0xFFFF0000, %%xmm2, %%xmm1\n " "1:" : : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VANDPS %%ds:0xFFFF0000, %%ymm2, %%ymm1\n " "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VHADDPS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_3(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VSQRTPD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_4(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VCVTTPS2DQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPD2DQ  %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTDQ2PS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVHPS %[input_1], %%xmm1, %%xmm2\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void avx_pt_instruction_8(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_9(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_10(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VMULSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_11(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VSQRTSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_12(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	__unused u32 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_13(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VMAXSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_14(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VUCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_15(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_16(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VSQRTSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_17(const char *msg)
{
	unsigned_32 = 0x1;
	unsigned_64 = 0x1;
	memcpy(&unsigned_128, &unsigned_64, sizeof(unsigned_64));
	memcpy(&unsigned_256, &unsigned_64, sizeof(unsigned_64));
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	unsigned_32 = 0x0;
	unsigned_64 = 0x0;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VSUBSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMULSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VSUBSS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_22(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMINSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_23(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_25(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMAXSD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_27(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VUCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VANDPS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VANDPD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_30(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVHPD %[input_1], %%xmm1, %%xmm2\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_32(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVHPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVDDUP %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_34(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMASKMOVPS %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_35(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_36(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVHPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_37(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_38(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_39(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile(ASM_TRY("1f") "VMASKMOVPS %%ymm2, %%ymm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_256) : );
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPS %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(unaligned_address_128())) : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_41(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPD %%ymm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_256) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVAPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_43(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPADDUSW %%ss:0xFFFF0000, %%ymm2, %%ymm1\n " "1:" : : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPADDUSB %%ds:0xFFFF0000, %%ymm2, %%ymm1\n " "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPADDSW %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_46(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_256())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_47(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_48(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVNTDQA %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_49(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPADDSB %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_50(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPADDQ %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_51(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXWQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void avx_pt_instruction_52(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXWQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_53(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXBQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_54(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVQ %%xmm2, %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXWD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_56(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXBD %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_57(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVQ %%ymm2, %%ymm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_256) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_58(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVQ %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_59(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXBW %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMASKMOVQ %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_61(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVSXDQ %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_62(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %%ss:0xFFFF0000\n " "1:" : : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %%ds:0xFFFF0000\n " "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_64(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_65(const char *msg)
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

static __unused void avx_pt_instruction_66(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_67(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_68(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_69(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_70(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADDSUB213PD %%ss:0xFFFF0000, %%ymm2, %%ymm1\n " "1:" : : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_71(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADDSUB213PD %%ds:0xFFFF0000, %%xmm2, %%xmm1\n " "1:" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_72(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PS %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_73(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_74(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_75(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_76(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFMADD231PS %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_256));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_77(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void avx_pt_instruction_78(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_79(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_80(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_81(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_82(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_83(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_84(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_85(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_86(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_87(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_88(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_89(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_90(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_91(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_92(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_93(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_94(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_95(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_96(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_97(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VFNMSUB231SS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_instruction_98(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA  %%ss:0xFFFF0000, %%eax\n"   ASM_TRY("1f")
		"VPGATHERQD %%xmm2,  %%ss:0xFFFF0000(%%eax,%%xmm0,1), %%xmm1\n" "1:"
		:  : [input_2] "m" (unsigned_64));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == SS_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_99(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA  %%ds:0xFFFF0000, %%eax\n"   ASM_TRY("1f")
		"VPGATHERDD %%xmm2,  %%ds:0xFFFF0000(%%eax,%%xmm0,1), %%xmm1\n" "1:"
		:  : [input_2] "m" (unsigned_32));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_100(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VPGATHERDQ %%xmm2, (%%eax,%%xmm0,1), %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_101(const char *msg)
{
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VPGATHERDQ %%ymm2, (%%eax,%%xmm0,1), %%ymm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_instruction_102(const char *msg)
{
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm0\n" : : [input_1] "m" (unsigned_256) : "memory");
	memset(&unsigned_256, 0xFF, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	asm volatile("LEA %[input_2], %%eax\n"   ASM_TRY("1f") "VPGATHERDD %%ymm2, (%%eax,%%ymm0,1), %%ymm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	memset(&unsigned_256, 0x00, sizeof(unsigned_256));
	asm volatile("VMOVDQU %[input_1], %%ymm1\n" : : [input_1] "m" (unsigned_256) : "memory");
	asm volatile("VMOVDQU %%ymm1, %%ymm2\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_pt_instruction_3, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_pt_instruction_4, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_6, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_7, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_pt_instruction_8, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(avx_pt_instruction_9, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(avx_pt_instruction_10, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_pt_instruction_11, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_pt_instruction_12, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_pt_instruction_13, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(avx_pt_instruction_14, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(avx_pt_instruction_15, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_pt_instruction_16, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_pt_instruction_17, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_18, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_pt_instruction_19, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_pt_instruction_20, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_21, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_22, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_23, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(avx_pt_instruction_24, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(avx_pt_instruction_25, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_26, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_27, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_28, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_29, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_30, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_pt_instruction_31, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_pt_instruction_32, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_33, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_34, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_35, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(avx_pt_instruction_36, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(avx_pt_instruction_37, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_38, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_39, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_40, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_41, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_42, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_43, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_44, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_45, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_46, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_47, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_48, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_49, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_50, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_51, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_52, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_pt_instruction_53, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_pt_instruction_54, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_55, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_56, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_57, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_CR0_TS_0();
	do_at_ring1(avx_pt_instruction_58, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_CR0_TS_0();
	do_at_ring2(avx_pt_instruction_59, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_60, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_61, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_62, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_63, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_64, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_65, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_66, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_67, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_68, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	condition_CR0_TS_0();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_69, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_70, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_71, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_72, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_73(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_pt_instruction_73, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_74(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_pt_instruction_74, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_75(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_75, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_76(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_76, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_77(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_77, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_78(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(avx_pt_instruction_78, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_79(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring1(avx_pt_instruction_79, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_80(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring2(avx_pt_instruction_80, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_81(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_pt_instruction_81, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_82(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring3(avx_pt_instruction_82, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_83(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(avx_pt_instruction_83, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_84(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring1(avx_pt_instruction_84, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_85(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring2(avx_pt_instruction_85, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_86(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_pt_instruction_86, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_87(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_hold_AVX();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring3(avx_pt_instruction_87, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_88(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_88, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_89(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring1(avx_pt_instruction_89, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_90(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring2(avx_pt_instruction_90, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_91(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_91, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_92(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring3(avx_pt_instruction_92, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_93(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_93, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_94(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_1();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring1(avx_pt_instruction_94, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_95(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_2();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring2(avx_pt_instruction_95, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_96(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_0();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_96, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_97(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_aligned();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring3(avx_pt_instruction_97, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_98(void)
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
	condition_Seg_Limit_SSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_98, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_99(void)
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
	condition_Seg_Limit_DSeg();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_99, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_100(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_100, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_101(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_instruction_101, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_102(void)
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
	condition_Seg_Limit_No();
	condition_pgfault_not_occur();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_instruction_102, "");
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
	avx_pt_0();
	avx_pt_1();
	avx_pt_2();
	avx_pt_3();
	avx_pt_4();
	avx_pt_5();
	avx_pt_6();
	avx_pt_7();
	avx_pt_8();
	avx_pt_9();
	avx_pt_10();
	avx_pt_11();
	avx_pt_12();
	avx_pt_13();
	avx_pt_14();
	avx_pt_15();
	avx_pt_16();
	avx_pt_17();
	avx_pt_18();
	avx_pt_19();
	avx_pt_20();
	avx_pt_21();
	avx_pt_22();
	avx_pt_23();
	avx_pt_24();
	avx_pt_25();
	avx_pt_26();
	avx_pt_27();
	avx_pt_28();
	avx_pt_29();
	avx_pt_30();
	avx_pt_31();
	avx_pt_32();
	avx_pt_33();
	avx_pt_34();
	avx_pt_35();
	avx_pt_36();
	avx_pt_37();
	avx_pt_38();
	avx_pt_39();
	avx_pt_40();
	avx_pt_41();
	avx_pt_42();
	avx_pt_43();
	avx_pt_44();
	avx_pt_45();
	avx_pt_46();
	avx_pt_47();
	avx_pt_48();
	avx_pt_49();
	avx_pt_50();
	avx_pt_51();
	avx_pt_52();
	avx_pt_53();
	avx_pt_54();
	avx_pt_55();
	avx_pt_56();
	avx_pt_57();
	avx_pt_58();
	avx_pt_59();
	avx_pt_60();
	avx_pt_61();
	avx_pt_62();
	avx_pt_63();
	avx_pt_64();
	avx_pt_65();
	avx_pt_66();
	avx_pt_67();
	avx_pt_68();
	avx_pt_69();
	avx_pt_70();
	avx_pt_71();
	avx_pt_72();
	avx_pt_73();
	avx_pt_74();
	avx_pt_75();
	avx_pt_76();
	avx_pt_77();
	avx_pt_78();
	avx_pt_79();
	avx_pt_80();
	avx_pt_81();
	avx_pt_82();
	avx_pt_83();
	avx_pt_84();
	avx_pt_85();
	avx_pt_86();
	avx_pt_87();
	avx_pt_88();
	avx_pt_89();
	avx_pt_90();
	avx_pt_91();
	avx_pt_92();
	avx_pt_93();
	avx_pt_94();
	avx_pt_95();
	avx_pt_96();
	avx_pt_97();
	avx_pt_98();
	avx_pt_99();
	avx_pt_100();
	avx_pt_101();
	avx_pt_102();

	return report_summary();
}