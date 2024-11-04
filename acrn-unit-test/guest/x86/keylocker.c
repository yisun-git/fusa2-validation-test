#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "keylocker.h"
#include "vm.h"
#include "vmalloc.h"
#include "misc.h"
#include "apic.h"
#include "register_op.h"
#include "delay.h"
#include "fwcfg.h"


/**
 * @brief case name: Guest CPUID KL
 *
 * Summary: Execute CPUID.(EAX=07H, ECX=0H) to get ECX[bit 23] shall be 0
 */
static void keylocker_ACRN_T13773_Guest_CPUID_KL_001(void)
{
	report("\t\t %s", ((cpuid_indexed(0x07, 0x0).c) & CPUID_07_KL) == 0,
		__FUNCTION__);
}

/**
 * @brief case name: Guest CPUID keylocker
 *
 * Summary: Execute CPUID.(EAX=19H, ECX=0H) to get EAX/EBX/ECX/EDX shall be 0
 */
static void keylocker_ACRN_T13774_Guest_CPUID_Keylocker_002(void)
{
	bool flag = false;
	if ((cpuid(0x19).a == 0) && (cpuid(0x19).b == 0)
		&& (cpuid(0x19).c == 0) && (cpuid(0x19).d == 0)) {
		flag = true;
	}
	report("\t\t %s", flag, __FUNCTION__);
}
/**
 * @brief case name: Guest CR4.KL
 *
 * Summary: CR4.KL shall be 0 owing to hiding keylocker
 */
static void keylocker_ACRN_T13775_Guest_read_CR4_KL_003(void)
{
	unsigned long cr4 = read_cr4();

	report("%s", ((cr4 & CR4_KL) == 0), __FUNCTION__);
}


/**
 * @brief case name: Guest CR4.KL set
 *
 * Summary: Set CR4.KL should get #GP.
 */
static void keylocker_ACRN_T13776_Guest_set_CR4_KL_004(void)
{
	unsigned long value = (read_cr4() | CR4_KL);

	/* write 1H to CR4.KL and should get #GP */
    	asm volatile (ASM_TRY("1f")
		     "mov %0, %%cr4\n\t"
		     "1:" 
		     :  : "r"(value) : "memory");

	report("%s", exception_vector() == GP_VECTOR, __FUNCTION__);
}


static unsigned ENCODEKEY128_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"ENCODEKEY128 %%ecx, %%eax\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned ENCODEKEY256_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"ENCODEKEY256 %%ecx, %%eax\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

typedef unsigned __attribute__((vector_size(64))) keylocker512;

