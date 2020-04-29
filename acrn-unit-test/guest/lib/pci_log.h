#ifndef __PCI_LOG_H__
#define __PCI_LOG_H__

enum log_level {
	DBG_LVL_ERROR = 0,
	DBG_LVL_WARN,
	DBG_LVL_INFO
};
static volatile char dbg_lvl = DBG_LVL_ERROR;
#define DBG_ERRO(...) do {\
	if (dbg_lvl >= DBG_LVL_ERROR) {\
		printf("\n");\
		printf("\033[31;5m");\
		printf("  [ERROR]");\
		printf("\033[0m");\
		printf("\033[31m");\
		printf(__VA_ARGS__);\
		printf(" <%s,%s,%d>", __FILE__, __FUNCTION__, __LINE__);\
		printf("\033[0m");\
		printf("\r\n");\
	} \
} while (0)

#define DBG_WARN(...) do {\
	if (dbg_lvl >= DBG_LVL_WARN) {\
		printf("\n");\
		printf("\033[33m");\
		printf("  [WARN]");\
		printf("\033[0m");\
		printf("\033[33m");\
		printf(__VA_ARGS__);\
		printf(" <%s,%s,%d>", __FILE__, __FUNCTION__, __LINE__);\
		printf("\033[0m");\
		printf("\r\n");\
	} \
} while (0)

#define DBG_INFO(...) do {\
	if (dbg_lvl >= DBG_LVL_INFO) {\
		printf("\n");\
		printf("\033[32m");\
		printf("  [INFO]");\
		printf("\033[0m");\
		printf(__VA_ARGS__);\
		printf(" <%s,%s,%d>", __FILE__, __FUNCTION__, __LINE__);\
		printf("\r\n");\
	} \
} while (0)

static inline void set_log_level(enum log_level lev)
{
	dbg_lvl = lev;
}
#endif