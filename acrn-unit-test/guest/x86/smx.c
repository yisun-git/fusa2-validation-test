#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "smx.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"
#include "register_op.h"

#ifdef __x86_64__
#define uint64_t unsigned long
#else
#define uint64_t unsigned long long
#endif
struct emulate_register {
	u32 a, b, c, d;
};


u32 get_startup_ia32_feature()
{
	return *((volatile u32 *)STARTUP_IA32_FEATURE_CONTROL);
}

u32 get_init_ia32_feature()
{
	return *((volatile u32 *)INIT_IA32_FEATURE_CONTROL);
}

u32 get_startup_cr4()
{
	return *((volatile u32 *)STARTUP_CR4_ADDR);
}

u32 get_init_cr4()
{
	return *((volatile u32 *)INIT_CR4_ADDR);
}


static int getsec_checking()
{
	struct emulate_register r;
	r.a = 1;
	asm volatile(ASM_TRY("1f")
		"getsec \n\t"
		"1:" : "=c"(r.c) : "a"(r.a));
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
		if (write_cr4_checking(cr4 | X86_CR4_SMX) == GP_VECTOR) {
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
		wrmsr_checking(IA32_FEATURE_CONTROL, 0x2) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_004
 *
 * Summary: execute wirte msr IA32_FEATURE_CONTROL [bit 14:8] from 0 to 1,
 *	    it shall get #GP exception.
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
 *	    it shall get #GP exception.
 */
static void smx_rqmid_28828_write_msr_ia32_feature_control()
{
	report("\t\t %s",
		wrmsr_checking(IA32_FEATURE_CONTROL,
			SMX_SENTER_GLOBAL_ENABLE) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: SMX unavailability_006
 *
 * Summary: Dump register IA32_FEATURE_CONTROL,
 * the bit 0 of the IA32_FEATURE_CONTROL MSR  shall be 1.
 * and the bit 15~8 of the IA32_FEATURE_CONTROL MSR  shall be 0.
 */
static void smx_rqmid_37359_read_msr_ia32_feature_control()
{
	u64 value = 0;

	value = rdmsr(MSR_IA32_FEATURE_CONTROL);

	report("\t\t %s", (((value & 0x1) == 1) && ((value & 0xff02) == 0)), __FUNCTION__);
}

/**
 * @brief case name: IA32_FEATURE_CONTROL[bit 15:8] for virtual BP_001
 *
 * Summary: Get IA32_FEATURE_CONTROL[bit 15:8] at BP start-up,
 * the value shall be 0.
 */
static void smx_rqmid_37046_IA32_FEATURE_CONTROL_BP_001()
{
	u32 value = get_startup_ia32_feature();
	value &= 0xff00;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Initialize CR4.SMXE for virtual BP_001
 *
 * Summary: Get CR4.SMXE(bit14) at BP start-up, the value shall be 0.
 */
static void smx_rqmid_37055_CR4_SMXE_BP_001()
{
	u32 value = get_startup_cr4();
	value &= 0x4000;

	report("\t\t %s", value == 0, __FUNCTION__);
}
#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name: IA32_FEATURE_CONTROL[bit 15:8] for virtual AP_001
 *
 * Summary: Get IA32_FEATURE_CONTROL[bit 15:8] at AP INIT,
 * the value shall be 0.
 */
static void smx_rqmid_37054_IA32_FEATURE_CONTROL_AP_001()
{
	u32 value = get_init_ia32_feature();
	value &= 0xff00;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Initialize CR4.SMXE for virtual AP_001
 *
 * Summary: Get CR4.SMXE(bit14) at AP INIT, the value shall be 0.
 */
static void smx_rqmid_37063_CR4_SMXE_AP_001()
{
	u32 value = get_init_cr4();
	value &= 0x4000;

	report("\t\t %s", value == 0, __FUNCTION__);
}
#endif

static void test_smx()
{
	smx_rqmid_28664_check_getsec_supported();
	smx_rqmid_28662_set_cr4_smxe();
	smx_rqmid_28665_write_msr_ia32_feature_control();
	smx_rqmid_28826_write_msr_ia32_feature_control();
	smx_rqmid_28828_write_msr_ia32_feature_control();
	smx_rqmid_37359_read_msr_ia32_feature_control();
	smx_rqmid_37046_IA32_FEATURE_CONTROL_BP_001();
	smx_rqmid_37055_CR4_SMXE_BP_001();
#ifdef IN_NON_SAFETY_VM
	smx_rqmid_37054_IA32_FEATURE_CONTROL_AP_001();
	smx_rqmid_37063_CR4_SMXE_AP_001();
#endif
}

static void print_case_list()
{
	printf("SMX feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28664u, "SMX unavailability_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28662u, "SMX unavailability_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28665u, "SMX unavailability_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 28826u, "SMX unavailability_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28828u, "SMX unavailability_005");
	printf("\t\t Case ID:%d case name:%s\n\r", 37359u, "SMX unavailability_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 37046u, "IA32_FEATURE_CONTROL[bit 15:8] for virtual BP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37055u, "CR4.SMXE for virtual BP_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 37054u, "IA32_FEATURE_CONTROL[bit 15:8] for virtual AP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37063u, "CR4.SMXE for virtual AP_001");
#endif
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

