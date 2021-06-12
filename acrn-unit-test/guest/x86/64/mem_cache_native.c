/*
 * Cache control Native Test Cases.
 *
 * SPDX-License-Identifier: GPL-2.0
 */
int pci_mem_get(void *pci_mem_address);

static long target;

void wrmsr_p(void *data)
{
	struct msr_data *wp = data;
	wrmsr(wp->index, wp->value);
}

/**
 * @brief case name:Cache parameters in CPUID.2H_001
 *
 * Summary: The CPUID.2H on the physical platform shall report that cache parameters are queried using CPUID.4H,
 * CPUID.02.EBX[bit 0-7] should be FFH(CPUID leaf 2 does not report cache descriptor information,
 * use CPUID leaf 4 to query cache parameters)
 */
void cache_rqmid_24443_native_cache_parameters_in_cpuid02(void)
{
	struct cpuid cpuid1;

	cpuid1 = cpuid(2);
	int expected;
	expected = cpuid1.b & 0xFF;

	report("%s Cache parameters in CPUID.2H (%s)\n", expected == 0xFF, __FUNCTION__,
		    expected == 0xFF ? "present" : "ABSENT");
}

/**
 * @brief case name:Physical PREFETCHW instruction_001
 *
 * Summary: PREFETCHW instruction shall be available on the physical platform.
 * On native, CPUID.(EAX=80000001H,ECX=0).ECX.PREFETCHW field[bit 8] should be 1H(support)
 * execute prefetchw instruction no exception.
 */
void cache_rqmid_24445_native_PREFETCHW_native(void)
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x80000001, 0);
	int expected;

	expected = cpuid.c & (1U << 8); /* PREFETCHW */
	/* clwb (%rbx): */
	asm volatile("prefetchw (%0)" : : "b"(&target));
	report("%s prefetchw (%s)\n", expected == 1, __FUNCTION__, expected ? "present" : "ABSENT");
}

/**
 * @brief case name:Cache control CLFLUSH instruction_002
 *
 * Summary: ACRN hypervisor shall expose cache invalidation instructions to any VM,
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Order read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute CLFLUSH instruction reflush cache,
 * 5. Order read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average value of each tsc_delay.
 * average should in CLFLUSH benchmark interval (average-3 *stdev, average+3*stdev), 
 * CLFLUSH benchmark get from native,refer CLFLUSH test on native
 */
