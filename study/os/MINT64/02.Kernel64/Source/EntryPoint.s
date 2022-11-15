[BITS 64]

SECTION .text

extern Main          ; 외부에서 정의된 함수 쓸 수 있도록 선언

START:
    mov ax, 0x10     ; IA-32e 모드 커널용 데이터 세그먼트 디스크립터 AX 레지스터에 저장
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    call Main

    jmp $