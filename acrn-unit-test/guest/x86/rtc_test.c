/*
 *  file rtc_test.c
 */
#include "libcflat.h"
#include "x86/asm/io.h"

#define RTC_INDEX_REG		0x70
#define RTC_TARGET_REG		0x71
#define SECOND_INDEX		0x0
#define SECOND_ALARM_INDEX	0x1
#define MINUTE_INDEX	0x2
#define MINUTE_ALARM_INDEX	0x3
#define HOUR_INDEX		0x4
#define HOUR_ALARM_INDEX	0x5
#define DAY_OF_WEEK_INDEX	0x6
#define DAY_OF_MONTH_INDEX	0x7
#define MONTH_INDEX		0x8
#define YEAR_INDEX		0x9
#define A_INDEX		0xA
#define B_INDEX		0xB
#define C_INDEX		0xC
#define B_DEFAULT_VALUE	0x6

static uint8_t read_reg_al()
{
	uint8_t al;

	asm volatile(
		"mov %%al, %0\n\t"
		: "=r"(al));

	return al;
}

/**
 *  @brief Case name: When a vCPU attempts to write guest RTC target register,
 *  ACRN hypervisor shall ignore the write_001
 *
 * Summary: the value of the RTC target register has no change after writing this target
 * register with different data
 *
 */
void rtc_rqmid_24557_write_guest_rtc_target_register(void)
{
	uint8_t reg8_value_prev;
	uint8_t reg8_value;


	outb(B_INDEX, RTC_INDEX_REG);
	reg8_value_prev = inb(RTC_TARGET_REG);
	outb(0xFF, RTC_TARGET_REG);
	reg8_value = inb(RTC_TARGET_REG);

	report("\t\t rtc_rqmid_24557_write_guest_rtc_target_register", reg8_value == reg8_value_prev);
}

/**
 * @brief Case name: RTC_When a vCPU attempts to read guest RTC target register and the current guest
 * RTC index is greater than BH, ACRN hypervisor shall write 0 to guest AL_001
 *
 * Summary: dump the value of AL after reading from RTC target register
 *
 */
void rtc_rqmid_24494_reading_rtc_target_register(void)
{
	uint8_t reg8_value;

	outb(C_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t rtc_rqmid_24494_reading_rtc_target_register", reg8_value > 0xB);
}

/**
 * @brief Case name: RTC When a vCPU attempts to write 2 bytes to guest RTC index register,
 * ACRN hypervisor shall ignore the write_001
 *
 * Summary: RTC When a vCPU attempts to write 2 bytes to guest RTC index register,
 * ACRN hypervisor shall ignore the write_001
 *
 */
void rtc_rqmid_24560_write_2_bytes_to_guest_rtc_index_register(void)
{
	uint16_t reg16_value;

	outb(B_INDEX, RTC_INDEX_REG);
	outw(0xFFFF, RTC_INDEX_REG);
	reg16_value = inw(RTC_INDEX_REG);

	report("\t\t rtc_rqmid_24560_write_2_bytes_to_guest_rtc_index_register", reg16_value == B_INDEX);
}

/**
 * @brief Case name: RTC_When a vCPU attempts to read guest RTC target register and the current guest
 * RTC index is BH, ACRN hypervisor shall write 6H to guest AL_001
 *
 * Summary: dump the value of AL after reading from RTC target register
 *
 */
void rtc_rqmid_24495_reading_rtc_target_register_and_current_guest_rtc_index_is_BH(void)
{
	uint8_t reg_b_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg_b_value = read_reg_al();

	report("\t\t rtc_rqmid_24495_reading_rtc_target_register_and_current_guest_rtc_index_is_BH %d",
		reg_b_value == B_DEFAULT_VALUE, reg_b_value);
}

/**
 * @brief Case name: When a vCPU attempts to write guest RTC index register,
 * ACRN hypervisor shall keep guest AL as the current guest RTC index_001
 *
 * Summary:  the value of AL is keeped to be current RTC index register after writing guest RTC index register
 *
 */
void rtc_rqmid_24556_reading_rtc_AL_is_keeped_to_be_current_rtc_index_register(void)
{
	uint8_t reg8_value;

	outb(YEAR_INDEX, RTC_INDEX_REG);
	reg8_value = read_reg_al();

	report("\t\t rtc_rqmid_24556_reading_rtc_AL_is_keeped_to_be_current_rtc_index_register %d",
		reg8_value == YEAR_INDEX, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to write 2 bytes to guest RTC target register,
 * ACRN hypervisor shall ignore the write_001
 *
 * Summary:  the value of RTC target register has no change after writing 2 bytes to this target register
 *
 */
void rtc_rqmid_24558_writing_2bytes_to_target_register_register_has_no_change(void)
{
	uint16_t reg16_value_prev;
	uint16_t reg16_value;

	outb(B_INDEX, RTC_INDEX_REG);
	reg16_value_prev = inw(RTC_TARGET_REG);
	outw(0xFFFF, RTC_TARGET_REG);
	reg16_value = inw(RTC_TARGET_REG);

	report("\t\t rtc_rqmid_24558_writing_2bytes_to_target_register_register_has_no_change %d",
		reg16_value == reg16_value_prev, reg16_value);
}

/**
 * @brief Case name: when a vCPU attempts to read guest RTC index register,
 * ACRN hypervisor shall write 0H to guest AL_001
 *
 * Summary:  the value of AL should be always 0H even if RTC index register is set to a different value
 *
 */
void rtc_rqmid_24387_writing_write_0h_to_guest_al(void)
{
	uint8_t reg8_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_INDEX_REG);
	reg8_value = read_reg_al();

	report("\t\t rtc_rqmid_24387_writing_write_0h_to_guest_al %d", reg8_value == 0, reg8_value);
}

static void print_case_list(void)
{
	printf("\t\t RTC feature case list:\n\r");
	printf("\t\t Segmentation 64-Bits Mode:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 24557u,
		"When a vCPU attempts to write guest RTC target register, ACRN hypervisor shall ignore the write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24494u,
		"RTC When a vCPU attempts to read guest RTC target register and the current guest RTC index is greater"
		" than BH, ACRN hypervisor shall write 0 to guest AL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24560u,
		"RTC When a vCPU attempts to write 2 bytes to guest RTC index register,"
		" ACRN hypervisor shall ignore the write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24495u,
		"RTC_When a vCPU attempts to read guest RTC target register and the current guest RTC index is BH,"
		"ACRN hypervisor shall write 6H to guest AL_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24556u,
		"When a vCPU attempts to write guest RTC index register, ACRN hypervisor shall keep guest \
		AL as the current guest RTC index_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24558u,
		"When a vCPU attempts to write 2 bytes to guest RTC target register,"
		"ACRN hypervisor shall ignore the write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24387u,
		"when a vCPU attempts to read guest RTC index register, ACRN hypervisor shall write 0H to guest AL_001");
}

static void test_rtc(void)
{
	rtc_rqmid_24557_write_guest_rtc_target_register();
	rtc_rqmid_24494_reading_rtc_target_register();
	rtc_rqmid_24560_write_2_bytes_to_guest_rtc_index_register();
	rtc_rqmid_24495_reading_rtc_target_register_and_current_guest_rtc_index_is_BH();
	rtc_rqmid_24556_reading_rtc_AL_is_keeped_to_be_current_rtc_index_register();
	rtc_rqmid_24558_writing_2bytes_to_target_register_register_has_no_change();
	rtc_rqmid_24387_writing_write_0h_to_guest_al();
}

int main(void)
{
	print_case_list();
	test_rtc();
	return report_summary();
}
