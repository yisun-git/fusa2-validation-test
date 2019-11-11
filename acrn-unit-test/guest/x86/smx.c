#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "smx.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"

static int getsec_checking()
{
	struct emulate_register r;
	r.a = 1;
	asm volatile(ASM_TRY("1f")
			"getsec \n\t"
			"1:": "=c"(r.c): "a"(r.a));
	return exception_vector();
}

/**
 * @brief case name: SMX unavailability_002
 *
 * Summary: Execute GETSEC instruction shall generate #UD
 */
static void smx_rqmid_28664_check_getsec_supported()
{
	ulong cr4 = read_cr4();
	bool flag = false;

	if ((cr4 & X86_CR4_SMX) == 0) {
		if (getsec_checking() == UD_VECTOR) {
			flag = true;
		}
	}
	report("\t\t %s", flag, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_001
 *
 * Summary:Execute CPUID.01H:ECX[bit 6] shall be 0, set CR4.SMXE to be 1 shall generate #GP
 */
static void smx_rqmid_28662_set_cr4_smxe()
{
	ulong cr4 = read_cr4();
	bool flag = false;
	if ((cpuid(1).c & CPUID_1_SMX_SUPPORTED) == 0) {
		if (write_cr4_exception_checking(cr4 | X86_CR4_SMX) == GP_VECTOR) {
			flag = true;
		}
	}
	report("\t\t %s", flag, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_003
 *
 * Summary: Enable VMX in SMX operation shall generate #GP
 */
static void smx_rqmid_28665_write_msr_ia32_feature_control()
{
	report("\t\t %s",
		wrmsr_checking(IA32_FEATURE_CONTROL, 0x3) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_004
 *
 * Summary: execute wirte msr IA32_FEATURE_CONTROL [bit 14:8] from 0 to 1,
 * 	    it shall get #GP exception.
 */
static void smx_rqmid_28826_write_msr_ia32_feature_control()
{
	report("\t\t %s",
		wrmsr_checking(IA32_FEATURE_CONTROL, SMX_SENTER_LOCAL_FUNCTON_ENABLE)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_005
 *
 * Summary: execute wirte msr?IA32_FEATURE_CONTROL [bit 15] from 0 to 1,
 *   	    it shall get #GP exception.
 */
static void smx_rqmid_28828_write_msr_ia32_feature_control()
{
	report("\t\t %s",
		wrmsr_checking(IA32_FEATURE_CONTROL,
		SMX_SENTER_GLOBAL_ENABLE) == GP_VECTOR, __FUNCTION__);
}

static void test_smx()
{
	smx_rqmid_28664_check_getsec_supported();
	smx_rqmid_28662_set_cr4_smxe();
	smx_rqmid_28665_write_msr_ia32_feature_control();
	smx_rqmid_28826_write_msr_ia32_feature_control();
	smx_rqmid_28828_write_msr_ia32_feature_control();
}

static void print_case_list()
{
	printf("SMX feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28664u, "SMX unavailability_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28662u, "SMX unavailability_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28665u, "SMX unavailability_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 28826u, "SMX unavailability_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28828u, "SMX unavailability_005");
	printf("-------------------------\n\r");
}

int main(void)
{
	setup_vm();
	setup_idt();
	print_case_list();
	test_smx();

	return report_summary();
}

