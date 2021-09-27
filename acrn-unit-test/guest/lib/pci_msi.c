#include "pci_util.h"
#include "pci_msi.h"
#include "alloc.h"
#include "./x86/isr.h"
#include "./x86/processor.h"

//#define PCI_MSI_DEBUG
#ifdef PCI_MSI_DEBUG
#define MSI_DEBUG_LEVEL		(DBG_LVL_INFO)
#else
#define MSI_DEBUG_LEVEL		(DBG_LVL_ERROR)
#endif

#define MSI_MESSAGE_CTRL_ENABLE		SHIFT_LEFT(1, 0)
#define MSI_MESSAGE_CTRL_64_BIT_ADDR_CAPABLE	SHIFT_LEFT(1, 7)

static void pci_msi_parse_capability(struct pci_dev *pdev, struct msi_capability *cap);
static void pci_msi_create(struct pci_dev *pdev);
static void pci_msi_destroy(struct pci_dev *pdev);
static int pci_msi_enable(struct pci_dev *pdev, bool flag);
static int pci_msi_config(struct pci_dev *pdev, uint64_t addr, uint16_t data);
static int pci_msi_probe(struct pci_dev *pdev);
static void pci_msi_irq_start(struct pci_dev *pdev, void *irq_handler);

static struct pci_msi_ops msi_ops_obj = {
	.msi_create = pci_msi_create,
	.msi_destroy = pci_msi_destroy,
	.msi_probe = pci_msi_probe,
	.msi_enable = pci_msi_enable,
	.msi_config = pci_msi_config,
	.msi_irq_start = pci_msi_irq_start
};

void pci_msi_init(struct pci_dev *pdev)
{
		set_log_level(MSI_DEBUG_LEVEL);
		pdev->msi_ops = &msi_ops_obj;
		pdev->msi_ops->msi_create(pdev);
}

static void pci_msi_irq_start(struct pci_dev *pdev, void *irq_handler)
{
	void (*_irq_handler)(isr_regs_t *regs) = irq_handler;
	struct msi *m = (struct msi *)(pdev->msi);
	if (NULL == pdev || NULL == irq_handler) {
		DBG_ERRO("Error pdev = %p, irq_handler = %p", pdev, irq_handler);
		return;
	}
	pci_msi_parse_capability(pdev, &(m->msi_cap));
	int vector = (m->msi_cap.message_data) & 0xFF;
	DBG_INFO("vector = %d ", vector);
	handle_irq(vector, _irq_handler);
	irq_enable();
}

static void pci_msi_parse_capability(struct pci_dev *pdev, struct msi_capability *cap)
{
	uint32_t reg_addr = 0U;
	struct msi *m = (struct msi *)(pdev->msi);
	uint8_t msi_io_start = m->msi_io_start;
	reg_addr = msi_io_start + PCI_MSI_FLAGS;
	cap->ctrl = pci_pdev_read_cfg(pdev->bdf, reg_addr, 2);

	reg_addr = msi_io_start + PCI_MSI_ADDRESS_LO;
	cap->message_addr_l = pci_pdev_read_cfg(pdev->bdf, reg_addr, 4);

	if (MSI_MESSAGE_CTRL_64_BIT_ADDR_CAPABLE & cap->ctrl) {
		reg_addr = msi_io_start + PCI_MSI_ADDRESS_HI;
		cap->message_addr_h = pci_pdev_read_cfg(pdev->bdf, reg_addr, 4);
		reg_addr = msi_io_start + PCI_MSI_DATA_64;
		cap->message_data = pci_pdev_read_cfg(pdev->bdf, reg_addr, 2);
	} else {
		reg_addr = msi_io_start + PCI_MSI_DATA_32;
		cap->message_data = pci_pdev_read_cfg(pdev->bdf, reg_addr, 2);
	}
}

