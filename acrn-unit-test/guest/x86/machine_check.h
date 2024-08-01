#ifndef __MACHINE_CHECK_H__
#define __MACHINE_CHECK_H__
/*
 * Machine Check support for x86
 */
#include "msr.h"
#define INIT_BASE_REG_ADDR				0x6000ULL
#define STARTUP_BASE_REG_ADDR			0x7000ULL
#define UNCHANGED_BASE_REG_ADDR			0x8000ULL

#define INIT_IA32_FEATURE_CONTROL		0x6004ULL
#define INIT_IA32_MCG_CAP_LOW_ADDR		0x600CULL
#define INIT_IA32_MCG_CAP_HIGH_ADDR		0x6010ULL
#define INIT_IA32_MCG_STATUS_LOW_ADDR	0x6014ULL
#define INIT_IA32_MCG_STATUS_HIGH_ADDR	0x6018ULL
#define INIT_IA32_P5_MC_TYPE_LOW_ADDR	0x601CULL
#define INIT_IA32_P5_MC_TYPE_HIGH_ADDR	0x6020ULL
#define INIT_IA32_P5_MC_ADDR_LOW_ADDR	0x6024ULL
#define INIT_IA32_P5_MC_ADDR_HIGH_ADDR	0x6028ULL
#define INIT_CR4_REG_ADDR	            0x602CULL

#define STARTUP_IA32_FEATURE_CONTROL	0x7004ULL
#define STARTUP_IA32_MCG_CAP	        0x700CULL
#define STARTUP_IA32_MCG_STATUS	        0x7014ULL
#define STARTUP_IA32_P5_MC_TYPE_LOW_ADDR	        0x701CULL
#define STARTUP_IA32_P5_MC_TYPE_HIGH_ADDR	        0x7020ULL
#define STARTUP_IA32_P5_MC_ADDR_LOW_ADDR	        0x7024ULL
#define STARTUP_IA32_P5_MC_ADDR_HIGH_ADDR	        0x7028ULL
#define STARTUP_CR4_REG_ADDR	        0x702CULL
#define STARTUP_IA32_MC0_STATUS_ADDR	0x7034ULL
#define STARTUP_IA32_MC9_STATUS_ADDR	0x707CULL
#define STARTUP_IA32_MC0_CTL2_ADDR	    0x7084ULL
#define STARTUP_IA32_MC3_CTL2_ADDR	    0x709CULL


/* MCG_CAP register defines */
#define MCG_BANKCNT_MASK	0xFFULL         /* Number of Banks */
#define MCG_CTL_P		(1ULL << 8)   /* MCG_CTL register available */
#define MCG_EXT_P		(1ULL << 9)   /* Extended registers available */
#define MCG_CMCI_P		(1ULL << 10)  /* CMCI supported */
#define MCG_TES_P               (1ULL << 11)  /* Software error recovery support present */
#define MCG_EXT_CNT_MASK	0xFF0000ULL     /* Number of Extended registers */
#define MCG_EXT_CNT_SHIFT	16ULL
#define MCG_EXT_CNT(c)		(((c) & MCG_EXT_CNT_MASK) >> MCG_EXT_CNT_SHIFT)
#define MCG_SER_P		(1ULL << 24)  /* MCA recovery/new status bits */
#define MCG_ELOG_P		(1ULL << 26)  /* Extended error log supported */
#define MCG_LMCE_P		(1ULL << 27)  /* Local machine check supported */

/* MCG_STATUS register defines */
#define MCG_STATUS_RIPV		(1ULL << 0)   /* restart ip valid */
#define MCG_STATUS_EIPV		(1ULL << 1)   /* ip points to correct instruction */
#define MCG_STATUS_MCIP		(1ULL << 2)   /* machine check in progress */
#define MCG_STATUS_LMCES	(1ULL << 3)   /* LMCE signaled */

/* MCG_EXT_CTL register defines */
#define MCG_EXT_CTL_LMCE_EN	(1ULL << 0) /* Enable LMCE */

/* MCi_STATUS register defines */
#define MCI_STATUS_VAL		(1ULL << 63)  /* valid error */
#define MCI_STATUS_OVER		(1ULL << 62)  /* previous errors lost */
#define MCI_STATUS_UC		(1ULL << 61)  /* uncorrected error */
#define MCI_STATUS_EN		(1ULL << 60)  /* error enabled */
#define MCI_STATUS_MISCV	(1ULL << 59)  /* misc error reg. valid */
#define MCI_STATUS_ADDRV	(1ULL << 58)  /* addr reg. valid */
#define MCI_STATUS_PCC		(1ULL << 57)  /* processor context corrupt */
#define MCI_STATUS_S		(1ULL << 56)  /* Signaled machine check */
#define MCI_STATUS_AR		(1ULL << 55)  /* Action required */

#define MSR_IA32_MCG_EXT_CTL	0x000004D0ULL

#define CPUID_1_EDX_MCE (1ULL << 7)
#define CPUID_1_EDX_MCA (1ULL << 14)
#define IA32_MCG_CAP_CTL_P		(1ULL << 8)
#define IA32_MCG_CAP_CMCI_P		(1ULL << 10)
#define IA32_MCG_CAP_LMCE_P		(1ULL << 27)

#define IA32_FEATURE_CONTROL_LOCK	(1ULL << 0)
#define IA32_FEATURE_CONTROL_LMCE_ON	(1ULL << 20)
#define CR4_MCE		(1ULL << 6) //(1UL<<6) 64, high 32 reserved

#define MAX_ERROR_REPORTING_BANK_NUM		(29U)
#define MAX_MCI_CTL2_BANK_NUM			(32U)
#define MAX_VALID_MCI_CTL2_BANK_NUM     (4U)

#endif
