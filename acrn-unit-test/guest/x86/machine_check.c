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

u64 rdmsr_checking(u32 index, u64 *result)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f")
				  "rdmsr\n\t" /*rdmsr, 0x0f, 0x32*/
				  "1:"
				  : "=a"(a), "=d"(d) : "c"(index) : "memory");
	*result = a | ((u64)d << 32);
	return exception_vector();
}

int wrmsr_checking(u32 index, u64 val)
{
	u32 a = val, d = val >> 32;
	asm volatile (ASM_TRY("1f")
				  "wrmsr\n\t" /*wrmsr: 0x0f, 0x30*/
				  "1:"
				  : : "a"(a), "d"(d), "c"(index) : "memory");
	return exception_vector();
}

int check_cpuid_1_edx(unsigned int bit)
{
	return (cpuid(1).d & bit) != 0;
}

int check_cpuid_5_ecx(unsigned int bit)
{
	return (cpuid(5).c & bit) != 0;
}

int write_cr4_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
				 "mov %0, %%cr4\n\t"
				 "1:" : : "r" (val));
	return exception_vector();
}

int do_at_ring3(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (USER_DS),
				  [user_cs] "i" (USER_CS),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x20)
				  : "rcx", "rdx");
	return ret;
}
#ifdef IN_SAFETY_VM
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
	for (int i = 0; i <= 9; i++) {
		rdmsr(MSR_IA32_MCx_STATUS(i));
		wrmsr(MSR_IA32_MCx_STATUS(i), 0);
	}
	/*if no exception happen,it will reach here,so we report pass*/
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
 *	Read each IA32_MCi_STATUS register(i>=aH), ACRN hypervisor shall generate #GP(0).
 *
 */
static void MCA_rqmid_27153_hide_invalid_Error_Reporting_register_bank_MSRs_safety_VM_001(void)
{
	u64 val;

	for (int i = 10; i < 29 ; i++) {
		rdmsr_checking(MSR_IA32_MCx_STATUS(i), &val);
		if ((exception_vector() != GP_VECTOR) || (exception_error_code() != 0)) {
			report("%s", 0, __FUNCTION__);
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
	report("%s", (*(uint32_t *)INIT_IA32_FEATURE_CONTROL & IA32_FEATURE_CONTROL_LMCE_ON) == 0, __FUNCTION__);
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
	report("%s", (*(uint32_t *)STARTUP_IA32_FEATURE_CONTROL & IA32_FEATURE_CONTROL_LMCE_ON) == 0, __FUNCTION__);
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
#define MAX_VAID_ERROR_REPORTING_BANK_NUM	(10U)
#define MAX_ERROR_REPORTING_BANK_NUM		(29U)
#define MAX_MCI_CTL2_BANK_NUM				(32U)

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
#endif
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
	u64 val = 0;

	val = rdmsr(MSR_IA32_MCG_CAP);
	report("%s", (val & IA32_MCG_CAP_CTL_P) == 0, __FUNCTION__);
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
 *	 An attempt to write to IA32_MCG_STATUS with any value other
 *	than 0 would result in #GP(0)
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

#ifdef QEMU
static volatile bool handled_mc_ex = false;
static void handle_mc_check(struct ex_regs *regs)
{
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

static void MCA_rqmid_33485_expose_valid_error_reporting_registers_bank_MSRs_safte_vm_004()
{

#define MC5_STATUS_ERR_REPORT_QEMU	0xbe00000000800400ULL
#define MC5_ADDR_ERR_REPORT_QEMU	0xbe00000000800400ULL
#define MC5_MISC_ERR_REPORT_QEMU	0x1
#define MCG_CAP_ERR_REPORT_QEMU		0x900010aULL
#define MCG_STATUS_ERR_REPORT_QEMU	0x2ULL

	handler old;
	u64 cr4;

	cr4 = read_cr4();
	write_cr4(cr4 | CR4_MCE);

	old = handle_exception(MC_VECTOR, handle_mc_check);

	while (!handled_mc_ex);

	if ((rdmsr(MSR_IA32_MCx_STATUS(5)) == MC5_STATUS_ERR_REPORT_QEMU)
		&& (rdmsr(MSR_IA32_MCx_ADDR(5)) == MC5_ADDR_ERR_REPORT_QEMU)
		&& (rdmsr(MSR_IA32_MCx_MISC(5)) == MC5_MISC_ERR_REPORT_QEMU)
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
static void test_mca_safety_vm(void)
{
#ifdef IN_SAFETY_VM
	MCA_rqmid_28168_guarantee_implemented_IA32_MCi_CTL_same_as_host_001();
	MCA_rqmid_27410_expose_valid_Error_Reporting_register_bank_count_safety_VM_001();
	MCA_rqmid_27281_expose_valid_Error_Reporting_register_bank_MSRs_safety_VM_001();
	MCA_rqmid_27153_hide_invalid_Error_Reporting_register_bank_MSRs_safety_VM_001();
#endif
}

static void test_mca_non_safety_vm(void)
{
#ifdef IN_NON_SAFETY_VM
	MCA_rqmid_26099_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_001();
	MCA_rqmid_26101_hide_Error_Reporting_register_bank_MSRs_of_non_safety_VM_002();
	MCA_rqmid_24454_set_IA32_FEATURE_CONTROL_LMCE_ON_following_init_001();
	MCA_rqmid_24456_set_IA32_FEATURE_CONTROL_LMCE_ON_following_startup_001();
#endif
}
void test_mca(void)
{
	test_mca_non_safety_vm();
	test_mca_safety_vm();

	MCA_rqmid_24604_hide_IA32_MCG_CTL_001();
	MCA_rqmid_25084_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_001();
	MCA_rqmid_25242_inject_GP_if_write_IA32_MCG_STATUS_with_non_zero_002();
}
static void print_case_list(void)
{
	printf("machine check feature case list:\n\r");
#ifdef QEMU
	printf("\t Case ID:%d case name:%s\n\r", 33485,
		   "expose valid error reporting registers bank MSRs safte-vm 004");
#else
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 28168,
		   "guarantee implemented IA32 MCi CTL same as host 001");
	printf("\t Case ID:%d case name:%s\n\r", 27410,
		   "expose valid Error Reporting register bank count safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 27281,
		   "expose valid Error Reporting register bank MSRs safety VM 001");
	printf("\t Case ID:%d case name:%s\n\r", 27153,
		   "hide valid Error Reporting register bank MSRs safety VM 001");
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
#endif
	printf("\t Case ID:%d case name:%s\n\r", 24604,
		   "hide IA32 MCG CTL 001");
	printf("\t Case ID:%d case name:%s\n\r", 25084,
		   "inject GP if write IA32 MCG STATUS with non zero 001");
	printf("\t Case ID:%d case name:%s\n\r", 25242,
		   "inject GP if write IA32 MCG STATUS with non zero 002");
#endif

}

int main(void)
{
	setup_idt();
	print_case_list();
#ifdef QEMU
	MCA_rqmid_33485_expose_valid_error_reporting_registers_bank_MSRs_safte_vm_004();
#else
	test_mca();
#endif
	return report_summary();
}

