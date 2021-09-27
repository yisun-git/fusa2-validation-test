/*
 *  file rtc_test.c
 */
#include "libcflat.h"
#include "x86/asm/io.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"
#include "rtc.h"

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
#define D_INDEX		0xD
#define E_INDEX		0xE
#define B_DEFAULT_VALUE	0x2

static uint8_t read_reg_al()
{
	uint8_t al;

	asm volatile(
		"mov %%al, %0\n\t"
		: "=r"(al));

	return al;
}

static uint16_t read_reg_ax()
{
	uint16_t ax_value;

	asm volatile(
		"mov %%ax, %0\n\t"
		: "=r"(ax_value));

	return ax_value;
}

static uint32_t read_reg_eax()
{
	uint32_t eax_value;

	asm volatile(
		"movl %%eax, %0\n\t"
		: "=r"(eax_value));

	return eax_value;
}

static int bcd_value_check(uint8_t bcd)
{
	int ret = true;
	if ((bcd < 0) || (bcd > 9)) {
		ret = false;
	}

	return ret;
}
static volatile uint8_t init_rtc_index = 0;
static volatile uint8_t rtc_year_value = 0;
static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int ap_start_count = 0;

__unused static void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}


#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

static void rtc_ap_unchanged_case_24601()
{
	if (ap_start_count == 0) {
		/*
		 *set new value to rtc index
		 */
		outb(YEAR_INDEX, RTC_INDEX_REG);
		/*
		 *Because RTC_INDEX_REG is write only, so we read YEAR_INDEX index's
		 * corresponding data to prove it unchange indirectly.
		 */
		rtc_year_value = inb(RTC_TARGET_REG);
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 1) {
		rtc_year_value = inb(RTC_TARGET_REG);
		ap_start_count++;
		wait_ap = 1;
	}
}

