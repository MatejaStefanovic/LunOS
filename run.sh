#!/bin/sh
set -e
. ./iso.sh

#qemu-system-$(./target-triplet-to-arch.sh $HOST) -s -no-shutdown -no-reboot -d cpu_reset -kernel kernel/LunOS.kernel  
qemu-system-$(./target-triplet-to-arch.sh $HOST) -kernel kernel/LunOS.kernel  

