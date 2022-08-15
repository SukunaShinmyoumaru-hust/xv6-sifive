#include "include/vm.h"
#include "include/proc.h"
#include "include/types.h"
#include "include/mmap.h"
#include "include/copy.h"
#include "include/syscall.h"
#include "include/vma.h"
#include "include/printf.h"
#include "include/errno.h"

uint64
sys_mmap(void)
{
  uint64 start;
  uint64 len;
  int prot;
  int flags;
  int fd;
  int off;
  if(argaddr(0, &start) < 0)
    return -1;
  if(argaddr(1, &len) < 0)
    return -1;
  if(argint(2, &prot) < 0)
    return -1;
  if(argint(3, &flags) < 0)
    return -1;
  if(argfd(4, &fd, NULL) < 0 && fd!=-1){
    //printf("fd:%d\n",fd);
    return -1;
  }
  if(argint(5, &off) < 0)
    return -1;

  uint64 ret = do_mmap(start, len, prot, flags, fd, off);
  // printf("[sys_map] ret(start) = %p\n",ret);
  return ret;
}

uint64
sys_brk(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  return growproc(n);
}

uint64
sys_munmap(void)
{
  uint64 start;
  uint64 len;
  if(argaddr(0, &start) < 0 || argaddr(1, &len) < 0){
    return -1;
  }
  return do_munmap(NULL, start, len);
}

uint64
sys_mprotect(void)
{
  uint64 addr;
  uint64 len;
  int prot;
  if(argaddr(0, &addr) < 0)
  {
    return -1;
  }
  if(argaddr(1, &len) < 0)
  {
    return -1;
  }
  if(argint(2, &prot) < 0)
  {
    return -1;
  }
  
  int perm = 0;
  if(prot & PROT_READ) 
    perm  |= (PTE_R | PTE_A);
  if(prot & PROT_WRITE)
    perm  |= (PTE_W | PTE_D);
  if(prot & PROT_EXEC)
    perm  |= (PTE_X | PTE_A);
    
  return uvmprotect(addr, len, perm);
}

uint64
sys_msync(void)
{
	uint64 addr;
	uint64 len;
	int flags;

	argaddr(0, &addr);
	argaddr(1, &len);
	argint(2, &flags);

	if (!(flags & (MS_ASYNC|MS_SYNC|MS_INVALIDATE)) ||
		((flags & MS_ASYNC) && (flags & MS_SYNC)) ||
		(addr % PGSIZE))
	{
		return -EINVAL;
	}

	return do_msync(addr, len, flags);
}