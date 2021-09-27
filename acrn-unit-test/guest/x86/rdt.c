#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "register_op.h"

#define MSR_IA32_L2_MASK_0 0x00000D10U
#define MSR_IA32_L3_MASK_0 0x00000C90U

#define MSR_IA32_L3_QOS_CFG 0x00000C81U
#define MSR_IA32_L2_QOS_CFG 0x00000C82U

#define MSR_IA32_QM_EVTSEL 0x00000C8DU
#define MSR_IA32_QM_CTR 0x00000C8EU
#define MSR_IA32_PQR_ASSOC 0x00000C8FU

static inline bool check_exception(unsigned vec, unsigned err_code)
{
	return (exception_vector() == vec) && (exception_error_code() == err_code);
}

/*
 * @brief case name: CPUID.(EAX=07H,ECX=0H):EBX[15]_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=07H,ECX=0H),
 *  ACRN Hypervisor shall guarantee the guest EBX[15] shall be 0
 *
 */
static void rdt_rqmid_28380_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).b & (1 << 15)) == 0, __FUNCTION__);
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
 * @brief case name: CPUID.(EAX=FH,ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=FH,ECX=0H),
 * ACRN Hypervisor shall gurantee the guest EAX, EBX, ECX, EDX shall be 0.
 *
 */
