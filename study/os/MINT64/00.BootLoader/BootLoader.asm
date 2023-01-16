[ORG 0x00]              ; 코드 시작 어드레서 0x00 설정
[BITS 16]               ; 이하의 코드는 16비트 코드로 설정

SECTION .text           ; text 섹션 정의

jmp 0x07C0:START        ; CS 세그먼트 레지스터에 0x07C0을 복사하면서, START 레이블로 이동

TOTALSECTORCOUNT:    dw  0x02 ; 부트 로더를 제외한 MINT64 OS 이미지 크기, 최대 1152 섹터까지 가능

KERNEL32SECTORCOUNT: dw  0x02 ; 보호 모드 커널의 총 섹터 수
BOOTSTRAPPROCESSOR: db  0x01    ; Bootstrap Processor 인지 여부

START:
    mov ax, 0x07C0      ; 부트 로더 시작 주소를 세그먼트 레지스터 값으로 변환
    mov ds, ax          ; DS 세그먼트 레지스터 설정
    mov ax, 0xB800      ; 비디오 메모리의 시작 어드레스를 세그먼트 레지스터 값으로 변환
    mov es, ax          ; ES 세그먼트 레지스터 설정

    mov ax, 0x0000      ; 스택 세그먼트의 시작 어드레스를 세그먼트 레지스터 값으로 변환
    mov ss, ax
    mov sp, 0xFFFE
    mov bp, 0xFFFE

    mov si, 0           ; 화면 초기화 루프 진행 전 루프에서 사용할 레지스터 값 초기화

.SCREENCLEARLOOP:
    mov byte [es:si], 0         ; 문자 필드 0으로 초기화
    mov byte [es:si+1], 0x0A    ; 속성 필드 0x0xA(검은 바탕 밝은 녹색) 설정

    add si, 2

    cmp si, 80*25*2             ; 총 채워야할 크기와 현재 크기 비교
    jl  .SCREENCLEARLOOP        ; si 레지스터 값이 더 작으면 .SCREENCLEARLOOP로 점프

    ; 화면 상단에 시작 메시지 출력
    push MESSAGE1       ; 출력 메시지 스택에 삽입
    push 0              ; y 좌표 스택에 삽입
    push 0              ; x 좌표 스택에 삽입
    call PRINTMESSAGE
    add  sp, 6          ; 파라미터 제거

    ; OS 이미지 로딩하는 메시지 출력
    push IMAGELOADINGMESSAGE
    push 1
    push 0
    call PRINTMESSAGE
    add sp, 6

    ; 디스크에서 OS 이미지를 로딩
    ; 디스크를 읽기 전에 먼저 리셋


RESETDISK:
    ; BIOS Reset Function 호출
    mov ax, 0           ; 서비스 번호 0
    mov dl, 0           ; 드라이브 번호 0 = 플로피디스크
    int 0x13

    jc HANDLEDISKERROR  ; 에러 발생 시 처리

    ; 디스크에서 섹터 읽음
    ; 디스크 내용을 메모리로 복사할 어드래스(ES:BX) 0x10000으로 설정

    mov si, 0x1000      ; OS 이미지를 복사할 어드레스(0x10000)
    mov es, si
    mov bx, 0x0000

    mov di, word[TOTALSECTORCOUNT] ; 복사할 OS 이미지의 섹터 수를 DI 레지스터에 설정

READDATA:
    cmp di, 0               ; 복사할 OS 이미지 섹터 수를 0과 비교
    je  READEND             ; 0이라면 READEND로 점프
    sub di, 0x1             ; 복사할 섹터 수 1 감소

    ; BIOS Read Function 호출
    mov ah, 0x02                ; BIOS 서비스 번호 2(Read Sector)
    mov al, 0x1                 ; 읽을 섹터 수는 1
    mov ch, byte [TRACKNUMBER]  ; 읽을 트랙 번호 설정
    mov cl, byte [SECTORNUMBER] ; 읽을 섹터 번호 설정
    mov dh, byte [HEADNUMBER]   ; 읽을 헤드 번호 설정
    mov dl, 0x00                ; 읽을 드라이브 번호, 0 = 플로피 디스크 설정
    int 0x13                    ; 인터럽트 서비스 수행
    jc HANDLEDISKERROR          ; 에러 발생 시 HANDLEDISKERROR로 이동

    ; 복사할 어드레스와 트랙, 헤드, 섹터 어드레스 계산
    add si, 0x0020      ; 1섹터 = 512(0x200)만큼 읽었으므로, 이를 세그먼트 레지스터 값으로 변환
    mov es, si          ; ES 세그먼트 레지스터에 더해서 어드레스를 한 섹터만큼 증가

    mov al, byte [SECTORNUMBER]
    add al, 0x01                ; 섹터 번호 1 증가

    mov byte [SECTORNUMBER], al ; 증가시킨 섹터 번호를 SECTORNUMBER에 다시 설정
    cmp al, 37                  ; 섹터 번호를 19와 비교 => 36개의 섹터 존재 
    jl READDATA                 ; 섹터 번호가 36 미만인 경우 READDATA로 이동

    xor byte [HEADNUMBER], 0x01     ; 헤드 번호를 0x01과 xor 연산하여 저장(0 -> 1, 1 -> 0)
    mov byte [SECTORNUMBER], 0x01   ; 헤드 번호가 바뀌는 것은 1~18 섹터를 읽은 후 임으로 다시 1로 저장

    cmp byte [HEADNUMBER], 0x00 ; 헤드 번호가 1이면 아직 헤드 1번의 섹터를 처리하기 전 이므로 다시 READDATA로 이동
    jne READDATA

    add byte [TRACKNUMBER], 0x01    ; 트랙 번호 1 증가
    jmp READDATA

