#include "libcflat.h"
#include "smp.h"
#include "apic.h"
#include "desc.h"
#include "processor.h"
#include "misc.h"
#include "delay.h"
#include "debug_print.h"
#include "interrupt.h"
#include "memory_type.h"
#include "isr.h"
#include "mp_initialization.h"

#define MP_APIC_ID_BSP	0
#define MP_APIC_ID_AP	1
#define ONE_SECOND		2100000000ULL

#ifdef __x86_64__

u32 bp_bsp_flag = 0, ap_bsp_flag = 0;

/*
 * ap_is_running and bsp_is_running are using for sync the status between BSP and AP.
 * Considering several round of AP reset in one case, during the period the BSP continutes
 * its process, so there needs a way to let BSP know which test case are running in AP
 * for result check needed.
 */
static volatile int ap_is_running = 0;
static volatile int bsp_is_running = 0;

/* start_run_id is used to identify which test case is running. */
static volatile int start_run_id = 0;

static volatile int wait_bp = 0;
static volatile int wait_ap = 0;
static volatile int ap_in_halt = 0xff;

void ap_sync()
{
	while (wait_bp != 1) {
		debug_print("%d %d\n", wait_ap, wait_bp);
		test_delay(1);
	}
	wait_bp = 0;
}

void bp_sync()
{
	while (wait_ap != 1) {
		debug_print("%d %d\n", wait_ap, wait_bp);
		test_delay(1);
	}
	wait_ap = 0;
}

void bp_sync_timeout(int timeout)
{int count = 0;

	while ((wait_ap != 1) && (count < timeout)) {
		debug_print("%d %d\n", wait_ap, wait_bp);
		test_delay(1);
		count++;
	}
}


void reset_ap(int apic_id)
{
	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, apic_id);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, apic_id);
}

/**
 * @brief Case name:Multiple-Processor Initialization_AP in normal status ignore SIPI_001
 *
 * Summary: no code of SIPI handler would be executed by AP after AP receive SIPI from BSP
 *
 */
static void MP_initialization_rqmid_26995_vCPU_is_AP_in_normal_status_ignore_SIPI(void)
{
	ap_is_running = 0;

	/* send SIPI to AP*/
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);

	/*if ap restart, wait for ap start complete*/
	test_delay(5);

	/*ap can't restart*/
	report("\t\t %s", ap_is_running == 0, __FUNCTION__);
}

/**
 * @brief Case name:Multiple-Processor Initialization_ACRN expose only one BSP to any VM_001
 *
 * Summary: hypervisor expose only one BSP to any VM,only one processor's BSP flag in IA32_APIC_BASE MSR is 1
 *
 */
static void MP_initialization_rqmid_27160_expose_only_one_BSP_to_any_VM_001(void)
{
	start_run_id = 27160;
	bp_bsp_flag = (rdmsr(MSR_IA32_APICBASE) & APIC_BSP) >> 8;

	/*wait ap get ap_bsp_flag*/
	debug_print("%d %d\n", wait_ap, wait_bp);
	bp_sync();

	report("\t\t %s", (ap_bsp_flag == 0) && (bp_bsp_flag != 0), __FUNCTION__);

	/*notify ap end*/
	wait_bp = 1;
}

static void MP_initialization_rqmid_27160_ap(void)
{
	start_run_id = 0;
	ap_bsp_flag = (rdmsr(MSR_IA32_APICBASE) & APIC_BSP) >> 8;

	/*ap exec done*/
	wait_ap = 1;
	ap_sync();
}

/**
 * @brief Case name:Multiple-Processor Initialization_AP in wait-for-SIPI status ignore INIT_001
 *
 * Summary: When the vCPU in wait-for-SIPI status and it is a AP,
 * if there have multi SIPI the AP will execute code at the vector encoded in the first SIPI
 *
 */
static void MP_initialization_rqmid_28469_ap_wait_for_sipi_ignore_init_001(void)
{
	start_run_id = 28469;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send INIT to AP again */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);

	/*wait ap start done*/
	test_delay(5);

	/*ap only start 1 time*/
	report("\t\t %s", ap_is_running == 1, __FUNCTION__);
}

/**
 * @brief Case name:Multiple-Processor Initialization_vCPU is AP in normal status handle INIT_001
 *
 * Summary: When the vCPU in normal status and it is a AP,
 * if it receives the INIT it will enter a Wait-for-SIPI status.
 *
 */
