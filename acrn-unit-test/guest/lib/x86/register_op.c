#include "vm.h"
#include "libcflat.h"
#include "processor.h"
#include "misc.h"
#include "desc.h"
#include "fwcfg.h"
#include "xsave.h"
#include "register_op.h"

int write_cr3_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
		"mov %0, %%cr3\n\t"
		"1:"
		: : "r"(val));
	return exception_vector();
}

int write_cr4_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
		"mov %0,%%cr4\n\t"
		"1:" : : "r" (val));
	return exception_vector();
}

int rdmsr_checking(u32 MSR_ADDR, u64 *result)
{
	u32 eax;
	u32 edx;

	asm volatile(ASM_TRY("1f")
		"rdmsr \n\t"
		"1:"
		: "=a"(eax), "=d"(edx) : "c"(MSR_ADDR));
	if (result) {
		*result = eax + ((u64)edx << 32);
	}
	return exception_vector();
}

int wrmsr_checking(u32 MSR_ADDR, u64 value)
{
	u32 edx = value >> 32;
	u32 eax = value;

	asm volatile(ASM_TRY("1f")
		"wrmsr \n\t"
		"1:"
		: : "c"(MSR_ADDR), "a"(eax), "d"(edx));
	return exception_vector();
}

bool test_rdmsr_exception(uint32_t msr, uint32_t vector, uint32_t error_code)
{
	uint64_t value;
	int result = rdmsr_checking(msr, &value);
	if ((result == vector) && (exception_error_code() == error_code)) {
		return true;
	} else {
		return false;
	}
}

bool test_wrmsr_exception(uint32_t msr, uint64_t value, uint32_t vector, uint32_t error_code)
{
	int result = wrmsr_checking(msr, value);
	if ((result == vector) && (exception_error_code() == error_code)) {
		return true;
	} else {
		return false;
	}
}

int write_cr4_osxsave(u32 bitvalue)
{
	u32 cr4;

	cr4 = read_cr4();
	if (bitvalue) {
		return write_cr4_checking(cr4 | X86_CR4_OSXSAVE);
	} else {
		return write_cr4_checking(cr4 & ~X86_CR4_OSXSAVE);
	}
}

/**Function about XSAVE feature: **/
int xsetbv_checking(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile(ASM_TRY("1f")
		"xsetbv\n\t" /* xsetbv */
		"1:"
		: : "a" (eax), "d" (edx), "c" (index));
	return exception_vector();
}

int xgetbv_checking(u32 index, u64 *result)
{
	u32 eax, edx;

	asm volatile(ASM_TRY("1f")
		".byte 0x0f,0x01,0xd0\n\t" /* xgetbv */
		"1:"
		: "=a" (eax), "=d" (edx)
		: "c" (index));
	*result = eax + ((u64)edx << 32);
	return exception_vector();
}

