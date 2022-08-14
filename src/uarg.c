#include"include/syscall.h"
#include"include/pm.h"
#include"include/copy.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  //struct proc *p = myproc();
  // if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
  if(either_copyin(1,(char*)ip, addr, sizeof(*ip)))
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)
    return -1;
  //printf("[argstr] str addr:%p\n");
  return fetchstr(addr, buf, max);
}

int
argfd(int n, int *pfd, struct file **pf)
{
  int fd = -1;
  struct file *f = NULL;
  if(pfd)*pfd = -1;
  if(pf)*pf = NULL;
  struct proc* p = myproc();
  if(argint(n, &fd) < 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(fd < 0 || fd >= NOFILEMAX(p) || (f=p->ofile[fd]) == NULL)
    return -1;
  if(pf)
    *pf = f;
  return 0;
}

int
argstruct(int n,void* st,int len){
  uint64 addr;
  if(argaddr(n,&addr)<0)return -1;
  if(addr&&copyin(myproc()->pagetable,st,addr,len)<0){
    return 0;
  }
  return addr;
}

int
freevec(char** argv,int len){
  for(int i = 0; i < len && argv[i] != 0; i++){
    //printf("[freevec] argv[%d]=%p\n",i,argv[i]);
    kfree(argv[i]);
  }
  return 0;
}

int
argstrvec(int n,char** argv,int max){
  int i = 0;
  uint64 uarg,uargv;
  memset(argv, 0, max*sizeof(uint64));
  if(argaddr(n, &uargv) < 0||uargv == 0){
    //__debug_warn("[argstrvec] uargv null\n");
    goto bad;
  }
  for(;;i++){
    if(i >= max){
      __debug_warn("[argstrvec] max is too small\n");
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      __debug_warn("[argstrvec] uargv:%p\n",uargv);
      __debug_warn("[argstrvec] fetch argv[%d] address bad\n",i);
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kmalloc(0x100);
    if(argv[i] == 0){
      __debug_warn("[argstrvec] no more space for argv[%d]\n",i);
      goto bad;
    }
    if(fetchstr(uarg, argv[i], 0x100) < 0){
      __debug_warn("[argstrvec] fetch argv[%d] string bad\n",i);
      goto bad;
    }
  }
  return i;
bad:
  freevec(argv,i+1);
  return -1;
}

