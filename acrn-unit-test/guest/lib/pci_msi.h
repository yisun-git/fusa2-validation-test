#ifndef __PCI_MSI_H__
#define __PCI_MSI_H__
#include "libcflat.h"
#include "pci_type.h"

struct msi_capability {
	uint16_t ctrl;
	uint16_t message_data;
	uint32_t message_addr_l;
	uint32_t message_addr_h;
};

struct msi {
	uint8_t msi_io_start;
	struct msi_capability msi_cap;
};

struct pci_msi_ops {
	void (*msi_create)(struct pci_dev *pdev);
	void (*msi_destroy)(struct pci_dev *pdev);
	int (*msi_probe)(struct pci_dev *pdev);
	int (*msi_enable)(struct pci_dev *pdev, bool flag);
	int (*msi_config)(struct pci_dev *pdev, uint64_t addr, uint16_t data);
	void (*msi_irq_start)(struct pci_dev *pdev, void *irq_handler);
};

#define PCI_MSI_PROBE(dev)		(dev)->msi_ops->msi_probe(dev)
#define PCI_MSI_ENABLE(dev, flag)		(dev)->msi_ops->msi_enable(dev, flag)
#define PCI_MSI_CONFIG(dev, addr, data)		(dev)->msi_ops->msi_config(dev, addr, data)
#define PCI_MSI_IRQ_START(dev, handler)		(dev)->msi_ops->msi_irq_start(dev, handler)

void pci_msi_init(struct pci_dev *pdev);
#endif