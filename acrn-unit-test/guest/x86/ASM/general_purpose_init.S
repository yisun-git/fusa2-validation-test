#include "../general_purpose.h"
mov %eax,INIT_EAX_ADDR
mov %edx,INIT_EDX_ADDR
mov %ebx,INIT_EBX_ADDR
mov %ecx,INIT_ECX_ADDR
mov %ebp,INIT_EBP_ADDR
mov %esp,INIT_ESP_ADDR
mov %esi,INIT_ESI_ADDR
mov %edi,INIT_EDI_ADDR

mov $stacktop, %esp
/* push 32bit eflags to eax */
pushfl
pop %eax
mov %eax, INIT_EFLAGS_ADDR

