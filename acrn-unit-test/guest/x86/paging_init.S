#include "paging.h"
mov %cr0, %eax
mov %eax, INIT_CR0_ADDR
mov %cr2, %eax
mov %eax, INIT_CR2_ADDR
mov %cr4, %eax
mov %eax, INIT_CR4_ADDR
/* push 32bit eflags to eax */
pushfl
pop %eax
mov %eax, INIT_EFLAGS_ADDR
mov $0xc0000080, %ecx
rdmsr
mov %eax, INIT_IA32_EFER_LOW_ADDR
mov %edx, INIT_IA32_EFER_HIGH_ADDR

