[ORG 0x00]
[BITS 16]

SECTION .text

START:
    mov ax, 0x1000      ; 보호 모드 엔트리 포인트의 시작 어드레스(0x10000) 세그먼트 레지스터 값으로 변환
    mov ds, ax
    mov es, ax

    ; Application Processor 이면 아래 과정 모두 뛰어넘은 후 보호 모드 컨널로 이동
    mov ax, 0x0000      ; AP 플래그를 확인하기 위해 
    mov es, ax          ; es 세그먼트 레지스터의 시작 어드레스를 0으로 설정

    cmp byte[es:0x7C09], 0x00   ; 플래그가 0이면 AP이므로
    je .APPLICATIONPROCESSORSTARTPOINT

    ; A20 게이트 활성화 코드
    mov ax, 0x2401      ; A20 게이트 활성화 코드
    int 0x15            ; BIOS 인터럽트 서비스 호출 0x15 = 바이오스 시스템 서비스

    jc .A20GATEERROR    ; 활성화 실패 시 EFLAGS 레지스터의 CF 비트가 1로 설정되므로 이를 이용
    jmp .A20GATESUCCESS

.A20GATEERROR:
    in  al, 0x92        ; 시스템 컨트롤 포트(0x92)에서 1 바이트 읽어 al 레지스터에 저장
    or  al, 0x02        ; or 연산을 통해 비트 1에 위치한 A20 게이트 필드 1로 설정
    and al, 0xFE        ; and 연산을 통해 시스템 리셋 필드를 0으로 설정
    out 0x92, al        ; 설정한 값을 시스템 컨트롤 포트에 저장

.A20GATESUCCESS:
.APPLICATIONPROCESSORSTARTPOINT:
    cli                 ; 인터럽트가 발생하지 못하도록 설정
    lgdt [GDTR]         ; GDTR 자료구조를 프로세서에 설정하여 GDT 테이블 로드

    ; 보호 모드로 진입
    ; Disable Paging, Disable Cache, Internal FPU, Disable Align Check, Enable ProtectedMode
    mov eax, 0x4000003B ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
    mov cr0, eax        ; CR0 컨트롤 레지스터에 플래그 설정

    ; 커널 코드 세그먼트를 0x00을 기준으로 하는 것으로 교체하고 EIP의 값을 0x00을 기준으로 재설정
    ; CS 세그먼트 셀렉터 : EIP
    jmp dword 0x18: (PROTECTEMODE - $$ + 0x10000)

; 보호 모드 진입
[BITS 32]
PROTECTEMODE:
    mov ax, 0x20    ; 보호 모드 커널용 데이터 세그먼트 디스크립터를 AX 레지스터에 저장
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax


    ; 스택을 0x00000000~0x0000FFFF 영역에 64KB 크기로 생성
    mov ss, ax
    mov esp, 0xFFFE
    mov ebp, 0xFFFE

    ; AP이면 아래의 과정을 모두 뛰어넘어서 C 언어 커널 엔트리 포인트로 이동
    cmp byte[0x7C09], 0x00
    je .APPLICATIONPROCESSORSTARTPOINT

    ; 화면에 보호 모드로 전환되었다는 메시지 출력
    push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)
    push 2
    push 0
    call PRINTMESSAGE
    add esp, 12

.APPLICATIONPROCESSORSTARTPOINT:
    ; CS 세그먼트 셀렉터를 커널 코드 디스크립터(0x08)로 변경하면서 0x10200으로 이동
    jmp dword 0x18: 0x10200 ; 0x10200에는 c언어 커널이 존재

; 함수 코드 영역
PRINTMESSAGE:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ecx
    push edx

    mov eax, dword [ebp + 12]
    mov esi, 160
    mul esi
    mov edi, eax


    mov eax, dword [ebp + 8]
    mov esi, 2
    mul esi
    add edi, eax

    mov esi, dword [ebp + 16]

.MESSAGELOOP:
    mov cl, byte [esi]

    cmp cl, 0
    je .MESSAGEEND

    mov [edi + 0xB8000], cl
    
    add esi, 1
    add edi, 2

    jmp .MESSAGELOOP

.MESSAGEEND:
    pop edx
    pop ecx
    pop eax
    pop edi
    pop esi
    pop ebp
    ret

; 데이터 영역
align 8, db 0   ; 아래의 데이터들을 8바이트에 맞춰 정렬하기 위해 추가

dw 0x0000       ; GDTR의 끝을 8바이트로 정렬하기 위해 추가

GDTR:
    dw GDTEND - GDT - 1     ; 아래에 위치하는 GDT 테이블의 전체 크기
    dd (GDT - $$ + 0x10000)

GDT:
    NULLDescriptor:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0x00
        db 0x00
        db 0x00
    
    IA_32eCODEDESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A             ; P = 1, DPL = 0, Code Segment, Execute/Read
        db 0xAF             ; G = 1, D = 0, L = 1, Limit[19:16]
        db 0x00

    IA_32eDATADESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92             ; P = 1, DPL = 0, Data Segment, Execute/Read
        db 0xAF
        db 0x00

    CODEDESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xCF
        db 0x00
    
    DATADESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0xCF
        db 0x00
    
GDTEND:

SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode SUCCESS~~~~!!', 0

times 512 - ($ - $$) db 0x00