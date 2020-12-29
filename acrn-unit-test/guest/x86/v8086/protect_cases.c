
void print_case_list(void)
{
	printf("v8086 feature case list:\n\t");
	printf("\t\tCase ID:%d case name:%s\n\t", 33866u, "write CR4 and the new guest CR4.VME is 1H_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37867u, "write CR4 and the new guest CR4.PVI is 1H_001");
#ifndef IN_NATIVE
	printf("\t\tCase ID:%d case name:%s\n\t", 33886u, "reads CPUID.01H_003");
#endif
	printf("\t\tCase ID:%d case name:%s\n\t", 33813u, "Virtual-8086 mode interrupt handling to any VM_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37609u, "Virtual-8086 mode interrupt handling to any VM_002");
	printf("\t\tCase ID:%d case name:%s\n\t", 37610u, "Virtual-8086 mode interrupt handling to any VM_003");
	printf("\t\tCase ID:%d case name:%s\n\t", 37611u, "Virtual-8086 mode interrupt handling to any VM_004");
	printf("\t\tCase ID:%d case name:%s\n\t", 33898u, "hypervisor shall expose v8086 paging VM_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 33890u, "hypervisor shall expose v8086 I/O protection VM_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37666u, "hypervisor shall expose v8086 I/O protection VM_002");
	printf("\t\tCase ID:%d case name:%s\n\t", 37673u, "hypervisor shall expose v8086 I/O protection VM_003");
	printf("\t\tCase ID:%d case name:%s\n\t", 37674u, "hypervisor shall expose v8086 I/O protection VM_004");
	printf("\t\tCase ID:%d case name:%s\n\t", 37675u, "hypervisor shall expose v8086 I/O protection VM_005");
	printf("\t\tCase ID:%d case name:%s\n\t", 37676u, "hypervisor shall expose v8086 I/O protection VM_006");
	printf("\t\tCase ID:%d case name:%s\n\t", 37677u, "hypervisor shall expose v8086 I/O protection VM_007");
	printf("\t\tCase ID:%d case name:%s\n\t", 37857u, "Real and virtual-8086 mode execution environment_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37858u, "Real and virtual-8086 mode execution environment_002");
	printf("\t\tCase ID:%d case name:%s\n\t", 37859u, "Real and virtual-8086 mode execution environment_003");
	printf("\t\tCase ID:%d case name:%s\n\t", 37864u, "Real and virtual-8086 mode execution environment_004");
	printf("\t\tCase ID:%d case name:%s\n\t", 37865u, "Real and virtual-8086 mode execution environment_005");
	printf("\t\tCase ID:%d case name:%s\n\t", 37866u, "Real and virtual-8086 mode execution environment_006");
	printf("\t\tCase ID:%d case name:%s\n\t", 37867u, "Real and virtual-8086 mode execution environment_007");
	printf("\t\tCase ID:%d case name:%s\n\t", 37868u, "Real and virtual-8086 mode execution environment_008");
}

void v8086_rqmid_33898_init(void)
{
	u8 *test_page;
	test_page = (u8 *) 0x800;
	test_page[0] = 0xef;
	test_page[1] = 0xbe;
}