static void rdt_rqmid_28387_cpuid_eax0F_ecx0_001()
{
	bool is_pass = false;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0F, 0x00);
	is_pass = (cpuid0fh.a == 0) \
				&& (cpuid0fh.b == 0) \
				&& (cpuid0fh.c == 0) \
				&& (cpuid0fh.d == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CPUID.(EAX=FH,ECX=1H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=FH,ECX=1H),
 *  ACRN Hypervisor shall gurantee the guest EAX, EBX, ECX, EDX shall be 0.
 *
 */
static void rdt_rqmid_28389_cpuid_eax0F_ecx1_001()
{
	bool is_pass = false;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0F, 0x01);
	is_pass = (cpuid0fh.a == 0) \
				&& (cpuid0fh.b == 0) \
				&& (cpuid0fh.c == 0) \
				&& (cpuid0fh.d == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CPUID.(EAX=10H,ECX=0H)_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=10H,ECX=0H),
 *  ACRN Hypervisor shall gurantee EAX,EBX,ECX,EDX shall be 0
 *
 */
static void rdt_rqmid_28381_cpuid_eax10_ecx0_001()
{
	bool is_pass = false;
	struct cpuid cpuid10h = {0};
	cpuid10h = cpuid_indexed(0x10, 0x00);
	is_pass = (cpuid10h.a == 0) \
				&& (cpuid10h.b == 0) \
				&& (cpuid10h.c == 0) \
				&& (cpuid10h.d == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CPUID.(EAX=10H,ECX=01H)_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=10H,ECX=1H),
 *  ACRN Hypervisor shall gurantee EAX,EBX,ECX,EDX shall be 0
 *
 */
static void rdt_rqmid_28382_cpuid_eax10_ecx1_001()
{
	bool is_pass = false;
	struct cpuid cpuid10h = {0};
	cpuid10h = cpuid_indexed(0x10, 0x01);
	is_pass = (cpuid10h.a == 0) \
				&& (cpuid10h.b == 0) \
				&& (cpuid10h.c == 0) \
				&& (cpuid10h.d == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CPUID.(EAX=10H,ECX=02H)_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=10H,ECX=2H),
 *  ACRN Hypervisor shall gurantee EAX,EBX,ECX,EDX shall be 0
 *
 */
static void rdt_rqmid_28383_cpuid_eax10_ecx2_001()
{
	bool is_pass = false;
	struct cpuid cpuid10h = {0};
	cpuid10h = cpuid_indexed(0x10, 0x02);
	is_pass = (cpuid10h.a == 0) \
				&& (cpuid10h.b == 0) \
				&& (cpuid10h.c == 0) \
				&& (cpuid10h.d == 0);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: CPUID.(EAX=10H,ECX=3H)_001
 *
 * Summary: When any vCPU attempts to read CPUID.(EAX=10H,ECX=3H),
 *  ACRN Hypervisor shall gurantee EAX,EBX,ECX,EDX shall be 0
 *
 */
static void rdt_rqmid_28384_cpuid_eax10_ecx3_001()
{
	bool is_pass = false;
	struct cpuid cpuid10h = {0};
	cpuid10h = cpuid_indexed(0x10, 0x03);
	is_pass = (cpuid10h.a == 0) \
				&& (cpuid10h.b == 0) \
				&& (cpuid10h.c == 0) \
				&& (cpuid10h.d == 0);
	report("%s", is_pass, __FUNCTION__);
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
		rdmsr_checking(MSR_IA32_L2_MASK_0 + i, NULL);
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

/*
 * @brief case name: IA32_L3_MASK_X_001
 *
 * Summary: When vCPU attempts to read guest IA32_L3_MASK_X,0<= X <=127, ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28396_IA32_l3_mask_x_001()
{
	bool result = true;

	for (int i = 0; i < 128; i++) {
		rdmsr_checking(MSR_IA32_L3_MASK_0 + i, NULL);
		if (!check_exception(GP_VECTOR, 0)) {
			printf("%d", i);
			result = false;
			break;
		}
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L3_MASK_X_002
 *
 * Summary: When vCPU attempts to write guest IA32_L3_MASK_X,0<= X <=127,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28403_IA32_l3_mask_x_002()
{
	bool result = true;

	for (int i = 0; i < 128; i++) {
		wrmsr_checking(MSR_IA32_L3_MASK_0 + i, 0);
		if (!check_exception(GP_VECTOR, 0)) {
			printf("%d", i);
			result = false;
			break;
		}
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_QM_EVTSEL_001
 *
 * Summary: When vCPU attempts to read guest IA32_QM_EVTSEL,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28400_IA32_QM_EVTSEL_001()
{
	bool result = true;

	rdmsr_checking(MSR_IA32_QM_EVTSEL, NULL);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_QM_EVTSEL_002
 *
 * Summary: When vCPU attempts to write guest IA32_QM_EVTSEL,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28405_IA32_QM_EVTSEL_002()
{
	bool result = true;

	wrmsr_checking(MSR_IA32_QM_EVTSEL, 0);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L2_QOS_CFG_001
 *
 * Summary: When vCPU attempts to read guest  IA32_L2_QOS_CFG,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28398_IA32_L2_QOS_CFG_001()
{
	bool result = true;

	rdmsr_checking(MSR_IA32_L2_QOS_CFG, NULL);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L2_QOS_CFG_002
 *
 * Summary: When vCPU attempts to write guest IA32_L2_QOS_CFG,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_27207_IA32_L2_QOS_CFG_002()
{
	bool result = true;

	wrmsr_checking(MSR_IA32_L2_QOS_CFG, 0);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L3_QOS_CFG_001
 *
 * Summary: When vCPU attempts to read guest IA32_L3_QOS_CFG,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28399_IA32_L3_QOS_CFG_001()
{
	bool result = true;

	rdmsr_checking(MSR_IA32_L3_QOS_CFG, NULL);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_L3_QOS_CFG_002
 *
 * Summary: When vCPU attempts to write guest IA32_L3_QOS_CFG,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28404_IA32_L3_QOS_CFG_002()
{
	bool result = true;

	wrmsr_checking(MSR_IA32_L3_QOS_CFG, 0);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_QM_CTR_001
 *
 * Summary: When vCPU attempts to read guest  IA32_QM_CTR,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_27188_IA32_QM_CTR_001()
{
	bool result = true;

	rdmsr_checking(MSR_IA32_QM_CTR, NULL);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_QM_CTR_002
 *
 * Summary: When vCPU attempts to write guest IA32_QM_CTR,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28407_IA32_QM_CTR_002()
{
	bool result = true;

	wrmsr_checking(MSR_IA32_QM_CTR, 0);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_PQR_ASSOC_001
 *
 * Summary: When vCPU attempts to read guest IA32_PQR_ASSOC,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_41192_IA32_PQR_ASSOC_001()
{
	bool result = true;

	rdmsr_checking(MSR_IA32_PQR_ASSOC, NULL);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name: IA32_PQR_ASSOC_002
 *
 * Summary: When vCPU attempts to write guest IA32_PQR_ASSOC,ACRN
 *  hypervisor shall guarantee that the vCPU will receive #GP(0)
 *
 */
static void rdt_rqmid_28408_IA32_PQR_ASSOC_002()
{
	bool result = true;

	wrmsr_checking(MSR_IA32_PQR_ASSOC, 0);
	if (!check_exception(GP_VECTOR, 0)) {
		result = false;
	}

	report("%s", result, __FUNCTION__);
}


static void print_case_list(void)
{
	printf("\t\t RDT feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28381u, "CPUID.(EAX=10H,ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28382u, "CPUID.(EAX=10H,ECX=01H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28383u, "CPUID.(EAX=10H,ECX=02H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28384u, "CPUID.(EAX=10H,ECX=3H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28387u, "CPUID.(EAX=FH,ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28389u, "CPUID.(EAX=FH,ECX=1H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28380u, "CPUID.(EAX=07H,ECX=0H):EBX[15]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28391u, "CPUID.(EAX=07H,ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28394u, "IA32_L2_MASK_X_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28402u, "IA32_L2_MASK_X_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28396u, "IA32_L3_MASK_X_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28403u, "IA32_L3_MASK_X_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28400u, "IA32_QM_EVTSEL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28405u, "IA32_QM_EVTSEL_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28398u, "IA32_L2_QOS_CFG_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27207u, "IA32_L2_QOS_CFG_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28399u, "IA32_L3_QOS_CFG_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28404u, "IA32_L3_QOS_CFG_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 27188u, "IA32_QM_CTR_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28407u, "IA32_QM_CTR_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 41192u, "IA32_PQR_ASSOC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28408u, "IA32_PQR_ASSOC_002");
	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();

	print_case_list();
	rdt_rqmid_28391_cpuid_eax07_ecx0_001();
	rdt_rqmid_28394_IA32_l2_mask_x_001();
	rdt_rqmid_28402_IA32_l2_mask_x_002();
	rdt_rqmid_28381_cpuid_eax10_ecx0_001();
	rdt_rqmid_28382_cpuid_eax10_ecx1_001();
	rdt_rqmid_28383_cpuid_eax10_ecx2_001();
	rdt_rqmid_28384_cpuid_eax10_ecx3_001();

	rdt_rqmid_28387_cpuid_eax0F_ecx0_001();
	rdt_rqmid_28389_cpuid_eax0F_ecx1_001();
	rdt_rqmid_28380_cpuid_eax07_ecx0_001();
	rdt_rqmid_28396_IA32_l3_mask_x_001();
	rdt_rqmid_28403_IA32_l3_mask_x_002();

	rdt_rqmid_28400_IA32_QM_EVTSEL_001();
	rdt_rqmid_28405_IA32_QM_EVTSEL_002();

	rdt_rqmid_28398_IA32_L2_QOS_CFG_001();
	rdt_rqmid_27207_IA32_L2_QOS_CFG_002();

	rdt_rqmid_28399_IA32_L3_QOS_CFG_001();
	rdt_rqmid_28404_IA32_L3_QOS_CFG_002();

	rdt_rqmid_27188_IA32_QM_CTR_001();
	rdt_rqmid_28407_IA32_QM_CTR_002();

	rdt_rqmid_41192_IA32_PQR_ASSOC_001();
	rdt_rqmid_28408_IA32_PQR_ASSOC_002();

	return report_summary();
}
