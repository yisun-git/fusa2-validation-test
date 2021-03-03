#ifndef __HSI_VIRTUAL_SPEC_H
#define __HSI_VIRTUAL_SPEC_H

#include "libcflat.h"
#include "processor.h"
#include "bitops.h"
#include "asm/page.h"
#include "asm/io.h"

#define uint64_t u64
#define uint16_t u16
#define uint32_t u32
#define uint8_t u8
/* define area */
#define ALIGNED(x) __attribute__((aligned(x)))
#define VM_EXIT_REASON_DEFAULT_VALUE 0xffffU
#define INVALID_EXIT_QUALIFICATION_VALUE (0xFFFFFFFFUL)

#define APIC_TPR_VAL2 0xFFU
#define APIC_TPR_VAL1 (1U << 4U)
#define VMX_VIRTUAL_APIC_PAGE_ADDR_FULL 0x00002012U
/**
 * @brief The register address of IA32_X2APIC_TPR MSR.
 */
#define MSR_IA32_EXT_APIC_TPR			0x00000808U
#define MSR_IA32_PAT				0x00000277U
#define MSR_PLATFORM_INFO			0x000000ce
#define MSR_IA32_EXT_APIC_ICR			0x00000830U

/*
 * define vector for posted interrupts
 */
#define POSTED_INTR_VECTOR	(0xE3)
/* when guest get posted vector, should use this vector handle interrupt */
#define POSTED_INTR_REQUEST_VECTOR 0xA0U
/**< Mask of destination field in local APIC ICR register */
#define APIC_DEST_DESTFLD 0x00000000U

#define HANDLE_FIRST_NMI_TIME	(2 * 2100000000UL)
#define BP_HANDLE_SECOND_NMI_END 1U
#define EPT_MAPING_SIZE_4G	(1ULL << 32U)
/**
 * @brief Address of Posted-interrupt notification vector field in the VMCS.
 */
#define VMX_POSTED_INTERRUPT_NOTIFICATON_VECTOR_FULL          0x00000002U

/* use a unused address 384M started memory */
#define GUEST_PHYSICAL_ADDRESS_START (384 * 1024 * 1024UL)
/* map hpa to gpa size by EPT */
#define GUEST_MEMORY_MAP_SIZE (4 * 1024 * 1024ULL)
/* map hpa start address */
#define HOST_PHY_ADDR_START (3ULL << 30U)

/* map hpa to gpa size by EPT */
#define GUEST_MEMORY_MAP_64M (64 * 1024 * 1024ULL)

#define INVEPT_TYPE_SINGLE_CONTEXT      1UL
#define INVEPT_TYPE_ALL_CONTEXTS	    2UL

#define EPT_MAPPING_RIGHT 1U
#define VMCALL_EXIT_SECOND (1ULL << 16U)

/**
 * @brief Address of controls for PAUSE-loop Exiting PLE_GP.
 */
#define VMX_PAUSE_LOOP_PLE_GAP         0x00004020U

/**
 * @brief Address of controls for PAUSE-loop Exiting PLE_WINDOW.
 */
#define VMX_PAUSE_LOOP_PLE_WINDOW         0x00004022U
/* ple gap TSC time is 100 big enough for two pause instruction execute time. */
#define VMX_PLE_GAP_TSC 100U
/* ple window TSC time is 1000 small enough for execute pause loop. */
#define VMX_PLE_WINDOW_TSC 1000U

#define GUEST_DR7_VALUE1 (1ULL << 10U)
#define GUEST_DR7_VALUE2 (GUEST_DR7_VALUE1 | (1ULL << 13U))
#define GUEST_LOAD_DR7_VALUE1 (0x401UL)
#define GUEST_IA32_PERF_INIT_VALUE 0x0
#define HOST_IA32_PERF_INIT_VALUE 0x0

#define IA32_PERF_GUEST_SET_VALUE 0x1
#define IA32_PERF_HOST_SET_VALUE 0x1

#define VMX_GUEST_IA32_PERF_CTL_FULL    0x00002808U
#define VMX_HOST_IA32_PERF_CTL_FULL    0x00002C04U
#define MSR_IA32_PERF_GLOBAL_CTRL        0x0000038FU
#define VMX_GUEST_IA32_BNDCFGS_FULL    0x00002812U
/**
 * @brief Address of guest DR7 control field in the VMCS.
 */
#define VMX_GUEST_DR7                  0x0000681aU
#define VMCS_REVISION_ID_LEN 4

/**
 * @brief Address of guest ES segment limit control field in the VMCS.
 */
#define VMX_GUEST_ES_LIMIT              0x00004800U
#define GUEST_ES_LIMIT_TEST_VALUE 0x3ffffUL

/**
 * @brief Address of CR0 guest/host mask control field in the VMCS.
 */
#define VMX_CR0_GUEST_HOST_MASK 0x00006000U
/**
 * @brief Address of CR4 guest/host mask control field in the VMCS.
 */
#define VMX_CR4_GUEST_HOST_MASK 0x00006002U
/**
 * @brief Address of CR0 read shadow field in the VMCS.
 */
#define VMX_CR0_READ_SHADOW     0x00006004U
/**
 * @brief Address of CR4 read shadow field in the VMCS.
 */
#define VMX_CR4_READ_SHADOW     0x00006006U

#define API1_CORE_ID 1
#define VMCS_REVISION_ID_LEN 4
#define GUEST_ES_LIMIT_TEST_VALUE 0x3ffffUL
#define VM0_ID 0
#define VM_EXIT_CR3_NUM 3UL
#define VM_EXIT_CR8_NUM 8UL
#define VM_EXIT_CR0_NUM 0UL
#define VM_EXIT_CR4_NUM 4UL
#define X86_CR4_PVI (1UL << 1U)

/* Use RTC target register port number test */
#define IN_BYTE_TEST_PORT_ID 0x71UL
#define TEST_MSR_BITMAP_MSR MSR_IA32_PAT
#define MSR_WRITE_BITMAP_OFFSET 2048U
#define APIC_TPR_VAL2 0xFFU
#define APIC_TPR_VAL1 (1U << 4U)
#define PREEMPTION_TIME_VALUE 1000
#define APIC_MMIO_BASE_ADDR (0xfee00000UL)
#define MSR_APIC_ICR_VALUE 0U
#define ILLEGAL_VIRTUAL_APIC_PAGE_ADDR (0xFFFFFFFFFFFFFFFFULL)

/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * IA32_PAT MSR is saved on VM exit.
 */
#define VMX_EXIT_CTLS_SAVE_PAT    (1U << 18U)
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * IA32_PAT MSR is loaded on VM exit.
 */
#define VMX_EXIT_CTLS_LOAD_PAT    (1U << 19U)
/**
 * @brief Address of guest IA32_PAT control field in the VMCS.
 */
#define VMX_GUEST_IA32_PAT_FULL      0x00002804U
/**
 * @brief Address of host IA32_PAT control field in the VMCS.
 */
#define VMX_HOST_IA32_PAT_FULL  0x00002C00U
/* use for set ia32_pat value which diff form IA32_PAT_HOST_VMCS_VALUE */
#define IA32_PAT_VALUE1 0x0000000070406ULL
#define IA32_PAT_HOST_VMCS_INIT_VALUE 0x7040600070406ULL
#define IA32_PAT_GUEST_VMCS_INIT_VALUE 0x7040600070406ULL
#define IA32_PAT_VALUE2 0x0000000001040506ULL
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * IA32_EFER MSR is saved on VM exit.
 */
#define VMX_EXIT_CTLS_SAVE_EFER   (1U << 20U)
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * IA32_EFER MSR is loaded on VM exit.
 */
#define VMX_EXIT_CTLS_LOAD_EFER   (1U << 21U)
/**
 * @brief Address of guest IA32_EFER control field in the VMCS.
 */
#define VMX_GUEST_IA32_EFER_FULL     0x00002806U

/**
 * @brief Address of host IA32_EFER control field in the VMCS.
 */
#define VMX_HOST_IA32_EFER_FULL 0x00002C02U

#define IA32_EFER_VALUE1 0x500
#define IA32_EFER_HOST_VMCS_INIT_VALUE 0xD00
#define IA32_EFER_GUEST_VMCS_INIT_VALUE 0xD00
#define IA32_EFER_VALUE2 0xD00
#define IA32_EFER_HOST_PYHSICAL_VALUE 0xD01
#define MSR_IA32_EFER                 0xC0000080U
#define MSR_IA32_EXT_APIC_EOI			0x0000080BU
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * the value of the VMX-preemption timer is saved on VM exit.
 */
#define VMX_EXIT_CTLS_SAVE_PREE_TIMER (1U << 22U)

/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * the IA32_EFER MSR is loaded on VM entry.
 */
#define VMX_ENTRY_CTLS_LOAD_EFER  (1U << 15U)
/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * the IA32_PERF_GLOBAL_CTRL MSR is loaded on VM exit.
 */
#define VMX_EXIT_CTLS_LOAD_PERF_GLOBAL_CTRL (1U << 12U)
#define IA32_BNDCFGS_VALUE1 0U
/* Enable MPX in supervisor mode */
#define IA32_BNDCFGS_VALUE2 (1UL << 0)
/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * the IA32_BNDCFGS is loaded on VM entry.
 */
#define VMX_ENTRY_CTLS_LOAD_BNDCFGS (1U << 16U)

/**
 * @brief Address of MSR bitmaps control field in the VMCS.
 */
#define VMX_MSR_BITMAP_FULL          0x00002004U
/* use a unused address offset is 448M memory */
#define GUEST_PHYSICAL_ADDRESS_TEST (448 * 1024 * 1024UL)

