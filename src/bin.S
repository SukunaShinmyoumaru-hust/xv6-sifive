    .align 4
    .section .data
    .global initcode
    .global initcodesize
initcode:
    .incbin "./usrinit/initcode"
codeend:

initcodesize:
    .long codeend-initcode

    .global sacrifice_start
    .global sacrifice_end
    .global sacrifice_size
sacrifice_start:
    .incbin "./usrinit/_sacrifice"
sacrifice_end:
    .quad 0xffffffff
sacrifice_size:
    .long sacrifice_end-sacrifice_start