static void MP_initialization_rqmid_33849_ap_in_normal_handle_INIT_001(void)
{
	start_run_id = 33849;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);

	/*wait ap start done*/
	test_delay(5);

	/*ap will restart*/
	report("\t\t %s", ap_is_running == 1, __FUNCTION__);
}

static void MP_initialization_rqmid_26996_ap(void)
{
	start_run_id = 0;
	bsp_is_running = 0;
	debug_print("%d %d\n", wait_ap, wait_bp);

	printf("ap send SIPI to bsp\n");
	/* send SIPI to BSP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_BSP);
	debug_print("%d %d\n", wait_ap, wait_bp);

	test_delay(5);
	wait_ap = 1;
	debug_print("%d %d\n", wait_ap, wait_bp);
}

/**
 * @brief Case name:Multiple-Processor Initialization_BSP in normal status ignore SIPI_001
 *
 * Summary: When the vCPU is BSP and it is in normal status,
 * if it receives the SIPI it ignores SIPI
 *
 */
static void MP_initialization_rqmid_26996_bsp_in_normal_ignore_SIPI_001(void)
{
	start_run_id = 26996;
	wait_ap = 0;

	debug_print("%d %d\n", wait_ap, wait_bp);
	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);

	debug_print("%d %d\n", wait_ap, wait_bp);

	/*wait ap send sipi done*/
	bp_sync();
	debug_print("%d %d\n", wait_ap, wait_bp);
	report("\t\t %s", bsp_is_running == 0, __FUNCTION__);
}

static void MP_initialization_rqmid_33850_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 33850*/
	bsp_is_running = 0;

	printf("ap send INIT to bsp\n");
	/* send INIT to BSP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_BSP);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, MP_APIC_ID_BSP);
	/* send SIPI to BSP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_BSP);

	debug_print("%d %d\n", wait_ap, wait_bp);
	test_delay(5);
	debug_print("%d %d\n", wait_ap, wait_bp);
	wait_ap = 1;
	debug_print("%d %d\n", wait_ap, wait_bp);
}

static void MP_initialization_rqmid_38919_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 38919*/
	bsp_is_running = 0;

	/* send INIT to BSP */
	printf("ap send SIPI to bsp\n");
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_BSP);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, MP_APIC_ID_BSP);
	test_delay(10);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_FIXED | 0xE0, MP_APIC_ID_BSP);
	test_delay(5);
	wait_ap = 1;
}

static void MP_initialization_rqmid_38925_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 38919*/
	bsp_is_running = 0;

	/* send INIT to BSP */
	printf("ap send SIPI to bsp\n");
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_BSP);
	printf("Wait for 15 second and observe bp's print info,if bp has not print info,then we think"
	" that bp ignore SIPI and bp is in halt state\n");
	printf("\n\n");
	test_delay(35);
	printf("send a interrupt to bp to let bp exit halt state\n");
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_FIXED | 0xE0, MP_APIC_ID_BSP);
	wait_ap = 1;
}


static void MP_initialization_rqmid_39037_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 39037*/
	bsp_is_running = 0;

	asm volatile ("hlt");
	wait_ap = 1;
	printf("ap transmit from halt state to normal execution\n");
}

static void MP_initialization_rqmid_39038_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 39038*/
	bsp_is_running = 0;
	wait_ap = 1;
	if (ap_is_running == 1) {
		printf("ap enter halt state\n");
		asm volatile ("hlt");
	}
	else {
		printf("ap exit halt state\n");
	}
	printf("ap receive SIPI and print this message\n");
}

static void MP_initialization_rqmid_39039_ap(void)
{
	start_run_id = 0;
	/*wait for BP to execute to case id 39039*/
	bsp_is_running = 0;
	wait_ap = 1;
	ap_in_halt = 1;
	asm volatile ("hlt");
	ap_in_halt = 0;
}

static void MP_initialization_rqmid_39053_ap(void)
{
	start_run_id = 0;

	wait_ap = 1;
	ap_in_halt = 1;
	asm volatile ("hlt");
	ap_in_halt = 0;
}

static void MP_initialization_ap_common_func(void)
{
	start_run_id = 0;
	printf("ap is in normal execution state.\n");
	wait_ap = 1;
}


/**
 * @brief Case name:Multiple-Processor Initialization_vCPU is BSP in normal status ignore INIT_001
 *
 * Summary: When the vCPU is BSP and it is in normal status,
 * if it receives the INIT it will ignore the IPI
 *
 */
