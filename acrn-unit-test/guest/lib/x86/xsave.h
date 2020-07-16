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
} sse_union;

typedef union {
	avx256 avx;
	float m[8];
	u32 avx_u[8];
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
#endif
