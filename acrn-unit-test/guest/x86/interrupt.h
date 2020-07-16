/*interrupt.h*/
#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_
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

#define EXTEND_INTERRUPT_E0				0xe0
#define CR0_EM_BIT						(1<<2)
#define CR0_NE_BIT						(1<<5)
#define CR0_AM_BIT						(1<<18)
#define CR4_RESERVED_BIT23				(1<<23)
#define RFLAG_TF_BIT					(1<<8)
#define RFLAG_NT_BIT					(1<<14)
#define RFLAG_RF_BIT					(1<<16)
#define RFLAG_VM_BIT					(1<<17)
#define RFLAG_AC_BIT					(1<<18)
#endif
