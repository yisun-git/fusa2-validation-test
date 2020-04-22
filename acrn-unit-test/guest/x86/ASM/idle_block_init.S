/*MSR_IA32_MISC_ENABLE		0x000001a0*/
mov $0x000001a0, %ecx
rdmsr
mov %eax, (0x8000)
mov %edx, (0x8000 + 4)

