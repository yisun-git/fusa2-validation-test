#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "smp.h"

//#define USE_DEBUG
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

#define MSR_IA32_EXT_XAPICID			0x00000802U
#define nop()		do { asm volatile ("nop\n\t" :::"memory"); } while (0)
#define TEST_TIME 10000

static volatile int start_sem1;
static volatile int start_sem2;
static volatile int stop_flag1;
static volatile int stop_flag2;
static int time;
static int pass_time;
static volatile u32 ptr;

static int time1;
static int time2;
static volatile int stop = 0;
static volatile int start_case_id = 0;

enum{
	CASE_ID_1 = 32739,
	CASE_ID_2 = 32742,
};

static u32 get_cur_lapic_id(void)
{
	u32 lapic_id;

	lapic_id = (u32) rdmsr(MSR_IA32_EXT_XAPICID);

	return lapic_id;
}

void asm_bts(u32 volatile *addr, int bit, int is_lock)
{
	if (is_lock) {
		__asm__ __volatile__("lock bts %1, %0"
			     : "+m" (*addr) : "Ir" (bit) : "cc", "memory");
	}
	else {
		__asm__ __volatile__("bts %1, %0"
			     : "+m" (*addr) : "Ir" (bit) : "cc", "memory");
	}
}

int bp_test(int vcpu, int is_lock)
{
	start_sem1 = 1;
	start_sem2 = 1;
	stop_flag1 = 0;
	stop_flag2 = 0;
	time = 0;
	pass_time = 0;
	ptr = 0;
	stop = 0;

	debug_print("bp start vcpu =%d %s\n", vcpu, is_lock ? "lock test" : "no lock test");

	while (1) {
		if ((stop_flag1 == 1) && (stop_flag2 == 1)) {
			stop_flag1 = 0;
			stop_flag2 = 0;
			time++;
		}
		else {
			continue;
		}

		if (ptr == 0xFFFFFFFF) {
			pass_time++;
		}

		if (time == TEST_TIME) {
			stop = 1;
			if (pass_time == TEST_TIME) {
				debug_print("%s %d pass %d %d \n", __FUNCTION__, __LINE__, time, pass_time);
				break;
			}
			else {
				debug_print("%s %d fail %d %d \n", __FUNCTION__, __LINE__, time, pass_time);
				break;
			}
		}
		else {
			ptr = 0;
			start_sem1 = 1;
			start_sem2 = 1;
			continue;
		}
	}

	debug_print("bp1 stop\n");
	return pass_time;
}

void ap1_test(int vcpu, int is_lock)
{
	time1 = 0;
	debug_print("ap1 start vcpu =%d %s\n", vcpu, is_lock ? "lock test" : "no lock test");

	while (1) {
		if (start_sem1 != 1) {
			if (stop == 1) {
				break;
			}
			else {
				continue;
			}
		}

		while (time1 < 32) {
			asm_bts(&ptr, time1, is_lock);
			time1 = time1 + 2;
		}

		start_sem1 = 0;
		stop_flag1 = 1;
		time1 = 0;
		continue;
	}

	debug_print("ap1 stop\n");
}

void ap2_test(int vcpu, int is_lock)
{
	time2 = 1;
	debug_print("ap2 start vcpu =%d %s\n", vcpu, is_lock ? "lock test" : "no lock test");

	while (1) {
		if (start_sem2 != 1) {
			if (stop == 1) {
				break;
			}
			else {
				continue;
			}
		}

		while (time2 < 32) {
			asm_bts(&ptr, time2, is_lock);
			time2 = time2 + 2;
		}

		start_sem2 = 0;
		stop_flag2 = 1;
		time2 = 1;
		continue;
	}

	debug_print("ap2 stop\n");
}

__attribute__((unused)) static void locked_atomic_rqmid_32739_ap()
{
	u32 vcpu;

	vcpu = get_cur_lapic_id();

	if (vcpu == 1) {
		ap1_test(vcpu, 1);
	}
	else if (vcpu == 2) {
		ap2_test(vcpu, 1);
	}
	else {
		debug_error("other ap no test cpu_id=%d\n", vcpu);
	}
}
/**
 * @brief case name:Atomic - expose LOCK prefix to VM_002
 *
 * Summary:Under 64 bit mode, Test the atomicity of the lock instruction(BTS)
 * in the case of multicore concurrency(with lock prifex).
 */
__attribute__((unused)) static void locked_atomic_rqmid_32739_expose_lock_prefix_02()
{
	u32 vcpu;
	int ret;

	start_case_id = CASE_ID_1;
	debug_print("start_case_id=%d\n", start_case_id);

	vcpu = get_cur_lapic_id();

	ret = bp_test(vcpu, 1);
	report("%s pass=%d", (ret == TEST_TIME), __FUNCTION__, ret);
}

__attribute__((unused)) static void locked_atomic_rqmid_32742_ap()
{
	u32 vcpu;

	vcpu = get_cur_lapic_id();

	if (vcpu == 1) {
		ap1_test(vcpu, 0);
	}
	else if (vcpu == 2) {
		ap2_test(vcpu, 0);
	}
	else {
		debug_print("other ap no test cpu_id=%d\n", vcpu);
	}
}
/**
 * @brief case name:Atomic - expose LOCK prefix to VM_003
 *
 * Summary:Under 64 bit mode, Test the atomicity of the lock instruction(BTS)
 * in the case of multicore concurrency(without lock prifex).
 */
__attribute__((unused)) static void locked_atomic_rqmid_32742_expose_unlock_prefix_03()
{
	u32 vcpu;
	int ret;

	start_case_id = CASE_ID_2;
	debug_print("start_case_id=%d\n", start_case_id);

	vcpu = get_cur_lapic_id();

	ret = bp_test(vcpu, 0);
	report("%s pass=%d", (ret < TEST_TIME), __FUNCTION__, ret);
}

void ap_main(void)
{
/* safety only 1 vcpu, no ap run*/
#ifdef IN_NON_SAFETY_VM
	printf("ap %d main test start\n", get_cur_lapic_id());
	while (start_case_id != CASE_ID_1) {
		nop();
	}
	locked_atomic_rqmid_32739_ap();

	while (start_case_id != CASE_ID_2) {
		nop();
	}
	locked_atomic_rqmid_32742_ap();
	debug_print("ap main test stop\n");
#endif
}

static void print_case_list()
{
	printf("locked atomic feature case list:\n\r");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 32739U, "Atomic - expose LOCK prefix to VM_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 32742u, "Atomic - expose LOCK prefix to VM_003");
#endif
}

int main(int ac, char **av)
{
	print_case_list();
	/*smp_init();*/

	/*first run case id 32739*/
#ifdef IN_NON_SAFETY_VM
	locked_atomic_rqmid_32739_expose_lock_prefix_02();
	locked_atomic_rqmid_32742_expose_unlock_prefix_03();
#endif
	return report_summary();
}

