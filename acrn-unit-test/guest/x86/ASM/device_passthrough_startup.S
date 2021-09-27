#ifdef IN_NON_SAFETY_VM
/*BP USB BAR0*/
mov $0x80000810,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7000)

/*BP USB BAR1*/
mov $0x80000814,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7004)
#endif

#ifdef IN_SAFETY_VM
/*BP ETH BAR0*/
mov $0x80000810,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7008)

/*BP ETH BAR1*/
mov $0x80000814,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x700c)
#endif

#ifdef IN_NON_SAFETY_VM
/*BP USB STATUS(offset 6 byte 2) */
mov $0x80000806,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x7020)
#endif

#ifdef IN_SAFETY_VM
/*BP ETH STATUS(offset 6 byte 2) */
mov $0x80000806,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x7024)
#endif
