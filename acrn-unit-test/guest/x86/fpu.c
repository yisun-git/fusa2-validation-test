#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "fpu.h"
#include "vm.h"
#include "alloc.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "ioram.h"
#include "apic.h"
#include "misc.h"
#include "fwcfg.h"
#include "smp.h"
#include "xsave.h"

#define get_bit(x, n)  (((x) >> (n)) & 1)
#define CR0_NE_BIT		(1<<5)

typedef struct fxsave_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u32 fip;
	u16 fcs;
	u16 rsvd2;
	u32 fdp;
	u16 fds;
	u16 rsvd3;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	sse_xmm xmm_regs[8];
} __attribute__((packed)) fxsave_t;

typedef struct fxsave_64bit_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u64 fip;
	u64 fdp;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	sse_xmm xmm_regs[8];
} __attribute__((packed)) fxsave_64bit_t;

typedef struct fsave_16bit_struct {
	u16 fcw;
	u16 fsw;
	u16 ftw;
	u16 fip_low;
	u16 fip_high;
	u16 fdp_low;
	u16 fdp_high;
} __attribute__((packed)) fsave_16bit_t;

typedef struct fsave_32bit_p_mode_struct {
	u32 fcw;
	u32 fsw;
	u32 ftw;
	u32 fip;
	u32 fcs;
	u32 fdp;
	u16 fds;
} __attribute__((packed)) fsave_32bit_t;

#ifdef __i386__
/*
 *
 *This function for i386 compiling, do nothing
 */
void save_unchanged_reg()
{
	asm volatile ("pause");
}
#elif __x86_64__
static uint32_t ap_start_count = 0U;
volatile long double st_unchange[8];

void save_unchanged_reg(void)
{
	volatile fxsave_64bit_t *fpu_xsave;
	ulong cr4;

	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	cr4 = read_cr4();
	write_cr4(cr4 | (1<<9));//set OSFXSR

	if (ap_start_count == 0) {
		fpu_xsave = (volatile fxsave_64bit_t *)INIT_UNCHANGED_FPU_SAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*fpu_xsave));

		for (int i = 0; i < 7; i++) {
			/*set a new value to st*/
			fpu_xsave->fpregs[i] = fpu_xsave->fpregs[i] + 1;
		}
		/*new value restore to fpu*/
		asm volatile("fxrstor %0" : : "m"(*fpu_xsave));
		/*save fxsave area to a gloabal var*/
		for (int i = 0; i < 8; i++) {
			st_unchange[i] = fpu_xsave->fpregs[i];
		}
		ap_start_count++;
	} else {
		fpu_xsave = (volatile fxsave_64bit_t *)INIT_UNCHANGED_FPU_SAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*fpu_xsave));

		/*save fxsave area to a gloabal var*/
		for (int i = 0; i < 8; i++) {
			st_unchange[i] = fpu_xsave->fpregs[i];
		}
	}
	write_cr4(cr4);
}

#endif


#ifdef IN_NATIVE
/*
 * @brief case name: Physical platform FPU capability enumeration_001
 *
 *Summary:Under 64 bit mode, execute CPUID(1):EDX.FPU[bit 0] equal to 1
 *
 */
static void fpu_rqmid_35010_physical_capability_enumeration_AC_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", get_bit(result, 0) == 1, __FUNCTION__);
}

/*
 * @brief case name: Physical platform shall support FXSAVE and FXRSTOR Instructions_001
 *
 *Summary:execute CPUID(1):EDX.FXSR[bit 24] shall be 1
 */
static void fpu_rqmid_35016_physical_support_FXSAVE_FXRSTOR_AC_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", get_bit(result, 24) == 1, __FUNCTION__);
}

/*
 * @brief case name: Physical platform shall have deprecated FPU CS/DS supported_001
 *
 *Summary:execute CPUID.(EAX=07H, ECX=0H) and checkÂ EBX.Deprecates_FPU_CS_FPU_DS [bit 13]=1

 */
static void fpu_rqmid_35017_physical_deprecated_FPU_CS_DS_AC_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).b;
	report("%s", get_bit(result, 13) == 1, __FUNCTION__);
}

/*
 * @brief case name: Physical platform shall have FDP_EXCPTN_ONLY disabled_001
 *
 *Summary:execute CPUID(EAX=07H, ECX=0H):EBX.FDP_EXCPTN_ONLY [bit 06] shall be 0
 */

static void fpu_rqmid_35018_physical_FDP_EXCPTN_only_disabled_AC_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).b;
	report("%s", get_bit(result, 6) == 0, __FUNCTION__);
}

