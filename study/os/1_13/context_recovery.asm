ISRKeyboard:
 ; 콘텍스트 저장
 ; RBP 레지스터부터 GS 세그먼트 셀렉터까지 모두 스택에 삽입
 push rbp
 mov rbp, rsp
 push rax
 push rbx
 push rcx
 push rdx
 push rdi
 push rsi
 push r8
 push r9
 push r10
 push r11
 push r12
 push r13
 push r14
 push r15
 
 mov ax, ds ; DS 세그먼트 셀렉터와 ES 세그먼트 셀렉터는 스택에 직접
 push rax ; 삽입할 수 없으므로, RAX 레지스터에 저장한 후 스택에 삽입
 mov ax, es
 push rax
 push fs
 push gs 
 ; 세그먼트 셀렉터 교체
 mov ax, 0x10 ; AX 레지스터에 커널 데이터 세그먼트 디스크립터 저장
 mov ds, ax ; DS 세그먼트 셀렉터부터 FS 세그먼트 셀렉터까지 모두
 mov es, ax ; 커널 데이터 세그먼트로 교체
 mov gs, ax
 mov fs, ax
 
 ; C로 작성된 핸들러 함수를 호출
 call kKeyboardHandler
 
 ; 콘텍스트 복원
 ; GS 세그먼트 셀렉터부터 RBP 레지스터까지 모두 스택에서 꺼내 복원
 pop gs
 pop fs
 pop rax
 mov es, ax ; ES 세그먼트 셀렉터와 DS 세그먼트 셀렉터는 스택에서 직접
 pop rax ; 꺼내 복원할 수 없으므로, RAX 레지스터에 저장한 뒤에 복원
 mov ds, ax
 
 pop r15
 pop r14
 pop r13
 pop r12
 pop r11
 pop r10
 pop r9
 pop r8
 pop rsi
 pop rdi
 pop rdx
 pop rcx
 pop rbx
 pop rax
 pop rbp 
 
 ; 스택에 에러 코드가 포함되어 있다면 제거
 add rsp, 8
 
 ; 프로세서 저장한 콘텍스트를 복원하여 실행 중인 코드로 복귀
 iretq