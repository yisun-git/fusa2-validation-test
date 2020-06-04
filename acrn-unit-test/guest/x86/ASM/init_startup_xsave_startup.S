/*set stack esp.*/
mov $stacktop, %esp
/*enable CR4.OSXSAVE[18] feature.*/
mov %cr4, %eax
bts $18, %eax
mov %eax, %cr4
/* get XINUSE[bit 2:0] default value in bp */
mov $0, %eax
mov $0, %edx
mov $1, %ecx
xgetbv
mov %eax, (0x6000)
mov %edx, (0x6004)
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

