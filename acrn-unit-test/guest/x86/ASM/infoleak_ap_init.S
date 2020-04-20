
#ia32_spec_ctrl
efer = 0x00000048
mov $efer, %ecx
rdmsr
mov %eax, (0x8200)

