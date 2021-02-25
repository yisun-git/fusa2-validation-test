/*
 * Copyright (C) 2019 Intel Corporation. All right reserved.
 * Test mode: Machine Check Architecture, according to the
 * Chapter 15, Vol 3, SDM
 * Written by xuepengcheng, 2019-06-01
 */
#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "types.h"
#include "isr.h"
#include "apic.h"
#include "machine_check.h"
#include "register_op.h"

int check_cpuid_1_edx(unsigned int bit)
{
	return (cpuid(1).d & bit) != 0;
}

int check_cpuid_5_ecx(unsigned int bit)
{
	return (cpuid(5).c & bit) != 0;
}

#ifdef IN_SAFETY_VM

/*
 * @brief case name: MCA_set_IA32_MCG_CAP_initial_state_of_safety_VMs_following_Start-up_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_MCG_CAP of safety VM to 040AH following start-up.
 *
 */
static void MCA_rqmid_24569_set_IA32_MCG_CAP_following_startup_001()
{
	report("%s", (*(volatile uint32_t *)STARTUP_IA32_MCG_CAP) == 0x40A, __FUNCTION__);
}

/*
 * @brief case name: MCA_rqmid_37031_initialize_IA32_MCi_CTL2_following_startup_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest IA32_MCi_CTL2 (0<=i<=3) of safety VM to 0H following start-up.
 *
 */
