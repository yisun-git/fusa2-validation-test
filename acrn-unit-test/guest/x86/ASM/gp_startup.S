#include "../general_purpose.h"
mov %eax,STARTUP_EAX_ADDR
mov %edx,STARTUP_EDX_ADDR
mov %ebx,STARTUP_EBX_ADDR
mov %ecx,STARTUP_ECX_ADDR
mov %ebp,STARTUP_EBP_ADDR
mov %esp,STARTUP_ESP_ADDR
mov %esi,STARTUP_ESI_ADDR
mov %edi,STARTUP_EDI_ADDR
mov $stacktop, %esp
call 11f
11:
pop %edx
mov %edx, STARTUP_EIP_ADDR
pushfl
pop %eax
mov %eax, STARTUP_EFLAGS_ADDR
