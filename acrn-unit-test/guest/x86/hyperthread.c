#include "libcflat.h"
#include "desc.h"
#include "processor.h"

/*
 * @brief case name: A vCPU of non-safety VM reads CPUID.(EAX=0BH,ECX=1H) EAX[bit 4:0]_001
 *
 * Summary: when a vCPU of non-safety VM reads CPUID.(EAX=0BH,ECX=1H) EAX[bit 4:0] The value
 * of EAX[bit 4:0] is 0x2H.
 *
 */
static __unused void hyper_rqmid_37644_eax0B_ecx1_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x01);
	temp = cpuid0fh.a & 0x001F;
	report("%s", (temp == 2), __FUNCTION__);
}

/*
 * @brief case name: A vCPU of safety VM reads CPUID.(EAX=0BH,ECX=1H) EAX[bit 4:0]_001
 *
 * Summary: when a vCPU of safety VM reads CPUID.(EAX=0BH,ECX=1H) EAX[bit 4:0] The value of EAX[bit 4:0] is 0x0H.
 *
 */
static __unused void hyper_rqmid_37643_eax0B_ecx1_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x01);
	temp = cpuid0fh.a & 0x001F;

	report("%s", (temp == 0), __FUNCTION__);
}

/*
 *@brief case name: read CPUID.(EAX=0BH,ECX=0H) EAX[bit 4:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=0H), ACRN hypervisor shall write 0H to EAX[bit 4:0].
 *
 */
static __unused void hyper_rqmid_37654_cpuid_eax0B_ecx0_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x00);
	temp = cpuid0fh.a & 0x001F;

	report("%s", (temp == 0), __FUNCTION__);
}

/*
 * @brief case name: read CPUID.(EAX=0BH,ECX>1H) EAX[bit 4:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX>1H), ACRN hypervisor shall write 0H to EAX[bit 4:0].
 *
 */
static __unused void hyper_rqmid_37655_cpuid_eax0B_ecx2_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x02);
	temp = cpuid0fh.a & 0x001F;

	report("%s", (temp == 0), __FUNCTION__);
}


/*
 * @brief case name:read basic leaf_001
 *
 * Summary: When a vCPU attempts to read CPUID.0H, ACRN hypervisor shall write 16H to EAX.
 *
 */
static __unused void hyper_rqmid_37656_cpuid_eax0(void)
{
	report("%s", (cpuid(0x0).a == 0x16), __FUNCTION__);
}


/*
 *@brief case name: read CPUID.(EAX=0BH,ECX>1H) ECX[bit 15:8]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX>1H), ACRN hypervisor shall write 0H to ECX[bit 15:8].
 *
 */
static __unused void hyper_rqmid_37658_cpuid_eax0B_ecx2_001(void)
{
	report("%s", ((((cpuid_indexed(0x0B, 0x02).c) & 0xFF00) >> 8) == 0), __FUNCTION__);
}


/*
 * @brief case name: read CPUID.(EAX=0BH,ECX>1H) EBX[bit 15:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX>1H), ACRN hypervisor shall write 3H to EBX[bit 15:0].

 */
static __unused void hyper_rqmid_37662_cpuid_eax0B_ecx2_001(void)
{
	printf("%s:cpuid_indexed(0x0B, 0x02).b=%#x\n", __FUNCTION__, cpuid_indexed(0x0B, 0x02).b);
	report("%s", (((cpuid_indexed(0x0B, 0x02).b) & 0xFFFF) == 0x03), __FUNCTION__);
}


/*
 *@brief case name: read CPUID.(EAX=0BH,ECX>1H) ECX[bit 7:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX>1H), The value of ECX[bit 7:0]  Keep unchanged.
 *
 */
static __unused void hyper_rqmid_37672_cpuid_eax0B_ecx2_001(void)
{
	report("%s", (((cpuid_indexed(0x0B, 0x02).c) & 0x00FF) == 2), __FUNCTION__);
}

/*
 *@brief case name: read CPUID.(EAX=0BH,ECX=1H) ECX[bit 7:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=1H), The value of ECX[bit 7:0]  Keep unchanged.
 *
 */
static __unused void hyper_rqmid_37678_cpuid_eax0B_ecx1_001(void)
{
	report("%s", (((cpuid_indexed(0x0B, 0x01).c) & 0x00FF) == 1), __FUNCTION__);
}


