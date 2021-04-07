#include "../tsc.h"

#tsc
efer = 0x10
mov $efer, %ecx
rdmsr
mov %eax, INIT_TSC_LOW_ADDR
mov %edx, INIT_TSC_HIGH_ADDR

#save cr4 to 0x8008
mov %cr4, %eax
mov %eax, INIT_CR4_ADDR

#tscadj
efer = 0x0000003b
mov $efer, %ecx
rdmsr
mov %eax, INIT_TSCADJ_LOW_ADDR
mov %edx, INIT_TSCADJ_HIGH_ADDR

#tscaux
efer = 0xC0000103
mov $efer, %ecx
rdmsr
mov %eax, INIT_TSCAUX_LOW_ADDR
mov %edx, INIT_TSCAUX_HIGH_ADDR
