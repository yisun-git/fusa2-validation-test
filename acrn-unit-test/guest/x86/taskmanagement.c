#include "libcflat.h"
#include "desc.h"
#include "types.h"
#include "apic-defs.h"
#include "apic.h"
#include "isr.h"
#include "processor.h"
#include "vm.h"
#include "vmalloc.h"
#include "atomic.h"
#include "smp.h"
#include "alloc_page.h"
#include "misc.h"
#include "taskmanagement.h"
#include "alloc_page.c"

#ifdef __x86_64__
#include "./64/taskmanagement_fn.c"
#else
#include "./32/taskmanagement_fn.c"
#endif



/**
 * @brief case name:CR0.TS following INIT_001
 *
 * Summary: When BP start-up, send init to AP, save register value after AP receive
 *	     INIT and SIPI
 */
static void task_management_rqmid_23827_ts_init(void)
{
	volatile u16 ap_cr4 = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x7000;
	ap_cr4 = *ptr;

	report("%s", (((ap_cr4 >> 3) & 0x1) == 0), __FUNCTION__);
}

/**
 * @brief case name:CR0.TS following start-up_001
 *
 * Summary: When BP start-up , Save register value before BP executing 1st instruction
 */
static void task_management_rqmid_23826_ts_start_up(void)
{
	volatile u16 bp_cr4 = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x6000;
	bp_cr4 = *ptr;

	report("%s", (((bp_cr4 >> 3) & 0x1) == 0), __FUNCTION__);
}
/**
 * @brief case name:EFLAGS.NT following INIT_001
 *
 * Summary: When BP start-up, send init to AP, save register value after AP receive
 *	     INIT and SIPI
 */
static void task_management_rqmid_23830_nt_init(void)
{
	volatile u16 ap_eflags = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x7004;
	ap_eflags = *ptr;

	report("%s", (((ap_eflags >> 14) & 0x1) == 0), __FUNCTION__);
}

/**
 * @brief case name:EFLAGS.NT following start-up_001
 *
 * Summary: When BP start-up , Save register value before BP executing 1st instruction
 */
static void task_management_rqmid_23829_nt_start_up(void)
{
	volatile u16 bp_eflags = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x6004;
	bp_eflags = *ptr;

	report("%s", (((bp_eflags >> 14) & 0x1) == 0), __FUNCTION__);
}

/**
 * @brief case name:TR selector following INIT_001
 *
 * Summary: When BP start-up, send init to AP, save register value after AP receive
 *	     INIT and SIPI
 */
static void task_management_rqmid_26024_tr_sel_init(void)
{
	volatile u16 ap_tr = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x7008;
	ap_tr = *ptr;

	report("%s", (ap_tr == 0), __FUNCTION__);
}

/**
 * @brief case name:TR selector following start-up_001
 *
 * Summary: When BP start-up , Save register value before BP executing 1st instruction
 */
static void task_management_rqmid_26023_tr_sel_start_up(void)
{
	volatile u16 bp_tr = 0;
	volatile u32 *ptr;

	ptr = (volatile u32 *)0x6008;
	bp_tr = *ptr;

	report("%s", (bp_tr == 0), __FUNCTION__);
}

