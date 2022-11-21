kEnableInterrupt:
    sti
    ret

kDisableInterrupt:
    cli
    ret

kReadEFLAGS:
    pushfq
    pop rax
    ret