#include "libcflat.h"
#include "smp.h"
#include "asm/io.h"
#include "asm/page.h"
#include "vmalloc.h"
#include "pci.h"
#include "asm/pci.h"
#include <linux/pci_regs.h>
#ifndef USE_SERIAL
#define USE_SERIAL
#endif

static struct spinlock lock;
static int serial_inited = 0;

#ifdef IN_NATIVE
#define USE_SERIAL_MMIO

#if defined(USE_SERIAL_MMIO)
#define SERIAL_MMIOBASE (0x81667000)
#endif

#if defined(USE_SERIAL_PCI)
#define SERIAL_PCI_BDF ((0x1e << 3) + 0x00) /* 00:1e.0 */
#endif

#if defined(USE_SERIAL_PIO)
#define SERIAL_IOBASE (0x2f8) /* RPL NUC */
#endif

#else
#define USE_SERIAL_PIO
#define SERIAL_IOBASE (0x3f8) /* ACRN */
#endif // #ifdef IN_NATIVE

enum serial_dev_type {
	INVALID,
	PIO,
	PCI,
	MMIO,
};

struct serial_dev {
	enum serial_dev_type type;
	u16 serial_iobase;
	phys_addr_t serial_mmio_paddr;
	void __iomem *serial_mmiobase;
	pcidevaddr_t pci_bdf;

	u32 reg_width;
};

#if defined(USE_SERIAL_PIO)
static struct serial_dev uart = {
	.type = PIO,
	.serial_iobase = SERIAL_IOBASE,
	.reg_width = 1,
};
#elif defined(USE_SERIAL_PCI)
static struct serial_dev uart = {
	.type = PCI,
	.pci_bdf = SERIAL_PCI_BDF,
	.reg_width = 4,
};
#elif defined(USE_SERIAL_MMIO)
static struct serial_dev uart = {
	.type = MMIO,
	.serial_mmio_paddr = SERIAL_MMIOBASE,
	.reg_width = 4,
};
#endif

static inline u8 mmio_read8(const volatile void *addr)
{
	return *(const volatile u8 *)addr;
}

static u32 mmio_read32(const volatile void *addr)
{
	return *(const volatile u32 *)addr;
}

static void mmio_write8(u8 val, volatile void *addr)
{
	*(volatile u8 *)addr = val;
}

static void mmio_write32(u32 val, volatile void *addr)
{
	*(volatile u32 *)addr = val;
}

static inline u8 serial_read(u8 reg)
{
	u32 ret;

	if (uart.type == PIO) {
		ret = inb(uart.serial_iobase + reg);
	} else {
		if (uart.reg_width == 4) {
			ret = mmio_read32(uart.serial_mmiobase + reg * uart.reg_width);
		} else {
			ret = mmio_read8(uart.serial_mmiobase + reg * uart.reg_width);
		}
	}

	return (u8)ret;
}

static inline void serial_write(u8 val, u8 reg)
{
	if (uart.type == PIO) {
		outb(val, uart.serial_iobase + reg);
	} else {
		if (uart.reg_width == 4) {
			mmio_write32(val, uart.serial_mmiobase + reg * uart.reg_width);
		} else {
			mmio_write8(val, uart.serial_mmiobase + reg * uart.reg_width);
		}
	}
}

static void serial_putchar(char ch)
{
	u8 lsr;

	do {
		lsr = serial_read(0x05);
	} while (!(lsr & 0x20));

	serial_write(ch, 0x00);
}

static void serial_put(char ch)
{
	/* Force carriage return to be performed on \n */
	if (ch == '\n') {
		serial_putchar('\r');
	}
	serial_putchar(ch);
}

