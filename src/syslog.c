#include "include/timer.h"
#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/syscall.h"
#include "include/copy.h"
#include "include/file.h"
#include "include/errno.h"
#include "include/sysinfo.h"
#include "include/pm.h"

char syslogbuf[1024];
int logbuflen = 0;

void
logbufinit(){
  logbuflen = 0;
  strncpy(syslogbuf,
  	"[log]init done\n",
  	1024);
  logbuflen+=strlen(syslogbuf);
}

uint64
sys_syslog(){
  int type;
  uint64 bufp;
  int len;
  if(argint(0,&type)<0){
    return -1;
  }
  if(argaddr(1,&bufp)<0){
    return -1;
  }
  if(argint(2,&len)<0){
    return -1;
  }
  switch(type){
    case SYSLOG_ACTION_READ_ALL:
    {
      if(either_copyout(1,bufp,syslogbuf,logbuflen)<0){
        return -1;
      }
      return logbuflen;
    }
    case SYSLOG_ACTION_SIZE_BUFFER: return sizeof(syslogbuf);
  }
  //printf("[syslog] type:%d bufp:%p len:%p\n",type,bufp,len);
  return 0;
}

uint64
sys_sysinfo(void)
{
	uint64 addr;
	// struct proc *p = myproc();

	if (argaddr(0, &addr) < 0) {
		return -1;
	}

	struct sysinfo info;
	memset(&info, 0, sizeof(info));

	info.uptime = r_time() / CLK_FREQ;
	info.totalram = PHYSTOP - RUSTSBI_BASE;
	info.freeram = idlepages() << PGSHIFT;
	info.bufferram = BSIZE * BNUM;
	info.procs = NPROC;
	info.mem_unit = PGSIZE;

	// if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0) {
	if (either_copyout(1,addr, (char *)&info, sizeof(info)) < 0) {
		return -1;
	}

	return 0;
}

