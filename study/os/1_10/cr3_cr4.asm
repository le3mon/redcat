mov eax, cr4
or eax, 0x20
mov cr4, eax


mov eax, 0x100000
mov cr3, eax