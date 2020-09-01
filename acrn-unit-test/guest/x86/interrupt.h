/*interrupt.h*/
#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#define IDT_BEYOND_LIMIT_INDEX	0xf0
#define EXTEND_INTERRUPT_E0				0xe0
#define EXTEND_INTERRUPT_80				0x80

#define ERROR_CODE_IDT_FLAG				(1<<1)
#define NMI_HANDLER_DELAY	2
#define NMI_BP_DELAY		1

#define CR0_EM_BIT						(1<<2)
#define CR0_NE_BIT						(1<<5)
#define CR0_AM_BIT						(1<<18)
#define CR4_RESERVED_BIT23				(1<<23)
#define RFLAG_TF_BIT					(1<<8)
#define RFLAG_OF_BIT					(1<<11)
#define RFLAG_NT_BIT					(1<<14)
#define RFLAG_RF_BIT					(1<<16)
#define RFLAG_VM_BIT					(1<<17)
#define RFLAG_AC_BIT					(1<<18)

#define STARTUP_IDTR_ADDR	0x6000

#define INIT_IDTR_ADDR		0x7000

#define APIC_ID_BSP		0

#endif
