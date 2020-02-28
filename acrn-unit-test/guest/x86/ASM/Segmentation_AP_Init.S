mov %cs,  %eax
mov %eax, (0x8000)
mov %ds,  %eax
mov %eax, (0x8000 + 0x4)
mov %es,  %eax
mov %eax, (0x8000 + 0x8)
mov %fs,  %eax
mov %eax, (0x8000 + 0xc)
mov %gs,  %eax
mov %eax, (0x8000 + 0x10)
mov %ss,  %eax
mov %eax, (0x8000 + 0x14)

xor %eax,%eax
mov %cr4,%eax
mov %eax,(0x8000 + 0x18)

/*save gdt*/
sgdt (0x8000 + 0x20)

/*save MSR_IA32_FS_BASE*/
xor %ecx,%ecx
mov $0xC0000100,%ecx
rdmsr
mov %edx,(0x8000 + 0x28)
mov %eax,(0x8000 + 0x30)

/*save MSR_IA32_GS_BASE*/
xor %ecx,%ecx
mov $0xC0000101,%ecx
rdmsr
mov %edx,(0x8000 + 0x34)
mov %eax,(0x8000 + 0x38)

/*save IA32_KERNEL_GS_BASE*/
xor %ecx,%ecx
mov $0xC0000102,%ecx
rdmsr
mov %edx,(0x8000 + 0x3c)
mov %eax,(0x8000 + 0x40)

/*access check
 *check ds write*/
mov $0xdeadbeef,%eax
mov %eax,%ds:0x100
/*check ds read*/
mov %ds:0x100 ,%eax
mov %eax,(0x9000)

/*check es write*/
mov $0xdeadbeef,%eax
mov %eax,%es:0x100
/*check es read*/
mov %es:0x100 ,%eax
mov %eax,(0x9000 + 0x4)

/*check fs write*/
mov $0xdeadbeef,%eax
mov %eax,%fs:0x100
/*check fs read*/
mov %fs:0x100 ,%eax
mov %eax,(0x9000 + 0x8)

/*check gs write*/
mov $0xdeadbeef,%eax
mov %eax,%gs:0x100
/*check gs read*/
mov %gs:0x100 ,%eax
mov %eax,(0x9000 + 0xc)

/*check ss write*/
mov $0xdeadbeef,%eax
mov %eax,%ss:0x100
/*check ss read*/
mov %ss:0x100 ,%eax
mov %eax,(0x9000 + 0x10)

/*check cs read*/
mov %cs:0x100 ,%eax
mov %eax,(0x9000 + 0x14)

