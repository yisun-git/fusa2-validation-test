/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2024/07/30
 * Description:         Test case/code for FuSa SRS hypercall
 *
 */

#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"

#include "asm/io.h"
#include "fwcfg.h"
#include "alloc_page.h"

#include "pci_util.h"
#include "pci_check.h"
#include "instruction_common.h"

#include "hypercall.h"


#ifdef USE_DEBUG
#define debug_print(fmt, args...) \
	printf("[%s:%s] line=%d core=%d "fmt"", __FILE__, __func__, __LINE__, get_cpu_id(),  ##args)
#else
#define debug_print(fmt, args...)
#endif

__unused static int64_t acrn_hypercall1_checking(uint64_t hcall_id, uint64_t param1)
{
	register uint64_t r8 asm("r8")  = hcall_id;
	int64_t ret;

	__asm__ __volatile__(ASM_TRY("1f")
		"vmcall;"
		"1:"
		: "=a"(ret)
		: "r"(r8), "D"(param1)
		: "memory"
	);

	return exception_vector();
}

#ifdef __x86_64__

#ifdef IN_NON_SAFETY_VM

/* HC_CREATE_VM/HC_DESTROY_VM */
static struct vm_creation cv = {
	/* Set VM name */
	.name = "POST_STD_VM1",
	/* Set VM flag: LAPIC not pt, no need hv poll IO completion */
	.vm_flag = 0 &
			   (~GUEST_FLAG_LAPIC_PASSTHROUGH) &
			   (~GUEST_FLAG_IO_COMPLETION_POLLING),
	/* Set CPU affinity: pCPU 2~3 to vCPU 0~1. */
	.cpu_affinity = (1UL << 2) | (1UL << 3)
	//Ouput of create_vm: vmid, vcpu_num
};

/* HC_SET_VCPU_REGS */
static struct acrn_vcpu_regs regs = {
	.vcpu_id = 0,
	/**
	 * Current platform:
	 * cr4_reserved_bits_mask:0xfffffffffe88f000, cr0_reserved_bits_mask:0xffffffff1ffaffe0
	 * cr4_rsv_bits_guest_value:0x0, cr0_rsv_bits_guest_value: 0x20
	 */
	.vcpu_regs.cr0 = 0x30U,
	.vcpu_regs.cs_ar = 0x009FU,
	.vcpu_regs.cs_sel = 0xF000U,
	.vcpu_regs.cs_limit = 0xFFFFU,
	.vcpu_regs.cs_base = 0xFFFF0000U,
	.vcpu_regs.rip = 0xFFF0U,
};

/* HC_VM_SET_MEMORY_REGIONS */
static struct vm_memory_region g_mr[] = {
	{.type = MR_ADD, .prot = ACRN_MEM_TYPE_WB, /*.gpa = user_vm_pa, .service_vm_gpa = user_vm_pin_page_pa, .size = region_size */},
	{.type = MR_ADD, .prot = ACRN_MEM_TYPE_WB, /*.gpa = user_vm_pa, .service_vm_gpa = user_vm_pin_page_pa, .size = region_size */},
};

static struct set_regions g_regions = {
	.vmid = 1,
	.mr_num = 2,
};

__unused static void setup_regions3()
{
	u8 *mr0_va = alloc_pages(7);
	u8 *mr0_pa = (u8 *)(uintptr_t)virt_to_phys((void *)mr0_va);;
	u8 *mr1_va = alloc_pages(7);
	u8 *mr1_pa = (u8 *)(uintptr_t)virt_to_phys((void *)mr1_va);;

	g_mr[0].gpa = 0;
	g_mr[0].service_vm_gpa = (u64)mr0_pa;
	g_mr[0].size = PAGE_SIZE * 128;
	g_mr[1].gpa = PAGE_SIZE * 128;
	g_mr[1].service_vm_gpa = (u64)mr1_pa;
	g_mr[1].size = PAGE_SIZE * 128;

	g_regions.regions_gpa = virt_to_phys(g_mr);
}

union pci_bdf const virt_bdf = {
	.bits.b = 0x0,
	.bits.d = 0x1,
	.bits.f = 0x0
};

/* HC_ASSIGN_PCIDEV/HC_DEASSIGN_PCIDEV */
static struct acrn_pcidev g_pcidev;

static void setup_pcidev()
{
	g_pcidev.virt_bdf = virt_bdf.value;
	g_pcidev.phys_bdf = usb_bdf_native.value;

	/* intr_line/intr_pin are not necessary for pt msi/msi-x device */

	/* TODO: it looks like usb device only has two BARs */
	g_pcidev.bar[0] = pci_pdev_read_cfg(usb_bdf_native, PCIR_BAR(0), 4);
	g_pcidev.bar[1] = pci_pdev_read_cfg(usb_bdf_native, PCIR_BAR(1), 4);
}

/* HC_SET_PTDEV_INTR_INFO/HC_RESET_PTDEV_INTR_INFO */
#define PCIR_INTLINE	0x3c
#define LEGACY_IRQ_NUM	16

static struct hc_ptdev_irq ptdev_irq = {
	.type = IRQ_INTX,
	.intx.pic_pin = false,
};

static void setup_ptdev_irq()
{
	ptdev_irq.virt_bdf = virt_bdf.value;
	ptdev_irq.phys_bdf = usb_bdf_native.value;
	/* LEGACY_IRQ_NUM ~ VIOAPIC_RTE_NUM */
	ptdev_irq.intx.virt_pin = LEGACY_IRQ_NUM + 1;
	/* Got through PCIR_INTLINE */
	ptdev_irq.intx.phys_pin = 3; // pci_pdev_read_cfg(usb_bdf_native, PCIR_INTLINE, 1); // out: 0xff
}

/* HC_ADD_VDEV/HC_REMOVE_VDEV */
#define IVSHMEM_VENDOR_ID	0x1af4
#define IVSHMEM_DEVICE_ID	0x1110
/* TODO: this mem size is faked for BAR2 which is mem bar. Should align with definition in scenario xml. */
#define IVSHMEM_MEM_SIZE	0x200000

/* BDF of IVSHMEM virtual device, should align with definition in scenario xml. */
union pci_bdf ivshmem_bdf = {
	.bits.b = 0x0,
	.bits.d = 0x3,
	.bits.f = 0x0
};

static struct acrn_vdev g_vdev = {
	.id.fields.vendor = IVSHMEM_VENDOR_ID,
	.id.fields.device = IVSHMEM_DEVICE_ID,
	/* TODO: need set real share memory region name here. Should align with definition in scenario xml. */
	.args = "hv:/shm_region1"
};

static void setup_vdev()
{
	g_vdev.slot = ivshmem_bdf.value;
	g_vdev.io_addr[0] = pci_pdev_read_cfg(ivshmem_bdf, PCIR_BAR(0), 4);
	g_vdev.io_addr[1] = pci_pdev_read_cfg(ivshmem_bdf, PCIR_BAR(1), 4);
	g_vdev.io_addr[2] = pci_pdev_read_cfg(ivshmem_bdf, PCIR_BAR(2), 4);
	g_vdev.io_addr[3] = pci_pdev_read_cfg(ivshmem_bdf, PCIR_BAR(3), 4);
	g_vdev.io_size[2] = IVSHMEM_MEM_SIZE;
}

#define ACRN_INVALID_CPUID (0xffffU)
static struct acrn_sbuf_param g_sbuf_param = {
	.cpu_id = ACRN_INVALID_CPUID,
	.sbuf_id = ACRN_ASYNCIO,
};

static void setup_sbuf()
{
	u8 *va = alloc_pages(1);
	u8 *pa = (u8 *)(uintptr_t)virt_to_phys((void *)va);;
	struct shared_buf *buf = (struct shared_buf *)va;

	buf->magic = SBUF_MAGIC;
	buf->ele_size = sizeof(uint64_t);
	buf->ele_num = (4096 - SBUF_HEAD_SIZE) / buf->ele_size;
	buf->size = buf->ele_size * buf->ele_num;
        /* set flag to 0 to make sure not overrun! */
	buf->flags = 0;
	buf->overrun_cnt = 0;
	buf->head = 0;
	buf->tail = 0;

	g_sbuf_param.gpa = (u64)pa;
}

static struct acrn_asyncio_info g_async_info = {
	.type = ACRN_ASYNCIO_MMIO,
	.match_data = 0,
	.addr = 0x380000000,
	.fd = 100,
	.data = 10,
};

static struct pci_dev pci_devs[MAX_PCI_DEV_NUM];
static uint32_t nr_pci_devs = MAX_PCI_DEV_NUM;

/*
 * Hypercalls for ACRN
 *
 * - VMCALL instruction is used to implement ACRN hypercalls.
 * - ACRN hypercall ABI:
 *   - Hypercall number is passed in R8 register.
 *   - Up to 2 arguments are passed in RDI, RSI.
 *   - Return value will be placed in RAX.
 */
static int64_t acrn_hypercall0(uint64_t hcall_id)
{
	register uint64_t r8 asm("r8")  = hcall_id;
	int64_t ret;

	__asm__ __volatile__(
		"vmcall;"
		: "=a"(ret)
		: "r"(r8)
		: "memory"
	);

	return ret;
}

static int64_t acrn_hypercall1(uint64_t hcall_id, uint64_t param1)
{
	register uint64_t r8 asm("r8")  = hcall_id;
	int64_t ret;

	__asm__ __volatile__(
		"vmcall;"
		: "=a"(ret)
		: "r"(r8), "D"(param1)
		: "memory"
	);

	return ret;
}

static int64_t acrn_hypercall2(uint64_t hcall_id, uint64_t param1, uint64_t param2)
{
	register uint64_t r8 asm("r8")  = hcall_id;
	int64_t ret;

	__asm__ __volatile__(
		"vmcall;"
		: "=a"(ret)
		: "r"(r8), "D"(param1), "S"(param2)
		: "memory"
	);

	return ret;
}

static void hypercall_at_ring3(const char *msg)
{
	uint64_t version;

	acrn_hypercall1_checking(HC_GET_API_VERSION, (uint64_t)&version);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), msg);
}

/**
 * @brief Case Name: inject_gp_when_hypercall_in_ring3
 *
 * ACRN-10494: When a vCPU of Service VM attempts to execute hypercall, the ACRN hypervisor shall
 * check if the vCPU is in ring0. Otherwise, ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 * Summary: Try to execute a hypercall in ring3, should generate #GP(0).
 */
static void hypercall_acrn_t13581_inject_gp_when_hypercall_in_ring3()
{
	do_at_ring3((void *)hypercall_at_ring3, __FUNCTION__);
}

static uint64_t get_random_invalid_hc_id()
{
	bool finished = false;
	uint64_t val;
	int len = sizeof(hc_id_range) / 16;
	int i;

	while (!finished) {
		i = 0;
		val = get_random_value();
		for (i = 0; i < len; i++) {
			// Break if get valid hc id
			if ((val >= hc_id_range[i].min) && (val <= hc_id_range[i].max)) {
				break;
			}
		}
		// Get invalid hc id
		if (i == len) {
			finished = true;
		}
	}

	return val;
}

/**
 * @brief Case Name: unsupported_hypercall
 *
 * ACRN-10495: When a vCPU of Service VM attempts to execute hypercall with an unsupported ID,
 * ACRN hypervisor shall set error code -ENOTTY to RAX.
 *
 * Summary: Execute hypercall with an unsupported ID, should return -ENOTTY.
 */
static void hypercall_acrn_t7416_unsupported_hypercall()
{
	bool chk = true;

	for (int i = 0; i < sizeof(hc_id_range) / 16; i++) {
		if (acrn_hypercall0(hc_id_range[i].min - 1) != -ENOTTY) {
			chk = false;
			break;
		}
		if (acrn_hypercall0(hc_id_range[i].max + 1) != -ENOTTY) {
			chk = false;
			break;
		}
	}

	if (acrn_hypercall0(get_random_invalid_hc_id()) != -ENOTTY) {
		chk = false;
	}

	report("%s",  (chk == true), __FUNCTION__);
}

/**
 * @brief Case Name: HC_GET_API_VERSION_001
 *
 * ACRN-10497: HC_GET_API_VERSION is used to get ACRN hypervisor version.
 * The first parameter is the GPA to save returned major/minor versions.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Use HC_GET_API_VERSION to get ACRN hypervisor version, on success should return 0.
 */
