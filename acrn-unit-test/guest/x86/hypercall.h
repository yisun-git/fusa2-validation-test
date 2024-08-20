#ifndef HYPERCALL_H
#define HYPERCALL_H

#define __packed        __attribute__((packed))
#define __aligned(x)		__attribute__((aligned(x)))

#define PLATFORM_MAX_PCPU_NUM		FW_CFG_NATIVE_CORE_NUMS
#define BSP_CPU_ID				0
#define AP1_CPU_ID				1

/* Hardcode: Need assure this vm id of service vm is aligned with scenario xml definition */
#define VMID_SERVICE_VM   2

#define CR4_RESERVED_BIT15		(1<<15)

/* GPA 4GB is out of EPT scope for test purpose */
#define GPA_4G_OUT_OF_EPT		(1ULL << 32U)

/* Use the unused memory address 384M for test purpose */
#define GPA_TEST (384 * 1024 * 1024UL)

/* Use the unused memory address 388M for guest shared buffer */
#define GPA_VALID_BUFFER			(388 * 1024 * 1024UL)

#define GUEST_FLAG_LAPIC_PASSTHROUGH		(1UL << 1U)  	/* Whether LAPIC is passed through */
#define GUEST_FLAG_IO_COMPLETION_POLLING	(1UL << 2U)  	/* Whether need hypervisor poll IO completion */

#define CONFIG_MAX_VM_NUM	16U
#define VM_MAX_GSI		48

/* Error Code */

/** Indicates that there is IO error. */
#define EIO		5
/** Indicates that no such dev. */
#define ENODEV		19
/** Indicates that argument is not valid. */
#define EINVAL		22
/** Indicates the operation is undefined. */
#define ENOTTY		25

/* vectors range */
#define VECTOR_DYNAMIC_START	0x20U
#define NR_MAX_VECTOR		0xFFU


/*
 * ACRN Hypercall ID definition, copied from ACRN HV code.
 */

#define BASE_HC_ID(x, y) (((x)<<24U)|(y))
#define HC_IDX(id) ((id)&(0xFFUL))

#define HC_ID 0x80UL


/* general */
#define HC_ID_GEN_BASE               0x0UL
#define HC_GET_API_VERSION          BASE_HC_ID(HC_ID, HC_ID_GEN_BASE + 0x00UL)
#define HC_SERVICE_VM_OFFLINE_CPU   BASE_HC_ID(HC_ID, HC_ID_GEN_BASE + 0x01UL)
#define HC_SET_CALLBACK_VECTOR      BASE_HC_ID(HC_ID, HC_ID_GEN_BASE + 0x02UL)

/* VM management */
#define HC_ID_VM_BASE               0x10UL
#define HC_CREATE_VM                BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x00UL)
#define HC_DESTROY_VM               BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x01UL)
#define HC_START_VM                 BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x02UL)
#define HC_PAUSE_VM                 BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x03UL)

#define HC_RESET_VM                 BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x05UL)
#define HC_SET_VCPU_REGS            BASE_HC_ID(HC_ID, HC_ID_VM_BASE + 0x06UL)

/* IRQ and Interrupts */
#define HC_ID_IRQ_BASE              0x20UL
#define HC_INJECT_MSI               BASE_HC_ID(HC_ID, HC_ID_IRQ_BASE + 0x03UL)

#define HC_SET_IRQLINE              BASE_HC_ID(HC_ID, HC_ID_IRQ_BASE + 0x05UL)

/* DM ioreq management */
#define HC_ID_IOREQ_BASE            0x30UL
#define HC_SET_IOREQ_BUFFER         BASE_HC_ID(HC_ID, HC_ID_IOREQ_BASE + 0x00UL)
#define HC_NOTIFY_REQUEST_FINISH    BASE_HC_ID(HC_ID, HC_ID_IOREQ_BASE + 0x01UL)
#define HC_ASYNCIO_ASSIGN           BASE_HC_ID(HC_ID, HC_ID_IOREQ_BASE + 0x02UL)
#define HC_ASYNCIO_DEASSIGN         BASE_HC_ID(HC_ID, HC_ID_IOREQ_BASE + 0x03UL)

/* Guest memory management */
#define HC_ID_MEM_BASE              0x40UL
#define HC_VM_SET_MEMORY_REGIONS    BASE_HC_ID(HC_ID, HC_ID_MEM_BASE + 0x02UL)

#define HC_SETUP_SBUF               BASE_HC_ID(HC_ID, HC_ID_MEM_BASE + 0x04UL)

/* PCI assignment*/
#define HC_ID_PCI_BASE              0x50UL
#define HC_SET_PTDEV_INTR_INFO      BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x03UL)
#define HC_RESET_PTDEV_INTR_INFO    BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x04UL)
#define HC_ASSIGN_PCIDEV            BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x05UL)
#define HC_DEASSIGN_PCIDEV          BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x06UL)

