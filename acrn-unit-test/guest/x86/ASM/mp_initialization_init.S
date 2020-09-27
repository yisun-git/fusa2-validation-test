#include "../mp_initialization.h"
mov $stacktop, %esp
call 11f
11:
pop %dx
mov %dx, MEM_ADDR_SAVE_EIP_VLAUE
mov %cs, MEM_ADDR_SAVE_CS_VALUE