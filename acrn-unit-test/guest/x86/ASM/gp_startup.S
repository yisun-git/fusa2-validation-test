#include "../general_purpose.h"
#ifdef RUN_IN_64BIT
movq %r8, (STARTUP_R8_ADDR)
movq %r9, (STARTUP_R9_ADDR)
movq %r10, (STARTUP_R10_ADDR)
movq %r11, (STARTUP_R11_ADDR)
movq %r12, (STARTUP_R12_ADDR)
movq %r13, (STARTUP_R13_ADDR)
movq %r14, (STARTUP_R14_ADDR)
movq %r15, (STARTUP_R15_ADDR)
#else
mov %eax,STARTUP_EAX_ADDR
mov %edx,STARTUP_EDX_ADDR
mov %ebx,STARTUP_EBX_ADDR
mov %ecx,STARTUP_ECX_ADDR
mov %ebp,STARTUP_EBP_ADDR
mov %esp,STARTUP_ESP_ADDR
mov %edi,STARTUP_EDI_ADDR
mov $stacktop, %esp
pushfl
pop %eax
mov %eax, STARTUP_EFLAGS_ADDR
#endif