/* use a unused address offset is 480M memory for change memory mapping */
#define HOST_PHYSICAL_ADDRESS_OFFSET (480 * 1024 * 1024UL)
/* magic number for test invept */
#define INIT_MAGIC_NUMBER (0x1122334455667788ULL)

#define MSR_IA32_TSC_AUX              0xC0000103U
#define TSC_AUX_VALUE1 0U
#define TSC_AUX_VALUE2 1U
#define BP_TSC_AUX_VALUE3 0xffU
#define MSR_CACHE_TYPE_WB_VALUE 0x0000000001040506ULL

/**
 * @brief Address of VM exit control field in the VMCS.
 */
#define VMX_EXIT_CONTROLS              0x0000400cU
/**
 * @brief Address of VM exit MSR store count control field in the VMCS.
 */
#define VMX_EXIT_MSR_STORE_COUNT       0x0000400eU
/**
 * @brief Address of VM exit MSR load count control field in the VMCS.
 */
#define VMX_EXIT_MSR_LOAD_COUNT        0x00004010U
/**
 * @brief Address of VM entry control field in the VMCS.
 */
#define VMX_ENTRY_CONTROLS             0x00004012U
/**
 * @brief Address of VM entry MSR load count control field in the VMCS.
 */
#define VMX_ENTRY_MSR_LOAD_COUNT       0x00004014U
/**
 * @brief Address of VM-exit MSR-load address control field in the VMCS.
 */
#define VMX_EXIT_MSR_LOAD_ADDR_FULL  0x00002008U
/**
 * @brief Address of VM-entry MSR-load address control field in the VMCS.
 */
#define VMX_ENTRY_MSR_LOAD_ADDR_FULL 0x0000200aU
/**
 * @brief Address of VM entry interruption information field in the VMCS.
 */
#define VMX_ENTRY_INT_INFO_FIELD       0x00004016U
/**
 * @brief Bit field in VM exit interrupt information that indicates
 * this interrupt information is valid.
 */
#define VMX_INT_INFO_VALID          (1U << 31U)
/**
 * @brief Interrupt type in VM exit interrupt information indicates this is a hardware exception.
 */
#define VMX_INT_TYPE_HW_EXP         3U
/**
 * @brief Undefined/Invalid Opcode vector in the Interrupt Descriptor Table (IDT).
 */
#define IDT_UD  6U
#define VMX_PREEMPTION_TIMER                 0x0000482EU
/* define for test save preemption timer on vm-exit */
#define SAVE_PREEMPTION_TIME_VALUE (0x10000000UL)
#define IA32_EFER_LME	(1ULL << 8U)

/**
 * @brief Address of secondary processor-based control field in the VMCS.
 */
#define HSI_VMX_PROC_VM_EXEC_CONTROLS2     0x0000401EU
/**
 * page-modification log address
 */
#define VMX_PAGE_MODIFICATION_LOG_ADDRESS 0x0000200EU
/**
 * page-modification log index feild
 */
#define VMX_PAGE_MODIFICATION_LOG_INDEX 0x0000812U

#define DEFAULT_PML_ENTRY 511U
/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * the IA32_PAT MSR is loaded on VM entry.
 */
#define VMX_ENTRY_CTLS_LOAD_PAT   (1U << 14U)

/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * the DR7 and the IA32_DEBUGCTL MSR is loaded on VM entry.
 */
#define VMX_ENTRY_CTLS_LOAD_DEBUG_REG  (1U << 2U)
#define MSR_IA32_BNDCFGS		0x00000d90
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * the IA32_BNDCFGS is cleared on VM exit.
 */
#define VMX_EXIT_CTLS_CLEAR_BNDCFGS (1U << 23U)
/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * a logical processor acknowledge the external interrupt on VM exit.
 */
#define VMX_EXIT_CTLS_ACK_IRQ     (1U << 15U)

#define EXTEND_INTERRUPT_80				0x80
#define EXTERNAL_INTER_VALID 1U
#define EXTERNAL_INTER_INVALID 0U
/**
 * @brief Address of VM exit interruption information field in the VMCS.
 */
#define VMX_EXIT_INT_INFO       0x00004404U

/**
 * @brief Single context INVVPID type which indicates only specified VPID's mappings will be invalidated.
 */
#define VMX_VPID_TYPE_SINGLE_CONTEXT 1UL
/**
 * @brief Single context INVVPID type which indicates all VPIDs' mappings will be invalidated.
 */
#define VMX_VPID_TYPE_ALL_CONTEXT    2UL

#define TSC_OFFSET_DEFAULT_VALUE (0x100000000000ULL)
/**
 * @brief Address of TSC offset control field in the VMCS.
 */
#define VMX_TSC_OFFSET_FULL          0x00002010U
#define TSC_SCALING_DEFAULT_VALUE 4
/**
 * @brief Address of TSC multiplier control field in the VMCS.
 */
#define VMX_TSC_MULTIPLIER_FULL          0x00002032U
/**
 * @brief The register address of IA32_TIME_STAMP_COUNTER MSR.
 */
#define MSR_IA32_TIME_STAMP_COUNTER 0x00000010U
/**
 * @brief Address of VM-exit MSR-store address control field in the VMCS.
 */
#define VMX_EXIT_MSR_STORE_ADDR_FULL 0x00002006U
#define UD_HANDLER_CALL_ONE_TIMES 1
#define HIGH_MSR_START_ADDR			0xc0000000U

/**
 * @brief Bit field in the IA32_VMX_EXIT_CTLS MSR that determines whether
 * a logical processor is in 64-bit mode after the next VM exit.
 */
#define VMX_EXIT_CTLS_HOST_ADDR64 (1U << 9U)
/**
 * @brief The LME bit of IA32_EFER.
 *
 * IA32e mode enable.
 * Enables IA-32e mode operation.
 */
#define MSR_IA32_EFER_LME_BIT (1UL << 8U)
/**
 * @brief The LMA bit of IA32_EFER.
 *
 * IA32e mode active.
 * Indicates IA-32e mode is active when set.
 */
#define MSR_IA32_EFER_LMA_BIT (1UL << 10U)
#define CS_L_BIT (1ULL << 53U)
/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * a logical processor is in IA-32e mode after VM entry.
 */
#define VMX_ENTRY_CTLS_IA32E_MODE (1U << 9U)
/**
 * @brief Bit field in the IA32_VMX_ENTRY_CTLS MSR that determines whether
 * whether the logical processor is in system management mode (SMM)
 * after VM entry.
 */
#define VMX_ENTRY_CTLS_ENTRY_TO_SMM (1U << 10U)
/**
 * @brief Address of exception bitmap control field in the VMCS.
 */
#define VMX_EXCEPTION_BITMAP           0x00004004U
#define CR4_RESERVED_BIT23				(1<<23)
/**
 * @brief CR0 cache disable bit.
 */
#define CR0_CD (1UL << 30U)
/**
 * @brief CR0 monitor coprocessor bit.
 */
#define CR0_MP (1UL << 1U)
/**
 * @brief CR0 not write through bit.
 */
#define CR0_NW (1UL << 29U)
#define X86_CR0_NW (1UL << 29)
#define CR4_SMEP (1UL << 20U)
#define CR4_PVI (1UL << 1U)

enum {
	MSR_READ,
	MSR_WRITE,
	MSR_READ_WRITE,
	MSR_BUTT,
};

#define GET_CASE_INFO(_condition, _case_id_, _case_fun_, _case_name_) \
{ \
	.condition = _condition,\
	.func = \
	hsi_rqmid_##_case_id_##_virtualization_specific_features_vm_exe_con_##_case_fun_##_001,\
	.case_id = _case_id_, \
	.pname = "HSI_Virtualization_Specific_Features_VM_Execution_Controls_"#_case_name_"_001",\
}

#define GET_CASE_INFO_GENERAL(_condition, _case_id_, _case_fun_, _case_name_) \
{ \
	.condition = _condition,\
	.func = \
	hsi_rqmid_##_case_id_##_virtualization_specific_features_##_case_fun_##_001,\
	.case_id = _case_id_, \
	.pname = "HSI_Virtualization_Specific_Features_"#_case_name_"_001",\
}

