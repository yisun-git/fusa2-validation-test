#ifndef __XSAVE_H__
#define __XSAVE_H__
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
#include "xsave_asm.h"

#define X86_CR0_EM				(1 << 2)
//#define X86_CR0_TS				(1 << 3)

#define X86_CR4_OSXSAVE				(1 << 18)
#define X86_CR4_OSFXSR				(1 << 9)
#define X86_CR4_OSXMMEXCPT			(1 << 10)

#define CPUID_XSAVE_FUC		0xd

//Processor Extended State Enumeration Sub-leaf (EAX = 0DH, ECX = 1)
#define SUPPORT_XSAVEOPT_BIT		0
#define SUPPORT_XSAVEC_BIT		1
#define SUPPORT_XGETBV_BIT		2
#define SUPPORT_XSAVES_XRTORS_BIT	3
#define SUPPORT_XCR0			0x7


#define SUPPORT_XSAVEOPT		(1 << SUPPORT_XSAVEOPT_BIT)
#define SUPPORT_XSAVEC			(1 << SUPPORT_XSAVEC_BIT)
#define SUPPORT_XGETBV			(1 << SUPPORT_XGETBV_BIT)
#define SUPPORT_XSAVES_XRTORS		(1 << SUPPORT_XSAVES_XRTORS_BIT)



#define XSAVE_AREA_MIN_SIZE (512+64)
#define XSAVE_AREA_MAX_SIZE 0x440
#define XSAVE_AREA_SIZE 200
#define SUPPORT_INIT_OPTIMIZATION 1
#define IA32_XSS_ADDR	0xDA0
#define XMM_REGISTER_SIZE	16*16
#define XSAVE_BV_LOCATION 512
#define YMM0_LOCATION 576


#define XCR0_MASK       0x00000000
#define X86_EFLAGS_AC	0x00040000    // (1UL << 18)

#define ALIGNED(x) __attribute__((aligned(x)))

struct xsave_st {
	u64 num_xsave __attribute__((aligned(8)));
} xsave_st;


typedef unsigned __attribute__((vector_size(16))) sse128;
typedef unsigned __attribute__((vector_size(32))) avx256;
typedef float __attribute__((vector_size(16))) avx128;

typedef union {
	sse128 sse;
	unsigned u[4];
	uint64_t sse_u64[2];
} sse_union;

typedef union {
	avx256 avx;
	float m[8];
	u32 avx_u[8];
	uint64_t avx_u64[4];
} avx_union;


/*
 *XSAVE structure in below:
 *fpu_sse_struct:	416 bytes;
 *xsave_header_struct:	64 bytes;
 *xsave_avx_struct :	256 bytes;
 *xsave_bndreg_struct:	64 bytes;
 *xsave_bndcsr_struct:	16 bytes;
 *Total: Only support: X87/SSE/AVX/MPX;
 *xsave_area_struct:	1040 bytes;
 */
typedef unsigned __attribute__((vector_size(16))) fpu_st;
typedef unsigned __attribute__((vector_size(16))) sse_xmm;
typedef unsigned __attribute__((vector_size(16))) avx_ymm;
typedef unsigned __attribute__((vector_size(16))) bnd_reg;

/*legacy area for fpu&sse.416 bytes totally*/
typedef struct fpu_sse_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  reserved;
	u16 fpop;
	u64 fpip;
	u64 fpdp;
	u32 mxcsr;
	u32 mxcsr_mask;
	fpu_st fpregs[8];
	sse_xmm xmm_regs[16];
} __attribute__((packed)) fpu_sse_t;

/*64bytes xsave header*/
typedef struct xsave_header_struct {
	u64 xstate_bv;
	u64 xcomp_bv;
	u64 reserved[6];
} xsave_header_t;

/* Ext. save area 2: AVX State 256bytes*/
typedef struct xsave_avx_struct {
	avx_ymm avx_ymm[16];
} xsave_avx_t;

/* Ext. save area 3: BNDREG 64bytes*/
typedef struct xsave_bndreg_struct {
	bnd_reg bnd_regs[4];
} xsave_bndreg_t;

/* Ext. save area 4: BNDCSR 16bytes*/
typedef struct xsave_bndcsr_struct {
	u64 cfg_reg_u;
	u64 status_reg;
} xsave_bndcsr_t;


/* we only support x87&sse&avx&mpx for XSAVE feature set now!!
 *  1040 bytes totally
 */
typedef struct xsave_area_struct {
	u8 fpu_sse[512];
	struct xsave_header_struct xsave_hdr;//64
	/*extended area*/
	u8 ymm[256];
	u8 lwp[128];/*this is a gap,i don't know what it should be until now....*/
	struct xsave_bndreg_struct bndregs;//64 bytes
	struct xsave_bndcsr_struct bndcsr;//16 bytes/*by cpuid.0d.04 return eax--0x40 this is 64 bytes */
} __attribute__((packed)) xsave_area_t;

