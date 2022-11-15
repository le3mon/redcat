[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text

; PARAM: DWORD dwEAX, DWORD* pdwEAX,* pdwEBX,* pdwECX,* pdwEDX
kReadCPUID:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push esi

    ; EAX 레지스터의 값으로 CPUID 실행
    mov eax, dword [ebp+8]
    cpuid

    ; pdwEAX에 반환값 설정
    mov esi, dword [ebp+12]
    mov dword [esi], eax

    ; pdwEBX에 반환값 설정
    mov esi, dword [ebp+16]
    mov dword [esi], ebx

    ; pdwECX에 반환값 설정
    mov esi, dword [ebp+20]
    mov dword [esi], ecx

    ; pdwEDX에 반환값 설정
    mov esi, dword [ebp+24]
    mov dword [esi], edx

    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

kSwitchAndExecute64bitKernel:
    ; CR4 레지스터의 PAE 비트 1로 설정
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; CR3 레지스터에 PML4 테이블 어드레스 설정
    mov eax, 0x100000
    mov cr3, eax


    ; IA32_EFER 레지스터에 LME 필드 1로 설정
    mov ecx, 0xC0000080 ; IA32_EFER 어드레스 설정
    rdmsr               ; IA32_EFER 값 읽어오기

    or eax, 0x0100      ; LME 필드 1로 설정
    wrmsr               ; IA32_EFEP 값 쓰기

    ; CR0 레지스터에 NW = 0, CD = 0, PG = 1로 설정하여
    mov eax, cr0
    or eax, 0xE0000000  ; NW 비트(29), CD 비트(30), PG 비트(31) 모두 1로 설정
    xor eax, 0x60000000 ; NW, CD 비트 xor을 통해 0으로 설정
    
    mov cr0, eax        ; 여기서 전환할 때 멈춤

    jmp 0x08:0x200000

    jmp $               ; 실행 X