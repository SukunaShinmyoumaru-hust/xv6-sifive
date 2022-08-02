    .section .text
    .extern __first_boot_magic
    .extern __get_boot_hartid
    .globl _entry
_entry:

    /* check first boot here */
    la t0, __first_boot_magic
    ld t1, (t0)
    li t2, 0x5a5a

    bne t1, t2, _secondary_boot
    
    la sp, boot_stack_top # temporary use stack top
    call __get_boot_hartid # return hartid to a0

_secondary_boot:

    mv t0, x0
    add t0, a0, 1
    slli t0, t0, 15
    la sp, boot_stack
    add sp, sp, t0
    call main

loop:
    j loop
    
    .section .bss.stack
    .align 12
    .globl boot_stack
boot_stack:
    .space 4096 * 5 * 8 /* 5 harts */
    .globl boot_stack_top
boot_stack_top:
