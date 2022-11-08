CODEDESCRIPTOR:
    dw 0xFFFF   ; 세그먼트 크기
    dw 0x0000   ; 기준 주소
    db 0x00     ; 기준 주소
    db 0x9A     ; P = 1, DPL = 0, S = 1, 타입 = 0x0A
    db 0xCF     ; G = 1, D/B = 1, L = 0, AVL = 0, 세그먼트 크기 = 0b1111
    db 0x00     ; 기준 주소

DATADESCRIPTOR:
    dw 0xFFFF   ; 세그먼트 크기
    dw 0x0000   ; 기준 주소
    db 0x00     ; 기즌 주소
    db 0x92     ; P = 1, DPL = 0, S = 1, 타입 = 0x02
    db 0xCF     ; G = 1, D/B = 1, L = 0, AVL = 0, 세그먼트 크기 = 0b1111
    db 0x00     ; 기준 주소