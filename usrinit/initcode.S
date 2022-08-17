# Initial process that execs /init.
# This code runs in user space.

#include "include/sysnum.h"
#include "include/fcntl.h"

# exec(init, argv)
.global start
start:
	la a5, option
	slli a0,a0,3
	add a5,a5,a0
	lw a5,0(a5)
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
	
	mv a0, a5
	la a1, argv
	sw a5, 0(a1)
	la a2, env
	li a7, SYS_execve
	ecall


# for(;;) exit()
end:
	li a7,SYS_exit
	ecall
	jal end
server:
	.string "./server.sh\0"
client:
	.string "./client.sh\0"
option:
	.dword server
	.dword client
console:
	.string "/dev/console\0"
testread:
	.space 12

.p2align 2
argv:
	.dword 0
	.dword 0
env:
	.dword 0
	
