#!/bin/sh
set -e
. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -s -d cpu_reset -kernel kernel/LunOS.kernel
qemu-system-x86_64 -bios /usr/share/OVMF/x64/OVMF.4m.fd -cdrom LunOS.iso -m 512M -smp 4 -cpu host -enable-kvm
