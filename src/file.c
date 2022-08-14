//
// Support functions for system calls that involve file descriptors.
//


#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/spinlock.h"
#include "include/sleeplock.h"
#include "include/fat32.h"
#include "include/file.h"
#include "include/pipe.h"
#include "include/stat.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/kalloc.h"
#include "include/vm.h"
#include "include/copy.h"


extern int disk_init_flag;

void
fileinit(void)
{
  disk_init_flag = 0;
  #ifdef DEBUG
  printf("fileinit\n");
  #endif
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f = kmalloc(sizeof(struct file));
  memset(f,0,sizeof(struct file));
  f->ref = 1;
  initlock(&f->lk,"file lock");
  return f;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&f->lk);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&f->lk);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  acquire(&f->lk);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&f->lk);
    return;
  }
  release(&f->lk);

  if(f->type == FD_PIPE){
    pipeclose(f->pipe, f->writable);
  } else if(f->type == FD_ENTRY){
    eput(f->ep);
  } else if (f->type == FD_DEVICE) {

  }
  kfree(f);
}

int fileillegal(struct file* f){
  switch (f->type) {
    case FD_PIPE:
    case FD_DEVICE:
        if(f->major < 0 || f->major >= getdevnum() || !devsw[f->major].read || !devsw[f->major].write)
          return 1;
    case FD_ENTRY:
        break;
    default:
      panic("fileillegal");
      return 1;
  }
  return 0;
}

void print_f_info(struct file* f){
  switch (f->type) {
    case FD_PIPE:
        printf("[file]PIPE\n");
        break;
    case FD_DEVICE:
        printf("[file]DEVICE name:%s\n",devsw[f->major].name);
        break;
    case FD_ENTRY:
        printf("[file]ENTRY name:%s\n",f->ep->filename);
        break;
    case FD_NONE:
        printf("[file]NONE\n");
    	return;
  }

}

void fileiolock(struct file* f){
  switch (f->type) {
    case FD_PIPE:
        break;
    case FD_DEVICE:
        acquire(&(devsw + f->major)->lk);
        break;
    case FD_ENTRY:
        elock(f->ep);
        break;
    case FD_NONE:
    	return;
  }
}

void fileiounlock(struct file* f){
  switch (f->type) {
    case FD_PIPE:
        break;
    case FD_DEVICE:
        release(&(devsw + f->major)->lk);
        break;
    case FD_ENTRY:
        eunlock(f->ep);
        break;
    case FD_NONE:
    	return;
  }
}

uint64
fileinput(struct file* f, int user, uint64 addr, int n, uint64 off){
  uint64 r = 0;
  switch (f->type) {
    case FD_PIPE:
        r = piperead(f->pipe, user, addr, n);
        break;
    case FD_DEVICE:
        r = (devsw + f->major)->read(user, addr, n);
        break;
    case FD_ENTRY:
        r = eread(f->ep, user, addr, off, n);
        break;
    case FD_NONE:
    	return 0;
  }
  return r;
}

uint64
fileoutput(struct file* f, int user, uint64 addr, int n, uint64 off){
  uint64 r = 0;
  switch (f->type) {
    case FD_PIPE:
        r = pipewrite(f->pipe, user, addr, n);
        break;
    case FD_DEVICE:
        r = (devsw + f->major)->write(user, addr, n);
        break;
    case FD_ENTRY:
        r = ewrite(f->ep, user, addr, off, n);
        break;
    case FD_NONE:
    	return 0;
  }
  return r;
}



// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  // struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_ENTRY){
    elock(f->ep);
    estat(f->ep, &st);
    eunlock(f->ep);
    // if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
    if(either_copyout(1, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}
void fileoff(struct file* f,uint64 off){
  acquire(&f->lk);
  f->off+=off;
  release(&f->lk);
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filekstat(struct file *f, uint64 addr)
{
  //struct proc *p = myproc();
  struct kstat kst;
  
  if(f->type == FD_ENTRY){
    elock(f->ep);
    ekstat(f->ep, &kst);
    eunlock(f->ep);
    kst.st_atime_sec = f->t0_nsec;
    kst.st_atime_nsec = f->t0_sec;
    kst.st_mtime_sec = f->t1_nsec;
    kst.st_mtime_nsec = f->t1_sec;
    if(kst.st_mtime_sec == 0x000000003ffffffe)kst.st_mtime_sec = 0;
    if(kst.st_atime_sec == 0x000000003ffffffe)kst.st_atime_sec = 0;
    if(kst.st_mtime_nsec == 0x0000000100000000)kst.st_mtime_sec = 0x0000000100000000;
    if(kst.st_atime_nsec == 0x0000000100000000)kst.st_atime_sec = 0x0000000100000000;
  }else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= getdevnum() || !devsw[f->major].read)
          return -1;
    struct devsw* mydev = devsw + f->major;
    acquire(&mydev->lk);
    devkstat(mydev,&kst);
    release(&mydev->lk);
  }else {
    return -1;
  }    
  if(either_copyout(1, addr, (char *)&kst, sizeof(kst)) < 0)
    // if(copyout2(addr, (char *)&kst, sizeof(kst)) < 0)
      return -1;
  return 0;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;
  if(f->readable == 0){
    return -1;
  }
  //printf("[file read]\n");
  //print_f_info(f);
  switch (f->type) {
    case FD_PIPE:
        r = piperead(f->pipe, 1, addr, n);
        if(r<0)r = 0;
        break;
    case FD_DEVICE:
        if(f->major < 0 || f->major >= getdevnum() || !devsw[f->major].read)
          return -1;
        struct devsw* mydev = devsw + f->major;
        acquire(&mydev->lk);
        r = mydev->read(1, addr, n);
        release(&mydev->lk);
        break;
    case FD_ENTRY:
        elock(f->ep);
        if((r = eread(f->ep, 1, addr, f->off, n)) > 0)
          fileoff(f, r);
        eunlock(f->ep);
        break;
    default:
      panic("fileread");
  }
  // printf("[file read] r:%p\n",r);
  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int ret = 0;
  //printf("major:%d off:%p\n",f->major,consolewrite-(char*)(devsw[f->major].write));
  if(!n)return 0;
  //print_f_info(f);
  //printf("[filewrite] addr:%p n:%p \n",addr,n);
  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, 1, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= getdevnum() || !devsw[f->major].write)
      return -1;
    struct devsw* mydev = devsw + f->major;
    struct spinlock* dlk = &(mydev->lk);
    acquire(dlk);
    ret = mydev->write(1, addr, n);
    release(dlk);
  } else if(f->type == FD_ENTRY){
    elock(f->ep);
    if (ewrite(f->ep, 1, addr, f->off, n) == n) {
      ret = n;
      fileoff(f, n);
    } else {
      ret = -1;
    }
    eunlock(f->ep);
  } else {
    panic("filewrite");
  }
  return ret;
}

uint64
filesend(struct file* fin,struct file* fout,uint64 addr,uint64 n){
  uint64 off = 0;
  uint64 rlen = 0;
  uint64 wlen = 0;
  uint64 ret = 0;
  if(addr){
    if(either_copyin(1,&off,addr,sizeof(uint64))<0){
      __debug_warn("[filesend]obtain addr bad\n");
      return -1;
    }
  }
  if(fileillegal(fin)||fileillegal(fout)){
      __debug_warn("[filesend]fin/fout illegal\n");
      return -1;
  }
  //printf("[filesend]want send n:%p\n",n);
  //printf("[filesend]before send fout off:%p\n",fout->off);
  //print_f_info(fin);
  //print_f_info(fout);
  fileiolock(fin);
  fileiolock(fout);
  while(n){
    char buf[1024];
    rlen = MIN(n,sizeof(buf));
    rlen = fileinput(fin,0,(uint64)&buf,rlen,off);
    //printf("[filesend] send rlen %p\n",rlen);
    off += rlen;
    if(!addr)fileoff(fin,rlen);
    n -= rlen;
    if(!rlen){
      break;
    }
    if(rlen<0){
      fileiounlock(fout);
      fileiounlock(fin);
      return rlen;
    }
    wlen = fileoutput(fout,0,(uint64)&buf,rlen,fout->off);
    //printf("[filesend] send wlen:%p\n",wlen);
    fout->off += wlen;
    if(!addr)fileoff(fout,wlen);
    ret += wlen;
    //printf("[filesend]-----start-----\n");
    //printf("[filesend]%s\n",buf);
    //printf("[filesend]-----end-----\n");
  }
  fileiounlock(fout);
  fileiounlock(fin);
  //printf("[filesend]after send fout off:%p\n",fout->off);
  if(addr){
    if(either_copyout(1,addr,&off,sizeof(uint64))<0){
      __debug_warn("[filesend]obtain addr bad\n");
      return -1;
    }
  }
  //printf("[filesend]ret:%p\n",ret);
  return ret;
}

