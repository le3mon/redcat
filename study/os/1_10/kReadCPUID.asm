global kReadCPUID

SECTION .text

kReadCPUID:
    push ebp
    mov ebp, esp ; 베이스 포인터 레지스터(EBP)에 스택 포인터 레지스터(ESP)의 값을 설정
    push eax ; 함수에서 임시로 사용하는 레지스터로 함수의 마지막 부분에서
    push ebx ; 스택에 삽입된 값을 꺼내 원래 값으로 복원
    push ecx
    push edx
    push esi

    ; eax 레지스터 값으로 CPUID 실행
    mov eax, dword [ebp + 8]
    cpuid

    ; 반환된 값을 파라미터에 저장
    mov esi, dword [ebp + 12]
    mov dword [esi], eax

    mov esi, dword [ebp + 16]
    mov dword [esi], ebx

    mov esi, dword [ebp + 20]
    mov dword [esi], ecx

    mov esi, dword [ebp + 24]
    mov dword [esi], edx

    pop esi ; 함수에서 사용이 끝난 ESI 레지스터부터 EBP 레지스터까지를 스택에
    pop edx ; 삽입된 값을 이용해서 복원
    pop ecx ; 스택은 가장 마지막에 들어간 데이터가 가장 먼저 나오는
    pop ebx ; 자료구조이므로 삽입의 역순으로
    pop eax ; 제거해야 함
    pop ebp ; 베이스 포인터 레지스터(EBP) 복원
    ret