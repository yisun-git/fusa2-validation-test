#ifndef __PCI_TYPE_H__
#define __PCI_TYPE_H__
#include "libcflat.h"

#define OK	(0)
#define ERROR	(-1)

#define IN
#define OUT
#define INOUT

#define PUBLIC
#define PROTECT
#define PRIVATE

#ifndef NULL
#define NULL	(void *)(0)
#endif

#ifdef __x86_64__
#define LD	"ld"
#define LX	"lx"
#define ptr_width	uint64_t
#define mem_size	uint64_t
#else
#define LD	"lld"
#define LX	"llx"
#define ptr_width	uint32_t
#define mem_size	uint32_t
#endif

#define ELEMENT_NUM(array)	sizeof(array) / sizeof(array[0])

union pci_bdf {
	uint16_t value;
	struct {
		uint8_t f : 3; /* BITs 0-2 */
		uint8_t d : 5; /* BITs 3-7 */
		uint8_t b; /* BITs 8-15 */
	} bits;
};

#define PCI_MAKE_BDF(name, bb, dd, ff) union pci_bdf  _##name##_bdf = {\
	.bits = {\
	.b = (bb),\
	.d = (dd),\
	.f = (ff)\
	} \
}

#define PCI_GET_BDF(name) (_##name##_bdf)

#define PCI_BAR_COUNT (0x6U)

struct pci_dev_ops;
struct pci_msi_ops;
struct pci_dev {
	union pci_bdf bdf;
	uint16_t vender_id;
	uint16_t device_id;
	#define DEV_VEN(device_id, vender_id)	((vender_id) | ((device_id) << 16))
	uint32_t nr_bars;
	uint32_t mmio_start[PCI_BAR_COUNT];
	uint32_t mmio_end[PCI_BAR_COUNT];
	#define PCI_DEV_BAR(dev, n)		(dev)->mmio_start[(n)]
	#define PCI_DEV_BAR_END(dev, n)		(dev)->mmio_end[(n)]
	#define PCI_DEV_NR_BARS(dev)	(dev)->nr_bars
	uint8_t msi_vector;
	void *msi;
	struct pci_dev_ops *dev_ops;
	struct pci_msi_ops *msi_ops;
};

struct pci_dev_ops {
	void (*init_dev)(struct pci_dev *dev);
	void (*deinit_dev)(struct pci_dev *dev);
	int32_t (*probe_dev)(struct pci_dev *dev);
	int32_t (*write_dev_cfg)(struct pci_dev *dev, uint32_t offset, uint32_t bytes, uint32_t val);
	int32_t (*read_dev_cfg)(const struct pci_dev *dev, uint32_t offset, uint32_t bytes, uint32_t *val);
};
#endif