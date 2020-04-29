/*
 * Enable e1000e MSI function
 * NUC e1000e MAC is 82574IT ethernet controller
 * NET framework is  82574IT_MAC + I219_phy
 * chunlin.wang
 *
 * MSI route
 * E1000e init -> E1000e trigger software -> MSI -> FSB -> localAPIC -> Core
 *
 */
#include "libcflat.h"
#include "./asm/io.h"
#include "./e1000_regs.h"
#include "./e1000_def.h"
#include "./x86/processor.h"
#include "./x86/apic.h"
#include "./x86/desc.h"
#include "./x86/isr.h"
#include "./e1000e.h"
#include "./pci_util.h"
#include "./pci_msi.h"

//#define E1000_DEBUG
#ifdef E1000_DEBUG
#define E1000_DEBUG_LEVEL		(DBG_LVL_INFO)
#else
#define E1000_DEBUG_LEVEL		(DBG_LVL_ERROR)
#endif

/*Disable E1000e PCI master function*/
#define DIS_MASTER                  0

/*Dump localAPIC ISR register in the interrupt context*/
#define DUMP_ISR                    0

/*Test localAPIC->Core interrupt response is ok*/
#define INT_TEST                    0

#define MSR_WR_LEGACY               0
#if MSR_WR_LEGACY
#ifndef APIC_BASE_MSR
#define APIC_BASE_MSR	            0x800
#endif
#endif

#define BIT_SET(x)                  (1 << (x))

#define MSI_VECTOR                  0x40

#define MSI_SIMULATE                1

#define NET_BAR_DEFAULT             0xDF200000

#define PCI_PCIR_BAR(x)             (0x10 + (x) * 4)
#define	PCIR_CAP_PTR	            0x34
#define ETH_ALEN	                6		/* Octets in one ethernet addr	 */

/* Message Signalled Interrupt registers */
#define PCI_MSI_FLAGS		2	/* Message Control */
#define PCI_MSI_FLAGS_ENABLE	0x0001	/* MSI feature enabled */
#define PCI_MSI_FLAGS_QMASK	0x000e	/* Maximum queue size available */
#define PCI_MSI_FLAGS_QSIZE	0x0070	/* Message queue size configured */
#define PCI_MSI_FLAGS_64BIT	0x0080	/* 64-bit addresses allowed */
#define PCI_MSI_FLAGS_MASKBIT	0x0100	/* Per-vector masking capable */
#define PCI_MSI_RFU		3	/* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO	4	/* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI	8	/* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32		8	/* 16 bits of data for 32-bit devices */
#define PCI_MSI_MASK_32		12	/* Mask bits register for 32-bit devices */
#define PCI_MSI_PENDING_32	16	/* Pending intrs for 32-bit devices */
#define PCI_MSI_DATA_64		12	/* 16 bits of data for 64-bit devices */
#define PCI_MSI_MASK_64		16	/* Mask bits register for 64-bit devices */
#define PCI_MSI_PENDING_64	20	/* Pending intrs for 64-bit devices */

/*Apic relation-ship*/
#define MSR_IA32_EXT_XAPICID			0x00000802U
#define MSR_IA32_EXT_APIC_SIVR			0x0000080FU
#define MSR_IA32_EXT_APIC_ISR0			0x00000810U
#define MSR_IA32_EXT_APIC_ISR7			0x00000817U
#define MSR_IA32_APIC_BASE			    0x0000001BU
#define MSR_IA32_EXT_APIC_LVT_CMCI		0x0000082FU
#define MSR_IA32_EXT_APIC_ICR			0x00000830U
#define MSR_IA32_EXT_APIC_LVT_TIMER		0x00000832U
#define MSR_IA32_EXT_APIC_LVT_THERMAL	0x00000833U
#define MSR_IA32_EXT_APIC_LVT_PMI		0x00000834U
#define MSR_IA32_EXT_APIC_LVT_LINT0		0x00000835U
#define MSR_IA32_EXT_APIC_LVT_LINT1		0x00000836U
#define MSR_IA32_EXT_APIC_LVT_ERROR		0x00000837U
#define LAPIC_LVT_MASK			  0x00010000U
#define MSR_IA32_EXT_APIC_SIVR			0x0000080FU
#define LAPIC_SVR_VECTOR			0x000000FFU
#define LAPIC_SVR_APIC_ENABLE_MASK		   0x00000100U
#define MSR_IA32_EXT_APIC_EOI			0x0000080BU

