#include "../pci_check.h"
#include "../../lib/e1000_def.h"
#include "../../lib/e1000_regs.h"
#include "../../lib/pci_regs.h"
#define PCI_CONFIG_ADDR		0xCF8U
#define PCI_CONFIG_DATA		0xCFCU
// USB BDF=00:1.0
#define USB_BDF_ENABLE		0x80000800U
// NET BDF=00:1.0
#define NET_BDF_ENABLE		0x80000800U

// test PCI port address
mov $(PCI_CONFIG_ADDR), %edx
inl (%dx), %eax
mov %eax, (BP_IO_PORT_ADDR)

// Passthrough Ethernet to safet VM
#ifdef IN_SAFETY_VM
// Read Net device interrupt line
// Format address BDF + offset
mov $(NET_BDF_ENABLE), %eax
add $(PCI_INTERRUPT_LINE), %eax
// Write port address
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inb (%dx), %al
// Save pci device config register value
mov %eax, (BP_NET_INTERRUP_LINE_ADDR)
#endif

// Passthrough USB to non-safet VM
#ifdef IN_NON_SAFETY_VM
// Read USB device interrupt line
// Format address BDF + offset
mov $(USB_BDF_ENABLE), %eax
add $(PCI_INTERRUPT_LINE), %eax
// Write port address
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inb (%dx), %al
// Save pci device config register value
mov %eax, (BP_USB_INTERRUP_LINE_ADDR)
#endif

#ifdef IN_SAFETY_VM
// Read Net device BAR0
mov $NET_BDF_ENABLE, %eax
add $(PCI_BASE_ADDRESS_0), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_BAR0_ADDR)

// Read Device CTRL Register in BAR0 space by MMIO
and $0xFFFFFFF0, %eax
add $0x8, %eax
mov (%eax), %ecx
mov %ecx, (BP_NET_DEVICE_STATUS_ADDR)
#endif

#ifdef IN_NON_SAFETY_VM
// Read USB device BAR0
mov $(USB_BDF_ENABLE), %eax
add $(PCI_BASE_ADDRESS_0), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_BAR0_ADDR)
// Read USB HCIVERSION in BAR0 space
and $0xFFFFFFF0, %eax
add $(HCIVERSION), %eax
mov (%eax), %cx
mov %ecx, (BP_USB_DEVICE_HCIVERSION_ADDR)
#endif

#ifdef IN_SAFETY_VM
// Read Net device MSI FlAG
mov $(NET_BDF_ENABLE), %eax
add $0xD0, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_MSI_CTRL_ADDR)
#endif

#ifdef IN_NON_SAFETY_VM
// Read USB device MSI FLAG
mov $(USB_BDF_ENABLE), %eax
add $0x80, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_MSI_CTRL_ADDR)
#endif

#ifdef IN_SAFETY_VM
// Read Net device Status and Command
mov $(NET_BDF_ENABLE), %eax
add $(PCI_COMMAND), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_STATUS_COMMAND_ADDR)
// Write command.master|command.memory to cammand register
mov $(NET_BDF_ENABLE), %eax
add $(PCI_COMMAND), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
mov (BP_NET_STATUS_COMMAND_ADDR), %eax
or $0x0406, %ax
outw %eax, (%dx)
#endif


#ifdef IN_SAFETY_VM
// Read Net device Status and Command
mov $(NET_BDF_ENABLE), %eax
add $(PCI_COMMAND), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_STATUS_COMMAND_ADDR)
#endif

#ifdef IN_NON_SAFETY_VM
// Read USB device  Status and Command
mov $(USB_BDF_ENABLE), %eax
add $(PCI_COMMAND), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_STATUS_COMMAND_ADDR)
#endif
