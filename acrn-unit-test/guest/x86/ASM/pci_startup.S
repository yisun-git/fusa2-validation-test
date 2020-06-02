/*test PCI port address*/
mov $0xCF8, %edx
inl (%dx), %eax
mov %eax, (0x7000)

/*Read Net device 00:1f.06 interrupt line.*/
mov $0x8000FC3C, %eax
mov $0xCF8, %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC, %edx
inb (%dx), %al
/*Save pci device config register value*/
mov %eax, (0x8000)

/*Read USB device 00:14.00 interrupt line.*/
mov $0x8000A03C, %eax
mov $0xCF8, %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC, %edx
inb (%dx), %al
/*Save pci device config register value*/
mov %eax, (0x9000)
