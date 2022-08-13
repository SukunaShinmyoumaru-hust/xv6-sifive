#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"
#include"include/syscall.h"
#include"include/exec.h"
#include"include/pm.h"
#include"include/uname.h"
#include"include/copy.h"
#include"include/rusage.h"

uint64
sys_execve()
{
  char path[FAT32_MAX_PATH], *argv[MAXARG] ,*env[MAXARG];
  int argvlen , envlen;
  if(argstr(0, path, FAT32_MAX_PATH) < 0){
    __debug_warn("[sys execve] invalid path\n");
    return -1;
  }
  if((argvlen = argstrvec(1,argv, MAXARG)) < 0){
    __debug_warn("[sys execve] invalid argv\n");
    return -1;
  }
  if((envlen = argstrvec(2,env,MAXARG)) <0){
    env[0] = 0;
  }

 int ret = exec(path, argv, env);

 freevec(argv,argvlen);
 freevec(env,envlen);

 return ret;
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_getppid(void){
  struct proc* p =myproc();
  if(p->parent)return p->parent->pid;
  else return 0;
}

uint64
sys_getuid(void)
{
  return myproc()->uid;
}

uint64
sys_geteuid(void)
{
  return myproc()->uid;
}

uint64
sys_getgid(void)
{
  return myproc()->gid;
}

uint64
sys_getegid(void)
{
  return myproc()->gid;
}

uint64 
sys_setuid(void)
{
  int uid;
  if(argint(0, &uid) < 0)
  {
    return -1;
  }
  myproc()->uid = uid;
  return 0;
}

uint64
sys_setgid(void)
{
  int gid;
  if(argint(0, &gid) < 0)
  {
    return -1;
  }
  myproc()->gid = gid;
  return 0;
}

uint64 
sys_uname(void) {
	uint64 addr;

	if (argaddr(0, &addr) < 0) {
		return -1;
	}

	return uname_copyout(addr);
}


uint64
sys_clone(void)
{
  uint64 flag, stack, ptid, ctid, tls;
  if (argaddr(0, &flag) < 0) 
  	return -1;
  if (argaddr(1, &stack) < 0) 
	return -1;
  if(argaddr(2, &ptid) < 0)
  	return -1;
  if(argaddr(3, &tls) < 0)
    return -1;
  if(argaddr(4, &ctid) < 0)
    return -1;
  return clone(flag, stack, ptid, tls, ctid);
}

uint64
sys_wait4()
{
  uint64 addr;
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  if(argaddr(1, &addr) < 0)
    return -1;
    
  //printf("[sys_wait4]pid %d:%s enter\n",myproc()->pid,myproc()->name);
  return wait4pid(pid,addr);
}

uint64
sys_set_tid_address(void){
  uint64 address;
  argaddr(0, &address);
  myproc()->clear_child_tid = address;
  return myproc()->pid;
}

uint64
sys_gettid(void){
  struct proc* p = myproc();
  uint64 address = p->clear_child_tid;
  int tid;
  if(address){
    if(either_copyin(1,&tid,address,sizeof(tid))<0){
      return -1;
    }
    return tid;
  }else{
    return p->pid;
  }
}

uint64
sys_exit()
{
  int n;
  if(argint(0,&n)<0){
    return -1;
  }
  exit(n);
  return 0;
}

uint64
sys_getrusage(void)
{
  int who;
  struct proc* p = myproc();
  uint64 rusage;
  struct rusage rs;
  if(argint(0,&who)<0){
    return -1;
  }
  if(argaddr(1,&rusage)<0){
    return -1;
  }
  rs = (struct rusage){
    .ru_utime = get_timeval(),
    .ru_stime = get_timeval(),
  };
  switch(who){
    case RUSAGE_SELF:
		case RUSAGE_THREAD:
			rs.ru_nvcsw = p->vswtch;
			rs.ru_nivcsw = p->ivswtch;
      break;
    default:
      break;
  }
  if(either_copyout(1,rusage,&rs,sizeof(rs))<0){
    return -1;
  }
  //__debug_info("[sys_getrusage] return 0\n");
  return 0;
}

uint64
sys_umask(void)
{
  int n;
  argint(0, &n);
  n = n & 0777;
  myproc()->umask = n;
  return 0;
}

uint64 
sys_nanosleep(void) {
	uint64 addr_sec, addr_usec;

	if (argaddr(0, &addr_sec) < 0) 
		return -1;
	if (argaddr(1, &addr_usec) < 0) 
		return -1;

	struct proc *p = myproc();
	uint64 sec, usec;
	if (either_copyin(1, (char*)&sec, addr_sec, sizeof(sec)) < 0) 
		return -1;
	if (either_copyin(1, (char*)&usec, addr_usec, sizeof(usec)) < 0) 
		return -1;
	int n = sec * 20 + usec / 50000000;

	int mask = p->tmask;
	if (mask) {
		printf(") ...\n");
	}
	acquire(&p->lock);
	uint64 tick0 = ticks;
	while (ticks - tick0 < n / 10) {
		if (p->killed) {
			return -1;
		}
		sleep(&ticks, &p->lock);
	}
	release(&p->lock);

	return 0;
}

