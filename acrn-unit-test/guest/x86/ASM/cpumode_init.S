#include "../cpumode.h"

#save cr0
mov %cr0, %eax
mov %eax, (INIT_CR0_ADDR)

efer = 0xc0000080  //IA32_EFER
mov $efer, %ecx
rdmsr
mov %eax, (INIT_IA32_EFER_LOW_ADDR)
mov %edx, (INIT_IA32_EFER_HIGH_ADDR)

mov $0xf000, %esp
pushfl
pop (INIT_EFLAGS_ADDR)
