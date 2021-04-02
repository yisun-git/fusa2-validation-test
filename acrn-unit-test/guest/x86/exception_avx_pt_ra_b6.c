#define PT_B6
#ifdef PT_B6
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
#else
asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
#endif

__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
__attribute__ ((aligned(64))) __unused static union_unsigned_128 unsigned_128;
__attribute__ ((aligned(64))) __unused static union_unsigned_256 unsigned_256;
u8 exception_vector_bak = 0xFF;
#ifdef PT_B6
u16 execption_inc_len = 0;
#else
extern u16 execption_inc_len;
#endif

#ifdef PT_B6
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
#endif

void test_VCVTPS2DQ_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VCVTPS2DQ %[input_1], %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xf9, 0x5b, 0x0d, 0x00, 0xf6, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#endif
}

void test_VCVTDQ2PS_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VCVTDQ2PS %[input_1], %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xf8, 0x5b, 0x0d, 0x00, 0xf6, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#endif
}

void test_VPADDSW_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VPADDSW %[input_1], %%ymm2, %%ymm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_256));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xed, 0xed, 0x0d, 0xe0, 0xf5, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_256));
	#endif
}

void test_VPADDQ_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VPADDQ %[input_1], %%ymm2, %%ymm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_256));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xed, 0xd4, 0x0d, 0xe0, 0xf5, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_256));
	#endif
}

void test_VCVTPS2PH_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VCVTPS2PH $0x1, %%xmm2, %[output]\n" "1:\n"
				: [output] "=m" (unsigned_64) : );
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc4, 0xe3, 0x79, 0x1d, 0x15, 0xa0, 0xf5, 0x05, 0x01, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	#endif
}

void test_VFMADDSUB213PD_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VFMADDSUB213PD %[input_1], %%xmm2, %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc4, 0xe2, 0xe9, 0xa6, 0x0d, 0x00, 0xf6, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#endif
}

void test_VFMADDSUB231PD_ud(void *data)
{
	#ifdef PT_B6
	asm volatile(ASM_TRY("1f") "VFMADDSUB231PD %[input_1], %%xmm2, %%xmm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#else
	asm volatile(ASM_TRY("1f")
				".byte 0xc4, 0xe2, 0xe9, 0xb6, 0x0d, 0x00, 0xf6, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
	#endif
}

static __unused void avx_pt_ra_b6_instruction_0(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTPS2DQ_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_1(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTDQ2PS_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_2(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VPADDSW_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_3(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VPADDQ_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_4(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTPS2PH_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_5(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTPS2PH_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_6(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VFMADDSUB213PD_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_instruction_7(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VFMADDSUB231PD_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_pt_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_0();
	do_at_ring0(avx_pt_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_used();
	do_at_ring0(avx_pt_ra_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_0();
	do_at_ring0(avx_pt_ra_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_used();
	do_at_ring0(avx_pt_ra_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_0();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_ra_b6_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_used();
	execption_inc_len = 3;
	do_at_ring0(avx_pt_ra_b6_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_0();
	do_at_ring0(avx_pt_ra_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_pt_ra_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_used();
	do_at_ring0(avx_pt_ra_b6_instruction_7, "");
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
	avx_pt_ra_b6_0();
	avx_pt_ra_b6_1();
	avx_pt_ra_b6_2();
	avx_pt_ra_b6_3();
	avx_pt_ra_b6_4();
	avx_pt_ra_b6_5();
	avx_pt_ra_b6_6();
	avx_pt_ra_b6_7();

	return report_summary();
}