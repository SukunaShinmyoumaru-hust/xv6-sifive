#include"include/types.h"
#include"include/printf.h"
#include"include/fcntl.h"
#include"include/dev.h"
#include"include/syscall.h"

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();
  //printf("[fdalloc]filelimit:%p\n",NOFILEMAX(p));
  for(fd = 0; fd < NOFILEMAX(p); fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -EMFILE;
}

uint64
sys_openat()
{
  char path[FAT32_MAX_PATH];
  int dirfd, flags,mode;
  int fd ,devno;
  struct file *f;
  struct file *dirf;
  struct dirent *dp = NULL;
  struct dirent *ep;
  argfd(0,&dirfd,&dirf);
  if(argstr(1, path, FAT32_MAX_PATH) < 0){
    __debug_warn("[sys openat] open not valid path\n");
    return -1;
  }
  if(argint(2, &flags) < 0
   ||argint(3, &mode) <0 )
    return -1;

  if(mode == O_RDWR){
  	flags = flags;
  }else if(mode == 0600){
  	flags = flags;
  	//clear execute
  }
  
  if(dirf&&dirf->type==FD_ENTRY){
    dp = dirf->ep;
    elock(dp);
    if(!(dp->attribute & ATTR_DIRECTORY)){
      eunlock(dp);
      dp = NULL;
    }
  }
  
  //printf("[sys openat]open %s\n",path);
  if(flags & O_CREATE){
    ep = create(dp,path, T_FILE, flags);
    if(ep == NULL){
      __debug_warn("[sys openat] create file %s failed\n",path);
      return -1;
    }
  } else {
    if((ep = ename(dp,path,&devno)) == NULL){
      __debug_warn("[sys openat] env path %s not found\n",path);
      return -1;
    }
    elock(ep);
    if(devno==-1&&(ep->attribute & ATTR_DIRECTORY) && (flags != O_RDONLY && flags != O_DIRECTORY)){
      __debug_warn("[sys openat] diretory only can be read\n");
      eunlock(ep);
      eput(ep);
      return -1;
    }
  }

  if((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0){
    if (f) {
      fileclose(f);
    }
    __debug_warn("file alloc failed\n");
    eunlock(ep);
    eput(ep);
    return -EMFILE;
  }

  if(ep!=dev && !(ep->attribute & ATTR_DIRECTORY) && (flags & O_TRUNC)){
    etrunc(ep);
  }
  if(devno ==-1){
    f->type = FD_ENTRY;
    f->off = (flags & O_APPEND) ? ep->file_size : 0;
    f->ep = ep;
    f->readable = !(flags & O_WRONLY);
    f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
  }else{
    f->type = FD_DEVICE;
    f->off = 0;
    f->ep = NULL;  
    f->major = devno;
    f->readable = !(flags & O_WRONLY);
    f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
  }
  eunlock(ep);
  if(dp){
    elock(dp);  
  }
  printf("[sys openat]open %s\n",path);
  return fd;
}

uint64
sys_dup()
{
  
  struct file *f;
  int fd;
  if(argfd(0, 0, &f) < 0){
    return -1;
  }
  if((fd=fdalloc(f)) < 0){
    return fd;
  }
  filedup(f);
  //printf("[dup]ret %d\n",fd);
  return fd;
}


uint64
sys_dup3(void)
{
  struct file *f;
  int newfd;
  struct proc* p = myproc();
  if(argfd(0, 0, &f) < 0) 
    return -1;
  if(argint(1, &newfd) < 0 || newfd < 0)
    return -1;
  if(newfd >= NOFILEMAX(p)){
    return -EMFILE;
  }
  if(p->ofile[newfd] != f) 
  {
    p->ofile[newfd] = filedup(f);
  }
  return newfd;
}

uint64
sys_read(void)
{
  struct file *f;
  int n;
  uint64 p;
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;
  return fileread(f, p, n);
}



uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;
  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0){
    return -1;
  }
  return filewrite(f, p, n);
}