/* intr_lapic_icr_dest_mode */
#define INTR_LAPIC_ICR_PHYSICAL 0x0U
/* intr_lapic_icr_level */
#define INTR_LAPIC_ICR_DEASSERT  0x0U
#define INTR_LAPIC_ICR_ASSERT       0x1U

#define INTR_LAPIC_ICR_LEVEL	0x1U

/*Daisy chain mode*/
typedef struct link_head {
	#if 0/*Not use malloc*/
	void *p_pre;
	void *p_next;/*End of NULL*/
	#endif
	int_func int_ck;
	void *p_arg;
} LINK_HEAD, *P_LINK_HEAD;

typedef struct e1000e_dev {
	uint8_t init_flag;
	struct pci_dev pci_obj;
	uint8_t mac_addr[ETH_ALEN];
	uint8_t perm_addr[ETH_ALEN];
	P_LINK_HEAD p_int_func;
	uint64_t int_cnt;
	uint32_t icr_crt;
} E1000E_DEV, *P_E1000E_DEV;

union lapic_base_msr {
	uint64_t value;
	struct {
		uint32_t rsvd_1:8;
		uint32_t bsp:1;
		uint32_t rsvd_2:1;
		uint32_t x2APIC_enable:1;
		uint32_t xAPIC_enable:1;
		uint32_t lapic_paddr:24;
		uint32_t rsvd_3:28;
	} fields;
};

/* x2APIC Interrupt Command Register (ICR) structure */
union apic_icr {
	uint64_t value;
	struct {
		uint32_t lo_32;
		uint32_t hi_32;
	} value_32;
	struct {
		uint32_t vector:8;
		uint32_t delivery_mode:3;
		uint32_t destination_mode:1;
		uint32_t rsvd_1:2;
		uint32_t level:1;
		uint32_t trigger_mode:1;
		uint32_t rsvd_2:2;
		uint32_t shorthand:2;
		uint32_t rsvd_3:12;
		uint32_t dest_field:32;
	} bits;
};

/* Enables interrupts on the current CPU */
#define CPU_IRQ_ENABLE()\
{\
	asm volatile ("sti\n" : : : "cc");\
}

#define __readl(reg)	*(uint32_t *)(uint64_t)(reg)
static inline uint32_t __er32(P_E1000E_DEV p_e1000e_dev, uint32_t reg)
{
	struct pci_dev *pdev = &(p_e1000e_dev->pci_obj);
	return __readl(PCI_DEV_BAR(pdev, 0) + reg);
}
#define er32(reg)	__er32(p_e1000e_dev, E1000_##reg)

#define __writel(reg, val)	*(uint32_t *)(uint64_t)(reg) = val;
static inline void __ew32(P_E1000E_DEV p_e1000e_dev, uint32_t reg, uint32_t val)
{
	struct pci_dev *pdev = &(p_e1000e_dev->pci_obj);
	__writel(PCI_DEV_BAR(pdev, 0) + reg, val);
}
#define ew32(reg, val)	__ew32(p_e1000e_dev, E1000_##reg, (val))

#define e1e_flush()	do {\
	__unused uint32_t tmp;\
	tmp = er32(STATUS);\
} while (0)

static int e1000e_probe(P_E1000E_DEV p_e1000e_dev);
static void e1000e_read_mac_addr(P_E1000E_DEV p_e1000e_dev);
static int pci_card_detect(P_E1000E_DEV p_e1000e_dev);
static void e1000e_hw_reset(P_E1000E_DEV p_e1000e_dev);
static void e1000e_int_enabe(P_E1000E_DEV p_e1000e_dev);
static uint32_t pci_resource_start(P_E1000E_DEV p_e1000e_dev, uint8_t bar);
static int e1000e_msi_probe(P_E1000E_DEV p_e1000e_dev);
static int e1000e_msi_config_enable(P_E1000E_DEV p_e1000e_dev, uint64_t message_addr, uint32_t message_data);
static inline uint64_t msr_read(uint32_t reg_num);
static inline void msr_write(uint32_t reg_num, uint64_t value64);
static void early_init_lapic(void);
static void init_lapic(void);
static void clear_lapic_isr(void);
#if DUMP_ISR
static void dump_lapic_isr(void);
#endif
static int pci_enable_dev(P_E1000E_DEV p_e1000e_dev);
#if DIS_MASTER
static int e1000e_disable_pcie_master(P_E1000E_DEV p_e1000e_dev);
#endif
static int e1000e_msi_handler_connect(P_E1000E_DEV p_e1000e_dev);
#if INT_TEST
static int lapic_send_ipi(P_E1000E_DEV p_e1000e_dev);
#endif
static void msi_irq(isr_regs_t *regs);
static int msi_int_end(__unused void *p_arg);
static int msi_int_handler_demo(void *p_arg);

