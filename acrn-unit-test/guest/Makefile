
SHELL := /usr/bin/env bash

ifeq ($(wildcard config.mak),)
$(error run ./configure first. See ./configure -h)
endif

export ENTRY_OFFSET = 0x4000800
export START_A = 0x4000800 + SIZEOF_HEADERS

FILE_NAME_HSI = $(shell echo $(FILE_NAME) | grep "hsi_mem_paging_access_low")
ifneq ($(FILE_NAME_HSI), )
	export ENTRY_OFFSET = 0x11000800
	export START_A = 0x11000800 + SIZEOF_HEADERS
endif

FILE_NAME_MULTIBOOT = $(shell echo $(FILE_NAME) | grep "multiboot_low")
ifneq ($(FILE_NAME_MULTIBOOT), )
    export ENTRY_OFFSET = 0x11000800
    export START_A = 0x11000800 + SIZEOF_HEADERS
endif

FILE_NAME_GP = $(shell echo $(FILE_NAME) | grep "general_purpose")
FILE_NAME_PAGING = $(shell echo $(FILE_NAME) | grep "paging")
FILE_NAME_TSC =  $(shell echo $(FILE_NAME) | grep "tsc")

ifeq ($(VM), safety)
    export ENTRY_OFFSET = 0x100000
    export START_A = 1M
endif

ifeq ($(V8086), v8086_mode)
    export ENTRY_OFFSET = 0x4000
    export START_A = 0x4000 + SIZEOF_HEADERS
endif

FILE_NAME_SAFETY_ANALYSIS = $(shell echo $(FILE_NAME) | grep "safety_analysis_low")
ifneq ($(FILE_NAME_SAFETY_ANALYSIS), )
  ifeq ($(VM), safety)
    export ENTRY_OFFSET = 0x11000800
    export START_A = 0x11000800 + SIZEOF_HEADERS
  else ifeq ($(VM), non-safety)
    export ENTRY_OFFSET = 0x10100000
    export START_A = 0x10100000 + SIZEOF_HEADERS
  endif
endif

include config.mak

# Set search path for all sources
VPATH = $(SRCDIR)

libdirs-get = $(shell [ -d "lib/$(1)" ] && echo "lib/$(1) lib/$(1)/asm")
ARCH_LIBDIRS := $(call libdirs-get,$(ARCH)) $(call libdirs-get,$(TEST_DIR))
OBJDIRS := $(ARCH_LIBDIRS)

DESTDIR := $(PREFIX)/share/kvm-unit-tests/

.PHONY: arch_clean clean distclean cscope

#make sure env CFLAGS variable is not used
CFLAGS =

ifeq ($(VM), safety)
    CFLAGS += -DIN_SAFETY_VM
else ifeq ($(VM), non-safety)
    CFLAGS += -DIN_NON_SAFETY_VM
else ifeq ($(NATIVE), yes)
	CFLAGS += -DIN_NATIVE
endif

ifeq ($(QEMU), 1)
	CFLAGS += -DQEMU
endif

ifeq ($(BP_HLT), 1)
	CFLAGS += -DBP_HLT_TEST
endif
ifeq ($(AP_HLT), 1)
	CFLAGS += -DAP_HLT_TEST
endif

ifneq ($(FILE_NAME_GP), )
	CFLAGS += -DIN_GP_TEST
endif

ifneq ($(FILE_NAME_PAGING), )
	CFLAGS += -DIN_PAGING_TEST
endif

ifneq ($(FILE_NAME_TSC), )
	CFLAGS += -DIN_TSC_TEST
endif

libgcc := $(shell $(CC) --print-libgcc-file-name)

libcflat := lib/libcflat.a
cflatobjs := \
	lib/argv.o \
	lib/printf.o \
	lib/string.o \
	lib/abort.o \
	lib/report.o \
	lib/stack.o

# libfdt paths
LIBFDT_objdir = lib/libfdt
LIBFDT_srcdir = $(SRCDIR)/lib/libfdt
LIBFDT_archive = $(LIBFDT_objdir)/libfdt.a
LIBFDT_include = $(addprefix $(LIBFDT_srcdir)/,$(LIBFDT_INCLUDES))
LIBFDT_version = $(addprefix $(LIBFDT_srcdir)/,$(LIBFDT_VERSION))

OBJDIRS += $(LIBFDT_objdir)

#include architecture specific make rules
include $(SRCDIR)/$(TEST_DIR)/Makefile

