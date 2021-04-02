asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"

#define TEST_SSE_EXCEED_64K_GP(ins, op) \
void test_##ins##_gp(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " %%ds:0xFFFF, " op "\n" "1:" \
				:  : [input_1] "m" (unsigned_128));	\
}

#define TEST_SSE_EXCEED_64K_GP_4(ins, op) \
void test_##ins##_gp(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " $0x1, %%ds:0xFFFF, " op "\n" "1:" \
				:  : [input_2] "m" (unsigned_128)); \
}

#define TEST_SSE_NM(ins, op) \
void test_##ins##_nm(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " %[input_1], " op "\n" "1:" \
				:  : [input_1] "m" (unsigned_128)); \
}

#define TEST_SSE_NM_4(ins, op) \
void test_##ins##_nm(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " $0x1, %[input_2], " op "\n" "1:" \
				:  : [input_2] "m" (unsigned_16)); \
}


#define TEST_SSE_NM_2(ins, op) \
void test_##ins##_nm(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " %[output]\n" "1:" \
				: [output] "=m" (unsigned_32) : ); \
}

#define TEST_SSE_MF(ins, op) \
void test_##ins##_mf(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " %[input_1], " op "\n" "1:" \
				:  : [input_1] "m" (unsigned_128)); \
}

#define TEST_SSE_UD(ins, op) \
void test_##ins##_ud(void *data) \
{ \
	asm volatile(ASM_TRY("1f") #ins " %[input_1], " op "\n" "1:" \
				:  : [input_1] "m" (unsigned_128)); \
}

__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;
extern u16 execption_inc_len;

void test_ROUNDSS_ud(void *data)
{
	__unused float r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
}

void test_ROUNDSS_xm(void *data)
{
	__unused float r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (r_a));
}