static LINK_HEAD int_func_t[] = {
	{
		.int_ck = (int_func)NULL,
		.p_arg = NULL,
	},
	{
		.int_ck = (int_func)NULL,
		.p_arg = NULL,
	},
	{
		.int_ck = (int_func)NULL,
		.p_arg = NULL,
	},
	{
		.int_ck = (int_func)NULL,
		.p_arg = NULL,
	},

	/*End*/
	{
		.int_ck = (int_func)msi_int_end,
		.p_arg = NULL,
	},
};
static E1000E_DEV e1000e_dev = {
	.init_flag = 0,
	.pci_obj = {
	  .bdf = {
	      .bits = {
					.b = 0x00,
					.d = 0x1f,
					.f = 0x6,
	      }
	  },
	  .nr_bars = 0,
	  .msi_vector = MSI_VECTOR,
	  .msi = NULL,
	},
	.p_int_func = int_func_t,
	.int_cnt = 0,
};

int msi_int_demo(uint32_t msi_trigger_num)
{
	uint32_t num = msi_trigger_num;

	msi_int_connect(msi_int_handler_demo, (void *)1);
	msi_int_connect(msi_int_handler_demo, (void *)2);
	msi_int_connect(msi_int_handler_demo, (void *)3);
	msi_int_connect(msi_int_handler_demo, (void *)4);

	e1000e_init();
	uint32_t lapic_id = apic_id();
	uint64_t msi_msg_addr = MAKE_NET_MSI_MESSAGE_ADDR(0xFFE00000, lapic_id);
	uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(0x40);
	e1000e_msi_config_enable(&e1000e_dev, msi_msg_addr, msi_msg_data);
	while (num) {
		num -= 1;
		msi_int_simulate();
		delay_loop_ms(1000);
		printf("[ int ]MSI int cnt:%ld\r\n", msi_int_cnt_get());
	}

	return 0;
}

void e1000e_msi_reset(void)
{
	struct pci_dev *pdev = &(e1000e_dev.pci_obj);
	PCI_MSI_ENABLE(pdev, 0);
	PCI_MSI_CONFIG(pdev, 0UL, 0xFFU);
}

void e1000e_msi_config(uint64_t msi_msg_addr, uint32_t msi_msg_data)
{
	struct pci_dev *pdev = &(e1000e_dev.pci_obj);
	PCI_MSI_CONFIG(pdev, msi_msg_addr, msi_msg_data);
	return;
}

int e1000e_init(void)
{
	int ret = 0;
	uint32_t size = 0;
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	struct pci_dev *pdev = &(e1000e_dev.pci_obj);

	set_log_level(E1000_DEBUG_LEVEL);
	ret = pci_card_detect(p_e1000e_dev);
	if (ret == -1) {
		printf("\ne1000e pci_card_detect failed \n");
		return -1;
	}
	if (p_e1000e_dev->init_flag) {
		return 0;
	}
	p_e1000e_dev->init_flag = !p_e1000e_dev->init_flag;

	printf("Enter %s/%s   %s  %s\r\n", __FILE__, __FUNCTION__, __DATE__, __TIME__);

	/*
	 * 0. Core interrupt handler
	 */
	setup_idt();
	pci_msi_init(pdev);
	/*
	 * 1. localAPIC init
	 */
	early_init_lapic();
	init_lapic();
	clear_lapic_isr();

	/*
	 * 2. device init
	 */
	if (ERROR == e1000e_probe(p_e1000e_dev)) {
		DBG_ERRO("Probe e1000e device is failed");
		return -1;
	}
	delay_loop_ms(1);

	PCI_DEV_BAR(pdev, 0) = pci_resource_start(p_e1000e_dev, 0);
	size = pci_pdev_get_bar_size(pdev->bdf, 0);
	DBG_INFO("bar 0 size = 0x%x ", size);
	PCI_DEV_BAR_END(pdev, 0) = PCI_DEV_BAR(pdev, 0) + size;
	PCI_DEV_NR_BARS(pdev) = 1;
	DBG_INFO("E1000E mem at %xH", PCI_DEV_BAR(pdev, 0));

	e1000e_hw_reset(p_e1000e_dev);

	/*Read MAC address to comfirm the device is reset seccessed*/
	e1000e_read_mac_addr(p_e1000e_dev);

	/*
	 * 3. msi init
	 */
	pci_enable_dev(p_e1000e_dev);
	e1000e_msi_probe(p_e1000e_dev);

#if INT_TEST
	/*test localAPIC code*/
	lapic_send_ipi(p_e1000e_dev);
#endif

	return 0;
}

