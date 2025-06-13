#!/bin/sh
set -e
. ./build.sh

# Create ISO directory structure (including EFI directory for UEFI boot)
mkdir -p isodir/EFI/BOOT

# Copy kernel to ISO root (Limine expects it here)
cp sysroot/boot/LunOS.kernel isodir/LunOS.kernel

# Create Limine configuration
cat > isodir/limine.conf << EOF
# Limine Configuration
timeout: 0
#graphics: yes
#wallpaper: boot():/lundberg.png
resolution: 1920x1080x32

# Kernel entry
/LunOS
protocol: limine
path: boot():/LunOS.kernel
EOF

# Copy Limine configuration to EFI directory as well (for UEFI boot)
cp isodir/limine.conf isodir/EFI/BOOT/limine.conf

# Copy Limine bootloader files for BIOS boot
cp /usr/share/limine/limine-bios.sys isodir/
cp /usr/share/limine/limine-bios-cd.bin isodir/

# Copy Limine bootloader files for UEFI boot
cp /usr/share/limine/limine-uefi-cd.bin isodir/
cp /usr/share/limine/BOOTX64.EFI isodir/EFI/BOOT/

# Copy wallpaper
cp ~/LunOS/lundberg.png isodir/

# Create ISO using xorriso with both BIOS and UEFI support
xorriso -as mkisofs -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    isodir -o LunOS.iso

# Install Limine bootloader to ISO for BIOS boot
limine bios-install LunOS.iso

echo "ISO created: LunOS.iso (supports both BIOS and UEFI boot)"