void cache_rqmid_25314_native_clflush(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = read_mem_cache_test(cache_l3_size);
		clflush_all_line(cache_l3_size);
		tsc_delay_after[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay_after[i] - tsc_delay_before[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native clflush read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Cache control CLFLUSHOPT instruction_002
 *
 * Summary: ACRN hypervisor shall expose cache invalidation instructions to any VM,
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Order read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute CLFLUSHOPT instruction reflush cache,
 * 5. Order read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average value of each tsc_delay.
 * average should in CLFLUSHOPT benchmark interval (average-3 *stdev, average+3*stdev), 
 * CLFLUSHOPT benchmark get from native,refer CLFLUSHOPT test on native
 */
void cache_rqmid_25472_native_clflushopt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = read_mem_cache_test(cache_l3_size);
		clflushopt_all_line(cache_l3_size);
		tsc_delay_after[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay_after[i] - tsc_delay_before[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native clflushopt read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Cache control cache invalidation instructions_002
 *
 * Summary: ACRN hypervisor shall expose cache invalidation instructions to any VM,
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Order read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute wbinvd instruction reflush cache,
 * 5. Order read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average value of each tsc_delay.
 * average should in wbinvd benchmark interval (average-3 *stdev, average+3*stdev), 
 * wbinvd benchmark get from native,refer wbinvd test on native
 */
void cache_rqmid_26891_native_invalidation(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = read_mem_cache_test(cache_l3_size);
		asm_wbinvd();
		tsc_delay_after[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay_after[i] - tsc_delay_before[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native wbinvd read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WB_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26913_native_l1_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WB read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WB_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26992_native_l1_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WB write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26917_native_l1_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26919_native_l1_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WT_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26921_native_l1_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WT read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WT_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26923_native_l1_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WT write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WP_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26925_native_l1_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WP read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_WP_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26927_native_l1_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 WP write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_UC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26929_native_l1_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 UC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L1_UC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a1 with 0x1000 elements, each of size is 8 bytes.
 * 2. Modify a1 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26931_native_l1_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L1 UC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WB_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26933_native_l2_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WB read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WB_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26935_native_l2_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WB write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26937_native_l2_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26939_native_l2_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WT_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26941_native_l2_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WT read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WT_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26943_native_l2_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WT write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WP_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26945_native_l2_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WP read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_WP_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26947_native_l2_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 WP write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_UC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26948_native_l2_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 UC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L2_UC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a2 with 0x8000 elements, each of size is 8 bytes.
 * 2. Modify a2 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a2 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26949_native_l2_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L2 UC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WB_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26950_native_l3_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WB read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WB_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26951_native_l3_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WB write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26952_native_l3_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26953_native_l3_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WT_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26954_native_l3_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WT read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WT_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26955_native_l3_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WT write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WP_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26956_native_l3_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WP read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_WP_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26957_native_l3_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 WP write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_UC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26958_native_l3_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 UC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native L3_UC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a3 with 0x100000 elements, each of size is 8 bytes.
 * 2. Modify a3 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a3 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26959_native_l3_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native L3 UC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WB_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26960_native_over_l3_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WB read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WB_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WB.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26961_native_over_l3_wb(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WB write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26962_native_over_l3_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26963_native_over_l3_wc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WT_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26964_native_over_l3_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WT read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WT_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WT.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26965_native_over_l3_wt(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WT write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WP_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26966_native_over_l3_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WP read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_WP_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to WP.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26967_native_over_l3_wp(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);
	tsc_delay_delta_total = 0;
	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);

	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 WP write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_UC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order read a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26968_native_over_l3_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 UC read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Memory type test on native over L3_UC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 *
 * 1. Allocate an array a4 with 0x200000 elements, each of size is 8 bytes.
 * 2. Modify a4 array memory type to UC.
 * 3. Refresh the cache with the wbinvd instruction.
 * 4. Order write the a4 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average and stdev for the next 40 times.
 */
void cache_rqmid_26969_native_over_l3_uc(void)
{
	int i;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native over L3 UC write average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_UC_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to UC.
 * 3. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27027_native_map_to_device_linear_uc(void)
{
	u64 *tmp = cache_test_array;
	int ret = pci_mem_get((void *)&cache_test_array);
	if (ret != 0) {
		cache_test_array = tmp;
		printf("%s can't get pci mem address\n", __FUNCTION__);
		return;
	}
	else {
		cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WP
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK4, 4096);

		/*Cache size 4k*/
		asm_mfence_wbinvd();
		tsc_delay_delta_total = cache_order_read(CACHE_DEVICE_4K_READ, cache_4k_size);
		asm_mfence_wbinvd();
		printf("native device 4k read average is %ld\r\n", tsc_delay_delta_total);
	}
	cache_test_array = tmp;
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_UC_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to UC.
 * 3. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27028_native_map_to_device_linear_uc(void)
{
	u64 *tmp = cache_test_array;
	int ret = pci_mem_get((void *)&cache_test_array);
	if (ret != 0) {
		cache_test_array = tmp;
		printf("%s can't get pci mem address\n", __FUNCTION__);
		return;
	}
	else {
		/* 601*8=0x3008*/
		// cache_test_array += 601;

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WP
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK4, 4096);

		/*Cache size 4k*/
		tsc_delay_delta_total = cache_order_write(CACHE_DEVICE_4K_WRITE, cache_4k_size);
		asm_mfence_wbinvd();
		printf("native device 4k write average is %ld\r\n", tsc_delay_delta_total);
	}

	cache_test_array = tmp;
}

/**
 * @brief case name:Physical CLFLUSHOPT instruction_001
 *
 * Summary: CLFLUSHOPT instruction shall be available on the physical platform.
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Disorder read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute clflushopt instruction reflush all cache,
 * 5. Disorder read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average and stdev value of each tsc_delay.
 */
void cache_rqmid_27487_native_clflushopt(void)
{
	int i, diff;

	tsc_delay_delta_total = 0;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = disorder_access_size(cache_l3_size);
		clflushopt_all_line(cache_l3_size);
		tsc_delay_after[i] = disorder_access_size(cache_l3_size);
		diff = tsc_delay_after[i] - tsc_delay_before[i];
		tsc_delay_delta_total += diff;
		debug_print("%d\n", diff);
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native clflushopt dis read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Physical cache invalidation instructions_001
 *
 * Summary: Cache invalidation instructions shall be available on the physical platform.
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Disorder read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute wbinvd instruction reflush all cache,
 * 5. Disorder read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average and stdev value of each tsc_delay.
 */
void cache_rqmid_27488_native_invalidation(void)
{
	int i, diff;

	tsc_delay_delta_total = 0;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = disorder_access_size(cache_l3_size);
		asm_wbinvd();
		tsc_delay_after[i] = disorder_access_size(cache_l3_size);
		diff = tsc_delay_after[i] - tsc_delay_before[i];
		tsc_delay_delta_total += diff;
		debug_print("%d\n", diff);
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native wbinvd dis read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Physical CLFLUSH instruction_001
 *
 * Summary: CLFLUSHOPT instruction shall be available on the physical platform.
 * CPUID.1.CLFLUSH flag Field[bit 19] should be 01H(support)
 * execute clflush instruction no exception.
 * 1. Allocate an array a3 with 0x100000 elements, each of size 8 bytes,
 * 2. Set a3 array memory type is wb,
 * 3. Disorder read a3 array fill L3 cache, record tsc_delay,
 * 4. Execute clflush instruction reflush all cache,
 * 5. Disorder read a3 array again, record tsc_delay.
 * repeat steps 3 to 5 40 times and calculate the average and stdev value of each tsc_delay.
 */
void cache_rqmid_27489_native_clflush(void)
{
	int i, diff;

	tsc_delay_delta_total = 0;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);/*index 0 is wb*/

	/*fill cache*/
	read_mem_cache_test(cache_l3_size);
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay_before[i] = disorder_access_size(cache_l3_size);
		clflush_all_line(cache_l3_size);
		tsc_delay_after[i] = disorder_access_size(cache_l3_size);
		diff = tsc_delay_after[i] - tsc_delay_before[i];
		tsc_delay_delta_total += diff;
		debug_print("%d\n", diff);
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;

	printf("native clflush dis read average is %ld\r\n", tsc_delay_delta_total);
}

/**
 * @brief case name:Physical has L1D cache_001
 *
 * Summary: The physical platform shall have level-1 data cache.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 */
void cache_rqmid_27965_native_has_l1d_cache(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 cache indexing_001
 *
 * Summary: Level-3 cache on the physical platform shall use complex cache indexing.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H (Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H (Level 3)
 * CPUID.(EAX=04H,ECX=3):EDX.Complex Cache Indexing Field[bit 2] shoud be
 * 1H(A complex function is used to index the cache, potentially using all address bits)
 */
void cache_rqmid_27968_native_l3_cache_indexing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 &&	get_bit_range(id.d, 2, 2) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I number of sets_001
 *
 * Summary: The number of sets of level-1 instruction cache on the physical platform shall be 64.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):ECX.Number of Sets Field[bits 0-31] shoud be
 * 3FH(Add one to the return value to get the result)
 */
void cache_rqmid_27970_native_l1i_cache_number_of_sets(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	id.c == 0x3F, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D number of ways_001
 *
 * Summary: The ways of associativity of level-1 data cache on the physical platform shall be 8.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=0):EBX.Ways of associativity Field[bits 22-31] shoud be
 * 07H(Add one to the return value to get the result)
 */
void cache_rqmid_27971_native_l1d_cache_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 && get_bit_range(id.b, 22, 31) == 0x7, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D non-inclusive_001
 *
 * Summary: Level-1 data cache on the physical platform shall be non-inclusive of lower levels of cache.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=0):EDX.Cache Inclusiveness Field[bit 1] shoud be
 * 0H(Cache is not inclusive of lower cache levels)
 */
void cache_rqmid_27972_native_l1d_cache_non_inclusive(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.d, 1, 1) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical INVD/WBINVD acting upon all levels_001
 *
 * Summary: WBINVD/INVD from a physical CPU shall act upon all levels of caches for the CPU.
 * 1.CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 *   CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *   CPUID.(EAX=04H,ECX=0):EDX.Write-Back Invalidate Field[bit 0] shoud be
 *   0H(WBINVD/INVD from threads sharing this cache acts upon lower level
 *   caches for threads sharing this cache)
 * 2.CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H(Instruction cache)
 *	 CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *	 CPUID.(EAX=04H,ECX=1):EDX.Write-Back Invalidate Field[bit 0] shoud be
 *	 0H(WBINVD/INVD from threads sharing this cache acts upon lower level
 * 3.caches for threads sharing this cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 *	 CPUID.(EAX=04H,ECX=2):EDX.Write-Back Invalidate Field[bit 0] shoud be
 *	 0H(WBINVD/INVD from threads sharing this cache acts upon lower level
 *   caches for threads sharing this cache)
 * 4.CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 *	 CPUID.(EAX=04H,ECX=3):EDX.Write-Back Invalidate Field[bit 0] shoud be
 *	 0H(WBINVD/INVD from threads sharing this cache acts upon lower level
 *	 caches for threads sharing this cache)
 */
void cache_rqmid_27973_native_invd_wbinvd_acting_upon_all_levels_cache(void)
{
	struct cpuid id;
	bool ret1d = true;
	bool ret1i = true;
	bool ret2 = true;
	bool ret3 = true;

	id = cpuid_indexed(0x04, 0);
	if (get_bit_range(id.a, 0, 4) != 0x1 || get_bit_range(id.a, 5, 7) != 0x1 || get_bit_range(id.d, 0, 0) != 0x0) {
		ret1d = false;
	}

	id = cpuid_indexed(0x04, 1);
	if (get_bit_range(id.a, 0, 4) != 0x2 || get_bit_range(id.a, 5, 7) != 0x1 || get_bit_range(id.d, 0, 0) != 0x0) {
		ret1i = false;
	}

	id = cpuid_indexed(0x04, 2);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x2 || get_bit_range(id.d, 0, 0) != 0x0) {
		ret2 = false;
	}

	id = cpuid_indexed(0x04, 3);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x3 || get_bit_range(id.d, 0, 0) != 0x0) {
		ret3 = false;
	}

	report("%s\n", ret1d == true && ret1i == true && ret2 == true && ret3 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical cache line partitions_001
 *
 * Summary: The physical line partitions of any level of cache on the physical platform shall be 1.
 * 1.CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 *   CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *	 CPUID.(EAX=04H,ECX=0):EBX.Physical Line partitions Filed[bits 12-21] shoud be
 *	 0H (Add one to the return value to get the result)
 * 2.CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H(Instruction cache)
 *	 CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *	 CPUID.(EAX=04H,ECX=1):EBX.Physical Line partitions Filed[bits 12-21] shoud be
 *	 0H (Add one to the return value to get the result)
 * 3.CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 *	 CPUID.(EAX=04H,ECX=2):EBX.Physical Line partitions Filed[bits 12-21] shoud be
 *	 0H (Add one to the return value to get the result)
 * 4.CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 *	 CPUID.(EAX=04H,ECX=3):EBX.Physical Line partitions Filed[bits 12-21] shoud be
 *	 0H (Add one to the return value to get the result)
 */
void cache_rqmid_27974_native_cache_line_partitions(void)
{
	struct cpuid id;
	bool ret1d = true;
	bool ret1i = true;
	bool ret2 = true;
	bool ret3 = true;

	id = cpuid_indexed(0x04, 0);
	if (get_bit_range(id.a, 0, 4) != 0x1 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.b, 12, 21) != 0x0) {
		ret1d = false;
	}

	id = cpuid_indexed(0x04, 1);
	if (get_bit_range(id.a, 0, 4) != 0x2 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.b, 12, 21) != 0x0) {
		ret1i = false;
	}

	id = cpuid_indexed(0x04, 2);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x2 ||
	    get_bit_range(id.b, 12, 21) != 0x0) {
		ret2 = false;
	}

	id = cpuid_indexed(0x04, 3);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x3 ||
	    get_bit_range(id.b, 12, 21) != 0x0) {
		ret3 = false;
	}

	report("%s\n", ret1d == true && ret1i == true && ret2 == true && ret3 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 number of ways_001
 *
 * Summary: The ways of associativity of level-3 cache on the physical platform shall be 16.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 * CPUID.(EAX=04H,ECX=3):EBX.Ways of associativity Field[bits 22-31] shoud be
 * 0FH(Add one to the return value to get the result)
 */
void cache_rqmid_27975_native_l3_cache_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 && get_bit_range(id.b, 22, 31) == 0xF, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 number of ways_001
 *
 * Summary: The ways of associativity of level-2 universal cache on the physical platform shall be 4.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EBX.Ways of associativity Field[bits 22-31] shoud be
 * 03H(Add one to the return value to get the result)
 */
void cache_rqmid_27976_native_l2_cache_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 && get_bit_range(id.b, 22, 31) == 0x3, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 cache self-initializing_001
 *
 * Summary: The level-2 universal cache on the physical platform shall be self-initializing.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EAX.Self Initializing Field[bit 8] shoud be 1H(Self Initializing)
 */
void cache_rqmid_27977_native_l2_self_initializing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 && get_bit_range(id.a, 8, 8) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D number of sets_001s
 *
 * Summary: The number of sets of level-1 data cache on the physical platform shall be 64.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=3):EBX.Ways of associativity Field[bits 22-31] shoud be
 * 07H(Add one to the return value to get the result)
 */
void cache_rqmid_27978_native_l1d_cache_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 && get_bit_range(id.b, 22, 31) == 0x7, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I cache max addressable APIC IDs_001
 *
 * Summary: The maximum number of addressable APIC IDs for logical processors sharing level-1
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EAX.Maximum number of addressable IDs for logical processors
 * sharing this cache Field[bits 14-25] shoud be 1H(Add one to the return value to get the result)
 */
void cache_rqmid_27979_native_l1i_cache_max_addressable_apic_ids(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.a, 14, 25) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D cache non-fully-associative_001
 *
 * Summary: The level-1 data cache on the physical platform shall not be fully associative.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=0):EAX.Fully Associative cache Field[bit 9] shoud be
 * 0H(Not Fully Associative)
 */
void cache_rqmid_27985_native_l1d_cache_non_fully_associative(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 && get_bit_range(id.a, 9, 9) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical has L3 cache_001
 *
 * Summary: Physical has L3 cache.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 */
void cache_rqmid_27986_native_l3_cache(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 && get_bit_range(id.a, 5, 7) == 0x3, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D cache max addressable APIC IDs_001
 *
 * Summary: The maximum number of addressable APIC IDs for logical processors sharing
 * level-1 data cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=0):EAX.Maximum number of addressable IDs for logical processors
 * sharing this cache Field[bits 14-25] shoud be 1H(Add one to the return value to get the result)
 */
void cache_rqmid_27988_native_l1d_cache_max_addressable_apic_ids(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 && get_bit_range(id.a, 14, 25) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical levels of cache_001
 *
 * Summary: The physical platform shall have 3 level caches.
 * 1.CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 *	 CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * 2.CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H(Instruction cache)
 *	 CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * 3.CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * 4.CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 */
void cache_rqmid_27989_native_have_3_level_caches(void)
{
	struct cpuid id;
	bool ret1d = true;
	bool ret1i = true;
	bool ret2 = true;
	bool ret3 = true;

	id = cpuid_indexed(0x04, 0);
	if (get_bit_range(id.a, 0, 4) != 0x1 || get_bit_range(id.a, 5, 7) != 0x1) {
		ret1d = false;
	}

	id = cpuid_indexed(0x04, 1);
	if (get_bit_range(id.a, 0, 4) != 0x2 || get_bit_range(id.a, 5, 7) != 0x1) {
		ret1i = false;
	}

	id = cpuid_indexed(0x04, 2);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x2) {
		ret2 = false;
	}

	id = cpuid_indexed(0x04, 3);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x3) {
		ret3 = false;
	}

	report("%s\n", ret1d == true && ret1i == true && ret2 == true && ret3 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 cache non-fully-associative_001
 *
 * Summary: The level-2 universal cache on the physical platform shall not be fully associative.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EAX.Fully Associative cache Field[bit 9] shoud be
 * 0H(Not Fully Associative)
 */
void cache_rqmid_27990_native_l2_cache_non_fully_associative(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 && get_bit_range(id.a, 9, 9) == 0x0, __FUNCTION__);
}


/**
 * @brief case name:Physical has L2 cache_001
 *
 * Summary: The physical platform shall have level-2 universal cache.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 */
void cache_rqmid_27991_native_has_l2_cache(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D cache indexing_001
 *
 * Summary: Level-1 data cache on the physical platform shall use direct cache indexing.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EDX.Complex Cache Indexing Field[bit 2] shoud be 0H(Direct mapped cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 */
void cache_rqmid_27992_native_l1d_cache_indexing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 number of sets_001
 *
 * Summary: The number of sets of level-2 universal cache on the physical platform shall be 1024.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):ECX.Number of Sets Field[bits 0-31] shoud be
 * 3FFH(Add one to the return value to get the result)
 */
void cache_rqmid_27993_native_l2_cache_number_of_sets(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 &&	id.c == 0x3FF, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I cache indexing_001
 *
 * Summary: Level-1 instruction cache on the physical platform shall use direct cache indexing.
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EDX.Complex Cache Indexing Field[bit 2] shoud be 0H(Direct mapped cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 */
void cache_rqmid_27994_native_l1i_cache_indexing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical has L1I cache_001
 *
 * Summary: The physical platform shall have level-1 instruction cache.
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 */
void cache_rqmid_27995_native_l1i_cache(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 && get_bit_range(id.a, 5, 7) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 number of sets_001
 *
 * Summary: The number of sets of level-3 cache on the physical platform shall be 8192.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 * 1FFFH(Add one to the return value to get the result)
 * CPUID.(EAX=04H,ECX=3):ECX.Number of Sets Field[bits 0-31] shoud be
 */
void cache_rqmid_27996_native_l3_cache_number_of_sets(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 && id.c == 0x1FFF, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 cache indexing_001
 *
 * Summary: Level-2 universal cache on the physical platform shall use direct cache indexing.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EDX.Complex Cache Indexing Field[bit 2] shoud be 0H(Direct mapped cache)
 */
void cache_rqmid_27997_native_l2_cache_indexing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I number of ways_001
 *
 * Summary: The ways of associativity of level-1 instruction cache on the physical platform shall be 8.
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EBX.Ways of associativity Field[bits 22-31] shoud be
 * 07H(Add one to the return value to get the result)
 */
void cache_rqmid_27998_native_l1i_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.b, 22, 31) == 0x7, __FUNCTION__);
}

/**
 * @brief case name:Physical cache coherency line size_001
 *
 * Summary: The system coherency line size of any level of cache on the physical platform shall be 64.
 * 1.CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 *	 CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *	 CPUID.(EAX=04H,ECX=0):EBX.System Coherency Line Size Filed[bits 0-11] shoud be
 *	 3FH(Add one to the return value to get the result)
 * 2.CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H(Instruction cache)
 *	 CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *	 CPUID.(EAX=04H,ECX=1):EBX.System Coherency Line Size Filed[bits 0-11] shoud be
 *	 3FH(Add one to the return value to get the result)
 * 3.CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 *	 CPUID.(EAX=04H,ECX=2):EBX.System Coherency Line Size Filed[bits 0-11] shoud be
 *	 3FH(Add one to the return value to get the result)
 * 4.CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 *	 CPUID.(EAX=04H,ECX=3):EBX.System Coherency Line Size Filed[bits 0-11]s shoud be
 *	 3FH(Add one to the return value to get the result)
 */
void cache_rqmid_27999_native_cache_line_partitions(void)
{
	struct cpuid id;
	bool ret1d = true;
	bool ret1i = true;
	bool ret2 = true;
	bool ret3 = true;

	id = cpuid_indexed(0x04, 0);
	if (get_bit_range(id.a, 0, 4) != 0x1 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.b, 0, 11) != 0x3F) {
		ret1d = false;
	}

	id = cpuid_indexed(0x04, 1);
	if (get_bit_range(id.a, 0, 4) != 0x2 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.b, 0, 11) != 0x3F) {
		ret1i = false;
	}

	id = cpuid_indexed(0x04, 2);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x2 ||
	    get_bit_range(id.b, 0, 11) != 0x3F) {
		ret2 = false;
	}

	id = cpuid_indexed(0x04, 3);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x3 ||
	    get_bit_range(id.b, 0, 11) != 0x3F) {
		ret3 = false;
	}

	report("%s\n", ret1d == true && ret1i == true && ret2 == true && ret3 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I non-inclusive_001
 *
 * Summary: Level-1 instruction cache on the physical platform shall be non-inclusive of lower levels of cache.
 *
 * @brief case name:Physical L1I cache non-fully-associative_001
 *
 * Summary: The level-1 instruction cache on the physical platform shall not be fully associative.
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EDX.Cache Inclusiveness Field[bit 1] shoud be
 * 0H(Cache is not inclusive of lower cache levels)
 */
void cache_rqmid_28000_native_l1i_non_inclusive(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.d, 1, 1) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I cache non-fully-associative_001
 *
 * Summary: The level-1 instruction cache on the physical platform shall not be fully associative
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EAX.Fully Associative cache Field[bit 9] shoud be 0H(Not Fully Associative)
 */
void cache_rqmid_28001_native_l1i_non_fully_associative(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.a, 9, 9) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 cache non-fully-associative_001
 *
 * Summary: The level-3 cache on the physical platform shall not be fully associative.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Fully Associative cache Field[bit 9] shoud be 0H(Not Fully Associative)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 */
void cache_rqmid_28002_native_l3_cache_non_fully_associative(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 && get_bit_range(id.a, 9, 9) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical max addressable core IDs_001
 *
 * Summary: The maximum number of addressable Core IDs for logical processors in the CPU
 * package on the physical platform shall be 8.
 * 1.CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 *   CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *   CPUID.(EAX=04H,ECX=0):EAX.Maximum number of addressable IDs for processor cores in the
 *   physical Field[bits 26-31] shoud be 7H(Add one to the return value to get the result)
 * 2.CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H(Instruction cache)
 *   CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 *   CPUID.(EAX=04H,ECX=1):EAX.Maximum number of addressable IDs for processor cores in the
 *   physical Field[bits 26-31] shoud be 7H(Add one to the return value to get the result)
 * 3.CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 *	 CPUID.(EAX=04H,ECX=2):EAX.Maximum number of addressable IDs for processor cores in the
 *	 physical Field[bits 26-31] shoud be 7H(Add one to the return value to get the result)
 * 4.CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 *	 CPUID.(EAX=04H,ECX=3):EAX.Maximum number of addressable IDs for processor cores in the
 *	 physical Field[bits 26-31] shoud be 7H(Add one to the return value to get the result)
 */
void cache_rqmid_28003_native_cache_line_partitions(void)
{
	struct cpuid id;
	bool ret1d = true;
	bool ret1i = true;
	bool ret2 = true;
	bool ret3 = true;

	id = cpuid_indexed(0x04, 0);
	if (get_bit_range(id.a, 0, 4) != 0x1 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.a, 26, 31) != 0x7) {
		ret1d = false;
	}

	id = cpuid_indexed(0x04, 1);
	if (get_bit_range(id.a, 0, 4) != 0x2 || get_bit_range(id.a, 5, 7) != 0x1 ||
	    get_bit_range(id.a, 26, 31) != 0x7) {
		ret1i = false;
	}

	id = cpuid_indexed(0x04, 2);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x2 ||
	    get_bit_range(id.a, 26, 31) != 0x7) {
		ret2 = false;
	}

	id = cpuid_indexed(0x04, 3);
	if (get_bit_range(id.a, 0, 4) != 0x3 || get_bit_range(id.a, 5, 7) != 0x3 ||
	    get_bit_range(id.a, 26, 31) != 0x7) {
		ret3 = false;
	}

	report("%s\n", ret1d == true && ret1i == true && ret2 == true && ret3 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 cache max addressable APIC IDs_001
 *
 * Summary: The maximum number of addressable APIC IDs for logical processors sharing level-3
 * cache on the physical platform shall be 16.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 * CPUID.(EAX=04H,ECX=3):EAX.Maximum number of addressable IDs for logical processors sharing
 * this cache Field[bits 14-25] shoud be 0FH(Add one to the return value to get the result)
 */
void cache_rqmid_28004_native_l3_cache_non_fully_associative(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 && get_bit_range(id.a, 14, 25) == 0xF, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 non-inclusive_001
 *
 * Summary: Level-2 universal cache on the physical platform shall be non-inclusive of lower levels of cache.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EDX.Cache Inclusiveness Field[bit 1] shoud be
 * 0H(Cache is not inclusive of lower cache levels)
 */
void cache_rqmid_28005_native_l2_cache_indexing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 &&	get_bit_range(id.d, 1, 1) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical L1D cache self-initializing_001
 *
 * Summary: The level-1 data cache on the physical platform shall be self-initializing.
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Type Field[bits 0-4] shoud be 1H(Data cache)
 * CPUID.(EAX=04H,ECX=0):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=0):EAX.Self Initializing Field[bit 8] shoud be 1H(Self Initializing)
 */
void cache_rqmid_28006_native_l1d_cache_number_of_ways(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 && get_bit_range(id.a, 8, 8) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:PAT controls shall be available on the physical platform_001
 *
 * Summary: PAT controls shall be available on the physical platform.
 * CPUID.(EAX=04H,ECX=0):EDX.PAT flag Field [bit 16] shoud be 01H(Support)
 * Get/set IA32_PAT MSR register no exception
 */
void cache_rqmid_28007_native_pat_available(void)
{
	u64 ia32_pat;
	struct cpuid id;
	bool ret = true;

	id = cpuid(0x01);
	if (get_bit_range(id.d, 16, 16) != 0x1) {
		ret = false;
	}

	ia32_pat = rdmsr(IA32_PAT_MSR);
	wrmsr(IA32_PAT_MSR, ia32_pat&(~0xFF));
	/*reset pat control*/
	wrmsr(IA32_PAT_MSR, ia32_pat);

	report("%s\n", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Physical L3 cache self-initializing_001
 *
 * Summary: The level-3 cache on the physical platform shall be self-initializing.
 * cache on the physical platform shall be 16.
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=3):EAX.Cache Level Field[bits 5-7] shoud be 3H(Level 3)
 * CPUID.(EAX=04H,ECX=3):EAX.Self Initializing Field[bit 8] shoud be 1H(Self Initializing)
 */
void cache_rqmid_28008_native_l3_cache_self_initializing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 && get_bit_range(id.a, 8, 8) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L1I cache self-initializing_001
 *
 * Summary: The level-1 instruction cache on the physical platform shall be self-initializing.
 * instruction cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Type Field[bits 0-4] shoud be 2H (Instruction Cache)
 * CPUID.(EAX=04H,ECX=1):EAX.Cache Level Field[bits 5-7] shoud be 1H(Level 1)
 * CPUID.(EAX=04H,ECX=1):EAX.Self Initializing Field[bit 8] shoud be 1H(Self Initializing)
 */
void cache_rqmid_28009_native_l1i_cache_self_initializing(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.a, 8, 8) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical L2 cache max addressable APIC IDs_001
 *
 * Summary: The maximum number of addressable APIC IDs for logical processors sharing level-2
 * universal cache on the physical platform shall be 2.
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Type Field[bits 0-4] shoud be 3H(Unified Cache)
 * CPUID.(EAX=04H,ECX=2):EAX.Cache Level Field[bits 5-7] shoud be 2H(Level 2)
 * CPUID.(EAX=04H,ECX=2):EAX.Maximum number of addressable IDs for logical processors sharing
 * this cache Field[bits 14-25] shoud be 1H(Add one to the return value to get the result)
 */
void cache_rqmid_28010_native_l2_cache_max_addressable_apic_ids(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 &&	get_bit_range(id.a, 14, 25) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:PAT controls shall be available on the physical platform_002
 *
 * Summary: PAT controls shall be available on the physical platform.
 * CPUID.(EAX=04H,ECX=0):EDX.PAT flag Field [bit 16] shoud be 01H(Support)
 * Get/set IA32_PAT MSR register pat entry to reserved value generate #GP(0).
 */
void cache_rqmid_28062_native_pat_available(void)
{
	bool ret1 = true;
	bool ret2 = true;
	trigger_func fun;
	struct msr_data wp;
	u64 ia32_pat;
	struct cpuid id;

	id = cpuid(0x01);
	if (get_bit_range(id.d, 16, 16) != 0x1) {
		ret1 = false;
	}

	ia32_pat = rdmsr(IA32_PAT_MSR);

	fun = wrmsr_p;
	wp.index = IA32_PAT_MSR;
	wp.value = (ia32_pat&(~0xFF))|0x2;
	ret2 = test_for_exception(GP_VECTOR, fun, (void *)&wp);
	/*reset pat control*/
	wrmsr(IA32_PAT_MSR, ia32_pat);

	report("%s\n", ret1 == true && ret2 == true, __FUNCTION__);
}

/**
 * @brief case name:Physical cache invalidation instructions_002
 *
 * Summary: Cache invalidation instructions shall be available on the physical platform.
 * execute wbinvd instruction no exception.
 */
void cache_rqmid_28088_native_native_invalidation(void)
{
	bool ret = true;

	asm_mfence();
	asm_wbinvd();

	report("%s execute wbinvd\n", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Physical deterministic cache parameters_001
 *
 * Summary: Deterministic cache parameters shall be available on the physical platform
 * CPUID.(EAX=04H,ECX=00H) should be return the sizes and characteristics of the L1 Data cache.
 */
void cache_rqmid_28089_native_deterministic_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x1 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.a, 8, 8) == 0x1 &&
			get_bit_range(id.a, 9, 9) == 0x0 &&	get_bit_range(id.a, 14, 25) == 0x1 &&
			get_bit_range(id.a, 26, 31) == 0x7 && get_bit_range(id.b, 0, 11) == 0x3F &&
			get_bit_range(id.b, 12, 21) == 0x0 && get_bit_range(id.b, 22, 31) == 0x7 &&
			id.c == 0x3F && get_bit_range(id.d, 0, 0) == 0x0 &&
			get_bit_range(id.d, 1, 1) == 0x0 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical deterministic cache parameters_002
 *
 * Summary: Deterministic cache parameters shall be available on the physical platform
 * CPUID.(EAX=04H,ECX=01H) should be return the sizes and characteristics of the L1 Instruction cache.
 */
void cache_rqmid_28091_native_deterministic_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x2 &&
			get_bit_range(id.a, 5, 7) == 0x1 &&	get_bit_range(id.a, 8, 8) == 0x1 &&
			get_bit_range(id.a, 9, 9) == 0x0 &&	get_bit_range(id.a, 14, 25) == 0x1 &&
			get_bit_range(id.a, 26, 31) == 0x7 && get_bit_range(id.b, 0, 11) == 0x3F &&
			get_bit_range(id.b, 12, 21) == 0x0 && get_bit_range(id.b, 22, 31) == 0x7 &&
			id.c == 0x3F && get_bit_range(id.d, 0, 0) == 0x0 &&
			get_bit_range(id.d, 1, 1) == 0x0 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical deterministic cache parameters_003
 *
 * Summary: Deterministic cache parameters shall be available on the physical platform
 * CPUID.(EAX=04H,ECX=02H) should be return the sizes and characteristics of the L2 Unified cache.
 */
void cache_rqmid_28093_native_deterministic_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x2 &&	get_bit_range(id.a, 8, 8) == 0x1 &&
			get_bit_range(id.a, 9, 9) == 0x0 &&	get_bit_range(id.a, 14, 25) == 0x1 &&
			get_bit_range(id.a, 26, 31) == 0x7 && get_bit_range(id.b, 0, 11) == 0x3F &&
			get_bit_range(id.b, 12, 21) == 0x0 && get_bit_range(id.b, 22, 31) == 0x3 &&
			id.c == 0x3FF && get_bit_range(id.d, 0, 0) == 0x0 &&
			get_bit_range(id.d, 1, 1) == 0x0 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical deterministic cache parameters_004
 *
 * Summary: Deterministic cache parameters shall be available on the physical platform
 * CPUID.(EAX=04H,ECX=03H) should be return the sizes and characteristics of the L3 Unified cache.
 */
void cache_rqmid_28095_native_deterministic_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x3 &&
			get_bit_range(id.a, 5, 7) == 0x3 &&	get_bit_range(id.a, 8, 8) == 0x1 &&
			get_bit_range(id.a, 9, 9) == 0x0 &&	get_bit_range(id.a, 14, 25) == 0xF &&
			get_bit_range(id.a, 26, 31) == 0x7 && get_bit_range(id.b, 0, 11) == 0x3F &&
			get_bit_range(id.b, 12, 21) == 0x0 && get_bit_range(id.b, 22, 31) == 0xF &&
			id.c == 0x1FFF && get_bit_range(id.d, 0, 0) == 0x0 &&
			get_bit_range(id.d, 1, 1) == 0x1 &&	get_bit_range(id.d, 2, 2) == 0x1, __FUNCTION__);
}

/**
 * @brief case name:Physical deterministic cache parameters_005
 *
 * Summary: Deterministic cache parameters shall be available on the physical platform
 * CPUID.(EAX=04H,ECX=04H) should be return the
 * parameters report the value associated with the cache type field is 0.
 */
void cache_rqmid_28096_native_deterministic_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 4);
	report("%s\n", get_bit_range(id.a, 0, 4) == 0x0 &&
			get_bit_range(id.a, 5, 7) == 0x0 &&	get_bit_range(id.a, 8, 8) == 0x0 &&
			get_bit_range(id.a, 9, 9) == 0x0 &&	get_bit_range(id.a, 14, 25) == 0x0 &&
			get_bit_range(id.a, 26, 31) == 0x0 && get_bit_range(id.b, 0, 11) == 0x0 &&
			get_bit_range(id.b, 12, 21) == 0x0 && get_bit_range(id.b, 22, 31) == 0x0 &&
			id.c == 0x0 && get_bit_range(id.d, 0, 0) == 0x0 &&
			get_bit_range(id.d, 1, 1) == 0x0 &&	get_bit_range(id.d, 2, 2) == 0x0, __FUNCTION__);
}

/**
 * @brief case name:Physical CLFLUSH/CLFLUSHOPT line size_001
 *
 * Summary: The CLFLUSH/CLFLUSHOPT line size on the physical platform shall be 64 bytes.
 * CPUID.1:EBX.CLFLUSH line size Field[bits 8-15] shoud be
 * 8H (Value * 8 = cache line size in bytes; used also by CLFLUSHOPT)
 */
void cache_rqmid_28097_native_clflush_clflushopt_line_size(void)
{
	struct cpuid id;

	id = cpuid(0x01);
	report("%s\n", get_bit_range(id.b, 8, 15) == 0x8, __FUNCTION__);
}

/**
 * @brief case name:Memory type test on native PAT UC_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify PAT entry 0 to UC.
 * 3. Order read the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order read the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order read the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order read the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 */
void cache_rqmid_28742_native_pat_uc(void)
{
	int i;
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 0(UC)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0x00)));

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L1 PAT UC read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L2 PAT UC read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L3 PAT UC read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native over L3 PAT UC read average is %ld\r\n", tsc_delay_delta_total);

	/*reset pat entry*/
	set_mem_cache_type_all(ia32_pat_tmp);
}

/**
 * @brief case name:Memory type test on native PAT UC_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify PAT entry 0 to UC.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 */
void cache_rqmid_28744_native_pat_uc(void)
{
	int i;
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 0(UC)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0x00)));

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L1 PAT UC write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L2 PAT UC write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L3 PAT UC write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native over L3 PAT UC write average is %ld\r\n", tsc_delay_delta_total);

	/*reset pat entry*/
	set_mem_cache_type_all(ia32_pat_tmp);
}

/**
 * @brief case name:Memory type test on native PAT WB_001
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify PAT entry 0 to WB.
 * 3. Order read the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order read the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order read the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order read the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 */
void cache_rqmid_28746_native_pat_wb(void)
{
	int i;
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 6(WB)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0xFF))|0x6);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L1 PAT WB read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L2 PAT WB read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L3 PAT WB read average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	read_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = read_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native over L3 PAT WB read average is %ld\r\n", tsc_delay_delta_total);

	/*reset pat entry*/
	set_mem_cache_type_all(ia32_pat_tmp);
}