void e1000e_reset(void)
{
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	e1000e_hw_reset(p_e1000e_dev);
	/*Read MAC address to comfirm the device is reset seccessed*/
	e1000e_read_mac_addr(p_e1000e_dev);
	pci_enable_dev(p_e1000e_dev);
	e1000e_msi_reset();
}

int msi_int_connect(int_func int_ck, void *p_arg)
{
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	P_LINK_HEAD p_int_func;

	for (p_int_func = p_e1000e_dev->p_int_func ; msi_int_end != p_int_func->int_ck ; p_int_func++) {
		if ((int_func)NULL == p_int_func->int_ck) {
			p_int_func->int_ck = int_ck;
			p_int_func->p_arg = p_arg;
			return 0;
		}
	}
	return (-1);
}

void msi_int_simulate(void)
{
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	e1000e_msi_handler_connect(p_e1000e_dev);
	e1000e_int_enabe(p_e1000e_dev);
	do {
		ew32(ICS, BIT_SET(24) | BIT_SET(0));
		#if DUMP_ISR
		dump_lapic_isr();
		#endif
		e1e_flush();
		#if DUMP_ISR
		dump_lapic_isr();
		#endif
	} while (0);
}

uint64_t msi_int_cnt_get(void)
{
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	return p_e1000e_dev->int_cnt;
}

int delay_loop_ms(uint32_t ms)
{
	volatile unsigned int i, j;
	for (i = 0; i < ms; i++) {
		for (j = 0; j < 0x40000; j++) {
		}
	}
	return 0;
}

static int msi_int_end(__unused void *p_arg)
{
	/*TO DO:add check code*/
	return 0;
}

static void msi_irq(isr_regs_t *regs)
{
	P_E1000E_DEV p_e1000e_dev = (P_E1000E_DEV)&e1000e_dev;
	P_LINK_HEAD p_int_func;
	uint32_t reg_val;

	p_e1000e_dev->int_cnt++;

	/*DBG_INFO("\033[43mEnter %s\033[0m",__func__);*/

	reg_val = er32(ICR);
	/*DBG_INFO("Clear ICR register[%08xH]",reg_val);*/

	p_e1000e_dev->icr_crt = reg_val;

	/*Exe user interrupt function*/
	for (p_int_func = p_e1000e_dev->p_int_func; msi_int_end != p_int_func->int_ck; p_int_func++) {
		if ((int_func)NULL == p_int_func->int_ck) {
			break;
		}
		p_int_func->int_ck(p_int_func->p_arg);
	}

	#if DUMP_ISR
	dump_lapic_isr();
	#endif
	msr_write(MSR_IA32_EXT_APIC_EOI, 0U);
}

static int e1000e_probe(P_E1000E_DEV p_e1000e_dev)
{
	union pci_bdf bdf = p_e1000e_dev->pci_obj.bdf;
	uint32_t reg_val = 0U;

	/*Detect PCIe device*/
	pci_card_detect(p_e1000e_dev);

	/*Test device PCI space writable*/
	reg_val = pci_pdev_read_cfg(bdf, 0xD4, 4);
	DBG_INFO("Dev[%x:%xH:%d] reg[D4H] old %08xH)",
			bdf.bits.b, bdf.bits.d, bdf.bits.f, reg_val);
	pci_pdev_write_cfg(bdf, 0xD4, 4, 0xFEE00000);
	reg_val = pci_pdev_read_cfg(bdf, 0xD4, 4);
	if (0xFEE00000 != (reg_val & 0xFEE00000)) {
		DBG_WARN("Dev[%x:%xH:%d] is not writable(%08x)",
				bdf.bits.b, bdf.bits.d, bdf.bits.f, reg_val);
	}

	return 0;
}