static void hypercall_acrn_t7417_hc_get_api_version_001()
{
	int64_t result;
	uint64_t version;

	result = acrn_hypercall1(HC_GET_API_VERSION, (uint64_t)&version);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_GET_API_VERSION_002
 *
 * ACRN-10497: HC_GET_API_VERSION is used to get ACRN hypervisor version.
 * The first parameter is the GPA to save returned major/minor versions.
 * If the [GPA, GPA + 8 bytes (size of hypervisor API version structure)] is out of EPT scope,
 * ACRN hypervisor fails to copy the version data into guest memory. Return -EINVAL.
 *
 * Summary: Use HC_GET_API_VERSION to get ACRN hypervisor version, if invalid 8-byte GPA is in RDI,
 * should return -EINVAL.
 */
static void hypercall_acrn_t7418_hc_get_api_version_002()
{
	int64_t result = acrn_hypercall1(HC_GET_API_VERSION, GPA_4G_OUT_OF_EPT);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SERVICE_VM_OFFLINE_CPU_001
 *
 * ACRN-10498: HC_SERVICE_VM_OFFLINE_CPU is used to offline designated vCPU. The vCPU is found by its LAPIC ID.
 * The first parameter is the LAPIC ID of the vCPU to offline.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Use HC_SERVICE_VM_OFFLINE_CPU to offline the last AP, should return 0.
 */
static void hypercall_acrn_t7419_hc_service_vm_offline_cpu_001()
{
	int64_t result;
	u8 apic_id;

	/* Offline the last AP */
	apic_id = get_lapicid_map(fwcfg_get_nb_cpus() - 1);
	result = acrn_hypercall1(HC_SERVICE_VM_OFFLINE_CPU, apic_id);

	report("%s",  (result == 0), __FUNCTION__);
}

/* Get invalid LAPIC ID */
static u8 get_invalid_lapicid()
{
	u8 id;
	int nums = fwcfg_get_nb_cpus();

	if (get_lapicid_map(0) == 0) {
		id = 0;
	} else {
		id = 1;
	}
	for (int i = 0; i < nums; i++) {
		if (get_lapicid_map(i) <= id) {
			id++;
		} else {
			break;
		}
	}
	return id;
}

/**
 * @brief Case Name: HC_SERVICE_VM_OFFLINE_CPU_002
 *
 * ACRN-10498: HC_SERVICE_VM_OFFLINE_CPU is used to offline designated vCPU. The vCPU is found by its LAPIC ID.
 * The first parameter is the LAPIC ID of the vCPU to offline.
 * If the LAPIC ID doesn't belong to Service VM, ACRN hypervisor does nothing and returns 0.
 *
 * Summary: Use HC_SERVICE_VM_OFFLINE_CPU to offline CPU which does not belong to current VM, should return 0.
 */
static void hypercall_acrn_t7420_hc_service_vm_offline_cpu_002()
{
	int64_t result;
	u8 apic_id;

	/* Offline CPU not belong to current VM */
	apic_id  = get_invalid_lapicid();
	debug_print("choose apic_id:%d\n", apic_id);
	result = acrn_hypercall1(HC_SERVICE_VM_OFFLINE_CPU, apic_id);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SERVICE_VM_OFFLINE_CPU_003
 *
 * ACRN-10498: HC_SERVICE_VM_OFFLINE_CPU is used to offline designated vCPU. The vCPU is found by its LAPIC ID.
 * The first parameter is the LAPIC ID of the vCPU to offline.
 * But if the input LAPIC ID related vCPU is BSP, ACRN hypervisor shall return -1.
 *
 * Summary: UUse HC_SERVICE_VM_OFFLINE_CPU to offline BSP, should return -1.
 */
static void hypercall_acrn_t7421_hc_service_vm_offline_cpu_003()
{
	int64_t result;
	u8 apic_id;

	/* Offline BSP */
	apic_id = get_lapicid_map(BSP_CPU_ID);
	result = acrn_hypercall1(HC_SERVICE_VM_OFFLINE_CPU, apic_id);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_CALLBACK_VECTOR
 *
 * ACRN-10499: HC_SET_CALLBACK_VECTOR is used to update the upcall notifier vector according to the input parameter.
 * The first parameter is the vector number to set.
 * On success, ACRN hypervisor shall return 0.
 * But if the input vector is out of range, i.e. less than 0x20U or bigger than 0xFFU, return -EINVAL.
 *
 * Summary: Use HC_SET_CALLBACK_VECTOR to update the upcall notifier vector,
 * if valid vector number [0x20U, 0xFFU] is in RDI, on success should return 0,
 * if invalid vector number out of range of [0x20U, 0xFFU] is in RDI, should return -EINVAL.
 */
static void hypercall_acrn_t13545_hc_set_callback_vector()
{
	u8 chk = 0;
	int64_t result;
	uint64_t random_value;

	/* Update upcall notifier vector with valid vector number */
	result = acrn_hypercall1(HC_SET_CALLBACK_VECTOR, NR_MAX_VECTOR);
	if (result == 0) {
		chk++;
	}

	/* Update upcall notifier vector with invalid vector number out of range of [0x20U, 0xFFU] */
	result = acrn_hypercall1(HC_SET_CALLBACK_VECTOR, VECTOR_DYNAMIC_START - 1);
	if (result == -EINVAL) {
		chk++;
	}
	result = acrn_hypercall1(HC_SET_CALLBACK_VECTOR, NR_MAX_VECTOR + 1);
	if (result == -EINVAL) {
		chk++;
	}

	do {
		random_value = get_random_value();
	} while ((random_value >= VECTOR_DYNAMIC_START) && (random_value <= NR_MAX_VECTOR));
	result = acrn_hypercall1(HC_SET_CALLBACK_VECTOR, random_value);
	if (result == -EINVAL) {
		chk++;
	}

	report("%s",  (chk == 4), __FUNCTION__);
}


/**
 * @brief Case Name: HC_CREATE_VM_001
 *
 * ACRN-10500: HC_CREATE_VM is used to create the target VM.
 * The first parameter is the VM creation information which can be set in bidirection.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Use HC_CREATE_VM to create the target VM, on success should return 0.
 */
static void hypercall_acrn_t7410_hc_create_vm_001()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Prepare the vm_creation struct for target VM */
	memcpy(&create_vm, &cv, sizeof(cv));

	/* Create target VM */
	result = acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;
	debug_print("0x%lx  %d  %d  0x%lx  0x%lx  0x%lx\n", result, create_vm.vmid, create_vm.vcpu_num,
				create_vm.vm_flag, create_vm.ioreq_buf, create_vm.cpu_affinity);

	/* Clean env, destroy target VM */
	if (vmid != 0) {
		acrn_hypercall1(HC_PAUSE_VM, vmid);
		acrn_hypercall1(HC_DESTROY_VM, vmid);
	}

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_CREATE_VM_002
 *
 * ACRN-10500: HC_CREATE_VM is used to create the target VM.
 * The first parameter is the VM creation information which can be set in bidirection.
 * ACRN hypervisor shall return error code in below scenarios.
 * 1. GPA contained in the first parameter is invalid, i.e.
 * [GPA, GPA + 48 bytes (size of create VM information structure)] is out of EPT scope, so that the information copy
 * from guest memory fails which causes the target VM cannot be found. The error code is -ENOTTY.
 *
 * Summary: Use HC_CREATE_VM to create the target VM, if invalid 48-byte GPA is in RDI, should return -ENOTTY.
 */
static void hypercall_acrn_t7411_hc_create_vm_002()
{
	/* Create target VM with GPA start from 4GB which is out of EPT scope */
	int64_t result = acrn_hypercall1(HC_CREATE_VM, GPA_4G_OUT_OF_EPT);

	report("%s",  (result == -ENOTTY), __FUNCTION__);
}

/**
 * @brief Case Name: HC_CREATE_VM_003
 *
 * ACRN-10500: HC_CREATE_VM is used to create the target VM.
 * The first parameter is the VM creation information which can be set in bidirection.
 * ACRN hypervisor shall return error code in below scenarios.
 * 2. The target VM state is not powered off. The error code is -1.
 *
 * Summary: Use HC_CREATE_VM to create target VM when its VM state is not powered off, should return -1.
 */
static void hypercall_acrn_t7412_hc_create_vm_003()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM (After that, target VM state is Created) */
	memcpy(&create_vm, &cv, sizeof(cv));
	result = acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;
	if (result == 0) {
		chk++;
	}

	/* Create target VM again */
	result = acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	if (vmid != 0) {
		acrn_hypercall1(HC_PAUSE_VM, vmid);
		acrn_hypercall1(HC_DESTROY_VM, vmid);
	}

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_CREATE_VM_004
 *
 * ACRN-10500: HC_CREATE_VM is used to create the target VM.
 * The first parameter is the VM creation information which can be set in bidirection.
 * ACRN hypervisor shall return error code in below scenarios.
 * 3. The target VM chooses invalid pCPUs, i.e. the cpu_affinity set in VM creation information doesn't belong to the
 * target VM cpu_affinity. The error code is -1.
 *
 * Summary: Use HC_CREATE_VM to create the target VM, if set its cpu_affinity scope out of which in scenario xml definition,
 * should return -1.
 */
static void hypercall_acrn_t7413_hc_create_vm_004()
{
	int64_t result;
	struct vm_creation create_vm;

	/* Prepare the vm_creation struct for target VM, with illegal cpu_affinity */
	memcpy(&create_vm, &cv, sizeof(cv));
	create_vm.cpu_affinity = (1UL << 4);

	/* Create target VM */
	result = acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_PAUSE_VM_001
 *
 * ACRN-10504: HC_PAUSE_VM is used to pause the target VM.
 * The first parameter is the VM ID of the target VM.
 * On success, ACRN hypervisor shall return 0. But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create target VM, use HC_PAUSE_VM to pause target VM, on success should return 0. Then destroy target VM,
 * so target VM state is powered off, try to pause target VM again with HC_PAUSE_VM, should return -1.
 */
static void hypercall_acrn_t7414_hc_pause_vm_001()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	result = acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM */
	result = acrn_hypercall1(HC_PAUSE_VM, vmid);
	if (result == 0) {
		chk++;
	}

	/* Destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Pause target VM again */
	result = acrn_hypercall1(HC_PAUSE_VM, vmid);
	if (result == -1) {
		chk++;
	}

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DESTROY_VM_001
 *
 * ACRN-10501: HC_DESTROY_VM is used to destroy the target VM.
 * The first parameter is the VM ID of the target VM.
 * On success, ACRN hypervisor shall return 0. But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not paused. The error code is -1.
 *
 * Summary: Create target VM, target VM state is not paused, use HC_DESTROY_VM to destroy target VM, should return -1.
 * Then pause target VM, use HC_DESTROY_VM again, on success should return 0.
 */
static void hypercall_acrn_t7415_hc_destroy_vm_001()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Destroy target VM */
	result = acrn_hypercall1(HC_DESTROY_VM, vmid);
	if (result == -1) {
		chk++;
	}

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Destroy target VM again */
	result = acrn_hypercall1(HC_DESTROY_VM, vmid);
	if (result == 0) {
		chk++;
	}

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_VM_001
 *
 * ACRN-10503: HC_RESET_VM is used to reset the target VM.
 * The first parameter is the VM ID of the target VM.
 * On success, ACRN hypervisor shall return 0. But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not paused. The error code is -1.
 *
 * Summary: Create target VM, target VM state is not paused, use HC_RESET_VM to reset target VM, should return -1.
 * Then pause target VM, use HC_RESET_VM again, on success should return 0.
 */
static void hypercall_acrn_t13583_hc_reset_vm_001()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Reset target VM */
	result = acrn_hypercall1(HC_RESET_VM, vmid);
	if (result == -1) {
		chk++;
	}

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Reset target VM again */
	result = acrn_hypercall1(HC_RESET_VM, vmid);
	if (result == 0) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: fail_to_find_target_vm_by_vmid
 *
 * ACRN-10496: ACRN hypervisor shall find the target VM through VM ID. ACRN hypervisor shall return 0 on success.
 * But ACRN hypervisor shall return -ENOTTY in below scenarios.
 * 1. The target VM cannot be found because the input vm id is wrong so that the converted relative vmid exceeds
 * the configured max VM number.
 * 2. The found VM is a pre-launched VM.
 *
 * Summary: Create target VM, execute HC_PAUSE_VM with wrong vmid exceeding the configured max VM number,
 * should return -ENOTTY; with vmid of a pre-launched VM, should return -ENOTTY; with correct vmid, should return 0.
 */
static void hypercall_acrn_t13582_fail_to_find_target_vm_by_vmid()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM, with wrong vmid exceeding the configured max VM number */
	result = acrn_hypercall1(HC_PAUSE_VM, CONFIG_MAX_VM_NUM);
	if (result == -ENOTTY) {
		chk++;
	}
	do {
		random_value = get_random_value();
	} while (random_value < CONFIG_MAX_VM_NUM);
	result = acrn_hypercall1(HC_PAUSE_VM, random_value);
	if (result == -ENOTTY) {
		chk++;
	}

	/* Pause target VM, with vmid of a pre-launched VM */
	result = acrn_hypercall1(HC_PAUSE_VM, (0 - VMID_SERVICE_VM));
	if (result == -ENOTTY) {
		chk++;
	}

	/* Pause target VM correctly */
	result = acrn_hypercall1(HC_PAUSE_VM, vmid);
	if (result == 0) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 4), __FUNCTION__);
}

/**
 * @brief Case Name: HC_START_VM_001
 *
 * ACRN-10502: HC_START_VM is used to start the target VM. The first parameter is the VM ID of the target VM.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, set IOREQ buffer and set VM memory regions,
 * then use HC_START_VM to start target VM, on success should return 0.
 */
static void hypercall_acrn_t13584_hc_start_vm_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Set VM memory regions */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Start target VM */
	result = acrn_hypercall1(HC_START_VM, vmid);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_START_VM_002
 *
 * ACRN-10502: HC_START_VM is used to start the target VM. The first parameter is the VM ID of the target VM.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -1.
 *
 * Summary: Create target VM, set IOREQ buffer, then pause target VM, so target VM state is not created,
 * then use HC_START_VM to start target VM, should return -1.
 */
static void hypercall_acrn_t13585_hc_start_vm_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Start target VM */
	result = acrn_hypercall1(HC_START_VM, vmid);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_START_VM_003
 *
 * ACRN-10502: HC_START_VM is used to start the target VM. The first parameter is the VM ID of the target VM.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. After creating VM, if there is no HC_SET_IOREQ_BUFFER executed successfully before this hypercall, the target VM
 * IOReq buffer page is not ready. Then, return the error code -1.
 *
 * Summary: Create target VM, without set IOREQ buffer, use HC_START_VM to start the target VM directly,
 * should return -1.
 */
static void hypercall_acrn_t13586_hc_start_vm_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Start target VM */
	result = acrn_hypercall1(HC_START_VM, vmid);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IOREQ_BUFFER_001
 *
 * ACRN-10508: HC_SET_IOREQ_BUFFER is used to set IO request shared buffer.
 * The first parameter is the VM ID of the target VM. The second parameter is the guest shared buffer address.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, use HC_SET_IOREQ_BUFFER to set IO request shared buffer, on success should return 0.
 */
static void hypercall_acrn_t13587_hc_set_ioreq_buffer_001()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	result = acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IOREQ_BUFFER_002
 *
 * ACRN-10508: HC_SET_IOREQ_BUFFER is used to set IO request shared buffer.
 * The first parameter is the VM ID of the target VM. The second parameter is the guest shared buffer address.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -1.
 *
 * Summary: Create target VM, then pause target VM, so target VM state is not created, then use HC_SET_IOREQ_BUFFER
 * to set IO request shared buffer, should return -1.
 */
static void hypercall_acrn_t13588_hc_set_ioreq_buffer_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Set IOREQ buffer */
	result = acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IOREQ_BUFFER_003
 *
 * ACRN-10508: HC_SET_IOREQ_BUFFER is used to set IO request shared buffer.
 * The first parameter is the VM ID of the target VM. The second parameter is the guest shared buffer address.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the guest shared buffer address from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + 8] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_IOREQ_BUFFER to set IO request shared buffer,
 * if invalid 8-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13589_hc_set_ioreq_buffer_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer, with GPA start from 4GB which is out of EPT scope in RDI */
	result = acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IOREQ_BUFFER_004
 *
 * ACRN-10508: HC_SET_IOREQ_BUFFER is used to set IO request shared buffer.
 * The first parameter is the VM ID of the target VM. The second parameter is the guest shared buffer address.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The guest shared buffer host physical address is invalid, i.e. the HPA converted through guest shared buffer
 * GPA is (0x1UL << 52U). That means the the page table entry cannot be found through guest shared buffer address
 * GPA (0x1UL << 52U). The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_IOREQ_BUFFER to set IO request shared buffer,
 * if invalid guest shared buffer is stored in the GPA in RSI, should return -1.
 */
static void hypercall_acrn_t13590_hc_set_ioreq_buffer_004()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer, with invalid guest shared buffer (1ULL << 32U) stored in the GPA in RSI */
	*gpa = GPA_4G_OUT_OF_EPT;
	result = acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_001
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_SET_VCPU_REGS to set the vCPU register of the target VM,
 * on success should return 0.
 */
static void hypercall_acrn_t13600_hc_set_vcpu_regs_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vcpu_regs vcpu_regs;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vcpu_regs struct */
	memcpy(&vcpu_regs, &regs, sizeof(regs));

	/* Set vCPU register of target VM */
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_002
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create, pause and destroy target VM, so target VM state is powered off,
 * then use HC_SET_VCPU_REGS to set the vCPU register of the target VM, should return -1.
 */
static void hypercall_acrn_t13591_hc_set_vcpu_regs_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vcpu_regs vcpu_regs;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Set vCPU register of target VM */
	memcpy(&vcpu_regs, &regs, sizeof(regs));
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_003
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. The GPA saved in the second parameter is 0. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_VCPU_REGS to set the vCPU register of the target VM,
 * if GPA 0 is in RSI, should return -1.
 */
static void hypercall_acrn_t13592_hc_set_vcpu_regs_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set vCPU register of target VM, with 0 in RSI */
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, 0);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_004
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The target VM state is started. The error code is -1.
 *
 * Summary: Create and start target VM, so target VM state is started, then use HC_SET_VCPU_REGS to set
 * the vCPU register of the target VM, should return -1.
 */
