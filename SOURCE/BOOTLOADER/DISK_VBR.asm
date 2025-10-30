; Virtual boot record for when loading from a disk image
dw 0xffee
dw 0xaacc


; 96 = sizeof(BPB)
times (510-96)-($-$$) db 0
; 0x55aa placed by kernel