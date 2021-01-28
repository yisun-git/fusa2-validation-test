#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "alloc.h"
#include "alloc_phys.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "asm/spinlock.h"
#include "debug_print.h"
#include "multiboot.h"

static int test_fun(int x)
{
	return x+1;
}

typedef int (*test_fun_p)(int);

static int read_write_exec_test(unsigned long start, unsigned long end, int is_exec)
{
	unsigned long len;
	unsigned char *point;
	int i;
	int ret;
	test_fun_p fun_p;

	debug_print("test start:%lx, end:%lx\n", start, end);
	len = end-start;
	point = (unsigned char *) start;
	fun_p = test_fun;

	for (i = 0; i < len; i += 4096) {
		/* test read and write*/
		point[i] = 0xFE;
		if (point[i] != 0xFE) {
			printf("write test error\n");
			return -1;
		}

		if (is_exec) {
			memcpy(&point[i], fun_p, 9);
			fun_p = (test_fun_p)&point[i];
			ret = fun_p(i);
			if (ret != (i+1)) {
				printf("exec error ret=%d, i=%d\n", ret, i);
				return -2;
			}
		}
	}
	return 0;
}

static void memory_1M_to_256M_check(const char *fun_name)
{
	int ret = 0;
	unsigned long start;
	unsigned long end;

	start = 0x100000;	//1M
	end = 0x10000000;   //256M

	ret = read_write_exec_test(start, end, 1);

	report("%s %d", ret == 0, fun_name, ret);
}

/**
 * @brief case name:1G memory region rights_002
 *
 * Summary: On 64 bit Mode, when testing memory region rights&map,
 * select a memory unit in each 4K page to test the read-write and executable.
 * This case and case 34127 construct 1G's check.
 */
static void __unused multiboot_rqmid_42697_1G_memory_region_rights_001()
{
	memory_1M_to_256M_check(__FUNCTION__);
}

static void print_case_list(void)
{
	printf("\t\t multiboot feature case list:\n\r");
	printf("\t\t multiboot 64-Bits Mode:\n\r");

#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 34127u, "1G memory region rights_002");
#endif

}

static void test_multiboot_low_list(void)
{
#ifdef IN_NON_SAFETY_VM
	multiboot_rqmid_42697_1G_memory_region_rights_001();
#endif
}

int main(int ac, char **av)
{
	print_case_list();
	test_multiboot_low_list();
	return report_summary();
}
