/*segmentation.h*/

#ifndef _SEGMENTATION_H_

#define _SEGMENTATION_H_

#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "alloc.h"
#include "apic-defs.h"
#include "apic.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "isr.h"
#include "atomic.h"
#include "types.h"
#include "misc.h"

#define MAX_CPUS        4
#define APICID_CPU0     0
#define APICID_CPU1     1
#define APICID_CPU2     2
#define APICID_CPU3     3

#define IPI_VECTOR      0x20
#define TICK_VECTOR     0xee
#define TICK_APINIT     0xF1

#define EOK             0
#define ETIMEDOUT       -1
#define ERANGE          -2
#define ENOMEM          -3
#define EFAULT          -4

/* GDT description info */
#define SEGMENT_LIMIT                                           0xFFFF
#define SEGMENT_LIMIT2                                          0xF

#define SEGMENT_PRESENT_SET                                     0x80
#define SEGMENT_PRESENT_CLEAR                                   0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_0                            0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_1                            0x20
#define DESCRIPTOR_PRIVILEGE_LEVEL_2                            0x40
#define DESCRIPTOR_PRIVILEGE_LEVEL_3                            0x60
#define DESCRIPTOR_TYPE_SYS                                     0x00
#define DESCRIPTOR_TYPE_CODE_OR_DATA                            0x10

#define SEGMENT_TYPE_DATE_READ_ONLY                             0x0
#define SEGMENT_TYPE_DATE_READ_ONLY_ACCESSED                    0x1
#define SEGMENT_TYPE_DATE_READ_WRITE                            0x2
#define SEGMENT_TYPE_DATE_READ_WRITE_ACCESSED                   0x3
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN                    0x4
#define SEGMENT_TYPE_DATE_READ_ONLY_EXP_DOWN_ACCESSED           0x5
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN                   0x6
#define SEGMENT_TYPE_DATE_READ_WRITE_EXP_DOWN_ACCESSED          0x7

#define SEGMENT_TYPE_CODE_EXE_ONLY                              0x8
#define SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED                     0x9
#define SEGMENT_TYPE_CODE_EXE_RAED                              0xA
#define SEGMENT_TYPE_CODE_EXE_RAED_ACCESSED                     0xB
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING                   0xC
#define SEGMENT_TYPE_CODE_EXE_ONLY_CONFORMING_ACCESSED          0xD
#define SEGMENT_TYPE_CODE_EXE_READ_CONFORMING                   0xE
#define SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED     0xF

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_LDT               0x2
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TASKGATE          0x5
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_CALLGATE          0x0C
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TSSG              0xB

#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE         0xE
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE          0xF

#define GRANULARITY_SET                                         0x80
#define GRANULARITY_clear                                       0x00
#define DEFAULT_OPERATION_SIZE_16BIT_SEGMENT                    0x00
#define DEFAULT_OPERATION_SIZE_32BIT_SEGMENT                    0x40
#define L_64_BIT_CODE_SEGMENT                                   0x20
#define AVL0                                                    0x0

#define DPLEVEL1                        0x20
#define DPLEVEL2                        0x40

#define SELECTOR_TI                     0x4

/* ring1 */
#define OSSERVISE1_CS32                 0x59
#define OSSERVISE1_DS                   0x61

/* ring2 */
#define OSSERVISE2_CS32                 0x6A
#define OSSERVISE2_DS                   0x72

#define	BASE1                           0x0
#define	BASE2                           0x0
#define	BASE3                           0x0
#define	BASE4                           0x0

#define GRANULARITY                     0x1

#define SEGMENT_PRESENT                 0x1
#define DESCRIPTOR_TYPE_SYS_SEGMENT     0x1
#define DESCRIPTOR_TYPE_CODE_DATE_SEG   0x0
#define BIT64_CODE_SEGMENT              0x1
#define DEFAULT_OPERtion_16_BIT_SEG     0
#define DEFAULT_OPERtion_32_BIT_SEG     1
#define STACK_CPY_NUMBER                0x0

#define UNUSED                          0
#define IST_TABLE_INDEX                 0

#define INTER_OFF_HEANDLER

#define __sum_off__

#define TICKS

#define asm_trigger_ud(void)    asm volatile("ud2\n");

ulong g_ap_stack[4096] = { 0 };

extern gdt_entry_t boot_ldt[50];

atomic64_t g_p1_found;
unsigned int g_p1_id = 0;
static volatile unsigned long g_llvt_count = 0;
static volatile unsigned long g_main_ticks = 0;
static volatile unsigned long g_test_ticks = 0;

typedef struct
{
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
} irq_stack_t, ipi_stack_t;

typedef void cpu_routine_t(void);

typedef enum _SEGMENT_REGISTER
{
	REG_CS = 0,
	REG_DS,
	REG_ES,
	REG_FS,
	REG_GS,
	REG_SS
} SEG_REG;

typedef enum _SEGMENT_REGISTER_BIT
{
	BIT_L = 0,
	BIT_B,
	BIT_D
} SEG_REG_BIT;

#ifndef __x86_64__
struct gate_descriptor {
	unsigned gd_looffset:16;	/* gate offset (lsb) */
	unsigned gd_selector:16;	/* gate segment selector */
	unsigned gd_stkcpy:5;		/* number of stack wds to cpy */
	unsigned gd_xx:3;			/* unused */
	unsigned gd_type:5;			/* segment type */
	unsigned gd_dpl:2;			/* segment descriptor priority level */
	unsigned gd_p:1;			/* segment descriptor present */
	unsigned gd_hioffset:16;	/* gate offset (msb) */
} __attribute__((packed));
#else //__x86_64__
struct gate_descriptor {
	uint64_t gd_looffset:16;	/* gate offset (lsb) */
	uint64_t gd_selector:16;	/* gate segment selector */
	uint64_t gd_ist:3;			/* IST table index */
	uint64_t gd_xx:5;			/* unused */
	uint64_t gd_type:5;			/* segment type */
	uint64_t gd_dpl:2;			/* segment descriptor priority level */
	uint64_t gd_p:1;			/* segment descriptor present */
	uint64_t gd_hioffset:48;	/* gate offset (msb) */
	uint64_t sd_xx1:32;
} __attribute__((packed));
#endif