static void hypercall_acrn_t13593_hc_set_vcpu_regs_004()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	struct acrn_vcpu_regs vcpu_regs;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Set VM memory regions */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Start target VM (After that, target VM state is Started) */
	result = acrn_hypercall1(HC_START_VM, vmid);
	if (result == 0) {
		chk++;
	}

	/* Set vCPU register of target VM */
	memcpy(&vcpu_regs, &regs, sizeof(regs));
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_005
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. Fail to copy vCPU register information from the GPA saved in the second parameter, * i.e.
 * the [GPA, GPA + 296 bytes (size of vCPU registers structure)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_VCPU_REGS to set the vCPU register of the target VM,
 * if invalid 296-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13594_hc_set_vcpu_regs_005()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set vCPU register of target VM, with GPA start from 4GB which is out of EPT scope in RSI */
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_006
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The vCPU id contained in vCPU register information exceeds the max vCPU number per VM
 * (depends on physical platform). The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_VCPU_REGS to set the vCPU register of the target VM,
 * if set its vcpu_id equal or exceeding the max vCPU number, should return -1.
 */
static void hypercall_acrn_t13595_hc_set_vcpu_regs_006()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vcpu_regs vcpu_regs;
	uint16_t vmid = 0;
	uint64_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set vCPU register of target VM, with illegal vcpu_id which equals the max vCPU number */
	memcpy(&vcpu_regs, &regs, sizeof(regs));
	vcpu_regs.vcpu_id = PLATFORM_MAX_PCPU_NUM;
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);
	if (result == -1) {
		chk++;
	}

	/* Set vCPU register of target VM, with random illegal vcpu_id */
	do {
		random_value = get_random_value();
	} while (random_value < PLATFORM_MAX_PCPU_NUM);
	vcpu_regs.vcpu_id = random_value;
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_VCPU_REGS_007
 *
 * ACRN-10505: HC_SET_VCPU_REGS is used to set the vCPU register of the target VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the vCPU register information to set.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 7. The vCPU CR0 or CR4 register value contained in vCPU register information got from
 * guest memory is not valid, i.e. the reserved bits are set. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_VCPU_REGS to set the vCPU register of the target VM,
 * if set its CR4 register value invalid, should return -1.
 */
static void hypercall_acrn_t13596_hc_set_vcpu_regs_007()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vcpu_regs vcpu_regs;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vcpu_regs struct, with illegal cr4 value */
	memcpy(&vcpu_regs, &regs, sizeof(regs));
	vcpu_regs.vcpu_regs.cr4 |= CR4_RESERVED_BIT15;

	/* Set vCPU register of target VM */
	result = acrn_hypercall2(HC_SET_VCPU_REGS, vmid, (uint64_t)&vcpu_regs);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IRQLINE_001
 *
 * ACRN-10506: HC_SET_IRQLINE is used to set or clear a virtual IRQ line for the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is IRQ line information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_SET_IRQLINE to set or clear a virtual IRQ line for the target VM,
 * on success should return 0.
 */
static void hypercall_acrn_t13597_hc_set_irqline_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_irqline_ops ops;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set virtual IRQ line */
	ops.gsi = 8;
	ops.op = GSI_SET_HIGH;
	result = acrn_hypercall2(HC_SET_IRQLINE, vmid, *((uint64_t *)&ops));

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IRQLINE_002
 *
 * ACRN-10506: HC_SET_IRQLINE is used to set or clear a virtual IRQ line for the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is IRQ line information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create, pause and destroy target VM, so target VM state is powered off, then use HC_SET_IRQLINE
 * to set or clear a virtual IRQ line for the target VM, should return -1.
 */
static void hypercall_acrn_t13598_hc_set_irqline_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_irqline_ops ops;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Set virtual IRQ line */
	ops.gsi = 8;
	ops.op = GSI_SET_HIGH;
	result = acrn_hypercall2(HC_SET_IRQLINE, vmid, *((uint64_t *)&ops));

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_IRQLINE_003
 *
 * ACRN-10506: HC_SET_IRQLINE is used to set or clear a virtual IRQ line for the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is IRQ line information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. The GSI contained in guest IRQ line information is bigger than target VM max GSI (48). The error code is -1.
 *
 * Summary: Create target VM, then HC_SET_IRQLINE to set or clear a virtual IRQ line for the target VM,
 * if set its gsi equal or exceeding max GSI (48), should return -1.
 */
static void hypercall_acrn_t13599_hc_set_irqline_003()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_irqline_ops ops;
	uint16_t vmid = 0;
	uint64_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set virtual IRQ line, with illegal gsi equal or exceeding max GSI (48) */
	ops.gsi = VM_MAX_GSI;
	ops.op = GSI_SET_HIGH;
	result = acrn_hypercall2(HC_SET_IRQLINE, vmid, *((uint64_t *)&ops));
	if (result == -1) {
		chk++;
	}
	do {
		random_value = get_random_value();
	} while (random_value < VM_MAX_GSI);
	ops.gsi = random_value;
	result = acrn_hypercall2(HC_SET_IRQLINE, vmid, *((uint64_t *)&ops));
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_INJECT_MSI_001
 *
 * ACRN-10507: HC_INJECT_MSI is used to inject a MSI interrupt to the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the MSI information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_INJECT_MSI to inject a MSI interrupt to the target VM,
 * on success should return 0.
 */
static void hypercall_acrn_t13601_hc_inject_msi_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_msi_entry msi;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Inject MSI interrupt */
	/* MSI addr[31:20] with base address, MSI addr[19:12] with dest VCPU ID */
	msi.msi_addr = (MSI_ADDR_BASE << 20) | (0UL << 12);
	/* MSI data[7:0] with vector */
	msi.msi_data = (0x10UL << 0);
	result = acrn_hypercall2(HC_INJECT_MSI, vmid, (uint64_t)&msi);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_INJECT_MSI_002
 *
 * ACRN-10507: HC_INJECT_MSI is used to inject a MSI interrupt to the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the MSI information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create, pause and destroy target VM, so target VM state is powered off,
 * then use HC_INJECT_MSI to inject a MSI interrupt to the target VM, should return -1.
 */