static int pci_card_detect(P_E1000E_DEV p_e1000e_dev)
{
	union pci_bdf bdf = p_e1000e_dev->pci_obj.bdf;
	uint32_t reg_val = 0U;
	reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 4);
	if ((reg_val == 0xFFFFFFFFU) || (reg_val == 0U)
		|| (reg_val == 0xFFFF0000U) || (reg_val == 0xFFFFU)) {
		DBG_WARN("Not find pci card");
		return (-1);
	}
	DBG_INFO("Find pci card[ID:%08xH]", reg_val);
	reg_val = er32(STATUS);
	DBG_INFO("Status register is %x08H", reg_val);
	return 0;
}

static void e1000e_read_mac_addr(P_E1000E_DEV p_e1000e_dev)
{
	u32 rar_high = 0U;
	u32 rar_low = 0U;
	u16 i = 0;

	rar_high = er32(RAH(0));
	rar_low = er32(RAL(0));
	for (i = 0; i < E1000_RAL_MAC_ADDR_LEN; i++)
		p_e1000e_dev->perm_addr[i] = (u8)(rar_low >> (i * 8));

	for (i = 0; i < E1000_RAH_MAC_ADDR_LEN; i++)
		p_e1000e_dev->perm_addr[i + 4] = (u8)(rar_high >> (i * 8));

	for (i = 0; i < ETH_ALEN; i++)
		p_e1000e_dev->mac_addr[i] = p_e1000e_dev->perm_addr[i];

	DBG_INFO("MAC address:%xH-%xH-%xH-%xH-%xH-%xH",
					p_e1000e_dev->mac_addr[0],
					p_e1000e_dev->mac_addr[1],
					p_e1000e_dev->mac_addr[2],
					p_e1000e_dev->mac_addr[3],
					p_e1000e_dev->mac_addr[4],
					p_e1000e_dev->mac_addr[5]);
}

static void e1000e_hw_reset(P_E1000E_DEV p_e1000e_dev)
{
	uint32_t ctrl = 0U;
	uint32_t manc = 0U;

	#if DIS_MASTER
	e1000e_disable_pcie_master(p_e1000e_dev);
	#endif

	DBG_INFO("Masking off all interrupts");
	ew32(IMC, 0xffffffff);

	/* Disable the Transmit and Receive units.  Then delay to allow
	 * any pending transactions to complete before we hit the MAC with
	 * the global reset.
	 */
	ew32(RCTL, 0);
	ew32(TCTL, E1000_TCTL_PSP);
	e1e_flush();

	/* Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	delay_loop_ms(100);

	/* Issue a global reset to the MAC.  This will reset the chip's
	 * transmit, receive, DMA, and link units.  It will not effect
	 * the current PCI configuration.  The global reset bit is self-
	 * clearing, and should clear within a microsecond.
	 */
	DBG_INFO("Issuing a global reset to MAC");
	ctrl = er32(CTRL);
	ew32(CTRL, (ctrl | E1000_CTRL_RST));

	/* Wait for EEPROM reload (it happens automatically) */
	delay_loop_ms(50);

	/* Dissable HW ARPs on ASF enabled adapters */
	manc = er32(MANC);
	manc &= ~(E1000_MANC_ARP_EN);
	ew32(MANC, manc);

	ew32(IMC, 0xffffffff);
	/* Clear any pending interrupt events. */
	er32(ICR);
}

static uint32_t pci_resource_start(P_E1000E_DEV p_e1000e_dev, uint8_t bar)
{
	union pci_bdf bdf = p_e1000e_dev->pci_obj.bdf;
	uint32_t reg_val = 0U;

	reg_val = pci_pdev_read_cfg(bdf, PCI_PCIR_BAR(bar), 4);
	if (0x00 == reg_val) {
		DBG_WARN("E1000e BAR get failed");
		pci_pdev_write_cfg(bdf, PCI_PCIR_BAR(bar), 4, NET_BAR_DEFAULT);
		reg_val = pci_pdev_read_cfg(bdf, PCI_PCIR_BAR(bar), 4);
		if (0x00 == reg_val) {
			DBG_ERRO("E1000e BAR config error");
		}
	}

	return reg_val;
}

