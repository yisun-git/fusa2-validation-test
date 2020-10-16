#include "../multiboot.h"
/*get AP cr0 INIT value*/
mov %cr0, %eax
mov %eax, MULTIBOOT_INIT_CR0_ADDR

/*get AP esi INIT value*/
mov %esi, MULTIBOOT_INIT_ESI_ADDR