static void hypercall_acrn_t13602_hc_inject_msi_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_msi_entry msi;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Inject MSI interrupt */
	/* MSI addr[31:20] with base address, MSI addr[19:12] with dest VCPU ID */
	msi.msi_addr = (MSI_ADDR_BASE << 20) | (0UL << 12);
	/* MSI data[7:0] with vector */
	msi.msi_data = (0x10UL << 0);
	result = acrn_hypercall2(HC_INJECT_MSI, vmid, (uint64_t)&msi);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_INJECT_MSI_003
 *
 * ACRN-10507: HC_INJECT_MSI is used to inject a MSI interrupt to the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the MSI information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy MSI information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + 16 bytes (size of MSI information structure)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_INJECT_MSI to inject a MSI interrupt to the target VM,
 * if invalid 16-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13603_hc_inject_msi_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	memcpy(&create_vm, &cv, sizeof(cv));

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Inject MSI interrupt, with GPA start from 4GB which is out of EPT scope in RSI */
	result = acrn_hypercall2(HC_INJECT_MSI, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_INJECT_MSI_004
 *
 * ACRN-10507: HC_INJECT_MSI is used to inject a MSI interrupt to the target VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the MSI information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The MSI address is invalid for case that LAPIC is virtualized, i.e the address base is not 0xfeeUL. The error code is -1.
 *
 * Summary: Create target VM, then use HC_INJECT_MSI to inject a MSI interrupt to the target VM,
 * if set its msi_addr invalid, should return -1.
 */
static void hypercall_acrn_t13604_hc_inject_msi_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_msi_entry msi;
	uint16_t vmid = 0;
	uint16_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Inject MSI interrupt, with invalid msi_addr */
	/* MSI addr[31:20] with base address, MSI addr[19:12] with dest VCPU ID */
	do {
		random_value = get_random_value() % 0x1000;
	} while (random_value == MSI_ADDR_BASE);
	msi.msi_addr = (random_value << 20) | (0UL << 12);
	/* MSI data[7:0] with vector */
	msi.msi_data = (0x10UL << 0);
	result = acrn_hypercall2(HC_INJECT_MSI, vmid, (uint64_t)&msi);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_NOTIFY_REQUEST_FINISH_001
 *
 * ACRN-10509: HC_NOTIFY_REQUEST_FINISH is used to notify the requestor vCPU for the completion of an IO request.
 * The first parameter is the VM ID of the target VM. The second parameter is the vCPU ID to notify.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, set IOREQ buffer, then use HC_NOTIFY_REQUEST_FINISH to notify IO request finish,
 * on success should return 0.
 */
static void hypercall_acrn_t13605_hc_notify_request_finish_001()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;
	uint64_t vcpu_id = 1;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Notify IO request finish */
	result = acrn_hypercall2(HC_NOTIFY_REQUEST_FINISH, vmid, vcpu_id);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_NOTIFY_REQUEST_FINISH_002
 *
 * ACRN-10509: HC_NOTIFY_REQUEST_FINISH is used to notify the requestor vCPU for the completion of an IO request.
 * The first parameter is the VM ID of the target VM. The second parameter is the vCPU ID to notify.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create target VM, set IOREQ buffer, then pause and destroy target VM, so target VM state is powered off,
 * then use HC_NOTIFY_REQUEST_FINISH to notify IO request finish, should return -1.
 */
static void hypercall_acrn_t13606_hc_notify_request_finish_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;
	uint64_t vcpu_id = 1;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Notify IO request finish */
	result = acrn_hypercall2(HC_NOTIFY_REQUEST_FINISH, vmid, vcpu_id);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_NOTIFY_REQUEST_FINISH_003
 *
 * ACRN-10509: HC_NOTIFY_REQUEST_FINISH is used to notify the requestor vCPU for the completion of an IO request.
 * The first parameter is the VM ID of the target VM. The second parameter is the vCPU ID to notify.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. If HC_SET_IOREQ_BUFFER hypercall is not executed before executing this hypercall,
 * the ioreq shared buffer is NULL. The error code is -1.
 *
 * Summary: Create target VM, without set IOREQ buffer, use HC_NOTIFY_REQUEST_FINISH
 * to notify IO request finish directly, should return -1.
 */
static void hypercall_acrn_t13607_hc_notify_request_finish_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t vcpu_id = 1;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Notify IO request finish */
	result = acrn_hypercall2(HC_NOTIFY_REQUEST_FINISH, vmid, vcpu_id);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_NOTIFY_REQUEST_FINISH_004
 *
 * ACRN-10509: HC_NOTIFY_REQUEST_FINISH is used to notify the requestor vCPU for the completion of an IO request.
 * The first parameter is the VM ID of the target VM. The second parameter is the vCPU ID to notify.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The vCPU ID saved in the second parameter exceeds the created vCPU number. The error code is -1.
 *
 * Summary: Create target VM, set IOREQ buffer, then use HC_NOTIFY_REQUEST_FINISH to notify IO request finish,
 * if set its vcpu_id equal or exceeding the created vCPU number in RSI, should return -1.
 */
static void hypercall_acrn_t13608_hc_notify_request_finish_004()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;
	uint64_t *gpa = (uint64_t *)GPA_TEST;
	*gpa = GPA_VALID_BUFFER;
	uint64_t random_value;

	memcpy(&create_vm, &cv, sizeof(cv));

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set IOREQ buffer */
	acrn_hypercall2(HC_SET_IOREQ_BUFFER, vmid, GPA_TEST);

	/* Notify IO request finish, with illegal vcpu_id equal or exceeding the created vCPU number */
	result = acrn_hypercall2(HC_NOTIFY_REQUEST_FINISH, vmid, create_vm.vcpu_num);
	if (result == -1) {
		chk++;
	}
	do {
		random_value = get_random_value();
	} while (random_value < create_vm.vcpu_num);
	result = acrn_hypercall2(HC_NOTIFY_REQUEST_FINISH, vmid, random_value);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_001
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions,
 * on success should return 0.
 */
static void hypercall_acrn_t13609_hc_vm_set_memory_regions_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the set_regions struct */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid; // relative vm id

	/* Setup EPT memory mapping */
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_002
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * ACRN hypervisor shall return error code in below scenarios.
 * 1. Fail to copy guest memory regions information out from guest memory which GPA is saved in the first parameter,
 *  i.e. the [GPA, GPA + 24 bytes (size of memory regions structure)] is out of EPT scope. The error code is -ENOTTY.
 *
 * Summary: Create target VM, then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions,
 * if invalid 24-byte GPA is in RDI, should return -ENOTTY.
 */
static void hypercall_acrn_t13610_hc_vm_set_memory_regions_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup EPT memory mapping, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENOTTY), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_003
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * ACRN hypervisor shall return error code in below scenarios.
 * 2. The target VM state is powered off. The error code is -1.
 *
 * Summary: Create, pause and destroy target VM, so target VM state is powered off,
 * then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions, should return -1.
 */
static void hypercall_acrn_t13611_hc_vm_set_memory_regions_003()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Setup EPT memory mapping */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_004
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * ACRN hypervisor shall return error code in below scenarios.
 * 3. Fail to copy memory region data out from guest memory which GPA is saved in the guest memory regions information,
 *  i.e. the [GPA, GPA + 32 bytes (size of one memory region structure)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions,
 * if set its regions_gpa invalid GPA, should return -1.
 */
static void hypercall_acrn_t13612_hc_vm_set_memory_regions_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup EPT memory mapping, with illegal regions_gpa start from 4GB which is out of EPT scope */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	regions.regions_gpa = GPA_4G_OUT_OF_EPT;
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_005
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. Guest memory region size contained in guest memory region information is not a multiple of 4KB. The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions,
 * if set its regions_gpa.size illegal value, should return -EINVAL.
 */
static void hypercall_acrn_t13613_hc_vm_set_memory_regions_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	struct vm_memory_region *mr0;
	uint16_t vmid = 0;
	uint64_t save_size;
	u8 random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup EPT memory mapping, with illegal regions_gpa.size not a multiple of 4KB */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	mr0 = (struct vm_memory_region *)regions.regions_gpa;
	save_size = mr0->size; // save size
	random_value = get_random_value();
	mr0->size = (random_value % 128) * PAGE_SIZE + (random_value % (PAGE_SIZE - 1)) + 1;
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	mr0->size = save_size; // restore size

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_VM_SET_MEMORY_REGIONS_006
 *
 * ACRN-10510: HC_VM_SET_MEMORY_REGIONS is used to setup EPT memory mapping for input multiple regions.
 * The first parameter is the memory regions information.
 * ACRN hypervisor shall return error code in below scenarios.
 * 5. Service VM GPA contained in guest memory region information is invalid, i.e. the [GPA, GPA + one memory region size]
 * is out of EPT scope. The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_VM_SET_MEMORY_REGIONS to setup EPT memory mapping for input multiple regions,
 * if set its regions_gpa.service_vm_gpa illegal value, should return -EINVAL.
 */
static void hypercall_acrn_t13614_hc_vm_set_memory_regions_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct set_regions regions;
	struct vm_memory_region *mr;
	uint16_t vmid = 0;
	uint64_t save_gpa;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup EPT memory mapping, with illegal regions_gpa.service_vm_gpa start from 4GB which is out of EPT scope */
	memcpy(&regions, &g_regions, sizeof(g_regions));
	regions.vmid = vmid;
	mr = (struct vm_memory_region *)regions.regions_gpa;
	save_gpa = mr->service_vm_gpa; // save service_vm_gpa
	mr->service_vm_gpa = GPA_4G_OUT_OF_EPT;
	result = acrn_hypercall1(HC_VM_SET_MEMORY_REGIONS, (uint64_t)&regions);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	mr->service_vm_gpa = save_gpa; // restore service_vm_gpa

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_001
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information. On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * on success should return 0.
 */
static void hypercall_acrn_t13615_hc_assign_pcidev_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_002
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -EINVAL.
 *
 * Summary: Create target VM, then pause target VM, so target VM state is not created,
 * then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM, should return -EINVAL.
 */
static void hypercall_acrn_t13616_hc_assign_pcidev_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_003
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the assigned virtual PCI device information from guest memory which GPA is saved in the second
 * parameter, i.e. the [GPA, GPA + 34 bytes (size of PCI device structure)] is out of EPT scope.
 * The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * if invalid 34-byte GPA is in RSI, should return -EINVAL.
 */
static void hypercall_acrn_t13617_hc_assign_pcidev_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_004
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The assigned virtual PCI device is not under the management of Service VM, i.e. the physical BDF contained in the
 * virtual PCI device information doesn't belong to Service VM. The error code is -ENODEV.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * if the assigned virtual PCI device doesn't belong to Service VM, should return -ENODEV.
 */
static void hypercall_acrn_t13618_hc_assign_pcidev_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct, with invalid phys_bdf which not belong to Service VM */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.phys_bdf = fake_bdf_native.value;

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_005
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. The assigned virtual PCI device current user is not itself,
 * i.e. the PCI device has been assigned to other VM. The error code is -ENODEV.
 *
 * Summary: Create target VM and another VM, then assign one virtual PCI dev to another VM firstly,
 * then use HC_ASSIGN_PCIDEV to assign the same virtual PCI dev to target VM, should return -ENODEV.
 */
static void hypercall_acrn_t13619_hc_assign_pcidev_005()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid1 = 0;
	uint16_t vmid2 = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid1 = create_vm.vmid;

	/* Create another VM */
	memcpy(create_vm.name, "POST_STD_VM2", sizeof(create_vm.name));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid2 = create_vm.vmid;
	debug_print("vmid1:%d, vmid2:%d\n", vmid1, vmid2);

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev to another VM firstly */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid2, (uint64_t)&pcidev);
	if (result == 0) {
		chk++;
	}

	/* Assign the same virtual PCI dev to target VM */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid1, (uint64_t)&pcidev);
	if (result == -ENODEV) {
		chk++;
	}
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM and another VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid1);
	acrn_hypercall1(HC_DESTROY_VM, vmid1);
	acrn_hypercall1(HC_PAUSE_VM, vmid2);
	acrn_hypercall1(HC_DESTROY_VM, vmid2);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_006
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The assigned virtual PCI device’s physical device doesn’t exist,
 * i.e. the virtual PCI device in Service VM is an emulated PCI device,
 * not a passthrough PCI device, which doesn't have a physical PCI device. The error code is -ENODEV.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * if the virtual PCI device in Service VM is an emulated PCI device, should return -ENODEV.
 */
static void hypercall_acrn_t13620_hc_assign_pcidev_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct, with phys_bdf of ivshmem emulated device */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.phys_bdf = ivshmem_bdf.value;

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_007
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 6. The assigned virtual PCI device’s physical device is a host bridge. The error code is -ENODEV.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * if the virtual PCI device’s physical device is a host bridge, should return -ENODEV.
 */
static void hypercall_acrn_t13621_hc_assign_pcidev_007()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct, with invalid phys_bdf which is a host bridge */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.phys_bdf = host_br_native.value;

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_008
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 7. The assigned virtual PCI device’s physical device is a bridge. The error code is -ENODEV.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev to target VM,
 * if the virtual PCI device’s physical device is a bridge, should return -ENODEV.
 */
static void hypercall_acrn_t13622_hc_assign_pcidev_008()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct, with invalid phys_bdf which is a bridge */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.phys_bdf = br_native.value;

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASSIGN_PCIDEV_009
 *
 * ACRN-10513: HC_ASSIGN_PCIDEV is used to assign one virtual PCI dev to a VM.
 * The first parameter is the VM ID of the target VM. The second parameter is the assigned virtual PCI device
 * information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 8. One of the assigned virtual PCI device’s BARs is a IO BAR but its base address GPA doesn’t equal HPA,
 * i.e. the BAR base address contained in the virtual PCI device information is not the corresponding HPA.
 * The error code is -EIO.
 *
 * Summary: Create target VM, then use HC_ASSIGN_PCIDEV to assign one virtual PCI dev which has IO BAR to target VM,
 * set the IO BAR's base address GPA not equal HPA, should return -EIO.
 */
static void hypercall_acrn_t13623_hc_assign_pcidev_009()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct, with the IO BAR's base address GPA not equal HPA */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.phys_bdf = vga_bdf_native.value;
	pcidev.bar[0] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(0), 4); // bar 0 low
	pcidev.bar[1] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(1), 4); // bar 0 high
	pcidev.bar[2] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(2), 4); // bar 1 low
	pcidev.bar[3] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(3), 4); // bar 1 high
	pcidev.bar[4] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(4), 4); // bar 2 low
	pcidev.bar[5] = pci_pdev_read_cfg(vga_bdf_native, PCIR_BAR(5), 4); // bar 2 high
	pcidev.bar[4] = GPA_TEST;
	debug_print("bars: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", pcidev.bar[0], pcidev.bar[1], pcidev.bar[2], pcidev.bar[3], pcidev.bar[4], pcidev.bar[5]);

	/* Assign virtual PCI dev */
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EIO), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_001
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_DEASSIGN_PCIDEV to deassign it
 * from target VM, on success should return 0.
 */
static void hypercall_acrn_t13624_hc_deassign_pcidev_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev */
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Deassign virtual PCI dev */
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_002
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created and is not paused either. The error code is -EINVAL.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then pause and destroy target VM, so target VM
 * state is not created or paused, then use HC_DEASSIGN_PCIDEV to deassign it from target VM, should return -EINVAL.
 */
static void hypercall_acrn_t13625_hc_deassign_pcidev_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev */
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Deassign virtual PCI dev */
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_003
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the assigned virtual PCI device information from guest memory which GPA is saved in the second
 * parameter, i.e. the [GPA, GPA + 34 bytes (size of PCI device structure)] is out of EPT scope.
 * The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_DEASSIGN_PCIDEV to deassign one virtual PCI dev from target VM,
 * if invalid 34-byte GPA is in RSI, should return -EINVAL.
 */
static void hypercall_acrn_t13626_hc_deassign_pcidev_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Deassign virtual PCI dev, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_004
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. Fail to find the virtual PCI device in target VM device list by comparing the virtual BDF, i.e. the input virtual
 * PCI device has never been assigned to target VM before. The error code is -ENODEV.
 *
 * Summary: Create target VM, then use HC_DEASSIGN_PCIDEV to deassign one virtual PCI dev from target VM
 * without assign it to target VM before, should return -ENODEV.
 */
static void hypercall_acrn_t13627_hc_deassign_pcidev_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Deassign virtual PCI dev */
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_005
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. The found virtual PCI device’s physical device doesn’t exist, i.e. the virtual PCI device is an emulated device
 * but not a passthrough device. The error code is -ENODEV.
 *
 * Summary: Create target VM, add an emulated device in hypervisor to target VM, then use HC_DEASSIGN_PCIDEV to
 * deassign it from target VM, so the found emulated device's physical device doesn't exist, should return -ENODEV.
 */
static void hypercall_acrn_t13628_hc_deassign_pcidev_005()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Add emulated device */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);
	if (result == 0) {
		chk++;
	}

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	pcidev.virt_bdf = ivshmem_bdf.value;
	pcidev.phys_bdf = ivshmem_bdf.value;

	/* Deassign virtual PCI dev */
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	if (result == -ENODEV) {
		chk++;
	}
	printf("result:0x%lx\n", result);

	/* Clean env, remove emulated device */
	acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_DEASSIGN_PCIDEV_006
 *
 * ACRN-10514: HC_DEASSIGN_PCIDEV is used to deassign one virtual PCI dev from a VM.
 * The first parameter is the VM ID of the target VM.
 * The second parameter is the assigned virtual PCI device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The found virtual PCI device’s physical device #BDF doesn’t equal to the physical #BDF which is contained in the
 * assigned virtual PCI device information. The error code is -ENODEV.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_DEASSIGN_PCIDEV to deassign it
 * from target VM, if set its phys_bdf incorrect value, should return -ENODEV.
 */
static void hypercall_acrn_t13629_hc_deassign_pcidev_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_pcidev struct */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));

	/* Assign virtual PCI dev */
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Deassign virtual PCI dev, with phys_bdf incorrect value */
	pcidev.phys_bdf += 1;
	result = acrn_hypercall2(HC_DEASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -ENODEV), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_001
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, on success should return 0.
 */
static void hypercall_acrn_t13630_hc_set_ptdev_intr_info_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_002
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then pause target VM, so target VM state is
 * not created, then use HC_SET_PTDEV_INTR_INFO to set interrupt mapping information of passthrough device,
 * should return -1.
 */
static void hypercall_acrn_t13631_hc_set_ptdev_intr_info_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_003
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the interrupt mapping information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + 24 bytes (size of passthrough device interrupt information structure)] is out of EPT scope.
 * The error code is -1.
 *
 * Summary: Create target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt mapping information of passthrough
 * device, if invalid 24-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13632_hc_set_ptdev_intr_info_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Set interrupt mapping information, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_004
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The interrupt type contained in information is not INTx. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, if set its type not INTx, should return -1.
 */
static void hypercall_acrn_t13633_hc_set_ptdev_intr_info_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct, with type not INTx */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));
	irq.type = IRQ_MSI;

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_005
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. Fail to find the virtual device through the virtual #BDF contained in information. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, if set its virt_bdf incorrect value, so fail to find the virtual device,
 * should return -1.
 */
static void hypercall_acrn_t13634_hc_set_ptdev_intr_info_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct, with incorrect virt_bdf */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));
	irq.virt_bdf += 1;

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_006
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The found virtual PCI device’s physical device #BDF doesn’t equal to the physical #BDF contained
 * in interrupt mapping information. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, if set its phys_bdf different with the found virtual device,
 * should return -1.
 */
static void hypercall_acrn_t13635_hc_set_ptdev_intr_info_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	result = acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct, with incorrect phys_bdf */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));
	irq.phys_bdf += 1;

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_007
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 6. The virtual pin contained in interrupt mapping information is invalid, i.e. it exceeds the max GSI number (48)
 * of target VM if the PIC pin number contained in the information equals to 0. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, if the virtual pin is invalid, should return -1.
 */
static void hypercall_acrn_t13636_hc_set_ptdev_intr_info_007()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;
	uint32_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Set interrupt mapping information, with intx.pic_pin equal 0, intx.virt_pin equal or exceeding the max GSI number (48) */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));
	irq.intx.pic_pin = false;
	irq.intx.virt_pin = 48;
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == -1) {
		chk++;
	}
	do {
		random_value = get_random_value();
	} while (random_value < 48);
	irq.intx.virt_pin = random_value;
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_008
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 7. The physical pin contained in interrupt mapping information is invalid, i.e.  it exceeds the max GSI number (depends on platform)
 * of target VM supported by ACRN hypervisor. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, then use HC_SET_PTDEV_INTR_INFO to set interrupt
 * mapping information of passthrough device, if the physical pin is invalid, should return -1.
 */
static void hypercall_acrn_t13637_hc_set_ptdev_intr_info_008()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;
	uint32_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Set interrupt mapping information, with intx.phys_pin equal or exceeding the max GSI number (120) */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));
	irq.intx.phys_pin = 120; // TBD: 120 is get from HV build. Need update later when able to get the value from vIO-APIC SRS.
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == -1) {
		chk++;
	}
	do {
		random_value = get_random_value();
	} while (random_value < 120);
	irq.intx.phys_pin = random_value;
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == -1) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SET_PTDEV_INTR_INFO_009
 *
 * ACRN-10511: HC_SET_PTDEV_INTR_INFO is used to set interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 8. Fail to add the INTx entry for target VM. One case is that the physical pin contained in interrupt mapping information
 * belongs to another VM but not the target VM, i.e. the physical pin for this passthrough device irq has been assigned to
 * another VM already. The error code is -ENODEV.
 *
 * Summary: Create target VM and another VM, assign two different virtual PCI devs to two VMs separately,
 * then use HC_SET_PTDEV_INTR_INFO to set interrupt mapping information of both devices, if set the same physical pin
 * for both devices, so fail to add the INTx entry for target VM, should return -ENODEV.
 */
static void hypercall_acrn_t13638_hc_set_ptdev_intr_info_009()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq1, irq2;
	uint16_t vmid1 = 0;
	uint16_t vmid2 = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid1 = create_vm.vmid;

	/* Create another VM */
	memcpy(create_vm.name, "POST_STD_VM2", sizeof(create_vm.name));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid2 = create_vm.vmid;

	debug_print("vmid1:%d, vmid2:%d\n", vmid1, vmid2);

	/* Assign one virtual PCI dev to target VM */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid1, (uint64_t)&pcidev);

	/* Assign another virtual PCI dev to another VM */
	pcidev.phys_bdf = audio_bdf_native.value;
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid2, (uint64_t)&pcidev);

	/* Set interrupt mapping information for virtual device on another VM, with intx.phys_pin equal 3 */
	memcpy(&irq2, &ptdev_irq, sizeof(ptdev_irq));
	irq2.phys_bdf = audio_bdf_native.value;
	irq2.intx.virt_pin += 1;
	irq2.intx.phys_pin = 3;
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid2, (uint64_t)&irq2);
	if (result == 0) {
		chk++;
	}

	/* Set interrupt mapping information for virtual device on target VM, with the same intx.phys_pin */
	memcpy(&irq1, &ptdev_irq, sizeof(ptdev_irq));
	irq1.intx.phys_pin = 3;
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid1, (uint64_t)&irq1);
	if (result == -ENODEV) {
		chk++;
	}
	printf("result:0x%lx\n", result);

	/* Clean env, destroy target VM and another VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid1);
	acrn_hypercall1(HC_DESTROY_VM, vmid1);
	acrn_hypercall1(HC_PAUSE_VM, vmid2);
	acrn_hypercall1(HC_DESTROY_VM, vmid2);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_001
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information of passthrough
 * device, then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information, on success should return 0.
 */
static void hypercall_acrn_t13639_hc_reset_ptdev_intr_info_001()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	result = acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == 0) {
		chk++;
	}

	/* Clear interrupt mapping information */
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);
	if (result == 0) {
		chk++;
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_002
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created or paused. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information of passthrough
 * device, then pause and destroy target VM, so target VM state is not created or paused,
 * then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information, should return -1.
 */
static void hypercall_acrn_t13640_hc_reset_ptdev_intr_info_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Clear interrupt mapping information */
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_003
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the interrupt mapping information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + 24 bytes (size of passthrough device interrupt information structure)] is out of EPT scope.
 * The error code is -1.
 *
 * Summary: Create target VM, then use HC_RESET_PTDEV_INTR_INFO to clear interrupt mapping information of passthrough
 * device, if invalid 24-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13641_hc_reset_ptdev_intr_info_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Clear interrupt mapping information of passthrough device, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_004
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The interrupt type contained in information is not INTx. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information
 * of passthrough device, then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information,
 * if set its type not INTx, should return -1.
 */
static void hypercall_acrn_t13642_hc_reset_ptdev_intr_info_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clear interrupt mapping information of passthrough device, with type not INTx */
	irq.type = IRQ_MSI;
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_005
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. Fail to find the virtual device through the #BDF contained in information. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information
 * of passthrough device, then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information,
 * if set its virt_bdf incorrect value, so fail to find the virtual device, should return -1.
 */
static void hypercall_acrn_t13643_hc_reset_ptdev_intr_info_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clear interrupt mapping information of passthrough device, with incorrect virt_bdf */
	irq.virt_bdf += 1;
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_006
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The found virtual PCI device’s physical device #BDF doesn’t equal to the #BDF contained in interrupt
 * mapping information. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information
 * of passthrough device, then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information,
 * if set its phys_bdf different with the found virtual device, should return -1.
 */
static void hypercall_acrn_t13644_hc_reset_ptdev_intr_info_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clear interrupt mapping information, with incorrect phys_bdf */
	irq.phys_bdf += 1;
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_RESET_PTDEV_INTR_INFO_007
 *
 * ACRN-10512: HC_RESET_PTDEV_INTR_INFO is used to clear interrupt mapping information of passthrough device.
 * The first parameter is the VM ID of the target VM. The second parameter is the interrupt mapping information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 6. The virtual pin contained in interrupt mapping information is invalid, i.e. it exceeds the max GSI number (48)
 * of target VM if the PIC pin number contained in the information equals to 0. The error code is -1.
 *
 * Summary: Create target VM, assign one virtual PCI dev to target VM, set interrupt mapping information
 * of passthrough device, then use HC_RESET_PTDEV_INTR_INFO to clear its interrupt mapping information,
 * if the virtual pin is invalid, should return -1.
 */
static void hypercall_acrn_t13645_hc_reset_ptdev_intr_info_007()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_pcidev pcidev;
	struct hc_ptdev_irq irq;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Assign virtual PCI dev */
	memcpy(&pcidev, &g_pcidev, sizeof(g_pcidev));
	acrn_hypercall2(HC_ASSIGN_PCIDEV, vmid, (uint64_t)&pcidev);

	/* Prepare the hc_ptdev_irq struct */
	memcpy(&irq, &ptdev_irq, sizeof(ptdev_irq));

	/* Set interrupt mapping information */
	acrn_hypercall2(HC_SET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clear interrupt mapping information, with intx.pic_pin equal 0, intx.virt_pin equal the max GSI number (48) */
	irq.intx.pic_pin = false;
	irq.intx.virt_pin = 48;
	result = acrn_hypercall2(HC_RESET_PTDEV_INTR_INFO, vmid, (uint64_t)&irq);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ADD_VDEV_001
 *
 * ACRN-10515: HC_ADD_VDEV is used to add an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * On success, ACRN hypervisor shall return 0. But ACRN hypervisor shall return error code in below scenarios.
 * 4. There is no create callback function in the emulated device operations.
 * One case is that the CONFIG_IVSHMEM_ENABLED is not enabled in ACRN hypervisor but try to add a ivshmem device.
 * The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_ADD_VDEV to add an emulated device, on success should return 0.
 * But if IVSHMEM is not configured in scenario xml in advanced, should return -EINVAL.
 */
static void hypercall_acrn_t13646_hc_add_vdev_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Add emulated device */
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);
	printf("result:0x%lx\n", result);

	/* Clean env, remove emulated device */
	acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/**
	 * Manual test:
	 * If test with scenario xml which has IVSHMEM definition, on success, return 0.
	 * If test with separate scenario xml which has no IVSHMEM definition, return -EINVAL.
	*/
	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ADD_VDEV_002
 *
 * ACRN-10515: HC_ADD_VDEV is used to add an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -EINVAL.
 *
 * Summary: Create and pause target VM, so target VM state is not created,
 * then use HC_ADD_VDEV to add an emulated device, should return -EINVAL.
 */
static void hypercall_acrn_t13647_hc_add_vdev_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Add emulated device */
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ADD_VDEV_003
 *
 * ACRN-10515: HC_ADD_VDEV is used to add an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the device information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + 192 bytes (size of virtual device structure)] is out of EPT scope. The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_ADD_VDEV to add an emulated device in hypervisor,
 * if invalid 192-byte GPA is in RSI, should return -EINVAL.
 */
static void hypercall_acrn_t13648_hc_add_vdev_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Add emulated device, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ADD_VDEV_004
 *
 * ACRN-10515: HC_ADD_VDEV is used to add an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. Fail to find the emulated device operations through device information, i.e. the device ID contained in the
 * information is not correct. The error code is -EINVAL. The supported emulated device IDs are below.
 *     3.1 IVSHMEM device ID: 0x11101af4.
 *     3.2 MCS9900 device ID: 0x99009710.
 *     3.3 VRP device ID: 0x9d148086.
 *
 * Summary: Create target VM, then use HC_ADD_VDEV to add an emulated device in hypervisor, if set its device ID
 * incorrect value, so fail to find the emulated device operations, should return -EINVAL.
 */
