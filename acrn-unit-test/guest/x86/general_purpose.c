#include "processor.h"
#include "instruction_common.h"
#include "xsave.h"
#include "libcflat.h"
#include "desc.h"
#include "misc.h"
#include "register_op.h"
#include "types.h"
#include "asm/io.h"
#include "general_purpose.h"
#include "regdump.h"
#include "interrupt.h"
#include "fwcfg.h"
#include "delay.h"


#define TRY_INSTRUCTION(_ins)\
do { \
	asm volatile( \
			ASM_TRY("1f") \
			_ins "\n" \
			"1:\n" \
			: : : "memory"); \
	if (exception_vector() != NO_EXCEPTION) { \
		printf("%s\n", _ins); \
		report("%s %d", false, __FUNCTION__, __LINE__); \
		return; \
	} \
} while (0)


#define CHECK_REPORT() do {\
			if (exception_vector() != NO_EXCEPTION) {\
				report("%s %d", false, __FUNCTION__, __LINE__);\
				return;\
		} \
} while (0)

#define get_bit(x, n)  (((x) >> (n)) & 1)

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;
static volatile int esp = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}


#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_eax_init_value()
{
	asm volatile ("movl $0xFFFF, %eax\n");
}

__unused static void modify_edx_init_value()
{
	asm volatile ("movl $0xFFFF, %edx\n");
}

__unused static void modify_eflags_init_value()
{
	/*cf change to 1*/
	write_rflags(read_rflags() | 1ul);
}

__unused static void modify_ebx_ecx_ebp_esp_esi_edi_init_value()
{
	asm volatile ("movl $0xFFFF, %ebx\n");
	asm volatile ("movl $0xFFFF, %ecx\n");
	asm volatile ("movl $0xFFFF, %ebp\n");
	asm volatile ("movl $0xFFFF, %esi\n");
	asm volatile ("movl $0xFFFF, %edi\n");
	asm volatile ("movl %%esp, %0\n" : "=m"(esp));
}