static unsigned AESDEC128KL_checking(void)
{
	keylocker512 m512;

	asm volatile(ASM_TRY("1f")
		"AESDEC128KL %0, %%xmm0 \n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESDEC256KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESDEC256KL %0, %%xmm0 \n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESDECWIDE128KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESDECWIDE128KL %0\n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESDECWIDE256KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESDECWIDE256KL %0\n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESENC128KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESENC128KL %0, %%xmm0 \n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESENC256KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESENC256KL %0, %%xmm0 \n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESENCWIDE128KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESENCWIDE128KL %0\n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned AESENCWIDE256KL_checking(void)
{
	keylocker512 m512;
	asm volatile(ASM_TRY("1f")
		"AESENCWIDE256KL %0\n\t"
		"1:"
		:
		: "m"(m512)
		: "memory");
	return exception_vector();
}

static unsigned LOADIWKEY_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"LOADIWKEY %%xmm2, %%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

/**
 * @brief case name: keylocker instruction execution. 
 *
 * Summary: When executing these instruction, user should get #UD, because of hidden key locker by hypervisor.
 */
static void keylocker_ACRN_T13777_Guest_keylocker_instruction_005(void)
{
	if (LOADIWKEY_checking() == UD_VECTOR &&
	    AESENCWIDE256KL_checking() == UD_VECTOR &&
	    AESENCWIDE128KL_checking() == UD_VECTOR &&
	    AESENC128KL_checking() == UD_VECTOR &&
	    AESENC256KL_checking() == UD_VECTOR &&
	    AESDECWIDE128KL_checking() == UD_VECTOR &&
	    AESDECWIDE256KL_checking() == UD_VECTOR &&
	    AESDEC256KL_checking() == UD_VECTOR &&
	    AESDEC128KL_checking() == UD_VECTOR &&
	    ENCODEKEY128_checking() == UD_VECTOR &&
	    ENCODEKEY256_checking() == UD_VECTOR)
	    report("\t\t %s", true, __FUNCTION__);
	else
	    report("\t\t %s", false, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS MASK_001
 *
 * Summary: Read from guest MSR register msr_copy_local_to_paltform shall generate #GP
 */
static void keylocker_ACRN_T13778_Read_msr_copy_local_to_paltform_006(void)
{
	u64 msr_ia32_copy_local_to_paltform;
	report("\t\t %s",
		rdmsr_checking(MSR_IA32_COPY_LOCAL_TO_PLATFORM,
			&msr_ia32_copy_local_to_paltform) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: keylocker_ACRN_6666_write_msr_copy_local_to_paltform
 *
 * Summary: Write from guest MSR register msr_copy_local_to_paltform shall generate #GP
 */
static void keylocker_ACRN_T13779_Write_msr_copy_local_to_paltform_007(void)
{
	u64 msr_ia32_copy_local_to_paltform = 0x12;
	report("\t\t %s",
		wrmsr_checking(MSR_IA32_COPY_LOCAL_TO_PLATFORM,
			msr_ia32_copy_local_to_paltform) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS MASK_001
 *
 * Summary: Read from guest MSR register msr_copy_local_to_paltform shall generate #GP
 */
static void keylocker_ACRN_T13780_Read_msr_copy_platform_to_local_008(void)
{
	u64 msr_ia32_copy_platform_to_paltform;
	report("\t\t %s",
		rdmsr_checking(MSR_IA32_COPY_PLATFORM_TO_LOCAL,
			&msr_ia32_copy_platform_to_paltform) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: keylocker_ACRN_6666_write_msr_copy_local_to_paltform
 *
 * Summary: Write from guest MSR register msr_copy_local_to_paltform shall generate #GP
 */
static void keylocker_ACRN_T13781_Write_msr_copy_platform_to_local_009(void)
{
	u64 msr_ia32_copy_platform_to_paltform = 0x13;
	report("\t\t %s",
		wrmsr_checking(MSR_IA32_COPY_PLATFORM_TO_LOCAL,
			msr_ia32_copy_platform_to_paltform) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS MASK_001
 *
 * Summary: Read from guest MSR register msr_copy_status shall generate #GP
 */
static void keylocker_ACRN_T13782_Read_msr_copy_status_010(void)
{
	u64 msr_ia32_copy_status;
	report("\t\t %s",
		rdmsr_checking(MSR_IA32_COPY_STATUS,
			&msr_ia32_copy_status) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: keylocker_ACRN_6666_write_msr_copy_local_to_paltform
 *
 * Summary: Write from guest MSR register msr_copy_status shall generate #GP
 */
static void keylocker_ACRN_T13783_Write_msr_copy_status_011(void)
{
	u64 msr_ia32_copy_status = 0x15;
	report("\t\t %s",
		wrmsr_checking(MSR_IA32_COPY_STATUS,
			msr_ia32_copy_status) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guestee that MSR UNCORE PRMRR PHYS MASK_001
 *
 * Summary: Read from guest MSR register msr_msr_ia32_iwkeybackup_status shall generate #GP
 */
static void keylocker_ACRN_T13784_Read_msr_msr_ia32_iwkeybackup_status_012(void)
{
	u64 msr_ia32_iwkeybackup_status;
	report("\t\t %s",
		rdmsr_checking(MSR_IA32_IWKEYBACKUP_STATUS,
			&msr_ia32_iwkeybackup_status) == GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: keylocker_ACRN_6666_write_msr_copy_local_to_paltform
 *
 * Summary: Write from guest MSR register msr_msr_ia32_iwkeybackup_status shall generate #GP
 */
static void keylocker_ACRN_T13785_Write_msr_ia32_iwkeybackup_status_013(void)
{
	u64 msr_ia32_iwkeybackup_status = 0x17;
	report("\t\t %s",
		wrmsr_checking(MSR_IA32_IWKEYBACKUP_STATUS,
			msr_ia32_iwkeybackup_status) == GP_VECTOR, __FUNCTION__);
}


static void print_case_list()
{
	printf("keylocker feature case list:\n\r");
	printf("\t Case ID:%s case name:%s\n\r", "T13773",
	          "keylocker_ACRN_T13773_Guest_CPUID_KL_001");
	printf("\t Case ID:%s case name:%s\n\r", "T13774",
	          "keylocker_ACRN_T13774_Guest_CPUID_Keylocker_002");
	printf("\t Case ID:%s case name:%s\n\r", "T13775",
	          "keylocker_ACRN_T13775_Guest_read_CR4_KL_003");
	printf("\t Case ID:%s case name:%s\n\r", "T13776",
	          "keylocker_ACRN_T13776_Guest_set_CR4_KL_004");
	printf("\t Case ID:%s case name:%s\n\r", "T13777",
	          "keylocker_ACRN_T13777_Guest_keylocker_instruction_005");
	printf("\t Case ID:%s case name:%s\n\r", "T13778",
	          "keylocker_ACRN_T13778_Read_msr_copy_local_to_paltform_006");
	printf("\t Case ID:%s case name:%s\n\r", "T13779",
	          "keylocker_ACRN_T13779_Write_msr_copy_local_to_paltform_007");
	printf("\t Case ID:%s case name:%s\n\r", "T13780",
	          "keylocker_ACRN_T13780_Read_msr_copy_platform_to_local_008");
	printf("\t Case ID:%s case name:%s\n\r", "T13781",
	          "keylocker_ACRN_T13781_Write_msr_copy_platform_to_local_009");
	printf("\t Case ID:%s case name:%s\n\r", "T13782",
	          "keylocker_ACRN_T13782_Read_msr_copy_status");
	printf("\t Case ID:%s case name:%s\n\r", "T13783",
	          "keylocker_ACRN_T13783_Write_msr_copy_status_011");
	printf("\t Case ID:%s case name:%s\n\r", "T13784",
	          "keylocker_ACRN_T13784_Read_msr_msr_ia32_iwkeybackup_status_012");
	printf("\t Case ID:%s case name:%s\n\r", "T13785",
	          "keylocker_ACRN_T13785_Write_msr_ia32_iwkeybackup_status_013");
}

static void test_keylocker()
{
	setup_ring_env();
	keylocker_ACRN_T13773_Guest_CPUID_KL_001();
	keylocker_ACRN_T13774_Guest_CPUID_Keylocker_002();
	keylocker_ACRN_T13775_Guest_read_CR4_KL_003();
	keylocker_ACRN_T13776_Guest_set_CR4_KL_004();
	keylocker_ACRN_T13777_Guest_keylocker_instruction_005();
	keylocker_ACRN_T13778_Read_msr_copy_local_to_paltform_006();
	keylocker_ACRN_T13779_Write_msr_copy_local_to_paltform_007();
	keylocker_ACRN_T13780_Read_msr_copy_platform_to_local_008();
	keylocker_ACRN_T13781_Write_msr_copy_platform_to_local_009();
	keylocker_ACRN_T13782_Read_msr_copy_status_010();
	keylocker_ACRN_T13783_Write_msr_copy_status_011();
	keylocker_ACRN_T13784_Read_msr_msr_ia32_iwkeybackup_status_012();
	keylocker_ACRN_T13785_Write_msr_ia32_iwkeybackup_status_013();
}

int main(void)
{
	print_case_list();
	setup_idt();
	test_keylocker();

	return report_summary();
}