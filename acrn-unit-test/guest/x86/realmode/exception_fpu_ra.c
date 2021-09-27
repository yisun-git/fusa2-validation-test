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

static __unused void fpu_ra_instruction_0(const char *msg)
{
	u16 cw = 0;
	asm volatile(ASM_TRY("1f") "FNSTCW %0\n" "1:\n" : "=m"(cw) : : "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_ra_instruction_1(const char *msg)
{
	u16 sw = 0;
	asm volatile(ASM_TRY("1f") "FSTSW %0\n" "1:\n" : "=m"(sw) : : "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void fpu_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	do_at_ring0(fpu_ra_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void fpu_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	condition_D_segfault_not_occur();
	do_at_ring0(fpu_ra_instruction_1, "");
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
	#ifdef PHASE_0
	fpu_ra_0();
	fpu_ra_1();
	#endif

	return report_summary();
}