#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "sgx.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"
#include "apic.h"
#include "isr.h"
#include "register_op.h"
#include "delay.h"
#include "fwcfg.h"


void save_unchanged_reg()
{
	asm volatile (
		"mov $0x0000003a, %ecx\n"
		"rdmsr\n"
		"mov %eax, (0x7000)\n"
		"mov %edx, (0x7004)"
	);
}

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_ap_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_sgx_lauch_bit_init_value()
{
	if (cpuid(7).c & (1ul << 30)) {
		wrmsr(IA32_FEATURE_CONTROL, rdmsr(IA32_FEATURE_CONTROL) | SGX_LAUCH_BIT);
	}
}

__unused static void modify_sgx_enable_bit_init_value()
{
	if (cpuid(7).b & (1ul << 2)) {
		wrmsr(IA32_FEATURE_CONTROL, rdmsr(IA32_FEATURE_CONTROL) | SGX_ENABLE_BIT);
	}
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 29562:
		fp = modify_sgx_lauch_bit_init_value;
		ap_init_value_process(fp);
		break;
	case 27404:
		fp = modify_sgx_enable_bit_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

#ifdef IN_NATIVE
/**
 * @brief case name:Physical SGX1 support_001
 *
 * Summary: read the value of CPUID.(EAX=7H, ECX=0H):EBX[bit 2] and
 * CPUID.(EAX=12H, ECX=0H):EAX.SGX1 [bit 0]from guest shall be 0
 *
 */
static void sgx_rqmid_36504_check_sgx_physical_support()
{
	if ((((cpuid(7).b) & CPUID_07_SGX) == 0) && (((cpuid(0x12).a) & 0x1) == 0)) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}
#else
extern void send_sipi();
bool g_is_init_ap = false;

/**
 * @brief case name:Guest CPUID.SGX LC_001
 *
 * Summary: Read CPUID.(EAX=7H, ECX=0H):ECX[bit 30] from guest shall be 0
 */
static void sgx_rqmid_27401_check_sgx_support()
{
	report("\t\t sgx_rqmid_27401_check_sgx_support",
		((cpuid(7).c) & CPUID_07_SGX) == 0);
}

/**
 * @brief case name: Guest CPUID leaf 12H_001
 *
 * Summary: Execute CPUID.12H get EAX,EBX,ECX,EDX shall be 0
 */
static void sgx_rqmid_27400_guest_cpuid_leaf_12h()
{
	bool flag = false;
	if ((cpuid(SGX_CPUID_ID).a == 0) && (cpuid(SGX_CPUID_ID).b == 0)
		&& (cpuid(SGX_CPUID_ID).c == 0) && (cpuid(SGX_CPUID_ID).d == 0)) {
		flag = true;
	}
	report("\t\t sgx_rqmid_27400_guest_cpuid_leaf_12h", flag);
}

/**
 * @brief case name: Guest CPUID SGX_KEYS
 *
 * Summary: Execute CPUID.7H to get EDX[bit 1]
 */
static void sgx_ACRN_T13683_Guest_CPUID_SGX_KEYS_001()
{
	report("\t\t sgx_ACRN_T13683_Guest_CPUID_SGX_KEYS_001",
		((cpuid(7).d) & CPUID_01_SGX_KEYS) == 0);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH1_002
 *
 * Summary: write from guest MSR register msr_sgxownerepoch1 shall generate #GP
 */
static void sgx_rqmid_32532_write_msr_sgxownerepoch1()
{
	u64 msr_sgxownerepoch1 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_SGXOWNEREPOCH1, msr_sgxownerepoch1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH1_001
 *
 * Summary: read from guest MSR register msr_sgxownerepoch1 shall generate #GP
 */
static void sgx_rqmid_32533_read_msr_sgxownerepoch1()
{
	u64 msr_sgxownerepoch1;
	report("\t\t %s",
		rdmsr_checking(MSR_SGXOWNEREPOCH1, &msr_sgxownerepoch1)
		== GP_VECTOR, __FUNCTION__);
}


/**
 * @brief case name: Guest MSR SGXOWNEREPOCH0_002
 *
 * Summary: write from guest MSR register msr_sgxownerepoch0 shall generate #GP
 */
static void sgx_rqmid_32534_write_msr_sgxownerepoch0()
{
	u64 msr_sgxownerepoch0 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(MSR_SGXOWNEREPOCH0, msr_sgxownerepoch0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest MSR SGXOWNEREPOCH0_001
 *
 * Summary: read from guest MSR register msr_sgxownerepoch0 shall generate #GP
 */
static void sgx_rqmid_32535_read_msr_sgxownerepoch0()
{
	u64 msr_sgxownerepoch0;
	report("\t\t %s",
		rdmsr_checking(MSR_SGXOWNEREPOCH0, &msr_sgxownerepoch0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH3_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash3 shall generate #GP
 */
static void sgx_rqmid_32540_write_ia32_sgxlepubkeyhash3()
{
	u64 ia32_sgxlepubkeyhash3 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH3, ia32_sgxlepubkeyhash3)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH3_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash3 shall generate #GP
 */
static void sgx_rqmid_32541_read_ia32_sgxlepubkeyhash3()
{
	u64 ia32_sgxlepubkeyhash3;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH3, &ia32_sgxlepubkeyhash3)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH2_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash2 shall generate #GP
 */
static void sgx_rqmid_32542_write_ia32_sgxlepubkeyhash2()
{
	u64 ia32_sgxlepubkeyhash2 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH2, ia32_sgxlepubkeyhash2)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH2_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash2 shall generate #GP
 */
static void sgx_rqmid_32543_read_ia32_sgxlepubkeyhash2()
{
	u64 ia32_sgxlepubkeyhash2;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH2, &ia32_sgxlepubkeyhash2)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH1_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash1 shall generate #GP
 */
static void sgx_rqmid_32544_write_ia32_sgxlepubkeyhash1()
{
	u64 ia32_sgxlepubkeyhash1 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH1, ia32_sgxlepubkeyhash1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH1_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash1 shall generate #GP
 */
static void sgx_rqmid_32545_read_ia32_sgxlepubkeyhash1()
{
	u64 ia32_sgxlepubkeyhash1;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH1, &ia32_sgxlepubkeyhash1)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH0_002
 *
 * Summary: write from guest MSR register ia32_sgxlepubkeyhash0 shall generate #GP
 */
static void sgx_rqmid_32546_write_ia32_sgxlepubkeyhash0()
{
	u64 ia32_sgxlepubkeyhash0 = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGXLEPUBKEYHASH0, ia32_sgxlepubkeyhash0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGXLEPUBKEYHASH0_001
 *
 * Summary: read from guest MSR register ia32_sgxlepubkeyhash0 shall generate #GP
 */
static void sgx_rqmid_32547_read_ia32_sgxlepubkeyhash0()
{
	u64 ia32_sgxlepubkeyhash0;
	report("\t\t %s",
		rdmsr_checking(IA32_SGXLEPUBKEYHASH0, &ia32_sgxlepubkeyhash0)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGX SVN STATUS_002
 *
 * Summary: write from guest MSR register ia32_sgx_svn_status shall generate #GP
 */
static void sgx_rqmid_32548_write_ia32_sgx_svn_status()
{
	u64 ia32_sgx_svn_status = VALUE_TO_WRITE_MSR;
	report("\t\t %s",
		wrmsr_checking(IA32_SGX_SVN_STATUS, ia32_sgx_svn_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest IA32 SGX SVN STATUS_001
 *
 * Summary: read from guest MSR register ia32_sgx_svn_status shall generate #GP
 */
static void sgx_rqmid_32549_read_ia32_sgx_svn_status()
{
	u64 ia32_sgx_svn_status;
	report("\t\t %s",
		rdmsr_checking(IA32_SGX_SVN_STATUS, &ia32_sgx_svn_status)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guest CPUID.SGX_001
 *
 * Summary: Execute CPUID.(EAX=7H, ECX=0H):EBX[bit 2] shall be 0H.
 */
static void sgx_rqmid_32550_check_supported_sgx()
{
	report("\t\t %s", ((cpuid(7).b) & (1ul << 2)) == 0, __FUNCTION__);
}

static unsigned encls_checking(int value)
{
	asm volatile("mov %0, %%eax\n\t"
		ASM_TRY("1f")
		"encls\n\t"
		"1:"
		:
		: "r"(value)
		: );
	return exception_vector();
}

static unsigned enclu_checking(int value)
{
	asm volatile("mov %0, %%eax\n\t"
		ASM_TRY("1f")
		"enclu\n\t"
		"1:"
		:
		: "r"(value)
		: );
	return exception_vector();
}

static unsigned enclv_checking(int value)
{
	asm volatile("mov %0, %%eax\n\t"
		ASM_TRY("1f")
		".byte 0x0F, 0x01, 0xC0\n\t"
		"1:"
		:
		: "r"(value)
		: );
	return exception_vector();
}


/**
 * @brief case name: Execute ENCLS_eax_00h_001
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38009_encls_eax_00h_001()
{
	if (encls_checking(0x00) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_01h_002
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38010_encls_eax_01h_002()
{
	if (encls_checking(0x01) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_02h_003
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38011_encls_eax_02h_003()
{
	if (encls_checking(0x02) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_03h_004
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38012_encls_eax_03h_004()
{
	if (encls_checking(0x03) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_04h_005
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38013_encls_eax_04h_005()
{
	if (encls_checking(0x04) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_05h_006
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38029_encls_eax_05h_006()
{
	if (encls_checking(0x05) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_06h_007
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38030_encls_eax_06h_007()
{
	if (encls_checking(0x06) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_07h_008
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38031_encls_eax_07h_008()
{
	if (encls_checking(0x07) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_08h_009
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38032_encls_eax_08h_009()
{
	if (encls_checking(0x08) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_09h_010
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38033_encls_eax_09h_010()
{
	if (encls_checking(0x09) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Ah_011
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38034_encls_eax_0Ah_011()
{
	if (encls_checking(0x0A) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Bh_012
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38035_encls_eax_0Bh_012()
{
	if (encls_checking(0x0B) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Ch_013
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38037_encls_eax_0Ch_013()
{
	if (encls_checking(0x0C) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Dh_014
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38039_encls_eax_0Dh_014()
{
	if (encls_checking(0x0D) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Eh_015
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38040_encls_eax_0Eh_015()
{
	if (encls_checking(0x0E) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Fh_014
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38041_encls_eax_0Fh_016()
{
	if (encls_checking(0x0F) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_10h_017
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38042_encls_eax_10h_017()
{
	if (encls_checking(0x10) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_0Bh_018
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38043_encls_eax_11h_018()
{
	if (encls_checking(0x11) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_12h_019
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38044_encls_eax_12h_019()
{
	if (encls_checking(0x12) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLS_eax_13h_019
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38045_encls_eax_13h_020()
{
	if (encls_checking(0x13) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLV_eax_00h_001
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38050_enclv_eax_00h_001()
{
	if (enclv_checking(0x00) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLV_eax_01h_002
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38052_enclv_eax_01h_002()
{
	if (enclv_checking(0x01) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLV_eax_02h_003
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38054_enclv_eax_02h_003()
{
	if (enclv_checking(0x02) == UD_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_00h_001
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38056_enclu_eax_00h_001(const char *msg)
{
	unsigned ret = enclu_checking(0x00);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_01h_002
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38057_enclu_eax_01h_002(const char *msg)
{
	unsigned ret = enclu_checking(0x01);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_02h_003
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38058_enclu_eax_02h_003(const char *msg)
{
	unsigned ret = enclu_checking(0x02);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_03h_004
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38059_enclu_eax_03h_004(const char *msg)
{
	unsigned ret = enclu_checking(0x03);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_04h_005
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38060_enclu_eax_04h_005(const char *msg)
{
	unsigned ret = enclu_checking(0x04);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_05h_005
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38061_enclu_eax_05h_006(const char *msg)
{
	unsigned ret = enclu_checking(0x05);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_06h_007
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38062_enclu_eax_06h_007(const char *msg)
{
	unsigned ret = enclu_checking(0x06);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Execute ENCLU_eax_07h_008
 *
 * Summary: verifies if the SGX instructions has been disabled
 *
 */

static void sgx_rqmid_38063_enclu_eax_07h_008(const char *msg)
{
	unsigned ret = enclu_checking(0x07);
	if (ret == UD_VECTOR || ret == GP_VECTOR) {
		report("\t\t %s", true, __FUNCTION__);
	} else {
		report("\t\t %s", false, __FUNCTION__);
	}
}

#endif
static void print_case_list()
{
	printf("SGX feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 36504u, "Physical SGX1 support_001");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 27401u, "Guest CPUID.SGX LC_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27400u, "Guest CPUID leaf 12H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32532u, "Guest MSR SGXOWNEREPOCH1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32533u, "Guest MSR SGXOWNEREPOCH1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27387u, "Guest MSR SGXOWNEREPOCH0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32535u, "Guest MSR SGXOWNEREPOCH0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32540u, "Guest IA32 SGXLEPUBKEYHASH3_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32541u, "Guest IA32 SGXLEPUBKEYHASH3_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32542u, "Guest IA32 SGXLEPUBKEYHASH2_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32543u, "Guest IA32 SGXLEPUBKEYHASH2_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32544u, "Guest IA32 SGXLEPUBKEYHASH1_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32545u, "Guest IA32 SGXLEPUBKEYHASH1_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32546u, "Guest IA32 SGXLEPUBKEYHASH0_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32547u, "Guest IA32 SGXLEPUBKEYHASH0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32548u, "Guest IA32 SGX SVN STATUS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32549u, "Guest IA32 SGX SVN STATUS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32550u, "Guest CPUID.SGX_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38009u, "Execute ENCLS_eax_00h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38010u, "Execute ENCLS_eax_01h_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38011u, "Execute ENCLS_eax_02h_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 38012u, "Execute ENCLS_eax_03h_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 38013u, "Execute ENCLS_eax_04h_005");
	printf("\t\t Case ID:%d case name:%s\n\r", 38029u, "Execute ENCLS_eax_05h_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 38030u, "Execute ENCLS_eax_06h_007");
	printf("\t\t Case ID:%d case name:%s\n\r", 38031u, "Execute ENCLS_eax_07h_008");
	printf("\t\t Case ID:%d case name:%s\n\r", 38032u, "Execute ENCLS_eax_08h_009");
	printf("\t\t Case ID:%d case name:%s\n\r", 38033u, "Execute ENCLS_eax_09h_010");
	printf("\t\t Case ID:%d case name:%s\n\r", 38034u, "Execute ENCLS_eax_0Ah_011");
	printf("\t\t Case ID:%d case name:%s\n\r", 38035u, "Execute ENCLS_eax_0Bh_012");
	printf("\t\t Case ID:%d case name:%s\n\r", 38037u, "Execute ENCLS_eax_0Ch_013");
	printf("\t\t Case ID:%d case name:%s\n\r", 38039u, "Execute ENCLS_eax_0Dh_014");
	printf("\t\t Case ID:%d case name:%s\n\r", 38040u, "Execute ENCLS_eax_0Eh_015");
	printf("\t\t Case ID:%d case name:%s\n\r", 38041u, "Execute ENCLS_eax_0Fh_016");
	printf("\t\t Case ID:%d case name:%s\n\r", 38042u, "Execute ENCLS_eax_10h_017");
	printf("\t\t Case ID:%d case name:%s\n\r", 38043u, "Execute ENCLS_eax_11h_018");
	printf("\t\t Case ID:%d case name:%s\n\r", 38044u, "Execute ENCLS_eax_12h_019");
	printf("\t\t Case ID:%d case name:%s\n\r", 38045u, "Execute ENCLS_eax_13h_020");
	printf("\t\t Case ID:%d case name:%s\n\r", 38050u, "Execute ENCLV_eax_00h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38052u, "Execute ENCLV_eax_01h_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38054u, "Execute ENCLV_eax_02h_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 38056u, "Execute ENCLU_eax_00h_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38057u, "Execute ENCLU_eax_01h_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38058u, "Execute ENCLU_eax_02h_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 38059u, "Execute ENCLU_eax_03h_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 38060u, "Execute ENCLU_eax_04h_005");
	printf("\t\t Case ID:%d case name:%s\n\r", 38061u, "Execute ENCLU_eax_05h_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 38062u, "Execute ENCLU_eax_06h_007");
	printf("\t\t Case ID:%d case name:%s\n\r", 38063u, "Execute ENCLU_eax_07h_008");
	printf("\t\t Case ID:%s case name:%s\n\r", "T13683", "Guest_CPUID_SGX_KEYS_001");
#endif
}

static void test_sgx()
{
	setup_ring_env();
#ifdef IN_NATIVE
	sgx_rqmid_36504_check_sgx_physical_support();
#else
	sgx_rqmid_27401_check_sgx_support();
	sgx_rqmid_27400_guest_cpuid_leaf_12h();
	sgx_rqmid_32532_write_msr_sgxownerepoch1();
	sgx_rqmid_32533_read_msr_sgxownerepoch1();
	sgx_rqmid_32534_write_msr_sgxownerepoch0();
	sgx_rqmid_32535_read_msr_sgxownerepoch0();
	sgx_rqmid_32540_write_ia32_sgxlepubkeyhash3();
	sgx_rqmid_32541_read_ia32_sgxlepubkeyhash3();
	sgx_rqmid_32542_write_ia32_sgxlepubkeyhash2();
	sgx_rqmid_32543_read_ia32_sgxlepubkeyhash2();
	sgx_rqmid_32544_write_ia32_sgxlepubkeyhash1();
	sgx_rqmid_32545_read_ia32_sgxlepubkeyhash1();
	sgx_rqmid_32546_write_ia32_sgxlepubkeyhash0();
	sgx_rqmid_32547_read_ia32_sgxlepubkeyhash0();
	sgx_rqmid_32548_write_ia32_sgx_svn_status();
	sgx_rqmid_32549_read_ia32_sgx_svn_status();
	sgx_rqmid_32550_check_supported_sgx();
	sgx_rqmid_38009_encls_eax_00h_001();
	sgx_rqmid_38010_encls_eax_01h_002();
	sgx_rqmid_38011_encls_eax_02h_003();
	sgx_rqmid_38012_encls_eax_03h_004();
	sgx_rqmid_38013_encls_eax_04h_005();
	sgx_rqmid_38029_encls_eax_05h_006();
	sgx_rqmid_38030_encls_eax_06h_007();
	sgx_rqmid_38031_encls_eax_07h_008();
	sgx_rqmid_38032_encls_eax_08h_009();
	sgx_rqmid_38033_encls_eax_09h_010();
	sgx_rqmid_38034_encls_eax_0Ah_011();
	sgx_rqmid_38035_encls_eax_0Bh_012();
	sgx_rqmid_38037_encls_eax_0Ch_013();
	sgx_rqmid_38039_encls_eax_0Dh_014();
	sgx_rqmid_38040_encls_eax_0Eh_015();
	sgx_rqmid_38041_encls_eax_0Fh_016();
	sgx_rqmid_38042_encls_eax_10h_017();
	sgx_rqmid_38043_encls_eax_11h_018();
	sgx_rqmid_38044_encls_eax_12h_019();
	sgx_rqmid_38045_encls_eax_13h_020();

	sgx_rqmid_38050_enclv_eax_00h_001();
	sgx_rqmid_38052_enclv_eax_01h_002();
	sgx_rqmid_38054_enclv_eax_02h_003();

	do_at_ring3(sgx_rqmid_38056_enclu_eax_00h_001, "");
	do_at_ring3(sgx_rqmid_38057_enclu_eax_01h_002, "");
	do_at_ring3(sgx_rqmid_38058_enclu_eax_02h_003, "");
	do_at_ring3(sgx_rqmid_38059_enclu_eax_03h_004, "");
	do_at_ring3(sgx_rqmid_38060_enclu_eax_04h_005, "");
	do_at_ring3(sgx_rqmid_38061_enclu_eax_05h_006, "");
	do_at_ring3(sgx_rqmid_38062_enclu_eax_06h_007, "");
	do_at_ring3(sgx_rqmid_38063_enclu_eax_07h_008, "");

	sgx_ACRN_T13683_Guest_CPUID_SGX_KEYS_001();
#endif
}

int main(void)
{
	print_case_list();
	setup_idt();
	test_sgx();

	return report_summary();
}

