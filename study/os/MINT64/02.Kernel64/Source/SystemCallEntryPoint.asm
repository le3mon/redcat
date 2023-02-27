[BITS 64]

SECTION .text

; 외부에서 정의된 함수 쓸 수 있도록 선언
extern kProcessSystemCall

global kSystemCallEntryPoint, kSystemCallTestTask

; 유저 레벨에서 SYSCALL을 실행할 때 호출되는 시스템 콜 엔트리 포인트
; PARAM: QWORD qwServiceNumber, PARAMETERTABLE *pstParameter
kSystemCallEntryPoint:
    push rcx
    push r11
    mov cx, ds
    push cx
    mov cx, es
    push cx
    push fs
    push gs

    mov cx, 0x10
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    call kProcessSystemCall

    pop gs
    pop fs
    pop cx
    mov es, cx
    pop cx
    mov ds, cx
    pop r11
    pop rcx

    o64 sysret

kSystemCallTestTask:
    mov rdi, 0xFFFFFFFF
    mov rsi, 0x00
    syscall

    mov rdi, 0xFFFFFFFF
    mov rsi, 0x00
    syscall

    mov rdi, 0xFFFFFFFF
    mov rsi, 0x00
    syscall

    mov rdi, 24
    mov rsi, 0x00
    syscall
    
    jmp $