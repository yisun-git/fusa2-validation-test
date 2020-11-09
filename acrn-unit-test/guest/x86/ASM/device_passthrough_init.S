#ifdef IN_NON_SAFETY_VM
/*AP USB BAR0*/
mov $0x80000810,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7010)

/*AP USB BAR1*/
mov $0x80000814,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7014)
#endif

#ifdef IN_SAFETY_VM
/*AP ETH BAR0*/
mov $0x80000810,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7018)

/*AP ETH BAR1*/
mov $0x80000814,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x701c)
#endif

#ifdef IN_NON_SAFETY_VM
/*AP USB STATUS(offset 6 byte 2) */
mov $0x80000806,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x7028)
#endif

#ifdef IN_SAFETY_VM
/*AP ETH STATUS(offset 6 byte 2) */
mov $0x80000806,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x702c)
#endif
