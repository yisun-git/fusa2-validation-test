#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "smp.h"

#define MSR_IA32_DEBUGCTL           0x000001D9U

#define MSR_LASTBRANCH_0_TO_IP      0x000006C0U
#define MSR_LASTBRANCH_31_TO_IP     0x000006DFU
#define MSR_LASTBRANCH_31_FROM_IP   0x0000069FU
#define MSR_LASTBRANCH_TOS          0x000001DAU

#define MSR_LBR_SELECT              0x000001C8U
#define MSR_LER_FROM_LIP            0x000001DDU
#define MSR_LER_TO_LIP              0x000001DEU


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

static unsigned wrmsr_checking(u32 index, u64 val)
{
	u32 a = val, d = val >> 32;

	asm volatile (ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		:
		: "a"(a), "d"(d), "c"(index)
		: "memory");

	return exception_vector();
}

/*
 * @brief case name: branch IA32 debugctl 001
 *
 * Summary: When a vCPU attempts to read IA32_DEBUGCTL, ACRN hypervisor shall
 *      guarantee that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28251_IA32_debugctl_001()
{
	unsigned vector;
	unsigned err_code;

	vector = rdmsr_checking(MSR_IA32_DEBUGCTL);
	err_code = exception_error_code();

	report("%s", (vector == GP_VECTOR) && (err_code == 0), __FUNCTION__);
}

/*
 * @brief case name: branch IA32 debugctl 002
 *
 * Summary: When a vCPU attempts to write IA32_DEBUGCTL, ACRN hypervisor shall
 *      guarantee that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28282_IA32_debugctl_002()
{
	unsigned vector;
	unsigned err_code;

	vector = wrmsr_checking(MSR_IA32_DEBUGCTL, 0);
	err_code = exception_error_code();

	report("%s", (vector == GP_VECTOR) && (err_code == 0), __FUNCTION__);
}
/*
 * @brief case name: branch CPUID01H EDX21 001
 *
 * Summary: When a vCPU attempts to read CPUID.01H,
 *      ACRN Hypervisor shall guarantee guest EDX[21] shall be 0.
 *
 */
static void branch_rqmid_28338_CPUID01H_EDX21_001()
{
	report("%s", (cpuid(1).d & (1 << 21)) == 0, __FUNCTION__);
}

/*
 * @brief case name: branch IA32 debugctl 002
 *
 * Summary: ACRN hypervisor should set  guest IA32_MISC_ENABLE[12:11]  to 0x3H.
 *
 */
static void branch_rqmid_28339_IA32_misc_enable_following_init_001()
{
	/*@cstart64.S, AP will save IA32_MISC_ENABLE lower 32bit value to 0x7000*/
	u32 ia32_misc_enable = *(u32 *)0x7000;
	report("%s", ((ia32_misc_enable >> 11) & 0x3) == 0x3, __FUNCTION__);

}
static void print_case_list(void)
{
	printf("\t\t branch profile feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28251u, "branch profile IA32 debugctl 001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28282u, "branch profile IA32 debugctl 002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28338u, "branch profile CPUID01H EDX21 001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28339u, "branch profile IA32 misc enable following init 001");
	printf("\t\t \n\r \n\r");
}
int main(void)
{
	setup_idt();

	print_case_list();
	branch_rqmid_28251_IA32_debugctl_001();
	branch_rqmid_28282_IA32_debugctl_002();
	branch_rqmid_28338_CPUID01H_EDX21_001();
	branch_rqmid_28339_IA32_misc_enable_following_init_001();

	return report_summary();
}

