#!/bin/sh
set -e
. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -s -no-shutdown -d cpu_reset -cdrom LunOS.iso
qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom LunOS.iso
