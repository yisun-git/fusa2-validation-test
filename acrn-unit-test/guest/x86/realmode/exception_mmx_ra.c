asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;
extern u16 execption_inc_len;

void test_pxor_gp_exception(void *data)
{
	asm volatile(ASM_TRY("1f") "PXOR %%ds:0xFFFF, %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
}

static __unused void mmx_ra_instruction_0(const char *msg)
{
	test_for_exception(GP_VECTOR, test_pxor_gp_exception, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void mmx_ra_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void mmx_ra_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mmx_ra_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void mmx_ra_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PXOR %[input_1], %%mm1\n" "1:\n"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mmx_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(mmx_ra_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(mmx_ra_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_ra_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(mmx_ra_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_ra_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(mmx_ra_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	asm volatile("fninit");
}

static __unused void mmx_ra_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_MMX_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(mmx_ra_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
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
	mmx_ra_0();
	mmx_ra_1();
	mmx_ra_2();
	mmx_ra_3();
	mmx_ra_4();
	#endif

	return report_summary();
}