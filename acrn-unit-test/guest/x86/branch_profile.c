#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "smp.h"
#include "msr.h"
#include "regdump.h"

#define MSR_IA32_DEBUGCTL           0x000001D9U
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
 * @brief case name: access_MSR_LBR_INFO_X_001
 *
 * Summary: When a vCPU attempts to access MSR_LBR_INFO_X (0<=X<=31),
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 * execute RDMSR to access MSR_LBR_INFO_X (0<=X<=31),
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static void branch_rqmid_39152_MSR_LBR_INFO_X_001()
{
	bool result = true;
	unsigned vector;
	unsigned err_code;

	for (int i = 0; i < 32; i++) {
		vector = rdmsr_checking(MSR_LBR_INFO_0 + i);
		err_code = exception_error_code();
		if ((vector != GP_VECTOR) || (err_code != 0)) {
			result = false;
			break;
		}
	}
	report("%s", result, __FUNCTION__);
}
/*
 * @brief case name: access_MSR_LBR_INFO_X_001
 *
 * Summary: When a vCPU attempts to access MSR_LBR_INFO_X (0<=X<=31),
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 * execute WRMSR to access MSR_LBR_INFO_X (0<=X<=31),
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 */
static void branch_rqmid_39153_MSR_LBR_INFO_X_002()
{
	bool result = true;
	unsigned vector;
	unsigned err_code;

	for (int i = 0; i < 32; i++) {
		vector = wrmsr_checking(MSR_LBR_INFO_0 + i, 0);
		err_code = exception_error_code();
		if ((vector != GP_VECTOR) || (err_code != 0)) {
			result = false;
			break;
		}
	}
	report("%s", result, __FUNCTION__);
}
/*
 * @brief case name: read IA32_MISC_ENABLE[bit 12:11]
 *
 * Summary: When a vCPU attempts to read the guest IA32_MISC_ENABLE[bit 12:11],
 * ACRN hypervisor shall guarantee that the vCPU gets 3H.
 *
 * Using RDMSR instruction to read IA32_MISC_ENABLE[11:12] .The value shall be 3H.
 */
static void branch_rqmid_39154_read_IA32_MISC_ENABLE_bit_12_11()
{
	u64 val;

	val = rdmsr(MSR_IA32_MISC_ENABLE);
	report("%s", ((val >> 11u) & 0x3) == 0x3, __FUNCTION__);
}
/*
 * @brief case name: IA32_MISC_ENABLE_bit_12_11_write
 *
 * Summary: When a vCPU attempts to write IA32_MISC_ENABLE and
 * the new guest IA32_MISC_ENABLE[bit 12:11] is different from the old guest IA32_MISC_ENABLE[bit 12:11],
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 * Using WRMSR instruction to write IA32_MISC_ENABLE[11:12] (the value can be 0,1 or 2),vCPU receives #GP(0).
 */
