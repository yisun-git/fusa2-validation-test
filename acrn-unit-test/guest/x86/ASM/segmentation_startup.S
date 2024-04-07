
MSR_BASE = 10
GDTR_BASE = 40
GDT_BASE = 50
ACCESS_BASE = 80

DUMP_MAGIC = 0xDEADBEAF
dump_sect_bp = 0x6000

mov $DUMP_MAGIC, %eax
mov %eax, dump_sect_bp

mov %cs,  %eax
mov %eax, (dump_sect_bp + 1 * 4)
mov %ds,  %eax
mov %eax, (dump_sect_bp + 2 * 4)
mov %es,  %eax
mov %eax, (dump_sect_bp + 3 * 4)
mov %fs,  %eax
mov %eax, (dump_sect_bp + 4 * 4)
mov %gs,  %eax
mov %eax, (dump_sect_bp + 5 * 4)
mov %ss,  %eax
mov %eax, (dump_sect_bp + 6 * 4)

xor %eax,%eax
mov %cr4,%eax
mov %eax, (dump_sect_bp + 7 * 4)

xor %eax,%eax
mov %cr0,%eax
mov %eax, (dump_sect_bp + 8 * 4)

/* for MSR dump */

/*save MSR_IA32_SYSENTER_CS*/
xor %ecx,%ecx
mov $0x00000174,%ecx
rdmsr
mov %edx,(dump_sect_bp + MSR_BASE * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 1) * 4)

/*save MSR_IA32_SYSENTER_ESP*/
xor %ecx,%ecx
mov $0x00000175,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 2) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 3) * 4)

/*save MSR_IA32_SYSENTER_EIP*/
xor %ecx,%ecx
mov $0x00000176,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 4) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 5) * 4)

/*save MSR_IA32_EFER*/
xor %ecx,%ecx
mov $0xC0000080,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 6) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 7) * 4)

/*save MSR_IA32_STAR*/
xor %ecx,%ecx
mov $0xC0000081,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 8) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 9) * 4)

/*save MSR_IA32_LSTAR*/
xor %ecx,%ecx
mov $0xC0000082,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 10) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 11) * 4)

/*save MSR_IA32_CSTAR*/
xor %ecx,%ecx
mov $0xC0000083,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 12) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 13) * 4)

/*save MSR_IA32_FMASK*/
xor %ecx,%ecx
mov $0xC0000084,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 14) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 15) * 4)


/*save MSR_IA32_FS_BASE*/
xor %ecx,%ecx
mov $0xC0000100,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 16) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 17) * 4)

/*save MSR_IA32_GS_BASE*/
xor %ecx,%ecx
mov $0xC0000101,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 18) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 19) * 4)

/*save IA32_KERNEL_GS_BASE*/
xor %ecx,%ecx
mov $0xC0000102,%ecx
rdmsr
mov %edx,(dump_sect_bp + (MSR_BASE + 20) * 4)
mov %eax,(dump_sect_bp + (MSR_BASE + 21) * 4)

/* for GDT/LDT and the tables */
/*save gdtr, PE mode: 32bit + 16 bit = 6 bytes*/
sgdt (dump_sect_bp + (GDTR_BASE * 4))

/*save ldtr, just save the segment selector */
sldt (dump_sect_bp + (GDTR_BASE + 3) * 4)

/* save the GDT info */
mov (dump_sect_bp + (GDTR_BASE * 4)), %ecx
inc %ecx
and $0x0000FFFF, %ecx
mov (dump_sect_bp + (GDTR_BASE * 4) + 2), %esi
mov $(dump_sect_bp + (GDT_BASE * 4)), %edi

/* check rep args in dump info */
mov %ecx,(dump_sect_bp + (GDTR_BASE + 4) * 4)
mov %esi,(dump_sect_bp + (GDTR_BASE + 5) * 4)
mov %edi,(dump_sect_bp + (GDTR_BASE + 6) * 4)

rep movsb

/*access check: check ds write*/
mov $DUMP_MAGIC,%eax
mov %eax,%ds:0x100
/*check ds read*/
mov %ds:0x100 ,%eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 0) * 4)

/*check es write*/
mov $DUMP_MAGIC,%eax
mov %eax,%es:0x110
/*check es read*/
mov %es:0x110 ,%eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 1) * 4)

/*check fs write*/
mov $DUMP_MAGIC,%eax
mov %eax,%fs:0x120
/*check fs read*/
mov %fs:0x120 ,%eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 2) * 4)

/*check gs write*/
mov $DUMP_MAGIC,%eax
mov %eax,%gs:0x130
/*check gs read*/
mov %gs:0x130 ,%eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 3) * 4)

/*check ss write*/
mov $DUMP_MAGIC,%eax
mov %eax,%ss:0x140
/*check ss read*/
mov %ss:0x140 ,%eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 4) * 4)

/*check cs read*/
mov %cs:0x100, %eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 5) * 4)

mov 0, %eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 6) * 4)

mov 0 + 4, %eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 7) * 4)

mov 0 + 8, %eax
mov %eax,(dump_sect_bp + (ACCESS_BASE + 8) * 4)

#if 0
/*check limit*/
mov %cs:0xFFFFFFFF,%eax
mov %ds:0xFFFFFFFF,%eax
mov %es:0xFFFFFFFF,%eax
mov %fs:0xFFFFFFFF,%eax
mov %gs:0xFFFFFFFF,%eax
mov %ss:0xFFFFFFFF,%eax

#endif
