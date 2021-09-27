#include "alloc.h"
#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "regdump.h"
#include "misc.h"
#include "xsave.h"

#define MSR_IA32_TSX_FORCE_ABORT  0x0000010FU
#define FORCE_ABORT_RTM           (1<<0)

static unsigned xbegin_checking(void)
{
	unsigned int result = 0xff;
	unsigned int temp = 0;

	asm volatile(
		"movl $0xFF, %%eax\n"
		"xbegin  exit1\n"
		"movq $0x5555, %1\n"
		"xend \n"
		"exit1:\n"
		"movl %%eax, %0\n"
		: "=a"(result), "=m"(temp)
		:
		:);

	return result;
}

#ifdef IN_NATIVE

/*
 * @brief case name: RTM available on the physical platform_001
 *
 * Summary: RTM shall be available on the physical platform..
 *
 */
static void tsx_rqmid_37763_cpuid_eax07_ebx0_001()
{
	report("%s", (((cpuid(0x7).b) >> 11) & 1) == 1, __FUNCTION__);
}

/*
 * @brief case name: RTM force-abort mode shall be available on the physical platform_001
 *
 * Summary:  RTM force-abort mode shall be available on the physical platform.
 *
 */
static void tsx_rqmid_37798_cpuid_eax07_edx0_001()
{
	unsigned int result = 0xFF;

	if ((((cpuid(0x7).d) >> 13) & 1) == 1)
	{
		wrmsr(MSR_IA32_TSX_FORCE_ABORT, rdmsr(MSR_IA32_TSX_FORCE_ABORT) | FORCE_ABORT_RTM);
		result = xbegin_checking();
		report("%s", result == 0, __FUNCTION__);
	}
	else
	{
		report("%s", false, __FUNCTION__);
	}
}

#else
static unsigned rdmsr_checking(u32 index)
{
	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		:
		: "c"(index)
		: "memory");

	return exception_vector();
}

static unsigned xtest_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"xtest\n\t"
		"1:"
		:
		:
		: );

	return exception_vector();
}

static unsigned xend_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"xend\n\t"
		"1:"
		:
		:
		: );

	return exception_vector();
}

static inline bool check_exception(unsigned vec, unsigned err_code)
{
	return (exception_vector() == vec) && (exception_error_code() == err_code);
}


/*
 * @brief case name: Hide HLE from VMs_001
 *
 * Summary: Under 64bit mode, A processor hides HLE execution if CPUID.07H.EBX.HLE [bit 4] equals 0.
 *
 */
static void tsx_rqmid_24636_cpuid_eax07_ecx0_001()
{
	report("%s", (((cpuid(0x7).b) >> 4) & 1) == 0, __FUNCTION__);
}

/*
 * @brief case name: Read CPUID.(EAX=7H, ECX=0H).EBX [bit 11]_001
 *
 * Summary: Under 64bit mode, execute CPUID.(EAX=7H,ECX=0H), the value of EBX [bit 11] shall be 1.
 *          This bit indicates the presence of RTM instructions.
 *
 */
static void tsx_rqmid_37743_cpuid_eax07_ecx0_001()
{
	report("%s", (((cpuid(0x7).b) >> 11) & 1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Read CPUID.(EAX=7H, ECX=0H).EDX [bit 13]_001
 *
 * Summary: This bit indicates the presence of MSR TSX_FORCE_ABORT and shall be 0 as the MSR is
 * hidden from the VMs(24115).
 *
 */
static void tsx_rqmid_37745_cpuid_eax07_ecx0_001()
{
	report("%s", (((cpuid(0x7).d) >> 13) & 1) == 0, __FUNCTION__);
}

/*
 * @brief case name: Trigger #GP(0) upon accessing TSX_FORCE_ABORT_001
 *
 * Summary: When vCPU attempts to read guest IA32_TSX_FORCE_ABORT,ACRN hypervisor shall
 * guarantee that the vCPU will receive #GP(0).
 *
 */

static void tsx_rqmid_37750_IA32_TSX_FORCE_ABORT_001()
{
	rdmsr_checking(MSR_IA32_TSX_FORCE_ABORT);
	report("%s", check_exception(GP_VECTOR, 0), __FUNCTION__);
}


/*
 * @brief case name: Expose XTEST to VMs_001
 *
 * Summary: ACRN hypervisor shall expose transactional execution
 * status query instruction to any VM.
 *
 */

static void tsx_rqmid_37759_instruction_xtest_check_001()
{
	bool result = true;

	xtest_checking();
	if (check_exception(UD_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: Trigger #GP(0) when a vCPU executes XEND_001
 *
 * Summary: When vCPU attempts to executes instruction of XEND ,ACRN hypervisor shall
 * guarantee that the vCPU will receive #GP(0).
 *
 */

static void tsx_rqmid_37761_instruction_xend_check_001()
{
	xend_checking();
	report("%s", check_exception(GP_VECTOR, 0), __FUNCTION__);
}

/*
 * @brief case name: Trigger RTM abort when a vCPU executes XBEGIN_001
 *
 * Summary: When a vCPU attempts to execute XBEGIN, ACRN hypervisor shall
 * guarantee that the vCPU causes an RTM abort.
 *
 */

static void tsx_rqmid_37762_instruction_xbegin_check_001()
{
	unsigned int value = 0;

	value = xbegin_checking();
	report("%s", (value & 0x1) == 0, __FUNCTION__);
}

/*
 * @brief case name: No operation when a vCPU executes XABORT_001
 *
 * Summary: No operation when a vCPU executes XABORT.
 *
 */

static void tsx_rqmid_37807_instruction_xabort_check_001()
{
	bool ret = false;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("xabort $255\n\t" : : :));
	report("%s", ret, __FUNCTION__);
}


#endif

static void print_case_list(void)
{
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 37763u, "RTM available on the physical platform_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37798u, "RTM force-abort mode shall be available on the "
	"physical platform_001");
#else
	printf("\t\t TSX feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 24636u, "Hide HLE from VMs_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37743u, "Read CPUID.(EAX=7H, ECX=0H).EBX [bit 11]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37745u, "Read CPUID.(EAX=7H, ECX=0H).EBX [bit 13]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37750u, "Trigger #GP(0) upon accessing TSX_FORCE_ABORT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37759u, "Expose XTEST to VMs_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37761u, "Trigger #GP(0) when a vCPU executes XEND_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37762u, "Trigger RTM abort when a vCPU executes XBEGIN_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37807u, "No operation when a vCPU executes XABORT_001");

	printf("\t\t \n\r \n\r");
#endif
}


int main(void)
{
	setup_idt();
	print_case_list();
#ifdef IN_NATIVE
	tsx_rqmid_37763_cpuid_eax07_ebx0_001();
	tsx_rqmid_37798_cpuid_eax07_edx0_001();
#else
	tsx_rqmid_24636_cpuid_eax07_ecx0_001();
	tsx_rqmid_37743_cpuid_eax07_ecx0_001();
	tsx_rqmid_37745_cpuid_eax07_ecx0_001();
	tsx_rqmid_37750_IA32_TSX_FORCE_ABORT_001();
	tsx_rqmid_37759_instruction_xtest_check_001();
	tsx_rqmid_37761_instruction_xend_check_001();
	tsx_rqmid_37762_instruction_xbegin_check_001();
	tsx_rqmid_37807_instruction_xabort_check_001();
#endif
	return report_summary();
}
