#include "libcflat.h"
#include "processor.h"
#include "desc.h"

#define MSR_IA32_L2_MASK_0 0x00000D10U
#define MSR_IA32_L3_MASK_0 0x00000C90U

#define MSR_IA32_L3_QOS_CFG 0x00000C81U
#define MSR_IA32_L2_QOS_CFG 0x00000C82U

#define MSR_IA32_QM_EVTSEL 0x00000C8DU
#define MSR_IA32_QM_CTR 0x00000C8EU
#define MSR_IA32_PQR_ASSOC 0x00000C8FU

static unsigned rdmsr_checking(u32 index)
{
	asm volatile(ASM_TRY("1f")
				 "rdmsr\n\t" //rdmsr, 0x0f, 0x32
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
				  "wrmsr\n\t" //wrmsr: 0x0f, 0x30
				  "1:"
				  :
				  : "a"(a), "d"(d), "c"(index)
				  : "memory");

	return exception_vector();
}
static inline bool check_exception(unsigned vec, unsigned err_code)
{
	return (exception_vector() == vec) && (exception_error_code() == err_code);
}
/*
 * @brief case name: CPUID.(EAX=07H,ECX=0H)_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=07H,ECX=0H),
 *  ACRN Hypervisor shall guarantee the guest EBX[12] shall be 0
 *
 */
static void rdt_rqmid_28391_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).b & (1 << 12)) == 0, __FUNCTION__);
}

/*
 * @brief case name: IA32_L2_MASK_X_001
 *
 * Summary: When vCPU attempts to read guest IA32_L2_MASK_X,0<= X <=127, ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28394_IA32_l2_mask_x_001()
{
	bool result = true;

	for (int i = 0; i < 128; i++) {
		rdmsr_checking(MSR_IA32_L2_MASK_0 + i);
		if (!check_exception(GP_VECTOR, 0)) {
			printf("%d", i);
			result = false;
			break;
		}
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L2_MASK_X_002
 *
 * Summary: When vCPU attempts to write guest IA32_L2_MASK_X,0<= X <=127,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28402_IA32_l2_mask_x_002()
{
	bool result = true;

	for (int i = 0; i < 128; i++) {
		wrmsr_checking(MSR_IA32_L2_MASK_0 + i, 0);
		if (!check_exception(GP_VECTOR, 0)) {
			printf("%d", i);
			result = false;
			break;
		}
	}

	report("%s", result, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("\t\t RDT feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28391u, "RDT cpuid eax07 ecx0 001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28394u, "RDT IA32 l2 mask x 001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28402u, "RDT IA32 l2 mask x 002");
	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();

	print_case_list();
	rdt_rqmid_28391_cpuid_eax07_ecx0_001();
	rdt_rqmid_28394_IA32_l2_mask_x_001();
	rdt_rqmid_28402_IA32_l2_mask_x_002();

	return report_summary();
}
