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
/*exec SSE instrction.*/
movapd %xmm1, %xmm2

/*xsetbv XCR0 to X87|SSE|AVX.*/
mov $0, %ecx
mov $0, %edx
mov $7, %eax
xsetbv

/*xgetbv XCR0 & XINUSE.*/
mov $0, %eax
mov $0, %edx
mov $1, %ecx
xgetbv

/*store XCR0 & XINUSE to memory.*/
mov %eax, (0x7000)
mov %edx, (0x8000)