struct task_gate {
	u16 resv_0;
	u16 selector;
	u8 resv_1 :8;
	u8 type :4;
	u8 system :1;
	u8 dpl :2;
	u8 p :1;
	u16 resv_2;
} gate;

#define VALUE_DEADBEEF              0xdeadbeef
#define MEMORY_ADDR_ZERO            0x00000000
#define SELECT_OFFSET_64M           0x400

#define CS_SS_GP
//#define TASK_SEGMENT_SS_CLEAN_P_145140 /*This must be run alone*/
#define SEGMENT_CLEAR_PBIT_DS_145139
//#define SEGMENT_DSCRIPT_ANY_BIT_ILLAEGAL_145135 /*Requirement has been deleted*/
#define ACCESSES_NONWRITABLE_DATA_SEGMENT_151141
#define ANY_SET_VAL_145148
#define SET_B_L_D_BIT_145168
//#define SEGMENT_RPL_GREATER_THAN_DPL_145238
//#define NULL_SEGMENT_SELECTOR_LDT_145968    /*Requirement has been deleted*/

#define READ_SEGMENT_INIT
#define READ_SEGMENT_HIDEN_PART
#define READ_SEGMENT_HIDEN_PART_LIMIT

#define READ_CR4_FSGSBASE_VALUE_139281
#define READ_GDTLVALUE_139279
#define READ_LDTVALUE_139277
#define READ_I32_FS_BASE_139275
#define READ_I32_GS_BASE_139273
#define READ_I32_KERNEL_GS_BASE_139272
#define IDT_ENTRY_BEYAND_SEGMENT_LIMIT_145134
#define DATA_SEGMENT_AND_S_TYPE_NON_DATA_SEGMENT_145142
#define DPL_OF_THE_CODE_SEGMENT_GENERATES_145146
#define EXCEPTION_HAPPENED_IN_IDT_145149
#define ACCESSES_CODE_DATA_SEGMENT_AND_EXCEPTION_HAPPENED_LDT_145141
#define SEGMENT_DESCRIPTOR_IS_MISSING_IN_MEMORY_145152
#define BASE_ADDRESS_IN_THE_SEGMENT_DESCRIPTOR_IS_UNALLOWED_145153
#define ACCESSES_SEGMENT_AND_SELECTOR_INDEX_BEYONDS_GDT_145159
#define DPL_OF_NONCONFORMING_CODE_SEGMENT_GENERATES_145165
#define NULL_SEGMENT_SELECTOR_GENERATES_147031
#define OPERAND_EFFECTIVE_ADDRESS_BEYONDS_STACK_SEGMENT_LIMIT_145158
#define ACCESSES_A_NONCONFORMING_CODE_SEGMENT_USING_A_CALL_GATE_145166
#define ACCESSES_NONCONFORMING_CODE_SEGMENT_USING_CALL_GATE_WITH_JMP_INSTRUCTION_145955
#define OFFSET_THAN_THE_LDT_DESCRIPTOR_LIMIT_GENERATES_146601
#define DPL_IN_THE_DATA_SEGMENT_DESCRIPTOR_GENERATES_146602
#define S_TYPE_BITS_NO_LDT_DESCRIPTOR_146600
#define READ_FROM_AN_EXCUTE_ONLY_CODE_SEGMENT_146122
#define WRITE_TO_CODE_SEGMENT_GENERATES_146121
#define ERROR_CODE_PRESENTATION_146064
#define ACCESSING_STACK_SEGMENT_CPL_RPL_DPL_145972
#define LOAD_LDT_P_BIT_LDT_DESCRIPTOR_CLEAR_145971
#define SEGMENT_SELECTOR_INDEX_LOADING_LDT_THAN_DGT_145970
#define TI_BIT_IN_SEGMENT_SELECTOR_GENERATES_145969
#define CPL_THAN_ZERO_SWHEN_LOADING_LDT_145966
#define ACCESSES_CONFORMING_CODE_SEGMENT_WITHOUT_CALLGATE_145957
#define DATA_SEGMENT_MEMORY_OPERAND_EFFECTIV_ADDRESS_OUTSIDE_SEGMENT_LIMIT_145157
#define EXPOSE_SEGMENTATION_IN_PROTECTED_MODE_164604
#define LOADING_LDT_MISS_BASE_ADDRESS_GENERATES_145964
#define EXPOSE_SEGMENTATION_DATA_STRUCTURES_146605
#define WHEN_ANY_VCPU_LOAD_LDT_AND_ACCESS_RIGHT_OF_BASE_ADDRESS_145965
#define ACCESSES_CODE_SEGMENT_WITH_CALL_GATE_NON_CODE_SEGMENT_146603
#define MEMORY_ADDRESS_ACCESSED_NON_CANONICAL_147011
#define NON_ZERO_UPPER_TYPE_IN_CALL_GATE_DESCRIPTOR_GENERATES_146860
#define D_AND_L_BITS_IN_SEGMENT_DESCRIPTOR_GENERATE_146849
#define EXPOSE_FS_GS_MSRS_146120

#endif
