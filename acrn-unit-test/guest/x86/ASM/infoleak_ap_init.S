#include "../info_leakage.h"

#ia32_spec_ctrl
spec_ctrl = IA32_SPEC_CTRL_MSR
mov $spec_ctrl, %ecx
rdmsr

/* if IA32_SPEC_CTRL_MSR is not 0, it means that we enter
 * for the second time, so we jump to 2:
 */
cmp $0, %eax
jne  2f

/* if we are here, it means that we enter for the first time,
 * we dump IA32_SPEC_CTRL_MSR and set IA32_SPEC_CTRL_MSR to 0x7
 * because only the lower 3 bits are available in IA32_SPEC_CTRL_MSR.
 */
mov %eax, (AP_IA32_SPEC_CTRL_INIT_ADDR)
mov $7, %eax
mov %eax, (AP_IA32_SPEC_CTRL_SIPI_ADDR)
wrmsr
jmp 3f

/* we dump IA32_SPEC_CTRL_MSR and reset it to 0. */
2:
mov %eax, (AP_IA32_SPEC_CTRL_SIPI_ADDR)
mov $0, %eax
wrmsr

3:

