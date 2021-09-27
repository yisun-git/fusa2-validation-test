#include "../avx.h"

/*xsave the reg to XSAVE_INIT_ADDR*/
mov % cr4, % eax
bts $18, % eax
mov % eax, % cr4
/*enable X87|SSE|AVX. component*/
mov $0, % ecx
mov $0, % edx
mov $7, % eax
xsetbv
xsave (XSAVE_INIT_ADDR)