static void MP_initialization_rqmid_33850_bsp_in_normal_ignore_INIT_001(void)
{
	start_run_id = 33850;
	wait_ap = 0;

	debug_print("%d %d\n", wait_ap, wait_bp);
	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);

	debug_print("%d %d\n", wait_ap, wait_bp);
	/*wait ap send INIT done*/
	bp_sync();

	debug_print("%d %d\n", wait_ap, wait_bp);
	report("\t\t %s", bsp_is_running == 0, __FUNCTION__);
}

static void ipi_isr_for_exit_halt_state(isr_regs_t *regs)
{
	printf("Enter ipi_isr_for_exit_halt_state\n");
	eoi();
}


/**
 * @brief Case name:vCPU is BSP in HALT state ignore INIT_001
 *
 * Summary: While the virtual BP of non-safety VM is in the HALT state, when an INIT IPI is sent to the virtual BP
 * by any vCPU of the same VM, ACRN hypervisor shall guarantee the INIT IPI is never delivered.
 *
 */
static void MP_initialization_rqmid_38919_bsp_in_HALT_ignore_INIT_001(void)
{
	start_run_id = 38919;
	wait_ap = 0;

	handle_irq(0xE0, ipi_isr_for_exit_halt_state);
	irq_enable();
	debug_print("%d %d\n", wait_ap, wait_bp);
	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	asm volatile ("hlt");
	bp_sync();

	printf("bp resume !!!!!\n");
	report("\t\t %s", true, __FUNCTION__);
}


static void MP_initialization_ap_isr(isr_regs_t *regs)
{
	printf("Enter interrupt\n");
	eoi();
}


/**
 * @brief Case name:vCPU transits from HALT state to normal execution state_001
 *
 * Summary:(228991) While a vCPU of any VM is in the HALT state, when any interrupt is delivered to the same
 * vCPU by ACRN hypervisor, the ACRN hypevisor shall set the virtual AP to the normal execution state.
 *
 */
static void MP_initialization_rqmid_39037_ap_transits_from_HALT_state_001(void)
{
	start_run_id = 39037;
	wait_ap = 0;

	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	handle_irq(EXTEND_INTERRUPT_E0, MP_initialization_ap_isr);
	irq_enable();
	test_delay(5);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_FIXED | EXTEND_INTERRUPT_E0, MP_APIC_ID_AP);
	bp_sync();
	report("\t\t%s", true, __FUNCTION__);
}

/**
 * @brief Case name:INIT IPI to virtual AP in HALT state_001
 *
 * Summary:(rqid:228987) While a virtual AP of the non-safety VM is in the HALT state, when an INIT IPI is
 * sent to the virtual AP by any vCPU of the same VM, ACRN hypervisor shall set the virtual AP
 * in wait-for-SIPI state.
 *
 */
static void MP_initialization_rqmid_39038_ap_halt_init_wait_sipi_001(void)
{
	int count = 0;
	start_run_id = 39038;
	wait_ap = 0;
	ap_is_running = 0;
	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	bp_sync();

	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, MP_APIC_ID_AP);
	/* send INIT to AP for 3  times */
	for (count = 0; count < 3; count++) {
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, MP_APIC_ID_AP);
		test_delay(2);
		printf("BP is sending INIT IPI to ap(count:%d)\n", count);
	}

	printf("BP sends a SIPI IPI to ap to let ap enter normal execution state\n");
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);
	bp_sync_timeout(5);//if timeout 5 expire, wait_ap var still is 0, BP exit waitting.
	/**/
	report("\t\t%s", wait_ap == 1, __FUNCTION__);
}

/**
 * @brief Case name:SIPI to virtual AP in HALT state_001
 *
 * Summary: While a virtual AP of the non-safety VM is in the HALT state, when a SIPI is sent to the
 * virtual AP by any vCPU of the same VM, ACRN hypervisor shall guarantee that the SIPI is never delivered.
 *
 */
static void MP_initialization_rqmid_39039_ap_halt_sipi_001(void)
{
	start_run_id = 39039;
	wait_ap = 0;

	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	bp_sync();
	test_delay(1);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);
	test_delay(5);

	report("\t\t%s", ap_in_halt == 1, __FUNCTION__);
	/*restore to default 0xFF*/
	ap_in_halt = 0xFF;
}

/**
 * @brief Case name:vCPU transits from normal execution state to HALT state_001
 *
 * Summary: While a vCPU of any VM is in the normal execution state, when a HLT instruction is executed by the
 * same vCPU, The ACRN hypevisor shall set the virtual AP to the HALT state.
 *
 */