READEND:
    ; OS 이미지 완료된 메시지를 출력
    push LOADINGCOMPLETEMESSAGE
    push 1
    push 20
    call PRINTMESSAGE
    add sp, 6

    ; 로딩한 가상 OS 이미지 실행
    jmp 0x1000:0x0000

; 함수 코드 영역
; 디스크 에러 처리 함수
HANDLEDISKERROR:
    push DISKERRORMESSAGE
    push 1
    push 20
    call PRINTMESSAGE

    jmp $

; 메시지 출력 함수
; 파라미터 : x 좌표, y 좌표, 문자열
PRINTMESSAGE:
    push bp
    mov bp, sp
    
    push es
    push si
    push di
    push ax
    push cx
    push dx

    mov ax, 0xB800              ; 비디오 메모리 시작 어드레스를 ES 세그먼트 레지스터에 설정
    mov es, ax

    ; Y 좌표를 이용해서 먼저 라인 어드레스를 구함
    mov ax, word [bp+6]         ; Y 좌표 값을 AX 레지스터에 설정
    mov si, 160                 ; 한 라인의 바이트 수(2*80)을 SI 레지스터에 설정
    mul si                      ; AX * SI 값을 AX에 설정
    mov di, ax

    ; X 좌표 구함
    mov ax, word [bp+4]         ; X 좌표 값을 AX 레지스터에 설정
    mov si, 2                   ; 한 문자를 나타내는 바이트 수 SI 레지스터에 설정
    mul si                      ; AX * SI 값 AX에 설정
    add di, ax                  ; 최종 좌표 값 di에 저장

    mov si, word [bp+8]         ; 문자열 주소를 si 레지스터에 저장

.MESSAGELOOP:
    mov cl, byte [si]           ; 주소가 가르키는 값(문자)를 cl에 저장

    cmp cl, 0                   ; cl 값과 0 비교, 
    je  .MESSAGEEND             ; 0일 경우 모든 메시지를 복사한 것 임으로 .MESSAGEEND로 이동

    mov byte [es:di], cl        ; 입력할 메시지 값을 비디오 메모리 영역에 저장

    add si, 1                   ; si 레지스터 값 1 증가 => 다음 메시지를 복사해야하기 때문
    add di, 2                   ; di 레지스터 값 2 증가 => 비디오 메모리 영역은 글자(1바이트)+속성(1바이트) 이기 때문에 2를 더해서 다음 글자넣는 위치로 이동

    jmp .MESSAGELOOP            ; 계속 반복

.MESSAGEEND:
    pop dx
    pop cx
    pop ax
    pop di
    pop si
    pop es
    pop bp
    ret

; 데이터 영역
; 부트 로더 시작 메시지

MESSAGE1:   db 'MINT64 OS Boot Loader Start~!!', 0 ;출력할 메시지 정의

DISKERRORMESSAGE:   db 'DISK Error~!!', 0
IMAGELOADINGMESSAGE:    db 'OS Image Loading...', 0
LOADINGCOMPLETEMESSAGE: db 'Complete~!!', 0

; 디스크 읽기 관련 변수
SECTORNUMBER:           db 0x02
HEADNUMBER:             db 0x00
TRACKNUMBER:            db 0x00

times 510 - ( $ - $$ )  db  0x00        ; $ : 현재 라인의 주소
                                        ; $$ : 현재 섹션의 시작 주소
                                        ; $ - $$ : 현재 섹션을 기준으로 하는 오프셋
                                        ; 510 - ($ - $$) : 현재부터 어드레스 510까지
                                        ; db 0x00 : 1 바이트 선언하고 값은 0x00
                                        ; time : 반복 수행
                                        ; 현재 위치에서 어드레스 510까지 0x00으로 채움
db 0x55             ; 1바이트 선언 값은 0x55
db 0xAA             ; 1바이트 선언 값은 0xAA