#define BUILD_CONDITION_FUNC(_func_, _vmcs_field_, _vmcs_bit, _is_set_bit) \
static void _func_##_condition(__unused struct st_vcpu *vcpu) \
{ \
	/* set or clear bit */ \
	exec_vmwrite32_bit(_vmcs_field_, _vmcs_bit, _is_set_bit); \
	DBG_INFO(#_vmcs_field_":0x%x\n", exec_vmread32(_vmcs_field_)); \
}

#define BUILD_GUEST_MAIN_FUNC(_func_, _case_suit_) \
static void _func_##_guest_main(void) \
{ \
	u32 i; \
	for (i = 0; i < ARRAY_SIZE(_case_suit_); i++) { \
		set_vmcs(_case_suit_[i].condition); \
		_case_suit_[i].func(); \
	} \
}

/* Access type according to SDM vol3 chapter 27.2.1 in Table 27-3 */
enum {
	ACCESS_TYPE_MOV_TO_CR = 0, //load
	ACCESS_TYPE_MOV_FROM_CR, //store
	ACCESS_TYPE_CLTS,
	ACCESS_TYPE_LMSW,
};
enum {
	VM_EXIT_QUALIFICATION_OTHERS_CR = 0,
	VM_EXIT_LOAD_CR3,
	VM_EXIT_STORE_CR3,
	VM_EXIT_LOAD_CR8,
	VM_EXIT_STORE_CR8,
};

enum {
	BIT_TYPE_CLEAR = 0,
	BIT_TYPE_SET,
};

/* posted-interrupt descriptor struct */
struct st_pi_desc {
	/* Posted Interrupt Requests, one bit per requested vector */
	uint64_t pi_req_bitmap[4];
	union {
		struct {
			/* Outstanding Notification */
			uint16_t on:1;

			/* Suppress Notification, of non-urgent interrupts */
			uint16_t sn:1;

			uint16_t rsvd_1:14;

			/* Notification Vector */
			uint8_t nv;

			uint8_t rsvd_2;

			/* Notification destination, a physical LAPIC ID */
			uint32_t ndst;
		} bits;
		uint64_t contrl_val;
	} controls;
	uint64_t rsvd[3];
} ALIGNED(64);

typedef enum {
	CON_PIN_CLEAR_EXTER_INTER_BIT = 0,
	CON_PIN_CLEAR_NMI_EXIT_BIT,
	CON_PIN_POST_INTER,
	CON_PIN_VIRT_NMI,
	CON_CPU_CTRL1_IRQ_WIN,
	CON_CPU_CTRL1_TPR_SHADOW,
	CON_CPU_CTRL1_HLT_EXIT,
	CON_CPU_CTRL1_INVLPG,
	CON_CPU_CTRL1_TSC_OFFSETTING,
	CON_CPU_CTRL1_MWAIT,
	CON_CPU_CTRL1_RDPMC,
	CON_CPU_CTRL1_RDTSC,
	CON_CPU_CTRL1_CR3_LOAD,
	CON_CPU_CTRL1_CR3_STORE,
	CON_CPU_CTRL1_CR8_LOAD,
	CON_CPU_CTRL1_CR8_STORE,
	CON_CPU_CTRL1_NMI_WIN,
	CON_CPU_CTRL1_MOV_DR,
	CON_CPU_CTRL1_UNCONDITIONAL_IO,
	CON_CPU_CTRL1_USE_IO_BITMAP,
	CON_CPU_CTRL1_MONITOR_TRP_FLAG,
	CON_CPU_CTRL1_USE_MSR_BITMAP_READ,
	CON_CPU_CTRL1_USE_MSR_BITMAP_WRITE,
	CON_CPU_CTRL1_MONITOR,
	CON_CPU_CTRL1_PAUSE,
	CON_CPU_CTRL1_ACTIVATE_SECOND_CONTROL,
	CON_CPU_CTRL1_PREEMPTION_TIMER,
	CON_CPU_CTRL2_ENABLE_EPT,
	CON_CPU_CTRL2_ENABLE_VPID,
	CON_CPU_CTRL2_PAUSE_LOOP,
	CON_CPU_CTRL2_ENABLE_VM_FUNC,
	CON_CPU_CTRL2_EPT_VIOLATION,
	CON_CPU_CTRL2_EPT_EXE_CONTRL,
	CON_CPU_CTRL2_VIRTUAL_APIC,
	CON_CPU_CTRL2_DESC_TABLE,
	CON_CPU_CTRL2_ENABLE_RDTSCP,
	CON_CPU_CTRL2_VIRTUAL_X2APIC,
	CON_CPU_CTRL2_WBINVD,
	CON_CPU_CTRL2_UNRESTRICTED_GUEST,
	CON_CPU_CTRL2_APIC_REG_VIRTUAL,
	CON_CPU_CTRL2_VIRTUAL_INTER_DELIVERY,
	CON_CPU_CTRL2_RDRAND,
	CON_CPU_CTRL2_VMCS_SHADOWING,
	CON_CPU_CTRL2_RDSEED,
	CON_CPU_CTRL2_ENABLE_PML,
	CON_CPU_CTRL2_ENABLE_XSAVES_XRSTORS,
	CON_CPU_CTRL2_USE_TSC_SCALING,
	CON_CPU_CTRL2_ENABLE_INVPCID,
	CON_EXIT_SAVE_DEBUG,
	CON_ENTRY_DEBUG_CONTROL,
	CON_ENTRY_LOAD_PERF,
	CON_ENTRY_LOAD_BNDCFGS,
	CON_EXIT_CLEAR_BNDCFGS,
	CON_EXIT_ACK_INTER,
	CON_EXIT_SAVE_PREEM_TIME,
	CON_EXIT_HOST_ADDR64,
	CON_ENTRY_IA32_MODE_GUEST,
	CON_ENTRY_TO_SMM,
	CON_IO_BITMAP,
	CON_EXCEPTION_BITMAP,

	CON_CR0_MASK,
	CON_CR0_READ_SHADOW,
	CON_CR4_MASK,
	CON_CR4_READ_SHADOW,
	CON_MSR_BITMAP,
	CON_EPT_CONSTRUCT,
	CON_EPT_READ_ACCESS,
	CON_EPT_WRITE_ACCESS,
	CON_EPT_EXECUTE_ACCESS,
	CON_EPT_MEM_TYPE_UC,
	CON_EPT_MEM_TYPE_WB,
	CON_VPID_INVALID,
	CON_ENTRY_GUEST_MSR_CONTRL,
	CON_EXIT_HOST_MSR_CONTRL,
	CON_EXIT_GUEST_MSR_STORE,
	CON_TSC_OFFSET_MULTI,
	CON_VM_EXIT_INFO,
	CON_EXIT_IA32_PAT,
	CON_ENTRY_IA32_PAT,
	CON_EXIT_IA32_EFER,
	CON_ENTRY_IA32_EFER,
	CON_EXIT_IA32_PERF,
	CON_BUFF,
} U_VMX_CONDITION;

typedef enum {
	CON_SECOND_CHECK_EPT_ENABLE = VMCALL_EXIT_SECOND,
	CON_SECOND_VM_EXIT_ENTRY,
	CON_SECOND_CHECK_DEBUG_REG,
	CON_CHANGE_MEMORY_MAPPING,
	CON_INVEPT,
	CON_INVVPID,
	CON_CHANGE_GUEST_MSR_AREA,
	CON_CHECK_TSC_AUX_VALUE,
	CON_CHECK_TSC_AUX_GUEST_VMCS_FIELD,
	CON_ENTRY_EVENT_INJECT,
	CON_GET_PML_INDEX,
	CON_42066_GET_RESULT,
	CON_CHANGE_GUEST_MSR_PAT,
	CON_42174_GET_RESULT,
	CON_CHANGE_GUEST_MSR_EFER,
	CON_42178_GET_RESULT,
	CON_42183_GET_RESULT,
	CON_42189_GET_PREEM_TIME,
	CON_42192_GET_RESULT,
	CON_42022_SET_CR0_READ_SHADOW,
	CON_42029_SET_CR4_READ_SHADOW,
	CON_42243_REPORT_CASE,

	CON_SECOND_BUFF,
} U_VM_EXIT_SECOND;

enum {
	CPU_BP_ID = 0,
	CPU_AP1_ID,
	CPU_AP2_ID,
	CPU_AP3_ID,
	CPU_BUFF_ID,
};

enum {
	EPT_VIOLATION_READ = 1ul << 0,
	EPT_VIOLATION_WRITE = 1ul << 1,
	EPT_VIOLATION_INST = 1ul << 2,
};
typedef enum {
	CASE_ID_BUTT = 0,
	CASE_ID_42244,
	CASE_ID_41113,
	CASE_ID_43420,
	CASE_ID_43421,
	CASE_ID_43423,
} EN_CASE_ID;

enum {
	TSC_FIRST_NMI_START = 0,
	TSC_FIRST_NMI_END,
	TSC_SECOND_NMI_START,
	TSC_NMI_BUFF,
};
enum {
	NMI_FIRST_TIME = 1,
	NMI_SECOND_TIME,
};

struct lapic_reg {
	u32 v;	/**<  Value of a local APIC register */
};

/**
 * @brief Structure definition presenting one local APIC register.
 *
 * @consistency N/A
 *
 * @alignment 4096
 *
 */
struct lapic_regs {
	struct lapic_reg rsv0[2]; /**< Reserved */
	struct lapic_reg id;	  /**< Local APIC ID Register */
	struct lapic_reg version; /**< Local APIC Version Register */
	struct lapic_reg rsv1[4]; /**< Reserved */
	struct lapic_reg tpr; /**< Task Priority Register (TPR) */
	struct lapic_reg apr; /**< Arbitration Priority Register (APR) */
	struct lapic_reg ppr; /**< Processor Priority Register (PPR) */
	struct lapic_reg eoi; /**< EOI Register */
	struct lapic_reg rrd; /**< Remote Read Register (RRD) */
	struct lapic_reg ldr; /**< Logical Destination Register */
	struct lapic_reg dfr; /**< Destination Format Register */
	struct lapic_reg svr; /**< Spurious Interrupt Vector Register */
	struct lapic_reg isr[8]; /**< In-Service Registers (ISR) */
	struct lapic_reg tmr[8]; /**< Trigger Mode Registers (TMR)*/
	struct lapic_reg irr[8]; /**< Interrupt Request Registers (IRR) */
	struct lapic_reg esr;	 /**< Error Status Register */
	struct lapic_reg rsv2[6]; /**< Reserved */
	struct lapic_reg lvt_cmci; /**< LVT Corrected Machine Check Interrupt (CMCI) Register */
	struct lapic_reg icr_lo;   /**< Interrupt Command Register (ICR); bits 0-31 */
	struct lapic_reg icr_hi;   /**< Interrupt Command Register (ICR); bits 32-63 */
	struct lapic_reg lvt[6];   /**< LVT Registers */
	struct lapic_reg icr_timer; /**< Initial Count Register (for Timer) */
	struct lapic_reg ccr_timer; /**< Current Count Register (for Timer) */
	struct lapic_reg rsv3[4];   /**< Reserved */
	struct lapic_reg dcr_timer; /**< Divide Configuration Register (for Timer) */
	struct lapic_reg self_ipi;  /**< SELF IPI Register, Available only in x2APIC mode */

	struct lapic_reg rsv5[192]; /**< Reserved, roundup sizeof current struct to 4KB */
} ALIGNED(PAGE_SIZE);

struct hsi_vlapic {
	/**
	 * @brief This field is used to cache the virtual LAPIC registers.
	 */
	struct lapic_regs apic_page;
} ALIGNED(PAGE_SIZE);

/* Intel SDM 24.8.2, the address must be 16-byte aligned */
struct msr_store_entry {
	uint32_t msr_index;
	uint64_t value;
} ALIGNED(16);

enum {
	MSR_AREA_TSC_AUX = 0,
	MSR_AREA_COUNT,
};

struct msr_store_area {
	struct msr_store_entry guest[MSR_AREA_COUNT];
	struct msr_store_entry host[MSR_AREA_COUNT];
};

enum {
	io_byte1 = 0,
	io_byte2,
	io_byte4,
	io_byte_buff,
};
/* Use PCI config read register port number1 test */
#define IO_BITMAP_TEST_PORT_NUM1 0xCFCUL
/* Use PCI config register port number2 test */
#define IO_BITMAP_TEST_PORT_NUM2 0xCF8UL
/**
 * @brief Address of I/O bitmap A control field in the VMCS for each I/O port in the range 0 through 0x7FFF.
 */
#define VMX_IO_BITMAP_A_FULL         0x00002000U
/**
 * @brief Address of I/O bitmap B control field in the VMCS for each I/O port in the range 0x8000 through 0xFFFF.
 */
#define VMX_IO_BITMAP_B_FULL         0x00002002U

enum {
	IO_ACCESS_SIZE_1BYTE = 0,
	IO_ACCESS_SIZE_2BYTE = 1,
	IO_ACCESS_SIZE_4BYTE = 3,
};
enum {
	INS_TYPE_OUT = 0,
	INS_TYPE_IN = 1,
};

struct vcpu_arch {
	/* vmcs region for this vcpu, MUST be 4KB-aligned */
	u8 vmcs[PAGE_SIZE];
	/* MSR bitmap region for this vcpu, MUST be 4-Kbyte aligned */
	u8 msr_bitmap[PAGE_SIZE];
	/* the I/O bitmap of vcpu */
	u8 io_bitmap[PAGE_SIZE * 2U];
	/**
	 * @brief per vcpu lapic
	 */
	struct hsi_vlapic vlapic;
	/* pid MUST be 64 bytes aligned */
	struct st_pi_desc pid ALIGNED(64);
	u8 pml[PAGE_SIZE] ALIGNED(PAGE_SIZE);
	unsigned long *pml4;
	u64 eptp;
	u64 guest_rip;
	u16 vpid;
	u32 exit_qualification;
	u32 guest_ins_len;
	u32 exit_reason;
	/* List of MSRS to be stored and loaded on VM exits or VM entries */
	struct msr_store_area msr_area;
} ALIGNED(PAGE_SIZE);
struct st_vcpu {
	struct vcpu_arch arch;
} ALIGNED(PAGE_SIZE);

typedef struct {
	u32 condition;
	void (*func)(void);
	u32 case_id;
	const char *pname;
} st_case_suit;

/* define for record vm exit */
enum {
	/* record write cr0.CD whether causes vm exit */
	VM_EXIT_WRITE_CR0_CD = 0,
	/* record write cr0.MP whether causes vm exit */
	VM_EXIT_WRITE_CR0_MP,
	/* record write cr4.SMEP whether causes vm exit */
	VM_EXIT_WRITE_CR4_SMEP,
	/* record write cr4.PVI whether causes vm exit */
	VM_EXIT_WRITE_CR4_PVI,
	/* record external interrupt 0x80 is valid on vm exit */
	VM_EXIT_EXTER_INTER_VALID,
	/* record ept violation exit caused by read memory */
	VM_EXIT_EPT_VIOLATION_READ,
	/* record ept violation exit caused by write memory */
	VM_EXIT_EPT_VIOLATION_WRITE,
	/* record RDMSR for special MSR_IA32_PAT vm exit */
	VM_EXIT_RDMSR_PAT,
	/* record RDMSR for special MSR_PLATFORM_INFO vm exit */
	VM_EXIT_RDMSR_PLATFROM,
	/* record RDMSR for apic icr register vm exit */
	VM_EXIT_RDMSR_APIC_ICR,

	VM_EXIT_RECORD_BUTT,
};

enum cpu_reg_name {
	/* General purpose register layout should align with
	 * struct acrn_gp_regs
	 */
	CPU_REG_RAX, /**< The RAX general purpose register */
	CPU_REG_RCX, /**< The RCX general purpose register */
	CPU_REG_RDX, /**< The RDX general purpose register */
	CPU_REG_RBX, /**< The RBX general purpose register */
	CPU_REG_RSP, /**< The RSP general purpose register */
	CPU_REG_RBP, /**< The RBP general purpose register */
	CPU_REG_RSI, /**< The RSI general purpose register */
	CPU_REG_RDI, /**< The RDI general purpose register */
	CPU_REG_R8, /**< The R8 general purpose register */
	CPU_REG_R9, /**< The R9 general purpose register */
	CPU_REG_R10, /**< The R10 general purpose register */
	CPU_REG_R11, /**< The R11 general purpose register */
	CPU_REG_R12, /**< The R12 general purpose register */
	CPU_REG_R13, /**< The R13 general purpose register */
	CPU_REG_R14, /**< The R14 general purpose register */
	CPU_REG_R15, /**< The R15 general purpose register */
	CPU_REG_BUFF,
};

enum {
	VM_EXIT_NOT_OCCURS = 0,
	VM_EXIT_OCCURS,
};

typedef struct {
	/* record general vm exit */
	u64 exit_reason;
	/* special vm exit reason recorded */
	u64 exit_reason_special;
	/* record execute IN instruction whether cause vm exit */
	uint8_t is_io_ins_exit;
	/* record INL for port io number1 vm exit */
	uint8_t is_io_num1_exit;
	/* record INL for port io number2 vm exit */
	uint8_t is_io_num2_exit;

	u32 exit_qualification;
	/* record ept mapping whether is right */
	u8 ept_map_flag;
	/* record access cr3,cr8 cause vm exit */
	uint32_t cr_qualification;
	/* record check result at host */
	u64 result;
	/* record execute RDMSR/WRMSR instruction whether cause vm exit */
	uint8_t is_msr_ins_exit;
	/* record WRMSR for special MSR_IA32_EXT_APIC_ICR vm exit */
	uint8_t is_wrmsr_apic_icr_exit;
	/* record #DB exception vm exit */
	uint8_t is_exception_db_exit;
	/* record #GP exception vm exit */
	uint8_t is_exception_gp_exit;

	/* record VM exit array */
	uint8_t is_vm_exit[VM_EXIT_RECORD_BUTT];
} st_vm_exit_info;

/* define check some result at host */
enum {
	RET_BUFF,
	RET_EXIT_SAVE_DEBUG,
	RET_CHECK_EPT_CONSTRUCT,
	RET_CHECK_TSC_AUX,
	RET_CHECK_GUEST_TSC_AUX_VMCS,
	RET_42066_CHECK_MSR_IA32_PAT,
	RET_42174_CHECK_MSR_IA32_EFER,
	RET_42178_CHECK_MSR_IA32_PERF,
	RET_42183_CHECK_MSR_IA32_BNDCFGS,
	RET_42189_CHECK_PREE_TIME,
	RET_42192_CHECK_RESULT,

};
typedef struct {
	u8 exit_reason;
	void (*exit_func)(struct st_vcpu *);
} st_vm_exit;

struct vmcs_hdr {
	u32 revision_id:31;
	u32 shadow_vmcs:1;
};

struct vmcs {
	struct vmcs_hdr hdr;
	u32 abort; /* VMX-abort indicator */
	/* VMCS data */
	char data[0];
};

struct invvpid_operand {
	u64 vpid;
	u64 gla;
};
struct regs {
	u64 rax;
	u64 rcx;
	u64 rdx;
	u64 rbx;
	u64 cr2;
	u64 rbp;
	u64 rsi;
	u64 rdi;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
	u64 rflags;
	u64 rsp;
};

struct vmentry_failure {
	/* Did a vmlaunch or vmresume fail? */
	bool vmlaunch;
	/* Instruction mnemonic (for convenience). */
	const char *instr;
	/* Did the instruction return right away, or did we jump to HOST_RIP? */
	bool early;
	/* Contents of [re]flags after failed entry. */
	unsigned long flags;
};

struct vmx_test {
	const char *name;
	int (*init)(struct vmcs *vmcs);
	void (*guest_main)(void);
	int (*exit_handler)(void);
	void (*syscall_handler)(u64 syscall_no);
	struct regs guest_regs;
	int (*entry_failure_handler)(struct vmentry_failure *failure);
	struct vmcs *vmcs;
	int exits;
	/* Alternative test interface. */
	void (*v2)(void);
};

union vmx_basic {
	u64 val;
	struct {
		u32 revision;
		u32	size:13,
			reserved1: 3,
			width:1,
			dual:1,
			type:4,
			insouts:1,
			ctrl:1,
			reserved2:8;
	};
};

union vmx_ctrl_msr {
	u64 val;
	struct {
		u32 set, clr;
	};
};

union vmx_ept_vpid {
	u64 val;
	struct {
		u32:16,
			super:2,
			: 2,
			invept:1,
			: 11;
		u32	invvpid:1;
	};
};

enum Encoding {
	/* 16-Bit Control Fields */
	VPID			= 0x0000ul,
	/* Posted-interrupt notification vector */
	PINV			= 0x0002ul,
	/* EPTP index */
	EPTP_IDX		= 0x0004ul,

	/* 16-Bit Guest State Fields */
	GUEST_SEL_ES		= 0x0800ul,
	GUEST_SEL_CS		= 0x0802ul,
	GUEST_SEL_SS		= 0x0804ul,
	GUEST_SEL_DS		= 0x0806ul,
	GUEST_SEL_FS		= 0x0808ul,
	GUEST_SEL_GS		= 0x080aul,
	GUEST_SEL_LDTR		= 0x080cul,
	GUEST_SEL_TR		= 0x080eul,
	GUEST_INT_STATUS	= 0x0810ul,
	GUEST_PML_INDEX         = 0x0812ul,

	/* 16-Bit Host State Fields */
	HOST_SEL_ES		= 0x0c00ul,
	HOST_SEL_CS		= 0x0c02ul,
	HOST_SEL_SS		= 0x0c04ul,
	HOST_SEL_DS		= 0x0c06ul,
	HOST_SEL_FS		= 0x0c08ul,
	HOST_SEL_GS		= 0x0c0aul,
	HOST_SEL_TR		= 0x0c0cul,

	/* 64-Bit Control Fields */
	IO_BITMAP_A		= 0x2000ul,
	IO_BITMAP_B		= 0x2002ul,
	MSR_BITMAP		= 0x2004ul,
	EXIT_MSR_ST_ADDR	= 0x2006ul,
	EXIT_MSR_LD_ADDR	= 0x2008ul,
	ENTER_MSR_LD_ADDR	= 0x200aul,
	VMCS_EXEC_PTR		= 0x200cul,
	TSC_OFFSET		= 0x2010ul,
	TSC_OFFSET_HI		= 0x2011ul,
	APIC_VIRT_ADDR		= 0x2012ul,
	APIC_ACCS_ADDR		= 0x2014ul,
	POSTED_INTR_DESC_ADDR	= 0x2016ul,
	EPTP			= 0x201aul,
	EPTP_HI			= 0x201bul,
	VMREAD_BITMAP           = 0x2026ul,
	VMREAD_BITMAP_HI        = 0x2027ul,
	VMWRITE_BITMAP          = 0x2028ul,
	VMWRITE_BITMAP_HI       = 0x2029ul,
	EOI_EXIT_BITMAP0	= 0x201cul,
	EOI_EXIT_BITMAP1	= 0x201eul,
	EOI_EXIT_BITMAP2	= 0x2020ul,
	EOI_EXIT_BITMAP3	= 0x2022ul,
	PMLADDR                 = 0x200eul,
	PMLADDR_HI              = 0x200ful,


	/* 64-Bit Readonly Data Field */
	INFO_PHYS_ADDR		= 0x2400ul,

	/* 64-Bit Guest State */
	VMCS_LINK_PTR		= 0x2800ul,
	VMCS_LINK_PTR_HI	= 0x2801ul,
	GUEST_DEBUGCTL		= 0x2802ul,
	GUEST_DEBUGCTL_HI	= 0x2803ul,
	GUEST_EFER		= 0x2806ul,
	GUEST_PAT		= 0x2804ul,
	GUEST_PERF_GLOBAL_CTRL	= 0x2808ul,
	GUEST_PDPTE		= 0x280aul,

	/* 64-Bit Host State */
	HOST_PAT		= 0x2c00ul,
	HOST_EFER		= 0x2c02ul,
	HOST_PERF_GLOBAL_CTRL	= 0x2c04ul,

	/* 32-Bit Control Fields */
	PIN_CONTROLS		= 0x4000ul,
	CPU_EXEC_CTRL0		= 0x4002ul,
	EXC_BITMAP		= 0x4004ul,
	PF_ERROR_MASK		= 0x4006ul,
	PF_ERROR_MATCH		= 0x4008ul,
	CR3_TARGET_COUNT	= 0x400aul,
	EXI_CONTROLS		= 0x400cul,
	EXI_MSR_ST_CNT		= 0x400eul,
	EXI_MSR_LD_CNT		= 0x4010ul,
	ENT_CONTROLS		= 0x4012ul,
	ENT_MSR_LD_CNT		= 0x4014ul,
	ENT_INTR_INFO		= 0x4016ul,
	ENT_INTR_ERROR		= 0x4018ul,
	ENT_INST_LEN		= 0x401aul,
	TPR_THRESHOLD		= 0x401cul,
	CPU_EXEC_CTRL1		= 0x401eul,

	/* 32-Bit R/O Data Fields */
	VMX_INST_ERROR		= 0x4400ul,
	EXI_REASON		= 0x4402ul,
	EXI_INTR_INFO		= 0x4404ul,
	EXI_INTR_ERROR		= 0x4406ul,
	IDT_VECT_INFO		= 0x4408ul,
	IDT_VECT_ERROR		= 0x440aul,
	EXI_INST_LEN		= 0x440cul,
	EXI_INST_INFO		= 0x440eul,

	/* 32-Bit Guest State Fields */
	GUEST_LIMIT_ES		= 0x4800ul,
	GUEST_LIMIT_CS		= 0x4802ul,
	GUEST_LIMIT_SS		= 0x4804ul,
	GUEST_LIMIT_DS		= 0x4806ul,
	GUEST_LIMIT_FS		= 0x4808ul,
	GUEST_LIMIT_GS		= 0x480aul,
	GUEST_LIMIT_LDTR	= 0x480cul,
	GUEST_LIMIT_TR		= 0x480eul,
	GUEST_LIMIT_GDTR	= 0x4810ul,
	GUEST_LIMIT_IDTR	= 0x4812ul,
	GUEST_AR_ES		= 0x4814ul,
	GUEST_AR_CS		= 0x4816ul,
	GUEST_AR_SS		= 0x4818ul,
	GUEST_AR_DS		= 0x481aul,
	GUEST_AR_FS		= 0x481cul,
	GUEST_AR_GS		= 0x481eul,
	GUEST_AR_LDTR		= 0x4820ul,
	GUEST_AR_TR		= 0x4822ul,
	GUEST_INTR_STATE	= 0x4824ul,
	GUEST_ACTV_STATE	= 0x4826ul,
	GUEST_SMBASE		= 0x4828ul,
	GUEST_SYSENTER_CS	= 0x482aul,
	PREEMPT_TIMER_VALUE	= 0x482eul,

	/* 32-Bit Host State Fields */
	HOST_SYSENTER_CS	= 0x4c00ul,

	/* Natural-Width Control Fields */
	CR0_MASK		= 0x6000ul,
	CR4_MASK		= 0x6002ul,
	CR0_READ_SHADOW		= 0x6004ul,
	CR4_READ_SHADOW		= 0x6006ul,
	CR3_TARGET_0		= 0x6008ul,
	CR3_TARGET_1		= 0x600aul,
	CR3_TARGET_2		= 0x600cul,
	CR3_TARGET_3		= 0x600eul,

	/* Natural-Width R/O Data Fields */
	EXI_QUALIFICATION	= 0x6400ul,
	IO_RCX			= 0x6402ul,
	IO_RSI			= 0x6404ul,
	IO_RDI			= 0x6406ul,
	IO_RIP			= 0x6408ul,
	GUEST_LINEAR_ADDRESS	= 0x640aul,

	/* Natural-Width Guest State Fields */
	GUEST_CR0		= 0x6800ul,
	GUEST_CR3		= 0x6802ul,
	GUEST_CR4		= 0x6804ul,
	GUEST_BASE_ES		= 0x6806ul,
	GUEST_BASE_CS		= 0x6808ul,
	GUEST_BASE_SS		= 0x680aul,
	GUEST_BASE_DS		= 0x680cul,
	GUEST_BASE_FS		= 0x680eul,
	GUEST_BASE_GS		= 0x6810ul,
	GUEST_BASE_LDTR		= 0x6812ul,
	GUEST_BASE_TR		= 0x6814ul,
	GUEST_BASE_GDTR		= 0x6816ul,
	GUEST_BASE_IDTR		= 0x6818ul,
	GUEST_DR7		= 0x681aul,
	GUEST_RSP		= 0x681cul,
	GUEST_RIP		= 0x681eul,
	GUEST_RFLAGS		= 0x6820ul,
	GUEST_PENDING_DEBUG	= 0x6822ul,
	GUEST_SYSENTER_ESP	= 0x6824ul,
	GUEST_SYSENTER_EIP	= 0x6826ul,

	/* Natural-Width Host State Fields */
	HOST_CR0		= 0x6c00ul,
	HOST_CR3		= 0x6c02ul,
	HOST_CR4		= 0x6c04ul,
	HOST_BASE_FS		= 0x6c06ul,
	HOST_BASE_GS		= 0x6c08ul,
	HOST_BASE_TR		= 0x6c0aul,
	HOST_BASE_GDTR		= 0x6c0cul,
	HOST_BASE_IDTR		= 0x6c0eul,
	HOST_SYSENTER_ESP	= 0x6c10ul,
	HOST_SYSENTER_EIP	= 0x6c12ul,
	HOST_RSP		= 0x6c14ul,
	HOST_RIP		= 0x6c16ul
};

#define VMX_ENTRY_FAILURE	(1ul << 31)
#define VMX_ENTRY_FLAGS		(X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF | \
				 X86_EFLAGS_ZF | X86_EFLAGS_SF | X86_EFLAGS_OF)

