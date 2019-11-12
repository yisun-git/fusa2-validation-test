mov %cr0, %eax
mov %eax, (0x6000)
mov %cs,  %eax
mov %eax, (0x6000 + 4)
mov $0xafaf, %eax
mov %eax, (0x6000 + 8)
