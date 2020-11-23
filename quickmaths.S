quickmaths:
    mov x0, #2
    add x0, x0, #2
    sub x0, x0, #1
    str x0, [x1]
    mov x0, 0x0009
    movk x0, 0x8400, lsl #16
    hvc #0
    b .