enum Reason {
	VMX_EXC_NMI		= 0,
	VMX_EXTINT		= 1,
	VMX_TRIPLE_FAULT	= 2,
	VMX_INIT		= 3,
	VMX_SIPI		= 4,
	VMX_SMI_IO		= 5,
	VMX_SMI_OTHER		= 6,
	VMX_INTR_WINDOW		= 7,
	VMX_NMI_WINDOW		= 8,
	VMX_TASK_SWITCH		= 9,
	VMX_CPUID		= 10,
	VMX_GETSEC		= 11,
	VMX_HLT			= 12,
	VMX_INVD		= 13,
	VMX_INVLPG		= 14,
	VMX_RDPMC		= 15,
	VMX_RDTSC		= 16,
	VMX_RSM			= 17,
	VMX_VMCALL		= 18,
	VMX_VMCLEAR		= 19,
	VMX_VMLAUNCH		= 20,
	VMX_VMPTRLD		= 21,
	VMX_VMPTRST		= 22,
	VMX_VMREAD		= 23,
	VMX_VMRESUME		= 24,
	VMX_VMWRITE		= 25,
	VMX_VMXOFF		= 26,
	VMX_VMXON		= 27,
	VMX_CR			= 28,
	VMX_DR			= 29,
	VMX_IO			= 30,
	VMX_RDMSR		= 31,
	VMX_WRMSR		= 32,
	VMX_FAIL_STATE		= 33,
	VMX_FAIL_MSR		= 34,
	VMX_MWAIT		= 36,
	VMX_MTF			= 37,
	VMX_MONITOR		= 39,
	VMX_PAUSE		= 40,
	VMX_FAIL_MCHECK		= 41,
	VMX_TPR_THRESHOLD	= 43,
	VMX_APIC_ACCESS		= 44,
	VMX_EOI_INDUCED		= 45,
	VMX_GDTR_IDTR		= 46,
	VMX_LDTR_TR		= 47,
	VMX_EPT_VIOLATION	= 48,
	VMX_EPT_MISCONFIG	= 49,
	VMX_INVEPT		= 50,
	VMX_PREEMPT		= 52,
	VMX_INVVPID		= 53,
	VMX_WBINVD		= 54,
	VMX_XSETBV		= 55,
	VMX_APIC_WRITE		= 56,
	VMX_RDRAND		= 57,
	VMX_INVPCID		= 58,
	VMX_VMFUNC		= 59,
	VMX_ENCLS		= 60,
	VMX_RDSEED		= 61,
	VMX_PML_FULL		= 62,
	VMX_XSAVES		= 63,
	VMX_XRSTORS		= 64,
	VMX_EXIT_REASON_NUM,
};

