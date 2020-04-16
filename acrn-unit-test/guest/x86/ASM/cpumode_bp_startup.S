
#ia32_efer
efer = 0xc0000080
mov $efer, %ecx
rdmsr
mov %eax, (0x8100)