__unused static void modify_r8_r15_init_value()
{
	asm volatile ("movq $0xFFFF, %r8\n");
	asm volatile ("movq $0xFFFF, %r9\n");
	asm volatile ("movq $0xFFFF, %r10\n");
	asm volatile ("movq $0xFFFF, %r11\n");
	asm volatile ("movq $0xFFFF, %r12\n");
	asm volatile ("movq $0xFFFF, %r13\n");
	asm volatile ("movq $0xFFFF, %r14\n");
	asm volatile ("movq $0xFFFF, %r15\n");
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 25070:
		fp = modify_eax_init_value;
		ap_init_value_process(fp);
		break;
	case 25080:
		fp = modify_edx_init_value;
		ap_init_value_process(fp);
		break;
	case 25061:
		fp = modify_eflags_init_value;
		ap_init_value_process(fp);
		break;
	case 25468:
		fp = modify_ebx_ecx_ebp_esp_esi_edi_init_value;
		ap_init_value_process(fp);
		break;
	case 46083:
		fp = modify_r8_r15_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

#ifdef __x86_64__
/*
 * @brief case name: Physical table lookup instructions support_001.
 *
 * Summary: Table lookup instructions shall be available on the physical platform.
 */
static int xlat_execute(void)
{
	__attribute__((aligned(64)))u64 addr;
	u64 *add = &addr;
	asm volatile(ASM_TRY("1f")
		     "xlat %[add]\n"
		     "1:"
		     : [add]"=m"(add) : :);
	return exception_vector();
}

static int xlatb_execute(void)
{
	asm volatile(ASM_TRY("1f")
		     "xlatb\n"
		     "1:"
		     :  : :);
	return exception_vector();
}

/*
 * @brief case name:Physical shift and rotate instructions support_001.
 *
 * Summary: Shift and rotate instructions shall be available on the physical platform.
 * SAL, SHL, SAR, SHR,SHLD, SHRD,ROL, ROR, RCL, RCR can be executed normally.
 */
static void  gp_rqmid_31253_shift_and_rotate_physical_support(void)
{
	TRY_INSTRUCTION("sal $1,%%eax\n");
	TRY_INSTRUCTION("shl $1,%%eax\n");
	TRY_INSTRUCTION("sar $1,%%eax\n");
	TRY_INSTRUCTION("shr $1,%%eax\n");
	TRY_INSTRUCTION("shld $1,%%eax,%%edx\n");
	TRY_INSTRUCTION("shrd $1,%%eax,%%edx\n");
	TRY_INSTRUCTION("rol %%eax\n");
	TRY_INSTRUCTION("ror %%eax\n");
	TRY_INSTRUCTION("rcl %%eax\n");
	TRY_INSTRUCTION("rcr %%eax\n");

	report("%s", true, __FUNCTION__);
}

/*--------------------------------Test case------------------------------------*/
static void  gp_rqmid_31282_table_lookup_physical_support(void)
{
	report("%s", ((xlat_execute() == PASS) && (xlatb_execute() == PASS)), __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports LZCNT instruction_001
 *
 * Summary: LZCNT instruction shall be available on the physical platform.
 *
 */
static void gp_rqmid_39004_cpuid_eax80000001_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x80000001, 0).c;
	report("%s", ((result >> 5) & 1), __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
#ifdef __x86_64__
/*
 * @brief case name: Physical support for MSR access instructions_001.
 *
 * Summary: MSR access instructions shall be available on the physical platform.
 */
static void  gp_rqmid_38975_msr_access_physical_support(void)
{
	u32 a, d;
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("rdmsr" : "=a"(a), "=d"(d) : "c"(IA32_PLATFORM_ID) : "memory");
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("wrmsr" : : "a"(a), "d"(d), "c"(MSR_IA32_TSCDEADLINE) : "memory");
	asm volatile ("1:");
	CHECK_REPORT();

	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name: Enter and leave instructions physical support_001
 *
 * Summary: Physical platform should support enter and leave instructions.
 */

static void  gp_rqmid_39014_enter_and_leave_instructions(void)
{

	TRY_INSTRUCTION("enter $0,$32\n");
	TRY_INSTRUCTION("leave\n");

	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports enhanced bitmap instructions_001
 *
 * Summary: The physical platform supports enhanced bitmap instructions
 *
 */
static void gp_rqmid_38980_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).b;
	report("%s", (result >> 3 & 1) && (result >> 8 & 1), __FUNCTION__);
}


/*
 * @brief case name:Physical random number generate instructions support_001
 *
 * Summary: Physical random number generator instructions shall be available on the physical
 * platform.
 *
 */
static void gp_rqmid_38977_cpuid_random_number_generate_instructions_001()
{
	u32 result1;
	u32 result2;
	result1 = cpuid_indexed(0x01, 0).c;
	result2 = cpuid_indexed(0x07, 0).b;

	if (get_bit(result1, 30) && get_bit(result2, 18)) {
		ulong random1 = 0;
		ulong random2 = 0;
		asm volatile(ASM_TRY("1f")
			"rdrand %0\n"
			"rdrand %1\n"
			"1:"
			: "=rm"(random1), "=rm"(random2)
			);
		report("%s", (NO_EXCEPTION == exception_vector()) && (random1 != random2), __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/*
 * @brief case name:Physical no-operation and undefined instruction support_001
 *
 * Summary: No-operation and undefined instruction shall be available on the physical platform.
 *
 */
static void gp_rqmid_38978_no_operation__instructions_001()
{
	unsigned long eip1;
	unsigned long eip2;

	asm volatile ("call 11f\n\t"
					"11:\n\t"
					"pop %0\n\t"
					: "=rm"(eip1)
					);

	asm volatile ("nop");
	asm volatile ("call 22f\n\t"
					"22:\n\t"
					"pop %0\n\t"
					: "=rm"(eip2)
					);

	report("%s", (eip2 - eip1 - 6) > 0, __FUNCTION__); /*The 6 is the length of instruction "call 22f , pop"*/
}

/*
 * @brief case name:Physical processor identification instruction support_001
 *
 * Summary: Physical processor identification instruction shall be available on the physical
 * platform ,refer to vol2 chaper 3.2 CPUID instruction .
 *
 */
static void gp_rqmid_38979_processor_identification_instructions_001()
{
	unsigned long result;
	unsigned long old_value;
	result = read_rflags();
	old_value = result;

	/*eflag bit 21 The ability of a program to set or clear this flag indicates support for the CPUID instruction*/
	write_rflags(result ^ (1UL << 21));
	if (result ==  read_rflags()) {
		report("%s", false, __FUNCTION__);
		return;
	}

	report("%s", true, __FUNCTION__);
	/*restore origin value*/
	write_rflags(old_value);
}

/*
 * @brief case name: Physical address computation instructions support_001.
 *
 * Summary: Address computation shall be available on the physical platform.
 */
static int lea_execute(void)
{
	u64 r_a;
	__attribute__((aligned(64)))u64 addr;
	u64 *add = &addr;
	asm volatile(ASM_TRY("1f")
		     "lea %[add], %0\n"
		     "1:"
		     : "=r"(r_a)
		     : [add]"m"(add));
	return exception_vector();
}

static void  gp_rqmid_31279_address_computation_physical_support(void)
{
	report("%s", lea_execute() == PASS, __FUNCTION__);
}

/*
 * @brief case name: Physical logical instructions support_001.
 *
 * Summary: Logical instructions shall be available on the physical platform.
 * XOR, AND, OR,and  NOT  can be executed normally.
 */

static void  gp_rqmid_31249_logical_instruction_physical_support(void)
{
	u32 r_a = 5, r_b = 2;

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("xor %1, %0\n" : "=r"(r_a) : "r"(r_b));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("and %1, %0\n" : "=r"(r_a) : "r"(r_b));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("or %1, %0\n" : "=r"(r_a) : "r"(r_b));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("not %0\n" : "+r"(r_a) : "r"(r_a));
	asm volatile ("1:");
	CHECK_REPORT();

	report("%s", true, __FUNCTION__);
}

static int lfs_execute(void)
{
	struct lseg_st lfs;

	lfs.offset = 0x0;
	lfs.selector = 0x10;

	asm volatile(ASM_TRY("1f")
		"lfs  %0, %%eax\t\n"
		"1:"
		::"m"(lfs));
	return exception_vector();
}


static int lgs_execute(void)
{
	struct lseg_st lgs;

	lgs.offset = 0x0;
	lgs.selector = 0x10;

	asm volatile(ASM_TRY("1f")
		"lgs  %0, %%eax\t\n"
		"1:"
		::"m"(lgs));
	return exception_vector();
}

static int lss_execute(void)
{
	struct lseg_st lss;

	lss.offset = 0x0;
	lss.selector = 0x10;

	asm volatile(ASM_TRY("1f")
		"lss  %0, %%eax\t\n"
		"1:"
		::"m"(lss));
	return exception_vector();
}

/*
 * @brief case name: Physical segment register instructions support_001
 *
 * Summary: Segment register load and save instructions shall be available on the physical platform.
 */
//LDS,LFS,LES,LGS,LSS

static void  gp_rqmid_31275_segment_load_save_physical_support(void)
{
	int count = 0;

	if (lfs_execute() == PASS) {
		count++;
	}

	if (lgs_execute() == PASS) {
		count++;
	}

	if (lss_execute() == PASS) {
		count++;
	}

	report("%s", count == 3, __FUNCTION__);
}

/*
 * @brief case name: Physical bit and byte instructions support_001
 *
 * Summary: Bit and byte instructions shall be available on the physical platform.
 */

static void  gp_rqmid_31257_bit_and_byte_physical_support(void)
{
	eflags_cf_to_1();
	TRY_INSTRUCTION("bt $7,%%dx\n");
	TRY_INSTRUCTION("bts $7,%%dx\n");
	TRY_INSTRUCTION("btr $7,%%dx\n");
	TRY_INSTRUCTION("btc $7,%%dx\n");
	TRY_INSTRUCTION("bsf %%cx,%%dx\n");
	TRY_INSTRUCTION("bsr %%cx,%%dx\n");
	TRY_INSTRUCTION("test $0,%%eax\n");
	TRY_INSTRUCTION("setc %%al\n");

	report("%s", true, __FUNCTION__);
}
/*
 * @brief case name:Physical Binary arithmetic instructions support_001.
 *
 * Summary: Binary arithmetic instructions shall be available on the physical platform.

 * ADD, ADC, SUB, SBB, INC, DEC, CMP, NEG, MUL, IMUL, DIV, IDIV can be executed normally.
 */
static void  gp_rqmid_31242_binary_arithmetic_physical_support(void)
{
	TRY_INSTRUCTION("add $1,%%eax\n");
	TRY_INSTRUCTION("adc $1,%%eax\n");
	TRY_INSTRUCTION("sub $1,%%eax\n");
	TRY_INSTRUCTION("sbb $1,%%eax\n");
	TRY_INSTRUCTION("inc %%eax\n");
	TRY_INSTRUCTION("dec %%eax\n");
	TRY_INSTRUCTION("cmp $1,%%eax\n");
	TRY_INSTRUCTION("neg %%eax\n");
	TRY_INSTRUCTION("mul %%eax\n");
	TRY_INSTRUCTION("imul %%eax\n");
	TRY_INSTRUCTION("div %%eax\n");
	TRY_INSTRUCTION("idiv %%eax\n");

	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name: Physical I/O instructions support_001.
 *
 * Summary: I/O instructions shall be available on the physical platform.
 */
 // IN/INS/OUT/OUTS/INSB/INSW/INSD/OUTSB/OUTSW/OUTSD

static void  gp_rqmid_31269_io_instructions_physical_support(void)
{
	volatile uint8_t mmio[100];
	static char st1[] = "abcdefghijklmnop";
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("in %%dx, %%eax\n" : : );
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("insb" : : "d" (0xe0), "D" (mmio));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("insw" : : "d" (0xe0), "D" (mmio));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("insl" : : "d" (0xe0), "D" (mmio));//insd
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile("out %%ax, %%dx\n" : : );
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile("outsb \n\t" : : "i"((short)0xe0), "S"(st1));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile("outsw \n\t" : : "i"((short)0xe0), "S"(st1));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile("outsl \n\t" : : "i"((short)0xe0), "S"(st1));//OUTSD
	asm volatile ("1:");
	CHECK_REPORT();

	report("%s", exception_vector() == NO_EXCEPTION, __FUNCTION__);
}

/*
 * @brief case name: Physical Flag control instructions support_001.
 *
 * Summary: Flag control instructions shall be available on the physical platform.
 */
// STC, CLC, CMC, STD, CLD LAHF/SAHF(64-bit Mode) PUSHF/POPF PUSHFD/POPFD,STI ,CLI

static void  gp_rqmid_31272_flag_control_physical_support(void)
{
	if ((cpuid(0x80000001).c & 1UL) == 0) {
		report("%s %d", false, __FUNCTION__, __LINE__);
		return;
	}

	TRY_INSTRUCTION("pushf\n");
	TRY_INSTRUCTION("popf\n");
	TRY_INSTRUCTION("stc\n");
	TRY_INSTRUCTION("clc\n");
	TRY_INSTRUCTION("cmc\n");
	TRY_INSTRUCTION("std\n");
	TRY_INSTRUCTION("cld\n");
	TRY_INSTRUCTION("sti\n");
	TRY_INSTRUCTION("cli\n");
	TRY_INSTRUCTION("lahf\n");
	TRY_INSTRUCTION("sahf\n");

	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name: Physical string operations instructions support_001.
 *
 * Summary: String operations shall be available on the physical platform.
 */
 //MOVSB/STOSB(REP),MOVS, CMPS, SCAS, LODS, STOS, can be executed normally.

static void  gp_rqmid_31265_string_operation_physical_support(void)
{
	u64 r_a = 1;
	u8 in[16];
	u8 out[16];
	u8 *source = in;
	u8 *dest = out;

	asm volatile ("cld\n");
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("rep/movsb\n":"+D"(dest):"S"(source), "c"(1));
	asm volatile ("1:");
	CHECK_REPORT();

	TRY_INSTRUCTION("rep stosb");
	TRY_INSTRUCTION("cmpsb\n");
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("scasq \n" : : "a"(r_a));
	asm volatile ("1:\n");
	CHECK_REPORT();

	TRY_INSTRUCTION("lodsb\n");
	TRY_INSTRUCTION("stosw\n");

	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name:Physical General data transfer instructions support_001.
 *
 * Summary: Genearal data transfer instructions shall be available on the phyical platform.
 * MOVE, CMOVcc, XCHG, BSWAP, XADD,CMPXCHG, CMPXCHG8B, CMPXCHG16B,PUSH, POP,
 * CBW, CWDE, CWD, CDQ, MOVSX, MOVZX, MOVSXD can be executed normally.
 */

static void  gp_rqmid_31238_data_transfer_physical_support_001(void)
{
	u64 r_a;
	__attribute__((aligned(64)))u64 addr;
	u64 *add1 = &addr;
	u8 tst[16];
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("mov %1, %0\n" : "=r" (r_a) : "m" (add1));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("cmovb %1, %0\n" : "=r" (r_a) : "m" (add1));
	asm volatile ("1:");
	CHECK_REPORT();

	TRY_INSTRUCTION("xchg %%dx,%%ax\n");
	TRY_INSTRUCTION("bswapl %%eax\n");
	TRY_INSTRUCTION("xaddl %%edx,%%eax\n");
	TRY_INSTRUCTION("cmpxchg %%dx,%%ax\n");

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("cmpxchg8b %0\n" : : "m" (add1));
	asm volatile ("1:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("cmpxchg16b %0\n" : : "m" (tst));
	asm volatile ("1:");
	CHECK_REPORT();

	TRY_INSTRUCTION("push %%rax\n");
	TRY_INSTRUCTION("pop %%rax\n");
	TRY_INSTRUCTION("cbw\n");
	TRY_INSTRUCTION("cwde\n");
	TRY_INSTRUCTION("cwd\n");
	TRY_INSTRUCTION("cdq\n");
	TRY_INSTRUCTION("movsx %%bl,%%eax");
	TRY_INSTRUCTION("movzx %%bl,%%eax");

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("movsxd %1, %0\n" : "=r" (r_a) : "m" (add1));
	asm volatile ("1:");
	CHECK_REPORT();

	report("%s", true, __FUNCTION__);
}

asm("test_31261: \n"
	"ret\n");
void test_31261(void);

void handled_interrupt_extern_80(isr_regs_t *regs)
{
	eoi();
}

/*
 * @brief case name: Physical control transfer instructions support_001.
 *
 * Summary: Control transfer instructions shall be available on the physical platform.
 * CALL, RET, INT, IRET,Jcc,JECXZ,INTn, JMP, LOOP, LOOPE, LOOPZ, LOOPNE, LOOPNZ
 * can be executed normally.
 */


static void  gp_rqmid_31261_control_transfer_physical_support(void)
{
	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("call test_31261 \n");
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("loop1: jge out \n"
					"jmp loop1 \n"
					"out: nop \n"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $10, %ecx\n\t"
					"1: inc %eax\n\t"
					"loop 1b\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $10, %ecx\n\t"
					"mov $1, %eax\n\t"
					"1: dec %eax\n\t"
					"loope 1b\n\t");
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $10, %ecx\n\t"
					"mov $1, %eax\n\t"
					"1: dec %eax\n\t"
					"loopz 1b\n\t");
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $10, %ecx\n\t"
					"mov $5, %eax\n\t"
					"1: dec %eax\n\t"
					"loopne 1b\n\t");
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $10, %ecx\n\t"
					"mov $5, %eax\n\t"
					"1: dec %eax\n\t"
					"loopnz 1b\n\t");
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $0, %ecx\n\t"
					".byte 0xE3, 0x00\n\t"	//jecxz 1f
					"1:nop\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $0, %ecx\n\t"
					"jnc 1f\n\t"
					"1:nop\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $0, %ecx\n\t"
					"jnz 1f\n\t"
					"1:nop\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $0, %ecx\n\t"
					"je 1f\n\t"
					"1:nop\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();

	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("mov $0, %ecx\n\t"
					"jne test_31261\n\t"
					"1:nop\n\t"
					);
	asm volatile ("2:");
	CHECK_REPORT();


	handle_irq(EXTEND_INTERRUPT_80, handled_interrupt_extern_80);
	sti();
	asm volatile (ASM_TRY("2f"):::"memory");
	asm volatile ("INT $"xstr(EXTEND_INTERRUPT_80)"\n\t");
	asm volatile ("2:");
	CHECK_REPORT();

	report("%s", true, __FUNCTION__);
}


#else
/*
 * @brief case name: Physical control transfer instructions support_002.
 *
 * Summary: Control transfer instructions shall be available on the physical platform.
 * bound,into  can be executed normally.
 */

static void  gp_rqmid_39833_control_transfer_physical_support_32bit(void)
{
	int t[2] = {0, 10};
	u32 r_a = 0x1;
	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("bound %1, %[add]\n"
		: [add]"=m"(t[0])
		: "r"(r_a)
		: "memory");

	asm volatile ("1:");
	CHECK_REPORT();
	TRY_INSTRUCTION("into");
	asm volatile ("1:");
	report("%s", exception_vector() == NO_EXCEPTION, __FUNCTION__);
}

/*
 * @brief case name:Physical General data transfer instructions support_002.
 *
 * Summary: Genearal data transfer instructions shall be available on the phyical platform.
 * PUSHA, POPA can be executed normally.
 */

static void  gp_rqmid_39834_data_transfer_physical_support_32bit(void)
{
	TRY_INSTRUCTION("pusha");
	TRY_INSTRUCTION("popa");
	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name: Physical Flag control instructions support_002.
 *
 * Summary: Flag control instructions shall be available on the physical platform.
 * pushfd popfd
 */

static void  gp_rqmid_39835_flag_control_physical_support_32bit(void)
{
	//asm volatile ("pushfd"); //32BIT
	TRY_INSTRUCTION(".byte 0x9c"); //32BIT
	//asm volatile ("popfd");
	TRY_INSTRUCTION(".byte 0x9d"); //32BIT
	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name:Physical Decimal Arithmetic instructions support_001.
 * Summary: Decimal arithmetic instructions shall be available on the physical platform.
 */
 //DAA, DAS AAA, AAS, AAM, AAD can be executed normally.

static void  gp_rqmid_31245_decimal_arithmetic_physical_support(void)
{
	TRY_INSTRUCTION("daa");
	TRY_INSTRUCTION("das");
	TRY_INSTRUCTION("aaa");
	TRY_INSTRUCTION("aas");
	TRY_INSTRUCTION("aam");
	TRY_INSTRUCTION("aad");

	report("%s", true, __FUNCTION__);
}

static int lds_execute(void)
{
	struct lseg_st lds;

	lds.offset = 0x0;
	lds.selector = 0x10;

	asm volatile(ASM_TRY("1f")
		"lds  %0, %%eax\t\n"
		"1:"
		::"m"(lds));
	return exception_vector();
}

static int les_execute(void)
{
	struct lseg_st les;

	les.offset = 0x0;
	les.selector = 0x10;

	asm volatile(ASM_TRY("1f")
		"les  %0, %%eax\t\n"
		"1:"
		::"m"(les));
	return exception_vector();
}


/*
 * @brief case name: Physical segment register instructions support_002
 *
 * Summary: Segment register load and save instructions shall be available on the physical platform.
 */
//LDS,LES

static void  gp_rqmid_39836_segment_load_save_physical_support_32bit(void)
{
	int count = 0;

	if (lds_execute() == PASS) {
		count++;
	}

	if (les_execute() == PASS) {
		count++;
	}

	report("%s", count == 2, __FUNCTION__);
}

#endif
#else

#ifdef __x86_64__

/**Some common function **/
static u16 *creat_non_aligned_add(void)
{
	__attribute__ ((aligned(16))) u64 addr;
	u64 *aligned_addr = &addr;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

u64 non_canon_addr = 0;
static u64 *get_non_canon_addr(void)
{
	u64 address = (unsigned long)&non_canon_addr;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address|(1UL<<63));
	}
	return (u64 *)address;
}

/* load the GDT to*/
void load_gdt_and_set_segment_rigster(void)
{
	asm volatile ("lgdt gdt64_desc\n"
		"mov $0x10, %ax\n"
		"mov %ax, %ds\n"
		"mov %ax, %es\n"
		"mov %ax, %fs\n"
		"mov %ax, %gs\n"
		"mov %ax, %ss\n"
	);
}
#endif

#ifdef __x86_64__
/*
 * @brief case name: 31315: Data transfer instructions Support_64 bit Mode_MOV_#GP_007.
 *
 * Summary:  Under 64 bit Mode and CPL0,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_007(void)
{
	u64 r_a;
	asm volatile (
		"mov %1, %0\n"
		: "=r" (r_a)
		: "m" (*(get_non_canon_addr())));
}

static void  gp_rqmid_31315_data_transfer_mov_gp_007(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_007;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31318: Data transfer instructions Support_64 bit Mode_MOV_#GP_008.
 *
 * Summary:  Under 64 bit Mode and CPL1,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_008(void)
{
	u64 r_a = 1;
	asm volatile ("mov %1, %0\n"
		: "=m"(*(get_non_canon_addr()))
		: "r"(r_a));
}

static void gp_rqmid_31318_data_transfer_mov_gp_008(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_008;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31321: Data transfer instructions Support_64 bit Mode_MOV_#GP_009.
 *
 * Summary:  Under 64 bit Mode and CPL2,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_009(void)
{
	u32 r_a = 1;
	asm volatile ("mov %1, %0\n"
		: "=m"(*(get_non_canon_addr()))
		: "r"(r_a));
}

static void  gp_rqmid_31321_data_transfer_mov_gp_009(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_009;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31324: Data transfer instructions Support_64 bit Mode_MOV_#GP_010.
 *
 * Summary:  Under 64 bit Mode and CPL3,
 * If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 * executing MOV shall generate #GP.
 */
static void mov_gp_010(void)
{
	u16 r_a = 1;

	eflags_ac_to_0();
	asm volatile ("mov %1, %0 \n"
		: "=r"(r_a)
		: "m"(*(get_non_canon_addr())));
}

static void  gp_rqmid_31324_data_transfer_mov_gp_010(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_010;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31327: Data transfer instructions Support_64 bit Mode_MOV_#GP_011.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_011(void)
{
	u32 r_a = 1;
	asm volatile ("mov %1, %0 \n"
		: "=m"(*(get_non_canon_addr()))
		: "r"(r_a));
}

static void ring3_mov_gp_011(const char *fun_name)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_011;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, fun_name);
}
static void  gp_rqmid_31327_data_transfer_mov_gp_011(void)
{
	cr0_am_to_0();

	do_at_ring3(ring3_mov_gp_011, __FUNCTION__);
}

/*
 * @brief case name: 31330: Data transfer instructions Support_64 bit Mode_MOV_#GP_012.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory access to the descriptor table is non-canonical(Ad. Cann.: non stack),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_012(void)
{
	u32 r_a = 1;
	asm volatile ("mov %1, %0 \n"
		: "=r"(r_a)
		: "m"(*(get_non_canon_addr())));
}

int ret_gp_12 = false;
static void  ring3_mov_gp_012(const char *fun_name)
{
	gp_trigger_func fun;
	//bool ret;

	fun = (gp_trigger_func)mov_gp_012;
	ret_gp_12 = test_for_exception(GP_VECTOR, fun, NULL);

	/*Because this case set cr0.am[bit 18], report in ring3 will generate AC*/
	//report("%s Execute Instruction:  MOV, to generate #GP",
	//	ret == true, fun_name);
}

static void  gp_rqmid_31330_data_transfer_mov_gp_012(void)
{
	cr0_am_to_1();
	eflags_ac_to_1();

	do_at_ring3(ring3_mov_gp_012, __FUNCTION__);
	report("%s Execute Instruction:  MOV, to generate #GP",
		ret_gp_12 == true, __FUNCTION__);
}

void gp_data_transfer_mov_ud(const char *fun_name)
{
	asm volatile(ASM_TRY("1f")
		"mov %0, %%cs \n"
		"1:"
		: : "r"(0x8) : );
	report("%s", (exception_vector() == UD_VECTOR), fun_name);
}
/*
 * @brief case name: 31333: Data transfer instructions Support_64 bit Mode_MOV_#UD_001.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31333_data_transfer_mov_ud_001(void)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31336: Data transfer instructions Support_64 bit Mode_MOV_#UD_002.
 *
 * Summary: Under 64 bit Mode and CPL1,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31336_data_transfer_mov_ud_002(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31339: Data transfer instructions Support_64 bit Mode_MOV_#UD_003.
 *
 * Summary: Under 64 bit Mode and CPL2,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31339_data_transfer_mov_ud_003(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31342: Data transfer instructions Support_64 bit Mode_MOV_#UD_004.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If attempt is made to load the CS register(CSLoad: true),
 *  executing MOV shall generate #UD.
 */
static void  gp_rqmid_31342_data_transfer_mov_ud_004(const char *msg)
{
	gp_data_transfer_mov_ud(__FUNCTION__);
}

/*
 * @brief case name: 31370: Data transfer instructions Support_64 bit Mode_MOV_#GP_013.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to clear CR0PG[bit 31](CR0.PG: 0),
 *  executing MOV shall generate #GP.
 */
static void mov_gp_013(void)
{
	u64 check_bit = 0;
	check_bit = read_cr0();
	check_bit &= (~(FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_31)));
	asm volatile (
		"mov %0, %%cr0\n"
		: : "r"(check_bit) : "memory");
}

static void  gp_rqmid_31370_data_transfer_mov_gp_013(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)mov_gp_013;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction:  MOV, to generate #GP",
		ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31378: Data transfer instructions Support_64 bit Mode_MOV_#GP_015.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to write a 1 to any reserved bit in CR4(CR4.R.W: 1),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_31378_data_transfer_mov_gp_015(void)
{
	gp_trigger_func fun;
	bool ret;
	unsigned long check_bit = 0;

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31385: Data transfer instructions Support_64 bit Mode_MOV_#GP_017.
 *
 * Summary: Under 64 bit Mode,
 *  If an attempt is made to write a 1 to any reserved bit in CR8(CR8.R.W: 1),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_31385_data_transfer_mov_gp_017(void)
{
	cr8_r_w_to_1(__FUNCTION__);
}

/*
 * @brief case name: 32284: Data transfer instructions Support_64 bit Mode_MOV_#GP_021.
 *
 * Summary: Under 64 bit Mode,
 *   If an attempt is made to leave IA-32e mode by clearing CR4PAE[bit 5](CR4.PAE: 0),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_32284_data_transfer_mov_gp_021(void)
{
	report("%s #GP exception", cr4_pae_to_0() == true, __FUNCTION__);
}

/*
 * @brief case name: 31271: Data transfer instructions Support_64 bit Mode_CMPXCHG16B_#GP_060.
 *
 * Summary: Under 64 bit Mode,
 *   If memory operand for CMPXCHG16B is not aligned on a 16-byte boundary(CMPXCHG16B_Aligned: false),
 *  executing CMPXCHG16B shall generate #GP.
 */
__unused static void cmpxchg16b_gp_060(void)
{
	asm volatile ("cmpxchg16b %[add] \n"
		: [add]"=m"(*(creat_non_aligned_add())) : : "memory");
}
static void  gp_rqmid_31271_data_transfer_cmpxchg16b_gp_060(void)
{
	int is_support;
	gp_trigger_func fun;
	bool ret;

	is_support = ((cpuid(0x1).c)>>13)&1;
	if (is_support) {
		fun = (gp_trigger_func)cmpxchg16b_gp_060;
		ret = test_for_exception(GP_VECTOR, fun, NULL);
	} else {
		ret = false;
	}

#if 0
	CHECK_INSTRUCTION_INFO(__FUNCTION__, cpuid_cmpxchg16b_to_1);
	asm volatile ("CMPXCHG16B %[add] \n" : [add]"=m"(*(creat_non_aligned_add())) : : "memory");
#endif
	/* Need modify unit-test framework, CMPXCHG16B can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31323: Data transfer instructions Support_64 bit Mode_PUSH_#UD_017.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the PUSH is of CS, SS, DS, or ES(Push Seg: hold),
 *  executing PUSH shall generate #UD.
 */
__unused static void push_ud_017(void)
{
	asm volatile (".byte 0x0e \n");	  /* "push %cs \n" */
}

static void  gp_rqmid_31323_data_transfer_push_ud_017(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)push_ud_017;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, PUSH %%cs can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32288: Data transfer instructions Support_64 bit Mode_POP_#GP_066.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the descriptor is outside the descriptor table limit(Descriptor limit: outside),
 *  executing POP shall generate #GP.
 */
__unused static void pop_gp_066(void)
{
	asm volatile ("pop %fs \n");
}

static void  gp_rqmid_32288_data_transfer_pop_ud_066(void)
{
	gp_trigger_func fun;
	bool ret;
	set_gdt_entry(DS_SEL, 0, 0xc000f, 0x93, 0);

	load_gdt_and_set_segment_rigster();
	//asm volatile ("pop %fs \n");

	fun = (gp_trigger_func)pop_gp_066;
	ret = test_for_exception(GP_VECTOR, fun, NULL);
	/* Need modify unit-test framework, PUSH %%cs can't use exception handler. */
	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31495: Binary arithmetic instructions Support_64 bit Mode_IDIV_#DE_007.
 *
 * Summary:Under 64 bit Mode and CPL0,
 * If the quotient is too large for the designated register(Quot large: true),
 * executing IDIV shall generate #DE.
 */
__unused static void idiv_de_007(void)
{
	long int a = -4294967296;
	long int b = -1;
	asm volatile ("mov %[dividend], %%eax \n"
		"cdq  \n"
		"idiv %[divisor] \n"
		: : [dividend]"r"((int)a), [divisor]"r"((int)b) : );
}

static void  gp_rqmid_31495_binary_arithmetic_idiv_de_007(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)idiv_de_007;
	ret = test_for_exception(DE_VECTOR, fun, NULL);
	/* Need modify unit-test framework, idiv will trigger double-fault in ASM_TRY handler. */
	report("%s", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31522: Binary arithmetic instructions Support_64 bit Mode_DIV_normal_execution_059.
 *
 * Summary:Under 64 bit Mode and CPL0,
 * If the quotient is too large for the designated register(Quot large: true),
 * executing IDIV shall generate #DE.
 */
__unused static void div_execute(void)
{
	int a = 100;
	int b = 2;

	asm volatile ("mov %[dividend], %%eax \n"
		"div %[divisor] \n"
		"1:"
		: : [dividend]"r"(a), [divisor]"r"(b) : );
}

static void  gp_rqmid_31522_binary_arithmetic_div_normal_059(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)div_execute;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31655: Control transfer instructions Support_64 bit Mode_JMP_#UD_030.
 *
 * Summary: Under 64 bit Mode and CPL0 ,(64-bit mode only),
 *  If a far jump is direct to an absolute address in memory(JMP Abs address: true),
 *  executing JMP shall generate #UD.
 */
__unused static void jmp_ud_030(void)
{
	asm volatile (".byte 0xea, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
}

static void  gp_rqmid_31655_control_transfer_jmp_ud_030(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)jmp_ud_030;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: JMP", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31286: Control transfer instructions Support_64 bit Mode_CALL_#UD_042.
 *
 * Summary: Under 64 bit Mode and CPL0 ,(64-bit mode only),
 *  If a far call is direct to an absolute address in memory(CALL Abs address: true),
 *  executing CALL shall generate #UD.
 */
__unused static void call_ud_042(void)
{
	/* "call (absolute address in memory) \n" */
	//u64 *data_add = (u64 *)0x000000000048e520;
	asm volatile (".byte 0x9a, 0x20, 0xe5, 0x48, 0xe8, 0x9d, 0xfe");
	/*
	 *asm volatile ("call %[addr]\n\t"
	 *	: : [addr]"m"(data_add)
	 *	: "memory");
	 */
}

static void  gp_rqmid_31286_control_transfer_call_ud_042(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)call_ud_042;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31379: Control transfer instructions Support_64 bit Mode_IRET_#GP_097.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If EFLAGSNT[bit 14] = 1(EFLAGS.NT: 1),  executing IRET shall generate #GP.
 */
__unused static void iret_gp_097(void)
{
	/* "call (absolute address in memory) \n" */
	/* u64 *data_add = (u64 *)0x000000000048e520; */
	asm volatile ("iret \n");
}

static void  gp_rqmid_31379_control_transfer_iret_gp_097(void)
{
	gp_trigger_func fun;
	bool ret;

	CHECK_INSTRUCTION_INFO(__FUNCTION__, eflags_nt_to_1);
	fun = (gp_trigger_func)iret_gp_097;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	/* Need modify unit-test framework, Error: bad register name `%%gs:4'. */
	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31652: Control transfer instructions Support_64 bit Mode_INT1_#GP_115.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *  If the vector selects a descriptor outside the IDT limits(DesOutIDT: true),
 *  executing INT1 shall generate #GP.
 */
__unused static void int_gp_115(void)
{
	//asm volatile (".byte 0xf1");
	asm volatile ("INT $14");
}

static __unused void  gp_rqmid_31652_control_transfer_int_gp_115(void)
{
	struct descriptor_table_ptr old_idt;
	struct descriptor_table_ptr new_idt;
	gp_trigger_func fun;
	bool ret;

	sidt(&old_idt);
	printf("idt limit: %#x \n", old_idt.limit);

	new_idt.limit = (16*14) - 1;
	new_idt.base = old_idt.base;

	lidt(&new_idt);
	printf("modify idt limit to: %#x \n", new_idt.limit);

	fun = (gp_trigger_func)int_gp_115;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction: CALL", ret == true, __FUNCTION__);

	/* resume environment */
	lidt(&old_idt);
}

/*
 * @brief case name: 31901: Segment Register instructions Support_64 bit Mode_LFS_#GP_125.
 *
 * Summary: Under 64 bit Mode and CPL0,
 *   If the memory address is in a non-canonical form(CS.CPL: 0, RPL?CPL: true, S. segfault: occur),
 *   executing LFS shall generate #GP.
 */
static void lfs_gp_125(void)
{
	u64 *address;
	struct lseg_st lfss;
	lfss.offset = 0xffffffff;
	lfss.selector = 0x280;

	address = creat_non_canon_add((u64)&lfss);
	asm volatile ("REX\n\t"
		"lfs %0, %%rax\n\t"
		: : "m"(address));
}

static void  gp_rqmid_31901_segment_instruction_lfs_gp_125(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf(" Set gdt64 segment not present\n ");
	set_gdt_entry(0x50, 0, 0xc000f, 0x93, 0);

	//printf(" Load gdt to lgdtr\n ");
	asm volatile ("lgdt gdt64_desc\n");

	fun = (gp_trigger_func)lfs_gp_125;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s Execute Instruction: LFS", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31751: Segment Register instructions Support_64 bit Mode_LSS_#GP_132.
 *
 * Summary: Under 64 bit Mode and CPL3,
 *  If the memory address is in a non-canonical form(Ad. Cann.: non mem),
 *  executing LSS shall generate #GP.
 */
__unused static void lss_gp_132(void)
{
	eflags_ac_to_1();
	asm volatile ("lss (%0), %%eax  \n"
		: : "r"(get_non_canon_addr()));
}

static void  ring3_lss_gp_132(const char *msg)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)lss_gp_132;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	/* Need modify unit-test framework, can't handle exception. */
	report("%s Execute Instruction: LFS", ret == true, msg);
}

static void  gp_rqmid_31751_segment_instruction_lss_gp_132()
{
	cr0_am_to_0();

	do_at_ring3(ring3_lss_gp_132, __FUNCTION__);
}

/*
 * @brief case name: 31937: MSR access instructions_64 Bit Mode_RDMSR_#GP_004.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a reserved  MSR address(ECX_MSR_Reserved: true),
 *  executing RDMSR shall generate #GP.
 */
static void  gp_rqmid_31937_msr_access_rdsmr_gp_004(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*D10H - D4FH: Reserved MSR Address Space for L2 CAT Mask Registers*/
	report("%s Execute Instruction: reserved",
		rdmsr_checking(0x00000D20, aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31939: MSR access instructions_64 Bit Mode_RDMSR_#GP_005.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a unimplemented MSR address(ECX_MSR_Reserved: true),
 *  executing RDMSR shall generate #GP.
 */
static void  gp_rqmid_31939_msr_access_rdsmr_gp_005(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*4000_0000H-4000_00FFH:
	 *All existing and future processors will not implement MSRs in this range.
	 */
	report("%s Execute Instruction: unimplement",
		rdmsr_checking(0x40000000, aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31949: MSR access instructions_64 Bit Mode_WRMSR_#GP_009.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a unimplemented MSR address(ECX_MSR_Reserved: true),
 *  executing wrmsr shall generate #GP.
 */
static void  gp_rqmid_31949_msr_access_wrmsr_gp_009(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*4000_0000H-4000_00FFH:
	 *All existing and future processors will not implement MSRs in this range.
	 */
	report("%s Execute Instruction: unimplement",
		wrmsr_checking(0x40000000, *aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 31953: MSR access instructions_64 Bit Mode_WRMSR_#GP_011.
 *
 * Summary: Under Protected/64 Bit Mode and CPL0,
 *  If the value in ECX specifies a reserved MSR address(ECX_MSR_Reserved: true),
 *  executing wrmsr shall generate #GP.
 */
static void  gp_rqmid_31953_msr_access_wrmsr_gp_011(void)
{
	__attribute__ ((aligned(16))) u64 addr = 0;
	u64 *aligned_addr = &addr;
	/*D10H - D4FH: Reserved MSR Address Space for L2 CAT Mask Registers*/
	report("%s Execute Instruction: unimplement",
		wrmsr_checking(0x00000D20, *aligned_addr) == GP_VECTOR, __FUNCTION__);
}

/*
 * @brief case name: 32163: Random number generator instructions Support_64 Bit Mode_RDRAND_#UD_006.
 *
 * Summary: Under 64 Bit Mode,
 *  If the F2H or F3H prefix is used(F2HorF3H: hold),
 *  executing RDRAND shall generate #UD.
 */
__unused static void rdrand_ud_006(void)
{
	asm volatile (".byte 0xf3, 0x0f, 0xc7, 0xf0 \n");
}

static void  gp_rqmid_32163_random_number_rdrand_ud_006(void)
{
	gp_trigger_func fun;
	bool ret;
	int is_support;

	is_support = ((cpuid(0x1).c)>>30)&1;

	if (is_support) {
		fun = (gp_trigger_func)rdrand_ud_006;
		ret = test_for_exception(UD_VECTOR, fun, NULL);
	} else {
		ret = false;
	}

	/* Need modify unit-test framework, can't handle eception. */
	report("%s Execute Instruction: rdrand", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32191: Undefined instruction Support_64 Bit Mode_UD1_#UD_010.
 *
 * Summary: Under Protected/Real Address/64 Bit Mode,
 *  Raises an invalid opcode exception in all operating modes(OpcodeExcp: true),
 *  executing UD1 shall generate #UD.
 */
static void ud1_ud_010(void)
{
	asm volatile (
		/* "ud1 %%eax, %%edx \n" */
		".byte 0x0f, 0xb9 \n"
		: : :);
}

static void  gp_rqmid_32191_ud_instruction_ud1_ud_010(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)ud1_ud_010;
	ret = test_for_exception(UD_VECTOR, fun, NULL);

	report("%s Execute Instruction: UD1", ret == true, __FUNCTION__);
}

/*
 * @brief case name: Read 1 byte from I/O port with no peripheral_001
 *
 * Summary: When a vCPU attempts to read 1 byte from a guest I/O port address and ACRN
 * hypervisor locates no register at the guest I/O port address, ACRN hypervisor shall
 * write 0FFH to guest AX
 */

static void  gp_rqmid_39006_Read_one_byte_from_IO_port_with_no_peripheral_001(void)
{
	u8 value;

	value = inb(0x2e8);
	report("%s", value == 0xFF, __FUNCTION__);
}

/*
 * @brief case name: Read 2 byte from I/O port with no peripheral_001
 *
 * Summary: When a vCPU attempts to read 2 byte from a guest I/O port address and ACRN
 * hypervisor locates no register at the guest I/O port address, ACRN hypervisor shall
 * write 0FFFFH to guest AX
 */

static void  gp_rqmid_39008_read_two_byte_from_IO_port_with_no_peripheral_001(void)
{
	u16 value;

	value = inw(0x2e8);
	report("%s", value == 0xFFFF, __FUNCTION__);
}

/*
 * @brief case name: Read 4 byte from I/O port with no peripheral_001
 *
 * Summary: When a vCPU attempts to read 4 byte from a guest I/O port address and ACRN
 * hypervisor locates no register at the guest I/O port address, ACRN hypervisor shall
 * write 0FFFFFFFFH to guest AX
 */

static void  gp_rqmid_39009_read_four_byte_from_IO_port_with_no_peripheral_001(void)
{
	u32 value;
	value = inl(0x2e8);
	report("%s", value == 0xFFFFFFFF, __FUNCTION__);
}


/*
 * @brief case name: Write to I/O port with no peripheral_001
 *
 * Summary: When a vCPU attempts to write a guest I/O port address and ACRN hypervisor locates no register at the guest
 * I/O port address, ACRN hypervisor shall guarantee that the write is ignored.
 */

static void  gp_rqmid_39055_Write_to_port_with_no_peripheral(void)
{
	u16 value;

	outw(0xaa55, 0x2e8);
	value = inw(0x2e8);
	report("%s", value == 0xFFFF, __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief case name: EAX registers init value_001
 * Summary: Get eax register at AP init, the register's value shall be 0H and same with SDM definition.
 */
static void gp_rqmid_25070_eax_register_Following_init(void)
{
	volatile u32 eax_init = *((volatile u32 *)INIT_EAX_ADDR);

	bool is_pass = true;
	if (eax_init != 0x0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(25070);

	eax_init = *((volatile u32 *)INIT_EAX_ADDR);
	if (eax_init != 0x0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: EDX register init value_001
 * Summary: Get edx register at AP init, the register's value should be 00080600H.
 */
static void gp_rqmid_25080_edx_register_Following_init(void)
{
	volatile u32 edx_init = *((volatile u32 *)INIT_EDX_ADDR);
	bool is_pass = true;
	if (edx_init != 0x80600) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(25080);
	edx_init = *((volatile u32 *)INIT_EDX_ADDR);
	if (edx_init != 0x80600) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: EFLAGS register init value_001
 * Summary: Get eflags register at AP init, the bit shall be 00000002H and same with SDM definition.
 */

static void gp_rqmid_25061_eflags_register_Following_init(void)
{
	volatile u32 eflags_init = *((volatile u32 *)INIT_EFLAGS_ADDR);
	bool is_pass = true;

	if (eflags_init != 0x00000002) {
		is_pass = false;
	}
	notify_modify_and_read_init_value(25061);
	eflags_init = *((volatile u32 *)INIT_EFLAGS_ADDR);
	if (eflags_init != 0x00000002) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name: EBX, ECX, ESI, EDI, EBP, ESP register init value_001
 * Summary: Get ebx,ecx,esi,edi,ebp,esp at AP init, these register's value shall be 0H.
 */

static void gp_rqmid_25468_ebx_ecx_and_other_register_Following_init(void)
{
	volatile u32 ebx_init = *((volatile u32 *)INIT_EBX_ADDR);
	volatile u32 ecx_init = *((volatile u32 *)INIT_ECX_ADDR);
	volatile u32 ebp_init = *((volatile u32 *)INIT_EBP_ADDR);
	volatile u32 esp_init = *((volatile u32 *)INIT_ESP_ADDR);
	volatile u32 esi_init = *((volatile u32 *)INIT_ESI_ADDR);
	volatile u32 edi_init = *((volatile u32 *)INIT_EDI_ADDR);

	bool is_pass = true;

	if (!((ebx_init == 0x0) && (ecx_init == 0x0) && (ebp_init == 0x0) &&
		(esp_init == 0x0) && (esi_init == 0x0) && (edi_init == 0x0))) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(25468);

	ebx_init = *((volatile u32 *)INIT_EBX_ADDR);
	ecx_init = *((volatile u32 *)INIT_ECX_ADDR);
	ebp_init = *((volatile u32 *)INIT_EBP_ADDR);
	esp_init = *((volatile u32 *)INIT_ESP_ADDR);
	esi_init = *((volatile u32 *)INIT_ESI_ADDR);
	edi_init = *((volatile u32 *)INIT_EDI_ADDR);

	if (!((ebx_init == 0x0) && (ecx_init == 0x0) && (ebp_init == 0x0) &&
		(esp_init == 0x0) && (esi_init == 0x0) && (edi_init == 0x0))) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);

}

/**
 * @brief case name: Set initial guest R8~R15 to 0H for virtual AP_001
 * Summary: ACRN hypervisor shall set initial guest R8~R15 to 0H following INIT.
 */

static void gp_rqmid_46083_r8_r15_following_init(void)
{
	volatile u64 r8 = *((volatile u64 *)INIT_R8_ADDR);
	volatile u64 r9 = *((volatile u64 *)INIT_R9_ADDR);
	volatile u64 r10 = *((volatile u64 *)INIT_R10_ADDR);
	volatile u64 r11 = *((volatile u64 *)INIT_R11_ADDR);
	volatile u64 r12 = *((volatile u64 *)INIT_R12_ADDR);
	volatile u64 r13 = *((volatile u64 *)INIT_R13_ADDR);
	volatile u64 r14 = *((volatile u64 *)INIT_R14_ADDR);
	volatile u64 r15 = *((volatile u64 *)INIT_R15_ADDR);
	bool is_pass = true;

	if (!((r8 == 0x0) && (r9 == 0x0) && (r10 == 0x0) && (r11 == 0x0) &&
		(r12 == 0x0) && (r13 == 0x0) && (r14 == 0x0) && (r15 == 0x0))) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(46083);

	r8 = *((volatile u64 *)INIT_R8_ADDR);
	r9 = *((volatile u64 *)INIT_R9_ADDR);
	r10 = *((volatile u64 *)INIT_R10_ADDR);
	r11 = *((volatile u64 *)INIT_R11_ADDR);
	r12 = *((volatile u64 *)INIT_R12_ADDR);
	r13 = *((volatile u64 *)INIT_R13_ADDR);
	r14 = *((volatile u64 *)INIT_R14_ADDR);
	r15 = *((volatile u64 *)INIT_R15_ADDR);

	if (!((r8 == 0x0) && (r9 == 0x0) && (r10 == 0x0) && (r11 == 0x0) &&
		(r12 == 0x0) && (r13 == 0x0) && (r14 == 0x0) && (r15 == 0x0))) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);

}

#endif

/**
 * @brief case name: EAX register start-up value_001
 * Summary: Get eax register at BP start-up, the register's value shall be 0, and same with SDM definition.
 */
static void gp_rqmid_25067_eax_register_start_up(void)
{
	volatile u32 eax_startup = *((volatile u32 *)STARTUP_EAX_ADDR);
#ifdef IN_NON_SAFETY_VM
	eax_startup = *((volatile u32 *)STARTUP_EAX_ADDR_IN_NON_SAFETY);
#endif
	report("%s", eax_startup == 0x0, __FUNCTION__);
}

/**
 * @brief case name: EDX register start-up value_001
 * Summary: Get edx register at BP start-up, the register's value shall be 0 and same
 * with SDM definition.
 */
static void gp_rqmid_25083_edx_register_start_up(void)
{
	volatile u32 edx_startup = *((volatile u32 *)STARTUP_EDX_ADDR);
#ifdef IN_NON_SAFETY_VM
	edx_startup = *((volatile u32 *)STARTUP_EDX_ADDR_IN_NON_SAFETY);
#endif
	report("%s", edx_startup == 0, __FUNCTION__);

}

/**
 * @brief case name: EBX, ECX, EDI, EBP, ESP register start-up value_001
 * Summary: Get ebx,ecx,edi,ebp,esp at BP start-up, these registers'value
 * shall be 0 and same with SDM definition.
 */
static void gp_rqmid_25308_ebx_ecx_and_other_register_start_up(void)
{

	volatile u32 ebx_startup = *((volatile u32 *)STARTUP_EBX_ADDR);
	volatile u32 ecx_startup = *((volatile u32 *)STARTUP_ECX_ADDR);
	volatile u32 ebp_startup = *((volatile u32 *)STARTUP_EBP_ADDR);
	volatile u32 esp_startup = *((volatile u32 *)STARTUP_ESP_ADDR);
	volatile u32 edi_startup = *((volatile u32 *)STARTUP_EDI_ADDR);

#ifdef IN_NON_SAFETY_VM
	ebx_startup = *((volatile u32 *)STARTUP_EBX_ADDR_IN_NON_SAFETY);
	ecx_startup = *((volatile u32 *)STARTUP_ECX_ADDR_IN_NON_SAFETY);
	ebp_startup = *((volatile u32 *)STARTUP_EBP_ADDR_IN_NON_SAFETY);
	esp_startup = *((volatile u32 *)STARTUP_ESP_ADDR_IN_NON_SAFETY);
	edi_startup = *((volatile u32 *)STARTUP_EDI_ADDR_IN_NON_SAFETY);
#endif
	report("%s", (ebx_startup == 0x0) && (ecx_startup == 0x0) && (ebp_startup == 0x0) &&
			 (esp_startup == 0x0) && (edi_startup == 0x0), __FUNCTION__);

}

/**
 * @brief case name: EFLAGS register start-up value_001
 * Summary: Get eflags register at BP start-up, the bit shall be 00000002H and same
 * with SDM definition.
 */

static void gp_rqmid_25063_eflags_register_start_up(void)
{
	volatile u32 eflags_startup = *((volatile u32 *)STARTUP_EFLAGS_ADDR);
#ifdef IN_NON_SAFETY_VM
	eflags_startup = *((volatile u32 *)STARTUP_EFLAGS_ADDR_IN_NON_SAFETY);
#endif
	report("%s", eflags_startup == 0x00000002, __FUNCTION__);

}

/**
 * @brief case name: R8~R15 register start-up value_001
 * Summary: Get R8~R15 at BP start-up,  these register's value shall be 0H.
 */

void gp_rqmid_46098_r8_r15_register_start_up(void)
{
	volatile u64 r8 = *((volatile u64 *)STARTUP_R8_ADDR);
	volatile u64 r9 = *((volatile u64 *)STARTUP_R9_ADDR);
	volatile u64 r10 = *((volatile u64 *)STARTUP_R10_ADDR);
	volatile u64 r11 = *((volatile u64 *)STARTUP_R11_ADDR);
	volatile u64 r12 = *((volatile u64 *)STARTUP_R12_ADDR);
	volatile u64 r13 = *((volatile u64 *)STARTUP_R13_ADDR);
	volatile u64 r14 = *((volatile u64 *)STARTUP_R14_ADDR);
	volatile u64 r15 = *((volatile u64 *)STARTUP_R15_ADDR);

	bool is_pass = false;
	if ((r8 == 0x0) && (r9 == 0x0) && (r10 == 0x0) && (r11 == 0x0) &&
		(r12 == 0x0) && (r13 == 0x0) && (r14 == 0x0) && (r15 == 0x0)) {
		is_pass = true;
	} else {
		printf("r8=0x%lx, r9=0x%lx, r10=0x%lx, r11=0x%lx, r12=0x%lx, r13=0x%lx, r14=0x%lx, r15=0x%lx\n",
			r8, r9, r10, r11, r12, r13, r14, r15);
	}

	report("%s", is_pass, __FUNCTION__);
}

#else  /* #ifndef __x86_64__*/
/*
 * @brief case name: 32003: Data transfer instructions Support_Protected Mode_MOV_#GP_001.
 *
 * Summary: Under Protected Mode,
 * If an attempt is made to write a 1 to any reserved bit in CR4(CR4.R.W: 1),
 * executing MOV shall generate #GP.
 */
static void  gp_rqmid_32003_data_transfer_mov_gp_001(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf("***** Set the reserved bit in CR4, such as CR4[12] *****\n");
	unsigned long check_bit = 0;

	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32005: Data transfer instructions Support_Protected Mode_MOV_#GP_002.
 *
 * Summary: Under 64 bit Mode,
 *   If an attempt is made to leave IA-32e mode by clearing CR4PAE[bit 5](CR4.PAE: 0),
 *  executing MOV shall generate #GP.
 */
static void  gp_rqmid_32005_data_transfer_mov_gp_002(void)
{
	gp_trigger_func fun;
	bool ret;

	//printf("***** Using MOV instruction to set CR4.PCIDE[bit 17] to 1  *****\n");
	unsigned long check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_17));

	fun = (gp_trigger_func)write_cr4_checking;
	ret = test_for_exception(GP_VECTOR, fun, (void *)check_bit);

	report("%s #GP exception", ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31737: Binary arithmetic instructions Support_Protected Mode_IDIV_#DE_001.
 *
 * Summary: Under Protected Mode and CPL0,
 *   If the source operand (divisor) is 0(Divisor = 0: true),
 *   executing IDIV shall generate #DE.
 */
__unused static void idiv_de_32bit_001(void)
{
	int a = 1;
	int b = 0;
	asm volatile ("mov %[dividend], %%eax \n"
		"idiv %[divisor] \n"
		: : [dividend]"r"(a), [divisor]"r"(b) : );
}

static void  gp_rqmid_31737_binary_arithmetic_idiv_de_32bit_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)idiv_de_32bit_001;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 31956: Control transfer instructions Support_Protected Mode_BOUND_#BR_001.
 *
 * Summary:Under Protected Mode and CPL0,
 *  If the bounds test fails with BOUND range exceeded(Bound test: fail),
 *  executing BOUND shall generate #BR.
 */
static void bound_32bit_br_001(void)
{
	int t[2] = {0, 10};
	u32 r_a = 0xff;
	asm volatile ("bound %1, %[add]\n"
		: [add]"=m"(t[0])
		: "r"(r_a)
		: "memory");
}

static void  gp_rqmid_31956_data_transfer_bound_32bit_br_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)bound_32bit_br_001;
	ret = test_for_exception(BR_VECTOR, fun, NULL);

	report("%s",  ret == true, __FUNCTION__);
}

/*
 * @brief case name: 32179: Decimal arithmetic instructions Support_Protected Mode_AAM_#DE_001.
 *
 * Summary: Under Protected/Real Address Mode ,
 *  If an immediate value of 0 is used(imm8 = 0: hold),
 *  executing AAM shall generate #DE.
 */
static void aam_32bit_de_001(void)
{
	asm volatile ("aam $0\n"
		: : :);
}

static void  gp_rqmid_32179_decimal_arithmetic_aam_32bit_de_001(void)
{
	gp_trigger_func fun;
	bool ret;

	fun = (gp_trigger_func)aam_32bit_de_001;
	ret = test_for_exception(DE_VECTOR, fun, NULL);

	report("%s Execute Instruction: AAM", ret == true, __FUNCTION__);
}

#endif   /* #ifndef __x86_64__*/

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempt to write guest
 * IA32_MISC_ENABLE [bit 0]_001
 *
 * Summary: Execute WRMSR instruction to write IA32_MISC_ENABLE [bit 0] with different form old value
 * shall generate #GP(0).
 */
void misc_msr_rqmid_46546_wt_IA32_MISC_ENABLE_bit0_001(void)
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE) ^ MSR_IA32_MISC_ENABLE_FAST_STRING;
	report("%s", (wrmsr_checking(IA32_MISC_ENABLE, msr_ia32_misc_enable) == GP_VECTOR) &&
		(exception_error_code() == 0), __FUNCTION__);
}

#endif

static void print_case_list(void)
{
	printf("\t\t General purpose instructions feature case list:\n\r");
#ifdef IN_NATIVE
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 38975u,
	"Physical support for MSR access instructions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38977u,
	"Physical random number generate instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38978u,
	"Physical no-operation and undefined instruction support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38979u,
	"Physical processor identification instruction support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38980u,
	"The physical platform supports enhanced bitmap instructions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39004u,
	"The physical platform supports LZCNT instruction_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39014u,
	"Enter and leave instructions physical support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31238u,
	"Physical General data transfer instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31261u,
	"Physical control transfer instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31265u,
	"Physical string operations instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31269u,
	"Physical I/O instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31272u,
	"Physical Flag control instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31279u,
	"Physical address computation instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31282u,
	"Physical table lookup instructions support_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 31249u,
	"Physical logical instructions support_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 31275u,
	"Physical segment register instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31257u,
	"Physical bit and byte instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31242u,
	"Physical Binary arithmetic instructions support_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 31253u,
	"Physical shift and rotate instructions support_001.");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 39833u,
	"Physical control transfer instructions support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 39834u,
	"Physical General data transfer instructions support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 39835u,
	"Physical Flag control instructions support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 31245u,
	"Physical Decimal Arithmetic instructions support_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 39836u,
	"Physical segment register instructions support_002");
#endif
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 39006u,
		"Read 1 byte from I/O port with no peripheral_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39008u,
		"Read 2 byte from I/O port with no peripheral_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39009u,
		"Read 4 byte from I/O port with no peripheral_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 39055u,
		"Write to I/O port with no peripheral_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 25070u,
	"EAX registers init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25080u,
		"EDX register init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25061u,
		"EFLAGS register init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25468u,
		"EBX, ECX, ESI, EDI, EBP, ESP register init value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 46083u,
		"Set initial guest R8~R15 to 0H for virtual AP_001");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 25067u,
	"EAX register start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25083u,
		"EDX register start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25308u,
		"EBX, ECX, EDI, EBP, ESP register start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25063u,
		"EFLAGS register start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 46098u,
		"R8~R15 register start-up value_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 46546u,
		"Guarantee that the vCPU receives #GP(0) when a vCPU attempt to write guest IA32_MISC_ENABLE [bit 0]_001");
#endif
}

/*------------------------------Test case End!---------------------------------*/
int main(void)
{
	print_case_list();
	setup_vm();
	setup_idt();
	setup_ring_env();
#ifdef IN_NATIVE
#ifdef __x86_64__
	gp_rqmid_38975_msr_access_physical_support();
	gp_rqmid_38977_cpuid_random_number_generate_instructions_001();
	gp_rqmid_38978_no_operation__instructions_001();
	gp_rqmid_38979_processor_identification_instructions_001();
	gp_rqmid_38980_cpuid_eax07_ecx0_001();
	gp_rqmid_39004_cpuid_eax80000001_ecx0_001();
	gp_rqmid_39014_enter_and_leave_instructions();
	gp_rqmid_31279_address_computation_physical_support();
	gp_rqmid_31282_table_lookup_physical_support();
	gp_rqmid_31249_logical_instruction_physical_support();
	gp_rqmid_31275_segment_load_save_physical_support();
	gp_rqmid_31257_bit_and_byte_physical_support();
	gp_rqmid_31242_binary_arithmetic_physical_support();
	gp_rqmid_31253_shift_and_rotate_physical_support();
	gp_rqmid_31272_flag_control_physical_support();
	gp_rqmid_31261_control_transfer_physical_support();
	gp_rqmid_31265_string_operation_physical_support();
	gp_rqmid_31269_io_instructions_physical_support();
	gp_rqmid_31238_data_transfer_physical_support_001();
#else
	gp_rqmid_39833_control_transfer_physical_support_32bit();
	gp_rqmid_39834_data_transfer_physical_support_32bit();
	gp_rqmid_39835_flag_control_physical_support_32bit();
	gp_rqmid_31245_decimal_arithmetic_physical_support();
	gp_rqmid_39836_segment_load_save_physical_support_32bit();
#endif
#else
#ifdef __x86_64__
	/*--------------------------64 bit--------------------------*/
	gp_rqmid_31253_shift_and_rotate_physical_support();
	gp_rqmid_31282_table_lookup_physical_support();
	gp_rqmid_39004_cpuid_eax80000001_ecx0_001();
	gp_rqmid_31522_binary_arithmetic_div_normal_059();
	gp_rqmid_31315_data_transfer_mov_gp_007();
	do_at_ring1(gp_rqmid_31318_data_transfer_mov_gp_008, "");
	do_at_ring2(gp_rqmid_31321_data_transfer_mov_gp_009, "");
	do_at_ring3(gp_rqmid_31324_data_transfer_mov_gp_010, "");
	do_at_ring1(gp_rqmid_31336_data_transfer_mov_ud_002, "");
	do_at_ring2(gp_rqmid_31339_data_transfer_mov_ud_003, "");
	do_at_ring3(gp_rqmid_31342_data_transfer_mov_ud_004, "");
	gp_rqmid_31751_segment_instruction_lss_gp_132();
	gp_rqmid_31327_data_transfer_mov_gp_011();
	gp_rqmid_31330_data_transfer_mov_gp_012();
	gp_rqmid_31333_data_transfer_mov_ud_001();
	gp_rqmid_31378_data_transfer_mov_gp_015();
	gp_rqmid_32284_data_transfer_mov_gp_021();
	gp_rqmid_31271_data_transfer_cmpxchg16b_gp_060();
	gp_rqmid_31323_data_transfer_push_ud_017();
	gp_rqmid_32288_data_transfer_pop_ud_066();
	gp_rqmid_31495_binary_arithmetic_idiv_de_007();
	gp_rqmid_31655_control_transfer_jmp_ud_030();
	gp_rqmid_31286_control_transfer_call_ud_042();
	gp_rqmid_31379_control_transfer_iret_gp_097();
	gp_rqmid_32163_random_number_rdrand_ud_006();
	gp_rqmid_32191_ud_instruction_ud1_ud_010();
	gp_rqmid_31370_data_transfer_mov_gp_013();
	gp_rqmid_31385_data_transfer_mov_gp_017();
	gp_rqmid_31652_control_transfer_int_gp_115();
	gp_rqmid_31937_msr_access_rdsmr_gp_004();
	gp_rqmid_31939_msr_access_rdsmr_gp_005();
	gp_rqmid_31949_msr_access_wrmsr_gp_009();
	gp_rqmid_31953_msr_access_wrmsr_gp_011();
	gp_rqmid_31901_segment_instruction_lfs_gp_125();
	gp_rqmid_39006_Read_one_byte_from_IO_port_with_no_peripheral_001();
	gp_rqmid_39008_read_two_byte_from_IO_port_with_no_peripheral_001();
	gp_rqmid_39009_read_four_byte_from_IO_port_with_no_peripheral_001();
	gp_rqmid_39055_Write_to_port_with_no_peripheral();
#ifdef IN_NON_SAFETY_VM
	gp_rqmid_25070_eax_register_Following_init();
	gp_rqmid_25080_edx_register_Following_init();
	gp_rqmid_25061_eflags_register_Following_init();
	gp_rqmid_25468_ebx_ecx_and_other_register_Following_init();
	gp_rqmid_46083_r8_r15_following_init();
#endif
	gp_rqmid_25067_eax_register_start_up();
	gp_rqmid_25083_edx_register_start_up();
	gp_rqmid_25308_ebx_ecx_and_other_register_start_up();
	gp_rqmid_25063_eflags_register_start_up();
	gp_rqmid_46098_r8_r15_register_start_up();
	/*---64 bit end----*/
#else /* #ifndef __x86_64__ */
	/*--------------------------32 bit--------------------------*/
	gp_rqmid_32003_data_transfer_mov_gp_001();
	gp_rqmid_32005_data_transfer_mov_gp_002();
	gp_rqmid_31737_binary_arithmetic_idiv_de_32bit_001();
	gp_rqmid_31956_data_transfer_bound_32bit_br_001();
	gp_rqmid_32179_decimal_arithmetic_aam_32bit_de_001();
	/*--------------------------32 bit end ----------------------*/
#endif
	misc_msr_rqmid_46546_wt_IA32_MISC_ENABLE_bit0_001();
#endif
	return report_summary();
}
