#include "../../lib/x86/xsave_asm.h"

/* record CR4 following INIT */
mov %cr4, %eax
mov %eax, XSAVE_INIT_CR4_ADDR

/*enable cr4.osxsave[18].*/
mov %cr4, %eax
bts $18, %eax
mov %eax, %cr4

/*Clear cr0.MP[1] and cr0.EM[2] for SSE.*/
mov %cr0, %eax
btr $1, %eax
btr $2, %eax
mov %eax, %cr0

/*enable cr4.OSFXSR[9] for SSE.*/
mov %cr4, %eax
bts $9, %eax
mov %eax, %cr4

