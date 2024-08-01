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
#include "machine_check_info.h"
#include "register_op.h"
#include "fwcfg.h"
#include "delay.h"
#include "misc.h"

void ap_main(void)
{
	asm volatile ("pause");
}

#ifndef IN_NON_SAFETY_VM
static bool check_mce_mca_capability()
{
	/*read CPUID.01H: EDX.MCA[bit 14]==1 CPUID.01H: EDX.MCE[bit 7] == 1*/
	if (((cpuid(1).d & CPUID_1_EDX_MCA) != CPUID_1_EDX_MCA) ||
			((cpuid(1).d & CPUID_1_EDX_MCE) != CPUID_1_EDX_MCE)) {
		return false;
	}
	return true;
}
#endif

#if defined(QEMU)
/*
 * @brief case name: MCA_ACRN_T10048_expose_Error_Reporting_Register_for_safety_VM_003.
 *
 *Summary:
 *	 Emulate MCE using Qemu and check Error-Reporting Register Bank MSRs;
 *
 *	Inject an MC exception into guest using qemu command in the qemu,
 *	by using:
 *	mce 1 5 0x9200000000800400 0xd 0x9e00000000800400 1,
 *	mce [-b] cpu bank status mcgstatus addr misc -- inject a MCE on the given CPU
 *	qemu will update Error Reporting Register, user can get these updates from
 *	safety VM or native.
 *
 */
