mov $0x0000003a, %ecx
rdmsr
mov %eax, (0x8000)
mov %edx, (0x8000 + 4)

