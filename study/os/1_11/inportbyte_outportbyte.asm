kInPortByte:
    push    rdx         ; 함수 호출 이전에 사용하던 값 스택에 저장
    mov     rdx, rdi    ; 포트 번호 값을 가진 첫 번째 인자 값(rdi)을 rdx에 저장
    mov     rax, 0      ; rax 레지스터 초기화
    in      al, dx      ; dx 레지스터에 저장된 포트 어드레스에서 한 바이트를 읽어 al 레지스터에 저장
                        ; al 레지스터는 함수의 반환 값으로 사용됨
    pop     rdx         ; 사용이 끝난 레지스터 복원
    ret

kOutPortByte:
    push    rdx
    push    rax

    mov     rdx, rdi    ; 포트 번호 값을 가진 첫 번째 인자 값(rdi)를 읽어 rdx에 저장
    mov     rax, rsi    ; 전송할 데이터를 가진 두 번째 인자 값(rsi)를 읽어 rax에 저장
    out     dx, al      ; dx 레지스터에 저장된 포트 어드레스에 al 레지스터에 저장된 한 바이트 값 적음

    pop     rax
    pop     rdx
    ret