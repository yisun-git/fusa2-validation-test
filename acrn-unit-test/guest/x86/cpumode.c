/*
 * Test for x86 cpumode
 *
 * Copyright (c) intel, 2020
 *
 * Authors:
 *  wenwumax <wenwux.ma@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "msr.h"
#include "vmalloc.h"
#include "alloc.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"
#include "cpumode.h"

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

__unused static void notify_modify_and_read_init_value(int case_id)
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

__unused static void modify_cr0_pe_init_init_value()
{
	write_cr0(read_cr0() | X86_CR0_PE);
}

__unused static void modify_efer_lme_init_value()
{
	wrmsr(X86_IA32_EFER, rdmsr(X86_IA32_EFER) | (1ull << 8));
}

__unused static void modify_efer_lma_init_value()
{
	wrmsr(X86_IA32_EFER, rdmsr(X86_IA32_EFER) | (1ull << 10));
}

#ifdef __x86_64__
void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 27814:
		fp = modify_cr0_pe_init_init_value;
		ap_init_value_process(fp);
		break;
	case 27816:
		fp = modify_efer_lme_init_value;
		ap_init_value_process(fp);
		break;
	case 27818:
		fp = modify_efer_lma_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#else
void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	while (1) {
		switch (cur_case_id) {
		case 27814:
			fp = modify_cr0_pe_init_init_value;
			ap_init_value_process(fp);
			break;
		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}
#endif

/**
 * @brief Case name: CPU Mode of Operation Forbidden switch back to real address mode_001.
 *
 * Summary: When a vCPU attempts to write CR0.PE, the old CR0.PE is 1 and the new CR0.PE is 0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0). 
 *
 */
void cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001(void)
{
	u32 cr0 = read_cr0();
	if (cr0 & X86_CR0_PE)
	{
		u32 cur_cr0 = cr0 & ~X86_CR0_PE;

		asm volatile(ASM_TRY("1f")
			"mov %0, %%cr0\n\t"
			"1:"
			::"r"(cur_cr0):"memory");
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);

	return;
}


static u32 bp_eax_ia32_efer = 0xffffffff;
void read_bp_startup(void)
{
	//ia32_efer
	asm ("mov (0x8100) ,%%eax\n\t"
		"mov %%eax, %0\n\t"
		: "=m"(bp_eax_ia32_efer));
}


/**
 * @brief Case name: CPU Mode of Operation_INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following startup. 
 *
 */
void cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001(void)
{
	uint32_t efer = 0;

	read_bp_startup();
	efer = bp_eax_ia32_efer;
	report("\t\t %s", (((efer >> 8) & 0x1) == 0), __FUNCTION__);

	return;
}

static u32 ap_eax_cr0 = 0xffffffff;
void read_ap_init(void)
{
	//read cr0
	asm ("mov (0x8200) ,%%eax\n\t"
		"mov %%eax, %0\n\t"
		: "=m"(ap_eax_cr0));
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief Case name: CPU Mode of Operation INITIAL GUEST MODE FOLLOWING INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest CR0.PE to 0H following INIT. 
 *
 */

void cpumode_rqmid_27814_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001(void)
{
	read_ap_init();
	u32 cr0 = ap_eax_cr0;
	bool is_pass = true;

	if ((cr0 & X86_CR0_PE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27814);
	read_ap_init();
	cr0 = ap_eax_cr0;

	if ((cr0 & X86_CR0_PE) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/**
 * @brief Case name: Initial guest IA32_EFER.LME [bit 8] following INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following INIT.
 *
 */

__unused void cpumode_rqmid_27816_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_INIT_001(void)
{

	bool is_pass = true;
	volatile u64 INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;

	if ((INIT_IA32_EFER & (1ULL << 8)) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27816);
	INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;
	if ((INIT_IA32_EFER & (1ULL << 8)) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}


/**
 * @brief Case name: Initial guest IA32_EFER.LMA [bit 10] following INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LMA[bit 10] to 0H following INIT.
 *
 */

__unused void cpumode_rqmid_27818_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_INIT_001(void)
{

	bool is_pass = true;
	volatile u64 INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;

	if ((INIT_IA32_EFER & (1ULL << 10)) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27818);
	INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;
	if ((INIT_IA32_EFER & (1ULL << 10)) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

#endif

static void print_case_list(void)
{
	printf("cpumode feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 28141u, "Forbidden switch back to real address mode_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27815u, "INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27814u, "INITIAL GUEST MODE FOLLOWING INIT_001");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 27816u, " Initial guest IA32_EFER.LME [bit 8] following INIT_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 27818u, " Initial guest IA32_EFER.LME [bit 8] following INIT_001.");
#endif
#endif
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

	cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001();
	cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001();
#ifdef IN_NON_SAFETY_VM
	cpumode_rqmid_27814_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001();
#ifdef __x86_64__
	cpumode_rqmid_27816_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_INIT_001();
	cpumode_rqmid_27818_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_INIT_001();
#endif
#endif

	return report_summary();
}
