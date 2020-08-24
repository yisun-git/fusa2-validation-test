/**
 * Test for x86 cache and memory cache control
 *
 * 64bit mode
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "pci_util.h"

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

void readmsr(void *data)
{
	u32 msr = *(u32 *)data;
	rdmsr(msr);
}

/**
 * @brief case name:Cache control deterministic cache parameters_001
 *
 * Summary: CPUID.(EAX=04H,ECX=00H) should be return the sizes and characteristics of the L1 Data cache.
 */
void cache_rqmid_23873_check_l1_dcache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 0);

	report("%s\n", (get_bit_range(id.a, 0, 4) == 0x1) &&
			(get_bit_range(id.a, 5, 7) == 0x1) && (get_bit_range(id.a, 8, 8) == 0x1) &&
			(get_bit_range(id.a, 9, 9) == 0x0) && (get_bit_range(id.a, 14, 25) == 0x1) &&
			(get_bit_range(id.a, 26, 31) == 0x7) && (get_bit_range(id.b, 0, 11) == 0x3F) &&
			(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0x7) &&
			(id.c == 0x3F) && (get_bit_range(id.d, 0, 0) == 0x0) &&
			(get_bit_range(id.d, 1, 1) == 0x0) && (get_bit_range(id.d, 2, 2) == 0x0), __FUNCTION__);
}

/**
 * @brief case name:Cache control deterministic cache parameters_002
 *
 * Summary: CPUID.(EAX=04H,ECX=01H) should be return the sizes and characteristics of the L1 Instruction cache.
 */
void cache_rqmid_23874_check_l1_icache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 1);

	report("%s\n", (get_bit_range(id.a, 0, 4) == 0x2) &&
			(get_bit_range(id.a, 5, 7) == 0x1) && (get_bit_range(id.a, 8, 8) == 0x1) &&
			(get_bit_range(id.a, 9, 9) == 0x0) && (get_bit_range(id.a, 14, 25) == 0x1) &&
			(get_bit_range(id.a, 26, 31) == 0x7) && (get_bit_range(id.b, 0, 11) == 0x3F) &&
			(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0x7) &&
			(id.c == 0x3F) && (get_bit_range(id.d, 0, 0) == 0x0) &&
			(get_bit_range(id.d, 1, 1) == 0x0) && (get_bit_range(id.d, 2, 2) == 0x0), __FUNCTION__);
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
		(get_bit_range(id.a, 5, 7) == 0x2) && (get_bit_range(id.a, 8, 8) == 0x1) &&
		(get_bit_range(id.a, 9, 9) == 0x0) && (get_bit_range(id.a, 14, 25) == 0x1) &&
		(get_bit_range(id.a, 26, 31) == 0x7) && (get_bit_range(id.b, 0, 11) == 0x3F) &&
		(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0x3) &&
		(id.c == 0x3FF) && (get_bit_range(id.d, 0, 0) == 0x0) &&
		(get_bit_range(id.d, 1, 1) == 0x0) && (get_bit_range(id.d, 2, 2) == 0x0), __FUNCTION__);
}

/**
 * @brief case name:Cache control deterministic cache parameters_004
 *
 * Summary: CPUID.(EAX=04H,ECX=03H) should be return the sizes and characteristics of the L3 Unified cache.
 */
void cache_rqmid_23876_check_l3_ucache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 3);

	report("%s\n", (get_bit_range(id.a, 0, 4) == 0x3) &&
			(get_bit_range(id.a, 5, 7) == 0x3) && (get_bit_range(id.a, 8, 8) == 0x1) &&
			(get_bit_range(id.a, 9, 9) == 0x0) && (get_bit_range(id.a, 14, 25) == 0xF) &&
			(get_bit_range(id.a, 26, 31) == 0x7) && (get_bit_range(id.b, 0, 11) == 0x3F) &&
			(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0xF) &&
			(id.c == 0x1FFF) && (get_bit_range(id.d, 0, 0) == 0x0) &&
			(get_bit_range(id.d, 1, 1) == 0x1) && (get_bit_range(id.d, 2, 2) == 0x1), __FUNCTION__);
}

/**
 * @brief case name:Cache control deterministic cache parameters_005
 *
 * Summary: CPUID.(EAX=04H,ECX=04H) should be return the
 *			parameters report the value associated with the cache type field is 0.
 */
