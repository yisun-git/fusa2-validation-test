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
#endif

// trigger_pgfault
static __unused u64 *trigger_pgfault(void)
{
	u64 *add1;
	add1 = malloc(sizeof(u64));
	set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	return (u64 *)add1;
}

// execution
static __unused void test_VCVTPD2DQ_0_ud(void *data)
{
	asm volatile(ASM_TRY("1f") ".byte 0xc5, 0xeb, 0xe6, 0xca \n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void test_VGATHERDPS_ud(void *data)
{
	asm volatile(ASM_TRY("1f") ".byte 0x67, 0xc4, 0xe2, 0x69, 0x92, 0xc0, 0x00 \n" "1:"
				:  : [input_1] "m" (unsigned_32));
}
static __unused void avx_pt_b6_instruction_0(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTPD2DQ_0_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPD2PS %%xmm2, %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTTPD2DQ %%xmm2, %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVLHPS %%xmm3, %%xmm2, %%xmm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VMOVHLPS %%xmm3, %%xmm2, %%xmm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_5(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VPADDSB %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_6(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VPADDB %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_7(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VPADDD %[input_1], %%ymm2, %%ymm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPMOVMSKB %%ymm1, %[output]\n" "1:"
	#ifdef __x86_64__
				: [output] "=r" (unsigned_64) : );
	#else
				: [output] "=r" (unsigned_32) : );
	#endif
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VPSLLDQ $0x1, %%ymm2, %%ymm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPH2PS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "VCVTPH2PS %[input_1], %%ymm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_13(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_14(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADDSUB132PD %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_15(const char *msg)
{
	__unused u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "VFMADD231PS %[input_1], %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_16(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VGATHERDPS_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xec, 0x77 \n 1:\n"
				: : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"VZEROUPPER \n 1:\n"
				: : );
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_b6_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"VZEROUPPER \n 1:\n"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void avx_pt_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_hold();
	do_at_ring0(avx_pt_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_0();
	do_at_ring0(avx_pt_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_not_enabled();
	do_at_ring0(avx_pt_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_b6_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_b6_instruction_4, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_hold();
	do_at_ring0(avx_pt_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_0();
	do_at_ring0(avx_pt_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_not_enabled();
	do_at_ring0(avx_pt_b6_instruction_7, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_b6_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_hold();
	do_at_ring0(avx_pt_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_0();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_b6_instruction_11, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_not_enabled();
	do_at_ring0(avx_pt_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_hold();
	do_at_ring0(avx_pt_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_0();
	do_at_ring0(avx_pt_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_not_enabled();
	do_at_ring0(avx_pt_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_Inv_AVX2_hold();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_b6_instruction_16, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_VEX_4v_1s_not_hold();
	do_at_ring0(avx_pt_b6_instruction_17, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_VEX_4v_1s_hold();
	condition_CR0_TS_1();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_b6_instruction_18, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	condition_Inv_Prf_not_hold();
	condition_CR4_OSXSAVE_1();
	condition_st_comp_enabled();
	condition_VEX_4v_1s_hold();
	condition_CR0_TS_0();
	do_at_ring0(avx_pt_b6_instruction_19, "");
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
	avx_pt_b6_0();
	avx_pt_b6_1();
	avx_pt_b6_2();
	avx_pt_b6_3();
	avx_pt_b6_4();
	avx_pt_b6_5();
	avx_pt_b6_6();
	avx_pt_b6_7();
	avx_pt_b6_8();
	avx_pt_b6_9();
	avx_pt_b6_10();
	avx_pt_b6_11();
	avx_pt_b6_12();
	avx_pt_b6_13();
	avx_pt_b6_14();
	avx_pt_b6_15();
	avx_pt_b6_16();
	avx_pt_b6_17();
	avx_pt_b6_18();
	avx_pt_b6_19();

	return report_summary();
}