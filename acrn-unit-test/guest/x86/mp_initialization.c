#include "libcflat.h"
#include "smp.h"
#include "apic.h"
#include "desc.h"
#include "processor.h"
#include "misc.h"

#ifdef __x86_64__

u32 bp_bsp_flag = 0,ap_bsp_flag = 0;

void test_delay(void)
{
    u64 tsc;
    tsc = rdtsc() + 1000000000;
    while (rdtsc() < tsc);
}

static void send_ipi_test(int to_cpu, int ipi_type)
{
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | ipi_type, to_cpu);
}

static volatile int ap_is_running = 0;
static volatile int bsp_is_running = 0;

void ap_run(void *data)
{
	printf("ap: cpu %d begin to run\n", apic_id());
	ap_is_running = 1;

	/* 146059 - BSP in normal status ignore SIPI */
	/* send SIPI to BSP */
	while(bsp_is_running == 0);
	send_ipi_test(0, APIC_DM_STARTUP);
}

static void MP_initialization_rqmid_146061_ap_run(void *data)
{
	int count = 15;
	printf("ap: cpu %d begin to run\n", apic_id());
	ap_is_running = 1;

	while(0 < count)
	{
		printf("ap: cpu %d is running\n", apic_id());
		test_delay();
		count--;
	}
}

/**
 * @brief Case name:Multiple-Processor Initialization_AP in normal status ignore SIPI_001
 *
 * Summary: no code of SIPI handler would be executed by AP after AP receive SIPI from BSP
 *
 */
static void MP_initialization_rqmid_146061_vCPU_is_AP_in_normal_status_ignore_SIPI(void)
{
	int ncpus,count = 15;

    smp_init();
    ncpus = cpu_count();
    printf("found %d cpus\n", ncpus);

	ap_is_running = 0;

    /* wakeup AP */
	set_idt_entry(0x20, MP_initialization_rqmid_146061_ap_run, 0);
	apic_icr_write(APIC_INT_ASSERT | APIC_DEST_PHYSICAL | APIC_DM_FIXED | 0x20, 1); 

	while(ap_is_running == 0);

	/* 146061 - AP in normal status ignore SIPI */
	/* send SIPI to AP */
	send_ipi_test(1, APIC_DM_STARTUP);

	while( 0 < count )
	{
		test_delay();
		count--;
	}
	report("\t\t MP_initialization_rqmid_146061_vCPU_is_AP_in_normal_status_ignore_SIPI",
		ap_is_running == 1);
}

/**
 * @brief Case name:Multiple-Processor Initialization_ACRN expose only one BSP to any VM_001
 *
 * Summary: hypervisor expose only one BSP to any VM,only one processor's BSP flag in IA32_APIC_BASE MSR is 1
 *
 */
void MP_initialization_rqmid_146062_expose_only_one_BSP_to_any_VM_001(void)
{
	bp_bsp_flag = (rdmsr(MSR_IA32_APICBASE) & APIC_BSP) >> 8;
	printf("bsp: bsp flag is %d\n", bp_bsp_flag);
	report("\t\t MP_initialization_rqmid_146062_expose_only_one_BSP_to_any_VM_001",
		bp_bsp_flag != ap_bsp_flag);
}

void MP_initialization_rqmid_146062_ap(void)
{
	ap_bsp_flag = (rdmsr(MSR_IA32_APICBASE) & APIC_BSP) >> 8;
	printf("ap: bsp flag is %d\n", ap_bsp_flag);
}

static void test_MP_list(void)
{
	MP_initialization_rqmid_146061_vCPU_is_AP_in_normal_status_ignore_SIPI();
	MP_initialization_rqmid_146062_expose_only_one_BSP_to_any_VM_001();
}

static void print_case_list(void)
{
	printf("\t\t MP initialization feature case list:\n\r");
	printf("\t\t MP initialization 64-Bits Mode:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 146061u,
		"Multiple-Processor Initialization_AP in normal status ignore SIPI_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 146062u,
		"Multiple-Processor Initialization_ACRN expose only one BSP to any VM_001");
}

int ap_main(void)
{
	MP_initialization_rqmid_146062_ap();
	return 0;
}

int main(void)
{
	print_case_list();
    test_MP_list();
    return report_summary();
}
#endif /* endif __x86_64__ */