#include "../paging.h"
mov %cr0, %eax
mov %eax, STARTUP_CR0_ADDR
mov %cr2, %eax
mov %eax, STARTUP_CR2_ADDR
mov %cr4, %eax
mov %eax, STARTUP_CR4_ADDR

/* push 32bit eflags to eax */
//pushf
//pop
mov %eax, STARTUP_EFLAGS_ADDR

mov $0xc0000080, %ecx
rdmsr
mov %eax, STARTUP_IA32_EFER_LOW_ADDR
mov %edx, STARTUP_IA32_EFER_HIGH_ADDR
