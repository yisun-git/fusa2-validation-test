#ifndef _RTC_H_
#define _RTC_H_

#define RTC_STARTUP_BASE_REG_ADDR			0x6000UL
#define RTC_UNCHANGED_BASE_REG_ADDR			0x7000UL
#define RTC_INIT_BASE_REG_ADDR				0x8000UL

#define RTC_STARTUP_RTC_INDEX_REG_ADDR		RTC_STARTUP_BASE_REG_ADDR
#define RTC_STARTUP_MAGIC_ADDR				(RTC_STARTUP_BASE_REG_ADDR + 0x4)

#define RTC_INIT_RTC_INDEX_REG_ADDR			RTC_INIT_BASE_REG_ADDR
#define RTC_INIT_RTC_MAGIC_ADDR				(RTC_INIT_BASE_REG_ADDR + 0x4)

#endif
