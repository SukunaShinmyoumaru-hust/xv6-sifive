#ifndef __VM_H 
#define __VM_H 

#include "types.h"
#include "riscv.h"
#include "proc.h"

void            kvminit(void);
void            kvminithart(void);
uint64          kvmpa(uint64);
void            kvmmap(uint64, uint64, uint64, int);
int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     kvmcreate(void);
// void            uvminit(pagetable_t, uchar *, uint);
void            uvminit(pagetable_t, pagetable_t, uchar *, uint);
// uint64          uvmalloc(pagetable_t, uint64, uint64);
uint64          uvmalloc(pagetable_t pagetable, uint64 start, uint64 end, int perm);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
// int             uvmcopy(pagetable_t, pagetable_t, uint64);
int             uvmcopy(pagetable_t, pagetable_t, pagetable_t, uint64);
// void            uvmfree(pagetable_t, uint64);
void            uvmfree(struct proc *p);
// void            uvmunmap(pagetable_t, uint64, uint64, int);
void            vmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          walkaddr(pagetable_t, uint64);
uint64          experm(pagetable_t pagetable, uint64 va,uint64 perm);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
pagetable_t     proc_kpagetable(void);
void            kvmfreeusr(pagetable_t kpt);
void            kvmfree(pagetable_t kpagetable, int stack_free);
uint64          kwalkaddr(pagetable_t pagetable, uint64 va);
uint64          kwalkaddr1(pagetable_t pagetable, uint64 va);
void		 checkkpt(int num);
void            vmprint(pagetable_t pagetable);
pte_t *         walk(pagetable_t pagetable, uint64 va, int alloc);
int             handle_page_fault(int kind, uint64 stval);
int             kernel_handle_page_fault(int kind, uint stval);
int 		uvmcopy2(pagetable_t old, pagetable_t new, pagetable_t knew, uint sz);
void        	freewalk(pagetable_t pagetable);
int		uvmprotect(uint64 va, uint64 len, int perm);
#endif 