static void MP_initialization_rqmid_39053_ap_normal_halt_001(void)
{
	start_run_id = 39053;
	wait_ap = 0;

	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	bp_sync();
	test_delay(5);
	report("\t\t%s", ap_in_halt == 1, __FUNCTION__);
	/*restore to default 0xFF*/
	ap_in_halt = 0xFF;
}


/**
 * @brief Case name:vCPU is BSP in HALT state ignore SIPI_001
 *
 * Summary: While the virtual BP of non-safety VM is in the HALT state, when an INIT IPI is sent to the virtual BP
 * by any vCPU of the same VM, ACRN hypervisor shall guarantee the INIT IPI is never delivered.
 *
 */
static void MP_initialization_rqmid_38925_bsp_in_HALT_ignore_SIPI_001(void)
{
	start_run_id = 38925;
	wait_ap = 0;

	handle_irq(0xE0, ipi_isr_for_exit_halt_state); //register a isr for bp exiting halt state to continue to test.
	irq_enable();
	/*restart ap*/
	reset_ap(MP_APIC_ID_AP);
	asm volatile ("hlt");

	printf("bp is running...\n");
	report("\t\t %s", true, __FUNCTION__);
}

/**
 * @brief Case name:Set virtual AP state to execute the virtual AP code when multiple SIPIs are received_001
 *
 * When the vCPU in wait-for-SIPI status and it is a AP, if there have multi SIPI the AP will execute code at the
 * vector encoded in the first SIPI, in compliance with Chapter 8.4 and 10.6 of volume 3 of SDM
 *
 */
static void MP_initialization_rqmid_26998_execute_init_code_only_first_sipi_001(void)
{
	start_run_id = 26998;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);
	test_delay(5);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);
	report("\t\t %s", ap_is_running == 1, __FUNCTION__);
}


/**
 * @brief Case name:Set virtual AP state to execute the virtual AP code when a SIPI is received_001
 *
 * While a virtual AP of the non-safety VM is in the wait-for-SIPI state, when a SIPI is sent to the virtual
 * AP by any vCPU of the same VM, ACRN hypervisor shall set the virtual AP to normal execution state.
 *
 */
static void MP_initialization_rqmid_26997_execute_AP_code_when_SIPI_is_eceived(void)
{
	start_run_id = 26997;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_AP);
	bp_sync();
	report("\t\t %s", ap_is_running == 1, __FUNCTION__);
}

/**
 * @brief Case name:Set CS selector to execute the virtual AP code when a SIPI is received_001
 *
 * While a virtual AP of the non-safety VM is in the wait-for-SIPI state, when a SIPI is sent to the virtual
 * AP by any vCPU of the same VM, ACRN hypervisor shall set CS selector of this virtual AP to the value
 *of shifting left the vector encoded in SIPI by 8H bits.
 *
 */
static void MP_initialization_rqmid_39396_set_CS_selector_to_a_specific_value_001(void)
{
	u16 cs_selector = 0xFFFF;
	u16 ip;

	start_run_id = 39396;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x2, MP_APIC_ID_AP);
	bp_sync();
	cs_selector = *(u16 *)MEM_ADDR_SAVE_CS_VALUE;
	//printf("cs_selector:%x\n", cs_selector);
/**
 *  objdump -axS -m i8086 mp_initialization.elf
 *
 *  mp_initialization_init.S's disassembly
 *  mov $stacktop, %esp
 *		1000c8:	66 bc 50 3d 11 00	mov    $0x113d50,%esp
 *	call 11f
 *		1000ce:	e8 00 00			call   1000d1 <gdt32_end+0x9>
 *	11:
 *	pop %dx
 *		1000d1:	5a					pop    %dx
 */
	ip = *(u16 *)MEM_ADDR_SAVE_EIP_VLAUE - 9; /*9 (0x1000d1-0x1000c8)is offest to ap's first instruction*/
	report("\t\t %s", (cs_selector == (2 << 8)) && (ip == 0), __FUNCTION__);
}


/**
 * @brief Case name:Set CS selector to execute the virtual AP code when multiple SIPIs are received_001
 *
 * While a virtual AP of the non-safety VM is in the wait-for-SIPI state, when multiple SIPIs are sent to the virtual
 * AP by any vCPU of the same VM, ACRN hypervisor shall set CS selector of this virtual AP to the value of shifting
 * the vector encoded in the first SIPI by 8H bits.
 *
 */