static void branch_rqmid_39155_IA32_MISC_ENABLE_bit_12_11_write()
{
	u64 val;

	val = rdmsr(MSR_IA32_MISC_ENABLE);
	val ^= ~(1UL << 12);/*make IA32_MISC_ENABLE[bit 12]  difference from old value*/
	wrmsr_checking(MSR_IA32_MISC_ENABLE, val);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_X_TO_IP_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LASTBRANCH_X_TO_IP, ACRN
 * hypervisor shall guarantee that the vCPU will receive #GP(0)
 * Using RDMSR/WRMSR instruction to read/write MSR_LASTBRANCH_X_TO_IP(0<=X<=31)
 */
static void branch_rqmid_28217_MSR_LASTBRANCH_X_TO_IP_001()
{
	for (u32 i = 0; i < 32; i++) {
		rdmsr_checking(MSR_LASTBRANCH_0_TO_IP + i);
		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_X_TO_IP_002
 *
 * Summary: When a vCPU attempts to read or write MSR_LASTBRANCH_X_TO_IP, ACRN
 * hypervisor shall guarantee that the vCPU will receive #GP(0)
 * Using RDMSR/WRMSR instruction to read/write MSR_LASTBRANCH_X_TO_IP(0<=X<=31)
 */
static void branch_rqmid_28255_MSR_LASTBRANCH_X_TO_IP_002()
{
	for (u32 i = 0; i < 32; i++) {
		wrmsr_checking(MSR_LASTBRANCH_0_TO_IP + i, 0);
		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_X_FROM_IP_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LASTBRANCH_X_FROM_IP , ACRN hypervisor
 * shall guarantee that the vCPU will receive #GP(0)
 * Using RDMSR/WRMSR instruction to read/write MSR_LASTBRANCH_X_FROM_IP (0<=X<=31)
 */
static void branch_rqmid_28218_MSR_LASTBRANCH_X_FROM_IP_001()
{
	for (u32 i = 0; i < 32; i++) {
		rdmsr_checking(MSR_LASTBRANCH_0_FROM_IP + i);
		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_X_FROM_IP_002
 *
 * Summary: When a vCPU attempts to read or write MSR_LASTBRANCH_X_FROM_IP , ACRN hypervisor
 * shall guarantee that the vCPU will receive #GP(0)
 * Using RDMSR/WRMSR instruction to read/write MSR_LASTBRANCH_X_FROM_IP (0<=X<=31)
 */
static void branch_rqmid_28262_MSR_LASTBRANCH_X_FROM_IP_002()
{
	for (u32 i = 0; i < 32; i++) {
		wrmsr_checking(MSR_LASTBRANCH_0_FROM_IP + i, 0);
		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: IA32_DS_AREA_001
 *
 * Summary: When a vCPU attempts to read or write IA32_DS_AREA, ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28222_IA32_DS_AREA_001()
{
	rdmsr_checking(MSR_IA32_DS_AREA);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: IA32_DS_AREA_002
 *
 * When a vCPU attempts to read or write IA32_DS_AREA, ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28266_MSR_IA32_DS_AREA_002()
{
	wrmsr_checking(MSR_IA32_DS_AREA, 0);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LBR_SELECT_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LBR_SELECT, ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28228_MSR_LBR_SELECT_001()
{
	rdmsr_checking(MSR_LBR_SELECT);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LBR_SELECT_002
 *
 * Summary:execute WRMSR instruction with input value
 * as the address of MSR_LBR_SELECT register and 0
 */
static void branch_rqmid_28269_MSR_LBR_SELECT_002()
{
	wrmsr_checking(MSR_LBR_SELECT, 0);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LER_FROM_LIP_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LER_FROM_LIP, ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28235_MSR_LER_FROM_LIP_001()
{
	rdmsr_checking(MSR_LER_FROM_LIP);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LER_FROM_LIP_002
 *
 * Summary:execute WRMSR instruction with input value
 * as the address of MSR_LER_FROM_LIP register and 0
 */
static void branch_rqmid_28272_MSR_LER_FROM_LIP_002()
{
	wrmsr_checking(MSR_LER_FROM_LIP, 0);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LER_TO_LIP_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LER_TO_LIP , ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28240_MSR_LER_TO_LIP_001()
{
	rdmsr_checking(MSR_LER_TO_LIP);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LER_TO_LIP_002
 *
 * Summary:execute WRMSR instruction with input value
 * as the address of MSR_LER_TO_LIP register and 0
 */
static void branch_rqmid_28276_MSR_LER_TO_LIP_002()
{
	wrmsr_checking(MSR_LER_TO_LIP, 0);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_TOS_001
 *
 * Summary: When a vCPU attempts to read or write MSR_LASTBRANCH_001 , ACRN hypervisor shall guarantee
 * that the vCPU will receive #GP(0)
 */
static void branch_rqmid_28246_MSR_LASTBRANCH_TOS_001()
{
	rdmsr_checking(MSR_LASTBRANCH_TOS);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
/*
 * @brief case name: MSR_LASTBRANCH_TOS_002
 *
 * Summary:execute WRMSR instruction with input value
 * as the address of MSR_LASTBRANCH_TOS register and 0
 */
static void branch_rqmid_28279_MSR_LASTBRANCH_TOS_002()
{
	wrmsr_checking(MSR_LASTBRANCH_TOS, 0);
	report("%s",
		   (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
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
 * @brief case name: CPUID.01H:ECX[2]_001
 *
 * Summary:When a vCPU attempts to read CPUID.01H,
 * ACRN Hypervisor shall write 0 to guest ECX[bit 2]
 */
static void branch_rqmid_29700_CPUID_01H_ECX_bit2_001()
{
	report("%s", !(cpuid(1).c & (1 << 2)), __FUNCTION__);
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
	report("%s", !(cpuid(1).d & (1 << 21)), __FUNCTION__);
}
/*
 * @brief case name: CPUID.01H:ECX[4]_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H, ACRN Hypervisor shall guarantee guest
 * ECX[4] shall be 0
 *
 */
static void branch_rqmid_28216_CPUID_01H_ECX_bit4_001()
{
	report("%s", !(cpuid(1).c & (1 << 4)), __FUNCTION__);
}
/*
 * @brief case name: IA32_MISC_ENABLE[bit 12:11] initial value following start-up_001
 *
 * Summary: ACRN hypervisor shall set the initial IA32_MISC_ENABLE[11-12] of guest VM to 0
 *
 */
static void branch_rqmid_28215_IA32_misc_enable_following_startup_001()
{
	/*@cstart64.S, BP will save IA32_MISC_ENABLE lower 32bit value to 0x6000*/
	u32 ia32_misc_enable = *(u32 *)0x6000;
	report("%s", ((ia32_misc_enable >> 11) & 0x3) == 0x3, __FUNCTION__);
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
	printf("\t\t Case ID:%d case name:%s\n\r", 39152u, "branch_rqmid_39152_MSR_LBR_INFO_X_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39153u, "branch_rqmid_39153_MSR_LBR_INFO_X_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 39154u, "branch_rqmid_39154_read_IA32_MISC_ENABLE_bit_12_11");
	printf("\t\t Case ID:%d case name:%s\n\r", 39155u, "branch_rqmid_39155_IA32_MISC_ENABLE_bit_12_11_write");
	printf("\t\t Case ID:%d case name:%s\n\r", 28251u, "branch profile IA32 debugctl 001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28282u, "branch profile IA32 debugctl 002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28338u, "branch profile CPUID01H EDX21 001");

	printf("\t\t Case ID:%d case name:%s\n\r", 28339u, "branch profile IA32 misc enable following init 001");

	printf("\t\t Case ID:%d case name:%s\n\r", 28215u, "branch profile IA32 misc enable following startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28216u, "CPUID_01H_ECX_bit4_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28217u, "MSR_LASTBRANCH_X_TO_IP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28218u, "MSR_LASTBRANCH_X_FROM_IP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28222u, "IA32_DS_AREA_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28228u, "MSR_LBR_SELECT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28235u, "MSR_LER_FROM_LIP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28240u, "MSR_LER_TO_LIP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28246u, "MSR_LASTBRANCH_TOS_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28255u, "MSR_LASTBRANCH_X_TO_IP_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28262u, "MSR_IA32_DS_AREA_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28266u, "MSR_IA32_DS_AREA_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28269u, "MSR_LBR_SELECT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28272u, "MSR_LER_FROM_LIP_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28276u, "MSR_LER_TO_LIP_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28279u, "MSR_LASTBRANCH_TOS_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29700u, "CPUID_01H_ECX_bit2_001");

	printf("\t\t \n\r \n\r");
}
int main(void)
{
	setup_idt();

	print_case_list();
	branch_rqmid_39152_MSR_LBR_INFO_X_001();
	branch_rqmid_39153_MSR_LBR_INFO_X_002();
	branch_rqmid_39154_read_IA32_MISC_ENABLE_bit_12_11();
	branch_rqmid_39155_IA32_MISC_ENABLE_bit_12_11_write();
	branch_rqmid_28217_MSR_LASTBRANCH_X_TO_IP_001();
	branch_rqmid_28255_MSR_LASTBRANCH_X_TO_IP_002();
	branch_rqmid_28218_MSR_LASTBRANCH_X_FROM_IP_001();
	branch_rqmid_28262_MSR_LASTBRANCH_X_FROM_IP_002();
	branch_rqmid_28222_IA32_DS_AREA_001();
	branch_rqmid_28266_MSR_IA32_DS_AREA_002();
	branch_rqmid_28228_MSR_LBR_SELECT_001();
	branch_rqmid_28269_MSR_LBR_SELECT_002();
	branch_rqmid_28235_MSR_LER_FROM_LIP_001();
	branch_rqmid_28272_MSR_LER_FROM_LIP_002();
	branch_rqmid_28240_MSR_LER_TO_LIP_001();
	branch_rqmid_28276_MSR_LER_TO_LIP_002();
	branch_rqmid_28246_MSR_LASTBRANCH_TOS_001();
	branch_rqmid_28279_MSR_LASTBRANCH_TOS_002();
	branch_rqmid_28251_IA32_debugctl_001();
	branch_rqmid_28282_IA32_debugctl_002();
	branch_rqmid_29700_CPUID_01H_ECX_bit2_001();
	branch_rqmid_28338_CPUID01H_EDX21_001();
	branch_rqmid_28216_CPUID_01H_ECX_bit4_001();
	branch_rqmid_28215_IA32_misc_enable_following_startup_001();
	branch_rqmid_28339_IA32_misc_enable_following_init_001();

	return report_summary();
}