void cache_rqmid_23877_check_no_cache_parameters(void)
{
	struct cpuid id;

	id = cpuid_indexed(0x04, 4);

	report("%s\n", (get_bit_range(id.a, 0, 4) == 0x0) &&
			(get_bit_range(id.a, 5, 7) == 0x0) && (get_bit_range(id.a, 8, 8) == 0x0) &&
			(get_bit_range(id.a, 9, 9) == 0x0) && (get_bit_range(id.a, 14, 25) == 0x0) &&
			(get_bit_range(id.a, 26, 31) == 0x0) && (get_bit_range(id.b, 0, 11) == 0x0) &&
			(get_bit_range(id.b, 12, 21) == 0x0) && (get_bit_range(id.b, 22, 31) == 0x0) &&
			(id.c == 0x0) && (get_bit_range(id.d, 0, 0) == 0x0) &&
			(get_bit_range(id.d, 1, 1) == 0x0) && (get_bit_range(id.d, 2, 2) == 0x0), __FUNCTION__);
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
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WB_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WB.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26973_guest_linear_normal_wb(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/*Cache size L1*/
	ret1 = cache_order_write_test(CACHE_L1_WRITE_WB, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_write_test(CACHE_L2_WRITE_WB, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_write_test(CACHE_L3_WRITE_WB, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_WB, cache_over_l3_size);

	report("%s WB write test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WC_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WC.
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
void cache_rqmid_26974_guest_linear_normal_wc(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WC, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WC, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WC, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WC, cache_over_l3_size);

	report("%s WC read test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WC_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WC.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26975_guest_linear_normal_wc(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/*Cache size L1*/
	ret1 = cache_order_write_test(CACHE_L1_WRITE_WC, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_write_test(CACHE_L2_WRITE_WC, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_write_test(CACHE_L3_WRITE_WC, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_WC, cache_over_l3_size);

	report("%s WC write test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WT_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WT.
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
void cache_rqmid_26976_guest_linear_normal_wt(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WT, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WT, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WT, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WT, cache_over_l3_size);

	report("%s WT read test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WT_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WT.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26977_guest_linear_normal_wt(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Cache size L1*/
	ret1 = cache_order_write_test(CACHE_L1_WRITE_WT, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_write_test(CACHE_L2_WRITE_WT, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_write_test(CACHE_L3_WRITE_WT, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_WT, cache_over_l3_size);

	report("%s WT write test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WP_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WP.
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
void cache_rqmid_26978_guest_linear_normal_wp(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WP, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WP, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WP, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WP, cache_over_l3_size);

	report("%s WP read test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_WP_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to WP.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26979_guest_linear_normal_wp(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Cache size L1*/
	ret1 = cache_order_write_test(CACHE_L1_WRITE_WP, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_write_test(CACHE_L2_WRITE_WP, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_write_test(CACHE_L3_WRITE_WP, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_WP, cache_over_l3_size);

	report("%s WP write test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_UC_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to UC.
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
void cache_rqmid_26980_guest_linear_normal_uc(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s UC read test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to memory-mapped guest linear addresses in normal cache mode_UC_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address range maps to memory,
 * ACRN hypervisor shall guarantee that the access follows caching and read/write policy of normal
 * cache mode with effective memory type being the PAT memory type of the guest address range.
 *
 * 1. Allocate an array a1 with 0x200000 elements, each of size 8 bytes,
 * 2. Modify a1 array memory type to UC.
 * 3. Order write the first 32KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 4. Order write the first 256KB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 5. Order write the first 8MB of the a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * 6. Order write the entire a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the 'L1/L2/L3/over L3' interval of WB benchmark
 * data interval (average-3 *stdev, average+3*stdev),WB benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_26981_guest_linear_normal_uc(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);

	/*Cache size L1*/
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s UC write test\n",
		   (ret1 == true) && (ret2 == true) && (ret3 == true) && (ret4 == true), __FUNCTION__);
}

void wr_cr0(void *data)
{
	u32 cr0 = *(u32 *)data;
	write_cr0(cr0);
}

/**
 * @brief case name:Cache_control_invalid_cache_operating_mode_configuration_001
 *
 * Summary: When a vCPU attempts to write guest CR0, the new guest
 * CR0.CD is 0H and the new guest CR0.NW is 1H, ACRN hypervisor
 * shall guarantee that the vCPU receives a #GP(0).
 */
void cache_rqmid_36874_invalid_cache_mode(void)
{
	u32 cr0 = read_cr0();
	bool ret = false;

	cr0 &= ~(1 << CR0_BIT_CD);
	cr0 |= (1 << CR0_BIT_NW);

	ret = test_for_exception(GP_VECTOR, wr_cr0, &cr0);

	report("%s, Invalid Cache control setting. Generates a general-protection exception #GP(0)\n",
		ret == true,
		__FUNCTION__);
}

/**
 * @brief case name:Memory type WT and WP shall be the same_001
 *
 * Summary: The physical platform shall guarantee that caching and read/write policy of normal cache mode
 * with effective memory type being WP is the same as the policy with effective memory type being WT.
 * 1. WT should in WP benchmark data interval (average-3 *stdev, average+3*stdev),
 *	  WP benchmark data (average, stdev ) get from native, refer Memory type test on native
 * 2. WP should in WT benchmark data interval (average-3 *stdev, average+3*stdev),
 *	  WT benchmark data (average, stdev ) get from native, refer Memory type test on native
 */
void cache_rqmid_28100_wt_wp_shall_be_the_same(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	bool retwt = true;
	bool retwp = true;

	/*Set memory type to WT*/
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WP, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WP, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WP, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WP, cache_over_l3_size);
	if ((ret1 != true) || (ret2 != true) || (ret3 != true) || (ret4 != true)) {
		retwt = false;
	}

	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/*Cache size L1*/
	ret1 = cache_order_read_test(CACHE_L1_READ_WT, cache_l1_size);

	/*Cache size L2*/
	ret2 = cache_order_read_test(CACHE_L2_READ_WT, cache_l2_size);

	/*Cache size L3*/
	ret3 = cache_order_read_test(CACHE_L3_READ_WT, cache_l3_size);

	/*Cache size over L3*/
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WT, cache_over_l3_size);
	if ((ret1 != true) || (ret2 != true) || (ret3 != true) || (ret4 != true)) {
		retwt = false;
	}

	report("%s\n", (retwt == true) && (retwp == true), __FUNCTION__);
}

/**
 * @brief case name:Cache_control_access_to_guest_linear_addresses_in_no-fill_cache_mode_001
 *
 * When a vCPU accesses a guest address range and the guest CR0.CD is 1H,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type
 * being UC.
 */
void cache_rqmid_36877_no_fill_cache_mode(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	u32 cr0;

	/* Don't care about NW bit,
	 * following case shall be passed on no matter what NW
	 */
	write_cr0_bybit(CR0_BIT_CD, 1);
	cr0 = read_cr0();

	report("%s CR0 cd bit shoud be set.\n", (cr0 & CR0_BIT_CD),  __FUNCTION__);

	/* Configure PAT to use UC, in No-fill Cache Mode, it
	 * should behavior like UC
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);
	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test in No-fill Cache Mode, should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test in No-fill Cache Mode, should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WB, however in No-fill Cache Mode,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test in No-fill Cache Mode, WB configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test in No-fill Cache Mode, WB configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WP, however in No-fill Cache Mode,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test in No-fill Cache Mode, WP configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test in No-fill Cache Mode, WP configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WT, however in No-fill Cache Mode,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test in No-fill Cache Mode, WT configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test in No-fill Cache Mode, WT configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WC, however in No-fill Cache Mode,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test in No-fill Cache Mode, WC configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test in No-fill Cache Mode, WC configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);
}

#define BAR_REMAP_USB_BASE    0xDFFF0000
#define PCI_BAR_MASK	      0xFFFF0000
#define USB_HCIVERSION        0x02
#define PCI_PCIR_BAR(x)	      (0x10 + (x) * 4)

/**
 * @brief In 64-bit mode,Select USB device BAR0 as the test object.
 *       Write the value 0xDFFF_0000 to the BAR0 register of USB device.
 *       Read the HCIVERSION register of USB device
 *       If HCIVERSION register value is 0x100, pass,otherwize fail
 *
 * none
 * Application Constraints: Only for USB.
 *
 * @param none
 *
 * @return OK. seccessed,pci_mem_address value is valid
 *
 * @retval (-1) fialed,pci_mem_address value is invalid
 */
int pci_mem_get(void *pci_mem_address)
{
	union pci_bdf bdf = {.bits = {.b = 0, .d = 0x14, .f = 0} };
	uint32_t bar_base;
	uint32_t reg_val;
	int ret = OK;

	pci_pdev_write_cfg(bdf, PCI_PCIR_BAR(0), 4, BAR_REMAP_USB_BASE);

	reg_val = pci_pdev_read_cfg(bdf, PCI_PCIR_BAR(0), 4);
	DBG_INFO("R reg[%xH] = [%xH]", PCI_PCIR_BAR(0), reg_val);

	bar_base = PCI_BAR_MASK & reg_val;

	reg_val = pci_pdev_read_mem(bdf, (bar_base + USB_HCIVERSION), 2);
	if (0x100 != reg_val) {
		DBG_ERRO("R mem[%xH] != [%xH]", bar_base + USB_HCIVERSION, 0x100);
		ret = ERROR;
	}

	if (OK == ret) {
		*((uint64_t *)pci_mem_address) = BAR_REMAP_USB_BASE;
	}

	return ret;
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WB_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WB.
 * 3. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27018_map_to_device_linear_wb(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WB
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK0, 4096);

		/*Cache size 4k*/
		ret = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s read test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WB_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WB.
 * 3. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27019_map_to_device_linear_wb(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		/* 601*8=0x3008*/
		//cache_test_array += 601;

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WB
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK0, 4096);

		/*Cache size 4k*/
		ret = cache_order_write_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s write test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WC_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WC.
 * 3. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27020_map_to_device_linear_wc(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WC
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK3, 4096);

		/*Cache size 4k*/
		ret = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s read test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WC_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WC.
 * 3. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27021_map_to_device_linear_wc(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		/* 601*8=0x3008*/
		// cache_test_array += 601;

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WC
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK3, 4096);

		/*Cache size 4k*/
		ret = cache_order_write_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s write test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WT_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WT.
 * 3. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27022_map_to_device_linear_wt(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WT
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK2, 4096);

		/*Cache size 4k*/
		ret = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s read test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WT_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WT.
 * 3. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27023_map_to_device_linear_wt(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		/* 601*8=0x3008*/
		// cache_test_array += 601;

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WT
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK2, 4096);

		/*Cache size 4k*/
		ret = cache_order_write_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s write test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WP_001
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WP.
 * 3. Order read a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27024_map_to_device_linear_wp(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WP
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK1, 4096);

		/*Cache size 4k*/
		ret = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s read test\n", (ret == true), __FUNCTION__);
}

/**
 * @brief case name:Cache control access to device-mapped guest linear addresses in normal cache mode_WP_002
 *
 * Summary: When a vCPU accesses a guest address range, the guest CR0.CD is 0H, the guest CR0.NW is 0H,
 * the guest CR0.PG is 1H and the guest address range maps to device,
 * ACRN hypervisor shall guarantee that the access follows caching and
 * read/write policy of normal cache mode with effective memory type being UC.
 *
 * 1. Allocate an array a1 from the physical pages of "map to device",
 *    a1 has 0x200 elements, each of size 8 bytes, (4K space)
 * 2. Modify a1 array memory type to WP.
 * 3. Order write a1 array 41 times, record each tsc_delay data.
 *    Remove the first test data and calculate the average TSC delay for the next 40 times.
 * Tsc delay average should be within the '4k' interval of 'map to device' UC benchmark
 * data interval (average-3 *stdev, average+3*stdev), 'map to device' UC benchmark data (average, stdev )
 * get from native, refer Memory type test on native
 */
void cache_rqmid_27026_map_to_device_linear_wp(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		/* 601*8=0x3008*/
		// cache_test_array += 601;

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WP
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK1, 4096);

		/*Cache size 4k*/
		ret = cache_order_write_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s write test\n", (ret == true), __FUNCTION__);
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
void cache_rqmid_27027_map_to_device_linear_uc(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/* This address has 4k reserved space to operate
		 * Set the next 4k sapce memory type to WP
		 */
		set_memory_type_pt(cache_test_array, PT_MEMORY_TYPE_MASK4, 4096);

		/*Cache size 4k*/
		ret = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s read test\n", (ret == true), __FUNCTION__);
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
void cache_rqmid_27028_map_to_device_linear_uc(void)
{
	bool ret = true;
	u64 *tmp = cache_test_array;
	int r;

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
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
		ret = cache_order_write_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	}

	cache_test_array = tmp;
	report("%s write test\n", (ret == true), __FUNCTION__);
}


#if 0
/**
 * @brief Cache_control_access_to_device-mapped_guest_linear_addresses_in_normal_cache_mode_001
 *
 * When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address
 * range maps to device, ACRN hypervisor shall guarantee that the access
 * follows caching and read/write policy of normal cache mode with effective
 * memory type being UC.
 *
 */
void cache_rqmid_36872_device_mapped_guest_linear_normal(void)
{
	// 27018 ~27024, 27026 ~27028
}
#endif

void cache_setup_mmu_range(pgd_t *cr3, phys_addr_t start, size_t len)
{
	u64 max = (u64)len + (u64)start;
	u64 phys = start;

	install_pages(cr3, phys, max - phys, (void *)(ulong)phys);
}

/**
 * @brief Cache_control_access_to_empty-mapped_guest_linear_addresses_in_normal_cache_mode_001
 *
 * When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 1H and the guest address
 * range maps to none, ACRN hypervisor shall guarantee that the access
 * follows caching and read/write policy of normal cache mode with effective
 * memory type being UC.
 */
void cache_rqmid_36875_empty_mapped_guest_linear_normal(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	u64 *tmp = cache_test_array;

	cache_setup_mmu_range(phys_to_virt(read_cr3()), (1ul << 36), (1ul << 30)); /*64G-65G  map to none*/
	cache_test_array = (u64 *)(1ul << 36);

	/* Configure PAT to use UC, due to map to none, it
	 * should behavior like UC
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK4);
	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test, should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test, should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WB, however due to map to none,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK0);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test, WB configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test, WB configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WP, however due to map to none,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK1);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test, WP configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test, WP configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WT, however due to map to none,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK2);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test, WT configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	report("%s Write test, WT configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Configure PAT to use WC, however due to map to none,
	 * it should behavior like UC.
	 */
	set_mem_cache_type(PT_MEMORY_TYPE_MASK3);

	/* Read test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_read_test(CACHE_L1_READ_UC, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_UC, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_UC, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_UC, cache_over_l3_size);

	report("%s Read test, WC configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	/* Write test, cache size L1, L2, L3 and over L3 */
	ret1 = cache_order_write_test(CACHE_L1_WRITE_UC, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_UC, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_UC, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_UC, cache_over_l3_size);

	cache_test_array = tmp;

	report("%s Write test, WC configured but should behavior like UC.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);
}

#if 0
void cache_disable_paging()
{
	write_cr0_bybit(X86_CR0_PG, 0);

	/*flush caches and TLBs*/
	asm_wbinvd();
	flush_tlb();
}

void cache_enable_paging()
{
	write_cr0_bybit(X86_CR0_PG, 0);

	/*flush caches and TLBs*/
	asm_wbinvd();
	flush_tlb();
}
#endif

/**
 * @brief cache_rqmid_36873_memory_mapped_guest_physical_normal
 *
 * When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 0H and the guest address
 * range maps to memory, ACRN hypervisor shall guarantee that the access
 * follows caching and read/write policy of normal cache mode with effective
 * memory type being WB.
 */
void cache_rqmid_36873_memory_mapped_guest_physical_normal(void)
{
	bool ret1 = true;
	bool ret2 = true;
	bool ret3 = true;
	bool ret4 = true;
	u32 cr0;

	write_cr0_bybit(CR0_BIT_CD, 0);
	write_cr0_bybit(CR0_BIT_NW, 0);
	cr0 = read_cr0();
	report("%s CR0 cd and nw bit shoud not be set.\n",
		~(cr0 & CR0_BIT_CD) && ~(cr0 & CR0_BIT_CD),  __FUNCTION__);

	ret1 = cache_order_read_test(CACHE_L1_READ_WB, cache_l1_size);
	ret2 = cache_order_read_test(CACHE_L2_READ_WB, cache_l2_size);
	ret3 = cache_order_read_test(CACHE_L3_READ_WB, cache_l3_size);
	ret4 = cache_order_read_test(CACHE_OVER_L3_READ_WB, cache_over_l3_size);

	report("%s READ Test.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);

	ret1 = cache_order_write_test(CACHE_L1_WRITE_WB, cache_l1_size);
	ret2 = cache_order_write_test(CACHE_L2_WRITE_WB, cache_l2_size);
	ret3 = cache_order_write_test(CACHE_L3_WRITE_WB, cache_l3_size);
	ret4 = cache_order_write_test(CACHE_OVER_L3_WRITE_WB, cache_over_l3_size);

	report("%s Write test.\n",
		(ret1 == true) && (ret2 == true) && (ret3 == true) &&
		(ret4 == true), __FUNCTION__);
}

/**
 * @brief cache_rqmid_36873_memory_mapped_guest_physical_normal
 *
 * When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 0H and the guest address
 * range maps to device, ACRN hypervisor shall guarantee that the access
 * follows caching and read/write policy of normal cache mode with effective
 * memory type being UC.
 */
void cache_rqmid_36876_device_mapped_guest_physical_normal(void)
{
	bool ret1 = true;
	bool ret2 = true;
	u64 *tmp = cache_test_array;
	int r;
	u32 cr0;

	write_cr0_bybit(CR0_BIT_CD, 0);
	write_cr0_bybit(CR0_BIT_NW, 0);
	cr0 = read_cr0();
	report("%s CR0 cd and nw bit shoud not be set.\n",
		~(cr0 & CR0_BIT_CD) && ~(cr0 & CR0_BIT_CD), __FUNCTION__);

	r = pci_mem_get((void *)&cache_test_array);
	if (r != 0) {
		cache_test_array = tmp;
		report("%s can't get pci mem address\n", 0, __FUNCTION__);
		return;
	}
	else {
		// cache_test_array += 601;	/* 601*8=0x3008*/

		/*Cache size 4k*/
		ret1 = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
		ret2 = cache_order_read_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
		report("%s device mapped physical \n", (ret1 == true) && (ret2 == true),
			__FUNCTION__);
	}

	cache_test_array = tmp;
}

/**
 * @brief cache_rqmid_36873_memory_mapped_guest_physical_normal
 *
 * When a vCPU accesses a guest address range, the guest CR0.CD is 0H,
 * the guest CR0.NW is 0H, the guest CR0.PG is 0H and the guest address
 * range maps to none, ACRN hypervisor shall guarantee that the access follows
 * caching and read/write policy of normal cache mode with effective memory type
 * being UC.
 */
void cache_rqmid_36886_empty_mapped_guest_physical_normal(void)
{
	bool ret1 = true;
	bool ret2 = true;
	u64 *tmp = cache_test_array;
	u32 cr0;

	cache_test_array = (u64 *) (1ul << 36);

	write_cr0_bybit(CR0_BIT_CD, 0);
	write_cr0_bybit(CR0_BIT_NW, 0);
	cr0 = read_cr0();
	report("%s CR0 cd and nw bit shoud not be set.\n",
		~(cr0 & CR0_BIT_CD) && ~(cr0 & CR0_BIT_CD), __FUNCTION__);

	// cache_test_array += 601;	/* 601*8=0x3008*/

	/*Cache size 4k*/
	ret1 = cache_order_read_test(CACHE_DEVICE_4K_READ, cache_4k_size);
	ret2 = cache_order_read_test(CACHE_DEVICE_4K_WRITE, cache_4k_size);
	report("%s device mapped physical \n", (ret1 == true) && (ret2 == true),
		__FUNCTION__);

	cache_test_array = tmp;
}


/**
 * @brief case name:Cache control CLFLUSH instruction_exception_001
 *
 * Summary: If the memory address is in a non-canonical form, execute CLFLUSH will generate #GP(0). 
 */
void cache_rqmid_27073_clflush_exception(void)
{
	bool ret = true;

	read_mem_cache_test_time_invd(cache_l1_size, 1);
	ret = test_for_exception(GP_VECTOR, cache_clflush_non_canonical, NULL);
	report("%s CLFLUSH #GP(0) exception\n", ret == true, __FUNCTION__);
}

/**
 * @brief case name: Cache_control_MTRR_general_support_001
 *
 * Summary: ACRN hypervisor shall hide MTTR general support from any VM.
 */
void cache_rqmid_36865_mtrr_general_support(void)
{
	struct cpuid id = cpuid_indexed(0x01, 0);
	u32 edx = id.d & (1 << 12);

	report("%s, Hide MTTR general support from any VM.\n", edx == 0x0, __FUNCTION__);
}

/**
 * @brief case name: Cache_control_MTRR_fixed_range_registers_001
 *
 * Summary: ACRN hypervisor shall hide MTRR fixed range registers from any VM.
 */
void cache_rqmid_36866_mtrr_fixed_range_registers(void)
{
	/* u64 mtrrcap_msr = rdmsr(IA32_MTRRCAP_MSR);
	 * u64 fixed_range = mtrrcap_msr & (1 << 8);
	 *
	 * The SRS just says "hide". Hide can be defined as one of the
	 * following two cases.
	 *  - GP(0) when reading this msr.
	 *  - The bit 8 of mtrrcap_msr equals to 0.
	 *
	 * Read IA32_MTRRCAP_MSR will cause GP(0), can't test bit 8.
	 */

	bool ret = false;
	u32 msr = IA32_MTRRCAP_MSR;
	ret = test_for_exception(GP_VECTOR, readmsr, &msr);

	report("%s, Hide MTRR fixed range registers from any VM.\n",
		ret == true,
		__FUNCTION__);
}

/**
 * @brief case name: Cache_control_MTRR_write-combining_memory_type_support_001
 *
 * Summary: ACRN hypervisor shall hide MTRR write-combining memory type support from any VM.
 */
void cache_rqmid_36867_mtrr_write_combining_memory_type_support(void)
{
	/* u64 mtrrcap_msr = rdmsr(IA32_MTRRCAP_MSR);
	 * u64 wc_supported = mtrrcap_msr & (1 << 10);
	 *
	 * The SRS just says "hide". Hide can be defined as one of the
	 * following two cases.
	 *  - GP(0) when reading this msr.
	 *  - The bit 10 of mtrrcap_msr equals to 0.
	 *
	 * Read IA32_MTRRCAP_MSR will cause GP(0), can't test bit 10.
	 */

	bool ret = false;
	u32 msr = IA32_MTRRCAP_MSR;
	ret = test_for_exception(GP_VECTOR, readmsr, &msr);

	report("%s, Hide MTRR write-combining memory type support from any VM.\n",
		ret == true,
		__FUNCTION__);
}

/**
 * @brief case name: Cache_control_system_management_range_register_001
 *
 * Summary: ACRN hypervisor shall hide MTRR system management range register from any VM.
 */
void cache_rqmid_36868_system_management_range_register(void)
{
	/* u64 mtrrcap_msr = rdmsr(IA32_MTRRCAP_MSR);
	 * u64 smrr_supported = mtrrcap_msr & (1 << 11);
	 *
	 * The SRS just says "hide". Hide can be defined as one of the
	 * following two cases.
	 *  - GP(0) when reading this msr.
	 *  - The bit 11 of mtrrcap_msr equals to 0.
	 *
	 * Read IA32_MTRRCAP_MSR will cause GP(0), can't test bit 11.
	 */

	bool ret = false;
	u32 msr = IA32_MTRRCAP_MSR;
	ret = test_for_exception(GP_VECTOR, readmsr, &msr);

	report("%s, Hide MTRR system management range register from any VM.\n",
		ret == true,
		__FUNCTION__);
}

/**
 * @brief case name: Cache_control_L3_cache_control_001
 *
 * Summary: ACRN hypervisor shall hide L3 cache control from any VM.
 */
void cache_rqmid_36870_l3_cache_control(void)
{
	u64 misc_enable_msr = rdmsr(IA32_MISC_ENABLE);
	u64 l3_enabled = misc_enable_msr & (1 << 6);

	report("%s, Hide L3 cache control from any VM.\n", l3_enabled == 0x0, __FUNCTION__);
}

/**
 * @brief case name: CLWB_instruction_001
 *
 * Summary: ACRN hypervisor shall hide CLWB instruction from any VM.
 */
void cache_rqmid_36871_clwb_instruction(void)
{
	struct cpuid id = cpuid_indexed(0x07, 0);
	u32 ebx = id.b & (1 << 24);

	report("%s, Hide CLWB instruction from any VM.\n", ebx == 0x0, __FUNCTION__);
}

/**
 * @brief case name: Cache_control_cache_invalidation_instructions_001
 *
 * Summary: ACRN hypervisor shall expose cache invalidation instructions to any VM.
 */
void cache_rqmid_36856_cache_invalidation_instructions(void)
{
	trigger_func fun;
	bool ret;

	fun = asm_wbinvd;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s, Execute wbinvd successfully.\n", ret == false, __FUNCTION__);
}

/**
 * @brief case name: Cache_control_CLFLUSHOPT_instruction_001
 *
 * Summary: ACRN hypervisor shall expose CLFLUSHOPT instruction to any VM.
 */
void cache_rqmid_36857_cflushopt_instruction(void)
{
	struct cpuid id = cpuid_indexed(0x07, 0);
	u32 ebx = id.b & (1 << 23);

	report("%s, Expose CLFLUSHOPT instruction to any VM.\n", ebx != 0x0, __FUNCTION__);

	/* Test CLFLUSHOPT behaviors
	 */
	trigger_func fun;
	bool ret;

	fun = asm_clflushopt;
	ret = test_for_exception(GP_VECTOR, fun, NULL);

	report("%s, Execute clflushopt successfully.\n", ret == false, __FUNCTION__);
}

/**
 * @brief case name: PREFETCHW_instruction_001
 *
 * Summary: ACRN hypervisor shall expose PREFETCHW instruction to any VM.
 */
void cache_rqmid_36860_prefetchw_instruction(void)
{
	struct cpuid id = cpuid_indexed(0x80000001, 0);
	u32 ecx = id.c & (1 << 8);

	report("%s, Expose PREFETCHW instruction to any VM.\n", ecx != 0x0, __FUNCTION__);
}

/**
 * @brief case name: Cache_control_PAT_controls_001
 *
 * Summary: ACRN hypervisor shall expose PAT controls to any VM.
 */
void cache_rqmid_36863_pat_controls(void)
{
	struct cpuid id = cpuid_indexed(0x01, 0);
	u32 edx = id.d & (1 << 16);

	report("%s, Expose PAT controls to any VM.\n", edx != 0x0, __FUNCTION__);
}

/**
 * @brief case name: Guest_CLFLUSH_CLFLUSHOPT_line_size_001
 *
 * Summary: When a vCPU attempts to read CPUID.1H, ACRN hypervisor
 *          shall write the CLFLUSH/CLFLUSHOPT line size in qword of
 *          the physical platform to guest EBX[bit 15:8].
 */
void cache_rqmid_36864_guest_clflush_clflushopt_line_size(void)
{
	struct cpuid id = cpuid_indexed(0x01, 0);
	u32 cache_line_size = get_bit_range(id.b, 8, 15);

	report("%s, Cache line size = %u.\n",
		CACHE_LINE_SIZE == (cache_line_size * 8),
		__FUNCTION__, cache_line_size * 8);
}

static struct case_fun_index cache_control_cases[] = {
	{23873, cache_rqmid_23873_check_l1_dcache_parameters},
	{23874, cache_rqmid_23874_check_l1_icache_parameters},
	{23875, cache_rqmid_23875_check_l2_ucache_parameters},
	{23876, cache_rqmid_23876_check_l3_ucache_parameters},
	{23877, cache_rqmid_23877_check_no_cache_parameters},
	{24189, cache_rqmid_24189_l1_data_cache_context_mode},
	{26972, cache_rqmid_26972_guest_linear_normal_wb},
	{26973, cache_rqmid_26973_guest_linear_normal_wb},
	{26974, cache_rqmid_26974_guest_linear_normal_wc},
	{26975, cache_rqmid_26975_guest_linear_normal_wc},
	{26976, cache_rqmid_26976_guest_linear_normal_wt},
	{26977, cache_rqmid_26977_guest_linear_normal_wt},
	{26978, cache_rqmid_26978_guest_linear_normal_wp},
	{26979, cache_rqmid_26979_guest_linear_normal_wp},
	{26980, cache_rqmid_26980_guest_linear_normal_uc},
	{26981, cache_rqmid_26981_guest_linear_normal_uc},
#if 0
	{27018, cache_rqmid_27018_map_to_device_linear_wb},
	{27019, cache_rqmid_27019_map_to_device_linear_wb},
	{27020, cache_rqmid_27020_map_to_device_linear_wc},
	{27021, cache_rqmid_27021_map_to_device_linear_wc},
	{27022, cache_rqmid_27022_map_to_device_linear_wt},
	{27023, cache_rqmid_27023_map_to_device_linear_wt},
	{27024, cache_rqmid_27024_map_to_device_linear_wp},
	{27026, cache_rqmid_27026_map_to_device_linear_wp},
	{27027, cache_rqmid_27027_map_to_device_linear_uc},
	{27028, cache_rqmid_27028_map_to_device_linear_uc},
	{36875, cache_rqmid_36875_empty_mapped_guest_linear_normal},
#endif
	{28100, cache_rqmid_28100_wt_wp_shall_be_the_same},
	{27073, cache_rqmid_27073_clflush_exception},
	{36865, cache_rqmid_36865_mtrr_general_support},
	{36866, cache_rqmid_36866_mtrr_fixed_range_registers},
	{36867, cache_rqmid_36867_mtrr_write_combining_memory_type_support},
	{36868, cache_rqmid_36868_system_management_range_register},
	{36870, cache_rqmid_36870_l3_cache_control},
	{36871, cache_rqmid_36871_clwb_instruction},
	{36856, cache_rqmid_36856_cache_invalidation_instructions},
	{36857, cache_rqmid_36857_cflushopt_instruction},
	{36860, cache_rqmid_36860_prefetchw_instruction},
	{36863, cache_rqmid_36863_pat_controls},
	{36864, cache_rqmid_36864_guest_clflush_clflushopt_line_size},
	{36874, cache_rqmid_36874_invalid_cache_mode},
	{23985, cache_rqmid_23985_invd},
	{36877, cache_rqmid_36877_no_fill_cache_mode},
};

static void print_cache_control_case_list_64()
{
	int i;

	printf("cache 64bit feature case list:\n\r");

	for (i = 0; i < (sizeof(cache_control_cases)/sizeof(cache_control_cases[0])); i++) {
		printf("Case ID: %d\n", cache_control_cases[i].rqmid);
	}

	printf("\n");
}

void cache_test_64(long rqmid)
{
	print_cache_control_case_list_64();

	cache_fun_exec(cache_control_cases, sizeof(cache_control_cases)/sizeof(cache_control_cases[0]), rqmid);
}

