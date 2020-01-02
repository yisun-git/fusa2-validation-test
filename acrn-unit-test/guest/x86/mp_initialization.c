#include "libcflat.h"
#include "smp.h"
#include "apic.h"
#include "desc.h"
#include "processor.h"
#include "misc.h"

#define MP_APIC_ID_BSP	0
#define MP_APIC_ID_AP	1
#define USE_DEBUG
#ifdef  USE_DEBUG
#define debug_print(fmt, args...) 	printf("[%s:%s] line=%d "fmt"",__FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...) 	printf("[%s:%s] line=%d "fmt"",__FILE__, __func__, __LINE__,  ##args)

#ifdef __x86_64__

u32 bp_bsp_flag = 0,ap_bsp_flag = 0;

void test_delay(int time)
{
	int count = 0;
	u64 tsc;
	tsc = rdtsc() + time*1000000000;

	while (rdtsc() < tsc){
		;
	}
}

static volatile int ap_is_running = 0;
static volatile int bsp_is_running = 0;

static volatile int start_run_id =0;

static volatile int wait_bp=0;
static volatile int wait_ap=0;
void ap_sync()
{
	while(wait_bp != 1){
		debug_print("%d %d\n", wait_ap, wait_bp);
		test_delay(1);
	}
	wait_bp = 0;
}

void bp_sync()
{
	while(wait_ap != 1){
		debug_print("%d %d\n", wait_ap, wait_bp);
		test_delay(1);
	}
	wait_ap = 0;
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

	report("\t\t %s", (ap_bsp_flag==0) && (bp_bsp_flag!=0), __FUNCTION__);

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
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_STARTUP , MP_APIC_ID_BSP);
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
	send_sipi();

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
	send_sipi();

	debug_print("%d %d\n", wait_ap, wait_bp);
	/*wait ap send INIT done*/
	bp_sync();

	debug_print("%d %d\n", wait_ap, wait_bp);
	report("\t\t %s", bsp_is_running == 0, __FUNCTION__);
}

static void test_MP_list(void)
{
	MP_initialization_rqmid_27160_expose_only_one_BSP_to_any_VM_001();
	MP_initialization_rqmid_26995_vCPU_is_AP_in_normal_status_ignore_SIPI();
	MP_initialization_rqmid_28469_ap_wait_for_sipi_ignore_init_001();
	MP_initialization_rqmid_33849_ap_in_normal_handle_INIT_001();
	MP_initialization_rqmid_26996_bsp_in_normal_ignore_SIPI_001();
	//test_delay(5);
	MP_initialization_rqmid_33850_bsp_in_normal_ignore_INIT_001();
}

static void print_case_list(void)
{
	printf("\t\t MP initialization feature case list:\n\r");
	printf("\t\t MP initialization 64-Bits Mode:\n\r");
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
}

int ap_main(void)
{
	ap_is_running++;
	printf("ap start run %d\n", ap_is_running);
	while(start_run_id == 0){
		test_delay(1);
	}
	debug_print("ap start run %d\n", start_run_id);
	switch(start_run_id)
	{
		case 27160:
			MP_initialization_rqmid_27160_ap();
			break;

		case 26996:
			MP_initialization_rqmid_26996_ap();
			break;

		case 33850:
			MP_initialization_rqmid_33850_ap();
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
    test_MP_list();
    return report_summary();
}
#endif /* endif __x86_64__ */
