DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=
DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc
ARCHDIR=arch/$(HOSTARCH)
LIMINE_CFG = limine.cfg

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/kernel.o \
kernel/console/tty/tty.o \
kernel/console/logging/klogging.o \
kernel/tests/vmm_tests.o \

OBJS=\
$(KERNEL_OBJS) \

LINK_LIST=\
$(LDFLAGS) \
$(KERNEL_OBJS) \
$(LIBS) \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: LunOS.kernel

LunOS.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)

$(ARCHDIR)/boot/crt/crtbegin.o $(ARCHDIR)/boot/crt/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

iso: LunOS.kernel $(LIMINE_CFG)
	mkdir -p isodir
	cp LunOS.kernel isodir/
	cp $(LIMINE_CFG) isodir/
	cp /usr/share/limine/limine-bios.sys isodir/
	cp /usr/share/limine/limine-bios-cd.bin isodir/
	cp /usr/share/limine/limine-uefi-cd.bin isodir/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		isodir -o LunOS.iso
	limine bios-install LunOS.iso


clean:
	rm -f LunOS.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: LunOS.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp LunOS.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
