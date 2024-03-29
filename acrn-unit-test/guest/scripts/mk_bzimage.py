#!/usr/bin/env python

import sys, os
import argparse
import struct

version_string = "acrn-unit-test"

def main(args):
    out_f = open(args.out_file, "wb")
    binary_f = open(args.raw_file, "rb")
    binary_buf = binary_f.read()

    # Sector #1

    # # till 01F0: 0's
    data = b'\x00'
    out_f.write(data * 0x1f1)

    # # 01F1/1: The size of the setup in sectors
    data = b'\x02'
    out_f.write(data)

    # # remaining till last 2 bytes of sector #1: 0's
    data = b'\x00'
    out_f.write(data * (0x200 - 2 - 0x1f2))
    data = b'\x55\xAA'
    out_f.write(data)

    # Sector #2: zero page header

    # 0200/2: jump instruction
    data = b'\xEB\x66'
    out_f.write(data)

    # 0202/4: magic signature "HdrS"
    data = b'HdrS'
    out_f.write(data)

    # 0206/2: boot protocol version: 2.10
    data = b'\x0A\x02'
    out_f.write(data)

    # 0208/4: realmode switch
    data = b'\x00'
    out_f.write(data * 4)

    # 020C/2 (obsolete)
    data = b'\x00'
    out_f.write(data * 2)

    # 020E/2: Pointer to kernel version string at the third sector
    data = b'\x00\x02'
    out_f.write(data)

    # 0210/1: Boot loader identifier (set by boot loader)
    data = b'\x00'
    out_f.write(data)

    # 0211/1: Boot protocol option flags: load at 1M
    data = b'\x01'
    out_f.write(data)

    # 0212/2: (for real-mode kernel)
    data = b'\x00'
    out_f.write(data * 2)

    # 0214/4: code32_start: 1M
    data = b'\x00\x00\x10\x00'
    out_f.write(data)

    # 0218/4: ramdisk_image (set by boot loader)
    data = b'\x00'
    out_f.write(data * 4)

    # 021C/4: initrd size (set by boot loader)
    data = b'\x00'
    out_f.write(data * 4)

    # 0220/4: DO NOT USE
    data = b'\x00'
    out_f.write(data * 4)

    # 0224/2: Free memory after setup end (write)
    data = b'\x00'
    out_f.write(data * 2)

    # 0226/1: Extended boot loader version
    data = b'\x00'
    out_f.write(data)

    # 0227/1: Extended boot loader ID
    data = b'\x00'
    out_f.write(data)

    # 0228/4: 32-bit pointer to the kernel command line (write)
    data = b'\x00'
    out_f.write(data * 4)

    # 022C/4: Highest legal initrd address (2G - 1)
    data = b'\xFF\xFF\xFF\x7F'
    out_f.write(data)

    # 0230/4: Physical addr alignment required for kernel (reloc)
    data = b'\x00\x00\x00\x00'
    out_f.write(data)

    # 0234/1: Whether kernel is relocatable or not (reloc)
    data = b'\x00'
    out_f.write(data)

    # 0235/1 (2.10+): Minimum alignment, as a power of two (reloc)
    data = b'\x00'
    out_f.write(data)

    # 0236/2 (2.12+): Boot protocol option flags
    data = b'\x00'
    out_f.write(data * 2)

    # 0238/4: Maximum size of the kernel command line (0x07FF)
    data = b'\xFF\x07\x00\x00'
    out_f.write(data)

    # 023C/4 (2.07+): Hardware subarchitecture
    # 0240/8 (2.07+): Subarchitecture-specific data
    # Both defaults to 0
    data = b'\x00'
    out_f.write(data * (4 + 8))

    # 0248/4 (2.08+): Offset of kernel payload
    # 024C/4 (2.08+): Length of kernel payload
    # Both 0
    data = b'\x00'
    out_f.write(data * (4 + 4))

    # 0250/8 (2.09+): 64-bit physical pointer to linked list
    data = b'\x00'
    out_f.write(data * 8)

    # 0258/8 (2.10+): Preferred loading address
    #
    # ACRN unit test images are statically linked at 4M and prepended by 4
    # sectors (i.e. 2K). Tell a bootloader that the preferred load address is 4M - 2K.
    entry_offset = int(args.entry_offset, 16)
    pref_addr = entry_offset - 512 * 1
    for b in [((pref_addr >> i) & 0xff) for i in (0,8,16,24)]:
        data = str(chr(b))
        a = struct.pack('B', b)
        out_f.write(a)

    data = b'\x00'
    out_f.write(data * 4)

    # 0260/4 (2.10+): Linear memory required during initialization
    binary_size = len(binary_buf) + 0x800
    for b in [((binary_size >> i) & 0xff) for i in (0,8,16,24)]:
        data = str(chr(b))
        a = struct.pack('B', b)
        out_f.write(a)

    # 0264/4 (2.11+): Offset of handover entry point
    data = b'\x00'
    out_f.write(data * 4)

    # remaining: all 0
    data = b'\x00'
    out_f.write(data * (0x400 - 0x268))

    # Sector #3: The version string at 0400, max 0200 bytes
    data = version_string.encode('utf-8')
    out_f.write(data)
    data = b'\x00'
    out_f.write(data * (0x200 - len(version_string)))

    # Sector $4: relocator code
    with open(args.relocator, "rb") as in_f:
        buf = in_f.read()
        max_size = 0x200 - 4
        if len(buf) > max_size:
            #print "Relocator too large: %d bytes (max %d bytes)" % (len(buf), max_size)
            sys.exit(1)
        out_f.write(buf)
        data = b'\x00'
        out_f.write(data * (max_size - len(buf)))

    # 07FC/4 (customized): size of the binary
    binary_size = len(binary_buf)
    for b in [((binary_size >> i) & 0xff) for i in (0,8,16,24)]:
        data = str(chr(b))
        a = struct.pack('B', b)
        out_f.write(a)

    # Sector #5 and onwards: The binary
    out_f.write(binary_buf)

    out_f.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert an test in raw to a bzImage")
    parser.add_argument("raw_file")
    parser.add_argument("relocator")
    parser.add_argument("out_file")
    parser.add_argument("entry_offset")
    args = parser.parse_args()

    main(args)
