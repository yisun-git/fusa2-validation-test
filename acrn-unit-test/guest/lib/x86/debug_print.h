#ifndef __DEBUG_PRINT_H__
#define __DEBUG_PRINT_H__

/*#define USE_DEBUG*/
#ifdef USE_DEBUG
#define debug_print(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)
#else
#define debug_print(fmt, args...)
#endif
#define debug_error(fmt, args...)	printf("[%s:%s] line=%d "fmt"", __FILE__, __func__, __LINE__,  ##args)

#endif
