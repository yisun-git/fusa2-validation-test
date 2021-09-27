#define NEED_CRC
#include "safety_analysis.h"
#include "crc32.c"

#define MEM_WR_ADDR_START_0     (0x00000000UL)
#define MEM_WR_ADDR_END_0       (0x10000000UL)
#define PCI_CARD_DETECT_NUM	(5)

/*
 * @brief case name:mitigation-Spatial Interface_Memory isolation - DMA_002
 *
 * Summary:ACRN hypervisor shall prevent a vCPU of a VM from accessing
 * memory assigned to another VM by DMA.
 */
static __unused void safety_analysis_rqmid_33883_mitigation_Spatial_Interface_Memory_isolation_DMA_002(void)
{
	uint64_t start = MEM_WR_ADDR_START_0;
	uint64_t end = MEM_WR_ADDR_END_0;
	uint64_t size = 0UL;
	bool is_pass = false;
	uint32_t mem_crc32 = 0U;
	size = FILL_DATA_COUNT;

#ifdef IN_NON_SAFETY_VM
	write_mem_data_ex(start, end, size, FILL_DATA_START_0);
	/*add this delay to let safety-vm write over*/
	busy_delay(10000);
#endif

#ifdef IN_SAFETY_VM
	/*add this delay to let non-safety-vm write first*/
	busy_delay(10000);
	write_mem_data_ex(start, end, size, FILL_DATA_START_1);
#endif

	/*caculate crc32 of memory area to check overlap */
	mem_crc32 = calc_crc32(start, end, size);

#ifdef IN_NON_SAFETY_VM
	is_pass = (mem_crc32 == FILL_DATA_CRC32_0) ? true : false;
#endif

#ifdef IN_SAFETY_VM
	is_pass = (mem_crc32 == FILL_DATA_CRC32_1) ? true : false;
#endif

#ifdef NEED_CRC
	dump_mem(start, end, size);
#endif

	printf("\nWrite_Read_Coherence_State: %d", is_pass);
	printf("\nMemory_CRC32: %x \n", mem_crc32);

	printf("\nplease using SP605 DMA, pass-through it to 2 VM separately,\
	and read address range:[0x00000000, 0x10000000) \n");
	printf("\ncompare it with the dump data respectively,\
	if both equal, then pass.\n");
	report("%s()", is_pass, __func__);

	int count = 0;
	while (count < PCI_CARD_DETECT_NUM) {
		sp605_card_detect();
		busy_delay(6000);
		count++;
	}

}

static __unused void print_case_list(void)
{
	printf("\t\t safety_analysis feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 33883u,
	"mitigation-Spatial Interface_Memory isolation - DMA_002");
}

int main(void)
{
	setup_idt();
	print_case_list();

	extern idt_entry_t boot_idt[256];
	printf("\n boot_idt=%p %s\n", boot_idt, __FILE__);
	safety_analysis_rqmid_33883_mitigation_Spatial_Interface_Memory_isolation_DMA_002();

	return report_summary();
}
