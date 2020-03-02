#include "libcflat.h"
#include "desc.h"
#include "processor.h"

#include "pmu_fu.h"


/**
 * @brief case name:When a vCPU attempts to read CPUID.0AH_001
 *
 * Summary: When a vCPU attempts to read CPUID.0AH, ACRN hypervisor shall
 *          write 0H to guest EAX, guest EBX, guest ECX, guest EDX
 */
static void pmu_rqmid_27001_cpuid_a(void)
{
    u32 res = 0;

    res |= cpuid(0xa).a;
    res |= cpuid(0xa).b;
    res |= cpuid(0xa).c;
    res |= cpuid(0xa).d;
    report("%s", (res == 0), __FUNCTION__);
}


/**
 * @brief case name:RDPMC_001
 *
 * Summary: Test RDPMC instruction execution should cause #UD when PMU is hidden.
 */
static void pmu_rqmid_27844_rdpmc(void)
{
    asm volatile(ASM_TRY("1f")
					"rdpmc \n\t"
					"1:":::);

    report("%s", exception_vector() == UD_VECTOR, __FUNCTION__);

}

/**
 * @brief case name:IA32_FIXED_CTR_CTRL_001
 *
 * Summary: When vCPU attempts to read guest IA32_FIXED_CTR_CTRL,
 *          ACRN hypervisor shall guarantee that the vCPU will receive #GP(0)
 */
static void pmu_rqmid_27141_rd_ia32_fixed_ctr_ctrl(void)
{
    u32 a, d;
    asm volatile (ASM_TRY("1f")
					"rdmsr\n\t"
					"1:"
					: "=a"(a), "=d"(d) : "c"(MSR_CORE_PERF_FIXED_CTR_CTRL) : "memory");

    report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:IA32_FIXED_CTR_CTRL_002
 *
 * Summary: When vCPU attempts to write guest IA32_FIXED_CTR_CTRL,
 *          ACRN hypervisor shall guarantee that the vCPU will receive #GP(0)
 */
static void pmu_rqmid_27142_wt_ia32_fixed_ctr_ctrl(void)
{
	u64 val = 1;
	u32 a = val;
	u32 d = (val >> 32);

    asm volatile (ASM_TRY("1f")
					"wrmsr\n\t"
					"1:"
					: : "a"(a), "d"(d), "c"(MSR_CORE_PERF_FIXED_CTR_CTRL) : "memory");

    report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/**
 * @brief case name:cr4_bit8_startup_001
 *
 * Summary: ACRN hypervisor shall set the initial cr4.VMXE[8] of guest VM to 0 following start-up
 *
 */

static void pmu_rqmid_29693_cr4_bit8_startup_001()
{
    u32 cr4 = *(u32 *)0x8000;

    report("%s", ((cr4 & (1 << 8)) == 0), __FUNCTION__);
}

static void print_case_list(void)
{
    printf("PMU feature case list:\n\r");
    printf("\t Case ID:%d case name:%s\n\r", 27001, "When a vCPU attempts to read CPUID.0AH_001");
    printf("\t Case ID:%d case name:%s\n\r", 27141, "IA32_FIXED_CTR_CTRL_001");
    printf("\t Case ID:%d case name:%s\n\r", 27142, "IA32_FIXED_CTR_CTRL_002");
    printf("\t Case ID:%d case name:%s\n\r", 27844, "RDPMC_001");
    printf("\t Case ID:%d case name:%s\n\r", 29693, "cr4[8] start-up_001");
}

int main(void)
{
    setup_vm();
    setup_idt();

    print_case_list();

    pmu_rqmid_27001_cpuid_a();
    pmu_rqmid_27141_rd_ia32_fixed_ctr_ctrl();
    pmu_rqmid_27142_wt_ia32_fixed_ctr_ctrl();
    pmu_rqmid_27844_rdpmc();
    pmu_rqmid_29693_cr4_bit8_startup_001();

    return report_summary();
}
