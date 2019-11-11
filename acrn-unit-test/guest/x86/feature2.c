/* common code */




#ifdef __x86_64__
/*test case which should run under 64bit  */
#include "64/feature2_fn.c"
#elif __i386__
/*test case which should run  under 32bit  */
#include "32/feature2_fn.c"
#endif