__attribute__((unused)) static void print_case_list(void)
{
	printf("Task Management feature case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 25205,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_001");
	printf("\t Case ID:%d case name:%s\n\r", 25206,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_002");
	printf("\t Case ID:%d case name:%s\n\r", 25207,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_003");
	printf("\t Case ID:%d case name:%s\n\r", 25208,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_004");
	printf("\t Case ID:%d case name:%s\n\r", 26173,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_005");
	printf("\t Case ID:%d case name:%s\n\r", 25209,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_006");
	printf("\t Case ID:%d case name:%s\n\r", 25210,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_007");
	printf("\t Case ID:%d case name:%s\n\r", 25211,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_008");
	printf("\t Case ID:%d case name:%s\n\r", 26093,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_NMI_009");
	printf("\t Case ID:%d case name:%s\n\r", 24564,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_001");
	printf("\t Case ID:%d case name:%s\n\r", 25087,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_002");
	printf("\t Case ID:%d case name:%s\n\r", 25089,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_003");
	printf("\t Case ID:%d case name:%s\n\r", 25092,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_004");
	printf("\t Case ID:%d case name:%s\n\r", 25093,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_005");
	printf("\t Case ID:%d case name:%s\n\r", 26172,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_006");
	printf("\t Case ID:%d case name:%s\n\r", 25195,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_007");
	printf("\t Case ID:%d case name:%s\n\r", 25196,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_008");
	printf("\t Case ID:%d case name:%s\n\r", 25197,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_009");
	printf("\t Case ID:%d case name:%s\n\r", 26091,
		"Invalid task switch in protected mode_Exception_Checking_on_IDT_by_Software_interrupt_010");
	printf("\t Case ID:%d case name:%s\n\r", 25212,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_001");
	printf("\t Case ID:%d case name:%s\n\r", 25219,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_002");
	printf("\t Case ID:%d case name:%s\n\r", 25221,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_003");
	printf("\t Case ID:%d case name:%s\n\r", 26174,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_004");
	printf("\t Case ID:%d case name:%s\n\r", 25222,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_005");
	printf("\t Case ID:%d case name:%s\n\r", 25223,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_006");
	printf("\t Case ID:%d case name:%s\n\r", 25224,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_007");
	printf("\t Case ID:%d case name:%s\n\r", 26094,
		"Invalid task switch in protected mode_Exception_Checking_on_IRET_008");
	printf("\t Case ID:%d case name:%s\n\r", 24561,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_001");
	printf("\t Case ID:%d case name:%s\n\r", 24613,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_002");
	printf("\t Case ID:%d case name:%s\n\r", 24562,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_003");
	printf("\t Case ID:%d case name:%s\n\r", 24566,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_004");
	printf("\t Case ID:%d case name:%s\n\r", 24562,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_005");
	printf("\t Case ID:%d case name:%s\n\r", 25025,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_006");
	printf("\t Case ID:%d case name:%s\n\r", 26171,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_007");
	printf("\t Case ID:%d case name:%s\n\r", 25026,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008");
	printf("\t Case ID:%d case name:%s\n\r", 25024,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008");
	printf("\t Case ID:%d case name:%s\n\r", 25028,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008");
	printf("\t Case ID:%d case name:%s\n\r", 26061,
		"Invalid task switch in protected mode_Exception_Checking_on_Task_gate_008");
	printf("\t Case ID:%d case name:%s\n\r", 24544,
		"Exception_Checking_on_TSS_selector_001");
	printf("\t Case ID:%d case name:%s\n\r", 24546,
		"Exception_Checking_on_TSS_selector_002");
	printf("\t Case ID:%d case name:%s\n\r", 24545,
		"Exception_Checking_on_TSS_selector_003");
	printf("\t Case ID:%d case name:%s\n\r", 25030,
		"Exception_Checking_on_TSS_selector_004");
	printf("\t Case ID:%d case name:%s\n\r", 25032,
		"Exception_Checking_on_TSS_selector_005");
	printf("\t Case ID:%d case name:%s\n\r", 25034,
		"Exception_Checking_on_TSS_selector_006");
	printf("\t Case ID:%d case name:%s\n\r", 26088,
		"Exception_Checking_on_TSS_selector_007");
	printf("\t Case ID:%d case name:%s\n\r", 23083,
		"guest CR0.TS keeps unchanged in task switch attempts_001");
	printf("\t Case ID:%d case name:%s\n\r", 23835,
		"guest CR0.TS keeps unchanged in task switch attempts_002");
	printf("\t Case ID:%d case name:%s\n\r", 23836,
		"guest CR0.TS keeps unchanged in task switch attempts_003");
	printf("\t Case ID:%d case name:%s\n\r", 24008,
		"guest CR0.TS keeps unchanged in task switch attempts_004");
	printf("\t Case ID:%d case name:%s\n\r", 23821, "Task Management expose CR0.TS_001");
	printf("\t Case ID:%d case name:%s\n\r", 23890, "Task Management expose CR0.TS_002");
	printf("\t Case ID:%d case name:%s\n\r", 23892, "Task Management expose CR0.TS_003");
	printf("\t Case ID:%d case name:%s\n\r", 23893, "Task Management expose CR0.TS_004");
	printf("\t Case ID:%d case name:%s\n\r", 23674, "Task Management expose TR_001");
	printf("\t Case ID:%d case name:%s\n\r", 23680, "Task Management expose TR_002");
	printf("\t Case ID:%d case name:%s\n\r", 23687, "Task Management expose TR_003");
	printf("\t Case ID:%d case name:%s\n\r", 23688, "Task Management expose TR_004");
	printf("\t Case ID:%d case name:%s\n\r", 23689, "Task Management expose TR_005");
	printf("\t Case ID:%d case name:%s\n\r", 23761, "Task switch in protect mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 23762, "Task switch in protect mode_002");
	printf("\t Case ID:%d case name:%s\n\r", 23763, "Task switch in protect mode_003");
	printf("\t Case ID:%d case name:%s\n\r", 24006, "Task switch in protect mode_004");
	printf("\t Case ID:%d case name:%s\n\r", 26076, "Task_Switch_in_IA-32e_mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 26073, "Task_Switch_in_IA-32e_mode_002");
	printf("\t Case ID:%d case name:%s\n\r", 24011, "Task_Switch_in_IA-32e_mode_003");
	printf("\t Case ID:%d case name:%s\n\r", 26256, "Task_Switch_in_IA-32e_mode_004");
	printf("\t Case ID:%d case name:%s\n\r", 24026, "TR Initial value following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 24027, "TR Initial value following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 23827, "CR0.TS following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 23826, "CR0.TS following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 23830, "EFLAGS.NT following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 23829, "EFLAGS.NT following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 26024, "TR selector following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 26023, "TR selector following start-up_001");

}

__attribute__((unused)) static void test_task_management_init_startup(void)
{
	task_management_rqmid_23827_ts_init();
	task_management_rqmid_23826_ts_start_up();
	task_management_rqmid_23830_nt_init();
	task_management_rqmid_23829_nt_start_up();
	task_management_rqmid_26024_tr_sel_init();
	task_management_rqmid_26023_tr_sel_start_up();
}

int main(void)
{
	setup_vm();
	setup_idt();

	//print_case_list();

#ifdef __x86_64__
	setup_tss64();

	test_task_management_64();
#else
	init_do_less_privilege();
	setup_tss32();

	test_task_management_32();
#endif

	return report_summary();
}
