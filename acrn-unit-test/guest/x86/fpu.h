#ifndef _FPU_H_
#define _FPU_H_

#include "apic-defs.h"

#define MSR_EFER_ME		(1ULL << 8)
#define EDX_PAT			(1ULL << 16)

#define CR0_BIT_PE					0
#define CR0_BIT_MP					1
#define CR0_BIT_EM					2
#define CR0_BIT_TS					3
#define CR4_BIT_OSFXSR				9


#define INIT_BASE_REG_ADDR				0x6000UL
#define STARTUP_BASE_REG_ADDR			0x7000UL
#define UNCHANGED_BASE_REG_ADDR			0x8000UL

#define STARTUP_CR0_REG_ADDR			STARTUP_BASE_REG_ADDR
#define STARTUP_FPU_CWS_ADDR            0x7100UL
#define STARTUP_FPU_FSAVE_ADDR          0x7200UL
#define STARTUP_FPU_XSAVE_ADDR			0x7300UL

#define INIT_CR0_REG_ADDR				INIT_BASE_REG_ADDR
#define INIT_FPU_SWS_ADDR               0x6100UL
#define INIT_FPU_CWS_ADDR				0x6200UL

#define INIT_FPU_SAVE_ADDR				0x6500UL
#define INIT_UNCHANGED_FPU_SAVE_ADDR	0x8500UL
#define INIT_FPU_FSAVE_ADDR				0x9000UL

#define INVALID_ADDR_OUT_PHY			(3UL << 30)

#endif
