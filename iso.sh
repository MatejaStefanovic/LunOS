#!/bin/sh
set -e
. ./build.sh

# Create ISO directory structure
mkdir -p isodir

# Copy kernel to ISO root (Limine expects it here)
cp sysroot/boot/LunOS.kernel isodir/LunOS.kernel

# Create Limine configuration
cat > isodir/limine.conf << EOF
# Limine Configuration
timeout: 0

# Kernel entry
/LunOS
protocol: limine
path: boot():/LunOS.kernel
EOF

# Copy Limine bootloader files
cp /usr/share/limine/limine-bios.sys isodir/
cp /usr/share/limine/limine-bios-cd.bin isodir/
cp /usr/share/limine/limine-uefi-cd.bin isodir/

# Create ISO using xorriso
xorriso -as mkisofs -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    isodir -o LunOS.iso

# Install Limine bootloader to ISO
limine bios-install LunOS.iso

echo "ISO created: LunOS.iso"
