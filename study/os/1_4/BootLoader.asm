[ORG 0x00]              ; 코드 시작 어드레서 0x00 설정
[BITS 16]               ; 이하의 코드는 16비트 코드로 설정

SECTION .text           ; text 섹션 정의

jmp 0x07C0:START        ; CS 세그먼트 레지스터에 0x07C0을 복사하면서, START 레이블로 이동

START:
    mov ax, 0x07C0      ; 부트 로더 시작 주소를 세그먼트 레지스터 값으로 변환
    mov ds, ax          ; DS 세그먼트 레지스터 설정
    mov ax, 0xB800      ; 비디오 메모리의 시작 어드레스를 세그먼트 레지스터 값으로 변환
    mov es, ax          ; ES 세그먼트 레지스터 설정

    mov si, 0           ; 화면 초기화 루프 진행 전 루프에서 사용할 레지스터 값 초기화

.SCREENCLEARLOOP:
    mov byte [es:si], 0         ; 문자 필드 0으로 초기화
    mov byte [es:si+1], 0x0A    ; 속성 필드 0x0xA(검은 바탕 밝은 녹색) 설정

    add si, 2

    cmp si, 80*25*2             ; 총 채워야할 크기와 현재 크기 비교
    jl  .SCREENCLEARLOOP        ; si 레지스터 값이 더 작으면 .SCREENCLEARLOOP로 점프

    mov si, 0           ; 다음 루프에서 사용할 레지스터 초기화
    mov di, 0           ; 위와 동일

.MESSAGELOOP:
    mov cl, byte [si+MESSAGE1]  ; 입력할 메시지 값을 cl 레지스터에 저장

    cmp cl, 0                   ; cl 값과 0 비교, 
    je  .MESSAGEEND             ; 0일 경우 모든 메시지를 복사한 것 임으로 .MESSAGEEND로 이동

    mov byte [es:di], cl        ; 입력할 메시지 값을 비디오 메모리 영역에 저장

    add si, 1                   ; si 레지스터 값 1 증가 => 다음 메시지를 복사해야하기 때문
    add di, 2                   ; di 레지스터 값 2 증가 => 비디오 메모리 영역은 글자(1바이트)+속성(1바이트) 이기 때문에 2를 더해서 다음 글자넣는 위치로 이동

    jmp .MESSAGELOOP            ; 계속 반복

.MESSAGEEND
    jmp $               ; 현재 위치 무한 루프

MESSAGE1:   db 'MINT64 OS Boot Loader Start~!!', 0 ;출력할 메시지 정의

times 510 - ( $ - $$ )  db  0x00        ; $ : 현재 라인의 주소
                                        ; $$ : 현재 섹션의 시작 주소
                                        ; $ - $$ : 현재 섹션을 기준으로 하는 오프셋
                                        ; 510 - ($ - $$) : 현재부터 어드레스 510까지
                                        ; db 0x00 : 1 바이트 선언하고 값은 0x00
                                        ; time : 반복 수행
                                        ; 현재 위치에서 어드레스 510까지 0x00으로 채움
db 0x55             ; 1바이트 선언 값은 0x55
db 0xAA             ; 1바이트 선언 값은 0xAA