static void hypercall_acrn_t13649_hc_add_vdev_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct, with incorrect id.fields.vendor */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));
	vdev.id.fields.vendor += 1;

	/* Add emulated device */
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ADD_VDEV_005
 *
 * ACRN-10515: HC_ADD_VDEV is used to add an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. The create callback function fails, i.e. the vdev is not configured in the VM configuration.
 * The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_ADD_VDEV to add an emulated device in hypervisor,
 * if set the share memory region name not aligned with which in scenario xml definition,
 * so the create callback function fails, should return -EINVAL.
 */
static void hypercall_acrn_t13650_hc_add_vdev_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));
	memcpy(vdev.args, "shm_region2", sizeof(vdev.args));

	/* Add emulated device */
	result = acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_REMOVE_VDEV_001
 *
 * ACRN-10516: HC_REMOVE_VDEV is used to remove an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, add en emulated device in hypervisor, then use HC_REMOVE_VDEV to remove it,
 * on success should return 0.
 */
static void hypercall_acrn_t13651_hc_remove_vdev_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Add emulated device */
	acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Remove emulated device */
	result = acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_REMOVE_VDEV_002
 *
 * ACRN-10516: HC_REMOVE_VDEV is used to remove an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created or paused. The error code is -EINVAL.
 *
 * Summary: Create target VM, add en emulated device in hypervisor, then pause and destroy target VM, so target VM
 * state is not created or paused, then use HC_REMOVE_VDEV to remove it, should return -EINVAL.
 */
static void hypercall_acrn_t13652_hc_remove_vdev_002()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Add emulated device */
	acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Pause and destroy target VM (After that, target VM state is Powered off) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	/* Remove emulated device */
	result = acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	report("%s",  (result == -EINVAL), __FUNCTION__);
	/**
	 * Limitation: emulated device is not correctly removed before target VM shutdown.
	 * After this test, can not add this emulated device again.
	 */
}

/**
 * @brief Case Name: HC_REMOVE_VDEV_003
 *
 * ACRN-10516: HC_REMOVE_VDEV is used to remove an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the device information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + size of virtual device structure] is out of EPT scope. The error code is -EINVAL.
 *
 * Summary: Create target VM, add en emulated device in hypervisor, then use HC_REMOVE_VDEV to remove it,
 * if invalid 192-byte GPA is in RSI, should return -EINVAL.
 */
static void hypercall_acrn_t13653_hc_remove_vdev_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Remove emulated device, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_REMOVE_VDEV, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_REMOVE_VDEV_004
 *
 * ACRN-10516: HC_REMOVE_VDEV is used to remove an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. Fail to find the emulated device operations through device information,
 * i.e. the device ID contained in the information is not correct. The error code is -EINVAL.
 *
 * Summary: Create target VM, add en emulated device in hypervisor, then use HC_REMOVE_VDEV to remove it,
 * if set its device ID incorrect value, so fail to find the emulated device operations, should return -EINVAL.
 */
static void hypercall_acrn_t13654_hc_remove_vdev_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;
	uint16_t save_vendor;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Add emulated device */
	acrn_hypercall2(HC_ADD_VDEV, vmid, (uint64_t)&vdev);

	/* Remove emulated device, with incorrect fields.vendor */
	save_vendor = vdev.id.fields.vendor;
	vdev.id.fields.vendor += 1;
	result = acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, correctly remove emulated device */
	vdev.id.fields.vendor = save_vendor;
	acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_REMOVE_VDEV_005
 *
 * ACRN-10516: HC_REMOVE_VDEV is used to remove an emulated device in hypervisor.
 * The first parameter is the VM ID of the target VM. The second parameter is the device information.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. Fail to find the virtual device through device slot contained in device information,
 * which means the HC_ADD_VDEV hypercall was not called for this vdev before. The error code is -EINVAL.
 *
 * Summary: Create target VM, with add an emulated device, use HC_REMOVE_VDEV to remove it directly,
 * should return -EINVAL.
 */
static void hypercall_acrn_t13655_hc_remove_vdev_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_vdev vdev;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_vdev struct */
	memcpy(&vdev, &g_vdev, sizeof(g_vdev));

	/* Remove emulated device */
	result = acrn_hypercall2(HC_REMOVE_VDEV, vmid, (uint64_t)&vdev);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_PM_GET_CPU_STATE_001
 *
 * ACRN-10517: HC_PM_GET_CPU_STATE is used to get the vCPU power information and
 * write the information into guest memory.
 * The first parameter is the power management command to execute.
 * The second parameter is the GPA to copy power management information back.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then use HC_PM_GET_CPU_STATE to get the vCPU power information,
 * on success should return 0.
 */
static void hypercall_acrn_t13656_hc_pm_get_cpu_state_001()
{
	int64_t result;
	struct vm_creation create_vm;
	uint64_t cmd;
	uint64_t state;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Get and write vCPU power information */
	cmd = (vmid << PMCMD_VMID_SHIFT) & PMCMD_VMID_MASK;
	cmd |= ACRN_PMCMD_GET_CX_CNT;
	state = 2;
	result = acrn_hypercall2(HC_PM_GET_CPU_STATE, cmd, (uint64_t)&state);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_PM_GET_CPU_STATE_002
 *
 * ACRN-10517: HC_PM_GET_CPU_STATE is used to get the vCPU power information and
 * write the information into guest memory.
 * The first parameter is the power management command to execute.
 * The second parameter is the GPA to copy power management information back.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. The target VM state is not created. The error code is -1.
 *
 * Summary: Create and pause target VM, so target VM state is not created,
 * then use HC_PM_GET_CPU_STATE to get the vCPU power information, should return -1.
 */
static void hypercall_acrn_t13657_hc_pm_get_cpu_state_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint64_t cmd;
	uint64_t state;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Pause target VM (After that, target VM state is Paused) */
	acrn_hypercall1(HC_PAUSE_VM, vmid);

	/* Get and write vCPU power information */
	cmd = (vmid << PMCMD_VMID_SHIFT) & PMCMD_VMID_MASK;
	cmd |= ACRN_PMCMD_GET_CX_CNT;
	state = 2;
	result = acrn_hypercall2(HC_PM_GET_CPU_STATE, cmd, (uint64_t)&state);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_PM_GET_CPU_STATE_003
 *
 * ACRN-10517: HC_PM_GET_CPU_STATE is used to get the vCPU power information and
 * write the information into guest memory.
 * The first parameter is the power management command to execute.
 * The second parameter is the GPA to copy power management information back.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. Fail to copy the power information into guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + size of data] (size of data depends on the input command) is out of EPT scope.
 * The error code is -EINVAL.
 *
 * Summary: Create target VM, then use HC_PM_GET_CPU_STATE to get the vCPU power information,
 * if invalid GPA is in RSI, should return -EINVAL.
 */
static void hypercall_acrn_t13658_hc_pm_get_cpu_state_003()
{
	int64_t result;
	struct vm_creation create_vm;
	uint64_t cmd;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Get and write vCPU power information, with GPA start from 4GB which is out of EPT scope */
	cmd = (vmid << PMCMD_VMID_SHIFT) & PMCMD_VMID_MASK;
	cmd |= ACRN_PMCMD_GET_CX_CNT;
	result = acrn_hypercall2(HC_PM_GET_CPU_STATE, cmd, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -EINVAL), __FUNCTION__);
}

/**
 * @brief Case Name: HC_PM_GET_CPU_STATE_004
 *
 * ACRN-10517: HC_PM_GET_CPU_STATE is used to get the vCPU power information and
 * write the information into guest memory.
 * The first parameter is the power management command to execute.
 * The second parameter is the GPA to copy power management information back.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The PM command is got by logical and the first parameter and the 0x000000ffU.
 * If it is not valid, i.e. bigger than 3, the error code is -1.
 *
 * Summary: Create target VM, then use HC_PM_GET_CPU_STATE to get the vCPU power information,
 * if the command is invalid, should return -1.
 */
static void hypercall_acrn_t13659_hc_pm_get_cpu_state_004()
{
	int64_t result;
	struct vm_creation create_vm;
	uint64_t cmd;
	uint64_t state;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Get and write vCPU power information, with command type exceeding 3 */
	cmd = (vmid << PMCMD_VMID_SHIFT) & PMCMD_VMID_MASK;
	cmd |= 4;
	state = 2;
	result = acrn_hypercall2(HC_PM_GET_CPU_STATE, cmd, (uint64_t)&state);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}


/**
 * @brief Case Name: HC_SETUP_SBUF_001
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * On success, ACRN hypervisor shall return 0. But ACRN hypervisor shall return error code in below scenarios.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 * on success should return 0.
 */
static void hypercall_acrn_t13660_hc_setup_sbuf_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_sbuf_param struct */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));

	/* Setup a share buffer */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SETUP_SBUF_002
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. Fail to copy the setup parameters from guest memory which GPA is saved in the second parameter, i.e.
 * the [GPA, GPA + size of setup parameters structure (16 bytes)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 *  if invalid GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13661_hc_setup_sbuf_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SETUP_SBUF_003
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. The GPA of share buffer contained in the setup parameters is 0. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 *  if set its gpa to 0, should return -1.
 */
static void hypercall_acrn_t13662_hc_setup_sbuf_003()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_sbuf_param struct, with invalid gpa */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	asp.gpa = 0;

	/* Setup a share buffer */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SETUP_SBUF_004
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3.  The GPA of share buffer contained in the setup parameters is invalid (0x1UL << 52U)
 * so that the corresponding HPA cannot be found. The error code is -1.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 * if invalid GPA of share buffer is contained in the GPA in RSI, should return -1.
 */
static void hypercall_acrn_t13663_hc_setup_sbuf_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_sbuf_param struct, with invalid share buffer (1ULL << 32U) contained in the GPA in RSI */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	asp.gpa = GPA_4G_OUT_OF_EPT;

	/* Setup a share buffer */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SETUP_SBUF_005
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. If the share buffer id is not 64, the error code is -1.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 * if the share buffer id is not 64, should return -1.
 */
static void hypercall_acrn_t13664_hc_setup_sbuf_005()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	uint16_t vmid = 0;
	uint32_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_sbuf_param struct, with invalid share buffer id */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	do {
		random_value = get_random_value();
	} while (random_value == ACRN_ASYNCIO);
	asp.sbuf_id = random_value;

	/* Setup a share buffer */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_SETUP_SBUF_006
 *
 * ACRN-10755: HC_SETUP_SBUF is used to setup a share buffer between Service VM and a target VM.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the setup parameters for the
 * share buffer, it is a 16 bytes structure which contains cpu id, share buffer id and the GPA of share buffer.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 5. If the share buffer id is ACRN_ASYNCIO (64) and the share buffer structure got from the GPA of share buffer
 * contained in the setup parameters is valid, but the magic number contained in the share buffer structure doesn't
 * equal to SBUF_MAGIC (0x5aa57aa71aa13aa3UL), the error code is -1.
 *
 * Summary: Create target VM, then use HC_SETUP_SBUF to setup a share buffer between Service VM and a target VM,
 * if the magic number contained in the share buffer structure is invalid, should return -1.
 */
static void hypercall_acrn_t13665_hc_setup_sbuf_006()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct shared_buf *sbuf;
	uint16_t vmid = 0;
	uint64_t random_value;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Prepare the acrn_sbuf_param struct, with invalid magic number not equal SBUF_MAGIC (0x5aa57aa71aa13aa3UL) */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	sbuf = (struct shared_buf *)asp.gpa;
	do {
		random_value = get_random_value();
	} while (random_value == SBUF_MAGIC);
	sbuf->magic = random_value;

	/* Setup a share buffer */
	result = acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	sbuf->magic = SBUF_MAGIC; // restore magic

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_ASSIGN_001
 *
 * ACRN-10680: HC_ASYNCIO_ASSIGN is used to register ioevent fd to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, setup a share buffer, then use HC_ASYNCIO_ASSIGN to register ioevent fd to
 * communicate async IO event, on success should return 0.
 */
static void hypercall_acrn_t13666_hc_asyncio_assign_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/* Register ioevent fd */
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);

	/* Remove registered ioevent fd */
	acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_ASSIGN_002
 *
 * ACRN-10680: HC_ASYNCIO_ASSIGN is used to register ioevent fd to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. Fail to copy the async IO information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + size of async IO information structure (32 bytes)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then use HC_ASYNCIO_ASSIGN to register ioevent fd to communicate async IO event,
 * if invalid 32-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13667_hc_asyncio_assign_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Register ioevent fd, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_ASSIGN_003
 *
 * ACRN-10680: HC_ASYNCIO_ASSIGN is used to register ioevent fd to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. The addr contained in the async IO information is 0. The error code is -1.
 *
 * Summary: Create target VM, setup a share buffer, then use HC_ASYNCIO_ASSIGN to register ioevent fd to
 * communicate async IO event, if set its addr to 0, should return -1.
 */