/*
 * @brief case name: Physical platform FPU environment_001
 *
 *Summary:execute CPUID.01H:EDX.FPU [bit 0] shall be 1
 */

static void fpu_rqmid_35019_physical_FPU_environment_AC_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", get_bit(result, 0) == 1, __FUNCTION__);
}

#else

#ifdef __x86_64__
/*
 * @brief case name: Ignore Changes of CR0.NE_001
 *
 *Summary:When a vCPU attempts to write CR0.NE, the old guest CR0.NE is 1 and the new guest CR0.NE is 0, ACRN hypervisor
 *shall guarantee that the write to the bit is ignored.
 */

static void fpu_rqmid_32385_Ignore_Changing_CR0_NE_to_001()
{
	ulong cr0;
	ulong val;
	cr0 = read_cr0();

	if (get_bit(cr0, 5) == 1) {
		val = cr0 & ~0x20; /*NE set to 0*/
		write_cr0(val);
		cr0 = read_cr0();
		report("%s", get_bit(cr0, 5) == 1, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/*
 * @brief case name: FPU shall hide FDP_EXCPTN_ONLY_001
 *
 *Summary:ACRN hypervisor shall hide Exception-only FDP update from any guest, in compliance
 *with Chapter 8.1.8, Vol.1, SDM.
 */

static void fpu_rqmid_40028_FPU_hide_Exception_only_FDP_update_001()
{
	report("%s", ((cpuid_indexed(7, 0).b >> 6) & 0x1) == 0, __FUNCTION__);
}


/*
 * @brief case name: x87 FPU Control Word states following start-up_001
 *
 *Summary:
 *	regitster						power up		reset			init
 *	x87 FPU Control Word			0040H			0040H			FINIT/FNINIT: 037FH
 *
 *	Get x87 FPU Control Word at BP start-up, the register value shall be 0040H and same with
 *	SDM definition.
 */
static void fpu_rqmid_32363_FPU_Control_Word_state_following_startup_001()
{
	printf("fpu control word value is %x\n", *(u16 *)STARTUP_FPU_CWS_ADDR);
	report("%s", *(u16 *)STARTUP_FPU_CWS_ADDR == 0x40, __FUNCTION__);
}

/*
 * @brief case name: CR0.MP state following start-up_001
 *
 *Summary:
 *	regitster   power up     reset			init
 *	CR0    60000010H		60000010H       60000010H
 *
 *	Get CR0.MP:cr0.bit[1] at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void fpu_rqmid_32381_cr0_mp_state_following_startup_001()
{
	report("%s", (*(u32 *)STARTUP_CR0_REG_ADDR & 0x2) == 0, __FUNCTION__);
}

/*
 * @brief case name: CR0.NE state following start-up_001
 *
 *Summary:
 *	regitster   power up     reset			init
 *	CR0    60000010H		60000010H       60000010H
 *
 *	Get CR0.NE:cr0.bit[5] at BP start-up, the bit shall be 1 and same with SDM definition.
 */
static void fpu_rqmid_39079_cr0_ne_state_following_startup_001()
{
	report("%s", ((*(u32 *)STARTUP_CR0_REG_ADDR >> 5) & 0x1) == 1, __FUNCTION__);
}

/*
 * @brief case name: CR0.EM state following start-up_001
 *
 *Summary:
 *	regitster   power up     reset			init
 *	CR0    60000010H		60000010H       60000010H
 *
 *	Get CR0.EM:cr0.bit[2] at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void fpu_rqmid_39080_cr0_em_state_following_startup_001()
{
	report("%s", ((*(u32 *)STARTUP_CR0_REG_ADDR >> 2) & 0x1) == 0, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Data Operand and Inst. Pointers states following start-up_001
 *
 *Summary:ACRN hypervisor shall set initial guest x87 FPU Data Operand and Inst. Pointers to
 *00000000H following start-up.
 *
 */
static void fpu_rqmid_32360_fpu_data_operand_and_Inst_Pointers_state_following_startup_001()
{
	fsave_32bit_t *fpu_save = NULL;

	fpu_save = (fsave_32bit_t *)STARTUP_FPU_FSAVE_ADDR;
	report("%s", (fpu_save->fdp == 0x0) && (fpu_save->fip == 0)
		, __FUNCTION__);
}


/*
 * @brief case name: x87 FPU Data Operand and CS Seg. Selectors states following start-up_001
 *
 *Summary:
 *	regitster					power up		reset			init
 *	x87 FPU Data Operand
	and CS Seg. Selectors		0000H			0000H			FINIT/FNINIT: 0000H
 *
 *	Get x87 FPU Data Operand and CS Seg Selectors at BP start-up, the register value shall be 0000H and
 *	same with SDM definition.
 */
static void fpu_rqmid_32362_fpu_data_operand_and_cs_seg_state_following_startup_001()
{
	fsave_32bit_t *fpu_save = NULL;

	fpu_save = (fsave_32bit_t *)STARTUP_FPU_FSAVE_ADDR;
	report("%s", (fpu_save->fdp == 0x0) && ((fpu_save->fcs & 0xFFFF) == 0)
		, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Tag Word states following start-up_001
 *
 *Summary:
	Get x87 FPU Tag Word at BP start-up, the register value shall be 5555H and same with SDM definition.
 */

static void fpu_rqmid_32356_Tag_Word_states_following_startup_001()
{
	fsave_32bit_t *fpu_save;	/*fsave area refer to sdm vol1 8.1.10*/

	fpu_save = (fsave_32bit_t *)STARTUP_FPU_FSAVE_ADDR;
	printf("fpu_save->ftw=%x\n", fpu_save->ftw);
	report("%s", (fpu_save->ftw & 0xFFFF) == 0x5555, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Status Word states following start-up_001
 *
 *Summary:
	ACRN hypervisor shall set initial guest x87 FPU Status Word to 0000H following start-up.
 */

static void fpu_rqmid_32358_Status_Word_states_following_startup_001()
{
	fsave_32bit_t *fpu_save;	/*fsave area refer to sdm vol1 8.1.10*/

	fpu_save = (fsave_32bit_t *)STARTUP_FPU_FSAVE_ADDR;
	printf("fpu_save->ftw=%x\n", fpu_save->fsw);
	report("%s", (fpu_save->fsw & 0xFFFF) == 0x0, __FUNCTION__);
}

/*
 * @brief case name: ST0 through ST7 states following start-up_001
 *
 *Summary:
	ACRN hypervisor shall set initial guest ST0 through ST7 to +0.0 following start-up..
 */

static void fpu_rqmid_32365_st0_through_st7_states_following_startup_001()
{
	u8 *ptr;
	volatile struct fxsave_struct *fpu_save_startup;

	/*the INIT value should be equal to BP startup value,because the startup value is 0,so we just compare with 0*/
	fpu_save_startup = (struct fxsave_struct *)STARTUP_FPU_XSAVE_ADDR;
	ptr = (u8 *)fpu_save_startup->fpregs;
	for (int i = 0; i < sizeof(fpu_save_startup->fpregs); i++) {
		if (*(ptr + i) != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
}


#endif
#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name: fpu shall expose deprecated fpu cs ds 001
 *
 *Summary:
 *  FSAVE/FNSAVE, FSTENV/FNSTENV
 *	If CR0.PE = 1, each instruction saves FCS and FDS into memory. If
 *	CPUID.(EAX=07H,ECX=0H):EBX[bit 13] = 1, the processor deprecates FCS and FDS;
 *	it saves each as 0000H.
 *
 *	Under protect mode, excute FSAVE dump registers FDS and FCS value shall be 0000H
 */
#ifdef __i386__
static void fpu_rqmid_32375_shall_expose_deprecated_cs_ds_001()
{
	u32 cr0;
	static u8 fsave[94] = {0};

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

	asm volatile("fsave %0 \n\t"
				 : "=m"(fsave) : : "memory");

	/*fcs locate byte[12,13];fds locate byte[20,21]*/
	if ((cpuid(0x7).b & (1 << 13))
		&& (fsave[12] == 0) && (fsave[13] == 0)
		&& (fsave[20] == 0) && (fsave[21] == 0)) {
		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}

}
/*
 * @brief case name: FPU execution environment_Protected Mode_FADD_#PF_001
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode ,
 * Rebulid the paging structure, to create the page fault(pgfault: occur),? executing FADD shall generate #PF .
 */
static void fpu_rqmid_31656_execution_environment_protected_mode_FADD_pf_001()
{
	ulong cr0;
	u32 *op1;

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u32));
	set_page_control_bit((void *)op1, PAGE_PTE, PAGE_P_FLAG, 0, true);

	asm volatile(
		ASM_TRY("1f")
		"fadd %0\n\t"
		"1:"
		: : "m"(*op1) :);
	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	write_cr0(cr0);
}
#endif
#ifdef __x86_64__
/*
 * @brief case name: x87 FPU Data Operand and Inst. Pointers states following INIT_001
 *
 *Summary:
 *	 register                      power up         reset                   init
 *	x87 FPU Data Operand
 *	and Inst. Pointers5            00000000H     00000000H         FINIT/FNINIT: 00000000H
 *
 *	Get X87 FPU Data Operand and Inst. Pointers register value at AP init,
 *the value shall be 00000000H and same with SDM definition.
 *
 */
static void fpu_rqmid_32361_x87_FPU_data_operand_and_Inst_pointer_states_following_INIT_001()
{
	struct fxsave_struct *fpu_save;

	fpu_save = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	if (((fpu_save->fip & 0xffffU) == 0) && (fpu_save->fcs == 0)
		&& ((fpu_save->fdp & 0xffffU) == 0) && (fpu_save->fds == 0)) {

		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}
}
/*
 * @brief case name: st0 through st7 states following INIT_002
 *
 *Summary:
 *	register			power up		reset					init
 *	ST0 through ST7     +0.0            +0.0                    FINIT/FNINIT: Unchanged
 *
 *  the registers of ST0 through ST7 following INIT should be equal to BP startup(power up value)
 * Note: this case must run before 32366
 */
static void fpu_rqmid_37198_st0_through_st7_states_following_INIT_002()
{
	u8 *ptr;
	volatile struct fxsave_struct *fpu_save_init;

	/*the INIT value should be equal to BP startup value,because the startup value is 0,so we just compare with 0*/
	fpu_save_init = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	ptr = (u8 *)fpu_save_init->fpregs;
	for (int i = 0; i < sizeof(fpu_save_init->fpregs); i++) {
		if (*(ptr + i) != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
}
/*
 * @brief case name: st0 through st7 states following INIT_001
 *
 *Summary:
 *	register			power up		reset					init
 *	ST0 through ST7     +0.0            +0.0                    FINIT/FNINIT: Unchanged
 *
 *  After AP receives first INIT, set the value of ST0 through ST7;
 *	Dump ST0 through ST7 value shall get the same value after second INIT.
 *
 */
static void fpu_rqmid_32366_st0_through_st7_states_following_INIT_001()
{
	volatile long double st1[8];
	volatile long double st2[8];

	for (int i = 0; i < 8; i++) {
		st1[i] = st_unchange[i];
	}

	/*send second sipi */
	send_sipi();
	for (int i = 0; i < 8; i++) {
		st2[i] = st_unchange[i];
	}

	/*compare st1 with st2  ,the result should be equal*/
	for (int i = 0; i < 8; i++) {
		if (st1[i] != st2[i]) {
			report("%s ", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: CR0.NE state following INIT_001
 *
 *Summary:
 *	register			power up		reset			init
 *	CR0                 60000010H     60000010H        60000010H
 *
 *  Get CR0 register value at AP init, the CR0 bit NE shall be 1 and same with SDM definition.
 */

static void fpu_rqmid_32380_CR0_NE_state_following_INIT_001()
{
	report("%s", ((*(u32 *)INIT_CR0_REG_ADDR >> 5) & 0x1) == 1, __FUNCTION__);
}


/*
 * @brief case name: CR0.MP state following INIT_001
 *
 *Summary:
 *	register			power up		reset			init
 *	CR0                 60000010H     60000010H        60000010H
 *
 *  Get CR0 register value at AP init, the CR0 bit MP shall be 0 and same with SDM definition.
 */

static void fpu_rqmid_32382_CR0_MP_state_following_INIT_001()
{
	report("%s", (*(u32 *)INIT_CR0_REG_ADDR & 0x2) == 0, __FUNCTION__);
}

/*
 * @brief case name: CR0.EM state following INIT_001
 *
 *Summary:
 *	register			power up		reset			init
 *	CR0                 60000010H     60000010H        60000010H
 *
 *  Get CR0 register value at AP init, the CR0 bit EM shall be 0 and same with SDM definition.
 */

static void fpu_rqmid_32384_CR0_EM_state_following_INIT_001()
{
	report("%s", ((*(u32 *)INIT_CR0_REG_ADDR >> 2) & 0x1) == 0, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Status Word states following INIT_001
 *
 *Summary:
 *  Get X87 FPU Status Word register value at AP init, the value shall be 0000H and same with SDM definition.
 */

static void fpu_rqmid_32359_Status_Word_states_following_INIT_001()
{
	report("%s", *(u16 *)INIT_FPU_SWS_ADDR == 0, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Tag Word states following INIT_001
 *
 *Summary:
 *	Get X87 FPU Tag Word register value at AP init, the value shall be FFFFH and same with SDM definition.
 */

static void fpu_rqmid_32357_Tag_Word_states_following_INIT_001()
{
	struct fsave_16bit_struct *fpu_save;/*fsave area ,pls refer to sdm vol1 8.1.10*/
#if 0
	struct fxsave_struct *fpu_xsave;
	fpu_xsave = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	printf("fpu_xsave->ftw=%x\n", fpu_xsave->ftw);
#endif

	fpu_save = (struct fsave_16bit_struct *)INIT_FPU_FSAVE_ADDR;
	printf("fpu_save->ftw=%x\n", fpu_save->ftw);
	report("%s", fpu_save->ftw == 0xFFFF, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Control Word states following INIT_001
 *
 *Summary:
 *	register						power up		reset			init
 *	x87 FPU Control Word			0040H			0040H			FINIT/FNINIT: 037FH
 *
 *  Get X87 FPU Control Word register value at AP init, the value shall be 037FH and same with SDM definition.
 */

static void fpu_rqmid_32364_Control_Word_states_following_INIT_001()
{
	printf("init Control Word 0x%x\n", *(u16 *)INIT_FPU_CWS_ADDR);
	report("%s", *(u16 *)INIT_FPU_CWS_ADDR == 0x37F, __FUNCTION__);
}

/*
 * @brief case name: x87 FPU Data Operand and CS Seg. Selectors states following INIT_001
 *
 *Summary:
 *	regitster					power up		reset			init
 *	x87 FPU Data Operand
	and CS Seg. Selectors		0000H			0000H			FINIT/FNINIT: 0000H
 *
 *	Get x87 FPU Data Operand and CS Seg Selectors at AP start-up, the register value shall be 0000H and
 *	same with SDM definition.
 */
static void fpu_rqmid_39119_fpu_data_operand_and_cs_seg_state_following_init_001()
{

	struct fxsave_struct *fpu_save = NULL;

	fpu_save = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	report("%s", (fpu_save->fdp == 0x0) && ((fpu_save->fcs & 0xFFFF) == 0), __FUNCTION__);
}

/*
 * @brief case name: fpu execution environment FDISI 001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 *	Execute FDISI instruction shall have N/A,
 * and the CRx and related registers listed below should be unchanged
 *
 *	R0.PE [bit 0],CR0.NE [bit 5],CR0.EM [bit 2],CR0.MP [bit 1]CR0.TS [bit 3],
 *CR4.OSFXSR [bit 9]Data Registers R0~R7,Control Register,Status Register,Tag Register,
 *Last instruction pointer register,Last data pointer register,Opcode register,should be unchanged
 *
 */
static void fpu_rqmid_32377_execution_environment_FDISI_001()
{
	bool ret = true;
	ulong cr0_1, cr4_1, cr0_2, cr4_2;
	__attribute__ ((aligned(16))) struct fxsave_64bit_struct fxsave_64bit_1, fxsave_64bit_2;

	cr0_1 = read_cr0();
	cr4_1 = read_cr4();
	asm volatile("fxsave %0":"=m"(fxsave_64bit_1));

	asm volatile("FDISI\n\t");

	cr0_2 = read_cr0();
	cr4_2 = read_cr4();
	asm volatile("fxsave %0":"=m"(fxsave_64bit_2));

	/*0x2F=10 1111:bit0,bit1,bit2,bit3,bit5*/
	if ((cr0_1 & 0x2FUL) != (cr0_2 & 0x2FUL)) {
		ret = false;
	}

	if ((cr4_1 & (1UL << 9)) != (cr4_2 & (1UL << 9))) {
		ret = false;
	}

	if (memcmp(fxsave_64bit_1.fpregs, fxsave_64bit_2.fpregs, 128U)) {
		ret = false;
	}

	if ((fxsave_64bit_1.fcw != fxsave_64bit_2.fcw)
		|| (fxsave_64bit_1.fsw != fxsave_64bit_2.fsw)
		|| (fxsave_64bit_1.ftw != fxsave_64bit_2.ftw)
		|| (fxsave_64bit_1.fop != fxsave_64bit_2.fop)
		|| (fxsave_64bit_1.fip != fxsave_64bit_2.fip)
		|| (fxsave_64bit_1.fdp != fxsave_64bit_2.fdp)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: FPU capability enumeration_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *	execute CPUID(1):EDX.FPU and CPUID(1):EDX.FXSR shall be 1
 *	a) FPU execution environment
 *			CPUID.01H:EDX.FPU [bit 0]
 *			CR0.PE [bit 0],CR0.EM [bit 2], CR0.MP[bit 1]
 *			CR0.TS [bit 3]
 *			REX Prefixes: REX.W
 *   b) FXSAVE and FXRSTOR instructions
 *			CPUID.01H:EDX.FXSR [bit 24]
 *			CR4.OSFXSR [bit 9]
 */
static void fpu_rqmid_32378_FPU_capability_enumeration_001()
{
	/*fpu:edx.bit[0];fxsr:edx.bit[24]*/
	u32 cr0;
	u32 check_value0 = 0;
	u32 check_value1 = 0;
	u32 check_value;
	u32 count = 0;
	u8 fxsave[512];
	int op = 0;

	/*check CPUID(1):EDX.FPU[bit 0]*/
	if (cpuid(0x1).d & (1)) {
		count++;
	} else {
		report("%s", false, __FUNCTION__);
		return;
	}


	/*check CPUID.01H:EDX.FXSR [bit 24]*/
		if (cpuid(0x1).d & (1 << 24)) {

			count++;
		} else {

			report("%s", false, __FUNCTION__);
			return;
		}

	cr0 = read_cr0();
	/*pls refer to sdm vol3 chaper 2.5 table 2-2 about fpu  execution*/
	/*Check CR0.PE [bit 0],CR0.EM [bit 2], CR0.MP[bit 1] CR0.TS [bit 3]*/
	check_value0 = ((cr0 & (1 << CR0_BIT_EM)) == 0) && ((cr0 & (1 << CR0_BIT_MP)) == 0) &&
					((cr0 & (1 << CR0_BIT_TS)) == 0);

	check_value1 = ((cr0 & (1 << CR0_BIT_EM)) == 0) && (((cr0 >> CR0_BIT_MP) & 1) == 1) &&
					((cr0 & (1 << CR0_BIT_TS)) == 0);

	check_value = ((cr0 & (1 << CR0_BIT_PE)) == 1) && (check_value0 || check_value1);
	if (check_value) {
		count++;
	} else {
		report("%s", false, __FUNCTION__);
		return;
	}

	/*check REX Prefixes: REX.W*/
	asm volatile(ASM_TRY("1f")
		"rex.w\n\t"
		"fild %0\n\t"
		"1:"
		::"m"(op));
	if (exception_vector() != NO_EXCEPTION) {
		report("%s", false, __FUNCTION__);
		return;
	}

	/* check CR4.OSFXSR [bit 9], it should be set*/
	write_cr4(read_cr4() | (1 << CR4_BIT_OSFXSR));

	/*check fxsave instrunction*/
	asm volatile(ASM_TRY("1f")
			 "fxsave %0\n\t"
			 "1:"
			 : "=m"(fxsave));

	if (exception_vector() != NO_EXCEPTION) {
		report("%s", false, __FUNCTION__);
		return;
	}

	/*check fxrstor instrunction*/
	asm volatile(ASM_TRY("1f")
			 "fxrstor %0\n\t"
			 "1:"
			 : : "m"(fxsave));
	if (exception_vector() != NO_EXCEPTION) {
		report("%s", false, __FUNCTION__);
		return;
	}

	report("%s", count == 3, __FUNCTION__);
}
/*
 * @brief case name: CR0.MP state following start-up_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 *	Under 64 bit Mode ,  
 * Rebulid the paging structure, to create the page fault(pgfault: occur),executing FICOMP shall generate #PF ..
 */
static void fpu_rqmid_31189_execution_environment_64_bit_Mode_FICOMP_PF_001()
{
	u32 cr0;
	u16 *op1;

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u16));
	set_page_control_bit((void *)op1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	asm volatile(ASM_TRY("1f")
				 "ficomp %0\n\t"
				 "1:"
				 : : "m"(*op1) :);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);
	write_cr0(cr0);
}

/*
 * @brief case name: FPU execution environment_64 bit Mode_FIST_#GP_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL0 ,
 * If the memory address is in a non-canonical form(Ad. Cann.: non mem),  executing FIST shall generate #GP.
 */
static void fpu_rqmid_31436_execution_environment_64_bit_Mode_FIST_GP_001()
{
	u32 cr0;
	u16 *op1;

	if ((read_cs() & 0x3) != 0) {

		report("%s \t should under ring0", 0, __FUNCTION__);
		return;
	}

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u16));
	op1 = (u16 *)((ulong)(op1) ^ (1ULL << 63)); /*make non-canonical address*/
	asm volatile(ASM_TRY("1f")
				 "fist %0\n\t"
				 "1:"
				 : : "m"(*op1) :);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
	write_cr0(cr0);
}
static void fpu_64_bit_mode_FILD_at_ring1(const char *msg)
{
	u8 level;
	float op1 = 1.2;
	int op2 = 0;
	unsigned short cw;

	level = read_cs() & 0x3;

	cw = 0x37b;/*bit2==0,report div0 exception*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 ASM_TRY("1f")
				 "FILD %2\n"
				 "1:"
				 : : "m"(cw), "m"(op1), "m"(op2));

	ring1_ret = (exception_vector() == MF_VECTOR) && (level == 1);
	asm volatile("fninit");
}
/*
 * @brief case name: FPU execution environment_64 bit Mode_FILD_#MF_002
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL1 ,
 * If there is a pending x87 FPU exception(FPU excp: hold), executing FILD shall generate #MF .
 */
static void fpu_rqmid_31907_execution_environment_64_bit_Mode_FILD_MF_002()
{
	u32 cr0;

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/
	write_cr0(read_cr0() | (1 << 5));/*set cr0.ne*/

	do_at_ring1(fpu_64_bit_mode_FILD_at_ring1, "");
	report("%s", ring1_ret,  __FUNCTION__);

	asm volatile("fninit");/*init the FPU exception status*/
	ring1_ret = false;/*reset the ring1 result*/
	write_cr0(cr0);
}
static void fpu_64bit_ss_handler(struct ex_regs *regs)
{
	unsigned ex_val;
	extern unsigned char fsave_ss_ret;
	ex_val = regs->vector | (regs->error_code << 16) |
			 (((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));

	regs->rip = (unsigned long)&fsave_ss_ret;
}
static void fpu_64bit_mode_fsave_ss_at_ring3(const char *msg)
{
	u8 level;
	ulong  op1;
	handler old;
	u8 fsave[94] = {0};

	old = handle_exception(SS_VECTOR, fpu_64bit_ss_handler);
	op1 = (ulong)fsave;
	op1 ^= (1UL << 63);/*make non-cannoical addr*/
	asm volatile(
		"fsave %0 \n\t"
		"fsave_ss_ret:"
		: "=m"(*(ulong *)op1));

	level = read_cs() & 0x3;
	ring3_ret = (exception_vector() == SS_VECTOR) && (level == 3);
	handle_exception(SS_VECTOR, old);
}
/*
 * @brief case name: FPU execution environment_64 bit Mode_FSAVE_#SS_010
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL3 ,
 * If a memory address referencing the SS segment is in a non-canonical form(Ad. Cann.: non stack),
 * executing FSAVE shall generate #SS . .
 */
static void fpu_rqmid_31551_execution_environment_64_bit_Mode_FSAVE_SS_010()
{
	u32 cr0;
	ulong eflags;

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear EM TS*/

	/*disable eflags AC check eflags[bit18]*/
	asm volatile(
		"pushf\n\t"
		"pop %%" R "ax\n\t"
		"mov %%" R "ax,%0 \n\t"
		"and $~(1<<18),%%" R "ax\n\t"
		"push %%" R "ax\n\t"
		"popf\n\t"
		: "=m"(eflags) : :);

	do_at_ring3(fpu_64bit_mode_fsave_ss_at_ring3, "");
	report("%s", ring3_ret,  __FUNCTION__);
	/*restore flags back*/
	asm volatile(
		"mov %0, %%eax \n\t"
		"push %%" R "ax \n\t"
		"popf \n\t"
		: : "m"(eflags) :);

	ring3_ret = false;/*reset the ring3 result*/
	write_cr0(cr0);
}
#endif
#endif
#endif
static void print_case_list(void)
{
	printf("FPU feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 35010,
		"Physical platform FPU capability enumeration_001");
	printf("\t Case ID:%d case name:%s\n\r", 35016,
		"Physical platform shall support FXSAVE and FXRSTOR Instructions_001");
	printf("\t Case ID:%d case name:%s\n\r", 35017,
		"Physical platform shall have deprecated FPU CS/DS supported_001");
	printf("\t Case ID:%d case name:%s\n\r", 35018,
		"Physical platform shall have FDP_EXCPTN_ONLY disabled_001");
	printf("\t Case ID:%d case name:%s\n\r", 35019,
		" Physical platform FPU environment_001");

#else
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 32381,
		   "cr0 mp state following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 39079,
		   "CR0.NE state following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 39080,
		   "CR0.EM state following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 32363,
		   "x87 FPU Control Word states following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 32362,
		   "x87 FPU Data Operand and CS Seg. Selectors states following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 32356,
		   "x87 FPU Tag Word states following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 32358,
		   "x87 FPU Status Word states following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 32365,
		   "ST0 through ST7 states following start-up_001");

#endif
#ifdef IN_NON_SAFETY_VM
#ifdef __i386__
	printf("\t Case ID:%d case name:%s\n\r", 32375, "shall expose deprecated cs ds 001");
	printf("\t Case ID:%d case name:%s\n\r", 31656, "execution environment protected mode FADD pf 001()");
#endif
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 32361,
		   "x87 FPU data operand and Inst pointer states following INIT 001");
	printf("\t Case ID:%d case name:%s\n\r", 37198,
		   "st0 through st7 states following INIT 002");
	printf("\t Case ID:%d case name:%s\n\r", 32366,
		   "st0 through st7 states following INIT 001;");
	printf("\t Case ID:%d case name:%s\n\r", 32380,
		   "CR0.NE state following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32382,
		   "CR0.MP state following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32384,
		   "CR0.EM state following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32357,
		   "x87 FPU Tag Word states following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32359,
		   "x87 FPU Status Word states following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32364,
		   "x87 FPU Control Word states following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 39119,
		   "x87 FPU Data Operand and CS Seg. Selectors states following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 32385,
		   "Ignore Changes of CR0.NE_001");
	printf("\t Case ID:%d case name:%s\n\r", 32377,
		   "execution environment FDISI 001");
	printf("\t Case ID:%d case name:%s\n\r", 32378,
		   "FPU capability enumeration 001");
	printf("\t Case ID:%d case name:%s\n\r", 31189,
		   "execution environment 64 bit Mode FICOMP PF 001");
	printf("\t Case ID:%d case name:%s\n\r", 31436,
		   "execution environment 64 bit Mode FIST GP 001");
	printf("\t Case ID:%d case name:%s\n\r", 31907,
		   "execution environment 64 bit Mode FIST MF 002");
	printf("\t Case ID:%d case name:%s\n\r", 31551,
		   "execution environment 64 bit Mode FSAVE SS 010");
#endif
#endif
#endif
}

int main(void)
{
	setup_idt();
	setup_vm();
	setup_ring_env();
	print_case_list();
#ifdef IN_NATIVE
	fpu_rqmid_35010_physical_capability_enumeration_AC_001();
	fpu_rqmid_35016_physical_support_FXSAVE_FXRSTOR_AC_001();
	fpu_rqmid_35017_physical_deprecated_FPU_CS_DS_AC_001();
	fpu_rqmid_35018_physical_FDP_EXCPTN_only_disabled_AC_001();
	fpu_rqmid_35019_physical_FPU_environment_AC_001();
#else
#ifdef __x86_64__
	fpu_rqmid_32381_cr0_mp_state_following_startup_001();
	fpu_rqmid_39079_cr0_ne_state_following_startup_001();
	fpu_rqmid_39080_cr0_em_state_following_startup_001();
	fpu_rqmid_32360_fpu_data_operand_and_Inst_Pointers_state_following_startup_001();
	fpu_rqmid_32363_FPU_Control_Word_state_following_startup_001();
	fpu_rqmid_32362_fpu_data_operand_and_cs_seg_state_following_startup_001();
	fpu_rqmid_32356_Tag_Word_states_following_startup_001();
	fpu_rqmid_32358_Status_Word_states_following_startup_001();
	fpu_rqmid_32385_Ignore_Changing_CR0_NE_to_001();
	fpu_rqmid_40028_FPU_hide_Exception_only_FDP_update_001();
	fpu_rqmid_32365_st0_through_st7_states_following_startup_001();
#endif
#ifdef IN_NON_SAFETY_VM
#ifdef __i386__
	fpu_rqmid_32375_shall_expose_deprecated_cs_ds_001();
	fpu_rqmid_31656_execution_environment_protected_mode_FADD_pf_001();
#endif
#ifdef __x86_64__
	fpu_rqmid_32361_x87_FPU_data_operand_and_Inst_pointer_states_following_INIT_001();
	fpu_rqmid_37198_st0_through_st7_states_following_INIT_002();
	fpu_rqmid_32366_st0_through_st7_states_following_INIT_001();
	fpu_rqmid_32380_CR0_NE_state_following_INIT_001();
	fpu_rqmid_32382_CR0_MP_state_following_INIT_001();
	fpu_rqmid_32384_CR0_EM_state_following_INIT_001();

	fpu_rqmid_32357_Tag_Word_states_following_INIT_001();
	fpu_rqmid_32359_Status_Word_states_following_INIT_001();
	fpu_rqmid_32364_Control_Word_states_following_INIT_001();
	fpu_rqmid_39119_fpu_data_operand_and_cs_seg_state_following_init_001();

	fpu_rqmid_32377_execution_environment_FDISI_001();
	fpu_rqmid_32378_FPU_capability_enumeration_001();
	fpu_rqmid_31189_execution_environment_64_bit_Mode_FICOMP_PF_001();
	fpu_rqmid_31436_execution_environment_64_bit_Mode_FIST_GP_001();
	fpu_rqmid_31907_execution_environment_64_bit_Mode_FILD_MF_002();
	fpu_rqmid_31551_execution_environment_64_bit_Mode_FSAVE_SS_010();
#endif
#endif
#endif
	return report_summary();
}

