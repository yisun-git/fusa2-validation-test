#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "types.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "ioram.h"
#include "alloc.h"
#include "mpx.h"
#include "regdump.h"
#include "misc.h"
#include "instruction_common.h"
#include "xsave.h"
#include "register_op.h"

/*
 * Software can use the XSAVE feature set to manage MPX state only if XCR0[4:3] = 11b.
 * In addition, software can execute MPX instructions only if  CR4.OSXSAVE = 1 and XCR0[4:3] = 11b.
 * Otherwise, any execution of an MPX instruction causes an invalid opcode exception (#UD).
 */
#ifdef __x86_64__
/**
 * @brief case name:MPX environment hiding_register_BNDREGS_001
 *
 * Summary: ACRN hypervisor shall hide MPX environment from any VM,
 * in compliance with Chapter 17.3 and Chapter 17.4, Vol. 1, SDM.
 *
 *	BNDREGS:Bound register component of Intel MPX state should be hided.
 * CPUID.(EAX=0DH,ECX=0):EAX[3] should be 0
 */
static void  mpx_rqmid_23119_env_hiding_register_bndregs_001(void)
{
	report("%s", (cpuid(0x0d).a & (1 << 3)) == 0, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_register_BNDCSR_001
 *
 * Summary: ACRN hypervisor shall hide MPX environment from any VM,
 * in compliance with Chapter 17.3 and Chapter 17.4, Vol. 1, SDM.
 *
 * Bounds configuration and status component of Intel MPX should be hided.
 * CPUID.(EAX=0DH,ECX=0):EAX[4] should be 0.
 */
static void mpx_rqmid_23120_env_hiding_register_bndcsr_001(void)
{
	report("%s", (cpuid(0x0d).a & (1 << 4)) == 0, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_xcr0[3,4]_002
 *
 * Summary: Under enable XSAVE feature set,execute to write XCR0[bits 4:3] shall generate #GP
 */
static void mpx_rqmid_23626_write_xcr0_bit_3_4(void)
{
	uint64_t test_bits;
	ulong cr4;

	cr4 = read_cr4();
	write_cr4_checking(cr4 | X86_CR4_OSXSAVE);

	test_bits = STATE_X87 | STATE_SSE | STATE_AVX;
	if (xsetbv_checking(XCR0_MASK, test_bits) != NO_EXCEPTION) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	test_bits = STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDREGS | STATE_MPX_BNDCSR;
	report("%s", xsetbv_checking(XCR0_MASK, test_bits) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name:MPX environment hiding_exception_BNDLDX_001
 *
 * Summary: When the bound directory entry is invalid,execute
 *          BNDLDX shall no exception and the the values of
 *          MSRs/Xsave/General purpose registers shall not change.
 */
static void mpx_rqmid_25251_bndldx_exception(void)
{
	u32 size;
	int a = 0xffff;
	bool result = true;
	void *ptr1, *ptr2;
	struct gen_reg_struct gen_reg1, gen_reg2;
	struct xsave_dump_struct *xsave1, *xsave2;

	memset(&gen_reg1, 0, sizeof(gen_reg1));
	memset(&gen_reg2, 0, sizeof(gen_reg2));

	gen_reg_dump(&gen_reg1);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	gen_reg_dump(&gen_reg2);
	if (memcmp(&gen_reg1, &gen_reg2, sizeof(struct gen_reg_struct))) {
		result = false;
	}

	ptr1 = msr_reg_dump(&size);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	ptr2 = msr_reg_dump(&size);
	if (memcmp(ptr1, ptr1, size)) {
		result = false;
	}
	free(ptr1);
	free(ptr2);

	xsave1 = malloc(sizeof(struct xsave_dump_struct));
	xsave2 = malloc(sizeof(struct xsave_dump_struct));
	assert(xsave1);
	assert(xsave2);
	xsave_reg_dump(xsave1);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	xsave_reg_dump(xsave2);
	if (memcmp(xsave1, xsave2, sizeof(struct xsave_dump_struct))) {
		result = false;
	}
	free(xsave1);
	free(xsave2);

	report("%s", result, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_exception_BNDMK_001
 *
 * Summary: Under the LOCK prefix is used execute BNDMK shall generate #UD
 */
static void mpx_rqmid_23894_lock_prefix_test(void)
{
	int a = 0;

	/**BNDMK*/
	asm volatile(ASM_TRY("1f")
				 ".byte 0xf0,0xf3,0x0f,0x1b,0x45,0xdc\n\t"
				 "1:"
				 : : "m"(a));

	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

#endif
#ifdef __i386__
/**
 * @brief case name:MPX environment hiding_instruction_BNDLDX_002
 *
 * Summary: When enable XSAVE feature set,execute BNDLDX shall no exception and
 *          shall not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23623_bndldx_normal(void)
{
	u32 size;
	int a = 0xffff;
	bool result = true;
	void *ptr1, *ptr2;
	struct gen_reg_struct gen_reg1, gen_reg2;
	struct xsave_dump_struct *xsave1, *xsave2;

	memset(&gen_reg1, 0, sizeof(gen_reg1));
	memset(&gen_reg2, 0, sizeof(gen_reg2));

	gen_reg_dump(&gen_reg1);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	gen_reg_dump(&gen_reg2);

	if (memcmp(&gen_reg1, &gen_reg2, sizeof(struct gen_reg_struct))) {
		result = false;
	}

	ptr1 = msr_reg_dump(&size);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	ptr2 = msr_reg_dump(&size);

	if (memcmp(ptr1, ptr1, size)) {
		result = false;
	}
	free(ptr1);
	free(ptr2);

	xsave1 = malloc(sizeof(struct xsave_dump_struct));
	xsave2 = malloc(sizeof(struct xsave_dump_struct));
	assert(xsave1);
	assert(xsave2);
	xsave_reg_dump(xsave1);
	asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a));
	xsave_reg_dump(xsave2);

	if (memcmp(xsave1, xsave2, sizeof(struct xsave_dump_struct))) {
		result = false;
	}
	free(xsave1);
	free(xsave2);

	report("%s", result, __FUNCTION__);
}
#endif
static void print_case_list(void)
{
	printf("MPX feature case list:\n\r");
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 23119,
		   "MPX environment hiding_register_BNDREGS_001");
	printf("\t Case ID:%d case name:%s\n\r", 23120,
		   "MPX environment hiding_register_BNDCSR_001");

	printf("\t Case ID:%d case name:%s\n\r", 23626,
		   "MPX environment hiding_xcr0[3,4]_002");
	printf("\t Case ID:%d case name:%s\n\r", 25251,
		   "MPX environment hiding_exception_BNDLDX_001");
	printf("\t Case ID:%d case name:%s\n\r", 23894,
		   "MPX environment hiding_exception_BNDMK_001");
#elif __i386__
	printf("\t Case ID:%d case name:%s\n\r", 23623,
		   "MPX environment hiding_instruction_BNDLDX_002");
#endif
}

static void test_mpx(void)
{
#ifdef __x86_64__
	mpx_rqmid_23119_env_hiding_register_bndregs_001();
	mpx_rqmid_23120_env_hiding_register_bndcsr_001();
	mpx_rqmid_23626_write_xcr0_bit_3_4();
	mpx_rqmid_25251_bndldx_exception();
	mpx_rqmid_23894_lock_prefix_test();
#elif __i386__
	mpx_rqmid_23623_bndldx_normal();
#endif
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();
	test_mpx();
	return report_summary();
}