static void e1000e_int_enabe(P_E1000E_DEV p_e1000e_dev)
{
	uint32_t reg_val = 0U;
	ew32(IMS, BIT_SET(24) | BIT_SET(0));
	reg_val = er32(IMS);
	DBG_INFO("Set IMS %08xH", reg_val);
}

static int e1000e_msi_probe(P_E1000E_DEV p_e1000e_dev)
{
	struct pci_dev *pdev = &(p_e1000e_dev->pci_obj);
	return PCI_MSI_PROBE(pdev);
}

void e1000e_msi_enable(void)
{
	struct pci_dev *pdev = &(e1000e_dev.pci_obj);
	PCI_MSI_ENABLE(pdev, 1);
}

void e1000e_msi_disable(void)
{
	struct pci_dev *pdev = &(e1000e_dev.pci_obj);
	PCI_MSI_ENABLE(pdev, 0);
}

static int e1000e_msi_config_enable(P_E1000E_DEV p_e1000e_dev, uint64_t message_addr, uint32_t message_data)
{
	struct pci_dev *pdev = &(p_e1000e_dev->pci_obj);
	PCI_MSI_ENABLE(pdev, 0);
	PCI_MSI_CONFIG(pdev, message_addr, message_data);
	PCI_MSI_ENABLE(pdev, 1);
	return 0;
}

static void early_init_lapic(void)
{
	union lapic_base_msr base;
	uint64_t sivr = 0UL;

	/* Get local APIC base address */
	base.value = msr_read(MSR_IA32_APIC_BASE);
	sivr = msr_read(MSR_IA32_EXT_APIC_SIVR);
	DBG_INFO("LocalAPIC BASE:%lxH ; SIVR:%lxH", base.value, sivr);

	/* Enable LAPIC in x2APIC mode*/
	/* The following sequence of msr writes to enable x2APIC
	 * will work irrespective of the state of LAPIC
	 * left by BIOS
	 */
	if (SHIFT_LEFT(1, 11) != (SHIFT_LEFT(1, 11) & (base.value))) {
		/* Step1: Enable LAPIC in xAPIC mode */
		base.fields.xAPIC_enable = 1U;
		msr_write(MSR_IA32_APIC_BASE, base.value);
	}
	if (SHIFT_LEFT(1, 10) != (SHIFT_LEFT(1, 10) & (base.value))) {
		/* Step2: Enable LAPIC in x2APIC mode */
		base.fields.x2APIC_enable = 1U;
		msr_write(MSR_IA32_APIC_BASE, base.value);
	}
}

static void init_lapic(void)
{
	/* Mask all LAPIC LVT entries before enabling the local APIC */
	msr_write(MSR_IA32_EXT_APIC_LVT_CMCI, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_TIMER, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_THERMAL, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_PMI, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_LINT0, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_LINT1, LAPIC_LVT_MASK);
	msr_write(MSR_IA32_EXT_APIC_LVT_ERROR, LAPIC_LVT_MASK);

	/* Enable Local APIC */
	/* TODO: add spurious-interrupt handler */
	msr_write(MSR_IA32_EXT_APIC_SIVR, LAPIC_SVR_APIC_ENABLE_MASK | LAPIC_SVR_VECTOR);

	/* Ensure there are no ISR bits set. */
	clear_lapic_isr();
}

static void clear_lapic_isr(void)
{
	uint32_t i = 0U;
	uint32_t isr_reg = 0U;

	/* This is a Intel recommended procedure and assures that the processor
	 * does not get hung up due to already set "in-service" interrupts left
	 * over from the boot loader environment. This actually occurs in real
	 * life, therefore we will ensure all the in-service bits are clear.
	 */
	for (isr_reg = MSR_IA32_EXT_APIC_ISR7; isr_reg >= MSR_IA32_EXT_APIC_ISR0; isr_reg--) {
		for (i = 0U; i < 32U; i++) {
			if (msr_read(isr_reg) != 0U) {
				msr_write(MSR_IA32_EXT_APIC_EOI, 0U);
			} else {
				break;
			}
		}
	}
}

#if DUMP_ISR
static void dump_lapic_isr(void)
{
	uint32_t i = 0U;
	uint32_t isr_reg = 0U;

	/* This is a Intel recommended procedure and assures that the processor
	 * does not get hung up due to already set "in-service" interrupts left
	 * over from the boot loader environment. This actually occurs in real
	 * life, therefore we will ensure all the in-service bits are clear.
	 */
	for (isr_reg = MSR_IA32_EXT_APIC_ISR7; isr_reg >= MSR_IA32_EXT_APIC_ISR0; isr_reg--) {
		for (i = 0U; i < 32U; i++) {
			if (msr_read(isr_reg) != 0U) {
				DBG_INFO("\033[43m[%xH]:%016lxH\033[0m", isr_reg, msr_read(isr_reg));
			} else {
				break;
			}
		}
	}
}
#endif