# cc-option
# Usage: OP_CFLAGS+=$(call cc-option, -falign-functions=0, -malign-functions=0)

cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
              > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

ifeq ($(V8086), v8086_mode)
COMMON_CFLAGS += -fvisibility=hidden -s $(autodepend-flags)
else
COMMON_CFLAGS += -g $(autodepend-flags)
endif
COMMON_CFLAGS += -Wall -Wwrite-strings -Wclobbered -Wempty-body -Wuninitialized
COMMON_CFLAGS += -Wignored-qualifiers -Wunused-but-set-parameter
COMMON_CFLAGS += -Werror

#The hypervisor triple fault when sse is used for 64 bit operations in protected mode,
#so I removed sse
COMMON_CFLAGS += -mno-sse
frame-pointer-flag=-f$(if $(KEEP_FRAME_POINTER),no-,)omit-frame-pointer
fomit_frame_pointer := $(call cc-option, $(frame-pointer-flag), "")
fnostack_protector := $(call cc-option, -fno-stack-protector, "")
fnostack_protector_all := $(call cc-option, -fno-stack-protector-all, "")
wno_frame_address := $(call cc-option, -Wno-frame-address, "")
fno_pic := $(call cc-option, -fno-pic, "")
no_pie := $(call cc-option, -no-pie, "")
COMMON_CFLAGS += $(fomit_frame_pointer)
COMMON_CFLAGS += $(fno_stack_protector)
COMMON_CFLAGS += $(fno_stack_protector_all)
COMMON_CFLAGS += $(wno_frame_address)
COMMON_CFLAGS += $(if $(U32_LONG_FMT),-D__U32_LONG_FMT__,)
COMMON_CFLAGS += $(fno_pic) $(no_pie)

CFLAGS += $(COMMON_CFLAGS)
CFLAGS += -Wmissing-parameter-type -Wold-style-declaration -Woverride-init

CXXFLAGS += $(COMMON_CFLAGS)

autodepend-flags = -MMD -MF $(dir $*).$(notdir $*).d

LDFLAGS += $(CFLAGS)

$(libcflat): $(cflatobjs)
	$(AR) rcs $@ $^

include $(LIBFDT_srcdir)/Makefile.libfdt
$(LIBFDT_archive): CFLAGS += -ffreestanding -I $(SRCDIR)/lib -I $(SRCDIR)/lib/libfdt -Wno-sign-compare
$(LIBFDT_archive): $(addprefix $(LIBFDT_objdir)/,$(LIBFDT_OBJS))
	$(AR) rcs $@ $^


# Build directory target
.PHONY: directories
directories:
	@mkdir -p $(OBJDIRS)

%.o: %.S
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $<

-include */.*.d */*/.*.d

all: directories $(shell cd $(SRCDIR) && git rev-parse --verify --short=8 HEAD >$(PWD)/build-head 2>/dev/null)

standalone: all
	@scripts/mkstandalone.sh

stitched: $(TEST_DIR)/stitched.bzimage

install: standalone
	mkdir -p $(DESTDIR)
	install tests/* $(DESTDIR)

clean: arch_clean
	$(RM) lib/.*.d $(libcflat) $(cflatobjs)
ifeq ($(TEST_DIR), x86)
	$(RM) $(TEST_DIR)/realmode/.*.d
	$(RM) $(TEST_DIR)/realmode/*.o
	$(RM) $(TEST_DIR)/realmode/*.elf
	$(RM) $(TEST_DIR)/realmode/*.raw
	$(RM) $(TEST_DIR)/realmode/*.bzimage
endif

libfdt_clean:
	$(RM) $(LIBFDT_archive) \
	$(addprefix $(LIBFDT_objdir)/,$(LIBFDT_OBJS)) \
	$(LIBFDT_objdir)/.*.d

distclean: clean libfdt_clean
	$(RM) $(TEST_DIR)-run msr.out cscope.* build-head
	$(RM) -r tests logs logs.old

cscope: cscope_dirs = lib lib/libfdt lib/linux $(TEST_DIR) $(ARCH_LIBDIRS) lib/asm-generic
cscope:
	$(RM) ./cscope.*
	find -L $(cscope_dirs) -maxdepth 1 \
		-name '*.[chsS]' -exec realpath --relative-base=$(PWD) {} \; | sort -u > ./cscope.files
	cscope -bk

-include scripts/testing.mk