static void MCA_ACRN_T10048_expose_Error_Reporting_Register_for_safety_VM_003(void)
{
#define MC5_STATUS_ERR_REPORT_QEMU	0x9200000000800400ULL
#define MC5_ADDR_ERR_REPORT_QEMU	0x9e00000000800400ULL
#define MC5_MISC_ERR_REPORT_QEMU	0x1
#define MCG_STATUS_ERR_REPORT_QEMU	0xdULL

	u64 cr4;

	/* Check the support of MCA/MCE */
	if (!check_mce_mca_capability())
	{
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	cr4 = read_cr4();
	write_cr4(cr4 | CR4_MCE);

	/* Wait here until trigger a MCE through Qemu */
	while (!rdmsr(MSR_IA32_MCx_ADDR(5))){
	}

	/* Implemented bits of safety-VM IA32_MCG_STATUS same as host except bit 3 is hidden for safety-VM */
	if ((rdmsr(MSR_IA32_MCx_STATUS(5)) == MC5_STATUS_ERR_REPORT_QEMU)
		&& (rdmsr(MSR_IA32_MCx_ADDR(5)) == MC5_ADDR_ERR_REPORT_QEMU)
		&& (rdmsr(MSR_IA32_MCx_MISC(5)) == MC5_MISC_ERR_REPORT_QEMU)
		&& ((rdmsr(MSR_IA32_MCG_STATUS) & ~MCG_STATUS_LMCES) == (MCG_STATUS_ERR_REPORT_QEMU & ~MCG_STATUS_LMCES))) {

		report("%s", 1, __FUNCTION__);
	} else {
		printf("%lx %lx %lx %lx \n", rdmsr(MSR_IA32_MCx_STATUS(5)), rdmsr(MSR_IA32_MCx_ADDR(5)), rdmsr(MSR_IA32_MCx_MISC(5)), rdmsr(MSR_IA32_MCG_STATUS));
		report("%s", 0, __FUNCTION__);
	}

	/* Restore the env */
	write_cr4(cr4);
}
#elif defined(IN_NATIVE)
static void init_mce_env_on_native()
{
	/* Check the support of MCA/MCE */
	if (!check_mce_mca_capability())
	{
		printf("Platform is not support MCE/MCA, return!! \n");
		return;
	}

	/* IA32_MCG_CAP.MCG_CMCI_P[bit 10] == 1 */
	if ((rdmsr(MSR_IA32_MCG_CAP) & MCG_CMCI_P) != MCG_CMCI_P) {
		printf("MCG_CMCI_P is not set, return!! \n");
		return;
	}
}
/*
 * @brief case name: MCA_ACRN_T9146_check_IA32_MCG_CAP_support_for_native_001
 *
 *Summary:
 *	 MSR IA32_MCG_CAP shall be available on physical platform.
 *	 MSR IA32_MCG_CAP is rad-only, read normal, write GP.
 *
 */
static void MCA_ACRN_T9146_check_IA32_MCG_CAP_support_for_native_001(void)
{

	u64 val;

	/* Reading MSR_IA32_MCG_CAP, IA32_MCG_CAP MSR is a read-only register */
	if (rdmsr_checking(MSR_IA32_MCG_CAP, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* Writting MSR_IA32_MCG_CAP */
	if (wrmsr_checking(MSR_IA32_MCG_CAP, 0x1) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9779_check_IA32_MCG_STATUS_support_for_native_002
 *
 *Summary:
 *	 MSR IA32_MCG_STATUS shall be available on physical platform.
 *	 MSR IA32_MCG_STATUS is read/write0, read/write0 normal, write non-zero inject #GP.
 *
 */
static void MCA_ACRN_T9779_check_IA32_MCG_STATUS_support_for_native_002(void)
{

	u64 val;

	/* Reading MSR_IA32_MCG_STATUS */
	if (rdmsr_checking(MSR_IA32_MCG_STATUS, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* Writting MSR_IA32_MCG_STATUS */
	val = 0;
	if (wrmsr_checking(MSR_IA32_MCG_STATUS, val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	val = 1;
	if (wrmsr_checking(MSR_IA32_MCG_STATUS, val) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* Ignore restore the env here, because writting non-zero will inject #GP */

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9781_check_IA32_P5_MC_ADDR_support_for_native_003
 *
 *Summary:
 *	 MSR IA32_P5_MC_ADDR shall be available on physical platform.
 *	 MSR IA32_P5_MC_ADDR is read/write.
 *
 */
static void MCA_ACRN_T9781_check_IA32_P5_MC_ADDR_support_for_native_003(void)
{

	u64 val;

	/* Reading MSR_IA32_P5_MC_ADDR */
	if (rdmsr_checking(MSR_IA32_P5_MC_ADDR, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	/* Writting MSR_IA32_P5_MC_ADDR */
	if (wrmsr_checking(MSR_IA32_P5_MC_ADDR, 0x1) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	if (wrmsr_checking(MSR_IA32_P5_MC_ADDR, 0x0) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* Restore default value */
	wrmsr(MSR_IA32_P5_MC_ADDR, val);

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9798_check_IA32_P5_MC_TYPE_support_for_native_004
 *
 *Summary:
 *	 MSR IA32_P5_MC_TYPE shall be available on physical platform.
 *	 MSR IA32_P5_MC_TYPE is read/write.
 *
 */
static void MCA_ACRN_T9798_check_IA32_P5_MC_TYPE_support_for_native_004(void)
{

	u64 val;

	/* Reading MSR_IA32_P5_MC_TYPE */
	if (rdmsr_checking(MSR_IA32_P5_MC_TYPE, &val) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	/*
	Writting zero to MSR_IA32_P5_MC_TYPE, only write zero, because
	this MSR is out of date for new platform.
	*/
	if (wrmsr_checking(MSR_IA32_P5_MC_TYPE, 0x0) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	/* Restore default value */
	wrmsr(MSR_IA32_P5_MC_TYPE, val);

	report("%s", 1, __FUNCTION__);

}
/*
 * @brief case name: MCA_ACRN_T9143_physical_error_reporting_register_suppor_for_native_001
 *
 *Summary:
 *	 Error-Reporting Register Banks MSRs shall be available on physical platform.
 *
 */
static void MCA_ACRN_T9143_physical_error_reporting_register_suppor_for_native_001(void)
{
	u64 pre_val;
	u64 val;
	int i;

	/* Check the support of MCA/MCE */
	if (!check_mce_mca_capability())
	{
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* IA32_MCG_CAP.MCG_CMCI_P[bit 10] == 1 */
	if ((rdmsr(MSR_IA32_MCG_CAP) & MCG_CMCI_P) != MCG_CMCI_P) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	for (i = 0; i < bank_num; i++) {

		/* Read MSR IA32_MCi_CTL */
		if (rdmsr_checking(MSR_IA32_MCx_CTL(i), &val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* Write MSR IA32_MCi_CTL */
		if (wrmsr_checking(MSR_IA32_MCx_CTL(i), ~(0x0)) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* Restore default value */
		wrmsr(MSR_IA32_MCx_CTL(i), val);

		/* Read/write MSR_IA32_MCx_CTL2 */
		val = rdmsr(MSR_IA32_MCx_CTL2(i));
		/*
		Write IA32_MCi_CTL2[bits 30] = 1 without exception
		If CMCI interface is not supported for a particular bank
		(but IA32_MCG_CAP[10] = 1), this bit is writeable but
		will always return 0 for that bank.
		*/
		if ((wrmsr_checking(MSR_IA32_MCx_CTL2(i), val | CMCI_EN)) == GP_VECTOR) {
			report("%s: line[%d] i=%d", 0, __FUNCTION__, __LINE__, i);
			return;
		}

		/* Write IA32_MCi_CTL2[bits 14:0] with value in [14:0] without exception */
		val = rdmsr(MSR_IA32_MCx_CTL2(i)) & 0x7FFF;
		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* Write reserved bit, like bit[63] will inject #GP */
		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), (pre_val | (1ULL << 63))) != GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		/* Restore default value */
		wrmsr(MSR_IA32_MCx_CTL2(i), val);

		if (rdmsr_checking(MSR_IA32_MCx_STATUS(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) == GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

				if (rdmsr_checking(MSR_IA32_MCx_ADDR(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) == GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

				if (rdmsr_checking(MSR_IA32_MCx_MISC(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if (wrmsr_checking(MSR_IA32_MCx_MISC(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}


		val = 1;
		if (wrmsr_checking(MSR_IA32_MCx_MISC(i), val) != GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9147_CR4_MCE_support_for_native_002
 *
 *Summary:
 *	 Setting CR4.MCE shall be available on physical platform.
 *
 */
static void MCA_ACRN_T9147_CR4_MCE_support_for_native_002(void)
{
	u64 cr4;

	/* Write cr4.mce = 1 without exception */
	cr4 = read_cr4();
	write_cr4(cr4 | CR4_MCE);
	if ((read_cr4() & CR4_MCE) != CR4_MCE) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	/* restore CR4 */
	write_cr4(cr4);

	report("%s", 1, __FUNCTION__);
}
#else
#ifdef IN_SAFETY_VM

/*
 * @brief case name: MCA_ACRN_T9917_expose_IA32_MCG_CAP_for_safety_VM_001
 *
 *Summary:
 *	 Hyperviser should expose Count (IA32_MCG_CAP[7:0]), MCG_CMCI_P (IA32_MCG_CAP[10]),
 *       MCG_TES_P (IA32_MCG_CAP[11]), MCG_SER_P (IA32_MCG_CAP[24]) to safety-VM.
 */
static void MCA_ACRN_T9917_expose_IA32_MCG_CAP_for_safety_VM_001(void)
{
	u64 mcg_cap;
	mcg_cap = rdmsr(MSR_IA32_MCG_CAP);

	if((mcg_cap & MCG_BANKCNT_MASK) != (IA32_MCG_CAP & MCG_BANKCNT_MASK)) {
		report("%s", 0, __FUNCTION__);
	}

	if((mcg_cap & MCG_CMCI_P) != (IA32_MCG_CAP & MCG_CMCI_P)) {
		report("%s", 0, __FUNCTION__);
	}

	if((mcg_cap & MCG_TES_P) != (IA32_MCG_CAP & MCG_TES_P)) {
		report("%s", 0, __FUNCTION__);
	}
	if((mcg_cap & MCG_SER_P) != (IA32_MCG_CAP & MCG_SER_P)) {
		report("%s", 0, __FUNCTION__);
	}

	report("%s", 1, __FUNCTION__);
}

/*
 *@brief case name: MCA_ACRN_T9136_hide_invalid_Error_Reporting_register_bank_MSRs_for_safety_VM_001
 *
 *Summary:
 *	When a vCPU of safety VM attempts to access any guest invalid Error-Reporting register bank MSRs,
 *	ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 *
 *	Read/write invalid IA32_MCi_CTL/IA32_MCi_STATUS/IA32_MCi_ADDR/IA32_MCi_MISC/IA32_MCi_CTL2 register,
 *      ACRN hypervisor shall generate #GP(0).
 *
 */
static void MCA_ACRN_T9136_hide_invalid_Error_Reporting_register_bank_MSRs_for_safety_VM_001(void)
{
	u64 val;
	int idx = (rdmsr(MSR_IA32_MCG_CAP) & 0xff);

	/* access IA32_MCi_CTL(i==IA32_MCG_CAP[0:7]), shoud generate #GP for safety VM */
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

	/* access IA32_MCi_STATUS(i==IA32_MCG_CAP[0:7]), shoud generate #GP for safety VM */
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

	/* access IA32_MCi_ADDR(i==IA32_MCG_CAP[0:7]), shoud generate #GP for safety VM */
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

	/* access IA32_MCi_MISC(i==IA32_MCG_CAP[0:7]), shoud generate #GP for safety VM */
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

	/* access IA32_MCi_CTL2(i==IA32_MCG_CAP[0:7]), shoud generate #GP for safety VM */
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
 * @brief case name: MCA_ACRN_T9913_expose_MCE_MCA_for_safety_VM_002
 *
 *Summary:
 *	ACRN hypervisor shall expose Machine-check exception and machine-check architecture to safety VM.
 *
 */
static void MCA_ACRN_T9913_expose_MCE_MCA_for_safety_VM_002(void)
{
	bool ret = true;

	ret = check_mce_mca_capability();

	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9109_expose_MCE_control_for_safety_VM_003
 *
 *Summary:
 *	ACRN hypervisor shall expose Machine-check exception control to safety VM.
 *	Guest would be able to read/write CR4.MCE.
 *
 */
static void MCA_ACRN_T9109_expose_MCE_control_for_safety_VM_003(void)
{
	bool ret = true;
	unsigned long cr4 = read_cr4();

	/* write 0H to CR4.MCE and check */
	write_cr4(cr4 & (~CR4_MCE));
	if ((read_cr4() & CR4_MCE) != 0) {

		ret = false;
		report("%s [%d]", ret, __FUNCTION__, __LINE__);
		return;
	}

	/* write 1H to CR4.MCE and check */
	write_cr4((read_cr4() | CR4_MCE));
	if ((read_cr4() & CR4_MCE) != CR4_MCE) {

		ret = false;
		report("%s [%d]", ret, __FUNCTION__, __LINE__);
		return;
	}

	/* restore cr4 */
	write_cr4(cr4 | CR4_MCE);
	report("%s", ret, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9914_expose_IA32_MCi_STATUS_for_safety_VM_004
 *
 *Summary:
 *  ACRN hypervisor expose guest MSR IA32_MCi_STATUS to safety VM.
 *
 */
static void MCA_ACRN_T9914_expose_IA32_MCi_STATUS_for_safety_VM_004(void)
{
	int i;
	u64 pre_val;
	u64 val;

	for (i = 0; i < bank_num; i++) {

		if (rdmsr_checking(MSR_IA32_MCx_STATUS(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) == GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_STATUS(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9915_expose_IA32_MCi_ADDR_for_safety_VM_005
 *
 *Summary:
 *  ACRN hypervisor expose MSR IA32_MCi_ADDR to safety VM.
 *
 */
static void MCA_ACRN_T9915_expose_IA32_MCi_ADDR_for_safety_VM_005(void)
{
	int i;
	u64 pre_val;
	u64 val;

	for (i = 0; i < bank_num; i++) {

		if (rdmsr_checking(MSR_IA32_MCx_ADDR(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_ADDR(i), val) == GP_VECTOR) || (exception_error_code() != 0)) {
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
 * @brief case name: MCA_ACRN_T9916_expose_IA32_MCi_MISC_for_safety_VM_006
 *
 *Summary:
 *  ACRN hypervisor expose MSR IA32_MCi_MISC to safety VM.
 *
 */
static void MCA_ACRN_T9916_expose_IA32_MCi_MISC_for_safety_VM_006(void)
{
	int i;
	u64 pre_val;
	u64 val;


	for (i = 0; i < bank_num; i++) {

		if (rdmsr_checking(MSR_IA32_MCx_MISC(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if (wrmsr_checking(MSR_IA32_MCx_MISC(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 1;
		if (wrmsr_checking(MSR_IA32_MCx_MISC(i), val) != GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T9934_expose_MSR_IA32_MCG_STATUS_for_safety_VM_001
 *
 *Summary:
 *	 Implemented bits of safety-VM IA32_MCG_STATUS same as host except bit 3 is hidden for safety-VM.
 *
 * 	An attempt to write to IA32_MCG_STATUS with any value other than 0 would result in #GP.
 */
static void MCA_ACRN_T9934_expose_MSR_IA32_MCG_STATUS_for_safety_VM_001(void)
{
	u64 pre_val;
	u64 val;

	/* accessing MSR_IA32_MCG_STATUS */
	if ((rdmsr_checking(MSR_IA32_MCG_STATUS, &pre_val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	val = 0;
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	val = 1;
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, val) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	wrmsr(MSR_IA32_MCG_STATUS, pre_val);

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MCA_ACRN_T10000_expose_IA32_MCi_CTL2_for_safety_VM_001
 *
 *Summary:
 *	 ACRN hypervisor shall expose MSR IA32_MCi_CTL2 to safety VM.
 *
 */
static void MCA_ACRN_T10000_expose_IA32_MCi_CTL2_for_safety_VM_001(void)
{
	int i;
	u64 val;
	u64 pre_val;

	for (i = 0; i < bank_num; i++) {
		if (rdmsr_checking(MSR_IA32_MCx_CTL2(i), &pre_val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
		val = (pre_val | CMCI_EN | 0x7FFF); /* Fill 1H to 0:14, enable CMCI_EN */

		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
		if (rdmsr(MSR_IA32_MCx_CTL2(i)) != IA32_MCi_CTL2[i]) {
			report("%s: line[%d] val= 0x%lx, %d", 0, __FUNCTION__, __LINE__, rdmsr(MSR_IA32_MCx_CTL2(i)), i);
			return;
		}
		/* Write reserved bit, like bit[63] will inject #GP */
		if (wrmsr_checking(MSR_IA32_MCx_CTL2(i), (pre_val | (1ULL << 63))) != GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		wrmsr(MSR_IA32_MCx_CTL2(i), pre_val);
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10046_expose_IA32_MCi_CTL_for_safety_VM_002
 *
 *Summary:
 *	ACRN hypervisor shall expose MSR IA32_MCi_CTL to safety VM.
 *
 */
static void MCA_ACRN_T10046_expose_IA32_MCi_CTL_for_safety_VM_002(void)
{
	int i = 0;

	for (i = 0; i < bank_num; i++) {

		if (wrmsr_checking(MSR_IA32_MCx_CTL(i), ~0x0) == GP_VECTOR) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
		if (rdmsr(MSR_IA32_MCx_CTL(i)) != IA32_MCi_CTL[i]) {
			report("%s: line[%d] val= 0x%lx", 0, __FUNCTION__, __LINE__, IA32_MCi_CTL[i]);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

static void test_mca_safety_vm(void)
{
	MCA_ACRN_T9917_expose_IA32_MCG_CAP_for_safety_VM_001();
	MCA_ACRN_T9136_hide_invalid_Error_Reporting_register_bank_MSRs_for_safety_VM_001();
	MCA_ACRN_T9913_expose_MCE_MCA_for_safety_VM_002();
	MCA_ACRN_T9109_expose_MCE_control_for_safety_VM_003();
	MCA_ACRN_T9914_expose_IA32_MCi_STATUS_for_safety_VM_004();
	MCA_ACRN_T9915_expose_IA32_MCi_ADDR_for_safety_VM_005();
	MCA_ACRN_T9916_expose_IA32_MCi_MISC_for_safety_VM_006();
	MCA_ACRN_T9934_expose_MSR_IA32_MCG_STATUS_for_safety_VM_001();
	MCA_ACRN_T10000_expose_IA32_MCi_CTL2_for_safety_VM_001();
	MCA_ACRN_T10046_expose_IA32_MCi_CTL_for_safety_VM_002();
}
#endif
#ifdef IN_NON_SAFETY_VM

/*
 * @brief case name: MCA_ACRN_T10049_read_IA32_MCG_CAP_for_non_safety_VM_002
 *
 *Summary:
 *	 When a vCPU of Non-safety VM attempts to read guest IA32_MCG_CAP, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_ACRN_T10049_read_IA32_MCG_CAP_for_non_safety_VM_002(void)
{
	if(rdmsr(MSR_IA32_MCG_CAP) != 0) {
		report("%s", 0, __FUNCTION__);
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10050_read_IA32_MCG_STATUS_for_non_safety_VM_001
 *
 *Summary:
 *	When a vCPU of Non-safety VM attempts to read guest IA32_MCG_STATUS, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_ACRN_T10050_read_IA32_MCG_STATUS_for_non_safety_VM_001(void)
{
	if(rdmsr(MSR_IA32_MCG_STATUS) != 0) {
		report("%s", 0, __FUNCTION__);
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10072_write_non_zero_to_IA32_MCG_STATUS_for_non_safety_VM_002
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to write non-zero to guest MSR IA32_MCG_STATUS, ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10072_write_non_zero_to_IA32_MCG_STATUS_for_non_safety_VM_002(void)
{
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, 1) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10089_write_zero_to_IA32_MCG_STATUS_for_non_safety_VM_001
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to write non-zero to guest MSR IA32_MCG_STATUS, ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10089_write_zero_to_IA32_MCG_STATUS_for_non_safety_VM_001(void)
{
	if ((wrmsr_checking(MSR_IA32_MCG_STATUS, 0) == GP_VECTOR)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MCA_ACRN_T9124_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001
 *
 *Summary:
 *	ACRN hypervisor shall hide general machine-check architecture from non-safety VM.
 *
 */
static void MCA_ACRN_T9124_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001(void)
{

	bool ret = true;

	ret = ((cpuid(1).d & CPUID_1_EDX_MCA) != CPUID_1_EDX_MCA) &&
		((cpuid(1).d & CPUID_1_EDX_MCE) != CPUID_1_EDX_MCE);

	report("%s", (ret == true ? 1 : 0), __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10210_hide_MSR_IA32_MCx_CTL_for_non_safety_VM_007
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest IA32_MCx_CTL,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10210_hide_MSR_IA32_MCx_CTL_for_non_safety_VM_007(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_CTL, shoud generate #GP for non-safety VM */
	for (i = 0; i <= bank_num; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_CTL(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10202_hide_MSR_IA32_MCx_STATUS_for_non_safety_VM_006
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest IA32_MCx_STATUS,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10202_hide_MSR_IA32_MCx_STATUS_for_non_safety_VM_006(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_STATUS, shoud generate #GP for non-satety VM */
	for (i = 0; i <= bank_num; i++) {
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

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10196_hide_MSR_IA32_MCx_ADDR_for_non_safety_VM_005
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest IA32_MCx_ADDR,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10196_hide_MSR_IA32_MCx_ADDR_for_non_safety_VM_005(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_ADDR, shoud generate #GP for non-safety VM */
	for (i = 0; i <= bank_num; i++) {
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
 * @brief case name: MCA_ACRN_T10187_hide_MSR_IA32_MCx_MISC_for_non_safety_VM_004
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest IA32_MCx_MISC,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10187_hide_MSR_IA32_MCx_MISC_for_non_safety_VM_004(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_MISC, shoud generate #GP for non-safety VM */
	for (i = 0; i <= bank_num; i++) {
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

/*
 * @brief case name: MCA_ACRN_T10160_hide_MSR_IA32_MCx_CTL2_for_non_safety_VM_003
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to access any guest IA32_MCx_CTL2,
 *      ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 */
static void MCA_ACRN_T10160_hide_MSR_IA32_MCx_CTL2_for_non_safety_VM_003(void)
{
	int i;
	u64 val;

	/* Read/Write IA32_MCi_CTL2, shoud generate #GP for non-safety VM */
	for (i = 0; i <= bank_num; i++) {
		if ((rdmsr_checking(MSR_IA32_MCx_CTL2(i), &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}

		val = 0;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
		val = 1;
		if ((wrmsr_checking(MSR_IA32_MCx_CTL2(i), val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: MCA_ACRN_T10159_read_CR4_MCE_bit_for_non_safety_VM_002
 *
 *Summary:
 *	When a vCPU of non-safety VM attempts to read guest CR4.MCE,
 *      ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 */
static void MCA_ACRN_T10159_read_CR4_MCE_bit_for_non_safety_VM_002(void)
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
static void MCA_ACRN_T9120_write_0H_to_CR4_MCE_for_non_safety_VM_001(void)
{
	bool ret = true;
	unsigned long cr4;

	cr4 = read_cr4();
	/* write 0H to CR4.MCE and check */
	if (write_cr4_checking(cr4 & (~CR4_MCE)) == GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	cr4 = read_cr4();

	if ((cr4 & CR4_MCE) != 0) {

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
static void MCA_ACRN_T9121_write_1H_to_CR4_MCE_for_non_safety_VM_001(void)
{
	unsigned long cr4 = read_cr4();

	/* write 1H to CR4.MCE and check */
	if ((write_cr4_checking(cr4 | CR4_MCE) != GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}
static void test_mca_non_safety_vm(void)
{
	MCA_ACRN_T10049_read_IA32_MCG_CAP_for_non_safety_VM_002();
	MCA_ACRN_T10050_read_IA32_MCG_STATUS_for_non_safety_VM_001();
	MCA_ACRN_T10072_write_non_zero_to_IA32_MCG_STATUS_for_non_safety_VM_002();
	MCA_ACRN_T10089_write_zero_to_IA32_MCG_STATUS_for_non_safety_VM_001();
	MCA_ACRN_T9124_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001();
	MCA_ACRN_T10210_hide_MSR_IA32_MCx_CTL_for_non_safety_VM_007();
	MCA_ACRN_T10202_hide_MSR_IA32_MCx_STATUS_for_non_safety_VM_006();
	MCA_ACRN_T10196_hide_MSR_IA32_MCx_ADDR_for_non_safety_VM_005();
	MCA_ACRN_T10187_hide_MSR_IA32_MCx_MISC_for_non_safety_VM_004();
	MCA_ACRN_T10160_hide_MSR_IA32_MCx_CTL2_for_non_safety_VM_003();
	MCA_ACRN_T10159_read_CR4_MCE_bit_for_non_safety_VM_002();
	MCA_ACRN_T9120_write_0H_to_CR4_MCE_for_non_safety_VM_001();
	MCA_ACRN_T9121_write_1H_to_CR4_MCE_for_non_safety_VM_001();
}
#endif

/*
 * @brief case name: MCA_ACRN_T10305_read_IA32_P5_MC_TYPE_001
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_TYPE to 0.
 *
 */
static void MCA_ACRN_T10305_read_IA32_P5_MC_TYPE_001(void)
{
	u64 val;

	if (rdmsr_checking(MSR_IA32_P5_MC_TYPE, &val) != GP_VECTOR && val == 0) {
		report("%s", 1, __FUNCTION__);
		return;
	}

	report("%s, MSR_IA32_P5_MC_TYPE: %lu", 0, __FUNCTION__, val);
}

/*
 * @brief case name: MCA_ACRN_T10319_read_IA32_P5_MC_ADDR_002
 *
 *Summary:
 *	 ACRN hypervisor shall set initial guest MSR IA32_P5_MC_ADDR to 0.
 *
 */
static void MCA_ACRN_T10319_read_IA32_P5_MC_ADDR_002(void)
{
	u64 val;

	if (rdmsr_checking(MSR_IA32_P5_MC_ADDR, &val) != GP_VECTOR && val == 0) {
		report("%s", 1, __FUNCTION__);
		return;
	}

	report("%s, MSR_IA32_P5_MC_ADDR: %lu", 0, __FUNCTION__, val);
}

/*
 * @brief case name: MCA_ACRN_T10897_expose_MSR_IA32_MCG_CAP_001
 *
 *Summary:
 *	 ACRN hypervisor shall expose MSR_IA32_MCG_CAP to any VM.
 *
 * 	Read MSR IA32_MCG_CAP shouldn't generate any exception, this MSR is read-ony.
 */
static void MCA_ACRN_T10897_expose_MSR_IA32_MCG_CAP_001(void)
{
	u64 val;

	/* accessing MSR_IA32_MCG_CAP */
	if ((rdmsr_checking(MSR_IA32_MCG_CAP, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10920_expose_MSR_IA32_MCG_STATUS_002
 *
 *Summary:
 *	 ACRN hypervisor shall expose MSR_IA32_MCG_STATUS to any VM.
 *
 * 	Read/Write0 MSR IA32_MCG_STATUS is normal operation, write non-zero should generate #GP.
 */
static void MCA_ACRN_T10920_expose_MSR_IA32_MCG_STATUS_002(void)
{
	u64 val;

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

	val = 1;
	if (wrmsr_checking(MSR_IA32_MCG_STATUS, val) != GP_VECTOR) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	/* Write none-zero will inject #GP, so ignore restore the env here. */

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10935_expose_MSR_IA32_P5_MC_ADDR_003
 *
 *Summary:
 *	 ACRN hypervisor shall expose MSR_IA32_P5_MC_ADDR to any VM.
 *
 * 	Read MSR MSR_IA32_P5_MC_ADDR should generate any exception, this MSR is read-only.
 */
static void MCA_ACRN_T10935_expose_MSR_IA32_P5_MC_ADDR_003(void)
{
	u64 val;

	val = 0;
	/* accessing MSR_IA32_P5_MC_ADDR, read-only*/
	if ((rdmsr_checking(MSR_IA32_P5_MC_ADDR, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10950_expose_MSR_IA32_P5_MC_TYPE_004
 *
 *Summary:
 *	 ACRN hypervisor shall exposeMSR_IA32_P5_MC_TYPE to any VM.
 *
 * 	Read MSR MSR_IA32_P5_MC_TYPE should generate any exception, this MSR is read-only.
 */
static void MCA_ACRN_T10950_expose_MSR_IA32_P5_MC_TYPE_004(void)
{
	u64 val;

	val = 0;
	/* accessing MSR_IA32_P5_MC_TYPE, read-only*/
	if ((rdmsr_checking(MSR_IA32_P5_MC_TYPE, &val) == GP_VECTOR) || (exception_error_code() != 0)) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	report("%s", 1, __FUNCTION__);
}
/**
 * @brief case name: MCA_ACRN_T10335_inject_GP_when_wring_IA32_P5_MC_Type_001
 *
 * Summary:
 * 	Injects a general protection (GP) exception when writing to the IA32_P5_MC_TYPE MSR.
 */
static void MCA_ACRN_T10335_inject_GP_when_wring_IA32_P5_MC_Type_001(void)
{

	if (wrmsr_checking(MSR_IA32_P5_MC_TYPE, 0x0) != GP_VECTOR || exception_error_code() != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	if (wrmsr_checking(MSR_IA32_P5_MC_TYPE, 0x1) != GP_VECTOR || exception_error_code() != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}
	report("%s", 1, __FUNCTION__);
}

/**
 * @brief case name: MCA_ACRN_T10336_inject_GP_when_wring_IA32_P5_MC_ADDR_002
 *
 * Summary:
 * 	Injects a general protection (GP) exception when writing to the IA32_P5_MC_ADDR MSR.
 */
static void MCA_ACRN_T10336_inject_GP_when_wring_IA32_P5_MC_ADDR_002(void)
{

	if (wrmsr_checking(MSR_IA32_P5_MC_ADDR, 0x0) != GP_VECTOR || exception_error_code() != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	if (wrmsr_checking(MSR_IA32_P5_MC_ADDR, 0x1) != GP_VECTOR || exception_error_code() != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}
	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: MCA_ACRN_T10379_hide_IA32_MCG_CAP_001
 *
 *Summary:
 *	 Hide MCG_CTL_P (IA32_MCG_CAP[8]), MCG_EXT_P (IA32_MCG_CAP[9]) and
 *       MCG_EXT_CNT (IA32_MCG_CAP[23:16]), MCG_EMC_P (IA32_MCG_CAP[25]), 
 *       MCG_ELOG_P  (IA32_MCG_CAP[26]),  MCG_LMCE_P (IA32_MCG_CAP[27]), these bits should be 0.
 */
static void MCA_ACRN_T10379_hide_IA32_MCG_CAP_001(void)
{
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_MCG_STATUS);
	if ((val & MCG_EXT_P) != 0 || (val & MCG_CTL_P) !=0 ||
	     (val & MCG_ELOG_P) != 0 || (val & MCG_LMCE_P) != 0 ||
	     (val & MCG_EXT_CNT_MASK) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}
	report("%s: line[%d]", 1, __FUNCTION__, __LINE__);
}
/*
 * @brief case name: MCA_ACRN_T10396_hide_mcg_ext_p_001
 *
 *Summary:
 *	 An attempt to access to IA32_MCG Extended Machine Check State MSRs would result in #GP(0)
 *
 *  MCG_EXT_P (IA32_MCG_CAP[9]) and MCG_EXT_CNT (IA32_MCG_CAP[23-16]) Capabilities are hided from VM..
 */
static void MCA_ACRN_T10396_hide_mcg_ext_p_001(void)
{
	u64 val = 0x0;

	val = rdmsr(MSR_IA32_MCG_CAP);
	/* IA32_MCG_CAP.IA32_MCG_CAP[bit 9] == 0 */
	if ((val & MCG_EXT_P) != 0) {
		report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
		return;
	}

	for (u64 start = 0x180; start <= 0x197; start++) {

		/* accessing IA32_MCG Extended Machine Check State MSRs will cause #GP(0) */
		if ((rdmsr_checking(start, &val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d] %lx", 0, __FUNCTION__, __LINE__, start);
			return;
		}

		val = 1;
		if ((wrmsr_checking(start, val) != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s: line[%d]", 0, __FUNCTION__, __LINE__);
			return;
		}
	}

	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: Hide IA32_MCG_CTL
 *
 *Summary:
 *	 ACRN hypervisor shall hide IA32_MCG_CTL MSR from any VM.
 *
 *  IA32_MCG_CAP.MCG_CTL_P[bit 8]= should be 0, and accessing IA32_MCG_CTL will cause #GP(0).
 */
static void MCA_ACRN_T9132_hide_IA32_MCG_CTL_001(void)
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

	val = 1;
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
static void MCA_ACRN_T9135_hide_IA32_MCG_EXT_CTL_001(void)
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
 * @brief case name: write IA32_MCG_CAP
 *
 *Summary:
 *	 When a vCPU of safety and non-safety VM attempts to write MSR IA32_MCG_CAP,
 *   hypervisor shall guarantee that the vCPU receives #GP(0).
 *
 */
static void MCA_ACRN_T9117_write_IA32_MCG_CAP_001(void)
{
	bool ret = true;

	if ((wrmsr_checking(MSR_IA32_MCG_CAP, 0) != GP_VECTOR) ||
		(wrmsr_checking(MSR_IA32_MCG_CAP, 1) != GP_VECTOR) || (exception_error_code() != 0)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}
#endif

static void print_case_list(void)
{
	printf("machine check feature case list:\n\r");
	#if defined(QEMU)
	printf("\t Case ID:%s case name:%s\n\r", "T10048",
	          "MCA_ACRN_T10048_expose_Error_Reporting_Register_for_safety_VM_003");
	#elif defined(IN_NATIVE)
	printf("\t Case ID:%s case name:%s\n\r", "T9146",
	          "MCA_ACRN_T9146_check_IA32_MCG_CAP_support_for_native_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9779",
	          "MCA_ACRN_T9779_check_IA32_MCG_STATUS_support_for_native_002");
	printf("\t Case ID:%s case name:%s\n\r", "T9781",
	          "MCA_ACRN_T9781_check_IA32_P5_MC_ADDR_support_for_native_003");
	printf("\t Case ID:%s case name:%s\n\r", "T9798",
	          "MCA_ACRN_T9798_check_IA32_P5_MC_TYPE_support_for_native_004");
	printf("\t Case ID:%s case name:%s\n\r", "T9143",
	          "MCA_ACRN_T9143_physical_error_reporting_register_suppor_for_native_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9147",
	          "MCA_ACRN_T9147_CR4_MCE_support_for_native_002");
	#else
	#ifdef IN_SAFETY_VM
	printf("\t Case ID:%s case name:%s\n\r", "T9917",
	          "MCA_ACRN_T9917_expose_IA32_MCG_CAP_for_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9136",
	          "MCA_ACRN_T9136_hide_invalid_Error_Reporting_register_bank_MSRs_for_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9913",
	          "MCA_ACRN_T9913_expose_MCE_MCA_for_safety_VM_002");
	printf("\t Case ID:%s case name:%s\n\r", "T9109",
	          "MCA_ACRN_T9109_expose_MCE_control_for_safety_VM_003");
	printf("\t Case ID:%s case name:%s\n\r", "T9914",
	          "MCA_ACRN_T9914_expose_IA32_MCi_STATUS_for_safety_VM_004");
	printf("\t Case ID:%s case name:%s\n\r", "T9915",
	          "MCA_ACRN_T9915_expose_IA32_MCi_ADDR_for_safety_VM_005");
	printf("\t Case ID:%s case name:%s\n\r", "T9916",
	          "MCA_ACRN_T9916_expose_IA32_MCi_MISC_for_safety_VM_006");
	printf("\t Case ID:%s case name:%s\n\r", "T9934",
	          "MCA_ACRN_T9934_expose_MSR_IA32_MCG_STATUS_for_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10000",
	          "MCA_ACRN_T10000_expose_IA32_MCi_CTL2_for_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10046",
	          "MCA_ACRN_T10046_expose_IA32_MCi_CTL_for_safety_VM_002");
	#endif
	#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%s case name:%s\n\r", "T10049",
	          "MCA_ACRN_T10049_read_IA32_MCG_CAP_for_non_safety_VM_002");
	printf("\t Case ID:%s case name:%s\n\r", "T10050",
	          "MCA_ACRN_T10050_read_IA32_MCG_STATUS_for_non_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10072",
	          "MCA_ACRN_T10072_write_non_zero_to_IA32_MCG_STATUS_for_non_safety_VM_002");
	printf("\t Case ID:%s case name:%s\n\r", "T10089",
	          "MCA_ACRN_T10089_write_zero_to_IA32_MCG_STATUS_for_non_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9124",
	          "MCA_ACRN_T9124_hide_MCA_in_CPUID_leaf_01H_for_non_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10210",
	          "MCA_ACRN_T10210_hide_MSR_IA32_MCx_CTL_for_non_safety_VM_007");
	printf("\t Case ID:%s case name:%s\n\r", "T10202",
	          "MCA_ACRN_T10202_hide_MSR_IA32_MCx_STATUS_for_non_safety_VM_006");
	printf("\t Case ID:%s case name:%s\n\r", "T10196",
	          "MCA_ACRN_T10196_hide_MSR_IA32_MCx_ADDR_for_non_safety_VM_005");
	printf("\t Case ID:%s case name:%s\n\r", "T10187",
	          "MCA_ACRN_T10187_hide_MSR_IA32_MCx_MISC_for_non_safety_VM_004");
	printf("\t Case ID:%s case name:%s\n\r", "T10160",
	          "MCA_ACRN_T10160_hide_MSR_IA32_MCx_CTL2_for_non_safety_VM_003");
	printf("\t Case ID:%s case name:%s\n\r", "T10159",
	          "MCA_ACRN_T10159_read_CR4_MCE_bit_for_non_safety_VM_002");
	printf("\t Case ID:%s case name:%s\n\r", "T9120",
	          "MCA_ACRN_T9120_write_0H_to_CR4_MCE_for_non_safety_VM_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9121",
	          "MCA_ACRN_T9121_write_1H_to_CR4_MCE_for_non_safety_VM_001");
	#endif
	printf("\t Case ID:%s case name:%s\n\r", "T10305",
	          "MCA_ACRN_T10305_read_IA32_P5_MC_TYPE_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10319",
	          "MCA_ACRN_T10319_read_IA32_P5_MC_ADDR_002");
	printf("\t Case ID:%s case name:%s\n\r", "T10897",
	          "MCA_ACRN_T10897_expose_MSR_IA32_MCG_CAP_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10920",
	          "MCA_ACRN_T10920_expose_MSR_IA32_MCG_STATUS_002");
	printf("\t Case ID:%s case name:%s\n\r", "T10935",
	          "MCA_ACRN_T10935_expose_MSR_IA32_P5_MC_ADDR_003");
	printf("\t Case ID:%s case name:%s\n\r", "T10950",
	          "MCA_ACRN_T10950_expose_MSR_IA32_P5_MC_TYPE_004");
	printf("\t Case ID:%s case name:%s\n\r", "T10335",
	          "MCA_ACRN_T10335_inject_GP_when_wring_IA32_P5_MC_Type_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10336",
	          "MCA_ACRN_T10336_inject_GP_when_wring_IA32_P5_MC_ADDR_002");
	printf("\t Case ID:%s case name:%s\n\r", "T10379",
	          "MCA_ACRN_T10379_hide_IA32_MCG_CAP_001");
	printf("\t Case ID:%s case name:%s\n\r", "T10396",
	          "MCA_ACRN_T10396_hide_mcg_ext_p_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9132",
	          "MCA_ACRN_T9132_hide_IA32_MCG_CTL_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9135",
	          "MCA_ACRN_T9135_hide_IA32_MCG_EXT_CTL_001");
	printf("\t Case ID:%s case name:%s\n\r", "T9117",
	          "MCA_ACRN_T9117_Write_IA32_MCG_CAP_001");
	#endif
}

int main(void)
{
	setup_idt();
	print_case_list();
#if defined(QEMU)
	MCA_ACRN_T10048_expose_Error_Reporting_Register_for_safety_VM_003();
#elif defined(IN_NATIVE)
	init_mce_env_on_native();

	MCA_ACRN_T9146_check_IA32_MCG_CAP_support_for_native_001();
	MCA_ACRN_T9779_check_IA32_MCG_STATUS_support_for_native_002();
	MCA_ACRN_T9781_check_IA32_P5_MC_ADDR_support_for_native_003();
	MCA_ACRN_T9798_check_IA32_P5_MC_TYPE_support_for_native_004();
	MCA_ACRN_T9143_physical_error_reporting_register_suppor_for_native_001();
	MCA_ACRN_T9147_CR4_MCE_support_for_native_002();
#else
	#ifdef IN_NON_SAFETY_VM
	test_mca_non_safety_vm();
	#endif
	#ifdef IN_SAFETY_VM
	test_mca_safety_vm();
	#endif
	/*common */
	MCA_ACRN_T10305_read_IA32_P5_MC_TYPE_001();
	MCA_ACRN_T10319_read_IA32_P5_MC_ADDR_002();
	MCA_ACRN_T10897_expose_MSR_IA32_MCG_CAP_001();
	MCA_ACRN_T10920_expose_MSR_IA32_MCG_STATUS_002();
	MCA_ACRN_T10935_expose_MSR_IA32_P5_MC_ADDR_003();
	MCA_ACRN_T10950_expose_MSR_IA32_P5_MC_TYPE_004();
	MCA_ACRN_T10335_inject_GP_when_wring_IA32_P5_MC_Type_001();
	MCA_ACRN_T10336_inject_GP_when_wring_IA32_P5_MC_ADDR_002();
	MCA_ACRN_T10379_hide_IA32_MCG_CAP_001();
	MCA_ACRN_T10396_hide_mcg_ext_p_001();
	MCA_ACRN_T9132_hide_IA32_MCG_CTL_001();
	MCA_ACRN_T9135_hide_IA32_MCG_EXT_CTL_001();
	MCA_ACRN_T9117_write_IA32_MCG_CAP_001();
#endif

	return report_summary();
}
