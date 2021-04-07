
#save cr0
mov %cr0, %eax
mov %eax, (0x8200)

efer = 0xc0000080  //IA32_EFER
mov $efer, %ecx
rdmsr
mov %eax, (0x8300)
mov %edx, (0x8304)
