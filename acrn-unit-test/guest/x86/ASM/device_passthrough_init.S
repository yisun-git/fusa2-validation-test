/*AP USB BAR0*/
mov $0x8000A010,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7010)

/*AP USB BAR1*/
mov $0x8000A014,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7014)

/*AP ETH BAR0*/
mov $0x8000FE10,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x7018)

/*AP ETH BAR1*/
mov $0x8000FE14,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFC,%edx
in (%dx),%eax
mov %eax,(0x701c)

/*AP USB STATUS(offset 6 byte 2) */
mov $0x8000A006,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x7028)

/*AP ETH STATUS(offset 6 byte 2) */
mov $0x8000FE06,%eax
mov $0xCF8,%edx
out %eax,(%dx)
/*Read PCI_CFG_DATA*/
mov $0xCFE,%edx
in (%dx),%ax
/*Save pci device config register value*/
movzwl %ax,%eax
mov %eax,(0x702c)
