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
#include "register_op.h"
__attribute__((aligned(64))) xsave_area_t st_xsave_area;

static __unused int xsave_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
				 "xsave %[addr]\n\t"
				 "1:"
				 : : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
				 : "memory");

	return exception_vector();
}
static __unused int xrstors_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
				 "xrstors %[addr]\n\t"
				 "1:"
				 : : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
				 : "memory");

	return exception_vector();
}
/*
 * Software can use the XSAVE feature set to manage MPX state only if XCR0[4:3] = 11b.
 * In addition, software can execute MPX instructions only if  CR4.OSXSAVE = 1 and XCR0[4:3] = 11b.
 * Otherwise, any execution of an MPX instruction causes an invalid opcode exception (#UD).
 */
#ifdef __i386__
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
/**
 * @brief case name:MPX environment hiding_exception_BNDCL_001
 *
 * Summary: If 67H prefix is not used and CS.D=0,execute BNDCL
 *          would gernerate #UD.Because of mpx is hide, it would
 *          execute with no exception and does not affect the values
 *          of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23993_environment_hiding_exception_BNDCL_001(void)
{
	int b = 10;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(
			  asm volatile("ljmpl $0x28, $1f\n\t"
						   ".code16\n"
						   "1: \n\t"
						   "bndcl %0, %%bnd0\n\t"
						   "ljmpl $0x8, $2f\n\t"
						   ".code32\n"
						   "2:"
						   : : "m"(b)));

	report("%s", ret, __FUNCTION__);
}

/**
 * @brief case name:MPX environment hiding_exception_BNDCU_001
 *
 * Summary: If 67H prefix is used and CS.D=1,execute BNDCU
 *          would gernerate #UD.Because of mpx is hide, it would
 *          execute with no exception and does not affect the values
 *          of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23899_environment_hiding_exception_BNDCU_001(void)
{
	int b = 10;
	bool ret = false;

	enable_xsave();
	/* MPX is hiden, here will do not generate #UD*/
	/*add 67H prefix for BNDCU*/
	ret = CHECK_INSTRUCTION_REGS(asm volatile(".byte 0x67,0xf3,0x0f,0x1a,0x45,0xdc\n\t" : : "m"(b)));
	report("%s", ret, __FUNCTION__);
}
#endif
#ifdef __x86_64__
/**
 * @brief case name:MPX environment hiding_exception_BNDCN_001
 *
 * Summary: If ModRM.r/m and REX encodes BND4-BND15 when Intel
 *          MPX is enabled,execute BNDCN would gernerate #UD.
 *          Because of mpx is hide, it would execute with no exception and
 *          does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_25254_environment_hiding_exception_BNDCN_001(void)
{
	int a = 10;
	bool ret = false;

	enable_xsave();
	/*add 66H prefix for BNDCN*/
	ret = CHECK_INSTRUCTION_REGS(asm volatile(".byte 0x66,0x0f,0x1b,0xc7\n\t" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}

/**
 * @brief case name:MPX environment hiding_exception_BNDLDX_001
 *
 * Summary: When the bound directory entry is invalid,execute
 *          BNDLDX shall no exception and the the values of
 *          MSRs/Xsave/General purpose registers shall not change.
 */
static void mpx_rqmid_25251_environment_hiding_exception_BNDLDX_001(void)
{
	int a = 0xffff;
	bool result = false;

	enable_xsave();
	result = CHECK_INSTRUCTION_REGS(asm volatile(ASM_TRY("1f")
									"bndldx %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", result, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_exception_BNDSTX_001
 *
 * Summary: If the destination operand points to a non-writable segment,
 *          execute BNDSTX would gernerate #GP.Because of mpx is hide,
 *          it would execute with no exception and does not affect the values
 *          of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_25252_environment_hiding_exception_BNDSTX_001(void)
{
	int c = 0xffff;
	int a = 0x80754567;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndstx %%bnd0, %1\n\t" : "=m"(a) : "m"(c)));
	report("%s", ret, __FUNCTION__);
}

static void mpx_bndmov_exception_ss_at_ring3(const char *msg)
{
	ulong op1;
	ulong addr = 0;
	bool ret = false;

	op1 = (ulong)&addr;
	op1 ^= (1UL << 63);/*make non-cannoical addr*/

	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndmov %0, %%bnd0"::"m"(*(ulong *)op1)));
	report("%s", ret, msg);
}

/**
 * @brief case name:MPX environment hiding_exception_BNDMOV_001
 *
 * Summary: If the memory address referencing the SS segment
 *          is in a non-canonical form,execute BNDMOV would gernerate #SS.
 *          Because of mpx is hide, it would execute with no exception and
 *          does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23898_environment_hiding_exception_BNDMOV_001(void)
{
	enable_xsave();
	do_at_ring3((void *)mpx_bndmov_exception_ss_at_ring3, __FUNCTION__);
}
static bool ret_ring3 = false;
static void mpx_bndmov_ac_at_ring3(const char *msg)
{
	typedef struct ac_test {
		char e;
		int  d;
		int  f;
	} __attribute__((packed)) ac_test_t;

	ac_test_t ac_t;

	ret_ring3 = CHECK_INSTRUCTION_REGS(asm volatile(ASM_TRY("1f")
									   "bndmov %0, %%bnd0\n\t"
									   "1:"
									   : : "m"(ac_t.d)));
}

/**
 * @brief case name:MPX environment hiding_exception_BNDMOV_002
 *
 * Summary: If alignment checking is enabled and an unaligned memory
 *          reference is made while CPL is 3,execute BNDMOV would gernerate #AC.
 *          Because of mpx is hide, it would execute with no exception and
 *          does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void __unused mpx_rqmid_24588_environment_hiding_exception_BNDMOV_002(void)
{
	ulong cr0 = read_cr0();

	write_cr0(cr0 | X86_CR0_AM);
	u64 rflags = read_rflags();
	write_rflags(rflags | X86_EFLAGS_AC);

	enable_xsave();
	do_at_ring3((void *)mpx_bndmov_ac_at_ring3, __FUNCTION__);
	report("%s", ret_ring3, __FUNCTION__);
	ret_ring3 = false;
}
/**
 * @brief case name:MPX environment hiding_exception_BNDMOV_003
 *
 * Summary: If a page fault occurs,execute BNDMOV would gernerate #PF.
 *          Because of mpx is hide, it would execute with no exception and
 *          does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_25250_environment_hiding_exception_BNDMOV_003(void)
{
	u64 *fault_addr;
	bool ret = false;

	enable_xsave();
	fault_addr = alloc_vpage();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndmov %%bnd0, %0\n\t" : "=m"(*fault_addr)::"memory"));

	report("%s", ret, __FUNCTION__);
}
/*Strategy Hide MPX Capability:
 *This test suites contains the test strategy covering the methods that checks the MPX Capability.
 */
/**
 * @brief case name:MPX environment hiding_CPUID_MPX_001
 *
 * Summary: Check if the processor hide MPX(CPUID.(EAX=07H,ECX=0H):
 *			EBX.MPX[bit 14] = 0 indicate the processor hide MPX)
 */
static void mpx_rqmid_23105_environment_hiding_CPUID_MPX_001(void)
{
	report("%s", ((raw_cpuid(7, 0).b) & MPX_BIT) == 0, __FUNCTION__);
}

static void fpu_mpx_hiding_cpuid_mawau_at_ring3(const char *msg)
{
	report("%s", ((cpuid(7).c >> 17) & 0x1f) == 0, msg);
}
/*
 * @brief case name: MPX environment hiding_CPUID_MAWAU_001
 *
 * Summary: Vol 1_17.3.1  If CPL = 3, the user MAWA (MAWAU) is used. The value of MAWAU is enumerated in
 * CPUID.(EAX=07H,ECX=0H):ECX.MAWAU[bits 21:17].
 *
 * Verify the value of CPUID.(EAX=07H, ECX=0H):ECX.MAWAU [bit 21:17]  equal zero when CPL=3
 */
static void mpx_rqmid_23106_environment_hiding_CPUID_MAWAU_001(void)
{
	do_at_ring3(fpu_mpx_hiding_cpuid_mawau_at_ring3, __FUNCTION__);
}
/*Hide MPX Registers :
 *This test suites contains the test strategy covering the methods
 *that checks the MPX Registers related to MPX.
 */

/*
 * @brief case name: MPX environment hiding_register_IA32_BNDCFGS_001
 * Vol 1_17.3.4 The RDMSR and WRMSR instructions can be used to read and write the IA32_BNDCFGS MSR
 *
 * Read IA32_BNDCFGS would generate #GP(0) because MPX hide.
 */
static void mpx_rqmid_23107_environment_hiding_register_IA32_BNDCFGS_001(void)
{
	u32 a, d;

	asm volatile(ASM_TRY("1f")
				 "rdmsr\n\t"
				 "1:"
				 : "=a"(a), "=d"(d) : "c"(MSR_IA32_BNDCFGS) : "memory");

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief case name:MPX environment hiding_register_IA32_BNDCFGS_002
 *
 * Summary: Write IA32_BNDCFGS would gernerate #GP because MPX hide
 */
static void mpx_rqmid_23824_environment_hiding_register_IA32_BNDCFGS_002(void)
{
	report("%s", wrmsr_checking(MSR_IA32_BNDCFGS, 1) == GP_VECTOR, __FUNCTION__);
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
 * @brief case name:MPX_environment_hiding_xcr0[3,4]_001
 *
 * Summary: Under enable XSAVE feature set,execute to dump XCR0[bits 4:3] shall be 0
 */
static void mpx_rqmid_23625_environment_hiding_xcr0_bit3_bit4_001(void)
{
	ulong cr4;
	u64 supported_xcr0;

	cr4 = read_cr4();
	write_cr4_checking(cr4 | X86_CR4_OSXSAVE);

	supported_xcr0 = get_supported_xcr0();
	report("%s", ((supported_xcr0 >> 3) & 3) == 0, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_xcr0[3,4]_002
 *
 * Summary: Under enable XSAVE feature set,execute to write XCR0[bit4] shall generate #GP
 */
static void mpx_rqmid_23626_environment_hiding_xcr0_bit3_bit4_002(void)
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

	test_bits = STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDCSR;
	report("%s", xsetbv_checking(XCR0_MASK, test_bits) == GP_VECTOR, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_xcr0[3,4]_003
 *
 * Summary: Under enable XSAVE feature set,execute to write XCR0[bit3] shall generate #GP
 */
static void mpx_rqmid_39257_environment_hiding_xcr0_bit3_bit4_003(void)
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

	test_bits = STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDREGS;
	report("%s", xsetbv_checking(XCR0_MASK, test_bits) == GP_VECTOR, __FUNCTION__);
}
/*339:Hide MPX instructions
 *This test suites contains the test strategy covering the methods
 *that checks the MPX instructions should be hidden.
 */
/**
 * @brief case name:MPX environment hiding_instruction_BNDMK_001
 *
 * Summary: Verify that the BNDMK instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23096_environment_hiding_instruction_BNDMK_001(void)
{
	int addr = 0;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndmk %0, %%bnd0\n\t" : : "m"(addr)));
	report("%s", ret, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_instruction_BNDCL_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23099_environment_hiding_instruction_BNDCL_001(void)
{
	int a = 10;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndcl %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MPX environment hiding_instruction_BNDCU_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23100_environment_hiding_instruction_BNDCU_001(void)
{
	int a = 10;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndcu %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MPX environment MPX environment hiding_instruction_BNDCN_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23101_environment_hiding_instruction_BNDCN_001(void)
{
	int a = 10;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndcn %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MPX environment hiding_instruction_BNDMOV_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purposeregisters.
 */
static void mpx_rqmid_23102_environment_hiding_instruction_BNDMOV_001(void)
{
	int a = 0;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndmov %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MPX environment hiding_instruction_BNDLDX_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23103_environment_hiding_instruction_BNDLDX_001(void)
{
	int a = 0;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndldx %0, %%bnd0" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MPX environment hiding_instruction_BNDSTX_001
 *
 * Summary: Verify that the instruction can be executed
 * with no exception and does not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23104_environment_hiding_instruction_BNDSTX_001(void)
{
	int a = 0;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndstx %%bnd0, %0" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
}
#endif
#ifdef __i386__
/**
 * @brief case name:MPX environment hiding_instruction_BNDLDX_002
 *
 * Summary: When enable XSAVE feature set,execute BNDLDX shall no exception and
 *          shall not affect the values of MSRs/Xsave/General purpose registers.
 */
static void mpx_rqmid_23623_environment_hiding_instruction_BNDLDX_002(void)
{
	int a = 0xffff;
	bool result = false;

	enable_xsave();
	result = CHECK_INSTRUCTION_REGS(asm volatile("bndldx %0, %%bnd0\n\t" : : "m"(a)));
	report("%s", result, __FUNCTION__);
}
/**
 * @brief case name:MPX environment hiding_instruction_BNDSTX_002
 *
 * Summary: When enable XSAVE feature set,execute BNDSTX shall no exception and
 *          shall not affect the values of MSRs/Xsave/General purposeregisters.
 */
static void mpx_rqmid_23624_environment_hiding_instruction_BNDSTX_002(void)
{
	int a = 0;
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("bndstx %%bnd0, %0" : : "m"(a)));
	report("%s", ret, __FUNCTION__);
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
	/*cailing case*/
	printf("\t Case ID:%d case name:%s\n\r", 23096,
		   "environment_hiding_instruction_BNDMK_001");
	printf("\t Case ID:%d case name:%s\n\r", 23099,
		   "environment_hiding_instruction_BNDCL_001");
	printf("\t Case ID:%d case name:%s\n\r", 23100,
		   "environment_hiding_instruction_BNDCU_001");
	printf("\t Case ID:%d case name:%s\n\r", 23101,
		   "environment_hiding_instruction_BNDCN_001");
	printf("\t Case ID:%d case name:%s\n\r", 23102,
		   "environment_hiding_instruction_BNDMOV_001");
	printf("\t Case ID:%d case name:%s\n\r", 23103,
		   "environment_hiding_instruction_BNDLDX_001");
	printf("\t Case ID:%d case name:%s\n\r", 23104,
		   "environment_hiding_instruction_BNDSTX_001");
	printf("\t Case ID:%d case name:%s\n\r", 23105,
		   "environment_hiding_CPUID_MPX_001");
	printf("\t Case ID:%d case name:%s\n\r", 23106,
		   "environment_hiding_CPUID_MAWAU_001");
	printf("\t Case ID:%d case name:%s\n\r", 23107,
		   "environment_hiding_register_IA32_BNDCFGS_001");
	printf("\t Case ID:%d case name:%s\n\r", 23625,
		   "environment_hiding_xcr0_bit3_bit4_001");
	printf("\t Case ID:%d case name:%s\n\r", 23824,
		   "environment_hiding_register_IA32_BNDCFGS_002");
	printf("\t Case ID:%d case name:%s\n\r", 23898,
		   "environment_hiding_exception_BNDMOV_001");
	printf("\t Case ID:%d case name:%s\n\r", 24588,
		   "environment_hiding_exception_BNDMOV_002");
	printf("\t Case ID:%d case name:%s\n\r", 25250,
		   "environment_hiding_exception_BNDMOV_003");
	printf("\t Case ID:%d case name:%s\n\r", 25254,
		   "environment_hiding_exception_BNDCN_001");
	printf("\t Case ID:%d case name:%s\n\r", 39257,
		   "mpx_rqmid_39257_environment_hiding_xcr0_bit3_bit4_003");
#elif __i386__
	printf("\t Case ID:%d case name:%s\n\r", 23894,
		   "mpx_rqmid_23894_lock_prefix_test");
	printf("\t Case ID:%d case name:%s\n\r", 23899,
		   "environment_hiding_exception_BNDCU_001");
	printf("\t Case ID:%d case name:%s\n\r", 23624,
		   "environment_hiding_instruction_BNDSTX_002");
	printf("\t Case ID:%d case name:%s\n\r", 25252,
		   "environment_hiding_exception_BNDSTX_001");
	printf("\t Case ID:%d case name:%s\n\r", 23993,
		   "environment_hiding_exception_BNDCL_001");
#endif
}

static void test_mpx(void)
{
#ifdef __x86_64__
	mpx_rqmid_23119_env_hiding_register_bndregs_001();
	mpx_rqmid_23120_env_hiding_register_bndcsr_001();
	mpx_rqmid_25251_environment_hiding_exception_BNDLDX_001();
	mpx_rqmid_23096_environment_hiding_instruction_BNDMK_001();
	mpx_rqmid_23099_environment_hiding_instruction_BNDCL_001();
	mpx_rqmid_23100_environment_hiding_instruction_BNDCU_001();
	mpx_rqmid_23101_environment_hiding_instruction_BNDCN_001();
	mpx_rqmid_23102_environment_hiding_instruction_BNDMOV_001();
	mpx_rqmid_23103_environment_hiding_instruction_BNDLDX_001();
	mpx_rqmid_23104_environment_hiding_instruction_BNDSTX_001();
	mpx_rqmid_23105_environment_hiding_CPUID_MPX_001();
	mpx_rqmid_23106_environment_hiding_CPUID_MAWAU_001();
	mpx_rqmid_23107_environment_hiding_register_IA32_BNDCFGS_001();
	mpx_rqmid_23625_environment_hiding_xcr0_bit3_bit4_001();
	mpx_rqmid_23626_environment_hiding_xcr0_bit3_bit4_002();
	mpx_rqmid_39257_environment_hiding_xcr0_bit3_bit4_003();
	mpx_rqmid_23824_environment_hiding_register_IA32_BNDCFGS_002();
	mpx_rqmid_23898_environment_hiding_exception_BNDMOV_001();
	mpx_rqmid_24588_environment_hiding_exception_BNDMOV_002();
	mpx_rqmid_25250_environment_hiding_exception_BNDMOV_003();
	mpx_rqmid_25252_environment_hiding_exception_BNDSTX_001();
	mpx_rqmid_25254_environment_hiding_exception_BNDCN_001();
#elif __i386__
	mpx_rqmid_23894_lock_prefix_test();
	mpx_rqmid_23899_environment_hiding_exception_BNDCU_001();
	mpx_rqmid_23623_environment_hiding_instruction_BNDLDX_002();
	mpx_rqmid_23624_environment_hiding_instruction_BNDSTX_002();
	mpx_rqmid_23993_environment_hiding_exception_BNDCL_001();
#endif
}

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();

	print_case_list();

	test_mpx();
	return report_summary();
}
