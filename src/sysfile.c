#include "include/types.h"
#include "include/printf.h"
#include "include/fcntl.h"
#include "include/dev.h"
#include "include/syscall.h"
#include "include/mmap.h"
#include "include/copy.h"
#include "include/pipe.h"
#include "include/errno.h"

extern struct dirent *selfexe;
// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdallocfrom(struct file *f,int start)
{
  int fd;
  struct proc *p = myproc();
  //printf("[fdalloc]filelimit:%p\n",NOFILEMAX(p));
  for(fd = start; fd < NOFILEMAX(p); fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -EMFILE;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  return fdallocfrom(f,0);
}


uint64
sys_openat()
{
  char path[FAT32_MAX_PATH];
  int dirfd, flags,mode;
  int fd ,devno = -1;
  struct file *f;
  struct file *dirf;
  struct dirent *dp = NULL;
  struct dirent *ep;
  struct proc* p = myproc();
  int err = 0;
  
  argfd(0,&dirfd,&dirf);
  if(argstr(1, path, FAT32_MAX_PATH) < 0){
    __debug_warn("[sys openat] open not valid path\n");
    return -1;
  }
  if(argint(2, &flags) < 0
   ||argint(3, &mode) <0 )
    return -1;
  // __debug_warn("[sys openat]1flags:%p mode:%p path:%s\n",flags,mode,path);
  // print_vma_info(p);
  if(mode == 0){
    mode = flags&O_DIRECTORY?0777:0666;
  }
  mode = mode & (~p->umask);
  //__debug_warn("[sys openat]2flags:%p mode:%p\n",flags,mode);
  if(mode | O_RDWR){
  	flags |= O_RDWR;
  }else if(mode == 0600){
  	flags = flags;
  	//clear execute
  }
  /*
  if(flags&0x8000){
    flags|=O_CREATE;
  }
  */
  
  if(dirf&&dirf->type==FD_ENTRY){
    dp = dirf->ep;
    elock(dp);
    if(!(dp->attribute & ATTR_DIRECTORY)){
      eunlock(dp);
      dp = NULL;
    }
  }    
  if((ep = ename(dp,path,&devno)) == NULL){  
    if(flags & O_CREATE){
      ep = create(dp,path, T_FILE, flags, &err);
      if(ep == NULL){
        __debug_warn("[sys openat] create file %s failed\n",path);
        return -1;
      }
    }
    if(!ep){
      if(dp>=0)
      __debug_warn("[sys openat] env path %s not found\n",path);
      return -1;
    }
  }else{
     elock(ep);
  }
  int pathlen = strlen(path);
  if(ep==dev&&devno==-1&&strncmp(path+pathlen-3,"dev",4)){
    eunlock(ep);
    __debug_warn("[sys openat] device %s don't exist\n",path);
    return -1;
  }
  if(devno==-1&&(ep->attribute & ATTR_DIRECTORY) && ( !(flags&O_WRONLY) && !(flags&O_RDWR) )){
    __debug_warn("[sys openat] diretory only can be read\n");
    eunlock(ep);
    eput(ep);
    return -1;
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
  p->exec_close[fd] = 0;
  // __debug_warn("[sys openat] fd:%d openat:%s\n",fd,path);
  return fd;
}

uint64
sys_mkdirat(void)
{
  char path[FAT32_MAX_PATH];
  struct dirent *ep, *dp = NULL;
  struct file *fp;
  int dirfd;
  int mode;
  int err = 0;
  struct proc* p = myproc();
  if((argfd(0, &dirfd, &fp) < 0)){
    if(path[0] != '/' && dirfd != AT_FDCWD)
    {
      __debug_warn("[sys_mkdirat] wrong dirfd\n");
      return -EBADF;
    }
    dp = p->cwd;
  }
  else
  {
    dp = fp->ep;
  }
  
  if(argstr(1, path, FAT32_MAX_PATH) < 0)
  {
    return -ENAMETOOLONG;
  }
  
  if(argint(2, &mode) < 0)
  {
    return -ENAMETOOLONG;
  }
  if(p->umask == 0){

  }
  else if((mode&p->umask)==p->umask){
    mode = mode - p->umask;
  }
  else{
    return -1;
  }
  //__debug_info("[sys_mkdirat] create %s, dirfd = %d, mode = %p\n", path, dirfd, mode);
  
  if((ep = create(dp, path, T_DIR, (mode & ~S_IFMT) | S_IFDIR, &err)) == NULL)
  {
    __debug_warn("[sys_mkdirat] create %s failed\n", path);
    return -EINVAL;
  }
  
  //__debug_info("[sys_mkdirat] create %s ing......\n", path);
  eunlock(ep);
  eput(ep);
  return err;
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
  int n,fd;
  uint64 p;
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;
  
  //printf("[sys read]fd:%d n:%d addr:%p\n",fd,n,p);
  
  return fileread(f, p, n);
}



uint64
sys_write(void)
{
  int fd;
  struct file *f;
  int n;
  uint64 p;
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0){
    return -1;
  }

  // __debug_info("[sys_write] pid=%d, fd=%d, n=%p, p=%p\n",myproc()->pid, fd, n, p);
  return filewrite(f, p, n);
}


uint64 
sys_readv(void){
  int fd;
  struct file* f;
  uint64 iov;
  int iovcnt;
  int totlen = 0;
  if(argfd(0, &fd, &f) < 0)
  {
    return -1;
  }
  if(argaddr(1, &iov) < 0)
  {
    return -1;
  }
  if(argint(2, &iovcnt) < 0)
  {
    return -1;
  }
  struct proc* p = myproc();
  struct iovec v;
  //printf("[readv]fd:%d iov:%p iovcnt:%d\n",fd,iov,iovcnt);
  for(int i = 0;i<iovcnt;i++){
    uint64 vec = iov+i*sizeof(v);
    copyin(p->pagetable,(char*)&v,vec,sizeof(v));
    //printf("%d iov base:%p len:%p\n",i,v.iov_base,v.iov_len);
    if(!v.iov_len)continue;
    totlen += fileread(f,(uint64)v.iov_base,v.iov_len);
  }
  return totlen;
}

uint64 
sys_writev(void){
  int fd;
  struct file* f;
  uint64 iov;
  int iovcnt;
  int totlen = 0;
  if(argfd(0, &fd, &f) < 0)
  {
    return -1;
  }
  if(argaddr(1, &iov) < 0)
  {
    return -1;
  }
  if(argint(2, &iovcnt) < 0)
  {
    return -1;
  }
  struct proc* p = myproc();
  struct iovec v;
  //printf("[writev]fd:%d name:%s iov:%p iovcnt:%d\n",fd,f->ep->filename,iov,iovcnt);
  for(int i = 0;i<iovcnt;i++){
    uint64 vec = iov+i*sizeof(v);
    copyin(p->pagetable,(char*)&v,vec,sizeof(v));
    //printf("%d iov base:%p len:%p\n",i,v.iov_base,v.iov_len);
    if(!v.iov_len)continue;
    totlen += filewrite(f,(uint64)v.iov_base,v.iov_len);
    //printf("[writev]next\n");
  }
  return totlen;
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

uint64
sys_unlinkat(void)
{
  struct file* fp;
  int dirfd,flags,len;
  struct dirent *dp;
  struct dirent* ep;
  char path[FAT32_MAX_PATH];
  
  if(argfd(0,&dirfd,&fp)<0){
  
  }
  if((len = argstr(1,path,FAT32_MAX_PATH))<0||
     argint(2,&flags)<0){
    return -1;   
  }
  
  if(dirfd != AT_FDCWD){
    if(fp == NULL)return -1;
    dp = fp->ep;
  }else
    dp = NULL;
 
  if((ep = ename(dp,path,0)) == NULL){
    return -1;
  }
  elock(ep);
  if((ep->attribute & ATTR_DIRECTORY) && !isdirempty(ep)){
      eunlock(ep);
      eput(ep);
      return -1;
  }
  elock(ep->parent);      // Will this lead to deadlock?
  eremove(ep);
  eunlock(ep->parent);
  eunlock(ep);
  eput(ep);
  return 0;

}

// get absolute cwd string
uint64
sys_getcwd(void)
{
  uint64 addr;
  if (argaddr(0, &addr) < 0)
    return -1;

  struct dirent *de = myproc()->cwd;
  char path[FAT32_MAX_PATH];
  char *s;
  int len;

  if (de->parent == NULL) {
    s = "/";
  } else {
    s = path + FAT32_MAX_PATH - 1;
    *s = '\0';
    while (de->parent) {
      len = strlen(de->filename);
      s -= len;
      if (s <= path)          // can't reach root "/"
        return -1;
      strncpy(s, de->filename, len);
      *--s = '/';
      de = de->parent;
    }
  }
  if(addr == 0){
    addr = growprocsize(FAT32_MAX_PATH);
    if(addr == 0)
    {
      __debug_warn("[sys_getcwd] alloc address failed\n");
      return -1;
    }
  }

  if (either_copyout(1, addr, s, strlen(s) + 1) < 0)
    return -1;
  return addr;
}


uint64
sys_fstat(void)
{
  struct file *f;
  int fd;
  uint64 st; // user pointer to struct stat

  if(argfd(0, &fd, &f) < 0 || argaddr(1, &st) < 0)
    return -1;
  //return filestat(f, st);
  //print_f_info(f);
  return filekstat(f, st);
}

uint64
sys_fstatat(void)
{
  int fd;
  uint64 st; // user pointer to struct stat
  int flags;
  char pathname[FAT32_MAX_FILENAME];
  struct file* fp;

  if(argfd(0, &fd, &fp) < 0&&fd!=AT_FDCWD)
    return -EMFILE;  
  if(argstr(1, pathname, FAT32_MAX_FILENAME + 1) < 0)
    return -ENAMETOOLONG;
  if(argaddr(2, &st) < 0)
    return -1;  
  if(argint(3, &flags) < 0)
    return -1;
  //return filestat(f, st);
  //printf("[sys fstatat]fd:%d pathname:%s flags:%p\n",fd,pathname,flags);
  struct dirent* ep;
  struct dirent* dp;
  struct proc* p = myproc();
  int devno = -1;
  if(fd==AT_FDCWD){
    dp = NULL;
  } else { 
    if(pathname[0] != '/' && fp == NULL)
    {
      return -EMFILE;
    }
    dp = fp ? fp->ep : NULL;
    
    if(dp && !(dp->attribute & ATTR_DIRECTORY)){
        printf("fd point to a non directory\n");
        return -1;
    }
  }

  // elock dp may cause deadlock
  ep = ename(dp, pathname, &devno);
  if(ep == NULL){
    //printf("[fstatat]path %s not found\n",pathname);
    return -ENOENT;  
  }

  struct kstat kst;
  if(devno == -1)
  {
    elock(ep);
    ekstat(ep,&kst);
    eunlock(ep);
    eput(ep);
  }
  else
  {
    if(devno < 0 || devno >= getdevnum())
    {
      return -1;
    }
    struct devsw *mydev = &devsw[devno];
    acquire(&mydev->lk);
    devkstat(mydev, &kst);
    release(&mydev->lk);
  }
  
  //printf("kst.mode:%p ATTR_DIRECTORY:%p\n",kst.st_mode,ATTR_DIRECTORY);
  if(copyout(p->pagetable, st, (char *)&kst, sizeof(kst)) < 0)
    return -1;
  return 0;
}

uint64
sys_faccessat(void)
{
  int dirfd;
  struct file *fp;
  char path[FAT32_MAX_FILENAME];
  int mode;
  int flags;
  struct dirent *dp, *ep;
  struct proc *p = myproc();
  int devno = -1;
  int emode = R_OK | W_OK | X_OK;

  if(argfd(0, &dirfd, &fp) < 0 && dirfd != AT_FDCWD)
  {
    return -EMFILE;
  }
  if(argstr(1, path, FAT32_MAX_FILENAME + 1) < 0)
  {
    return -1;
  }
  if(argint(2, &mode) < 0)
  {
    return -1;
  }
  if(argint(3, &flags) < 0)
  {
    return -1;
  }

  if(path[0] == '/')
  {
    dp = NULL;
  }
  else if (AT_FDCWD == dirfd)
  {
    dp = p->cwd;
  }
  else
  {
    if(NULL == fp)
    {
      __debug_warn("[sys_faccessat] dirfd illegal\n");
      return -EMFILE;
    }
    dp = fp->ep;
  }

  ep = ename(dp, path, &devno);
  if(ep == NULL){
    printf("[faccessat] path %s not found\n",path);
    return -1;  
  }

  // check file whether exist or not
  if(mode == F_OK)
  {
    return 0;
  }

  if(devno == -1)
  {
    
  }
  else
  {

  }

  if((emode & mode) != mode)
  {
    return -1;
  }

  return 0;
}


uint64 
sys_lseek(void)
{
  struct file *f;
  uint64 offset;
  int fd;
  int whence;
  uint64 ret = -1;

  if(argfd(0, &fd, &f) < 0 || argaddr(1, &offset) < 0 || argint(2, &whence) < 0)
  {
    return -1;
  }

  if(f->type != FD_ENTRY)
  {
    return -ESPIPE;
  }

  ret = filelseek(f, offset, whence);
  return ret;
}


uint64
sys_renameat2(void)
{
  char old[FAT32_MAX_PATH], new[FAT32_MAX_PATH];
  int olddirfd, newdirfd;
  struct file *oldfp;
  struct file *newfp;
  struct dirent *olddp = NULL, *newdp = NULL;
  struct proc *p = myproc();
  // int flags;
  int olddevno = -1, newdevno = -1;
  struct dirent *src = NULL, *dst = NULL, *pdst = NULL;
  int srclock = 0;
  char *name;

  if (argstr(1, old, FAT32_MAX_PATH) < 0 || argstr(3, new, FAT32_MAX_PATH) < 0) {
      return -ENAMETOOLONG;
  }

  if(argfd(0, &olddirfd, &oldfp) < 0)
  {
    if(old[0] != '/' && olddirfd != AT_FDCWD)
    {
      return -EBADF;
    }
    olddp = p->cwd;
  }

  if(argfd(2, &newdirfd, &newfp) < 0)
  {
    if(new[0] != '/' && newdirfd != AT_FDCWD)
    {
      return -EBADF;
    }
    newdp = p->cwd;
  }

  // if(argint(4, &flags) < 0)
  // {
  //   return -ENAMETOOLONG;
  // }
 
  if ((src = ename(olddp, old, &olddevno)) == NULL || (pdst = enameparent(newdp, new, old, &newdevno)) == NULL
      || (name = formatname(old)) == NULL) {
    goto fail;          // src doesn't exist || dst parent doesn't exist || illegal new name
  }
  for (struct dirent *ep = pdst; ep != NULL; ep = ep->parent) {
    if (ep == src) {    // In what universe can we move a directory into its child?
      goto fail;
    }
  }

  uint off;
  elock(src);     // must hold child's lock before acquiring parent's, because we do so in other similar cases
  srclock = 1;
  elock(pdst);
  dst = dirlookup(pdst, name, &off);
  if (dst != NULL) {
    eunlock(pdst);
    if (src == dst) {
      goto fail;
    } else if (src->attribute & dst->attribute & ATTR_DIRECTORY) {
      elock(dst);
      if (!isdirempty(dst)) {    // it's ok to overwrite an empty dir
        eunlock(dst);
        goto fail;
      }
      elock(pdst);
    } else {                    // src is not a dir || dst exists and is not an dir
      goto fail;
    }
  }

  if (dst) {
    eremove(dst);
    eunlock(dst);
  }
  memmove(src->filename, name, FAT32_MAX_FILENAME);
  emake(pdst, src, off);
  if (src->parent != pdst) {
    eunlock(pdst);
    elock(src->parent);
  }
  eremove(src);
  eunlock(src->parent);
  struct dirent *psrc = src->parent;  // src must not be root, or it won't pass the for-loop test
  src->parent = edup(pdst);
  src->off = off;
  src->valid = 1;
  eunlock(src);

  eput(psrc);
  if (dst) {
    eput(dst);
  }
  eput(pdst);
  eput(src);

  return 0;

fail:
  if (srclock)
    eunlock(src);
  if (dst)
    eput(dst);
  if (pdst)
    eput(pdst);
  if (src)
    eput(src);
  return -1;

}


uint64
sys_ioctl(void)
{
	#define TIOCGWINSZ	0x5413
	#define TCGETS		0x5401

	struct winsize {
		uint16 ws_row;		/* rows， in character */
		uint16 ws_col; 		/* columns, in characters */
		uint16 ws_xpixel;	/* horizontal size, pixels (unused) */
		uint16 ws_ypixel;	/* vertical size, pixels (unused) */
	};

	#define ICRNL		0000400
	#define OPOST		0000001
	#define ONLCR		0000004
	#define ICANON		0000002
	#define ECHO		0000010


	struct termios {
		uint16 c_iflag; /* 输入模式标志*/
		uint16 c_oflag; /* 输出模式标志*/
		uint16 c_cflag; /* 控制模式标志*/
		uint16 c_lflag; /*区域模式标志或本地模式标志或局部模式*/
		uint8 c_line; /*行控制line discipline */
		uint8 c_cc[8]; /* 控制字符特性*/
	};

	int fd;
	struct file *f;
	uint64 request;
	uint64 argp;

	if (argfd(0, &fd, &f) < 0 || argaddr(1, &request) < 0 || argaddr(2, &argp) < 0)
		return -EBADF;

	if (f->type != FD_DEVICE&&f->type != FD_PIPE){
               //__debug_info("[sys_ioctl] fd:%d f->type not device or pipe\n",fd);
		return -EPERM;
        }

  // __debug_info("[sys_ioctl] request = %p\n", request);
	switch (request) {
	case TIOCGWINSZ: {
		struct winsize win = {
			.ws_row = 24,
			.ws_col = 80,
		};
		if (either_copyout(1, argp, (char*)&win, sizeof(win)) < 0){
      __debug_info("[sys_ioctl] copyout1\n");
			return -EFAULT;
    }
		break;
	}
	case TCGETS: {
		struct termios terminfo = {
			.c_iflag = ICRNL,
			.c_oflag = OPOST|ONLCR,
			.c_cflag = 0,
			.c_lflag = ICANON|ECHO,
			.c_line = 0,
			.c_cc = {0},
		};
		if (either_copyout(1, argp, (char*)&terminfo, sizeof(terminfo)) < 0){
      __debug_info("[sys_ioctl] copyout2\n");
			return -EFAULT;
    }
		break;
	}
	default:
		return 0;
	}

	return 0;
}


uint64
sys_fcntl(void)
{
  int fd;
  int cmd;
  uint64 arg;
  struct file* f;
  struct proc* p = myproc();
  if(argfd(0, &fd, &f) < 0)
    return -1;
  if(argint(1, &cmd) < 0)
    return -1;
  if(argaddr(2, &arg) < 0)
    return -1;
   //printf("[sys fcntl]fd:%d cmd:%d arg:%p\n",fd,cmd,arg);
  if(cmd == F_GETFD){
    return p->exec_close[fd];
  }else if(cmd == F_SETFD){
    p->exec_close[fd] = arg;
  }else if(cmd == F_DUPFD){
    if((fd=fdallocfrom(f,arg)) < 0){
      return fd;
    }
    filedup(f);
    //__debug_warn("[sys fcntl]return fd:%d\n",fd);
    return fd;
  }else if(cmd == F_DUPFD_CLOEXEC){
    if((fd=fdallocfrom(f,arg)) < 0){
      return fd;
    }
    filedup(f);
    p->exec_close[fd] = 1;
    //__debug_warn("[sys fcntl]return fd:%d\n",fd);
    return fd;
  }
  return 0;
}


uint64
sys_getdents64(void) 
{
  struct file* fp;
  int fd;
  uint64 buf;
  uint64 len;

  if(argfd(0, &fd, &fp) < 0 || argaddr(1, &buf) < 0 || argaddr(2, &len) < 0) {
    return -1;
  }

  return dirent_next(fp, buf, len);
}

uint64
sys_pipe2(void)
{
  uint64 fdarray; // user pointer to array of two integers
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();

  if(argaddr(0, &fdarray) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0){
    __debug_warn("[pipe2] pipe alloc failed\n");
    return -1;
  }
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    __debug_warn("[pipe2] fd alloc failed\n");
    return -1;
  }
  // if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
  //    copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
  if(either_copyout(1, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
     either_copyout(1, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    __debug_warn("[pipe2] copy failed\n");
    return -1;
  }
  //printf("[pipe] fd0:%d fd1:%d\n",fd0,fd1);
  return 0;
}

uint64
sys_readlinkat(void)
{
  int dirfd;
  struct file* df;
  struct dirent *dp = NULL;
  char pathname[FAT32_MAX_PATH+1];
  uint64 buf;
  int bufsz;
  struct proc *p = myproc();
  int devno = -1;

  if(argint(3,&bufsz)<0){
    return -1;
  }
  if(argaddr(2,&buf)<0){
    return -1;
  }
  if(argstr(1,pathname,FAT32_MAX_PATH+1)<0){
    return -1;
  }
  if(argfd(0,&dirfd,&df)<0){
    if(dirfd!=AT_FDCWD&&pathname[0]!='/'){
      return -1;
    }
    dp = p->cwd;
  }else{
    dp = df->ep;
  }

  //if(dirfd>=0)print_f_info(df);
  //printf("[readlinkat] pathname:%s\n",pathname);
  //printf("[readlinkat] buf:%p bufsz:%p\n",buf,bufsz);
  struct dirent* ep = ename(dp, pathname, &devno);

  if(ep == selfexe){
    if(either_copyout(1,buf,myproc()->name,bufsz)<0){
      __debug_info("[sys_readlinkat] copyout error\n");
      return -1;
    }
    return 0;
  }
  //__debug_info("[sys_readlinkat] pathname not matched\n");
  return -1;
}


uint64
sys_fsync(void)
{
  return 0;
}

uint64 
sys_sendfile(void)
{
  int out_fd;
  int in_fd;
  struct file *fout;
  struct file *fin;
  uint64 offset;
  uint64 count;
  if(argfd(0, &out_fd, &fout) < 0)
  {
    return -1;
  }
  if(argfd(1, &in_fd, &fin) < 0)
  {
    return -1;
  }
  if(argaddr(2, &offset) < 0)
  {
    return -1;
  }
  if(argaddr(3, &count) < 0)
  {
    return -1;
  }
  //__debug_info("out_fd: %d, in_fd: %d, offset: %p, count: %p\n", out_fd, in_fd, offset, count);
  
  return filesend(fin,fout,offset,count);
}

