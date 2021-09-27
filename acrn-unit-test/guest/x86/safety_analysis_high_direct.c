#include "safety_analysis.h"
#include "crc32.c"
#define MEM_WR_ADDR_START_0     (0x10000000UL)
#define MEM_WR_ADDR_END_0       (0x20000000UL)

/*
 * @brief case name:mitigation-Spatial Interface_Memory isolation - direct access_001
 *
 * Summary:ACRN hypervisor shall prevent a vCPU of a VM from directly
 * accessing memory assigned to another VM.
 */
static __unused void safety_analysis_rqmid_33884_mitigation_Spatial_Interface_Memory_isolation_direct_access_001(void)
{
	uint64_t start = MEM_WR_ADDR_START_0;
	uint64_t end = MEM_WR_ADDR_END_0;
	uint64_t size = 0UL;
	bool is_pass = false;
	uint32_t mem_crc32_first = 0U;
	uint32_t mem_crc32_second = 0U;
	size = (end - start) / 4;
#ifdef IN_NON_SAFETY_VM
	write_mem_data(start, size, FILL_DATA_START_0);
	mem_crc32_first = Crc32_ComputeBuf(0, (void *)start, size * 4);
	/*add this delay 10s to let safety-vm write over*/
	busy_delay(10000);
#endif

#ifdef IN_SAFETY_VM
	/*add this delay 10s to let non-safety-vm write first*/
	busy_delay(10000);
	write_mem_data(start, size, FILL_DATA_START_1);
	mem_crc32_first = Crc32_ComputeBuf(0, (void *)start, size * 4);

	/*add this delay 10s to let non-safety-vm write over*/
	busy_delay(10000);
	/*caculate crc32 of memory area to check overlap */
	mem_crc32_second = Crc32_ComputeBuf(0, (void *)start, size * 4);
	is_pass = (mem_crc32_first == mem_crc32_second) ? true : false;
#endif

#ifdef IN_NON_SAFETY_VM
	/*caculate crc32 of memory area to check overlap */
	mem_crc32_second = Crc32_ComputeBuf(0, (void *)start, size * 4);
	is_pass = (mem_crc32_first == mem_crc32_second) ? true : false;

	/*write the memory range second time to check the VM1 overlaped*/
	write_mem_data(start, size, FILL_DATA_START_0);
#endif

	report("%s", is_pass, __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("\t\t safety_analysis feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 33884u,
	"mitigation-Spatial Interface_Memory isolation - direct access_001");
}

int main(void)
{
	setup_idt();
	print_case_list();

	extern idt_entry_t boot_idt[256];
	printf("\n boot_idt=%p %s\n", boot_idt, __FILE__);
	safety_analysis_rqmid_33884_mitigation_Spatial_Interface_Memory_isolation_direct_access_001();

	return report_summary();
}
