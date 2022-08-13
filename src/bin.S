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

    .global mounts
    .global mounts_size
mounts:
    .incbin "./bin/mounts"
mounts_end:

mounts_size:
    .long mounts_end - mounts

    .global meminfo
    .global meminfo_size
meminfo:
    .incbin "./bin/meminfo"
meminfo_end:

meminfo_size:
    .long meminfo_end - meminfo