//Modified manually: add the conditional compilation for the size limit.
#ifdef PHASE_0
TEST_SSE_EXCEED_64K_GP(PADDD, "%%xmm1");
static __unused void sse_ra_instruction_0(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PADDD_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(SUBPD, "%%xmm1");
static __unused void sse_ra_instruction_1(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(UD_VECTOR, test_SUBPD_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_2(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	asm volatile(ASM_TRY("1f") "SUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(SUBPD, "%%xmm1");
static __unused void sse_ra_instruction_3(const char *msg)
{
	test_for_exception(NM_VECTOR, test_SUBPD_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(XORPS, "%%xmm1");
static __unused void sse_ra_instruction_5(const char *msg)
{
	test_for_exception(GP_VECTOR, test_XORPS_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(SUBPS, "%%xmm1");
static __unused void sse_ra_instruction_6(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(UD_VECTOR, test_SUBPS_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_7(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	asm volatile(ASM_TRY("1f") "SUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(SUBPS, "%%xmm1");
static __unused void sse_ra_instruction_8(const char *msg)
{
	test_for_exception(NM_VECTOR, test_SUBPS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "SUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(UCOMISD, "%%xmm1");
static __unused void sse_ra_instruction_10(const char *msg)
{
	test_for_exception(GP_VECTOR, test_UCOMISD_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(UCOMISD, "%%xmm1");
static __unused void sse_ra_instruction_11(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(UD_VECTOR, test_UCOMISD_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_12(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(UCOMISD, "%%xmm1");
static __unused void sse_ra_instruction_13(const char *msg)
{
	test_for_exception(NM_VECTOR, test_UCOMISD_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(UCOMISS, "%%xmm1");
static __unused void sse_ra_instruction_15(const char *msg)
{
	test_for_exception(GP_VECTOR, test_UCOMISS_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(UCOMISS, "%%xmm1");
static __unused void sse_ra_instruction_16(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(UD_VECTOR, test_UCOMISS_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_17(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	u64 r_a = 8.0 / 3.0;
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (r_a));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(UCOMISS, "%%xmm1");
static __unused void sse_ra_instruction_18(const char *msg)
{
	test_for_exception(NM_VECTOR, test_UCOMISS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(MOVSLDUP, "%%xmm1");
static __unused void sse_ra_instruction_20(const char *msg)
{
	test_for_exception(GP_VECTOR, test_MOVSLDUP_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(HSUBPS, "%%xmm1");
static __unused void sse_ra_instruction_21(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(UD_VECTOR, test_HSUBPS_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_22(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(HSUBPS, "%%xmm1");
static __unused void sse_ra_instruction_23(const char *msg)
{
	test_for_exception(NM_VECTOR, test_HSUBPS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") "HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
TEST_SSE_NM(PADDB, "%%xmm1");
static __unused void sse_ra_instruction_25(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PADDB_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_26(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PACKUSWB %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_NM(XORPS, "%%xmm1");
static __unused void sse_ra_instruction_27(const char *msg)
{
	test_for_exception(NM_VECTOR, test_XORPS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_28(const char *msg)
{
	asm volatile(ASM_TRY("1f") "XORPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(PMINUD, "%%xmm1");
static __unused void sse_ra_instruction_29(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PMINUD_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PMINSB, "%%xmm1");
static __unused void sse_ra_instruction_30(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PMINSB_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMINSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_NM_4(PINSRW, "%%xmm1");
static __unused void sse_ra_instruction_32(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PINSRW_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_33(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PINSRW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_ra_instruction_34(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_35(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM_4(ROUNDPS, "%%xmm1");
static __unused void sse_ra_instruction_36(const char *msg)
{
	test_for_exception(NM_VECTOR, test_ROUNDPS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_37(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDPS $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP_4(ROUNDSS, "%%xmm1");
static __unused void sse_ra_instruction_38(const char *msg)
{
	test_for_exception(GP_VECTOR, test_ROUNDSS_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PMOVZXWQ, "%%xmm1");
static __unused void sse_ra_instruction_39(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PMOVZXWQ_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_40(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXWQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_NM(MOVSLDUP, "%%xmm1");
static __unused void sse_ra_instruction_41(const char *msg)
{
	test_for_exception(NM_VECTOR, test_MOVSLDUP_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_42(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_NM_2(STMXCSR, "%[output]");
static __unused void sse_ra_instruction_43(const char *msg)
{
	test_for_exception(NM_VECTOR, test_STMXCSR_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_44(const char *msg)
{
	asm volatile(ASM_TRY("1f") "STMXCSR %[output]\n" "1:"
				: [output] "=m" (unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(MOVDDUP, "%%xmm1");
static __unused void sse_ra_instruction_45(const char *msg)
{
	test_for_exception(GP_VECTOR, test_MOVDDUP_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(MOVDDUP, "%%xmm1");
static __unused void sse_ra_instruction_46(const char *msg)
{
	test_for_exception(NM_VECTOR, test_MOVDDUP_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_47(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(PSHUFB, "%%mm1");
static __unused void sse_ra_instruction_48(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PSHUFB_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_MF(PSHUFB, "%%mm1");
static __unused void sse_ra_instruction_49(const char *msg)
{
	test_for_exception(MF_VECTOR, test_PSHUFB_mf, NULL);
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_2
TEST_SSE_UD(PSHUFB, "%%mm1");
static __unused void sse_ra_instruction_50(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PSHUFB_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PSHUFB, "%%mm1");
static __unused void sse_ra_instruction_51(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PSHUFB_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_52(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(PSIGNW, "%%xmm1");
static __unused void sse_ra_instruction_53(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PSIGNW_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PSIGNW, "%%xmm1");
static __unused void sse_ra_instruction_54(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PSIGNW_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_55(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSIGNW %%xmm2, %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(PSADBW, "%%mm1");
static __unused void sse_ra_instruction_56(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PSADBW_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_MF(PSADBW, "%%mm1");
static __unused void sse_ra_instruction_57(const char *msg)
{
	test_for_exception(MF_VECTOR, test_PSADBW_mf, NULL);
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(PSADBW, "%%mm1");
static __unused void sse_ra_instruction_58(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PSADBW_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PSADBW, "%%mm1");
static __unused void sse_ra_instruction_59(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PSADBW_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_60(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP_4(PCMPISTRM, "%%xmm1");
static __unused void sse_ra_instruction_61(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PCMPISTRM_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_NM_4(PCMPISTRM, "%%xmm1");
static __unused void sse_ra_instruction_62(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PCMPISTRM_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_63(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

TEST_SSE_EXCEED_64K_GP(PSUBQ, "%%mm1");
static __unused void sse_ra_instruction_64(const char *msg)
{
	test_for_exception(GP_VECTOR, test_PSUBQ_gp, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

TEST_SSE_MF(PSUBQ, "%%mm1");
static __unused void sse_ra_instruction_65(const char *msg)
{
	test_for_exception(MF_VECTOR, test_PSUBQ_mf, NULL);
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

TEST_SSE_UD(PSUBQ, "%%mm1");
static __unused void sse_ra_instruction_66(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PSUBQ_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

TEST_SSE_NM(PSUBQ, "%%mm1");
static __unused void sse_ra_instruction_67(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PSUBQ_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_68(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_ra_instruction_69(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(XM_VECTOR, test_ROUNDSS_ud, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_70(const char *msg)
{
	u16 val = 0x01;
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	memcpy(&unsigned_128, &val, sizeof(u16));
	test_for_exception(XM_VECTOR, test_ROUNDSS_xm, NULL);
	memset(&unsigned_128, 0x00, sizeof(unsigned_128));
	report("%s", (exception_vector() == XM_VECTOR), __FUNCTION__);
}

TEST_SSE_NM_4(ROUNDSS, "%%xmm1");
static __unused void sse_ra_instruction_71(const char *msg)
{
	test_for_exception(NM_VECTOR, test_ROUNDSS_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_ra_instruction_72(const char *msg)
{
	asm volatile(ASM_TRY("1f") "ROUNDSS $0x1, %%xmm2, %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_32));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_0
static __unused void sse_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_15, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_20, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_21, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void sse_ra_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_29, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_34, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_36(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_37, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_38, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_39(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_39, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_40(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_40, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_41(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_41, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_42(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_42, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_43(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_43, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_44(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_44, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_45(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_45, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_46(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_46, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_47(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_47, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_48(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_48, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_49(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_ra_instruction_49, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_2
static __unused void sse_ra_50(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_ra_instruction_50, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_51(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_51, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_52(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_52, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_53(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_53, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_54(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_54, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_55(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_55, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_56(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_56, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_57(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_ra_instruction_57, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_58(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_ra_instruction_58, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_59(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_59, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_60(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_60, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_61(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_61, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_62(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_62, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_63(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_63, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_64(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(sse_ra_instruction_64, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_65(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_ra_instruction_65, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_66(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_ra_instruction_66, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_67(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_67, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_68(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_68, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_69(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_0();
	do_at_ring0(sse_ra_instruction_69, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_70(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_hold();
	condition_CR4_OSXMMEXCPT_1();
	do_at_ring0(sse_ra_instruction_70, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_71(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_1();
	do_at_ring0(sse_ra_instruction_71, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_ra_72(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_exceed_64K_not_hold();
	condition_unmasked_SIMD_not_hold();
	condition_CR0_TS_0();
	do_at_ring0(sse_ra_instruction_72, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}
//Modified manually: add the conditional compilation for the size limit.
#endif

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	asm volatile("fninit");
	#ifdef PHASE_0
	sse_ra_0();
	sse_ra_1();
	sse_ra_2();
	sse_ra_3();
	sse_ra_4();
	sse_ra_5();
	sse_ra_6();
	sse_ra_7();
	sse_ra_8();
	sse_ra_9();
	sse_ra_10();
	sse_ra_11();
	sse_ra_12();
	sse_ra_13();
	sse_ra_14();
	sse_ra_15();
	sse_ra_16();
	sse_ra_17();
	sse_ra_18();
	sse_ra_19();
	sse_ra_20();
	sse_ra_21();
	sse_ra_22();
	sse_ra_23();
	sse_ra_24();
	#endif
	#ifdef PHASE_1
	sse_ra_25();
	sse_ra_26();
	sse_ra_27();
	sse_ra_28();
	sse_ra_29();
	sse_ra_30();
	sse_ra_31();
	sse_ra_32();
	sse_ra_33();
	sse_ra_34();
	sse_ra_35();
	sse_ra_36();
	sse_ra_37();
	sse_ra_38();
	sse_ra_39();
	sse_ra_40();
	sse_ra_41();
	sse_ra_42();
	sse_ra_43();
	sse_ra_44();
	sse_ra_45();
	sse_ra_46();
	sse_ra_47();
	sse_ra_48();
	sse_ra_49();
	#endif
	#ifdef PHASE_2
	sse_ra_50();
	sse_ra_51();
	sse_ra_52();
	sse_ra_53();
	sse_ra_54();
	sse_ra_55();
	sse_ra_56();
	sse_ra_57();
	sse_ra_58();
	sse_ra_59();
	sse_ra_60();
	sse_ra_61();
	sse_ra_62();
	sse_ra_63();
	sse_ra_64();
	sse_ra_65();
	sse_ra_66();
	sse_ra_67();
	sse_ra_68();
	sse_ra_69();
	sse_ra_70();
	sse_ra_71();
	sse_ra_72();
	#endif

	return report_summary();
}
