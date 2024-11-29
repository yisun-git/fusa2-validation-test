#include "../pci_check.h"
#include "../../lib/e1000_def.h"
#include "../../lib/e1000_regs.h"
#include "../../lib/pci_regs.h"
#define PCI_CONFIG_ADDR		0xCF8U
#define PCI_CONFIG_DATA		0xCFCU
// USB BDF=00:1.0
#define USB_BDF_ENABLE		0x80000800U
// IVSHMEM BDF=00:10.0
#define IVSHMEM_BDF_ENABLE	0x80008000U

// test PCI port address
mov $(PCI_CONFIG_ADDR), %edx
inl (%dx), %eax
mov %eax, (BP_IO_PORT_ADDR)

// Passthrough USB to non-safet VM
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

// Read USB device MSI FLAG
mov $(USB_BDF_ENABLE), %eax
add $0x80, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_MSI_CTRL_ADDR)

// Read USB device  Status and Command
mov $(USB_BDF_ENABLE), %eax
add $(PCI_COMMAND), %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_STATUS_COMMAND_ADDR)

// Read IVSHMEM device MSIX FLAG
mov $(IVSHMEM_BDF_ENABLE), %eax
add $0x40, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
// Read PCI_CFG_DATA
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_IVSHMEM_MSIX_CTRL_ADDR)
