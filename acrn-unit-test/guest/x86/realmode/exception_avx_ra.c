asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
__attribute__ ((aligned(64))) __unused static union_unsigned_128 unsigned_128;
__attribute__ ((aligned(64))) __unused static union_unsigned_256 unsigned_256;
u8 exception_vector_bak = 0xFF;
extern u16 execption_inc_len;

void test_VMAXPD_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xe9, 0x5f, 0x0d, 0x40, 0x06, 0x06, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
}

void test_VPAVGB_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xc5, 0xed, 0xe0, 0x0d, 0xe0, 0xf5, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_256));
}

void test_VCVTPS2PH_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xc4, 0xe3, 0x79, 0x1d, 0x15, 0xa0, 0xf5, 0x05, 0x01, 0x01\n" "1:\n"
				: [output] "=m" (unsigned_64) : );
}

void test_VFMSUBADD132PD_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xc4, 0xe2, 0xe9, 0x97, 0x0d, 0x00, 0xf6, 0x05, 0x01\n" "1:\n"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void avx_ra_instruction_0(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VMAXPD_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_ra_instruction_1(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VPAVGB_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_ra_instruction_2(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VCVTPS2PH_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_ra_instruction_3(const char *msg)
{
	test_for_exception(UD_VECTOR, test_VFMSUBADD132PD_ud, 0);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void avx_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	do_at_ring0(avx_ra_instruction_0, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_AVX2_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	do_at_ring0(avx_ra_instruction_1, "");
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_ra_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_F16C_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	execption_inc_len = 3;
	do_at_ring0(avx_ra_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void avx_ra_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr4 = read_cr4();
	condition_CPUID_FMA_1();
	condition_LOCK_not_used();
	condition_VEX_used();
	do_at_ring0(avx_ra_instruction_3, "");
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
	#ifdef PHASE_0
	avx_ra_0();
	avx_ra_1();
	avx_ra_2();
	avx_ra_3();
	#endif

	return report_summary();
}