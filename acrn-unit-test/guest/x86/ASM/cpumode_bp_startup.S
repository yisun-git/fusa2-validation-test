#include "../cpumode.h"

mov %cr0, %eax
mov %eax, (STARTUP_CR0_ADDR)

#ia32_efer
efer = 0xc0000080
mov $efer, %ecx
rdmsr
mov %eax, (STARTUP_IA32_EFER_ADDR)

mov $stacktop, %esp
pushfl
pop (STARTUP_EFLAGS_ADDR)