#define HC_ADD_VDEV                 BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x09UL)
#define HC_REMOVE_VDEV              BASE_HC_ID(HC_ID, HC_ID_PCI_BASE + 0x0AUL)

/* Power management */
#define HC_ID_PM_BASE               0x80UL
#define HC_PM_GET_CPU_STATE         BASE_HC_ID(HC_ID, HC_ID_PM_BASE + 0x00UL)

/* Device with bdf 00:14.0 should belong to Service VM */
union pci_bdf const usb_bdf_native = {
	.bits.b = 0x0,
	.bits.d = 0x14,
	.bits.f = 0x0
};

/* Device with bdf 00:1f.3 should belong to Service VM */
union pci_bdf const audio_bdf_native = {
	.bits.b = 0x0,
	.bits.d = 0x1f,
	.bits.f = 0x3
};

/* Device with bdf 00:02.0 should belong to Service VM */
union pci_bdf const vga_bdf_native = {
	.bits.b = 0x0,
	.bits.d = 0x2,
	.bits.f = 0x0
};

/* Device with bdf 02:00.0 should not belong to Service VM */
union pci_bdf const fake_bdf_native = {
	.bits.b = 0x2,
	.bits.d = 0x0,
	.bits.f = 0x0
};

/* Device with bdf 00:00.0 should be a host bridge */
union pci_bdf const host_br_native = {
	.bits.b = 0x0,
	.bits.d = 0x0,
	.bits.f = 0x0
};

/* Device with bdf 00:06.0 should be a bridge */
union pci_bdf const br_native = {
	.bits.b = 0x0,
	.bits.d = 0x6,
	.bits.f = 0x0
};

struct test_range {
	uint64_t min;
	uint64_t max;
};

__unused static struct test_range hc_id_range[] = {
	{.min = HC_GET_API_VERSION, .max = HC_SET_CALLBACK_VECTOR},
	{.min = HC_CREATE_VM, .max = HC_PAUSE_VM},
	{.min = HC_RESET_VM, .max = HC_SET_VCPU_REGS},
	{.min = HC_INJECT_MSI, .max = HC_INJECT_MSI},
	{.min = HC_SET_IRQLINE, .max = HC_SET_IRQLINE},
	{.min = HC_SET_IOREQ_BUFFER, .max = HC_ASYNCIO_DEASSIGN},
	{.min = HC_VM_SET_MEMORY_REGIONS, .max = HC_VM_SET_MEMORY_REGIONS},
	{.min = HC_SETUP_SBUF, .max = HC_SETUP_SBUF},
	{.min = HC_SET_PTDEV_INTR_INFO, .max = HC_DEASSIGN_PCIDEV},
	{.min = HC_ADD_VDEV, .max = HC_REMOVE_VDEV},
	{.min = HC_PM_GET_CPU_STATE, .max = HC_PM_GET_CPU_STATE},

};

struct hypercall_case {
	uint32_t case_id;
	const char *case_name;
};

/**
 * Hypervisor api version info, return it for HC_GET_API_VERSION hypercall
 */
__unused struct hc_api_version {
	/** hypervisor api major version */
	uint32_t major_version;

	/** hypervisor api minor version */
	uint32_t minor_version;
};

/**
 * @brief Info to create a VM, the parameter for HC_CREATE_VM hypercall
 */
struct vm_creation {
	/** created vmid return to HSM. Keep it first field */
	uint16_t vmid;

	/** Reserved */
	uint16_t reserved0;

	/** VCPU numbers this VM want to create */
	uint16_t vcpu_num;

	/** Reserved */
	uint16_t reserved1;

	/** the name of this VM */
	uint8_t	 name[16];

	/* VM flag bits from Guest OS, now used
	 *  GUEST_FLAG_SECURE_WORLD_ENABLED          (1UL<<0)
	 */
	uint64_t vm_flag;

	uint64_t ioreq_buf;

	/**
	 *   The least significant set bit is the PCPU # the VCPU 0 maps to;
	 *   second set least significant bit is the PCPU # the VCPU 1 maps to;
	 *   and so on...
	*/
	uint64_t cpu_affinity;
};

/* HC_SET_VCPU_REGS */

/* General-purpose register layout aligned with the general-purpose register idx
 * when vmexit, such as vmexit due to CR access, refer to SMD Vol.3C 27-6.
 */
struct acrn_gp_regs {
	uint64_t rax;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rsp;
	uint64_t rbp;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
};

/* struct to define how the descriptor stored in memory.
 * Refer SDM Vol3 3.5.1 "Segment Descriptor Tables"
 * Figure 3-11
 */
struct acrn_descriptor_ptr {
	uint16_t limit;
	uint64_t base;
	uint16_t reserved[3];   /* align struct size to 64bit */
} __packed;

