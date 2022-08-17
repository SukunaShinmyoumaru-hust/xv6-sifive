#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "types.h"
#include "sysnum.h"
#include "file.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "poll.h"
#include "syscall.h"
#include "sysinfo.h"
#include "kalloc.h"
#include "vm.h"
#include "string.h"
#include "printf.h"
#include "socket.h"

int fetchaddr(uint64 addr, uint64 *ip);

int fetchstr(uint64 addr, char *buf, int max);
int fdalloc(struct file *f);
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);
int argstr(int n, char *buf, int max);
void syscall(void);
int argfd(int n, int *pfd, struct file **pf);
int argepoll(int n, int *pfd, struct file **pf, struct epoll** pepoll);
int argsock(int n, int *pfd, struct file **pf, struct socket** psk);
int argstruct(int n,void* st,int len);
int argstrvec(int n,char** argv,int max);
int freevec(char** argv,int len);
#endif
