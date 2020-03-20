/**
 * Test for x86 cache and memory cache control
 *
 * 64bit mode
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

/**
 * @brief Construct non canonical address
 *
 * Set cache_test_array array address bit 63rd to 1
 * Execute CLFLUSH will generate #GP(0).
 */
void cache_clflush_non_canonical(void *data)
{
	unsigned long address;

	address = (unsigned long)(&cache_test_array);
	address = (address|(1UL<<63));
	/*asm volatile("clflush (%0)" : : "b" (address));*/
	asm_clflush(address);
}

/**
 * @brief case name:Cache control deterministic cache parameters_003
 *
 * Summary: CPUID.(EAX=04H,ECX=02H) should be return the sizes and characteristics of the L2 Unified cache.
 */
void cache_rqmid_23875_check_l2_ucache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 2);

	report("%s\n", (get_bit_range(id.a, 0, 4) == 0x3) &&
		(get_bit_range(id.a, 5, 7) == 0x2) &&	(get_bit_range(id.a, 8, 8) == 0x1) &&
		(get_bit_range(id.a, 9, 9) == 0x0) &&	(get_bit_range(id.a, 14, 25) == 0x1) &&
		(get_bit_range(id.a, 26, 31) == 0x7) && (get_bit_range(id.b, 0, 11) == 0x3F) &&
		(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0x3) &&
		(id.c == 0x3FF) && (get_bit_range(id.d, 0, 0) == 0x0) &&
		(get_bit_range(id.d, 1, 1) == 0x0) &&	(get_bit_range(id.d, 2, 2) == 0x0), __FUNCTION__);
}

/**
 * @brief case name:INVD instruction_001
 *
 * Summary: Hypervisor should hide invd instruction,
 * execute invd instruction generate result a #GP exception.
 */
void cache_rqmid_23985_invd(void)
{
	trigger_func fun;
	bool ret;

	asm_mfence();
	fun = asm_invd;
	//asm_invd();
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s execute invd generate #GP(0)\n", ret == true, __FUNCTION__);
}

/**
 * @brief case name:Cache control L1 data cache context mode_001
 *
 * Summary: CPUID.1:ECX.L1 Context ID Field[bit 10] should be 0H(Not support L1 data cache context mode).
 */
void cache_rqmid_24189_l1_data_cache_context_mode(void)
{
	struct cpuid id;
	int value = 0;

	id = cpuid_indexed(0x01, 0);
	value = get_bit_range(id.c, 10, 10);

	report("%s l1 data cache context mode %s\n", value == 0x0, __FUNCTION__, (value == 0x1) ? "present" : "absent");
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WB_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WB.
 * 3. Order read the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order read the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order read the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order read the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26972_guest_linear_normal_wb(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;

	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WB, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WB, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WB, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WB, cache_over_l3_size);

	report("%s WB read test\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control CLFLUSH instruction_exception_001
 *
 * Summary: If the memory address is in a non-canonical form, execute CLFLUSH will generate #GP(0). 
 */
void cache_rqmid_27073_clflush_exception_001(void)
{
	bool ret = true;

	read_mem_cache_test_time_invd(cache_l1_size, 1);
	ret = test_for_exception(GP_VECTOR, cache_clflush_non_canonical, NULL);
	report("%s CLFLUSH #GP(0) exception\n", ret == true, __FUNCTION__);
}

void print_case_list_64()
{
	printf("cache 64bit feature case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 23875, "Cache control deterministic cache parameters_003");
	printf("\t Case ID:%d case name:%s\n\r", 23985, "INVD instruction_001");
	printf("\t Case ID:%d case name:%s\n\r", 24189, "Cache control L1 data cache context mode_001");
	printf("\t Case ID:%d case name:%s\n\r", 26972,
		"Cache control access to memory-mapped guest linear addresses in normal cache mode_WB_001");
	printf("\t Case ID:%d case name:%s\n\r", 27073, "Cache control CLFLUSH instruction_exception_001");
}

void cache_test_64(long rqmid)
{
	print_case_list_64();

	struct case_fun_index case_fun[] = {
		{23875, cache_rqmid_23875_check_l2_ucache_parameters},
		{23985, cache_rqmid_23985_invd},
		{24189, cache_rqmid_24189_l1_data_cache_context_mode},
		{26972, cache_rqmid_26972_guest_linear_normal_wb},
		{27073, cache_rqmid_27073_clflush_exception_001},
	};

	cache_fun_exec(case_fun, sizeof(case_fun)/sizeof(case_fun[0]), rqmid);
}

