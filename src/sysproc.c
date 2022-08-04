#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"
#include"include/syscall.h"
#include"include/exec.h"

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
 //printf("[sys exec]ret:%d\n",ret);
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
sys_exit()
{
  int n;
  if(argint(0,&n)<0){
    return -1;
  }
  exit(n);
  return 0;
}
