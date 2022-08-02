# Initial process that execs /init.
# This code runs in user space.

#include "include/sysnum.h"

# exec(init, argv)
.global start
start:
	li a0, 2
	li a1, 1
	li a2, 0
	li a7, SYS_dev
	ecall	# dev(O_RDWR, CONSOLE, 0);
	
	li a0, 0
	li a7, SYS_dup
	ecall	# dup(0);  // stdout

	li a0, 0
	li a7, SYS_dup
	ecall	# dup(0);  // stderr
	
	la a0, init
	la a1, argv
	li a7, SYS_execve
	ecall


# for(;;) exit()
end:
	li a7,SYS_exit
	ecall
	jal end
init:
	.string "/sacrifice\0"

.p2align 2
argv:
	.dword 0
	
