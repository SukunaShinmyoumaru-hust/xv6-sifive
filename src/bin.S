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
    .incbin "./usrinit/mytest.sh"
sacrifice_end:
    .quad 0xffffffff
sacrifice_size:
    .long sacrifice_end-sacrifice_start
    
    .global localtime
    .global localtime_size
localtime:
    .incbin "./bin/localtime"
localtime_end:

localtime_size:
    .long localtime_end - localtime