/**
 * @brief case name:Memory type test on native PAT WB_002
 *
 * Summary: This part verifies different memory type and instructions will invalid cache on native,
 * target is to get benchmark data
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify PAT entry 0 to WB.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 */
void cache_rqmid_28747_native_pat_uc(void)
{
	int i;
	u64 ia32_pat_tmp;

	ia32_pat_tmp = rdmsr(IA32_PAT_MSR);
	/*bit 0-7 set to 6(WB)*/
	set_mem_cache_type_all((ia32_pat_tmp&(~0xFF))|0x6);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l1_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l1_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L1 PAT WB write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l2_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l2_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L2 PAT WB write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native L3 PAT WB write average is %ld\r\n", tsc_delay_delta_total);

	/*Remove the first test data*/
	write_mem_cache_test(cache_over_l3_size);
	tsc_delay_delta_total = 0;
	for (i = 0; i < CACHE_TEST_TIME_MAX; i++) {
		tsc_delay[i] = write_mem_cache_test(cache_over_l3_size);
		tsc_delay_delta_total += tsc_delay[i];
	}
	tsc_delay_delta_total /= CACHE_TEST_TIME_MAX;
	printf("native over L3 PAT WB write average is %ld\r\n", tsc_delay_delta_total);

	/*reset pat entry*/
	set_mem_cache_type_all(ia32_pat_tmp);
}

