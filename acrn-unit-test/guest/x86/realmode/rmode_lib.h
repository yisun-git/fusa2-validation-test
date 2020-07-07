#ifndef __RMODE_LIB_H__
#define __RMODE_LIB_H__

#define xstr(s...) xxstr(s)
#define xxstr(s...) #s

/* Because the first page is used for AP startup,
 * save the exception vector rflags error code to page 16 (this page is not used)
 */
#define EXCEPTION_ADDR	0xF004
#define EXCEPTION_VECTOR_ADDR	EXCEPTION_ADDR
#define EXCEPTION_RFLAGS_ADDR	0xF005
#define EXCEPTION_ECODE_ADDR	0xF006

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;

#define ASM_TRY(catch)				\
	"movw $0, %gs:"xstr(EXCEPTION_VECTOR_ADDR)" \n\t"			\
	"movw $0, %gs:"xstr(EXCEPTION_ECODE_ADDR)" \n\t"			\
	".pushsection .data.ex \n\t"		\
	".word 1111f," catch "\n\t"		\
	".popsection \n\t"			\
	"1111:"
struct excp_record {
	u16 ip;
	u16 handler;
};

struct excp_regs {
	u16 ax, cx, dx, bx;
	u16 bp, si, di;
	u16 vector;
	u16 error_code;
	u16 ip;
	u16 cs;
	u16 rflags;
};

void print_serial(const char *buf);
void print_serial_u32(u32 value);
void report(const char *name, _Bool ok);
void report_summary(void);
u8 exception_vector(void);
u16 exception_error_code(void);
#endif