/**
 * @brief registers info for vcpu.
 */
struct acrn_regs {
	struct acrn_gp_regs gprs;
	struct acrn_descriptor_ptr gdt;
	struct acrn_descriptor_ptr idt;

	uint64_t        rip;
	uint64_t        cs_base;
	uint64_t        cr0;
	uint64_t        cr4;
	uint64_t        cr3;
	uint64_t        ia32_efer;
	uint64_t        rflags;
	uint64_t        reserved_64[4];

	uint32_t        cs_ar;
	uint32_t        cs_limit;
	uint32_t        reserved_32[3];

	/* don't change the order of following sel */
	uint16_t        cs_sel;
	uint16_t        ss_sel;
	uint16_t        ds_sel;
	uint16_t        es_sel;
	uint16_t        fs_sel;
	uint16_t        gs_sel;
	uint16_t        ldt_sel;
	uint16_t        tr_sel;
};

/**
 * @brief Info to set vcpu state
 *
 * the parameter for HC_SET_VCPU_STATE
 */
struct acrn_vcpu_regs {
	/** the virtual CPU ID for the VCPU to set state */
	uint16_t vcpu_id;

	/** reserved space to make cpu_state aligned to 8 bytes */
	uint16_t reserved[3];

	/** the structure to hold vcpu state */
	struct acrn_regs vcpu_regs;
};

/* HC_SET_IRQLINE */

/** Operation types for setting IRQ line */
#define GSI_SET_HIGH		0U
#define GSI_SET_LOW		1U
#define GSI_RAISING_PULSE	2U
#define GSI_FALLING_PULSE	3U

/**
 * @brief Info to Set/Clear/Pulse a virtual IRQ line for a VM
 *
 * the parameter for HC_SET_IRQLINE hypercall
 */
struct acrn_irqline_ops {
	uint32_t gsi;
	uint32_t op;
};

/* HC_INJECT_MSI */

#define	MSI_ADDR_BASE			0xfeeUL	/* Base address for MSI messages */

/**
 * @brief Info to inject a MSI interrupt to VM
 *
 * the parameter for HC_INJECT_MSI hypercall
 */
struct acrn_msi_entry {
	/** MSI addr[19:12] with dest VCPU ID */
	uint64_t msi_addr;

	/** MSI data[7:0] with vector */
	uint64_t msi_data;
};

/* HC_VM_SET_MEMORY_REGIONS */

#define ACRN_MEM_TYPE_MASK              0x000007C0U
#define ACRN_MEM_TYPE_WB                0x00000040U
#define ACRN_MEM_TYPE_WT                0x00000080U
#define ACRN_MEM_TYPE_UC                0x00000100U
#define ACRN_MEM_TYPE_WC                0x00000200U
#define ACRN_MEM_TYPE_WP                0x00000400U

/**
 * @brief Info to set guest memory region mapping
 *
 * the parameter for HC_VM_SET_MEMORY_REGION hypercall
 */
struct vm_memory_region {
#define MR_ADD		0U
#define MR_DEL		2U
#define MR_MODIFY	3U
	/** set memory region type: MR_ADD or MAP_DEL */
	uint32_t type;

	/** memory attributes: memory type + RWX access right */
	uint32_t prot;

	/** the beginning guest physical address of the memory region*/
	uint64_t gpa;

	/** Service VM's guest physcial address which gpa will be mapped to */
	uint64_t service_vm_gpa;

	/** size of the memory region */
	uint64_t size;
} __aligned(8);

/**
 * set multi memory regions, used for HC_VM_SET_MEMORY_REGIONS
 */
struct set_regions {
	/** vmid for this hypercall */
	uint16_t vmid;

	/** Reserved */
	uint16_t reserved0;

	/** Reserved */
	uint32_t reserved1;

	/**  memory region numbers */
	uint32_t mr_num;

	/** the gpa of regions buffer, point to the regions array:
	 *      struct vm_memory_region regions[mr_num]
	 *      the max buffer size is one page.
	 */
	uint64_t regions_gpa;
} __aligned(8);

/* Type of PCI device assignment */
#define ACRN_PTDEV_QUIRK_ASSIGN	(1U << 0)

#define ACRN_PCI_NUM_BARS	6U
/**
 * @brief Info to assign or deassign PCI for a VM
 *
 */
struct acrn_pcidev {
	/** the type of the the pass-through PCI device */
	uint32_t type;

	/** virtual BDF# of the pass-through PCI device */
	uint16_t virt_bdf;

	/** physical BDF# of the pass-through PCI device */
	uint16_t phys_bdf;

	/** the PCI Interrupt Line, initialized by ACRN-DM, which is RO and
	 *  ideally not used for pass-through MSI/MSI-x devices.
	 */
	uint8_t intr_line;

