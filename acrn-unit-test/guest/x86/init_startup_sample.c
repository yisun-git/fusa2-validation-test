#include "libcflat.h"
#include "desc.h"
#include "types.h"
#include "processor.h"
#include "apic.h"
#include "isr.h"
extern void send_sipi();
void save_unchanged_reg()
{
    asm volatile (
        "mov %cr0, %" R "ax\n"
        "mov %eax, (0x7000)\n"
        "mov %cs,%eax\n"
        );
}
static void print_case_list()
{
	printf("init_startup_sample feature case list:\n\r");
	printf("\t\t Requirement: %s DNG:%d case name:%s\n\r", "init_startup_sample", 1234u, "Startup check cs");
	printf("\t\t Requirement: %s DNG:%d case name:%s\n\r", "init_startup_sample", 2345u, "INIT check cs");
	printf("\t\t Requirement: %s DNG:%d case name:%s\n\r", "init_startup_sample", 3456u, "Unchanged check cr0");
}
int main(void)
{
	volatile u16 unchanged_ap_cr0 = 0;
	volatile u32 unchanged_ap_cs = 0;
	volatile u16 ap_cr0 = 0;
	volatile u32 ap_cs = 0;
	volatile u32 * ptr;
	volatile u32 bp_cr0;
	volatile u32 bp_cs;

	ptr = (volatile u32 *)0x6000;
	bp_cr0 = *ptr;
	bp_cs = *(ptr + 1);

	print_case_list();

	printf("------------------------Startup check----------------------------------\n\r");
	/*first start up*/
	printf("BP:\n\r");
	printf("bp_greg_cr0:0x%x\n\r", bp_cr0);
	printf("bp_greg_cs:0x%x\n\r", bp_cs);
	report("Startup check CS reg:", (bp_cs==0x8));

	printf("------------------------Init check----------------------------------\n\r");
	ptr = (volatile u32 *)0x7000;
	ap_cr0 = *ptr;
	ap_cs = *(ptr + 1);
	printf("AP:\n\r");
	printf("ap_greg_cr:0x%x\n\r", ap_cr0);
	printf("ap_greg_cs:0x%x\n\r", ap_cs);
	report("Init check CS reg:", (ap_cs==0x8));

	printf("------------------------unchanged check------------------------------\n\r");
	/*cp ap register value to tmp before send sipi */
	ptr = (volatile u32 *)0x8000;
	unchanged_ap_cr0 = *ptr;
	unchanged_ap_cs = *(ptr + 1);
	printf("AP unchanged\n\r");
	printf("unchanged_ap_cr0:0x%x\n\r", unchanged_ap_cr0);
	printf("unchanged_ap_cs:0x%x\n\r", unchanged_ap_cs);

	/*send sipi to ap*/
	send_sipi();
	/*get init value again after ap */
	ptr = (volatile u32 *)0x7000;
	ap_cr0 = *ptr;
	ap_cs = *(ptr + 1);
	/*compare init value with unchanged */
	report("ap unchanged cr0:", (unchanged_ap_cr0==ap_cr0));

	return report_summary();
}