static int pci_enable_dev(P_E1000E_DEV p_e1000e_dev)
{
	union pci_bdf bdf = p_e1000e_dev->pci_obj.bdf;
	uint32_t reg_val = 0U;

	reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
	reg_val |= (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER | PCI_COMMAND_INTX_DISABLE);
	pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val);
	DBG_INFO("Config command register[%02xH] as [%04xH]", PCI_COMMAND, reg_val);
	delay_loop_ms(10);
	reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_PIN, 1);
	DBG_INFO("INT PIN register:%xH", reg_val);
	if (reg_val) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		DBG_INFO("check command register[%02xH] as [%04xH]", PCI_COMMAND, reg_val);
		if (reg_val & PCI_COMMAND_INTX_DISABLE) {
			pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val & ~PCI_COMMAND_INTX_DISABLE);
			DBG_INFO("Enable INTx function");
		}
	}
	reg_val = pci_pdev_read_cfg(bdf, 0xCC, 2);
	DBG_INFO("PWM status:%xH", reg_val);

	return 0;
}

#if DIS_MASTER
static int e1000e_disable_pcie_master(P_E1000E_DEV p_e1000e_dev)
{
	uint32_t ctrl = 0U;
	uint32_t timeout = MASTER_DISABLE_TIMEOUT;

	ctrl = er32(CTRL);
	ctrl |= E1000_CTRL_GIO_MASTER_DISABLE;
	ew32(CTRL, ctrl);

	while (timeout) {
		if (!(er32(STATUS) & E1000_STATUS_GIO_MASTER_ENABLE)) {
			break;
		}
		timeout--;
	}

	if (!timeout) {
		DBG_WARN("Master requests are pending");
		return (-1);
	}
	DBG_INFO("Master requests are OK");
	return 0;
}
#endif

static int e1000e_msi_handler_connect(P_E1000E_DEV p_e1000e_dev)
{
	struct pci_dev *pdev = &(p_e1000e_dev->pci_obj);
	PCI_MSI_IRQ_START(pdev, msi_irq);
	return 0;
}

#if INT_TEST
static int lapic_send_ipi(P_E1000E_DEV p_e1000e_dev)
{
	int vec = p_e1000e_dev->msi_vector;
	uint32_t i = 0U;

	for (i = 0; i < 3; i++) {
		msr_write(MSR_IA32_EXT_APIC_ICR, APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_FIXED | vec);
		asm volatile ("nop");
		DBG_INFO("Self ipi");
		delay_loop_ms(1000);
		printf("\r\n");
	}
	return 0;
}
#endif

/* Read MSR */
static inline uint64_t cpu_msr_read(uint32_t reg)
{
	uint32_t  msrl = 0U;
	uint32_t msrh = 0U;
	asm volatile (" rdmsr ":"=a"(msrl), "=d"(msrh) : "c" (reg));
	return (((uint64_t)msrh << 32U) | msrl);
}

/* Write MSR */
static inline void cpu_msr_write(uint32_t reg, uint64_t msr_val)
{
	asm volatile (" wrmsr " : : "c"(reg), "a"((uint32_t)msr_val), "d"((uint32_t)(msr_val >> 32U)));
}

static inline uint64_t msr_read(uint32_t reg_num)
{
#if MSR_WR_LEGACY
	uint32_t reg = 0U;
	reg = (reg_num - APIC_BASE_MSR) * 16;
	return apic_read(reg);
#else
	return cpu_msr_read(reg_num);
#endif
}

static inline void msr_write(uint32_t reg_num, uint64_t value64)
{
	#if MSR_WR_LEGACY
	uint32_t reg = 0U;
	reg = (reg_num - APIC_BASE_MSR) * 16;
	apic_write(reg, value64);
	#else
	cpu_msr_write(reg_num, value64);
	#endif
}

static int msi_int_handler_demo(void *p_arg)
{
	printf("[ int ]Run %s(%d)\r\n", __func__, (uint32_t)(uint64_t)p_arg);
	return 0;
}