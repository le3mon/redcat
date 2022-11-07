[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x1000:START

SECTORCOUNT:        dw 0x0000       ; 현재 실행 중인 섹터 번호
TOTALSECTORCOUNT    equ 1024        ; 가상 OS의 총 섹터 수

START:
    mov ax, cs
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    ; 각 섹터 별로 코드를 생성
    %assign i 0
    %rep TOTALSECTORCOUNT
        %assign i   i + 1
        
        ; 현재 실행 중인 코드가 포함된 섹터의 위치를 화면 좌표로 변환
        mov ax, 2
        mul word [SECTORCOUNT]
        mov si, ax

        ; 계산된 결과를 비디오 메모리 오프셋으로 삼아 세 번째 라인부터 0 출력
        move byte [es:si+(160*2)], '0' + (i % 10)
        add word[SECTORCOUNT], 1

        ; 마지막 섹터면 더 수행할 섹터가 없으므로 무한 루프 수행, 그렇지 않으면 다음 섹터 이동
        %if i == TOTALSECTORCOUNT
            jmp $
        %else
            jmp (0x1000+i*0x20):0x0000  ; 다음 섹터 오프셋으로 이동
        %endif
        
        times 512 - ( $ - $$ ) db 0x00
    %endrep