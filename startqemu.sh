qemu-system-aarch64 -M virt,virtualization=true -cpu cortex-a57 -smp 4 -m 2g \
-cdrom ~/Documents/nobackup/Fedora-Server-dvd-aarch64-33-1.2.iso \
-kernel ~/Documents/nobackup/fedoraarm64kernel/images/pxeboot/vmlinuz \
-initrd ~/Documents/nobackup/fedoraarm64kernel/images/pxeboot/initrd \
-nic user -nographic \
-append "inst.stage2=hd:LABEL=Fedora-S-dvd-aarch64-33 rescue"
