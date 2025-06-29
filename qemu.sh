#!/bin/sh
set -e
. ./iso.sh

# Use qemu-system-x86_64 for x86_64 architecture
# Add more memory and enable KVM if available for better performance
#qemu-system-x86_64 -cdrom LunOS.iso -m 512M -enable-kvm 2>/dev/null || \
#qemu-system-x86_64 -cdrom LunOS.iso -m 512M -monitor stdio
#qemu-system-x86_64 -cdrom LunOS.iso -m 512M -smp 4

qemu-system-x86_64 -bios /usr/share/OVMF/x64/OVMF.4m.fd -cdrom LunOS.iso -m 512M -smp 4 -cpu host -enable-kvm

#qemu-system-x86_64 -bios /usr/share/OVMF/x64/OVMF.4m.fd -cdrom LunOS.iso -m 512M -smp 4 -cpu host -enable-kvm -s -S