static int pci_msi_probe(struct pci_dev *pdev)
{
	union pci_bdf bdf = pdev->bdf;
	uint32_t reg_val = 0U;
	uint32_t reg_addr = 0U;
	int ret = OK;
	int i = 0;
	int repeat = 0;

	if (NULL == pdev) {
		DBG_ERRO("pdev null");
		return -1;
	}

	reg_addr = PCI_CAPABILITY_LIST;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 1);

	reg_addr = reg_val;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);

	/*
	 * pci standard config space [00h,100h).So,MSI register must be in [00h,100h),
	 * if there is MSI register
	 */
	repeat = 0x100 / 4;
	for (i = 0; i < repeat; i++) {
		/*Find MSI register group*/
		if (PCI_CAP_ID_MSI == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}

	struct msi *m = (struct msi *)(pdev->msi);
	m->msi_io_start = 0;
	if (repeat == i) {
		DBG_ERRO("Didn't find MSI function");
		ret = ERROR;
	} else {
		DBG_INFO("Did find MSI function at %02xH", reg_addr);
		struct msi *m = (struct msi *)(pdev->msi);
		m->msi_io_start = reg_addr;
	}

	return ret;
}

static void pci_msi_create(struct pci_dev *pdev)
{
	if (NULL == pdev) {
		DBG_ERRO("pdev null");
		return;
	}
	static struct msi cur_msi;
	struct msi *msi_obj = &cur_msi;
	if (NULL == msi_obj) {
		DBG_ERRO("msi_obj NULL failed");
		return;
	}
	pdev->msi = msi_obj;
	return;
}

static void pci_msi_destroy(struct pci_dev *pdev)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = pdev->bdf;
	struct msi *m = (struct msi *)(pdev->msi);
	if (NULL == pdev) {
		DBG_ERRO("pdev null");
		return;
	}
	reg_addr = m->msi_io_start + PCI_MSI_FLAGS;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
	reg_val &= ~MSI_MESSAGE_CTRL_ENABLE;
	pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
	//free(pdev->msi);
	pdev->msi = NULL;
}

static int pci_msi_enable(struct pci_dev *pdev, bool flag)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = pdev->bdf;
	struct msi *m = (struct msi *)(pdev->msi);
	if (NULL == pdev) {
		DBG_ERRO("pdev null");
		return -1;
	}
	reg_addr = m->msi_io_start + PCI_MSI_FLAGS;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
	if (flag) {
		reg_val |= MSI_MESSAGE_CTRL_ENABLE;
	} else {
		reg_val &= ~MSI_MESSAGE_CTRL_ENABLE;
	}
	pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
	DBG_INFO("Config msi controller register[%02xH] as [%04xH]", reg_addr, reg_val);
	return 0;
}

static int pci_msi_config(struct pci_dev *pdev, uint64_t addr, uint16_t data)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = 0U;
	uint32_t ctrl = 0U;
	union pci_bdf bdf = pdev->bdf;
	struct msi *m = (struct msi *)(pdev->msi);
	uint8_t msi_io_start = m->msi_io_start;

	if (NULL == pdev) {
		DBG_ERRO("pdev null");
		return -1;
	}

	reg_addr = msi_io_start + PCI_MSI_FLAGS;
	ctrl = pci_pdev_read_cfg(bdf, reg_addr, 2);
	/*message-address register _L and _H*/
	reg_addr = msi_io_start + PCI_MSI_ADDRESS_LO;
	reg_val = (addr) & 0xFFFFFFFFUL;
	pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	DBG_INFO("Config msi address register[%02xH] as [%08xH] \n", reg_addr, reg_val);

	if (MSI_MESSAGE_CTRL_64_BIT_ADDR_CAPABLE & ctrl) {
		reg_addr = msi_io_start + PCI_MSI_ADDRESS_HI;
		reg_val = ((addr) >> 32) & 0xFFFFFFFFUL;
		pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		DBG_INFO("Config msi address upper register[%02xH] as [%08xH] \n", reg_addr, reg_val);

		/*message-data register*/
		reg_addr = msi_io_start + PCI_MSI_DATA_64;
		reg_val = data;
		pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
		DBG_INFO("Config msi data register[%02xH] as [%04xH]", reg_addr, reg_val);
	} else {
		/*message-data register*/
		reg_addr = msi_io_start + PCI_MSI_DATA_32;
		reg_val = data;
		pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
		DBG_INFO("Config msi data register[%02xH] as [%04xH]", reg_addr, reg_val);
	}
	return 0;
}