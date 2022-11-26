kReadTSC:
    pusb rdx

    rdtsc

    shl rdx, 32     ; rdx 값을 왼쪽으로 32비트 이동
    or  rax, rdx    ; 상위 32비트(rdx), 하위 32비트(rax) or 연산하여 64비트 값으로 만듦

    pop rdx
    ret