enum Ctrl_exi {
	EXI_SAVE_DBGCTLS	= 1UL << 2,
	EXI_HOST_64		= 1UL << 9,
	EXI_LOAD_PERF		= 1UL << 12,
	EXI_INTA		= 1UL << 15,
	EXI_SAVE_PAT		= 1UL << 18,
	EXI_LOAD_PAT		= 1UL << 19,
	EXI_SAVE_EFER		= 1UL << 20,
	EXI_LOAD_EFER		= 1UL << 21,
	EXI_SAVE_PREEMPT	= 1UL << 22,
};

enum Ctrl_ent {
	ENT_LOAD_DBGCTLS	= 1UL << 2,
	ENT_GUEST_64		= 1UL << 9,
	ENT_LOAD_PERF		= 1UL << 13,
	ENT_LOAD_PAT		= 1UL << 14,
	ENT_LOAD_EFER		= 1UL << 15,
};

enum Ctrl_pin {
	PIN_EXTINT		= 1ul << 0,
	PIN_NMI			= 1ul << 3,
	PIN_VIRT_NMI		= 1ul << 5,
	PIN_PREEMPT		= 1ul << 6,
	PIN_POST_INTR		= 1ul << 7,
};

enum Ctrl0 {
	CPU_INTR_WINDOW		= 1ul << 2,
	CPU_TSC_OFFSET		= 1ul << 3,
	CPU_HLT			= 1ul << 7,
	CPU_INVLPG		= 1ul << 9,
	CPU_MWAIT		= 1ul << 10,
	CPU_RDPMC		= 1ul << 11,
	CPU_RDTSC		= 1ul << 12,
	CPU_CR3_LOAD		= 1ul << 15,
	CPU_CR3_STORE		= 1ul << 16,
	CPU_CR8_LOAD		= 1ul << 19,
	CPU_CR8_STORE		= 1ul << 20,
	CPU_TPR_SHADOW		= 1ul << 21,
	CPU_NMI_WINDOW		= 1ul << 22,
	CPU_IO			= 1ul << 24,
	CPU_MOV_DR		= 1ul << 23,
	CPU_IO_BITMAP		= 1ul << 25,
	CPU_MONITOR_TRAP_FLAG = 1ul << 27,
	CPU_MSR_BITMAP		= 1ul << 28,
	CPU_MONITOR		= 1ul << 29,
	CPU_PAUSE		= 1ul << 30,
	CPU_SECONDARY		= 1ul << 31,
};

