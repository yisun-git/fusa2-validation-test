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
#include "instruction_common.h"
#include "xsave.h"
#include "register_op.h"
#include "fwcfg.h"
#include "delay.h"

#ifdef __x86_64__
#include "./64/taskmanagement_fn.c"
#else
#include "./32/taskmanagement_fn.c"
#endif

void save_unchanged_reg(void)
{

}

int main(void)
{
	setup_vm();
	setup_idt();

#ifdef __x86_64__
	setup_tss64();

	print_case_list_64();
	test_task_management_64();
#else
	init_do_less_privilege();
	setup_tss32();

	print_case_list_32();
	test_task_management_32();
#endif

	return report_summary();
}
