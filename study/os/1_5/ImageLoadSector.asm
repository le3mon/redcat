TOTALSECTORCOUNT:   dw  1024    ; 부트 로더를 제외한 MINT OS 이미지 크기
SECTORNUMBER:       db  0x02    ; OS 이미지가 시작하는 섹터 번호를 저장하는 영역
HEADNUMBER:         db  0x00    ; OS 이미지가 시작하는 헤드 번호를 저장하는 영역
TRACKNUMBER:        db  0x00    ; OS 이미지가 시작하는 트랙 번호를 저장하는 영역

    ; 디스크 내용을 복사할 어드레스(ES:BX)를 0x10000으로 설정
    mov si, 0x1000              ; 
    mov es, si                  ; ES 세그먼트 레지스터에 값 설정
    mov bx, 0x0000              ; BX 레지스터에 0ㅌ0000을 설정하여 복사랄 어드레스를 0x1000:0000(0x10000)으로 설정

    mov di, word [TOTALSECTORCOUNT] ; 복사할 OS 이미지의 섹터 수를 di 레지스터에 저장

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
    cmp al, 19                  ; 섹터 번호를 19와 비교
    jl READDATA                 ; 섹터 번호가 19 미만인 경우 READDATA로 이동

    xor byte [HEADNUMBER], 0x01     ; 헤드 번호를 0x01과 xor 연산하여 저장(0 -> 1, 1 -> 0)
    mov byte [SECTORNUMBER], 0x01   ; 헤드 번호가 바뀌는 것은 1~18 섹터를 읽은 후 임으로 다시 1로 저장

    cmp byte [HEADNUMBER], 0x00 ; 헤드 번호가 1이면 아직 헤드 1번의 섹터를 처리하기 전 이므로 다시 READDATA로 이동
    jne READDATA

    add byte [TRACKNUMBER], 0x01    ; 트랙 번호 1 증가
    jmp READDATA
READEND:

HANDLEDISKERROR: