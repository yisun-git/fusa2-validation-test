#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include <linux/pci_regs.h>
#include "alloc_page.h"
#include "alloc_phys.h"
#include "asm/page.h"
#include "x86/vm.h"

#include "instruction_common.h"
#include "xsave.h"
#include "regdump.h"
#include "hsi_16_64_cpumode.h"

#define USE_DEBUG
#include "debug_print.h"

#define CS_L_BIT (1ULL << 53U)
#define CPU_MODE_REAL 0
#define CPU_MODE_64BIT_MODE 1
/* start_run_id is used to identify which test case is running. */
static volatile bool ap_booted = false;

__unused static inline uint64_t get_gdt_base(void)
{
	struct descriptor_table_ptr gdtb;
	asm volatile ("sgdt %0" : "=m"(gdtb) : : "memory");

	return gdtb.base;
}

static u32 cpu_mode = CPU_MODE_REAL;
static void ap_check_cpu_mode()
{
	u16 chk = 0;
	u64 ia32_efer;

	/* check if CR0.PE should be 0 in real-mode */
	if (((*(volatile u32 *)INIT_CR0_ADDR) & X86_CR0_PE) == 0) {
		chk++;
	}

	ia32_efer = rdmsr(MSR_IA32_EFER);
	/* set ia32_efer.lme to 1*/
	/* wrmsr(MSR_IA32_EFER, (ia32_efer | IA32_EFER_LME)); */
	/* make sure current mode is IA-32E mode */
	ia32_efer = rdmsr(MSR_IA32_EFER);
	if ((ia32_efer & IA32_EFER_LME) != 0) {
		chk++;
	}

	uint16_t cs = read_cs();
	uint64_t gdt_base = get_gdt_base();

	/* make sure CS.L is set to 1 */
	if ((((uint64_t *)gdt_base)[cs >> 3] & CS_L_BIT) != 0) {
		chk++;
	}

	if (chk == 3) {
		cpu_mode = CPU_MODE_64BIT_MODE;
	}
	ap_booted = true;
}
/*
 * @brief case name: HSI_Generic_Processor_Features_Cpu_Modes_Operation_BP_001
 *
 * Summary: Under native mode, when the physical AP power-up or a reset, check AP at real mode,
 * then set IA32_EFER.LME to 1 and set cr0.pe to 1, to switch from read-mode to 64-bit mode.
 */
static __unused void hsi_rqmid_47009_generic_processor_features_cpu_mode_operation_ap_001(void)
{

	while (ap_booted != true) {
		asm volatile ("pause");
	}
	report("%s", (cpu_mode == CPU_MODE_64BIT_MODE), __FUNCTION__);
}


static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t Demo test case list: \n\r");
#ifdef IN_NATIVE
#ifdef __x86_64__
	printf("\t Case ID: %d Case name: %s\n\r", 47009u, "HSI generic processor features " \
		"cpu mode operation bp 001");
#endif
#endif
	printf("\t \n\r \n\r");
}

int ap_main(void)
{
#ifdef IN_NATIVE
#ifdef __x86_64__
	debug_print("ap_cpu_mode %d starts running \n", get_lapic_id());
	if (get_lapic_id() != 2) {
		return 0;
	}
	ap_check_cpu_mode();

#endif
#endif
	return 0;
}

int main(void)
{
	setup_idt();
	setup_vm();
	print_case_list();

#ifdef IN_NATIVE
#ifdef __x86_64__
	hsi_rqmid_47009_generic_processor_features_cpu_mode_operation_ap_001();
#endif
#endif
	return report_summary();
}

