mov $0x0000003a, %ecx
rdmsr
mov %eax, (0x6000)
mov %edx, (0x6000 + 4)