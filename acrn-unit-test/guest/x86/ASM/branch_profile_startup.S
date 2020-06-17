mov $0x1a0, %ecx
rdmsr
mov %eax, (0x6000)

