#include "include/timer.h"
#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/syscall.h"
#include "include/copy.h"
#include "include/file.h"
#include "include/errno.h"

uint64
sys_clock_gettime(void){
	clockid_t tid;
	uint64 addr;

	if(argaddr(0, &tid) < 0 || argaddr(1, &addr) < 0)
	{
		return -1;
	}

	uint64 tmp_ticks = r_time();
	struct timespec tsp;
	

	switch (tid)
	{
	case CLOCK_REALTIME:
		tsp.tv_sec = tmp_ticks / CLK_FREQ;
		tsp.tv_nsec = tmp_ticks / CLK_FREQ / 1000000000;
		break;
	
	default:
		break;
	}
	if(either_copyout(1,addr,(char*)&tsp,sizeof(struct timespec))<0){
	  return -1;
	}

	return 0;

}

uint64 sys_utimensat(void){
	int fd;
	uint64 pathaddr;
	char pathname[FAT32_MAX_FILENAME];
	uint64 buf;
	int flags;
	struct timespec ts[2];
	struct proc* p = myproc();
	struct file *f;
	struct file *fp = NULL;
	struct dirent *ep, *dp;
	int devno = -1;
	if(argfd(0,&fd,&fp)<0 && fd!=AT_FDCWD && fd!=-1){
	  return -1;
	}
	if(argaddr(1,&pathaddr)==0){
	  if(pathaddr&&argstr(1,pathname,FAT32_MAX_FILENAME+1)<0){
	    return -1;
	  }
	}else{
	  return -1;
	}
	if(argaddr(2,&buf)<0){
	  return -1;
	}
	if(argint(3,&flags)<0){
	  return -1;
	}


	if(buf != NULL){
	  if(copyin(p->pagetable,(char*)ts,buf,2*sizeof(struct timespec))<0){
	    return -1;
	  }
	}
	else
	{
		ts[0].tv_sec = TICK_TO_US(p->proc_tms.utime);
		ts[0].tv_nsec = TICK_TO_US(p->proc_tms.utime);
		ts[1].tv_sec = TICK_TO_US(p->proc_tms.utime);
		ts[1].tv_nsec = TICK_TO_US(p->proc_tms.utime);
	}

	if(pathname[0] == '/')
	{
		dp = NULL;
	}
	else if(fd == AT_FDCWD)
	{
		dp = NULL;
	}
	else
	{
		if(fp == NULL)
		{
			__debug_warn("[sys_utimensat] DIRFD error\n");
			return -EMFILE;
		}
		dp = fp->ep;
	}

	ep = ename(dp, pathname, &devno);
	if(ep == NULL)
	{
		// __debug_warn("[sys_utimensat] file not found\n");
		return -ENOENT;
	}


	if(pathaddr){
		f = findfile(pathname);
		f->t0_sec = ts[0].tv_sec;
		f->t0_nsec = ts[0].tv_nsec;
		f->t1_sec = ts[1].tv_sec;
		f->t1_nsec = ts[1].tv_nsec;
	}
	else if(fd >= 0 && ts[0].tv_sec != 1){
		if(argfd(0,&fd,&f)<0) return -1;
		if(ts[0].tv_sec > f->t0_sec || ts[0].tv_sec == 0) f->t0_sec = ts[0].tv_sec;
		if(ts[0].tv_nsec > f->t0_nsec || ts[0].tv_nsec == 0) f->t0_nsec = ts[0].tv_nsec;
		if(ts[1].tv_sec > f->t1_sec || ts[1].tv_sec == 0) f->t1_sec = ts[1].tv_sec;
		if(ts[1].tv_nsec > f->t1_nsec || ts[1].tv_nsec == 0) f->t1_nsec = ts[1].tv_nsec;
	}

	// printf("[sys utimesat]fd:%d\tpathname:%s\n",fd,pathaddr?pathname:"(nil)");
	// printf("[sys utimesat]buf:%p\n",buf);
	// printf("[sys utimesat]timespec[0] tv_sec:%p\ttv_nsec:%p\n",ts[0].tv_sec,ts[0].tv_nsec);
	// printf("[sys utimesat]timespec[1] tv_sec:%p\ttv_nsec:%p\n",ts[1].tv_sec,ts[1].tv_nsec);
	// printf("[sys utimesat]flags:%p\n",flags);
	return 0;
}
