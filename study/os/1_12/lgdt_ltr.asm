kLoadGDTR:
    lgdt    [rdi]
    ret

kLoadTR:
    ltr     di
    ret