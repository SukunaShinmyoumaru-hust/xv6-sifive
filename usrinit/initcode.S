# Initial process that execs /init.
# This code runs in user space.

#include "include/sysnum.h"
#include "include/fcntl.h"

# exec(init, argv)
.global start
start:
	li a0, AT_FDCWD
	la a1, console
	li a2, O_RDWR
	li a7, SYS_openat
	ecall	# dev(O_RDWR, CONSOLE, 0);
	
	li a0, 0
	li a7, SYS_dup
	ecall	# dup(0);  // stdout

	li a0, 0
	li a7, SYS_dup
	ecall	# dup(0);  // stderr
	
	la a0, init
	la a1, argv
	la a2, env
	li a7, SYS_execve
	ecall


# for(;;) exit()
end:
	li a7,SYS_exit
	ecall
	jal end
init:
	.string "./sacrifice\0"
console:
	.string "/dev/console\0"
testread:
	.space 12

.p2align 2
argv:
	.dword init
	.dword 0
env:
	.dword 0
	
