mov $0xa1a1,%edx
mov %cr0, %eax
mov %eax, (0x8000)
mov %cs,  %eax
mov %eax, (0x8000 + 4)
mov $0xa1a1, %eax
mov %eax, (0x8000 + 8)

