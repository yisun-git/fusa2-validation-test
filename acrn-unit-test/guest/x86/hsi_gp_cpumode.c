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
#include "hsi_gp_cpumode.h"

#define USE_DEBUG
#include "debug_print.h"

/*
 * @brief case name: HSI_Generic_Processor_Features_Cpu_Modes_Operation_BP_001
 *
 * Summary: Under native mode, when the physical BP power-up or a reset, check BP at protect mode,
 * then set IA32_EFER.LME to 1, to switch from protected-mode to 64-bit mode.
 */
static __unused void hsi_rqmid_42290_generic_processor_features_cpu_mode_operation_bp_001(void)
{
	u16 chk = 0;

	u32 cr0 = *((u32 *)SAVE_CR0);
	u64 ia32_efer = *((u64 *)SAVE_IA32_EFER);
	/* make sure BP power-up or reset at protect mode */
	if (((cr0 & X86_CR0_PE) != 0) &&
		((ia32_efer & IA32_EFER_LME) == 0)) {
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

	debug_print("current ia32_efer:%lx\n", ia32_efer);

	report("%s", (chk == 2), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t Demo test case list: \n\r");
#ifdef IN_NATIVE
#ifdef __x86_64__
	printf("\t Case ID: %d Case name: %s\n\r", 42290u, "HSI generic processor features " \
		"cpu mode operation bp 001");
#endif
#endif
	printf("\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	setup_vm();
	print_case_list();

#ifdef IN_NATIVE
#ifdef __x86_64__
	hsi_rqmid_42290_generic_processor_features_cpu_mode_operation_bp_001();
#endif
#endif
	return report_summary();
}

