#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"
#include"include/syscall.h"
#include"include/exec.h"
#include"include/pm.h"
#include"include/uname.h"
#include"include/copy.h"

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
sys_getgid(void)
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
sys_brk(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  return growproc(n);
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