/*
 *@brief case name: read CPUID.(EAX=0BH,ECX=0H) ECX[bit 7:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=0H), The value of ECX[bit 7:0]  Keep unchanged.
 *
 */
static __unused void hyper_rqmid_37679_cpuid_eax0B_ecx0_001(void)
{
	report("%s", (((cpuid_indexed(0x0B, 0x00).c) & 0x00FF) == 0), __FUNCTION__);
}


/*
 * @brief case name: read CPUID.(EAX=0BH,ECX=1H) ECX[bit 15:8]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=1H), ACRN hypervisor shall write 2 to ECX[bit 15:8].
 *
 */
static __unused void hyper_rqmid_37659_cpuid_eax0B_ecx1_001(void)
{
	report("%s", ((((cpuid_indexed(0x0B, 0x01).c) & 0xFF00) >> 8) == 0x02), __FUNCTION__);
}

/*
 * @brief case name: read CPUID.(EAX=0BH,ECX=0H) ECX[bit 15:8]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=0H), ACRN hypervisor shall write 1 to ECX[bit 15:8].
 *
 */
static __unused void hyper_rqmid_37660_cpuid_eax0B_ecx0_001(void)
{
	report("%s", ((((cpuid_indexed(0x0B, 0x00).c) & 0xFF00) >> 8) == 1), __FUNCTION__);
}


/*
 * @brief case name:A vCPU of non-safety VM reads CPUID.01H EDX[bit 28]_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H, ACRN hypervisor shall write 1 to guest EDX[28].
 */
static __unused void hyper_rqmid_29599_cpuid(void)
{
	report("%s", (((cpuid(0x1).d) >> 28) & 1) == 1, __FUNCTION__);
}

/*
 * @brief case name:A vCPU of safety VM reads CPUID.01H EDX[bit 28]_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H, ACRN hypervisor shall write 0 to guest EDX[28].
 */
static __unused void hyper_rqmid_37640_cpuid(void)
{
	report("%s", (((cpuid(0x1).d) >> 28) & 1) == 0, __FUNCTION__);
}

/*
 * @brief case name:Read CPUID.1H:EBX[bit 23:16]_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H, ACRN hypervisor shall write 10H to EBX[bit 23:16].
 */
static __unused void hyper_rqmid_37642_cpuid(void)
{
	report("%s", ((((cpuid(0x1).b) & 0xFF0000) >> 16) == 0x10), __FUNCTION__);
}

/*
 * @brief case name:Read CPUID.(EAX=4H,ECX=0H) EAX[bit 31:16]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=4H,ECX=0H), ACRN hypervisor shall write 7H to EAX[bit 31:16].
 */
static __unused void hyper_rqmid_37645_cpuid_eax04_ecx0_001()
{
	printf("%s:cpuid(0x4).a=%#x\n", __FUNCTION__, cpuid(0x4).a);
	report("%s", ((((cpuid(0x4).a) & 0xFFFF0000) >> 16) == 0x7), __FUNCTION__);
}

/*
 * @brief case name:Read CPUID.(EAX=0BH,ECX=0H) EBX[bit 15:0]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=0BH,ECX=0H), ACRN hypervisor shall write 1H to EBX[bit 15:0].
 */
static __unused void hyper_rqmid_37646_cpuid_eax0B_ecx0_001()
{
	report("%s", (((cpuid(0xB).b) & 0xFFFF) == 0x1), __FUNCTION__);
}

/*
 * @brief case name: A vCPU of non-safety VM reads CPUID.(EAX=0BH,ECX=1H) EBX[bit 15:0]_001
 *
 * Summary: When a vCPU of non-safety VM attempts to read CPUID.(EAX=0BH,ECX=1H), ACRN hypervisor shall write
 * 3H to EBX[bit 15:0].
 *
 */
static __unused void hyper_rqmid_37647_eax0B_ecx1_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x01);
	temp = cpuid0fh.b & 0xFFFF;

	report("%s", (temp == 3), __FUNCTION__);
}

/*
 * @brief case name: A vCPU of safety VM reads CPUID.(EAX=0BH,ECX=1H) EBX[bit 15:0]_001
 *
 * Summary: When a vCPU of safety VM attempts to read CPUID.(EAX=0BH,ECX=1H), ACRN hypervisor shall write
 * 1H to EBX[bit 15:0].
 */