enum Ctrl1 {
	CPU_VIRT_APIC_ACCESSES	= 1ul << 0,
	CPU_EPT			= 1ul << 1,
	CPU_DESC_TABLE		= 1ul << 2,
	CPU_RDTSCP		= 1ul << 3,
	CPU_VIRT_X2APIC		= 1ul << 4,
	CPU_VPID		= 1ul << 5,
	CPU_WBINVD		= 1ul << 6,
	CPU_URG			= 1ul << 7,
	CPU_APIC_REG_VIRT	= 1ul << 8,
	CPU_VINTD		= 1ul << 9,
	CPU_PAUSE_LOOP	= 1ul << 10,
	CPU_RDRAND		= 1ul << 11,
	CPU_INVPCID		= 1UL << 12,
	CPU_VMFUNC		= 1ul << 13,
	CPU_SHADOW_VMCS		= 1ul << 14,
	CPU_RDSEED		= 1ul << 16,
	CPU_PML                 = 1ul << 17,
	CPU_EPT_VIOLATION	= 1ul << 18,
	CPU_XSAVES_XRSTORS	= 1ul << 20,
	CPU_MODE_BASE_EXE_CON_FOR_EPT = 1ul << 22,
	CPU_TSC_SCALLING	= 1ul << 25,
};

enum Intr_type {
	VMX_INTR_TYPE_EXT_INTR = 0,
	VMX_INTR_TYPE_NMI_INTR = 2,
	VMX_INTR_TYPE_HARD_EXCEPTION = 3,
	VMX_INTR_TYPE_SOFT_INTR = 4,
	VMX_INTR_TYPE_SOFT_EXCEPTION = 6,
};

/*
 * Interruption-information format
 */
#define INTR_INFO_VECTOR_MASK           0xff            /* 7:0 */
#define INTR_INFO_INTR_TYPE_MASK        0x700           /* 10:8 */
#define INTR_INFO_DELIVER_CODE_MASK     0x800           /* 11 */
#define INTR_INFO_UNBLOCK_NMI_MASK      0x1000          /* 12 */
#define INTR_INFO_VALID_MASK            0x80000000      /* 31 */

#define INTR_INFO_INTR_TYPE_SHIFT       8

#define INTR_TYPE_EXT_INTR              (0 << 8) /* external interrupt */
#define INTR_TYPE_RESERVED              (1 << 8) /* reserved */
#define INTR_TYPE_NMI_INTR		(2 << 8) /* NMI */
#define INTR_TYPE_HARD_EXCEPTION	(3 << 8) /* processor exception */
#define INTR_TYPE_SOFT_INTR             (4 << 8) /* software interrupt */
#define INTR_TYPE_PRIV_SW_EXCEPTION	(5 << 8) /* priv. software exception */
#define INTR_TYPE_SOFT_EXCEPTION	(6 << 8) /* software exception */
#define INTR_TYPE_OTHER_EVENT           (7 << 8) /* other event */

/*
 * VM-instruction error numbers
 */
enum vm_instruction_error_number {
	VMXERR_VMCALL_IN_VMX_ROOT_OPERATION = 1,
	VMXERR_VMCLEAR_INVALID_ADDRESS = 2,
	VMXERR_VMCLEAR_VMXON_POINTER = 3,
	VMXERR_VMLAUNCH_NONCLEAR_VMCS = 4,
	VMXERR_VMRESUME_NONLAUNCHED_VMCS = 5,
	VMXERR_VMRESUME_AFTER_VMXOFF = 6,
	VMXERR_ENTRY_INVALID_CONTROL_FIELD = 7,
	VMXERR_ENTRY_INVALID_HOST_STATE_FIELD = 8,
	VMXERR_VMPTRLD_INVALID_ADDRESS = 9,
	VMXERR_VMPTRLD_VMXON_POINTER = 10,
	VMXERR_VMPTRLD_INCORRECT_VMCS_REVISION_ID = 11,
	VMXERR_UNSUPPORTED_VMCS_COMPONENT = 12,
	VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT = 13,
	VMXERR_VMXON_IN_VMX_ROOT_OPERATION = 15,
	VMXERR_ENTRY_INVALID_EXECUTIVE_VMCS_POINTER = 16,
	VMXERR_ENTRY_NONLAUNCHED_EXECUTIVE_VMCS = 17,
	VMXERR_ENTRY_EXECUTIVE_VMCS_POINTER_NOT_VMXON_POINTER = 18,
	VMXERR_VMCALL_NONCLEAR_VMCS = 19,
	VMXERR_VMCALL_INVALID_VM_EXIT_CONTROL_FIELDS = 20,
	VMXERR_VMCALL_INCORRECT_MSEG_REVISION_ID = 22,
	VMXERR_VMXOFF_UNDER_DUAL_MONITOR_TREATMENT_OF_SMIS_AND_SMM = 23,
	VMXERR_VMCALL_INVALID_SMM_MONITOR_FEATURES = 24,
	VMXERR_ENTRY_INVALID_VM_EXECUTION_CONTROL_FIELDS_IN_EXECUTIVE_VMCS = 25,
	VMXERR_ENTRY_EVENTS_BLOCKED_BY_MOV_SS = 26,
	VMXERR_INVALID_OPERAND_TO_INVEPT_INVVPID = 28,
};

