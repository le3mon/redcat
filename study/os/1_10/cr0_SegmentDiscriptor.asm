mov eax, cr0
or eax, 0xE0000000  ; NW 비트(29), CD 비트(30), PG 비트(31) 모두 1로 설정
xor eax, 0x60000000 ; NW 비트, CD 비트 0으로 설정
mov cr0, eax

jmp 0x08:0x200000   ; CS 세그먼트 셀렉터를 IA-32e 모드용 코드 세그먼트
                    ; 디스크립터로 교체하고 0x200000 어드레스로 이동

;0x200000 어드레스에 위치하는 코드
[BITS 64]
mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

mov ss, ax
mov rsp, 0x6FFFF8
mov rbp, 0x6FFFF8