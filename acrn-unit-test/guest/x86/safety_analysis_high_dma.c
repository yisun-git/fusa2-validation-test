#include "safety_analysis.h"
#include "crc32.c"

#define MEM_WR_ADDR_START_0     (0x10000000UL)
#define MEM_WR_ADDR_END_0       (0x20000000UL)
#define PCI_CARD_DETECT_NUM     (5)

/*
 * @brief case name:mitigation-Spatial Interface_Memory isolation - DMA_001
 *
 * Summary:ACRN hypervisor shall prevent a vCPU of a VM from
 * accessing memory assigned to another VM by DMA.
 */
static __unused void safety_analysis_rqmid_33882_mitigation_Spatial_Interface_Memory_isolation_DMA_001(void)
{
	uint64_t start = MEM_WR_ADDR_START_0;
	uint64_t end = MEM_WR_ADDR_END_0;
	uint64_t size = 0UL;
	bool is_pass = false;
	uint32_t mem_crc32 = 0U;
	size = (end - start) / 4;

#ifdef IN_NON_SAFETY_VM
	write_mem_data(start, size, FILL_DATA_START_0);
	/*add this delay to let safet-vm write over*/
	busy_delay(10000);
#endif

#ifdef IN_SAFETY_VM
	/*add this delay to let non-safet-vm write first*/
	busy_delay(10000);
	write_mem_data(start, size, FILL_DATA_START_1);
#endif

	/*caculate crc32 of memory area to check overlap */
	mem_crc32 = Crc32_ComputeBuf(0, (void *)start, size * 4);

#ifdef IN_NON_SAFETY_VM
	is_pass = (mem_crc32 == FILL_DATA_CRC32_0) ? true : false;
#endif

#ifdef IN_SAFETY_VM
	is_pass = (mem_crc32 == FILL_DATA_CRC32_1) ? true : false;
#endif
	DUMP_DATA_U32("Dump Head Memory 0x10000000UL", (start), 64);
	printf("\nWrite_Read_Coherence_State: %d", is_pass);
	printf("\nMemory_CRC32: %x \n", mem_crc32);
	printf("\nplease using SP605 DMA, pass-through it to 2 VM separately,\
	and read address range:[0x10000000, 0x20000000) \n");
	printf("\ncompare it with the dump data respectively,\
	if both equal, then pass.\n");

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
	printf("\t\t Case ID:%d case name:%s\n\r", 33882u,
	"mitigation-Spatial Interface_Memory isolation - DMA_001");
}

int main(void)
{
	setup_idt();
	print_case_list();

	extern idt_entry_t boot_idt[256];
	printf("\n boot_idt=%p %s\n", boot_idt, __FILE__);
	safety_analysis_rqmid_33882_mitigation_Spatial_Interface_Memory_isolation_DMA_001();

	return report_summary();
}
