#include "../rtc.h"
/*save RTC index in memory during init*/
in $0x70, %al
mov %eax, (RTC_INIT_RTC_INDEX_REG_ADDR)
/* save magic in memory during init.
 * not use, just test */
mov $0xa1a1, %eax
mov %eax, (RTC_INIT_RTC_MAGIC_ADDR)

