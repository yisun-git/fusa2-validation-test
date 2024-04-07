#include "../rtc.h"
/*save RTC index in memory during startup*/
in $0x70, %al
mov %eax, (RTC_STARTUP_RTC_INDEX_REG_ADDR)
/* save magic in memory during startup.
 * not use, just test */
mov $0xafaf, %eax
mov %eax, (RTC_STARTUP_MAGIC_ADDR)