static void MCA_rqmid_37031_initialize_IA32_MCi_CTL2_following_startup_001()
{

	int i;
	printf("read msr = 0x%lx\r\n", rdmsr(MSR_IA32_MCx_CTL2(0)));
	for (i = 0; i < MAX_VALID_MCI_CTL2_BANK_NUM; i++) {
		if ((*(volatile uint64_t *)(STARTUP_IA32_MC0_CTL2_ADDR + i * 8)) != 0) {
			report("%s: line[%d] val= 0x%lx", 0, __FUNCTION__, __LINE__,
					(*(volatile uint64_t *)(STARTUP_IA32_MC0_CTL2_ADDR + i * 8)));
			return;
		}

	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA guarantee implemented bits of safety-VM IA32_MCi_CTL same as host 001
 *
 *Summary:
 *	For any IA32_MCi_CTL (0<=i<=9) of safety-VM, ACRN hypervisor shall guarantee that
 *	the implemented bits in guest IA32_MCi_CTL (0<=i<=9) are the same as
 *	the implemented bits in host IA32_MCi_CTL (0<=i<=9).
 *
 *	To read IA32_MCi_CTL registers of safety-VM ,
 *	each register value should be the same as host's value
 *
 */
static void MCA_rqmid_28168_guarantee_implemented_IA32_MCi_CTL_same_as_host_001(void)
{
	bool ret = true;

	if ((rdmsr(MSR_IA32_MCx_CTL(0)) != 0xfff) || (rdmsr(MSR_IA32_MCx_CTL(1)) != 0xf)
		|| (rdmsr(MSR_IA32_MCx_CTL(2)) != 0xf) || (rdmsr(MSR_IA32_MCx_CTL(3)) != 0x1f)
		|| (rdmsr(MSR_IA32_MCx_CTL(4)) != 0x3f) || (rdmsr(MSR_IA32_MCx_CTL(5)) != 0x1)
		|| (rdmsr(MSR_IA32_MCx_CTL(6)) != 0xff) || (rdmsr(MSR_IA32_MCx_CTL(7)) != 0xff)
		|| (rdmsr(MSR_IA32_MCx_CTL(8)) != 0xff) || (rdmsr(MSR_IA32_MCx_CTL(9)) != 0xff)
	   ) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: MCA expose valid Error Reporting register bank count of safety VM 001
 *
 *Summary:
 *	 ACRN hypervisor shall expose physical Error-Reporting register banks count to safety VM,
 *	in compliance with Chapter 15.3.1.1, Vol. 3, SDM.
 *
 *	Read the 0-7bit of IA32_MCG_CAP.Count register, the bank count shall 0AH.
 *
 */
/*27410-MCA_expose_valid_Error_Reporting_register_bank_count_of_safety_VM_001*/
static void MCA_rqmid_27410_expose_valid_Error_Reporting_register_bank_count_safety_VM_001()
{
	report("%s", (rdmsr(MSR_IA32_MCG_CAP) & 0xFFu) == 0xa, __FUNCTION__);
}
/*
 *@brief case name: MCA expose valid Error Reporting register bank MSRs of safety VM 001(void)
 *
 *Summary:
 *	 ACRN hypervisor shall expose Error-Reporting register bank MSRs(0<=i<=9) to safety VM,
 *	in compliance with Chapter 15.3.2, Vol. 3, SDM.;
 *	Clear each IA32_MCi_STATUS register, it shall have no exception.
 *
 */

/*27281-MCA_expose_valid_Error_Reporting_register_bank_MSRs_of_safety_VM_001*/
static void MCA_rqmid_27281_expose_valid_Error_Reporting_register_bank_MSRs_safety_VM_001(void)
{
	int i;
	u64 pre_val;
	u64 val;

	/* Read/Write IA32_MCi_CTL, normal operation for safety VM(0<=i<=9) */
	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if (rdmsr_checking(MSR_IA32_MCx_CTL(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if (wrmsr_checking(MSR_IA32_MCx_CTL(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		if (rdmsr(MSR_IA32_MCx_CTL(i)) != val) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
#if 0
		val = 1;
		if (wrmsr_checking(MSR_IA32_MCx_CTL(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		if (rdmsr(MSR_IA32_MCx_CTL(i)) != val) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
#endif
		/* restore msr */
		wrmsr(MSR_IA32_MCx_CTL(i), pre_val);
	}

#if 0

	/* Read/Write IA32_MCi_CTL, shoud generate #GP when i >= 0AH */
	if (rdmsr_checking(MSR_IA32_MCx_CTL(i), &val) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	val = 0;
	/* Read/Write IA32_MCi_CTL, shoud generate #GP when i >= 0AH */
	if (wrmsr_checking(MSR_IA32_MCx_CTL(i), val) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	val = 1;
	/* Read/Write IA32_MCi_CTL, shoud generate #GP when i >= 0AH */
	if (wrmsr_checking(MSR_IA32_MCx_CTL(i), val) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

#endif

		/* Read/Write IA32_MCi_ADDR, normal operation for safety VM(0<=i<=9) */
	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if (rdmsr_checking(MSR_IA32_MCx_CTL2(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		if (rdmsr(MSR_IA32_MCx_CTL2(i)) != val) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
#if 0
		val = 1;
		if (wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		if (rdmsr(MSR_IA32_MCx_STATUS(i)) != val) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
#endif
		/* restore msr */
		wrmsr(MSR_IA32_MCx_CTL2(i), pre_val);
	}


	report("%s", 1, __FUNCTION__);
}

/*
 *@brief case name: MCA hide invalid Error Reporting register bank MSRs for safety VM 001
 *
 *Summary:
 *	When a vCPU of safety VM attempts to access any guest Error-Reporting register bank MSRs (i>=aH) ,
 *	ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 *
 *	Read IA32_MCi_CTL/IA32_MCi_STATUS/IA32_MCi_ADDR/IA32_MCi_MISC/IA32_MCi_CTL2 register(i>=aH),
 *      ACRN hypervisor shall generate #GP(0).
 *
 */
static void MCA_rqmid_27153_hide_invalid_Error_Reporting_register_bank_MSRs_safety_VM_001(void)
{
	u64 val;
	int idx = 0x0A;

	/* access IA32_MCi_CTL(i==0xAH), shoud generate #GP for safety VM */
	rdmsr_checking(MSR_IA32_MCx_CTL(idx), &val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	wrmsr_checking(MSR_IA32_MCx_CTL(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	wrmsr_checking(MSR_IA32_MCx_CTL(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* access IA32_MCi_STATUS(i==0xAH), shoud generate #GP for safety VM */
	rdmsr_checking(MSR_IA32_MCx_STATUS(idx), &val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	wrmsr_checking(MSR_IA32_MCx_STATUS(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	wrmsr_checking(MSR_IA32_MCx_STATUS(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* access IA32_MCi_ADDR(i==0xAH), shoud generate #GP for safety VM */
	rdmsr_checking(MSR_IA32_MCx_ADDR(idx), &val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	wrmsr_checking(MSR_IA32_MCx_ADDR(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	wrmsr_checking(MSR_IA32_MCx_ADDR(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* access IA32_MCi_MISC(i==0xAH), shoud generate #GP for safety VM */
	rdmsr_checking(MSR_IA32_MCx_MISC(idx), &val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	wrmsr_checking(MSR_IA32_MCx_MISC(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	wrmsr_checking(MSR_IA32_MCx_MISC(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* access IA32_MCi_CTL2(i==0xAH), shoud generate #GP for safety VM */
	rdmsr_checking(MSR_IA32_MCx_CTL2(idx), &val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	wrmsr_checking(MSR_IA32_MCx_CTL2(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	wrmsr_checking(MSR_IA32_MCx_CTL2(idx), val);
	if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA Expose general MCA to safety VM
 *
 *Summary:
 *	ACRN hypervisor shall expose general machine-check architecture to safety VM.
 *
 */
static void MCA_rqmid_36881_expose_general_MCA_to_safety_VM_001(void)
{
	bool ret = true;
	unsigned long cr4 = read_cr4();

	/* write 0H to CR4.MCE and check */
	write_cr4(cr4 & (~CR4_MCE));
	if ((read_cr4() & CR4_MCE) != 0) {

		ret = false;
	}

	/* write 1H to CR4.MCE and check */
	write_cr4((read_cr4() | CR4_MCE));
	if ((read_cr4() & CR4_MCE) != CR4_MCE) {

		ret = false;
	}

	/* restore cr4 */
	write_cr4(cr4 | CR4_MCE);
	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: read IA32_MCi_CTL2 for safety VM
 *
 *Summary:
 *	When a vCPU of safety-VM attempts to read guest MSR IA32_MCi_CTL2 (3<i<=9),
 *      ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_rqmid_37025_read_IA32_MCi_CTL2_for_safety_VM_001(void)
{
	int i;
	u64 val;

	/* Read read guest MSR IA32_MCi_CTL2 (3 <i<= 9),  */
	for (i = 4; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		val = rdmsr(MSR_IA32_MCx_CTL2(i));
		if (val  != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: report unavailable ADDR and MISC for valid error-reporting register bank MSRs in safety VM
 *
 *Summary:
 *	When a vCPU in the safety VM attempts to read from an Error-Reporting register bank MSR (0 <= i <= 9),
 *  ACRN hypervisor shall guarantee that the vCPU gets 0 at bit 59:58.
 *
 */
static void MCA_rqmid_37027_report_unavailable_add_and_misc_001(void)
{
	int i;
	u64 val;

	/* Read IA32_MCi_STATUS, shoud guarantee that the vCPU gets 0 at bit 59:58 */
	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		val = rdmsr(MSR_IA32_MCx_STATUS(i));
		if ((val & (MCI_STATUS_ADDRV | MCI_STATUS_MISCV)) != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: write IA32_MCi_STATUS with non-zero for safety VM
 *
 *Summary:
 *	When a vCPU of safety VM attempts to write non-zero to guest MSR IA32_MCi_STATUS (0<=i<=9),
 *  ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_36992_write_IA32_MCi_STATUS_with_non_zero_for_safety_VM_001(void)
{
	int i;
	u64 val = 1;

	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: access IA32_MCi_ADDR  for safety VM
 *
 *Summary:
 *  When a vCPU of safety VM attempts to access guest MSR IA32_MCi_ADDR (0<=i<=9),
 *  ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_37639_access_IA32_MCi_ADDR_for_safety_VM_001(void)
{
	int i;
	u64 val;

	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		if ((rdmsr_checking(MSR_IA32_MCx_ADDR(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: access IA32_MCi_MISC  for safety VM
 *
 *Summary:
 *  When a vCPU of safety VM attempts to access guest MSR IA32_MCi_MISC (0<=i<=9),
 *  ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_37641_access_IA32_MCi_MISC_for_safety_VM_001(void)
{
	int i;
	u64 val;

	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		if ((rdmsr_checking(MSR_IA32_MCx_MISC(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_MISC(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_MISC(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}


#endif
#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name: MCA set IA32_FEATURE_CONTROL.LMCE_ON init state following init 001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_FEATURE_CONTROL.LMCE_ON[bit 20]
 *	of non-safety VM to 0 following INIT. IA32_FEATURE_CONTROL, reference the Table 2-2, Vol. 4, SDM.
 *
 *	Read the IA32_FEATURE_CONTROL[bit 20].LMCE_ON initial value is 0 at AP init. in none-safety VM
 */
static void MCA_rqmid_24454_set_IA32_FEATURE_CONTROL_LMCE_ON_following_init_001()
{
	report("%s", (*(volatile uint32_t *)INIT_IA32_FEATURE_CONTROL & IA32_FEATURE_CONTROL_LMCE_ON) == 0,
			__FUNCTION__);
}

/*
 * @brief case name: MCA set IA32_FEATURE_CONTROL.LMCE_ON init state following startup 001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_FEATURE_CONTROL.LMCE_ON[bit 20]
 *	of non-safety VM to 0 following INIT. IA32_FEATURE_CONTROL, reference the Table 2-2, Vol. 4, SDM.
 *
 *	Read the IA32_FEATURE_CONTROL[bit 20].LMCE_ON initial value is 0 at AP init.
 */
static void MCA_rqmid_24456_set_IA32_FEATURE_CONTROL_LMCE_ON_following_startup_001()
{
	report("%s", (*(volatile uint32_t *)STARTUP_IA32_FEATURE_CONTROL & IA32_FEATURE_CONTROL_LMCE_ON) == 0,
			__FUNCTION__);
}


/*
 * @brief case name: MCA_set_IA32_MCG_CAP_initial_state_of_non-safety_VMs_following_Start-up_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_MCG_CAP of non-safety VM to 0H following start-up.
 *
 */
static void MCA_rqmid_24551_set_IA32_MCG_CAP_following_startup_001()
{
	report("%s", (*(volatile uint32_t *)STARTUP_IA32_MCG_CAP) == 0, __FUNCTION__);
}

/*
 * @brief case name: MCA_rqmid_24534_set_IA32_MCG_CAP_following_init_001
 *
 *Summary:
 *	ACRN hypervisor shall set initial guest MSR IA32_MCG_CAP of non-safety VM to 0H following INIT.
 *
 */
static void MCA_rqmid_24534_set_IA32_MCG_CAP_following_init_001()
{
	report("%s", (*(volatile uint64_t *)INIT_IA32_MCG_CAP_LOW_ADDR) == 0, __FUNCTION__);
}

/*
 * @brief case name: MCA_set_IA32_MCG_STATUS_initial_state_of_non-safety_VMs_following_INIT_001
 *
 *Summary:
 *	ACRN hypervisor shall set initial guest MSR IA32_MCG_STATUS to 0 following INIT.
 *
 */
static void MCA_rqmid_24531_set_IA32_MCG_STATUS_following_init_001()
{
	report("%s", (*(volatile uint64_t *)INIT_IA32_MCG_STATUS_LOW_ADDR) == 0, __FUNCTION__);
}

/*
 * @brief case name: MCA_set_IA32_MCG_STATUS_initial_state_of_non-safety_VMs_following_Start-up_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_MCG_STATUS of non-safety VM to 0H following start-up.
 *
 */
static void MCA_rqmid_24533_set_IA32_MCG_STATUS_following_startup_001()
{
	report("%s", (*(volatile uint32_t *)STARTUP_IA32_MCG_STATUS) == 0, __FUNCTION__);
}

/*
 *cap:bank_num should not 0, but tested 0
 *0, no exception
 *13, #GP
 *==No00  MSR_IA32_MC0_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC0_CTL2:exp:0
 *==No01  MSR_IA32_MC1_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC1_CTL2:exp:0
 *==No02  MSR_IA32_MC2_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC2_CTL2:exp:0
 *==No03  MSR_IA32_MC3_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC3_CTL2:exp:0
 *==No04  MSR_IA32_MC4_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC4_CTL2:exp:0
 *==No05  MSR_IA32_MC5_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC5_CTL2:exp:0
 *==No06  MSR_IA32_MC6_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC6_CTL2:exp:0
 *==No07  MSR_IA32_MC7_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC7_CTL2:exp:0
 *==No08  MSR_IA32_MC8_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC8_CTL2:exp:0
 *==No09  MSR_IA32_MC9_CTL: exp:0 STATUS:exp:0    ADDR:exp:0      MISC:exp:0      MC9_CTL2:exp:0
 *==No10  MSR_IA32_MC10_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC10_CTL2:exp:13
 *==No11  MSR_IA32_MC11_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC11_CTL2:exp:13
 *==No12  MSR_IA32_MC12_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC12_CTL2:exp:13
 *==No13  MSR_IA32_MC13_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC13_CTL2:exp:13
 *==No14  MSR_IA32_MC14_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC14_CTL2:exp:13
 *==No15  MSR_IA32_MC15_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC15_CTL2:exp:13
 *==No16  MSR_IA32_MC16_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC16_CTL2:exp:13
 *==No17  MSR_IA32_MC17_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC17_CTL2:exp:13
 *==No18  MSR_IA32_MC18_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC18_CTL2:exp:13
 *==No19  MSR_IA32_MC19_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC19_CTL2:exp:13
 *==No20  MSR_IA32_MC20_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC20_CTL2:exp:13
 *==No21  MSR_IA32_MC21_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC21_CTL2:exp:13
 *==No22  MSR_IA32_MC22_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC22_CTL2:exp:13
 *==No23  MSR_IA32_MC23_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC23_CTL2:exp:13
 *==No24  MSR_IA32_MC24_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC24_CTL2:exp:13
 *==No25  MSR_IA32_MC25_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC25_CTL2:exp:13
 *==No26  MSR_IA32_MC26_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC26_CTL2:exp:13
 *==No27  MSR_IA32_MC27_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC27_CTL2:exp:13
 *==No28  MSR_IA32_MC28_CTL: exp:13       STATUS:exp:13   ADDR:exp:13     MISC:exp:13     MC28_CTL2:exp:13
 *==No29  MC29_CTL2:exp:13
 *==No30  MC30_CTL2:exp:13
 *==No31  MC31_CTL2:exp:13
 */

/*
 * @brief case name: MCA hide Error Reporting register bank MSRs of non safety VM 001
 *
 *Summary:
 *	 When any vCPU of non-safety VM attempts to access guest Error-Reporting Register Bank MSRs
 *	(MSR address range is [400H-473H] or [280H-29FH]),
 *	ACRN hypervisor shall inject #GP(0) to vCPU. MSR registers, reference the Table 2-2, Vol. 4, SDM.
 *
 *	 Read Error-Reporting Register Bank MSRs(MSR address range is [400H-473H]),
 *	ACRN hypervisor shall generate #GP(0).
 *
 */
static void MCA_rqmid_26099_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_001(void)
{
	u32 a, d;

	for (u32 i = 0x400; i < 0x474; i++) {
		asm volatile (ASM_TRY("1f")
					  "rdmsr\n\t"
					  "1:"
					  : "=a"(a), "=d"(d) : "c"(i) : "memory");

		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
}
/*
 *@brief case name: MCA inject GP if write IA32_MCG_STATUS with non zero 002
 *
 *Summary:
 *	 When any vCPU of non-safety VM attempts to access guest Error-Reporting Register Bank MSRs
 *	(MSR address range is [400H-473H] or [280H-29FH]),
 *	ACRN hypervisor shall inject #GP(0) to vCPU. MSR registers, reference the Table 2-2, Vol. 4, SDM.
 *
 *	 Read Error-Reporting Register Bank MSRs(MSR address range is [280H-29FH]),
 *	ACRN hypervisor shall generate #GP(0).
 *
 */
static void MCA_rqmid_26101_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_002(void)
{
	u32 a, d;

	for (u32 i = 0x280; i < 0x2A0; i++) {
		asm volatile (ASM_TRY("1f")
					  "rdmsr\n\t"
					  "1:"
					  : "=a"(a), "=d"(d) : "c"(i) : "memory");

		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_hide_MCA_in_CPUID_leaf_01H_for_non-safety_VM_001
 *
 *Summary:
 *	ACRN hypervisor shall hide general machine-check architecture from non-safety VM.
 *
 */
static void MCA_rqmid_26257_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001(void)
{

	bool ret = true;

	/* CPUID.01H:EDX[bit 14].MCA == 0H. */
	if (check_cpuid_1_edx(CPUID_1_EDX_MCA)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: MCA Write 1H to CR4.MCE for non-safety VM
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to write 1H to guest CR4.MCE,
 *	ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_36880_write_1H_to_CR4_MCE_for_non_safety_VM_001(void)
{
	unsigned long cr4 = read_cr4();

	/* write 1H to CR4.MCE and check */
	if ((write_cr4_checking(cr4 | CR4_MCE) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA Access Error-Reporting register bank MSRs to non-safety VM
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest Error-Reporting Register Bank MSRs (0H<=i<=0AH),
 *  the bank count is 0AH on target validation platform, here will cover when i == 0AH,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_36803_access_error_reporting_register_bank_MSRs_to_non_safety_VM_001(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_CTL, shoud generate #GP for non-safety VM */
	for (i = 0; i <= MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_CTL(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	/* Read/Write IA32_MCi_STATUS, shoud generate #GP for non-satety VM */
	for (i = 0; i <= MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_STATUS(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* write zero */
		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* write non-zero*/
		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	/* Read/Write IA32_MCi_ADDR, shoud generate #GP for non-safety VM */
	for (i = 0; i <= MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_ADDR(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	/* Read/Write IA32_MCi_MISC, shoud generate #GP for non-safety VM */
	for (i = 0; i <= MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_MISC(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_MISC(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	/* Read/Write IA32_MCi_CTL2, shoud generate #GP for non-safety VM */
	for (i = 0; i <= MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_CTL2(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Read CR4.MCE bit for non-safety VM
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to read guest CR4.MCE,
 *      ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_rqmid_36792_read_CR4_MCE_bit_for_non_safety_VM_001(void)
{
	bool ret = true;

	unsigned long cr4 = read_cr4();

	if ((cr4 & CR4_MCE) != 0) {

		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to CR4.MCE for non-safety VM
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to write 0H to guest CR4.MCE,
 *	ACRN hypervisor shall keep CR4.MCE unchanged.
 *
 */
static void MCA_rqmid_36797_write_0H_to_CR4_MCE_for_non_safety_VM_001(void)
{
	bool ret = true;
	unsigned long cr4;

	/* write 1H to CR4.MCE and check */
	if ((write_cr4_checking(0) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	cr4 = read_cr4();

	if ((cr4 & CR4_MCE) != 0) {

		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

#endif

/*
 * @brief case name: MCA_rqmid_24477_set_IA32_P5_MC_TYPE_following_startup_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_TYPE to 0 following start-up.
 *
 */
static void MCA_rqmid_24477_set_IA32_P5_MC_TYPE_following_startup_001()
{
	report("%s", (*(volatile uint64_t *)STARTUP_IA32_P5_MC_TYPE_LOW_ADDR) == 0, __FUNCTION__);
}

/*
 * @brief case name: MCA_rqmid_24477_set_IA32_P5_MC_ADDR_following_startup_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_ADDR to 0 following start-up.
 *
 */
static void MCA_rqmid_24476_set_IA32_P5_MC_ADDR_following_startup_001()
{
	report("%s", (*(volatile uint64_t *)STARTUP_IA32_P5_MC_ADDR_LOW_ADDR) == 0, __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name: MCA_rqmid_24478_set_IA32_P5_MC_TYPE_following_init_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_TYPE to 0 following INIT.
 *
 */
static void MCA_rqmid_24478_set_IA32_P5_MC_TYPE_following_init_001()
{
	report("%s", (*(volatile uint64_t *)INIT_IA32_P5_MC_TYPE_LOW_ADDR) == 0, __FUNCTION__);
}



/*
 * @brief case name: MCA_rqmid_24473_set_IA32_P5_MC_ADDR_following_init_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_ADDR to 0 following INIT.
 *
 */
static void MCA_rqmid_24473_set_IA32_P5_MC_ADDR_following_init_001()
{
	report("%s", (*(volatile uint64_t *)INIT_IA32_P5_MC_ADDR_LOW_ADDR) == 0, __FUNCTION__);
}

/*
 * @brief case name: MCA_rqmid_27737_set_CR4_MCE_following_init_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest CR4.MCE to 0H following INIT.
 *
 */
static void MCA_rqmid_27737_set_CR4_MCE_following_init_001()
{
	report("%s", ((*(volatile uint32_t *)INIT_CR4_REG_ADDR) & CR4_MCE) == 0, __FUNCTION__);
}
#endif

/*
 * @brief case name: MCA_rqmid_27736_set_CR4_MCE_following_startup_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest CR4.MCE to 0H following startup.
 *
 */
static void MCA_rqmid_27736_set_CR4_MCE_following_startup_001()
{
	report("%s", ((*(volatile uint32_t *)STARTUP_CR4_REG_ADDR) & CR4_MCE) == 0, __FUNCTION__);
}


/*
 * @brief case name: MCA set hide IA32_MCG_CTL 001
 *
 *Summary:
 *	 When any vCPU attempts to access guest MSR IA32_MCG_CTL, ACRN hypervisor shall inject #GP(0) to vCPU.
 *	 MCG_CTL_P[bit 8] is not set in IA32_MCG_CAP on target validation platform,
 *   Reference the Chapter 15.3.1.3, Vol. 3, SDM.
 *
 *	 Read msr IA32_MCG_CAP.MCG_CTL_P[bit 8] shall be 0H in none-safety & safety VM
 */
static void MCA_rqmid_24604_hide_IA32_MCG_CTL_001(void)
{
	u64 val;

	val = rdmsr(MSR_IA32_MCG_CAP);
	if ((val & IA32_MCG_CAP_CTL_P) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	if ((rdmsr_checking(MSR_IA32_MCG_CTL, &val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_MCG_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA inject GP if write IA32_MCG_STATUS with non zero 001
 *
 *Summary:
 *	 An attempt to write to IA32_MCG_STATUS with any value other
 *	than 0 would result in #GP(0)
 *
 *	 Write a value(non-zero) to IA32_MCG_STATUS register in 64bit mode,
 *	ACRN hypervisor shall generate #GP(0).
 */
static void MCA_rqmid_25084_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_001()
{
	u64 val = 0x01;
	u64 ev = 0;
	unsigned err_code;

	ev = wrmsr_checking(MSR_IA32_MCG_STATUS, val);
	err_code = exception_error_code();

	report("%s", (ev == GP_VECTOR) && (err_code == 0), __FUNCTION__);
}

/*
 * @brief case name: MCA inject GP if write IA32_MCG_STATUS with non zero 002
 *
 *Summary:
 *	 ACRN hypervisor shall hide local machine check exception from any VM.
 *
 *	 Write a value(zero) to IA32_MCG_STATUS register,
 *   the result shall be 0H by read the register.
 */
static void MCA_rqmid_25242_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_002()
{
	u64 val = 0x0;

	wrmsr(MSR_IA32_MCG_STATUS, val);
	report("%s", rdmsr(MSR_IA32_MCG_STATUS) == 0, __FUNCTION__);
}

/*
 * @brief case name: hide  local machine check exception
 *
 *Summary:
 *	 An attempt to write to IA32_MCG_STATUS with any value other
 *	than 0 would result in #GP(0)
 *
 *  IA32_MCG_CAP.MCG_LMCE_ P[bit 27]shold be 0, and accessing IA32_MCG_EXT_CTL will cause #GP(0).
 */
static void MCA_rqmid_36893_hide_local_machine_check_exception_001()
{
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_MCG_CAP);
	/* IA32_MCG_CAP.MCG_LMCE_ P[bit 27] == 0 */
	if ((val & MCG_LMCE_P) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing IA32_MCG_EXT_CTL will cause #GP(0) */
	if ((rdmsr_checking(MSR_IA32_MCG_EXT_CTL, &val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_EXT_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Hide IA32_MCG_CTL
 *
 *Summary:
 *	 ACRN hypervisor shall hide IA32_MCG_CTL MSR from any VM.
 *
 *  IA32_MCG_CAP.MCG_CTL_P[bit 8]= shold be 0, and accessing IA32_MCG_CTL will cause #GP(0).
 */
static void MCA_rqmid_36888_hide_IA32_MCG_CTL_001()
{
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_MCG_CAP);
	/* IA32_MCG_CAP.MCG_CTL_P[bit 8] == 0 */
	if ((val & MCG_CTL_P) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing IA32_MCG_CTL will cause #GP(0) */
	if ((rdmsr_checking(MSR_IA32_MCG_CTL, &val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Hide IA32_MCG_EXT_CTL
 *
 *Summary:
 *	 ACRN hypervisor shall hide IA32_MCG_EXT_CTL MSR from any VM.
 *
 *  IA32_MCG_CAP.MCG_LMCE_P[bit27]= shold be 0, and accessing IA32_MCG_CTL will cause #GP(0).
 */
static void MCA_rqmid_37439_hide_IA32_MCG_EXT_CTL_001()
{
	u64 val;

	val = rdmsr(MSR_IA32_MCG_CAP);
	/* IA32_MCG_CAP.MCG_LMCE_P[bit27] == 0 */
	if ((val & MCG_LMCE_P) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing IA32_MCG_EXT_CTL will cause #GP(0) */
	if ((rdmsr_checking(MSR_IA32_MCG_EXT_CTL, &val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_EXT_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_MCG_EXT_CTL, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Read IA32_FEATURE_CONTROL.LMCE_ON
 *
 *Summary:
 *	 When a vCPU of any VM attempts to read guest IA32_FEATURE_CONTROL.LMCE_ON,
 *   ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_rqmid_36793_read_IA32_FEATURE_CONTROL_LMCE_ON_001()
{
	bool ret = true;
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_FEATURE_CONTROL);
	/* IA32_MCG_CAP.LMCE_ON[bit 20] == 0 */
	if ((val & IA32_FEATURE_CONTROL_LMCE_ON) != 0) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: write IA32_MCG_CAP
 *
 *Summary:
 *	 When a vCPU of safety and non-safety VM attempts to write MSR IA32_MCG_CAP,
 *   hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 */
static void MCA_rqmid_36781_Write_IA32_MCG_CAP_001()
{
	bool ret = true;

	if ((wrmsr_checking(MSR_IA32_MCG_CAP, 0) != GP_VECTOR) ||
		(wrmsr_checking(MSR_IA32_MCG_CAP, 1) != GP_VECTOR) || (exception_error_code() != 0)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: write IA32_P5_MC_TYPE with non-zero
 *
 *Summary:
 *	 When a vCPU of any VM attempts to write non-zero to guest MSR IA32_P5_MC_TYPE,
 *   ACRN hypervisor shall inject #GP(0) to the vCPU
 *
 */
static void MCA_rqmid_36847_write_IA32_P5_MC_TYPE_with_non_zero_001()
{
	bool ret = true;

	if ((wrmsr_checking(MSR_IA32_P5_MC_TYPE, 1) != GP_VECTOR) || (exception_error_code() != 0)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: write IA32_P5_MC_ADDR with non-zero
 *
 *Summary:
 *	 When a vCPU of safety and non-safety VM attempts to write non-zero to guest MSR IA32_P5_MC_ADDR,
 *   ACRN hypervisor shall inject #GP(0) to the vCPU
 *
 */
static void MCA_rqmid_36780_write_IA32_P5_MC_ADDR_with_non_zero_001()
{
	bool ret = true;

	if ((wrmsr_checking(MSR_IA32_P5_MC_ADDR, 1) != GP_VECTOR) || (exception_error_code() != 0)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}


/*
 * @brief case name: Expose MC common MSRs
 *
 *Summary:
 *	 ACRN hypervisor shall expose common Machine Check MSRs to any VM.
 *
 * Global Machine Check MSR IA32_P5_MC_ADDR/IA32_P5_MC_TYPE/IA32_MCG_CAP/IA32_MCG_STATUS are
 * all available for Kabylake forwards processers.
 */
static void MCA_rqmid_37648_expose_MC_common_MSRs_001()
{
	u64 val;


	/* accessing MSR_IA32_MCG_CAP */
	if ((rdmsr_checking(MSR_IA32_MCG_CAP, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_CAP, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_MCG_CAP, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}


	/* accessing MSR_IA32_MCG_STATUS */
	if ((rdmsr_checking(MSR_IA32_MCG_STATUS, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1; /*MCG_STATUS write non-zero generate GP(0)*/
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing MSR_IA32_P5_MC_ADDR */
	if ((rdmsr_checking(MSR_IA32_P5_MC_ADDR, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_P5_MC_ADDR, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_P5_MC_ADDR, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing MSR_IA32_P5_MC_TYPE */
	if ((rdmsr_checking(MSR_IA32_P5_MC_TYPE, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 0;
	if ((wrmsr_checking(MSR_IA32_P5_MC_TYPE, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_P5_MC_TYPE, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Write LMCE_ON bit of IA32_FEATURE_CONTROL
 *
 *Summary:
 *	 While the guest MSR IA32_FEATURE_CONTROL.LOCK is 1H, when a vCPU of any VM attempts to
 *   write guest IA32_FEATURE_CONTROL.LMCE_ON, ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_rqmid_36779_write_LMCE_ON_bit_of_IA32_FEATURE_CONTROL_001()
{
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_FEATURE_CONTROL);

	if ((val  & IA32_FEATURE_CONTROL_LOCK) != IA32_FEATURE_CONTROL_LOCK) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	if ((wrmsr_checking(MSR_IA32_P5_MC_TYPE, val | IA32_FEATURE_CONTROL_LMCE_ON) != GP_VECTOR)
			|| (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}


#ifdef QEMU
static volatile bool handled_mc_ex = false;
static void handle_mc_check(struct ex_regs *regs)
{
	printf("received MC exception\r\n");
	handled_mc_ex = true;
}

/*
 * @brief case name: MCA expose valid error reporting registers bank MSRs of safte vm 004
 *
 *Summary:
 *	 Emulate MCE using Qemu and check Error-Reporting Register Bank MSRs;
 *	(hypervisor: ACRN hypervisor shall expose physical Error-Reporting register bank MSRs
 *	(number of bank is less than 0BH) to safety VM, in compliance with Chapter 15.3.2, Vol. 3, SDM.)
 *
 *	Inject an MC exception into guest using qemu command in the qemu,
 *	by using: mce 2 5 0xbe00000000800400 2 0x3f628bd69349 1
 *	the guest shall be generate #MC .
 *
 */

static void MCA_rqmid_33845_expose_valid_error_reporting_registers_bank_MSRs_safte_vm_004()
{

#define MC5_STATUS_ERR_REPORT_QEMU	0xbe00000000800400ULL
#define MC5_ADDR_ERR_REPORT_QEMU	0xbe00000000800400ULL
#define MC5_MISC_ERR_REPORT_QEMU	0x1
#define MCG_CAP_ERR_REPORT_QEMU		0x40aULL
#define MCG_STATUS_ERR_REPORT_QEMU	0x4ULL

	handler old;
	u64 cr4;

	cr4 = read_cr4();
	write_cr4(cr4 | CR4_MCE);

	old = handle_exception(MC_VECTOR, handle_mc_check);

	while (!handled_mc_ex);

	if ((rdmsr(MSR_IA32_MCx_STATUS(5)) == MC5_STATUS_ERR_REPORT_QEMU)
		/* && (rdmsr(MSR_IA32_MCx_ADDR(5)) == MC5_ADDR_ERR_REPORT_QEMU) */
		/* && (rdmsr(MSR_IA32_MCx_MISC(5)) == MC5_MISC_ERR_REPORT_QEMU) */
		&& (rdmsr(MSR_IA32_MCG_CAP) == MCG_CAP_ERR_REPORT_QEMU)
		&& (rdmsr(MSR_IA32_MCG_STATUS) == MCG_STATUS_ERR_REPORT_QEMU)) {

		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}

	write_cr4(cr4);
	handle_exception(MC_VECTOR, old);
}
#endif

#ifdef IN_NATIVE

/*
 * @brief case name: Physical Common machine check MSRs support
 *
 *Summary:
 *	 Common machine check MSRs shall be available on physical platform.
 *
 */
static void MCA_rqmid_36510_physical_common_machine_check_MSRs_support_AC_001()
{

	u64 val;

	/* accessing MSR_IA32_MCG_CAP */
	if (rdmsr_checking(MSR_IA32_MCG_CAP, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing MSR_IA32_MCG_STATUS */
	if (rdmsr_checking(MSR_IA32_MCG_STATUS, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing MSR_IA32_P5_MC_ADDR */
	if (rdmsr_checking(MSR_IA32_P5_MC_ADDR, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* accessing MSR_IA32_P5_MC_TYPE */
	if (rdmsr_checking(MSR_IA32_P5_MC_TYPE, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);

}

/*
 * @brief case name: Some physical Error-Reporting Register Banks MSRs support
 *
 *Summary:
 *	 Error-Reporting Register Banks MSRs (0<=i<=9) shall be available on physical platform.
 *
 */
static void MCA_rqmid_36509_some_physical_error_reporting_register_banks_MSRs_suppor_AC_001()
{

	u64 val;
	int i;

	if ((rdmsr(MSR_IA32_MCG_CAP) & 0xff) != MAX_VALID_ERROR_REPORTING_BANK_NUM) {

		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {

		/* read MSR IA32_MCi_CTL */
		if (rdmsr_checking(MSR_IA32_MCx_CTL(i), &val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* read MSR IA32_MCi_STATUS */
		if (rdmsr_checking(MSR_IA32_MCx_STATUS(i), &val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* read MSR IA32_MCi_CTL2 */
		if (rdmsr_checking(MSR_IA32_MCx_CTL2(i), &val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

	}

	report("%s", 1, __FUNCTION__);

}

/*
 * @brief case name: General MCA support
 *
 *Summary:
 *	 General machine-check architecture shall be available on physical platform.
 *
 */
static void MCA_rqmid_36514_general_MCA_support_AC_001()
{
	u64 val;
	int i;
	u64 cr4;

	/*read CPUID.01H: EDX.MCA[bit 14]==1 CPUID.01H: EDX.MCE[bit 7] == 1*/
	if (((cpuid(1).d & CPUID_1_EDX_MCA) != CPUID_1_EDX_MCA) ||
			((cpuid(1).d & CPUID_1_EDX_MCE) != CPUID_1_EDX_MCE)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* IA32_MCG_CAP.MCG_CMCI_P[bit 10] == 1 */
	if ((rdmsr(MSR_IA32_MCG_CAP) & MCG_CMCI_P) != MCG_CMCI_P) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	for (i = 0; i < MAX_VALID_MCI_CTL2_BANK_NUM; i++) {
		val = rdmsr(MSR_IA32_MCx_CTL2(i));
		wrmsr(MSR_IA32_MCx_CTL2(i), val | CMCI_EN);
		/* write IA32_MCi_CTL2[bits 30] = 1 without exception */
		if ((rdmsr(MSR_IA32_MCx_CTL2(i)) & CMCI_EN) != CMCI_EN) {
			report("%s: line[%d] i=%d", 0, __FUNCTION__, __LINE__, i);
			return;
		}

		/*restore default value */
		wrmsr(MSR_IA32_MCx_CTL2(i), val);

		/* Write IA32_MCi_CTL2[bits 14:0] with value in [14:0] without exception */
		val = rdmsr(MSR_IA32_MCx_CTL2(i)) & 0x7FFF;
		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	/* Write cr4.mce = 1 without exception */
	cr4 = read_cr4();
	write_cr4(cr4 | CR4_MCE);
	if ((read_cr4() & CR4_MCE) != CR4_MCE) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	/* restore CR4 */
	write_cr4(cr4);

	/* read IA32_MCi_STATUS without exception */
	for (i = 0; i < MAX_VALID_ERROR_REPORTING_BANK_NUM; i++) {
		/* IA32_MCi_STATUS */
		if (rdmsr_checking(MSR_IA32_MCx_STATUS(i), &val) == GP_VECTOR) {
			report("%s: line[%d] i=%d", 0, __FUNCTION__, __LINE__, i);
			return;
		}

	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC0_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC0_CTL on the physical platform shall be 0FFFH
 *
 */
static void MCA_rqmid_36517_machine_check_implemented_bits_of_host_IA32_MC0_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC0_CTL, shoud get 0FFFH */
	if (rdmsr(MSR_IA32_MCx_CTL(0)) != 0x0FFF) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC1_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC1_CTL on the physical platform shall be 0FH
 *
 */
static void MCA_rqmid_36515_machine_check_implemented_bits_of_host_IA32_MC1_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC1_CTL, shoud get 0FH */
	if (rdmsr(MSR_IA32_MCx_CTL(1)) != 0x0F) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC2_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC2_CTL on the physical platform shall be 0FH
 *
 */
static void MCA_rqmid_36518_machine_check_implemented_bits_of_host_IA32_MC2_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC1_CTL, shoud get 0FH */
	if (rdmsr(MSR_IA32_MCx_CTL(2)) != 0x0F) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC3_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC3_CTL on the physical platform shall be 01FH
 *
 */
static void MCA_rqmid_36507_machine_check_implemented_bits_of_host_IA32_MC3_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC3_CTL, shoud get 01FH */
	if (rdmsr(MSR_IA32_MCx_CTL(3)) != 0x01F) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC4_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC4_CTL on the physical platform shall be 03FH
 *
 */
static void MCA_rqmid_36506_machine_check_implemented_bits_of_host_IA32_MC4_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC4_CTL, shoud get 03FH */
	if (rdmsr(MSR_IA32_MCx_CTL(4)) != 0x03F) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC5_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC5_CTL on the physical platform shall be 01H
 *
 */
static void MCA_rqmid_36508_machine_check_implemented_bits_of_host_IA32_MC5_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC5_CTL, shoud get 01H */
	if (rdmsr(MSR_IA32_MCx_CTL(5)) != 0x01) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC6_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC6_CTL on the physical platform shall be 0FFH
 *
 */
static void MCA_rqmid_36511_machine_check_implemented_bits_of_host_IA32_MC6_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC6_CTL, shoud get 0FFH */
	if (rdmsr(MSR_IA32_MCx_CTL(6)) != 0xFF) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC7_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC7_CTL on the physical platform shall be 0FFH
 *
 */
static void MCA_rqmid_36512_machine_check_implemented_bits_of_host_IA32_MC7_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC7_CTL, shoud get 0FFH */
	if (rdmsr(MSR_IA32_MCx_CTL(7)) != 0xFF) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC8_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC8_CTL on the physical platform shall be 0FFH
 *
 */
static void MCA_rqmid_36513_machine_check_implemented_bits_of_host_IA32_MC8_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC8_CTL, shoud get 0FFH */
	if (rdmsr(MSR_IA32_MCx_CTL(8)) != 0xFF) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: Implemented bits of host IA32_MC9_CTL
 *
 *Summary:
 *	 The implemented bits in host IA32_MC9_CTL on the physical platform shall be 0FFH
 *
 */
static void MCA_rqmid_36516_machine_check_implemented_bits_of_host_IA32_MC9_CTL_AC_001()
{

	bool ret = true;

	/* Read IA32_MC9_CTL, shoud get 0FFH */
	if (rdmsr(MSR_IA32_MCx_CTL(9)) != 0xFF) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}


#endif

static void test_mca_safety_vm(void)
{
#ifdef IN_SAFETY_VM
	MCA_rqmid_24569_set_IA32_MCG_CAP_following_startup_001();
	MCA_rqmid_37031_initialize_IA32_MCi_CTL2_following_startup_001();
	MCA_rqmid_28168_guarantee_implemented_IA32_MCi_CTL_same_as_host_001();
	MCA_rqmid_27410_expose_valid_Error_Reporting_register_bank_count_safety_VM_001();
	MCA_rqmid_27281_expose_valid_Error_Reporting_register_bank_MSRs_safety_VM_001();
	MCA_rqmid_27153_hide_invalid_Error_Reporting_register_bank_MSRs_safety_VM_001();
	MCA_rqmid_36881_expose_general_MCA_to_safety_VM_001();
	MCA_rqmid_37025_read_IA32_MCi_CTL2_for_safety_VM_001();
	MCA_rqmid_37027_report_unavailable_add_and_misc_001();
	MCA_rqmid_36992_write_IA32_MCi_STATUS_with_non_zero_for_safety_VM_001();
	MCA_rqmid_37639_access_IA32_MCi_ADDR_for_safety_VM_001();
	MCA_rqmid_37641_access_IA32_MCi_MISC_for_safety_VM_001();
#endif
}

static void test_mca_non_safety_vm(void)
{
#ifdef IN_NON_SAFETY_VM
	MCA_rqmid_26099_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_001();
	MCA_rqmid_26101_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_002();
	MCA_rqmid_24454_set_IA32_FEATURE_CONTROL_LMCE_ON_following_init_001();
	MCA_rqmid_24456_set_IA32_FEATURE_CONTROL_LMCE_ON_following_startup_001();
	MCA_rqmid_24551_set_IA32_MCG_CAP_following_startup_001();
	MCA_rqmid_24534_set_IA32_MCG_CAP_following_init_001();
	MCA_rqmid_24531_set_IA32_MCG_STATUS_following_init_001();
	MCA_rqmid_24533_set_IA32_MCG_STATUS_following_startup_001();
	MCA_rqmid_26257_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001();
	MCA_rqmid_36880_write_1H_to_CR4_MCE_for_non_safety_VM_001();
	MCA_rqmid_36803_access_error_reporting_register_bank_MSRs_to_non_safety_VM_001();
	MCA_rqmid_36792_read_CR4_MCE_bit_for_non_safety_VM_001();
	MCA_rqmid_36797_write_0H_to_CR4_MCE_for_non_safety_VM_001();
#endif
}
void test_mca(void)
{
	test_mca_non_safety_vm();
	test_mca_safety_vm();
	MCA_rqmid_24477_set_IA32_P5_MC_TYPE_following_startup_001();
	MCA_rqmid_24476_set_IA32_P5_MC_ADDR_following_startup_001();
#ifdef IN_NON_SAFETY_VM
	MCA_rqmid_24478_set_IA32_P5_MC_TYPE_following_init_001();
	MCA_rqmid_24473_set_IA32_P5_MC_ADDR_following_init_001();
	MCA_rqmid_27737_set_CR4_MCE_following_init_001();
#endif
	MCA_rqmid_27736_set_CR4_MCE_following_startup_001();
	MCA_rqmid_24604_hide_IA32_MCG_CTL_001();
	MCA_rqmid_25084_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_001();
	MCA_rqmid_25242_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_002();
	MCA_rqmid_36893_hide_local_machine_check_exception_001();
	MCA_rqmid_36888_hide_IA32_MCG_CTL_001();
	MCA_rqmid_37439_hide_IA32_MCG_EXT_CTL_001();
	MCA_rqmid_36793_read_IA32_FEATURE_CONTROL_LMCE_ON_001();
	MCA_rqmid_36781_Write_IA32_MCG_CAP_001();
	MCA_rqmid_36847_write_IA32_P5_MC_TYPE_with_non_zero_001();
	MCA_rqmid_36780_write_IA32_P5_MC_ADDR_with_non_zero_001();
	MCA_rqmid_37648_expose_MC_common_MSRs_001();
	MCA_rqmid_36779_write_LMCE_ON_bit_of_IA32_FEATURE_CONTROL_001();
}
static void print_case_list(void)
{
	printf("machine check feature case list:\n\r");
#ifdef QEMU
	printf("\t Case ID:%d case name:%s\n\r", 33485,
		   "expose valid error reporting registers bank MSRs safte-vm 004");
#elif IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 36510,
	   "physical_common_machine_check_MSRs_support_001");
	printf("\t Case ID:%d case name:%s\n\r", 36509,
	   "Some physical Error-Reporting Register Banks MSRs support");
	printf("\t Case ID:%d case name:%s\n\r", 36514,
	   "General MCA support");
	printf("\t Case ID:%d case name:%s\n\r", 36517,
	   "implemented_bits_of_host_IA32_MC0_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36515,
	   "implemented_bits_of_host_IA32_MC1_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36518,
	   "implemented_bits_of_host_IA32_MC2_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36507,
	   "implemented_bits_of_host_IA32_MC3_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36506,
	   "implemented_bits_of_host_IA32_MC4_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36508,
	   "implemented_bits_of_host_IA32_MC5_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36511,
	   "implemented_bits_of_host_IA32_MC6_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36512,
	   "implemented_bits_of_host_IA32_MC7_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36513,
	   "implemented_bits_of_host_IA32_MC8_CTL_001");
	printf("\t Case ID:%d case name:%s\n\r", 36516,
	   "implemented_bits_of_host_IA32_MC9_CTL_001");
#else
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 24569,
		   "MCA set IA32_MCG_CAP initial state of safty VMs following Start-up 001");
	printf("\t Case ID:%d case name:%s\n\r", 37030,
		   "set_IA32_MCi_STATUS_0_9_initial_state_of_safety_VMs_following_Start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 37031,
			"set_IA32_MCi_CTL2_0_3_initial_state_of_safety_VMs_following_Start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 28168,
			"guarantee implemented IA32 MCi CTL same as host 001");
	printf("\t Case ID:%d case name:%s\n\r", 27410,
			"expose valid Error Reporting register bank count safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 27281,
			"expose valid Error Reporting register bank MSRs safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 27153,
			"hide valid Error Reporting register bank MSRs safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 36881,
			"Expose general MCA to safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 37025,
			"read IA32_MCi_CTL2 for safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 37027,
			"Report unavailable ADDR and MISC in Safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 36992,
			"write IA32_MCi_STATUS with non-zero for safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 37639,
			"access IA32_MCi_ADDR for safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 37641,
		   "access IA32_MCi_MISC for safety VM");
#endif
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 26099,
		   "hide Error Reporting register bank MSRs of non safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 26101,
		   "hide Error Reporting register bank MSRs of non safety VM 002");
	printf("\t Case ID:%d case name:%s\n\r", 24454,
		   "set IA32 FEATURE CONTROL LMCE ON following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 24456,
		   "set IA32 FEATURE CONTROL LMCE ON following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 24534,
		   "set IA32_MCG_CAP following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 24531,
		   "set IA32_MCG_STATUS following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 24533,
		   "set IA32_MCG_STATUS following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 26257,
		   "MCA_hide_MCA_in_CPUID_leaf_01H_for_non-safety_VM_001");
	printf("\t Case ID:%d case name:%s\n\r", 36880,
		   "Write 1H to CR4.MCE for non-safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 36883,
		   "Access Error-Reporting register bank MSRs to non-safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 36792,
		   "Read CR4.MCE bit for non-safety VM");
	printf("\t Case ID:%d case name:%s\n\r", 36797,
		   "Write 0H to CR4.MCE for non-safety VM");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 24477,
			"set IA32_P5_MC_TYPE following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 24478,
			"set IA32_P5_MC_TYPE following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 24476,
			"set IA32_P5_MC_ADDR following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 24473,
			"set IA32_P5_MC_ADDR following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 27737,
			"set CR4.MCE following init 001");
	printf("\t Case ID:%d case name:%s\n\r", 27736,
			"set CR4.MCE following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 24604,
			"hide IA32 MCG CTL 001");
	printf("\t Case ID:%d case name:%s\n\r", 25084,
			"inject GP if write IA32 MCG STATUS with non zero 001");
	printf("\t Case ID:%d case name:%s\n\r", 25242,
			"inject GP if write IA32 MCG STATUS with non zero 002");
	printf("\t Case ID:%d case name:%s\n\r", 36893,
			"hide  local machine check exception");
	printf("\t Case ID:%d case name:%s\n\r", 36888,
			"Hide IA32_MCG_CTL");
	printf("\t Case ID:%d case name:%s\n\r", 37439,
			"Hide IA32_MCG_EXT_CTL");
	printf("\t Case ID:%d case name:%s\n\r", 36793,
			"Read IA32_FEATURE_CONTROL.LMCE_ON");
	printf("\t Case ID:%d case name:%s\n\r", 36781,
			"write IA32_MCG_CAP");
	printf("\t Case ID:%d case name:%s\n\r", 36847,
			"write IA32_P5_MC_TYPE with non-zero");
	printf("\t Case ID:%d case name:%s\n\r", 36780,
			"write IA32_P5_MC_ADDR with non-zero");
	printf("\t Case ID:%d case name:%s\n\r", 37648,
			"Expose MC common MSRs");
	printf("\t Case ID:%d case name:%s\n\r", 36779,
			"Write LMCE_ON bit of IA32_FEATURE_CONTROL");
#endif

}

int main(void)
{
	setup_idt();
	print_case_list();
#ifdef QEMU
	MCA_rqmid_33845_expose_valid_error_reporting_registers_bank_MSRs_safte_vm_004();
#elif IN_NATIVE
	MCA_rqmid_36510_physical_common_machine_check_MSRs_support_AC_001();
	MCA_rqmid_36509_some_physical_error_reporting_register_banks_MSRs_suppor_AC_001();
	MCA_rqmid_36514_general_MCA_support_AC_001();
	MCA_rqmid_36517_machine_check_implemented_bits_of_host_IA32_MC0_CTL_AC_001();
	MCA_rqmid_36515_machine_check_implemented_bits_of_host_IA32_MC1_CTL_AC_001();
	MCA_rqmid_36518_machine_check_implemented_bits_of_host_IA32_MC2_CTL_AC_001();
	MCA_rqmid_36507_machine_check_implemented_bits_of_host_IA32_MC3_CTL_AC_001();
	MCA_rqmid_36506_machine_check_implemented_bits_of_host_IA32_MC4_CTL_AC_001();
	MCA_rqmid_36508_machine_check_implemented_bits_of_host_IA32_MC5_CTL_AC_001();
	MCA_rqmid_36511_machine_check_implemented_bits_of_host_IA32_MC6_CTL_AC_001();
	MCA_rqmid_36512_machine_check_implemented_bits_of_host_IA32_MC7_CTL_AC_001();
	MCA_rqmid_36513_machine_check_implemented_bits_of_host_IA32_MC8_CTL_AC_001();
	MCA_rqmid_36516_machine_check_implemented_bits_of_host_IA32_MC9_CTL_AC_001();
#else
	test_mca();
#endif

	return report_summary();
}