void ap_main(void)
{
	/*
	 *test only on the ap 2,other ap return directly
	 */
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	while (1) {
		switch (cur_case_id) {
		case 24601:
			rtc_ap_unchanged_case_24601();
			cur_case_id = 0;
			break;
		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}
#endif


/**
 *  @brief Case name: When a vCPU attempts to write guest RTC target register_001
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

	report("\t\t %s 0x%x 0x%x", (reg8_value == reg8_value_prev), __FUNCTION__, reg8_value, reg8_value_prev);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is greater than DH_001
 *
 * Summary: Set the RTC index to EH, and read the RTC target register,
 * the AL register value is 0.
 *
 */
void rtc_rqmid_24494_reading_rtc_target_register(void)
{
	uint8_t reg8_value;

	outb(E_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", reg8_value == 0, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to write 2 bytes to guest RTC index register_001
 *
 * Summary: RTC When a vCPU attempts to write 2 bytes to guest RTC index register,
 * ACRN hypervisor shall ignore the write_001
 *
 */
void rtc_rqmid_24560_write_2_bytes_to_guest_rtc_index_register(void)
{
	uint8_t reg8_value1;
	uint8_t reg8_value2;

	outb(YEAR_INDEX, RTC_INDEX_REG);
	reg8_value1 = inb(RTC_TARGET_REG);

	outw(0xFFFF, RTC_INDEX_REG);
	reg8_value2 = inb(RTC_TARGET_REG);

	report("\t\t %s 0x%x", (reg8_value1 == reg8_value2),
		__FUNCTION__, reg8_value1);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is BH_001
 *
 * Summary: Set the RTC index to BH, and read the RTC target register,
 * the AL register value is 2.
 *
 */
void rtc_rqmid_24495_reading_rtc_target_register_and_current_guest_rtc_index_is_BH(void)
{
	uint8_t reg_b_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg_b_value = read_reg_al();

	report("\t\t %s 0x%x",
		reg_b_value == B_DEFAULT_VALUE, __FUNCTION__, reg_b_value);
}

/**
 * @brief Case name: When a vCPU attempts to write guest RTC index register_001
 *
 * Summary:  the value of AL is keeped to be current RTC index register after writing guest RTC index register
 *
 */
void rtc_rqmid_24556_reading_rtc_AL_is_keeped_to_be_current_rtc_index_register(void)
{
	uint8_t reg8_value1;

	outb(0xFF, RTC_INDEX_REG);
	reg8_value1 = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value1 == 0xFF), __FUNCTION__, reg8_value1);
}

/**
 * @brief Case name: 2-byte write to port 71H_001
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

	report("\t\t %s 0x%x", (reg16_value_prev == reg16_value), __FUNCTION__, reg16_value);
}

/**
 * @brief Case name: when a vCPU attempts to read guest RTC index register_001
 *
 * Summary:  the value of AL should be always 0H even if RTC index register is set to a different value
 *
 */
void rtc_rqmid_24387_reading_read_0h_from_guest_al(void)
{
	uint8_t reg8_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_INDEX_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest 2 bytes from guest I/O port address 70H_001
 *
 * Summary:  In 64 bit mode, read 2 bytes form guest I/O port address 70H,
 * and then read AX register, the AX should be equal to 0.
 */
void rtc_rqmid_24452_reading_2_bytes_from_70H(void)
{
	uint16_t reg16_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inw(RTC_INDEX_REG);
	reg16_value = read_reg_ax();

	report("\t\t %s 0x%x", (reg16_value == 0), __FUNCTION__, reg16_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest 4 bytes from guest I/O port address 70H_001
 *
 * Summary:  In 64 bit mode, read 4 bytes form guest I/O port address 70H,
 * and then read EAX register, the EAX should be equal to 0.
 */
void rtc_rqmid_24453_reading_4_bytes_from_70H(void)
{
	uint32_t reg16_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inl(RTC_INDEX_REG);
	reg16_value = read_reg_ax();

	report("\t\t %s 0x%x", (reg16_value == 0), __FUNCTION__, reg16_value);
}

/**
 * @brief Case name: When a vCPU attempts to write 4 bytes to guest RTC index register_001
 *
 * Summary:In 64 bit mode, the value of RTC index register has
 * no change after writing 4 bytes to this index register
 */
void rtc_rqmid_24563_writing_4_bytes_to_70H(void)
{
	uint8_t reg8_value1;
	uint8_t reg8_value2;

	outb(YEAR_INDEX, RTC_INDEX_REG);
	reg8_value1 = inb(RTC_TARGET_REG);
	(void)outl(0xFFFFFFFF, RTC_INDEX_REG);
	reg8_value2 = inb(RTC_TARGET_REG);

	report("\t\t %s 0x%x", (reg8_value1 == reg8_value2),
		__FUNCTION__, reg8_value1);
}

void rtc_port_io_70H_71H(const char *fname)
{
	uint8_t reg8_value;

	outb(YEAR_INDEX, 0x72);
	(void)inb(0x73);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x ", (reg8_value == 0xFF),
		fname, reg8_value);
}

/**
 * @brief Case name: ACRN hypervisor shall locate guest RTC
 * target register of any VM at guest I/O port address 71H_001
 *
 * Summary: From other io port will not get RTC value for example 0x72/0x73
 *
 */
void rtc_rqmid_26032_reading_port_71H_register(void)
{
	rtc_port_io_70H_71H(__FUNCTION__);
}

/**
 * @brief Case name: ACRN hypervisor shall locate guest RTC index register
 * of any VM at guest I/O port address 70H_001
 *
 * Summary: From other io port will not get RTC value for example 0x72/0x73
 *
 */
void rtc_rqmid_26030_reading_port_70H_register(void)
{
	rtc_port_io_70H_71H(__FUNCTION__);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 2H_001
 *
 * Summary: Set the RTC index to 2H, and read the RTC target register,
 * the AL register value is minute of current date in BCD representation.
 *
 */
void rtc_rqmid_24474_reading_read_index_2h_minute(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int minute;

	outb(MINUTE_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	minute = (bcd[0] * 10) + bcd[1];
	if ((minute < 0) || (minute > 59)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 5H_001
 *
 * Summary: Set the RTC index to 5H, and read the RTC target register, the AL register value is 0.
 *
 */
void rtc_rqmid_24492_reading_read_index_5h_hour_alarm(void)
{
	uint8_t reg8_value;

	outb(HOUR_ALARM_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: 2-byte read to port 71H
 *
 * Summary: Set the RTC index to BH, and read 2 bytes from the RTC target register,
 * the AX register value is 2H(the zero-extended default value).
 *
 */
void rtc_rqmid_24565_reading_read_2_bytes_from_71H(void)
{
	uint16_t reg16_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inw(RTC_TARGET_REG);
	reg16_value = read_reg_ax();

	report("\t\t %s 0x%x", (reg16_value == B_DEFAULT_VALUE), __FUNCTION__, reg16_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is AH_001
 *
 * Summary: Set the RTC index to AH, and read the RTC target register, the AL register value is 0.
 *
 */
void rtc_rqmid_24493_reading_read_index_ah(void)
{
	uint8_t reg8_value;

	outb(A_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 4H_001
 *
 * Summary: Set the RTC index to 4H, and read the RTC target register,
 * the AL register value is hour of current date in BCD representation.
 *
 */
void rtc_rqmid_24475_reading_read_index_4h_hour(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int minute;

	outb(HOUR_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	minute = (bcd[0] * 10) + bcd[1];
	if ((minute < 0) || (minute > 23)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}


/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 1H_001
 *
 * Summary: Set the RTC index to 1H, and read the RTC target register, the AL register value is 0.
 *
 */
void rtc_rqmid_24490_reading_read_index_1h(void)
{
	uint8_t reg8_value;

	outb(SECOND_ALARM_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 0H_001
 *
 * Summary: Set the RTC index to 0H, and read the RTC target register,
 * the AL register value is second of current date in BCD representation.
 *
 */
void rtc_rqmid_24472_reading_read_index_0H_second(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int second;

	outb(SECOND_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	second = (bcd[0] * 10) + bcd[1];
	if ((second < 0) || (second > 59)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 3H_001
 *
 * Summary: Set the RTC index to 3H, and read the RTC target register, the AL register value is 0.
 *
 */
void rtc_rqmid_24491_reading_read_index_3h(void)
{
	uint8_t reg8_value;

	outb(MINUTE_ALARM_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 7H_001
 *
 * Summary: Set the RTC index to 7H, and read the RTC target register,
 * the AL register value is day of month of current date in BCD representation.
 *
 */
void rtc_rqmid_24487_reading_read_index_7H_day_of_month(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int day;

	outb(DAY_OF_MONTH_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	day = (bcd[0] * 10) + bcd[1];
	if ((day < 0) || (day > 31)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 8H_001
 *
 * Summary: Set the RTC index to 8H, and read the RTC target register,
 * the AL register value is month of current date in BCD representation.
 *
 */
void rtc_rqmid_24488_reading_read_index_8H_month(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int month;

	outb(MONTH_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	month = (bcd[0] * 10) + bcd[1];
	if ((month < 0) || (month > 12)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 9H_001
 *
 * Summary: Set the RTC index to 9H, and read the RTC target register,
 * the AL register value is year of current date in BCD representation.
 *
 */
void rtc_rqmid_24489_reading_read_index_9H_year(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int year;

	outb(YEAR_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	year = (bcd[0] * 10) + bcd[1];
	/*10 year range*/
	if ((year < 20) || (year > 30)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: 4-byte read to port 71H_001
 *
 * Summary: Set the RTC index to BH, and read 4 bytes from the RTC target register,
 * the EAX register value is 2H(the zero-extended default value).
 *
 */
void rtc_rqmid_24567_reading_read_4_bytes_from_71H(void)
{
	uint32_t reg32_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inl(RTC_TARGET_REG);
	reg32_value = read_reg_eax();

	report("\t\t %s 0x%x", (reg32_value == B_DEFAULT_VALUE), __FUNCTION__, reg32_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is 6H_001
 *
 * Summary: Set the RTC index to 6H, and read the RTC target register,
 * the AL register value is day of week of current date in BCD representation.
 *
 */
void rtc_rqmid_24486_reading_read_index_6H_day_of_week(void)
{
	int ret = true;
	uint8_t reg8_value;
	int bcd[2];
	int day;

	outb(DAY_OF_WEEK_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		ret = false;
	}

	day = (bcd[0] * 10) + bcd[1];
	if ((day < 1) || (day > 7)) {
		ret = false;
	}

	report("\t\t %s 0x%x", ret, __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is CH_001
 *
 * Summary: Set the RTC index to CH, and read the RTC target register, the AL register value is 0.
 *
 */
void rtc_rqmid_32340_reading_read_index_ch(void)
{
	uint8_t reg8_value;

	outb(C_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register
 * and the current guest RTC index is DH_001
 *
 * Summary: Set the RTC index to DH, and read the RTC target register, the AL register value is 80H.
 *
 */
void rtc_rqmid_32341_reading_read_index_dh(void)
{
	uint8_t reg8_value;

	outb(D_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg8_value = read_reg_al();

	report("\t\t %s 0x%x", (reg8_value == 0x80), __FUNCTION__, reg8_value);
}

/**
 * @brief Case name: 4-byte write to port 71H_001
 *
 * Summary:  Set the RTC index to BH, read 4 bytes from the RTC target register,
 * the EAX register value is 2H(the zero-extended default value);
 * then write 4 bytes value to the RTC target register, read 4 bytes from the RTC
 * target register again, the EAX register should keep to be 2H.
 *
 */
void rtc_rqmid_24559_writing_4bytes_to_target_register_register_has_no_change(void)
{
	uint32_t reg32_value_prev;
	uint32_t reg32_value;

	outb(B_INDEX, RTC_INDEX_REG);
	reg32_value_prev = inl(RTC_TARGET_REG);
	outl(0xFFFFFFFF, RTC_TARGET_REG);
	reg32_value = inl(RTC_TARGET_REG);

	report("\t\t %s 0x%x", (reg32_value_prev == reg32_value), __FUNCTION__, reg32_value);
}

/**
 * @brief Case name: ACRN hypervisor shall set initial current guest RTC index to 0H following startup_001
 *
 * Summary: Get RTC index register at BP start-up, the RTC index value is 0H.
 */
void rtc_rqmid_24553_rtc_index_startup(void)
{
	volatile u32 *ptr;
	volatile u32 index;
	volatile uint8_t rtc_index;

	ptr = (volatile u32 *)RTC_STARTUP_RTC_INDEX_REG_ADDR;
	index = *ptr;
	rtc_index = (uint8_t)index;

	report("\t\t %s 0x%x", (rtc_index == 0), __FUNCTION__, rtc_index);
}

/**
 * @brief Case name: ACRN hypervisor shall keep current guest RTC index unchanged following INIT_002
 *
 * Summary: At AP receives the first init, save the RTC index value, and the RTC index values should be equal to 0H.
 */
void rtc_rqmid_37835_rtc_index_init_unchange_002(void)
{
	volatile u32 *ptr;
	volatile u32 index;
	volatile uint8_t rtc_index;

	ptr = (volatile u32 *)RTC_INIT_RTC_INDEX_REG_ADDR;
	index = *ptr;
	rtc_index = (uint8_t)index;

	report("\t\t %s 0x%x ", (rtc_index == 0), __FUNCTION__, rtc_index);
}

/**
 * @brief case name:ACRN hypervisor shall keep current guest RTC index unchanged following INIT_001
 *
 * Summary: After AP receives first INIT, set the RTC index value to BH;
 * dump RTC index value, AP INIT again, then dump RTC index value again,
 * two dumps value should be equal.
 */
void __unused rtc_rqmid_24601_rtc_index_init_unchange_001(void)
{
	volatile uint8_t rtc_year_value1;
	volatile uint8_t rtc_year_value2;

	/*
	 *trigger ap_main function entering switch 24601
	 */
	cur_case_id = 24601;
	wait_ap_ready();
	rtc_year_value1 = rtc_year_value;
	/*
	 *send sipi to ap  trigger ap_main function was called to get rtc_year_value again.
	 */
	send_sipi();
	cur_case_id = 24601;
	wait_ap_ready();
	/*
	 *get rtc_year value again after ap reset
	 */
	rtc_year_value2 = rtc_year_value;
	/*
	 *compare init value with unchanged
	 */
	report("\t\t %s", (rtc_year_value1 == rtc_year_value2), __FUNCTION__);
}

int rtc_get_second(int *sec)
{
	uint8_t reg8_value;
	int bcd[2];
	int second;


	outb(SECOND_INDEX, RTC_INDEX_REG);
	reg8_value = inb(RTC_TARGET_REG);

	bcd[0] = reg8_value >> 4;
	bcd[1] = reg8_value & 0xF;
	if (!bcd_value_check(bcd[0]) || !bcd_value_check(bcd[1])) {
		return -1;
	}

	second = (bcd[0] * 10) + bcd[1];
	if (sec) {
		*sec = second;
	}

	return 0;
}

void rtc_current_data_and_time(const char *fun)
{
	int ret = true;
	int second_1;
	int second_2;
	int del;
	int count = 60;

	while (count--) {
		if ((count == 0) || (rtc_get_second(&second_1) != 0)) {
			printf("get second1 error\n");
			return;
		}
		/* Prevent rewinding */
		if (second_1 <= 53) {
			printf("1second=%d\n", second_1);
			break;
		}

		test_delay(1);
	}

	test_delay(5);

	if (rtc_get_second(&second_2) != 0) {
		printf("get second2 error\n");
		return;
	}
	del = second_2 - second_1;
	if ((del > 6) || (del < 4)) {
		ret = false;
	}

	report("\t\t %s %d %d", ret, fun, second_1, second_2);
}

/**
 * @brief Case name: When a vCPU attempts to read guest RTC target register_current time_001
 *
 * Summary: Read the current time under guest, and read it again after 5 seconds of delay.
 * The time difference between the two times is 4-6 seconds (the error range is 1 second).
 *
 */
void rtc_rqmid_24570_physical_platform_current_date_and_time(void)
{
	rtc_current_data_and_time(__FUNCTION__);
}

/**
 * @brief Case name: RTC shall be available on the physical platform_001
 *
 * Summary: Read the current time under native, and read it again after 5 seconds of delay.
 * The time difference between the two times is 4-6 seconds (the error range is 1 second).
 *
 */
void rtc_rqmid_39716_physical_platform_rtc(void)
{
	rtc_current_data_and_time(__FUNCTION__);
}

static void print_case_list(void)
{
#ifndef IN_NATIVE
	printf("\t\t RTC feature case list:\n\r");
	printf("\t\t Segmentation 64-Bits Mode:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 24557u,
		"When a vCPU attempts to write guest RTC target register_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24494u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is greater than BH_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24560u,
		"When a vCPU attempts to write 2 bytes to guest RTC index register_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24495u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is BH_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24556u,
		"When a vCPU attempts to write guest RTC index register_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24558u, "2-byte write to port 71H_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24387u,
		"when a vCPU attempts to read guest RTC index register_001");
#endif
}

static void print_case_list_scaling(void)
{
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 39716u,
		"RTC shall be available on the physical platform_001");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 24452u,
		"When a vCPU attempts to read guest 2 bytes from guest I/O port address 70H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24453u,
		"When a vCPU attempts to read guest 4 bytes from guest I/O port address 70H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24563u,
		"When a vCPU attempts to write 4 bytes to guest RTC index register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26032u,
		"ACRN hypervisor shall locate guest RTC target register of any VM at guest I/O port address 71H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26030u,
		"ACRN hypervisor shall locate guest RTC index register of any VM at guest I/O port address 70H_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24474u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 2H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24492u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 5H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24565u, "2-byte read to port 71H");
	printf("\t\t Case ID:%d case name:%s\n\r", 24493u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is AH_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24475u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 4H_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24490u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24472u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 0H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24491u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 3H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24487u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 7H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24488u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 8H_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24489u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 9H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24567u, "4-byte read to port 71H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24486u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is 6H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32340u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is CH_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 32341u,
		"When a vCPU attempts to read guest RTC target register and the current guest RTC index is DH_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 24559u, "4-byte write to port 71H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24553u,
		"ACRN hypervisor shall set initial current guest RTC index to 0H following startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 24570u,
		"When a vCPU attempts to read guest RTC target register_current time_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 37835u,
		"ACRN hypervisor shall keep current guest RTC index unchanged following INIT_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 24601u,
		"ACRN hypervisor shall keep current guest RTC index unchanged following INIT_001");
#endif
#endif
}

static void test_scaling_rtc(void)
{
#ifdef IN_NATIVE
	rtc_rqmid_39716_physical_platform_rtc();
#else
	rtc_rqmid_24452_reading_2_bytes_from_70H();
	rtc_rqmid_24453_reading_4_bytes_from_70H();
	rtc_rqmid_24563_writing_4_bytes_to_70H();
	rtc_rqmid_26032_reading_port_71H_register();
	rtc_rqmid_26030_reading_port_70H_register();

	rtc_rqmid_24474_reading_read_index_2h_minute();
	rtc_rqmid_24492_reading_read_index_5h_hour_alarm();
	rtc_rqmid_24565_reading_read_2_bytes_from_71H();
	rtc_rqmid_24493_reading_read_index_ah();
	rtc_rqmid_24475_reading_read_index_4h_hour();

	rtc_rqmid_24490_reading_read_index_1h();
	rtc_rqmid_24472_reading_read_index_0H_second();
	rtc_rqmid_24491_reading_read_index_3h();
	rtc_rqmid_24487_reading_read_index_7H_day_of_month();
	rtc_rqmid_24488_reading_read_index_8H_month();

	rtc_rqmid_24489_reading_read_index_9H_year();
	rtc_rqmid_24567_reading_read_4_bytes_from_71H();
	rtc_rqmid_24486_reading_read_index_6H_day_of_week();
	rtc_rqmid_32340_reading_read_index_ch();
	rtc_rqmid_32341_reading_read_index_dh();

	rtc_rqmid_24559_writing_4bytes_to_target_register_register_has_no_change();
	rtc_rqmid_24553_rtc_index_startup();
	rtc_rqmid_24570_physical_platform_current_date_and_time();

#ifdef IN_NON_SAFETY_VM
	/*Must be in front of 24601 */
	rtc_rqmid_37835_rtc_index_init_unchange_002();
	rtc_rqmid_24601_rtc_index_init_unchange_001();
#endif
#endif
}

static void test_sample_rtc(void)
{
#ifndef IN_NATIVE
	rtc_rqmid_24557_write_guest_rtc_target_register();
	rtc_rqmid_24494_reading_rtc_target_register();
	rtc_rqmid_24560_write_2_bytes_to_guest_rtc_index_register();
	rtc_rqmid_24495_reading_rtc_target_register_and_current_guest_rtc_index_is_BH();
	rtc_rqmid_24556_reading_rtc_AL_is_keeped_to_be_current_rtc_index_register();
	rtc_rqmid_24558_writing_2bytes_to_target_register_register_has_no_change();
	rtc_rqmid_24387_reading_read_0h_from_guest_al();
#endif
}

int main(void)
{
	print_case_list();
	print_case_list_scaling();

	test_sample_rtc();
	test_scaling_rtc();

	return report_summary();
}