static void MP_initialization_rqmid_39405_execute_AP_code_at_first_sipi_vector(void)
{
	volatile u16 cs_selector = 0xFFFF;

	start_run_id = 39405;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, 1);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x1, MP_APIC_ID_AP);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x2, MP_APIC_ID_AP);

	bp_sync();
	cs_selector = *(volatile u16 *)MEM_ADDR_SAVE_CS_VALUE;
	//printf("cs_selector:%x\n", cs_selector);

	report("\t\t %s", cs_selector == (1 << 8), __FUNCTION__);
}

/**
 * @brief Case name:
 * Set CS base address to execute the virtual AP code when multiple SIPIs are sent to the virtual AP_001
 *
 * Summary: While a virtual AP of the non-safety VM is in the wait-for-SIPI state, when multiple SIPIs are
 *sent to the virtual AP by any vCPU of the same VM, ACRN hypervisor shall set CS base address of this
 *virtual AP to the value of shifting left the vector encoded in the first SIPI by cH bits.
 */
static void MP_initialization_rqmid_39473_execute_AP_code_at_first_sipi_vector(void)
{
	volatile u16 cs_selector = 0xFFFF;
	u16 cs_base;

	start_run_id = 39473;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, 1);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x1, MP_APIC_ID_AP);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x2, MP_APIC_ID_AP);

	bp_sync();
	cs_selector = *(volatile u16 *)MEM_ADDR_SAVE_CS_VALUE;
	cs_base = cs_selector << 4;

	report("\t\t %s", cs_base == (1 << 12), __FUNCTION__);
}

/**
 * @brief Case name:
 *     Set CS base address to execute the virtual AP code when a SIPI is sent to the virtual AP_001
 *
 * Summary:While a virtual AP of the non-safety VM is in the wait-for-SIPI state, when a SIPI is sent to the virtual
 * AP by any vCPU of the same VM, ACRN hypervisor shall set CS base address of this virtual AP to the value of
 * shifting left the vector encoded in SIPI by cH bits.
 */
static void MP_initialization_rqmid_39474_set_CS_base_to_a_specific_value_001(void)
{
	u16 cs_selector = 0xFFFF;
	u16 cs_base;

	start_run_id = 39396;
	wait_ap = 0;
	ap_is_running = 0;

	/* send INIT to AP */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, MP_APIC_ID_AP);
	/* send SIPI to AP */
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP | 0x2, MP_APIC_ID_AP);
	bp_sync();
	cs_selector = *(u16 *)MEM_ADDR_SAVE_CS_VALUE;
	cs_base = cs_selector << 4;
	report("\t\t %s", (cs_base == (2 << 12)), __FUNCTION__);
}


#ifdef IN_SAFETY_VM
/**
 * @brief Case name:ignore INIT for safety VM_001
 *
 * Summary: When an INIT IPI is sent by any vCPU of the safety VM, ACRN hypervisor shall
 * guarantee the INIT IPI is never delivered.
 *
 */
static void MP_initialization_rqmid_38911_ignore_INIT_for_safety_VM_001(void)
{
	apic_icr_write(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_INIT, MP_APIC_ID_BSP);
	report("\t\t %s", true, __FUNCTION__);
}

/**
 * @brief Case name:ignore SIPI for safety VM_001
 *
 * Summary: When a SIPI is sent by any vCPU of the safety VM, ACRN hypervisor shall guarantee
 * the SIPI is never delivered.
 *
 */
static void MP_initialization_rqmid_38912_ignore_SIPI_for_safety_VM_001(void)
{
	apic_icr_write(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_STARTUP, MP_APIC_ID_BSP);
	report("\t\t %s", true, __FUNCTION__);
}
#endif