#define SAVE_GPR				\
	"xchg %rax, regs\n\t"			\
	"xchg %rcx, regs+0x8\n\t"		\
	"xchg %rdx, regs+0x10\n\t"		\
	"xchg %rbx, regs+0x18\n\t"		\
	"xchg %rbp, regs+0x28\n\t"		\
	"xchg %rsi, regs+0x30\n\t"		\
	"xchg %rdi, regs+0x38\n\t"		\
	"xchg %r8, regs+0x40\n\t"		\
	"xchg %r9, regs+0x48\n\t"		\
	"xchg %r10, regs+0x50\n\t"		\
	"xchg %r11, regs+0x58\n\t"		\
	"xchg %r12, regs+0x60\n\t"		\
	"xchg %r13, regs+0x68\n\t"		\
	"xchg %r14, regs+0x70\n\t"		\
	"xchg %r15, regs+0x78\n\t"

#define LOAD_GPR	SAVE_GPR

#define SAVE_GPR_C				\
	"xchg %%rax, regs\n\t"			\
	"xchg %%rcx, regs+0x8\n\t"		\
	"xchg %%rdx, regs+0x10\n\t"		\
	"xchg %%rbx, regs+0x18\n\t"		\
	"xchg %%rbp, regs+0x28\n\t"		\
	"xchg %%rsi, regs+0x30\n\t"		\
	"xchg %%rdi, regs+0x38\n\t"		\
	"xchg %%r8, regs+0x40\n\t"		\
	"xchg %%r9, regs+0x48\n\t"		\
	"xchg %%r10, regs+0x50\n\t"		\
	"xchg %%r11, regs+0x58\n\t"		\
	"xchg %%r12, regs+0x60\n\t"		\
	"xchg %%r13, regs+0x68\n\t"		\
	"xchg %%r14, regs+0x70\n\t"		\
	"xchg %%r15, regs+0x78\n\t"

#define LOAD_GPR_C	SAVE_GPR_C

#define VMX_IO_SIZE_MASK	0x7
#define _VMX_IO_BYTE		0
#define _VMX_IO_WORD		1
#define _VMX_IO_LONG		3
#define VMX_IO_DIRECTION_MASK	(1ul << 3)
#define VMX_IO_IN		(1ul << 3)
#define VMX_IO_OUT		0
#define VMX_IO_STRING		(1ul << 4)
#define VMX_IO_REP		(1ul << 5)
#define VMX_IO_OPRAND_IMM	(1ul << 6)
#define VMX_IO_PORT_MASK	0xFFFF0000
#define VMX_IO_PORT_SHIFT	16

#define VMX_TEST_START		0
#define VMX_TEST_VMEXIT		1
#define VMX_TEST_EXIT		2
#define VMX_TEST_RESUME		3
#define VMX_TEST_VMABORT	4
#define VMX_TEST_VMSKIP		5

#define HYPERCALL_BIT		(1ul << 12)
#define HYPERCALL_MASK		0xFFF
#define HYPERCALL_VMEXIT	0x1
#define HYPERCALL_VMABORT	0x2
#define HYPERCALL_VMSKIP	0x3

#define EPTP_PG_WALK_LEN_SHIFT	3ul
#define EPTP_AD_FLAG		(1ul << 6)

#define EPT_MEM_TYPE_UC		0ul
#define EPT_MEM_TYPE_WC		1ul
#define EPT_MEM_TYPE_WT		4ul
#define EPT_MEM_TYPE_WP		5ul
#define EPT_MEM_TYPE_WB		6ul

#define EPT_RA			(1ul << 0)
#define EPT_WA			(1ul << 1)
#define EPT_EA			(1ul << 2)
#define EPT_PRESENT		(EPT_RA | EPT_WA | EPT_EA)
#define EPT_ACCESS_FLAG		(1ul << 8)
#define EPT_DIRTY_FLAG		(1ul << 9)
#define EPT_LARGE_PAGE		(1ul << 7)
#define EPT_EXE_FOR_USER_MODE (1ul << 10)
#define EPT_MEM_TYPE_SHIFT	3ul
#define EPT_IGNORE_PAT		(1ul << 6)
#define EPT_SUPPRESS_VE		(1ull << 63)

#define EPT_CAP_WT		1ull
#define EPT_CAP_PWL4		(1ull << 6)
#define EPT_CAP_UC		(1ull << 8)
#define EPT_CAP_WB		(1ull << 14)
#define EPT_CAP_2M_PAGE		(1ull << 16)
#define EPT_CAP_1G_PAGE		(1ull << 17)
#define EPT_CAP_INVEPT		(1ull << 20)
#define EPT_CAP_INVEPT_SINGLE	(1ull << 25)
#define EPT_CAP_INVEPT_ALL	(1ull << 26)
#define EPT_CAP_AD_FLAG		(1ull << 21)
#define VPID_CAP_INVVPID	(1ull << 32)
#define VPID_CAP_INVVPID_ADDR   (1ull << 40)
#define VPID_CAP_INVVPID_CXTGLB (1ull << 41)
#define VPID_CAP_INVVPID_ALL    (1ull << 42)
#define VPID_CAP_INVVPID_CXTLOC	(1ull << 43)

#define PAGE_SIZE_2M		(512 * PAGE_SIZE)
#define PAGE_SIZE_1G		(512 * PAGE_SIZE_2M)
#define EPT_PAGE_LEVEL		4
#define EPT_PGDIR_WIDTH		9
#define EPT_PGDIR_MASK		511
#define EPT_PGDIR_ENTRIES	(1 << EPT_PGDIR_WIDTH)
#define EPT_LEVEL_SHIFT(level)	(((level)-1) * EPT_PGDIR_WIDTH + 12)
#define EPT_ADDR_MASK		GENMASK_ULL(51, 12)
#define PAGE_MASK_2M		(~(PAGE_SIZE_2M-1))

#define EPT_VLT_RD		1
#define EPT_VLT_WR		(1 << 1)
#define EPT_VLT_FETCH		(1 << 2)
#define EPT_VLT_PERM_RD		(1 << 3)
#define EPT_VLT_PERM_WR		(1 << 4)
#define EPT_VLT_PERM_EX		(1 << 5)
#define EPT_VLT_PERMS		(EPT_VLT_PERM_RD | EPT_VLT_PERM_WR | \
				 EPT_VLT_PERM_EX)
#define EPT_VLT_LADDR_VLD	(1 << 7)
#define EPT_VLT_PADDR		(1 << 8)

#define MAGIC_VAL_1		0x12345678ul
#define MAGIC_VAL_2		0x87654321ul
#define MAGIC_VAL_3		0xfffffffful
#define MAGIC_VAL_4		0xdeadbeeful

#define INVEPT_SINGLE		1
#define INVEPT_GLOBAL		2

#define INVVPID_ADDR            0
#define INVVPID_CONTEXT_GLOBAL	1
#define INVVPID_ALL		2
#define INVVPID_CONTEXT_LOCAL	3

#define ACTV_ACTIVE		0
#define ACTV_HLT		1

/*
 * VMCS field encoding:
 * Bit 0: High-access
 * Bits 1-9: Index
 * Bits 10-12: Type
 * Bits 13-15: Width
 * Bits 15-64: Reserved
 */
#define VMCS_FIELD_HIGH_SHIFT		(0)
#define VMCS_FIELD_INDEX_SHIFT		(1)
#define VMCS_FIELD_TYPE_SHIFT		(10)
#define VMCS_FIELD_WIDTH_SHIFT		(13)
#define VMCS_FIELD_RESERVED_SHIFT	(15)
#define VMCS_FIELD_BIT_SIZE		(BITS_PER_LONG)

extern struct regs regs;

extern union vmx_basic basic;
extern union vmx_ctrl_msr ctrl_pin_rev;
extern union vmx_ctrl_msr ctrl_cpu_rev[2];
extern union vmx_ctrl_msr ctrl_exit_rev;
extern union vmx_ctrl_msr ctrl_enter_rev;
extern union vmx_ept_vpid  ept_vpid;

extern u64 *vmxon_region;

void vmx_set_test_stage(u32 s);
u32 vmx_get_test_stage(void);
void vmx_inc_test_stage(void);

static int vmx_on(void)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;
	asm volatile ("push %1; popf; vmxon %2; setbe %0\n\t"
		      : "=q" (ret) : "q" (rflags), "m" (vmxon_region) : "cc");
	return ret;
}

static int vmx_off(void)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	asm volatile("push %1; popf; vmxoff; setbe %0\n\t"
		     : "=q"(ret) : "q" (rflags) : "cc");
	return ret;
}

static inline int make_vmcs_current(struct vmcs *vmcs)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	asm volatile ("push %1; popf; vmptrld %2; setbe %0"
		      : "=q" (ret) : "q" (rflags), "m" (vmcs) : "cc");
	return ret;
}

static inline int vmcs_clear(struct vmcs *vmcs)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	asm volatile ("push %1; popf; vmclear %2; setbe %0"
		      : "=q" (ret) : "q" (rflags), "m" (vmcs) : "cc");
	return ret;
}

static inline u64 vmcs_read(enum Encoding enc)
{
	u64 val;
	asm volatile ("vmread %1, %0" : "=rm" (val) : "r" ((u64)enc) : "cc");
	return val;
}

