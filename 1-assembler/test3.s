.globl plus, main

plus:
    add w0, w0, w1
    mov w3, w0
    ret

main:
    mov w0, 3
    mov w1, 4
    b plus
    ret