static void __unused test_MP_list(void)
{
	MP_initialization_rqmid_27160_expose_only_one_BSP_to_any_VM_001();
	MP_initialization_rqmid_26995_vCPU_is_AP_in_normal_status_ignore_SIPI();
	MP_initialization_rqmid_28469_ap_wait_for_sipi_ignore_init_001();
	MP_initialization_rqmid_33849_ap_in_normal_handle_INIT_001();
	MP_initialization_rqmid_26996_bsp_in_normal_ignore_SIPI_001();
	//test_delay(5);
	MP_initialization_rqmid_39037_ap_transits_from_HALT_state_001();//228991
	MP_initialization_rqmid_39038_ap_halt_init_wait_sipi_001();//228987
	MP_initialization_rqmid_39039_ap_halt_sipi_001();//228988
	MP_initialization_rqmid_39053_ap_normal_halt_001();//228990
	MP_initialization_rqmid_26998_execute_init_code_only_first_sipi_001(); //146096
	MP_initialization_rqmid_26997_execute_AP_code_when_SIPI_is_eceived(); //146060
	MP_initialization_rqmid_39396_set_CS_selector_to_a_specific_value_001();
	MP_initialization_rqmid_39405_execute_AP_code_at_first_sipi_vector();
	MP_initialization_rqmid_39473_execute_AP_code_at_first_sipi_vector();
	MP_initialization_rqmid_39474_set_CS_base_to_a_specific_value_001();
	MP_initialization_rqmid_38925_bsp_in_HALT_ignore_SIPI_001();//228992
	MP_initialization_rqmid_38919_bsp_in_HALT_ignore_INIT_001();//228993
	MP_initialization_rqmid_33850_bsp_in_normal_ignore_INIT_001();//146058
}

static void print_case_list(void)
{
	printf("\t\t MP initialization feature case list:\n\r");
	printf("\t\t MP initialization 64-Bits Mode:\n\r");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27160u,
		"Multiple-Processor Initialization_ACRN expose only one BSP to any VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26995u,
		"Multiple-Processor Initialization_AP in normal status ignore SIPI_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28469u,
		"Multiple-Processor Initialization_AP in wait-for-SIPI status ignore INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33849u,
		"Multiple-Processor Initialization_vCPU is AP in normal status handle INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26996u,
		"Multiple-Processor Initialization_BSP in normal status ignore SIPI_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 33850u,
		"Multiple-Processor Initialization_vCPU is BSP in normal status ignore INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39037u,
		"vCPU transits from HALT state to normal execution state_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39038u,
		"INIT IPI to virtual AP in HALT state_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39039u,
		"SIPI to virtual AP in HALT state_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39053u,
		"vCPU transits from normal execution state to HALT state_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26998u,
		"Set virtual AP state to execute the virtual AP code when multiple SIPIs are received_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26997u,
		"Set virtual AP state to execute the virtual AP code when a SIPI is received_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39396u,
		"Set CS selector to execute the virtual AP code when a SIPI is received_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39405u,
		"Set CS selector to execute the virtual AP code when multiple SIPIs are received_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39473u,
		"Set CS base address to execute the virtual AP code when multiple SIPIs are sent to the virtual AP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39474u,
		"Set CS base address to execute the virtual AP code when a SIPI is sent to the virtual AP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38919u,
		"vCPU is BSP in HALT state ignore INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38925u,
		"vCPU is BSP in HALT state ignore SIPI_001");

#endif
#ifdef IN_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 38911u, "ignore INIT for safety VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38912u, "ignore SIPI for safety VM_001");
#endif

}

int ap_main(void)
{
	/*only run in vcpu1*/
	if (get_lapic_id() != 1) {
		return 0;
	}

	ap_is_running++;
	printf("ap start run %d apic=%d\n", ap_is_running, get_lapic_id());
	while (start_run_id == 0) {
		test_delay(1);
	}
	debug_print("ap start run %d\n", start_run_id);
	switch (start_run_id) {
	case 27160:
		MP_initialization_rqmid_27160_ap();
		break;
	case 26996:
		MP_initialization_rqmid_26996_ap();
		break;
	case 33850:
		MP_initialization_rqmid_33850_ap();
		break;
	case 38919:
		MP_initialization_rqmid_38919_ap();
		break;
	case 38925:
		MP_initialization_rqmid_38925_ap();
		break;
	case 39037:
		MP_initialization_rqmid_39037_ap();
		break;
	case 39038:
		MP_initialization_rqmid_39038_ap();
		break;
	case 39039:
		MP_initialization_rqmid_39039_ap();
		break;
	case 39053:
		MP_initialization_rqmid_39053_ap();
		break;
	case 26998:
	case 26997:
	case 39396:
	case 39405:
	case 39473:
	case 39474:
		MP_initialization_ap_common_func();
		break;
	default:
		break;
	}
	return 0;
}

int main(void)
{
	bsp_is_running++;
	print_case_list();
#ifdef IN_NON_SAFETY_VM
	test_MP_list();
#endif
#ifdef IN_SAFETY_VM
	MP_initialization_rqmid_38911_ignore_INIT_for_safety_VM_001();
	MP_initialization_rqmid_38912_ignore_SIPI_for_safety_VM_001();
#endif
	return report_summary();
}
#endif /* endif __x86_64__ */