void serial_pci_init(void)
{
	struct pci_dev pci_dev;
	u16 cmd;
	bool ret;

	if (uart.type != PCI)
		return;

	pci_dev_init(&pci_dev, uart.pci_bdf);
	ret = pci_bar_is_valid(&pci_dev, 0);
	assert(ret);

	cmd = pci_config_readw(pci_dev.bdf, PCI_COMMAND);
	if (!pci_bar_is_memory(&pci_dev, 0)) {
		uart.type = PIO;
		uart.serial_iobase = (u16)pci_bar_get_addr(&pci_dev, 0);
		uart.reg_width = 1;
		cmd |= PCI_COMMAND_IO;
	} else {
		uart.type = MMIO;
		uart.serial_mmio_paddr = pci_bar_get_addr(&pci_dev, 0);
		uart.serial_mmiobase = (void *)(intptr_t)uart.serial_mmio_paddr;
		assert(uart.serial_mmiobase);
		cmd |= PCI_COMMAND_MEMORY;
	}
	pci_config_writew(pci_dev.bdf, PCI_COMMAND, cmd);
}

static void serial_init(void)
{
	u8 lcr;

	if (uart.type == PCI) {
		serial_pci_init();
	}
	else if (uart.type == MMIO) {
		uart.serial_mmiobase = (void *)(intptr_t)uart.serial_mmio_paddr;
		assert(uart.serial_mmiobase);
	}

	/* set DLAB */
	lcr = serial_read(0x03);
	lcr |= 0x80;
	serial_write(lcr, 0x03);

	/* set baud rate to 115200 */
	serial_write(0x01, 0x00);
	serial_write(0x00, 0x01);

	/* clear DLAB */
	lcr = serial_read(0x03);
	lcr &= ~0x80;
	serial_write(lcr, 0x03);

	/* IER: disable interrupts */
	serial_write(0x00, 0x01);
	/* LCR: 8 bits, no parity, one stop bit */
	serial_write(0x03, 0x03);
	/* FCR: disable FIFO queues */
	serial_write(0x00, 0x02);
	/* MCR: RTS, DTR on */
	serial_write(0x03, 0x04);
}

/*
 * after paging enabled, this function is called to remap serial_mmiobase
 * it is not necessary for 64bit native, because mmio address is 1:1
 * maped after paging enable.
 */
#ifndef __x86_64__
void serial_mmio_remap(pgd_t *cr3)
{
#ifdef IN_NATIVE
	install_page(cr3, (u32)uart.serial_mmiobase,
				uart.serial_mmiobase);
#endif
}
#endif

static void print_serial(const char *buf)
{
	unsigned long len = strlen(buf);
#ifdef USE_SERIAL
	unsigned long i;
	if (!serial_inited) {
		serial_init();
		serial_inited = 1;
	}

	for (i = 0; i < len; i++) {
		serial_put(buf[i]);
	}
#else
	asm volatile ("rep/outsb" : "+S"(buf), "+c"(len) : "d"(0xf1));
#endif
}

void puts(const char *s)
{
	spin_lock(&lock);
	print_serial(s);
	spin_unlock(&lock);
}

void exit(int code)
{
	while (1);
#ifdef USE_SERIAL
	static const char shutdown_str[8] = "Shutdown";
	int i;

	/* test device exit (with status) */
	outl(code, 0xf4);

	/* if that failed, try the Bochs poweroff port */
	for (i = 0; i < 8; i++) {
		outb(shutdown_str[i], 0x8900);
	}
#else
	asm volatile("out %0, %1" : : "a"(code), "d"((short)0xf4));
#endif
	while (1);
}

void __iomem *ioremap(phys_addr_t phys_addr, size_t size)
{
	phys_addr_t base = phys_addr & PAGE_MASK;
	phys_addr_t offset = phys_addr - base;

	/*
	 * The kernel sets PTEs for an ioremap() with page cache disabled,
	 * but we do not do that right now. It would make sense that I/O
	 * mappings would be uncached - and may help us find bugs when we
	 * properly map that way.
	 */
	return vmap(phys_addr, size) + offset;
}
