#include "../multiboot.h"

/*get BP cr0 startup value*/
mov %cr0, %eax
mov %eax, MULTIBOOT_STARTUP_CR0_ADDR

/*get BP esi startup value*/
mov %esi, MULTIBOOT_STARTUP_ESI_ADDR