static inline int vmcs_read_checking(enum Encoding enc, u64 *value)
{
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;
	u64 encoding = enc;
	u64 val;

	asm volatile ("shl $8, %%rax;"
		      "sahf;"
		      "vmread %[encoding], %[val];"
		      "lahf;"
		      "shr $8, %%rax"
		      : /* output */ [val]"=rm"(val), "+a"(rflags)
		      : /* input */ [encoding]"r"(encoding)
		      : /* clobber */ "cc");

	*value = val;
	return rflags & (X86_EFLAGS_CF | X86_EFLAGS_ZF);
}

static inline int vmcs_write(enum Encoding enc, u64 val)
{
	bool ret;
	asm volatile ("vmwrite %1, %2; setbe %0"
		: "=q"(ret) : "rm" (val), "r" ((u64)enc) : "cc");
	return ret;
}

static inline u32 exec_vmread32(u32 field)
{
	return (u32)vmcs_read(field);
}

static inline void exec_vmwrite32(u32 field, u32 value)
{
	vmcs_write(field, (u64)value);
}

static inline int vmcs_set_bits(enum Encoding enc, u64 val)
{
	return vmcs_write(enc, vmcs_read(enc) | val);
}

static inline int vmcs_clear_bits(enum Encoding enc, u64 val)
{
	return vmcs_write(enc, vmcs_read(enc) & ~val);
}

static inline int vmcs_save(struct vmcs **vmcs)
{
	bool ret;
	unsigned long pa;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	asm volatile ("push %2; popf; vmptrst %1; setbe %0"
		      : "=q" (ret), "=m" (pa) : "r" (rflags) : "cc");
	*vmcs = (pa == -1ull) ? NULL : phys_to_virt(pa);
	return ret;
}

static inline bool invept(unsigned long type, u64 eptp)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	struct {
		u64 eptp, gpa;
	} operand = {eptp, 0};
	asm volatile("push %1; popf; invept %2, %3; setbe %0"
		: "=q" (ret)
		: "r" (rflags), "m"(operand), "r"(type)
		: "cc");
	return ret;
}

static inline bool invvpid(unsigned long type, u64 vpid, u64 gla)
{
	bool ret;
	u64 rflags = read_rflags() | X86_EFLAGS_CF | X86_EFLAGS_ZF;

	struct invvpid_operand operand = {vpid, gla};
	asm volatile("push %1; popf; invvpid %2, %3; setbe %0"
		: "=q" (ret)
		: "r" (rflags), "m"(operand), "r"(type)
		: "cc");
	return ret;
}

static inline int enable_unrestricted_guest(void)
{
	if (!(ctrl_cpu_rev[0].clr & CPU_SECONDARY)) {
		return -1;
	}
	if (!(ctrl_cpu_rev[1].clr & CPU_URG)) {
		return -1;
	}
	vmcs_write(CPU_EXEC_CTRL0, vmcs_read(CPU_EXEC_CTRL0) | CPU_SECONDARY);
	vmcs_write(CPU_EXEC_CTRL1, vmcs_read(CPU_EXEC_CTRL1) | CPU_URG);
	return 0;
}

static inline void disable_unrestricted_guest(void)
{
	vmcs_write(CPU_EXEC_CTRL1, vmcs_read(CPU_EXEC_CTRL1) & ~CPU_URG);
}

const char *exit_reason_description(u64 reason);
void print_vmexit_info(void);
void print_vmentry_failure_info(struct vmentry_failure *failure);
void ept_sync(int type, u64 eptp);
void vpid_sync(int type, u16 vpid);
void install_ept_entry(unsigned long *pml4, int pte_level,
		unsigned long guest_addr, unsigned long pte,
		unsigned long *pt_page);
void install_1g_ept(unsigned long *pml4, unsigned long phys,
		unsigned long guest_addr, u64 perm);
void install_2m_ept(unsigned long *pml4, unsigned long phys,
		unsigned long guest_addr, u64 perm);
void install_ept(unsigned long *pml4, unsigned long phys,
		unsigned long guest_addr, u64 perm);
void setup_ept_range(unsigned long *pml4, unsigned long start,
		     unsigned long len, u64 perm);
bool get_ept_pte(unsigned long *pml4, unsigned long guest_addr, int level,
		unsigned long *pte);
void set_ept_pte(unsigned long *pml4, unsigned long guest_addr,
		int level, u64 pte_val);
void check_ept_ad(unsigned long *pml4, u64 guest_cr3,
		  unsigned long guest_addr, int expected_gpa_ad,
		  int expected_pt_ad);
void clear_ept_ad(unsigned long *pml4, u64 guest_cr3,
		  unsigned long guest_addr);

bool ept_2m_supported(void);
bool ept_1g_supported(void);
bool ept_huge_pages_supported(int level);
bool ept_execute_only_supported(void);
bool ept_ad_bits_supported(void);

void enter_guest(void);

typedef void (*test_guest_func)(void);
typedef void (*test_teardown_func)(void *data);
void test_set_guest(test_guest_func func);
void test_add_teardown(test_teardown_func func, void *data);
void test_skip(const char *msg);

void __abort_test(void);

#define TEST_ASSERT(cond) \
do { \
	if (!(cond)) { \
		report("%s:%d: Assertion failed: %s", 0, \
		       __FILE__, __LINE__, #cond); \
		dump_stack(); \
		__abort_test(); \
	} \
	report_pass(); \
} while (0)

#define TEST_ASSERT_MSG(cond, fmt, args...) \
do { \
	if (!(cond)) { \
		report("%s:%d: Assertion failed: %s\n" fmt, 0, \
		       __FILE__, __LINE__, #cond, ##args); \
		dump_stack(); \
		__abort_test(); \
	} \
	report_pass(); \
} while (0)

#define __TEST_EQ(a, b, a_str, b_str, assertion, fmt, args...) \
do { \
	typeof(a) _a = a; \
	typeof(b) _b = b; \
	if (_a != _b) { \
		char _bin_a[BINSTR_SZ]; \
		char _bin_b[BINSTR_SZ]; \
		binstr(_a, _bin_a); \
		binstr(_b, _bin_b); \
		report("%s:%d: %s failed: (%s) == (%s)\n" \
		       "\tLHS: %#018lx - %s - %lu\n" \
		       "\tRHS: %#018lx - %s - %lu%s" fmt, 0, \
		       __FILE__, __LINE__, \
		       assertion ? "Assertion" : "Expectation", a_str, b_str, \
		       (unsigned long) _a, _bin_a, (unsigned long) _a, \
		       (unsigned long) _b, _bin_b, (unsigned long) _b, \
		       fmt[0] == '\0' ? "" : "\n", ## args); \
		dump_stack(); \
		if (assertion) \
			__abort_test(); \
	} \
	report_pass(); \
} while (0)

#define TEST_ASSERT_EQ(a, b) __TEST_EQ(a, b, #a, #b, 1, "")
#define TEST_ASSERT_EQ_MSG(a, b, fmt, args...) \
	__TEST_EQ(a, b, #a, #b, 1, fmt, ## args)
#define TEST_EXPECT_EQ(a, b) __TEST_EQ(a, b, #a, #b, 0, "")
#define TEST_EXPECT_EQ_MSG(a, b, fmt, args...) \
	__TEST_EQ(a, b, #a, #b, 0, fmt, ## args)

/* common function */
extern struct st_vcpu *get_bp_vcpu(void);
extern u64 vcpu_get_gpreg(u8 idx);
extern void vmx_set_test_condition(u32 s);
extern u32 vmx_get_test_condition(void);
extern void set_exit_reason(u32 exit_reason);
extern u32 get_exit_reason(void);
extern void exec_vmwrite32_bit(u32 field, u32 bitmap, u32 is_bit_set);
extern void set_vmcs(u32 condition);
void set_io_bitmap(uint8_t *io_bitmap, uint32_t port_io, uint32_t is_bit_set, uint32_t bytes);
void set_msr_bitmap(uint8_t *msr_bitmap, uint32_t msr,
	uint32_t is_bit_set, uint8_t is_rd_wt);
static void msr_passthroudh();
__unused static void delay_tsc(u64 count);

void ept_gpa2hpa_map(struct st_vcpu *vcpu, u64 hpa, u64 gpa,
		u32 size, u64 permission, bool is_clear);
int vm_exec_init(__unused struct vmcs *vmcs);

static inline u64 hva2hpa(const void *hva)
{
	return (u64)hva;
}

static inline void vmcall(void)
{
	asm volatile("vmcall");
}

__unused static inline uint64_t hsi_vm_exit_qualification_bit_mask(uint64_t exit_qual, uint32_t msb, uint32_t lsb)
{
	return (exit_qual & (((1UL << (msb + 1U)) - 1UL) - ((1UL << lsb) - 1UL)));
}

/* access Control-Register Info using exit qualification field */
__unused static inline uint64_t hsi_vm_exit_cr_access_cr_num(uint64_t exit_qual)
{
	return hsi_vm_exit_qualification_bit_mask(exit_qual, 3U, 0U);
}

__unused static inline uint64_t hsi_vm_exit_cr_access_reg_idx(uint64_t exit_qual)
{
	return (hsi_vm_exit_qualification_bit_mask(exit_qual, 11U, 8U) >> 8U);
}

__unused static inline uint64_t hsi_vm_exit_cr_access_type(uint64_t exit_qual)
{
	return hsi_vm_exit_qualification_bit_mask(exit_qual, 5U, 4U);
}

__unused static inline uint64_t hsi_vm_exit_io_access_port_num(uint64_t exit_qual)
{
	return ((hsi_vm_exit_qualification_bit_mask(exit_qual, 31, 16) >> 16) & 0xffffULL);
}

/* common data structe */
extern st_vm_exit_info vm_exit_info;
#endif

