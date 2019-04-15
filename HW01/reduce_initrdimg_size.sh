cd /lib/modules/4.9.161
find . -name *.ko -exec strip --strip-unneeded {} +
update-initramfs -c -k 4.9.161