/* xsave_dump_struct:752 bytes totally*/
typedef struct xsave_dump_struct {
	struct fpu_sse_struct fpu_sse;// 416 bytes
	struct xsave_avx_struct ymm;// 256 bytes
	struct xsave_bndreg_struct bndregs;//64 bytes
	struct xsave_bndcsr_struct bndcsr;//16 bytes
} xsave_dump_t;//
/***
 *XSAVE structure end!
 */


struct xsave_exception {
	uint8_t vector;
	uint16_t error_code;
};

/* Expected register values following the start-up/INIT */
#define XSAVE_EXPECTED_STARTUP_XCR0		1UL
#define XSAVE_EXPECTED_STARTUP_XINUSE		3UL
#define XSAVE_EXPECTED_STARTUP_CR4_OSXSAVE	0UL
#define XSAVE_EXPECTED_INIT_CR4_OSXSAVE		0UL

#define XSAVE_LEGACY_AREA_SIZE			512U
#define XSAVE_HEADER_AREA_SIZE			64U
#define XSAVE_AVX_AREA_SIZE			256U
#define XSAVE_EXTEND_AREA_SIZE			XSAVE_AVX_AREA_SIZE

/* Mask of compacted form */
#define XSAVE_COMPACTED_FORMAT			(1UL << 63U)

/* ST0 is located in the legacy region */
#define XSAVE_ST0_OFFSET			4U
#define XSAVE_ST0_BITS_79_64_MASK		0xFFU

/* XMM0 is located in the legacy region */
#define XSAVE_XMM0_OFFSET			20U
/* YMM0 is located in the extended region */
#define XSAVE_YMM0_OFFSET			0U

#define XSAVE_SUPPORTED_USER_STATES		(STATE_X87 | STATE_SSE | STATE_AVX)
#define XSAVE_SUPPORTED_SUPERVISOR_STATES	0UL

/* CPUID.(EAX=DH, ECX=i):ECX[0]: user or supervisor state component */
#define XSAVE_USER_SUPERVISOR			(1U < 0U)
/* CPUID.(EAX=DH, ECX=i):ECX[1]: alignment indicator */
#define XSAVE_ALIGNMENT				(1U < 1U)

/* Task Switched (bit 3 of CR0) */
#define CR0_TS_MASK				(1UL << 3U)

union xsave_header {
	uint64_t value[XSAVE_HEADER_AREA_SIZE / sizeof(uint64_t)];
	struct {
		/* bytes 7:0 */
		uint64_t xstate_bv;
		/* bytes 15:8 */
		uint64_t xcomp_bv;
	} hdr;
};

struct xsave_area {
	uint64_t legacy_region[XSAVE_LEGACY_AREA_SIZE / sizeof(uint64_t)];
	union xsave_header xsave_hdr;
	uint64_t extend_region[XSAVE_EXTEND_AREA_SIZE / sizeof(uint64_t)];
};

struct xsave_fxsave_struct {
	uint16_t fcw;
	uint16_t fsw;
	uint8_t  ftw;
	uint8_t  revd1;
	uint16_t fop;
	uint32_t fip;
	uint16_t fcs;
	uint16_t rsvd2;
	uint32_t fdp;
	uint16_t fds;
	uint16_t rsvd3;
	uint32_t mxcsr;
	uint32_t mxcsr_mask;
	uint64_t fpregs[16];
	uint64_t xmm_regs[32];
	uint64_t rsvd4[12];
} __attribute__((packed));


static inline void asm_write_xcr(uint32_t reg, uint64_t val)
{
	asm volatile("xsetbv" : : "c"(reg), "a"((uint32_t)val), "d"((uint32_t)(val >> 32U)));
}

static inline uint64_t asm_read_xcr(uint32_t reg)
{
	uint32_t xcrl, xcrh;

	asm volatile("xgetbv " : "=a"(xcrl), "=d"(xcrh) : "c"(reg));
	return (((uint64_t)xcrh << 32U) | xcrl);
}

static inline void asm_write_cr4_osxsave(uint64_t osxsave_bit)
{
	uint64_t cr4 = read_cr4();
	uint64_t new_val;

	if (osxsave_bit) {
		new_val = cr4 | X86_CR4_OSXSAVE;
	} else {
		new_val = cr4 & (~X86_CR4_OSXSAVE);
	}

	write_cr4(new_val);
}

static inline uint64_t asm_read_cr4_osxsave(void)
{
	return (read_cr4() & X86_CR4_OSXSAVE);
}

static inline void asm_xsaveopt(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile("xsaveopt %0"
			: : "m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");
}

static inline void asm_xsavec(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile("xsavec %0"
			: : "m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");
}

static inline void asm_xsave(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile("xsave %0"
			: : "m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");
}

static inline void asm_xrstor(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile("xrstor %0"
			: : "m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");
}

static inline void asm_fxsave(struct xsave_fxsave_struct *fxsave_ptr)
{
	asm volatile("fxsave %0" : "=m"(*(fxsave_ptr)));
}

static inline void asm_pause(void)
{
	asm volatile("pause" ::: "memory");
}

struct xsave_case {
	uint32_t case_id;
	const char *case_name;
};
#endif
