qemu-system-x86_64 -L . -m 64 -fda ./DiskWithPackage.img --boot a -M pc -smp 16,sockets=16
# qemu-system-x86_64 -L . -m 64 -fda ./Disk.img -hda ./HDD.img --boot a -M pc -serial tcp::4444,server,nowait -smp 2,sockets=2
# sudo qemu-system-x86_64 -L . -m 64 -fda ./Disk.img -hda ./HDD.img -boot a -M pc -serial tcp::4444,server,nowait -smp 16, sockets=5 -curses
# sudo qemu-system-x86_64 -L . -m 64 -fda Disk.img -hda HDD.img -boot a -M pc
# sudo qemu-system-x86_64 -L . -m 64 -fda Disk.img -boot a -M pc

