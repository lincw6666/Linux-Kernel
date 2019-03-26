#!/bin/sh

cd /usr/src/linux-4.9.161
# Compile Linux kernel.
make -j 2 clean
make -j 2
make modules_install
make install
# Update grub.
update-initramfs -c -k 4.9.161
update-grub
# Boot into new kernel.
reboot