static void hypercall_acrn_t13668_hc_asyncio_assign_003()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct, with invalid addr */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));
	async_info.addr = 0;

	/* Register ioevent fd */
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_ASSIGN_004
 *
 * ACRN-10680: HC_ASYNCIO_ASSIGN is used to register ioevent fd to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The async IO trying to be added has been registered, i.e. there is a member in vm->aiodesc_queue has same addr
 * and type as the input async IO information. Furthermore, the member's data is same as input async IO information,
 * or the member's match_data is 0, or the match_data of the input aysnc IO information is 0. The error code is -1.
 *
 * Summary: Create target VM, setup a share buffer, then use HC_ASYNCIO_ASSIGN to register ioevent fd to
 * communicate async IO event for 2 times, if they have the same addr and type, and one of their match_data is 0,
 * so the async IO trying to be added has been registered, should return -1.
 */
static void hypercall_acrn_t13669_hc_asyncio_assign_004()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/* Register ioevent fd */
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);
	if (result == 0) {
		chk++;
	}

	/* Register ioevent fd again, with the same addr and type, and match_data equal 0 */
	async_info.match_data = 0;
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);
	if (result == -1) {
		chk++;
	}

	/* Remove registered ioevent fd */
	async_info.match_data = g_async_info.match_data; // restore match_data
	acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_ASSIGN_005
 *
 * ACRN-10680: HC_ASYNCIO_ASSIGN is used to register ioevent fd to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 4. There is no free member in the async IO descriptor array maintained by target VM,
 * i.e. there is no async IO descriptor whose addr and fd are both 0. The error code is -1.
 *
 * Summary: Create target VM, setup a share buffer, then use HC_ASYNCIO_ASSIGN to register ioevent fd
 * for ACRN_ASYNCIO_MAX times, so that there is no free member in the async IO descriptor array
 * maintained by target VM, then use HC_ASYNCIO_ASSIGN again, should return -1.
 */
static void hypercall_acrn_t13670_hc_asyncio_assign_005()
{
	u8 chk = 0;
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;
	int i;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/**
	 * Register ioevent fd for ACRN_ASYNCIO_MAX times, so that no free member
	 * in the async IO descriptor array maintained by target VM.
	 */
	for (i = 0; i < ACRN_ASYNCIO_MAX; i++) {
		async_info.addr += 8;
		result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);
		//debug_print("try %d result: 0x%lx\n", i, result);
		if (result != 0) {
			break;
		}
	}
	if (i == ACRN_ASYNCIO_MAX) {
		chk++;
	}

	/* Register ioevent fd, when no free member in the async IO descriptor array maintained by target VM */
	async_info.addr += 8;
	result = acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);
	debug_print("test result: 0x%lx\n", result);
	if (result == -1) {
		chk++;
	}

	/* Clean env, remove registered ioevent fd */
	async_info.addr = g_async_info.addr;
	for (i = 0; i < ACRN_ASYNCIO_MAX; i++) {
		async_info.addr += 8;
		acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);
	}

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_DEASSIGN_001
 *
 * ACRN-10681: HC_ASYNCIO_DEASSIGN is used to remove registered ioevent fd which is used to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * On success, ACRN hypervisor shall return 0.
 *
 * Summary: Create target VM, then register ioevent fd to communicate async IO event, then use HC_ASYNCIO_DEASSIGN
 * to remove it, on success should return 0.
 */
static void hypercall_acrn_t13671_hc_asyncio_deassign_001()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/* Register ioevent fd */
	acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);

	/* Remove registered ioevent fd */
	result = acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == 0), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_DEASSIGN_002
 *
 * ACRN-10681: HC_ASYNCIO_DEASSIGN is used to remove registered ioevent fd which is used to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 1. Fail to copy the async IO information from guest memory which GPA is saved in the second parameter,
 * i.e. the [GPA, GPA + size of async IO information structure (32 bytes)] is out of EPT scope. The error code is -1.
 *
 * Summary: Create target VM, then register ioevent fd to communicate async IO event, then use HC_ASYNCIO_DEASSIGN
 * to remove it, if invalid 32-byte GPA is in RSI, should return -1.
 */
static void hypercall_acrn_t13672_hc_asyncio_deassign_002()
{
	int64_t result;
	struct vm_creation create_vm;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Remove registered ioevent fd, with GPA start from 4GB which is out of EPT scope */
	result = acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, GPA_4G_OUT_OF_EPT);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_DEASSIGN_003
 *
 * ACRN-10681: HC_ASYNCIO_DEASSIGN is used to remove registered ioevent fd which is used to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 2. The addr contained in the async IO information is 0. The error code is -1.
 *
 * Summary: Create target VM, then register ioevent fd to communicate async IO event, then use HC_ASYNCIO_DEASSIGN
 * to remove it, if set its addr to 0, should return -1.
 */
static void hypercall_acrn_t13673_hc_asyncio_deassign_003()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/* Register ioevent fd */
	acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);

	/* Remove registered ioevent fd, with invalid addr */
	async_info.addr = 0;
	result = acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

/**
 * @brief Case Name: HC_ASYNCIO_DEASSIGN_004
 *
 * ACRN-10681: HC_ASYNCIO_DEASSIGN is used to remove registered ioevent fd which is used to communicate async IO event.
 * The first parameter is the target VM id. The second parameter is the GPA which saves the async IO information,
 * it is a 32 bytes structure which contains members type, match_data, addr, fd and data.
 * Only when all of the following are true, we can say an async IO descriptor maintained by hypervisor matches the input async IO info:
 * 1) The input addr is same as the addr of async IO descriptor maintained by hypervisor.
 * 2) The input type is same as the type of async IO descriptor maintained by hypervisor.
 * 3) The input fd is same as the fd of async IO descriptor maintained by hypervisor.
 * 4) The input match_data and the match_data of async IO descriptor maintained by hypervisor are both true or both false.
 * 5) The input data is same as the data of async IO descriptor maintained by hypervisor.
 * But ACRN hypervisor shall return error code in below scenarios.
 * 3. The above match criteria cannot be fulfilled . The error code is -1.
 *
 * Summary: Create target VM, then register ioevent fd to communicate async IO event, then use HC_ASYNCIO_DEASSIGN
 * to remove it, if one of the type, match_data, addr, fd or data is incorrect value, should return -1.
 */
static void hypercall_acrn_t13674_hc_asyncio_deassign_004()
{
	int64_t result;
	struct vm_creation create_vm;
	struct acrn_sbuf_param asp;
	struct acrn_asyncio_info async_info;
	uint16_t vmid = 0;

	/* Create target VM */
	memcpy(&create_vm, &cv, sizeof(cv));
	acrn_hypercall1(HC_CREATE_VM, (uint64_t)&create_vm);
	vmid = create_vm.vmid;

	/* Setup a share buffer */
	memcpy(&asp, &g_sbuf_param, sizeof(g_sbuf_param));
	acrn_hypercall2(HC_SETUP_SBUF, vmid, (uint64_t)&asp);

	/* Prepare the acrn_asyncio_info struct */
	memcpy(&async_info, &g_async_info, sizeof(g_async_info));

	/* Register ioevent fd */
	acrn_hypercall2(HC_ASYNCIO_ASSIGN, vmid, (uint64_t)&async_info);

	/* Remove registered ioevent fd, with random selected incorrect type, match_data, addr, fd or data */
	switch (get_random_value() % 5) {
	case 0:
		async_info.type += 1;
		break;
	case 1:
		async_info.match_data = (async_info.match_data == 0) ? 1 : 0;
		break;
	case 2:
		async_info.addr += 0x10;
		break;
	case 3:
		async_info.fd += 1;
		break;
	case 4:
		async_info.data += 1;
		break;
	}
	debug_print("type:0x%x, match_data:0x%x, addr:0x%lx, fd:0x%lx, data:0x%lx\n", async_info.type, async_info.match_data, async_info.addr, async_info.fd, async_info.data);
	result = acrn_hypercall2(HC_ASYNCIO_DEASSIGN, vmid, (uint64_t)&async_info);

	/* Clean env, destroy target VM */
	acrn_hypercall1(HC_PAUSE_VM, vmid);
	acrn_hypercall1(HC_DESTROY_VM, vmid);

	report("%s",  (result == -1), __FUNCTION__);
}

#elif defined IN_SAFETY_VM

/**
 * @brief Case Name: inject_ud_when_hypercall_from_safety_vm
 *
 * ACRN-10492: Acrn hypervisor shall inject #UD to VM if the hypercall is not sent from Service VM.
 *
 * Summary: Try to execute a hypercall from Safety VM, should generate #UD.
 */
