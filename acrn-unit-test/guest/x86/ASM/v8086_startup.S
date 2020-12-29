#include "../v8086_protect.h"

mov %cr4, %eax
mov %eax, (V8086_CR4_STARTUP_ADDR)
