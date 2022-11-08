jmp dword 0x08: (PROTECTEMODE - $$ + 0x10000) ; 커널 코드 세그먼트 기준으로 0x00 이지만 실제 코드는 0x10000을 기준으로 실행됨

[BITS 32]
PROTECTEMODE:
    mov ax, 0x10    ; 보호 모드 커널용 데이터 세그먼트 디스크립터를 AX 레지스터에 저장
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax


    ; 스택을 0x00000000~0x0000FFFF 영역에 64KB 크기로 생성
    mov ss, ax
    mov esp, 0xFFFE
    mov ebp, 0xFFFE