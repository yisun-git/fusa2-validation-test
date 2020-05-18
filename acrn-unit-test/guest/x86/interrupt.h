/*segmentation.h*/

#ifndef _INTERRUPT_H_

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

/* GDT description info */
#define SEGMENT_LIMIT                                           0xFFFF
#define SEGMENT_LIMIT2                                          0xF
#define SEGMENT_LIMIT_ALL										0xFFFFF

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
#define SEGMENT_TYPE_CODE_EXE_READ                              0xA
#define SEGMENT_TYPE_CODE_EXE_READ_ACCESSED                     0xB
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
#define GRANULARITY_CLEAR                                       0x00
#define DEFAULT_OPERATION_SIZE_16BIT_SEGMENT                    0x00
#define DEFAULT_OPERATION_SIZE_32BIT_SEGMENT                    0x40
#define L_64_BIT_CODE_SEGMENT                                   0x20
#define AVL0                                                    0x0

/* ring1 */
#define OSSERVISE1_CS32                 0x59
#define OSSERVISE1_DS                   0x61

#endif