static __unused void hyper_rqmid_37650_eax0B_ecx1_001(void)
{
	int  temp;
	struct cpuid cpuid0fh = {0};
	cpuid0fh = cpuid_indexed(0x0B, 0x01);
	temp = cpuid0fh.b & 0xFFFF;

	report("%s", (temp == 1), __FUNCTION__);
}

static void print_case_list(void)
{
	printf("Hyper_Threading feature case list:\n\r");
#ifdef IN_NATIVE

#else
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29599, "When a vCPU attempts to read CPUID.01H_001");
	printf("\t Case ID:%d case name:%s\n\r", 37644u, "A vCPU of non-safety VM reads CPUID.(EAX=0BH,ECX=1H)\
	EAX[bit 4:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37647u, "A vCPU of non-safety VM reads CPUID.(EAX=0BH,ECX=1H)\
	EBX[bit 15:0]_001");
#endif

#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 37643u, "A vCPU of safety VM reads CPUID.(EAX=0BH,ECX=1H)\
	EAX[bit 4:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37640u, "A vCPU of safety VM reads CPUID.01H EDX[bit 28]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37650u, "A vCPU of safety VM reads CPUID.(EAX=0BH,ECX=1H)\
	EBX[bit 15:0]_001");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 37642u, "Read CPUID.1H:EBX[bit 23:16]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37645u, "Read CPUID.(EAX=4H,ECX=0H) EAX[bit 31:16]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37646u, "Read CPUID.(EAX=0BH,ECX=0H) EBX[bit 15:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37654u, "read CPUID.(EAX=0BH,ECX=0H) EAX[bit 4:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37655u, "read CPUID.(EAX=0BH,ECX>1H) EAX[bit 4:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37656u, "read basic leaf_001");
	printf("\t Case ID:%d case name:%s\n\r", 37658u, "read CPUID.(EAX=0BH,ECX>1H) ECX[bit 15:8]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37659u, "read CPUID.(EAX=0BH,ECX=1H) ECX[bit 15:8]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37660u, "read CPUID.(EAX=0BH,ECX=0H) ECX[bit 15:8]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37662u, "read CPUID.(EAX=0BH,ECX>1H) EBX[bit 15:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37672u, "read CPUID.(EAX=0BH,ECX>1H) ECX[bit 7:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37678u, "read CPUID.(EAX=0BH,ECX=1H) ECX[bit 7:0]_001");
	printf("\t Case ID:%d case name:%s\n\r", 37679u, "read CPUID.(EAX=0BH,ECX=0H) ECX[bit 7:0]_001");
#endif
}

int main(void)
{
	setup_idt();
	setup_vm();

	print_case_list();

#ifdef IN_NATIVE

#else
#ifdef IN_NON_SAFETY_VM
	hyper_rqmid_29599_cpuid();
	hyper_rqmid_37644_eax0B_ecx1_001();
	hyper_rqmid_37647_eax0B_ecx1_001();
#endif

#ifdef IN_SAFETY_VM
	hyper_rqmid_37643_eax0B_ecx1_001();
	hyper_rqmid_37640_cpuid();
	hyper_rqmid_37650_eax0B_ecx1_001();
#endif

	hyper_rqmid_37642_cpuid();
	hyper_rqmid_37645_cpuid_eax04_ecx0_001();
	hyper_rqmid_37646_cpuid_eax0B_ecx0_001();
	hyper_rqmid_37654_cpuid_eax0B_ecx0_001();
	hyper_rqmid_37655_cpuid_eax0B_ecx2_001();
	hyper_rqmid_37656_cpuid_eax0();
	hyper_rqmid_37658_cpuid_eax0B_ecx2_001();
	hyper_rqmid_37659_cpuid_eax0B_ecx1_001();
	hyper_rqmid_37660_cpuid_eax0B_ecx0_001();
	hyper_rqmid_37662_cpuid_eax0B_ecx2_001();
	hyper_rqmid_37672_cpuid_eax0B_ecx2_001();
	hyper_rqmid_37678_cpuid_eax0B_ecx1_001();
	hyper_rqmid_37679_cpuid_eax0B_ecx0_001();
#endif
	return report_summary();
}