struct case_fun_index cache_control_native_cases[] = {
	{24443, cache_rqmid_24443_native_cache_parameters_in_cpuid02},
	{24445, cache_rqmid_24445_native_PREFETCHW_native},
#ifdef DUMP_CACHE_NATIVE_DATA
	{25314, cache_rqmid_25314_native_clflush},
	{25472, cache_rqmid_25472_native_clflushopt},
	{26891, cache_rqmid_26891_native_invalidation},
	{26913, cache_rqmid_26913_native_l1_wb},
	{26917, cache_rqmid_26917_native_l1_wc},
	{26919, cache_rqmid_26919_native_l1_wc},
	{26921, cache_rqmid_26921_native_l1_wt},
	{26923, cache_rqmid_26923_native_l1_wt},
	{26925, cache_rqmid_26925_native_l1_wp},
	{26927, cache_rqmid_26927_native_l1_wp},
	{26929, cache_rqmid_26929_native_l1_uc},
	{26931, cache_rqmid_26931_native_l1_uc},
	{26933, cache_rqmid_26933_native_l2_wb},
	{26935, cache_rqmid_26935_native_l2_wb},
	{26937, cache_rqmid_26937_native_l2_wc},
	{26939, cache_rqmid_26939_native_l2_wc},
	{26941, cache_rqmid_26941_native_l2_wt},
	{26943, cache_rqmid_26943_native_l2_wt},
	{26945, cache_rqmid_26945_native_l2_wp},
	{26947, cache_rqmid_26947_native_l2_wp},
	{26948, cache_rqmid_26948_native_l2_uc},
	{26949, cache_rqmid_26949_native_l2_uc},
	{26950, cache_rqmid_26950_native_l3_wb},
	{26951, cache_rqmid_26951_native_l3_wb},
	{26952, cache_rqmid_26952_native_l3_wc},
	{26953, cache_rqmid_26953_native_l3_wc},
	{26954, cache_rqmid_26954_native_l3_wt},
	{26955, cache_rqmid_26955_native_l3_wt},
	{26956, cache_rqmid_26956_native_l3_wp},
	{26957, cache_rqmid_26957_native_l3_wp},
	{26958, cache_rqmid_26958_native_l3_uc},
	{26959, cache_rqmid_26959_native_l3_uc},
	{26960, cache_rqmid_26960_native_over_l3_wb},
	{26961, cache_rqmid_26961_native_over_l3_wb},
	{26962, cache_rqmid_26962_native_over_l3_wc},
	{26963, cache_rqmid_26963_native_over_l3_wc},
	{26964, cache_rqmid_26964_native_over_l3_wt},
	{26965, cache_rqmid_26965_native_over_l3_wt},
	{26966, cache_rqmid_26966_native_over_l3_wp},
	{26967, cache_rqmid_26967_native_over_l3_wp},
	{26968, cache_rqmid_26968_native_over_l3_uc},
	{26969, cache_rqmid_26969_native_over_l3_uc},
	{26992, cache_rqmid_26992_native_l1_wb},
	{27027, cache_rqmid_27027_native_map_to_device_linear_uc},
	{27028, cache_rqmid_27028_native_map_to_device_linear_uc},
	{27487, cache_rqmid_27487_native_clflushopt},   //clflushopt disorder
	{27488, cache_rqmid_27488_native_invalidation}, //invalidation disorder
	{27489, cache_rqmid_27489_native_clflush},      //clflush disorder
	{28742, cache_rqmid_28742_native_pat_uc},
	{28744, cache_rqmid_28744_native_pat_uc},
	{28746, cache_rqmid_28746_native_pat_wb},
	{28747, cache_rqmid_28747_native_pat_uc},
#endif
	{27965, cache_rqmid_27965_native_has_l1d_cache},
	{27968, cache_rqmid_27968_native_l3_cache_indexing},
	{27970, cache_rqmid_27970_native_l1i_cache_number_of_sets},
	{27971, cache_rqmid_27971_native_l1d_cache_number_of_ways},
	{27972, cache_rqmid_27972_native_l1d_cache_non_inclusive},
	{27973, cache_rqmid_27973_native_invd_wbinvd_acting_upon_all_levels_cache},
	{27974, cache_rqmid_27974_native_cache_line_partitions},
	{27975, cache_rqmid_27975_native_l3_cache_number_of_ways},
	{27976, cache_rqmid_27976_native_l2_cache_number_of_ways},
	{27977, cache_rqmid_27977_native_l2_self_initializing},
	{27978, cache_rqmid_27978_native_l1d_cache_number_of_ways},
	{27979, cache_rqmid_27979_native_l1i_cache_max_addressable_apic_ids},
	{27985, cache_rqmid_27985_native_l1d_cache_non_fully_associative},
	{27986, cache_rqmid_27986_native_l3_cache},
	{27988, cache_rqmid_27988_native_l1d_cache_max_addressable_apic_ids},
	{27989, cache_rqmid_27989_native_have_3_level_caches},
	{27990, cache_rqmid_27990_native_l2_cache_non_fully_associative},
	{27991, cache_rqmid_27991_native_has_l2_cache},
	{27992, cache_rqmid_27992_native_l1d_cache_indexing},
	{27993, cache_rqmid_27993_native_l2_cache_number_of_sets},
	{27994, cache_rqmid_27994_native_l1i_cache_indexing},
	{27995, cache_rqmid_27995_native_l1i_cache},
	{27996, cache_rqmid_27996_native_l3_cache_number_of_sets},
	{27997, cache_rqmid_27997_native_l2_cache_indexing},
	{27998, cache_rqmid_27998_native_l1i_number_of_ways},
	{27999, cache_rqmid_27999_native_cache_line_partitions},
	{28000, cache_rqmid_28000_native_l1i_non_inclusive},
	{28001, cache_rqmid_28001_native_l1i_non_fully_associative},
	{28002, cache_rqmid_28002_native_l3_cache_non_fully_associative},
	{28003, cache_rqmid_28003_native_cache_line_partitions},
	{28004, cache_rqmid_28004_native_l3_cache_non_fully_associative},
	{28005, cache_rqmid_28005_native_l2_cache_indexing},
	{28006, cache_rqmid_28006_native_l1d_cache_number_of_ways},
	{28007, cache_rqmid_28007_native_pat_available},
	{28008, cache_rqmid_28008_native_l3_cache_self_initializing},
	{28009, cache_rqmid_28009_native_l1i_cache_self_initializing},
	{28010, cache_rqmid_28010_native_l2_cache_max_addressable_apic_ids},
	{28062, cache_rqmid_28062_native_pat_available},
	{28088, cache_rqmid_28088_native_native_invalidation},
	{28089, cache_rqmid_28089_native_deterministic_cache_parameters},
	{28091, cache_rqmid_28091_native_deterministic_cache_parameters},
	{28093, cache_rqmid_28093_native_deterministic_cache_parameters},
	{28095, cache_rqmid_28095_native_deterministic_cache_parameters},
	{28096, cache_rqmid_28096_native_deterministic_cache_parameters},
	{28097, cache_rqmid_28097_native_clflush_clflushopt_line_size},
};

static void print_cache_control_native_case_list_64()
{
	int i;

	printf("cache native case list:\n\r");

	for (i = 0; i < (sizeof(cache_control_native_cases)/sizeof(cache_control_native_cases[0])); i++) {
		printf("Case ID: %d\n", cache_control_native_cases[i].rqmid);
	}

	printf("\n\n");
}

void cache_test_native(long rqmid)
{
	print_cache_control_native_case_list_64();

	cache_fun_exec(cache_control_native_cases,
		       sizeof(cache_control_native_cases)/sizeof(cache_control_native_cases[0]), rqmid);
}

