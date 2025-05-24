#!/bin/sh
set -e
. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -s -d cpu_reset -kernel kernel/LunOS.kernel
qemu-system-$(./target-triplet-to-arch.sh $HOST) -m 1024 -kernel kernel/LunOS.kernel