// Read from dir f.
// addr is a user virtual address.
int
dirnext(struct file *f, uint64 addr)
{
  // struct proc *p = myproc();

  if(f->readable == 0 || !(f->ep->attribute & ATTR_DIRECTORY))
    return -1;

  struct dirent de;
  struct stat st;
  int count = 0;
  int ret;
  elock(f->ep);
  while ((ret = enext(f->ep, &de, f->off, &count)) == 0) {  // skip empty entry
    f->off += count * 32;
  }
  eunlock(f->ep);
  if (ret == -1)
    return 0;

  f->off += count * 32;
  estat(&de, &st);
  // if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
  if(either_copyout(1, addr, (char *)&st, sizeof(st)) < 0)
    return -1;

  return 1;
}

struct file*
findfile(char* path)
{
  int dev;
  struct dirent* ep = ename(NULL,path,&dev);
  struct proc* p = myproc();
  if(ep == NULL)return NULL;
  elock(ep);
  for(int i = 0;i<NOFILEMAX(p);i++){
    if(p->ofile[i]->type==FD_ENTRY&&p->ofile[i]->ep==ep){
      eunlock(ep);
      eput(ep);
      return p->ofile[i];
    }
    if(p->ofile[i]->type==FD_DEVICE&&p->ofile[i]->major==dev){
      eunlock(ep);
      eput(ep);
      return p->ofile[i];
    }
  }
  eunlock(ep);
  eput(ep);
  return NULL;
}


int
dirent_next(struct file *f, uint64 addr, int n)
{
  if(f->readable == 0 || !(f->ep->attribute & ATTR_DIRECTORY))
    return -1;
  //printf("[dirent next]addr:%p n:%p\n",addr,n);
  struct dirent de;
  struct linux_dirent64 lde;
  int count = 0;
  int ret;
  int copysize = 0;
  elock(f->ep);
  while (1) {
    acquire(&f->lk);
    lde.d_off = f->off;
    ret = enext(f->ep, &de, f->off, &count);
    f->off += count * 32;
    release(&f->lk);
    // empty entry
    if(ret == 0) {
      continue;
    }
    // end of file, return 0
    if(ret == -1) {
      eunlock(f->ep);
      return copysize;
    }

    memcpy(lde.d_name, de.filename, sizeof(de.filename));
    lde.d_type = (de.attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    lde.d_ino = 0;
    // Size of this dent, varies from length of filename.
    int size = sizeof(struct linux_dirent64) - sizeof(lde.d_name) + strlen(lde.d_name) + 1;
    size += (sizeof(uint64) - (size % sizeof(uint64))) % sizeof(uint64); // Align to 8.
    lde.d_reclen = size;
    int realsize = lde.d_reclen;
    // buf size limits
    if(lde.d_reclen > n) {
      break;
    }

    // copy error, return -1
    if(either_copyout(1,addr, (char *)&lde, realsize) < 0){
      eunlock(f->ep);
      return -1;
    }
    
    addr += realsize;
    n -= realsize;
    copysize += realsize;
  }
  eunlock(f->ep);

  f->off += count * 32;
  return copysize; 
}

uint64 
filelseek(struct file *f, uint64 offset, int whence)
{
  uint64 ret = -1;
  switch (f->type)
  {
  case FD_ENTRY: 
    elock(f->ep);
    acquire(&f->lk);
    switch (whence)
    {
    case SEEK_SET:
      ret = f->off = offset;
      break;
    case SEEK_CUR:
      ret = (f->off += offset);
      break;
    case SEEK_END:
      ret = f->off = f->ep->file_size + offset;
      break;
    default:
      break;
    }
    release(&f->lk);
    eunlock(f->ep);
    break;
  case FD_PIPE:
    acquire(&f->pipe->lock);
    acquire(&f->lk);
    switch (whence)
    {
      case SEEK_SET:
        ret = f->off = offset;
        break;
      case SEEK_CUR:
        ret = (f->off += offset);
        break;
      default:
        break;
    }
    release(&f->lk);
    release(&f->pipe->lock);
    break;
  default:
    break;
  }

  return ret;
}

// uint64 
// sendfile(struct file *fout, struct file* fin, off_t *offset, uint64 count)
// {
  
// }
