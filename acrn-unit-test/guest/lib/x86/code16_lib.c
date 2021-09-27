#ifndef CODE16_LIB
#define CODE16_LIB

#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#ifdef USE_SERIAL
static u16 serial_iobase = 0x3f8;
static u16 serial_inited;

static void outb(u8 data, u16 port)
{
	asm volatile("out %0, %1" : : "a"(data), "d"(port));
}

static u8 inb(u16 port)
{
	u8 data;

	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static void serial_outb(char ch)
{
	u8 lsr;

	do {
		lsr = inb(serial_iobase + 0x05);
	} while (!(lsr & 0x20));

	outb(ch, serial_iobase + 0x00);
}

static void serial_put(char ch)
{
	/* Force carriage return to be performed on \n */
	if (ch == '\n')
		serial_outb('\r');
	serial_outb(ch);
}

static void serial_init(void)
{
	u8 lcr;

	/* set DLAB */
	lcr = inb(serial_iobase + 0x03);
	lcr |= 0x80;
	outb(lcr, serial_iobase + 0x03);

	/* set baud rate to 115200 */
	outb(0x01, serial_iobase + 0x00);
	outb(0x00, serial_iobase + 0x01);

	/* clear DLAB */
	lcr = inb(serial_iobase + 0x03);
	lcr &= ~0x80;
	outb(lcr, serial_iobase + 0x03);

	/* IER: disable interrupts */
	outb(0x00, serial_iobase + 0x01);
	/* LCR: 8 bits, no parity, one stop bit */
	outb(0x03, serial_iobase + 0x03);
	/* FCR: disable FIFO queues */
	outb(0x00, serial_iobase + 0x02);
	/* MCR: RTS, DTR on */
	outb(0x03, serial_iobase + 0x04);
}

static void serial_print(const char *buf, int len)
{
	if (!serial_inited) {
		serial_init();
		serial_inited = 1;
	}

	for (int i = 0; i < len; i++)
		serial_put(buf[i]);
}
#else
static void serial_print(const char *buf, int len)
{
	(void)buf;
	(void)len;
}
#endif

static u32 fill_str(char *dest, const char *src)
{
	u32 len = 0;
	char *d = dest;
	const char *s = src;

	while (*s) {
		*d++ = *s++;
		len++;
	}
	return len;
}

static const char digits[] = "0123456789abcdef";
static u32 fill_hex(char *buf, u32 num)
{
	char hex[8 + 1]; /* the max bits(8) of int(0xffffffff)  + EOS */
	char *ptr = hex + sizeof(hex) - 1;

	*ptr = '\0';
	do {
		*--ptr = digits[num & 0xf];
		num >>= 4;
	} while (num > 0);
	return fill_str(buf, ptr);
}

static u32 fill_dec(char *buf, u32 num)
{
	char decimal[10 + 1];  /* the max bits(10) of int(4294967295)  + EOS */
	char *ptr = decimal + sizeof(decimal) - 1;

	*ptr = '\0';
	do {
		*--ptr = '0' + (num % 10);
		num /= 10;
	} while (num > 0);
	return fill_str(buf, ptr);
}

/* only %%, %d, %u, %x and %s are supported. */
static void vprint(const char *fmt, va_list va)
{
	#define MAX_BUF_LEN 256
	char buffer[MAX_BUF_LEN];
	const char *str = fmt;
	char *buf = buffer;

	while (*str && (buffer + MAX_BUF_LEN > buf)) {
		if ('%' == *str) {
			switch (*++str) {
			/* handle the percent character(%) in the string */
			case '%':
				*buf++ = '%';
				break;
			case 'd':
			case 'u':
				buf += fill_dec(buf, va_arg(va, u32));
				break;
			case 'x':
				buf += fill_hex(buf, va_arg(va, u32));
				break;
			case 's':
				buf += fill_str(buf, va_arg(va, const char *));
				break;
			default: /* unsupported format */
				buf = buffer;
				buf += fill_str(buf, "unsupported format: %");
				*buf++ = *str;
				*buf++ = '\n';
				goto end;
			}
		} else {
			*buf++ = *str;
		}
		str++;
	}
end:
	*buf = '\0';
	serial_print(buffer, buf - buffer);
}

#ifndef PRINT_NAME
#define PRINT_NAME printf
#endif
int PRINT_NAME(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vprint(fmt, va);
	va_end(va);
	return 0;
}

#endif
