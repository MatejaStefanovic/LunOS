#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/LunOS.kernel isodir/boot/LunOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "LunOS" {
	multiboot /boot/LunOS.kernel
}
EOF
grub-mkrescue -o LunOS.iso isodir