static void hypercall_acrn_t13547_inject_ud_when_hypercall_from_safety_vm()
{
	uint64_t version;

	acrn_hypercall1_checking(HC_GET_API_VERSION, (uint64_t)&version);

	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

#endif

#elif __i386__
#ifdef IN_NON_SAFETY_VM

/**
 * @brief Case Name: inject_ud_when_hypercall_under_protected_mode
 *
 * ACRN-10491: For non-64-bit modes, ACRN hypervisor shall inject #UD.
 *
 * Summary: Try to execute a hypercall under Protected Mode, should generate #UD.
 */
static void hypercall_acrn_t13546_inject_ud_when_hypercall_under_protected_mode()
{
	uint32_t version;

	__asm__ __volatile__(ASM_TRY("1f")
		"vmcall;"
		"1:"
		: : "D"((uint32_t)&version)
		: "memory"
	);

	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

#endif
#endif

#ifdef __x86_64__

#define NUM_NON_SAFETY_VM_CASES		107U
struct hypercall_case non_safety_vm_cases[NUM_NON_SAFETY_VM_CASES] = {
	{13581u, "inject_gp_when_hypercall_in_ring3"},
	{7416u, "unsupported_hypercall"},
	{7417u, "HC_GET_API_VERSION_001"},
	{7418u, "HC_GET_API_VERSION_002"},
	{7419u, "HC_SERVICE_VM_OFFLINE_CPU_001"},
	{7420u, "HC_SERVICE_VM_OFFLINE_CPU_002"},
	{7421u, "HC_SERVICE_VM_OFFLINE_CPU_003"},
	{13545u, "HC_SET_CALLBACK_VECTOR"},
	{7410u, "HC_CREATE_VM_001"},
	{7411u, "HC_CREATE_VM_002"},
	{7412u, "HC_CREATE_VM_003"},
	{7413u, "HC_CREATE_VM_004"},
	{7414u, "HC_PAUSE_VM_001"},
	{7415u, "HC_DESTROY_VM_001"},
	{13583u, "HC_RESET_VM_001"},
	{13582u, "fail_to_find_target_vm_by_vmid"},
	{13584u, "HC_START_VM_001"},
	{13585u, "HC_START_VM_002"},
	{13586u, "HC_START_VM_003"},
	{13587u, "HC_SET_IOREQ_BUFFER_001"},
	{13588u, "HC_SET_IOREQ_BUFFER_002"},
	{13589u, "HC_SET_IOREQ_BUFFER_003"},
	{13590u, "HC_SET_IOREQ_BUFFER_004"},
	{13600u, "HC_SET_VCPU_REGS_001"},
	{13591u, "HC_SET_VCPU_REGS_002"},
	{13592u, "HC_SET_VCPU_REGS_003"},
	{13593u, "HC_SET_VCPU_REGS_004"},
	{13594u, "HC_SET_VCPU_REGS_005"},
	{13595u, "HC_SET_VCPU_REGS_006"},
	{13596u, "HC_SET_VCPU_REGS_007"},
	{13597u, "HC_SET_IRQLINE_001"},
	{13598u, "HC_SET_IRQLINE_002"},
	{13599u, "HC_SET_IRQLINE_003"},
	{13601u, "HC_INJECT_MSI_001"},
	{13602u, "HC_INJECT_MSI_002"},
	{13603u, "HC_INJECT_MSI_003"},
	{13604u, "HC_INJECT_MSI_004"},
	{13605u, "HC_NOTIFY_REQUEST_FINISH_001"},
	{13606u, "HC_NOTIFY_REQUEST_FINISH_002"},
	{13607u, "HC_NOTIFY_REQUEST_FINISH_003"},
	{13608u, "HC_NOTIFY_REQUEST_FINISH_004"},
	{13609u, "HC_VM_SET_MEMORY_REGIONS_001"},
	{13610u, "HC_VM_SET_MEMORY_REGIONS_002"},
	{13611u, "HC_VM_SET_MEMORY_REGIONS_003"},
	{13612u, "HC_VM_SET_MEMORY_REGIONS_004"},
	{13613u, "HC_VM_SET_MEMORY_REGIONS_005"},
	{13614u, "HC_VM_SET_MEMORY_REGIONS_006"},
	{13615u, "HC_ASSIGN_PCIDEV_001"},
	{13616u, "HC_ASSIGN_PCIDEV_002"},
	{13617u, "HC_ASSIGN_PCIDEV_003"},
	{13618u, "HC_ASSIGN_PCIDEV_004"},
	{13619u, "HC_ASSIGN_PCIDEV_005"},
	{13620u, "HC_ASSIGN_PCIDEV_006"},
	{13621u, "HC_ASSIGN_PCIDEV_007"},
	{13622u, "HC_ASSIGN_PCIDEV_008"},
	{13623u, "HC_ASSIGN_PCIDEV_009"},
	{13624u, "HC_DEASSIGN_PCIDEV_001"},
	{13625u, "HC_DEASSIGN_PCIDEV_002"},
	{13626u, "HC_DEASSIGN_PCIDEV_003"},
	{13627u, "HC_DEASSIGN_PCIDEV_004"},
	{13628u, "HC_DEASSIGN_PCIDEV_005"},
	{13629u, "HC_DEASSIGN_PCIDEV_006"},
	{13630u, "HC_SET_PTDEV_INTR_INFO_001"},
	{13631u, "HC_SET_PTDEV_INTR_INFO_002"},
	{13632u, "HC_SET_PTDEV_INTR_INFO_003"},
	{13633u, "HC_SET_PTDEV_INTR_INFO_004"},
	{13634u, "HC_SET_PTDEV_INTR_INFO_005"},
	{13635u, "HC_SET_PTDEV_INTR_INFO_006"},
	{13636u, "HC_SET_PTDEV_INTR_INFO_007"},
	{13637u, "HC_SET_PTDEV_INTR_INFO_008"},
	{13638u, "HC_SET_PTDEV_INTR_INFO_009"},
	{13639u, "HC_RESET_PTDEV_INTR_INFO_001"},
	{13640u, "HC_RESET_PTDEV_INTR_INFO_002"},
	{13641u, "HC_RESET_PTDEV_INTR_INFO_003"},
	{13642u, "HC_RESET_PTDEV_INTR_INFO_004"},
	{13643u, "HC_RESET_PTDEV_INTR_INFO_005"},
	{13644u, "HC_RESET_PTDEV_INTR_INFO_006"},
	{13645u, "HC_RESET_PTDEV_INTR_INFO_007"},
	{13646u, "HC_ADD_VDEV_001"},
	{13647u, "HC_ADD_VDEV_002"},
	{13648u, "HC_ADD_VDEV_003"},
	{13649u, "HC_ADD_VDEV_004"},
	{13650u, "HC_ADD_VDEV_005"},
	{13651u, "HC_REMOVE_VDEV_001"},
	{13653u, "HC_REMOVE_VDEV_003"},
	{13654u, "HC_REMOVE_VDEV_004"},
	{13655u, "HC_REMOVE_VDEV_005"},
	{13652u, "HC_REMOVE_VDEV_002"},
	{13656u, "HC_PM_GET_CPU_STATE_001"},
	{13657u, "HC_PM_GET_CPU_STATE_002"},
	{13658u, "HC_PM_GET_CPU_STATE_003"},
	{13659u, "HC_PM_GET_CPU_STATE_004"},
	{13660u, "HC_SETUP_SBUF_001"},
	{13661u, "HC_SETUP_SBUF_002"},
	{13662u, "HC_SETUP_SBUF_003"},
	{13663u, "HC_SETUP_SBUF_004"},
	{13664u, "HC_SETUP_SBUF_005"},
	{13665u, "HC_SETUP_SBUF_006"},
	{13666u, "HC_ASYNCIO_ASSIGN_001"},
	{13667u, "HC_ASYNCIO_ASSIGN_002"},
	{13668u, "HC_ASYNCIO_ASSIGN_003"},
	{13669u, "HC_ASYNCIO_ASSIGN_004"},
	{13670u, "HC_ASYNCIO_ASSIGN_005"},
	{13671u, "HC_ASYNCIO_DEASSIGN_001"},
	{13672u, "HC_ASYNCIO_DEASSIGN_002"},
	{13673u, "HC_ASYNCIO_DEASSIGN_003"},
	{13674u, "HC_ASYNCIO_DEASSIGN_004"},
};

#define NUM_SAFETY_VM_CASES		1U
struct hypercall_case safety_vm_cases[NUM_SAFETY_VM_CASES] = {
	{13547u, "inject_ud_when_hypercall_from_safety_vm"},
};

#elif __i386__

#define NUM_NON_SAFETY_VM_CASES		1U
struct hypercall_case non_safety_vm_cases[NUM_NON_SAFETY_VM_CASES] = {
	{13546u, "inject_ud_when_hypercall_under_protected_mode"},
};

#define NUM_SAFETY_VM_CASES		0U
struct hypercall_case safety_vm_cases[NUM_NON_SAFETY_VM_CASES] = {

};

#endif

static void print_case_list(void)
{
	__unused uint32_t i;

	printf("\t\t hypercall feature case list:\n\r");

#ifdef IN_NON_SAFETY_VM
	for (i = 0U; i < NUM_NON_SAFETY_VM_CASES; i++) {
		printf("\t\t Case ID:%d Case Name::%s\n\r",
			non_safety_vm_cases[i].case_id, non_safety_vm_cases[i].case_name);
	}
#elif defined IN_SAFETY_VM
	for (i = 0U; i < NUM_SAFETY_VM_CASES; i++) {
		printf("\t\t Case ID:%d Case Name::%s\n\r",
			safety_vm_cases[i].case_id, safety_vm_cases[i].case_name);
	}
#endif

	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	setup_vm();
	setup_ring_env();

	print_case_list();

#ifdef __x86_64__
#ifdef IN_NON_SAFETY_VM

	setup_regions3();
	setup_pcidev();
	setup_vdev();
	setup_ptdev_irq();
	setup_sbuf();

	pci_pdev_enumerate_dev(pci_devs, &nr_pci_devs);
	if (nr_pci_devs == 0) {
		DBG_ERRO("pci_pdev_enumerate_dev finds no devs!");
		return 0;
	}

	hypercall_acrn_t13581_inject_gp_when_hypercall_in_ring3();

	hypercall_acrn_t7416_unsupported_hypercall();

	hypercall_acrn_t7417_hc_get_api_version_001();
	hypercall_acrn_t7418_hc_get_api_version_002();

	hypercall_acrn_t7419_hc_service_vm_offline_cpu_001();
	hypercall_acrn_t7420_hc_service_vm_offline_cpu_002();
	hypercall_acrn_t7421_hc_service_vm_offline_cpu_003();

	hypercall_acrn_t13545_hc_set_callback_vector();

	hypercall_acrn_t7410_hc_create_vm_001();
	hypercall_acrn_t7411_hc_create_vm_002();
	hypercall_acrn_t7412_hc_create_vm_003();
	hypercall_acrn_t7413_hc_create_vm_004();

	hypercall_acrn_t7414_hc_pause_vm_001();

	hypercall_acrn_t7415_hc_destroy_vm_001();

	hypercall_acrn_t13583_hc_reset_vm_001();

	hypercall_acrn_t13582_fail_to_find_target_vm_by_vmid();

	hypercall_acrn_t13584_hc_start_vm_001();
	hypercall_acrn_t13585_hc_start_vm_002();
	hypercall_acrn_t13586_hc_start_vm_003();

	hypercall_acrn_t13587_hc_set_ioreq_buffer_001();
	hypercall_acrn_t13588_hc_set_ioreq_buffer_002();
	hypercall_acrn_t13589_hc_set_ioreq_buffer_003();
	hypercall_acrn_t13590_hc_set_ioreq_buffer_004();

	hypercall_acrn_t13600_hc_set_vcpu_regs_001();
	hypercall_acrn_t13591_hc_set_vcpu_regs_002();
	hypercall_acrn_t13592_hc_set_vcpu_regs_003();
	hypercall_acrn_t13593_hc_set_vcpu_regs_004();
	hypercall_acrn_t13594_hc_set_vcpu_regs_005();
	hypercall_acrn_t13595_hc_set_vcpu_regs_006();
	hypercall_acrn_t13596_hc_set_vcpu_regs_007();

	hypercall_acrn_t13597_hc_set_irqline_001();
	hypercall_acrn_t13598_hc_set_irqline_002();
	hypercall_acrn_t13599_hc_set_irqline_003();

	hypercall_acrn_t13601_hc_inject_msi_001();
	hypercall_acrn_t13602_hc_inject_msi_002();
	hypercall_acrn_t13603_hc_inject_msi_003();
	hypercall_acrn_t13604_hc_inject_msi_004();

	hypercall_acrn_t13605_hc_notify_request_finish_001();
	hypercall_acrn_t13606_hc_notify_request_finish_002();
	hypercall_acrn_t13607_hc_notify_request_finish_003();
	hypercall_acrn_t13608_hc_notify_request_finish_004();

	hypercall_acrn_t13609_hc_vm_set_memory_regions_001();
	hypercall_acrn_t13610_hc_vm_set_memory_regions_002();
	hypercall_acrn_t13611_hc_vm_set_memory_regions_003();
	hypercall_acrn_t13612_hc_vm_set_memory_regions_004();
	hypercall_acrn_t13613_hc_vm_set_memory_regions_005();
	hypercall_acrn_t13614_hc_vm_set_memory_regions_006();

	hypercall_acrn_t13615_hc_assign_pcidev_001();
	hypercall_acrn_t13616_hc_assign_pcidev_002();
	hypercall_acrn_t13617_hc_assign_pcidev_003();
	hypercall_acrn_t13618_hc_assign_pcidev_004();
	hypercall_acrn_t13619_hc_assign_pcidev_005();
	hypercall_acrn_t13620_hc_assign_pcidev_006();
	hypercall_acrn_t13621_hc_assign_pcidev_007();
	hypercall_acrn_t13622_hc_assign_pcidev_008();
	hypercall_acrn_t13623_hc_assign_pcidev_009();

	hypercall_acrn_t13624_hc_deassign_pcidev_001();
	hypercall_acrn_t13625_hc_deassign_pcidev_002();
	hypercall_acrn_t13626_hc_deassign_pcidev_003();
	hypercall_acrn_t13627_hc_deassign_pcidev_004();
	hypercall_acrn_t13628_hc_deassign_pcidev_005();
	hypercall_acrn_t13629_hc_deassign_pcidev_006();

	hypercall_acrn_t13630_hc_set_ptdev_intr_info_001();
	hypercall_acrn_t13631_hc_set_ptdev_intr_info_002();
	hypercall_acrn_t13632_hc_set_ptdev_intr_info_003();
	hypercall_acrn_t13633_hc_set_ptdev_intr_info_004();
	hypercall_acrn_t13634_hc_set_ptdev_intr_info_005();
	hypercall_acrn_t13635_hc_set_ptdev_intr_info_006();
	hypercall_acrn_t13636_hc_set_ptdev_intr_info_007();
	hypercall_acrn_t13637_hc_set_ptdev_intr_info_008(); // TBD
	hypercall_acrn_t13638_hc_set_ptdev_intr_info_009();

	hypercall_acrn_t13639_hc_reset_ptdev_intr_info_001();
	hypercall_acrn_t13640_hc_reset_ptdev_intr_info_002();
	hypercall_acrn_t13641_hc_reset_ptdev_intr_info_003();
	hypercall_acrn_t13642_hc_reset_ptdev_intr_info_004();
	hypercall_acrn_t13643_hc_reset_ptdev_intr_info_005();
	hypercall_acrn_t13644_hc_reset_ptdev_intr_info_006();
	hypercall_acrn_t13645_hc_reset_ptdev_intr_info_007();

	hypercall_acrn_t13646_hc_add_vdev_001();
	hypercall_acrn_t13647_hc_add_vdev_002();
	hypercall_acrn_t13648_hc_add_vdev_003();
	hypercall_acrn_t13649_hc_add_vdev_004();
	hypercall_acrn_t13650_hc_add_vdev_005();

	hypercall_acrn_t13651_hc_remove_vdev_001();
	hypercall_acrn_t13653_hc_remove_vdev_003();
	hypercall_acrn_t13654_hc_remove_vdev_004();
	hypercall_acrn_t13655_hc_remove_vdev_005();
	/* hypercall_acrn_t13652_hc_remove_vdev_002 should run at the last of all hc_remove_vdev_* */
	hypercall_acrn_t13652_hc_remove_vdev_002();

	hypercall_acrn_t13656_hc_pm_get_cpu_state_001();
	hypercall_acrn_t13657_hc_pm_get_cpu_state_002();
	hypercall_acrn_t13658_hc_pm_get_cpu_state_003();
	hypercall_acrn_t13659_hc_pm_get_cpu_state_004();

	hypercall_acrn_t13660_hc_setup_sbuf_001();
	hypercall_acrn_t13661_hc_setup_sbuf_002();
	hypercall_acrn_t13662_hc_setup_sbuf_003();
	hypercall_acrn_t13663_hc_setup_sbuf_004();
	hypercall_acrn_t13664_hc_setup_sbuf_005();
	hypercall_acrn_t13665_hc_setup_sbuf_006();

	hypercall_acrn_t13666_hc_asyncio_assign_001();
	hypercall_acrn_t13667_hc_asyncio_assign_002();
	hypercall_acrn_t13668_hc_asyncio_assign_003();
	hypercall_acrn_t13669_hc_asyncio_assign_004();
	hypercall_acrn_t13670_hc_asyncio_assign_005();

	hypercall_acrn_t13671_hc_asyncio_deassign_001();
	hypercall_acrn_t13672_hc_asyncio_deassign_002();
	hypercall_acrn_t13673_hc_asyncio_deassign_003();
	hypercall_acrn_t13674_hc_asyncio_deassign_004();


#elif defined IN_SAFETY_VM
	hypercall_acrn_t13547_inject_ud_when_hypercall_from_safety_vm();
#endif

#elif __i386__
#ifdef IN_NON_SAFETY_VM
	hypercall_acrn_t13546_inject_ud_when_hypercall_under_protected_mode();
#endif

#endif

	return report_summary();
}
