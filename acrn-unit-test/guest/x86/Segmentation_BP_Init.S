mov %cs,  %eax
mov %eax, (0x6000)
mov %ds,  %eax
mov %eax, (0x6000 + 0x4)
mov %es,  %eax
mov %eax, (0x6000 + 0x8)
mov %fs,  %eax
mov %eax, (0x6000 + 0xc)
mov %gs,  %eax
mov %eax, (0x6000 + 0x10)
mov %ss,  %eax
mov %eax, (0x6000 + 0x14)

/*access rigth*/
/*write*/
mov $0xdeadbeef,%eax
mov %eax,%ds:0x100
//read
mov %ds:0x100 ,%eax
mov %eax,(0x7000)

/*write*/
mov $0xdeadbeef,%eax
mov %eax,%es:0x100
/*read*/
mov %es:0x100 ,%eax
mov %eax,(0x7000 + 0x4)
/*write*/
mov $0xdeadbeef,%eax
mov %eax,%fs:0x100
/*read*/
mov %fs:0x100 ,%eax
mov %eax,(0x7000 + 0x8)

/*write*/
mov $0xdeadbeef,%eax
mov %eax,%gs:0x100
/*read*/
mov %gs:0x100 ,%eax
mov %eax,(0x7000 + 0xc)

/*write*/
mov $0xdeadbeef,%eax
mov %eax,%ss:0x100
/*read*/
mov %ss:0x100 ,%eax
mov %eax,(0x7000 + 0x10)

/*write*/
/*mov $0xdeadbeef,%eax
 *mov %eax,%cs:0x100*/
/*read*/
mov %cs:0x100 ,%eax
mov %eax,(0x7000 + 0x14)

/*limit*/
mov %cs:0xFFFFFFFF,%eax
mov %ds:0xFFFFFFFF,%eax
mov %es:0xFFFFFFFF,%eax
mov %fs:0xFFFFFFFF,%eax
mov %gs:0xFFFFFFFF,%eax
mov %ss:0xFFFFFFFF,%eax

xor %eax,%eax
mov %cr4,%eax
mov %eax,(0x6000 + 0x18)  /*fsgsbase*/

sgdt (0x6000 + 0x20) /*gdt*/

sldt (0x6000 + 0x24)

/*fs_base_start_up*/
xor %ecx,%ecx
mov $0xC0000100,%ecx
rdmsr
mov %edx,(0x6000 + 0x28)
mov %eax,(0x6000 + 0x30)

/*gs_base_start_up*/
xor %ecx,%ecx
mov $0xC0000101,%ecx
rdmsr
mov %edx,(0x6000 + 0x34)
mov %eax,(0x6000 + 0x38)

/*start_i32_kernel_gs_base*/
xor %ecx,%ecx
mov $0xC0000102,%ecx
rdmsr
mov %edx,(0x6000 + 0x3c)
mov %eax,(0x6000 + 0x40)
