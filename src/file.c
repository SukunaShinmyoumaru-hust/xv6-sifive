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
#include "include/vm.h"
#include "include/copy.h"

struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

extern int disk_init_flag;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
  disk_init_flag = 0;
  struct file *f;
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    memset(f, 0, sizeof(struct file));
  }
  #ifdef DEBUG
  printf("fileinit\n");
  #endif
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return NULL;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    //pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_ENTRY){
    eput(ff.ep);
  } else if (ff.type == FD_DEVICE) {

  }
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
        printf("[file]DEVICE name:%d\n",devsw[f->major].name);
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
        acquire(&f->pipe->lock);
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
        release(&f->pipe->lock);
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
    if(copyout2(addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filekstat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
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
  if(copyout(p->pagetable, addr, (char *)&kst, sizeof(kst)) < 0)
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

  switch (f->type) {
    case FD_PIPE:
        acquire(&f->pipe->lock);
        r = piperead(f->pipe, 1, addr, n);
        release(&f->pipe->lock);
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
          f->off += r;
        eunlock(f->ep);
        break;
    default:
      panic("fileread");
  }
  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int ret = 0;
  //printf("major:%d off:%p\n",f->major,consolewrite-(char*)(devsw[f->major].write));
  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE){
    acquire(&f->pipe->lock);
    ret = pipewrite(f->pipe, 1, addr, n);
    release(&f->pipe->lock);
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
      f->off += n;
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
  }else{
    off = fin->off;
  }
  if(fileillegal(fin)||fileillegal(fout)){
      __debug_warn("[filesend]fin/fout illegal\n");
      return -1;
  }
  //printf("[filesend]want send n:%p\n",n);
  //printf("[filesend]before send fout off:%p\n",fout->off);
  print_f_info(fin);
  print_f_info(fout);
  fileiolock(fin);
  fileiolock(fout);
  while(n){
    char buf[512];
    rlen = MIN(n,512);
    rlen = fileinput(fin,0,(uint64)&buf,rlen,off);
    printf("[filesend] send rlen %p\n",rlen);
    off += rlen;
    n -= rlen;
    if(!rlen){
      break;
    }
    wlen = fileoutput(fout,0,(uint64)&buf,rlen,fout->off);
    printf("[filesend] send wlen:%p\n",rlen,wlen);
    fout->off += wlen;
    ret += wlen;
  }
  fileiounlock(fout);
  fileiounlock(fin);
  //printf("[filesend]after send fout off:%p\n",fout->off);
  if(addr){
    if(either_copyout(1,addr,&off,sizeof(uint64))<0){
      __debug_warn("[filesend]obtain addr bad\n");
      return -1;
    }
  }else{
    fin->off = off;
  }
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
  if(copyout2(addr, (char *)&st, sizeof(st)) < 0)
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
    lde.d_off = f->off;
    ret = enext(f->ep, &de, f->off, &count);
    f->off += count * 32;
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
    eunlock(f->ep);
    break;
  case FD_PIPE:
    acquire(&f->pipe->lock);
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
