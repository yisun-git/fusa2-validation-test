#include "../info_leakage.h"

#ia32_spec_ctrl
spec_ctrl = IA32_SPEC_CTRL_MSR
mov $spec_ctrl, %ecx
rdmsr

mov %eax, (AP_IA32_SPEC_CTRL_INIT_ADDR)