	/** the PCI Interrupt Pin, initialized by ACRN-DM, which is RO and
	 *  ideally not used for pass-through MSI/MSI-x devices.
	 */
	uint8_t intr_pin;

	/** the base address of the PCI BAR, initialized by ACRN-DM. */
	uint32_t bar[ACRN_PCI_NUM_BARS];
};

/**
 * Intr mapping info per ptdev, the parameter for HC_SET_PTDEV_INTR_INFO
 * hypercall
 */
struct hc_ptdev_irq {
#define IRQ_INTX 0U
#define IRQ_MSI 1U
#define IRQ_MSIX 2U
	/** irq mapping type: INTX or MSI */
	uint32_t type;

	/** virtual BDF of the ptdev */
	uint16_t virt_bdf;

	/** physical BDF of the ptdev */
	uint16_t phys_bdf;

	/** INTX remapping info */
	struct intx_info {
		/** virtual IOAPIC/PIC pin */
		uint32_t virt_pin;

		/** physical IOAPIC pin */
		uint32_t phys_pin;

		/** is virtual pin from PIC */
		bool pic_pin;

		/** Reserved */
		uint8_t reserved[3];
	} intx;

} __aligned(8);

/**
 * @brief Info to create or destroy a virtual PCI or legacy device for a VM
 *
 * the parameter for HC_CREATE_VDEV or HC_DESTROY_VDEV hypercall
 */
struct acrn_vdev {
	/*
	 * the identifier of the device, the low 32 bits represent the vendor
	 * id and device id of PCI device and the high 32 bits represent the
	 * device number of the legacy device
	 */
	union {
		uint64_t value;
		struct {
			uint16_t vendor;
			uint16_t device;
			uint32_t legacy_id;
		} fields;
	} id;

	/*
	 * the slot of the device, if the device is a PCI device, the slot
	 * represents BDF, otherwise it represents legacy device slot number
	 */
	uint64_t slot;

	/** the IO resource address of the device, initialized by ACRN-DM. */
	uint32_t io_addr[ACRN_PCI_NUM_BARS];

	/** the IO resource size of the device, initialized by ACRN-DM. */
	uint32_t io_size[ACRN_PCI_NUM_BARS];

	/** the options for the virtual device, initialized by ACRN-DM. */
	uint8_t	args[128];
};

/**
 * @brief Info PM command from DM/HSM.
 *
 * The command would specify request type(e.g. get px count or data) for
 * specific VM and specific VCPU with specific state number.
 * For Px, PMCMD_STATE_NUM means Px number from 0 to (MAX_PSTATE - 1),
 * For Cx, PMCMD_STATE_NUM means Cx entry index from 1 to MAX_CX_ENTRY.
 */
#define PMCMD_VMID_MASK		0xff000000U
#define PMCMD_VCPUID_MASK	0x00ff0000U
#define PMCMD_STATE_NUM_MASK	0x0000ff00U
#define PMCMD_TYPE_MASK		0x000000ffU

#define PMCMD_VMID_SHIFT	24U
#define PMCMD_VCPUID_SHIFT	16U
#define PMCMD_STATE_NUM_SHIFT	8U

enum acrn_pm_cmd_type {
	ACRN_PMCMD_GET_PX_CNT,
	ACRN_PMCMD_GET_PX_DATA,
	ACRN_PMCMD_GET_CX_CNT,
	ACRN_PMCMD_GET_CX_DATA,
};

#define ACRN_ASYNCIO		64
#define SBUF_MAGIC      0x5aa57aa71aa13aa3UL
#define SBUF_HEAD_SIZE  64U

/* Make sure sizeof(struct shared_buf) == SBUF_HEAD_SIZE */
struct shared_buf {
	uint64_t magic;
	uint32_t ele_num;	/* number of elements */
	uint32_t ele_size;	/* sizeof of elements */
	uint32_t head;		/* offset from base, to read */
	uint32_t tail;		/* offset from base, to write */
	uint32_t flags;
	uint32_t reserved;
	uint32_t overrun_cnt;	/* count of overrun */
	uint32_t size;		/* ele_num * ele_size */
	uint32_t padding[6];
};

/**
 * Setup parameter for share buffer, used for HC_SETUP_SBUF hypercall
 */
struct acrn_sbuf_param {
	/** sbuf cpu id */
	uint16_t cpu_id;

	/** Reserved */
	uint16_t reserved;

	/** sbuf id */
	uint32_t sbuf_id;

	/** sbuf's guest physical address */
	uint64_t gpa;
} __aligned(8);

#define ACRN_ASYNCIO_MAX		64U
#define ACRN_ASYNCIO_PIO	(0x01U)
#define ACRN_ASYNCIO_MMIO	(0x02U)

struct acrn_asyncio_info {
	uint32_t type;
	uint32_t match_data;
	uint64_t addr;
	uint64_t fd;
	uint64_t data;
};


#endif
