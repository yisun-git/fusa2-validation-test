#include "../../lib/x86/xsave_asm.h"

/* record CR4 following start-up */
mov %cr4, %eax
mov %eax, XSAVE_STARTUP_CR4_ADDR

/*enable CR4.OSXSAVE[18] feature.*/
mov %cr4, %eax
bts $18, %eax
mov %eax, %cr4

/* record XCR0 following start-up */
mov $0, %eax
mov $0, %edx
mov $0, %ecx
xgetbv
mov %eax, (XSAVE_STARTUP_XCR0_LOW_ADDR)
mov %edx, (XSAVE_STARTUP_XCR0_HI_ADDR)

/* set XCR0 to 7 (As XCR1 is equal to XCR0 & XINUSE, XINUSE[2:0] is equal to XCR1 when XCR0 is set to 7.) */
mov $0, %ecx
mov $0, %edx
mov $7, %eax
xsetbv

/* record XCR1 following start-up */
mov $0, %eax
mov $0, %edx
mov $1, %ecx
xgetbv
mov %eax, (XSAVE_STARTUP_XCR1_LOW_ADDR)
mov %edx, (XSAVE_STARTUP_XCR1_HI_ADDR)

/*set stack esp.*/
mov $stacktop, %esp

/*enable XCRO X87|SSE.*/
mov $0, %ecx
mov $0, %edx
mov $3, %eax
xsetbv

/*clear CR0.MP[1] and CR0.EM[2] for SSE.*/
mov %cr0, %eax
btr $1, %eax
btr $2, %eax
mov %eax, %cr0

/*enable CR4.OSFXSR[9] for SSE.*/
mov %cr4, %eax
bts $9, %eax
mov %eax, %cr4
