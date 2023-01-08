[BITS 64]

SECTION .text

extern Main          ; 외부에서 정의된 함수 쓸 수 있도록 선언
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount ; APIC ID 레지스터의 어드레스와 꺠어난 코어의 개수

START:
    mov ax, 0x10     ; IA-32e 모드 커널용 데이터 세그먼트 디스크립터 AX 레지스터에 저장
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    ; 부트 로더 영역의 Bootstrap Processor 플래그를 확인하여, BSP이면 바로 메인으로 이동
    cmp byte[0x7C09], 0x01
    je .BOOTSTRAPPROCESSORSTARTPOINT

    ; AP만 실행하는 영역
    ; 스택의 꼭대기는 APIC ID를 이용해서 0x700000 이하로 이동
    ; 최대 16개 코어까지 지원 가능하므로 스택 영역인 1M을 16으로 나눈 값인 64KB만큼씩 아래로 이동하면서 설정
    ; 로컬 APIC의 APIC ID 레지스터에서 ID 추출
    mov rax, 0
    mov rbx, qword[g_qwAPICIDAddress]
    mov eax, dword[rbx]
    shr rax, 24

    mov rbx, 0x10000
    mul rbx

    sub rsp, rax
    sub rbp, rax

    lock inc dword[g_iWakeUpApplicationProcessorCount]

.BOOTSTRAPPROCESSORSTARTPOINT:
    call Main

    jmp $

