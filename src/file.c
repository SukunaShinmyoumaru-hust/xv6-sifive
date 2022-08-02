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

struct devsw devsw[NDEV];
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
    if(copyout(p->pagetable, addr, (char *)&kst, sizeof(kst)) < 0)
    // if(copyout2(addr, (char *)&kst, sizeof(kst)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  switch (f->type) {
    case FD_PIPE:
        //r = piperead(f->pipe, addr, n);
        break;
    case FD_DEVICE:
        if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
          return -1;
        r = devsw[f->major].read(1, addr, n);
        break;
    case FD_ENTRY:
        if(f->ep == devnull)return 0;
        if(f->ep == devzero){
          return zero_out(addr,n);
        }
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

extern char consolewrite[];
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
    //ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_ENTRY){
    if(f->ep == devnull||f->ep == devzero)return 0;
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
  struct dirent* ep = ename(NULL,path);
  struct proc* p = myproc();
  if(ep == NULL)return NULL;
  elock(ep);
  for(int i = 0;i<NOFILEMAX(p);i++){
    if(p->ofile[i]->type==FD_ENTRY&&p->ofile[i]->ep==ep){
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

  struct dirent de;
  struct linux_dirent64 lde;
  int count = 0;
  int ret;
  int wsize = n;
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
      return 0;
    }

    memcpy(lde.d_name, de.filename, sizeof(de.filename));
    lde.d_type = (de.attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    lde.d_ino = 0;
    // Size of this dent, varies from length of filename.
    int size = sizeof(struct linux_dirent64) - sizeof(lde.d_name) + strlen(lde.d_name) + 1;
    size += (sizeof(uint64) - (size % sizeof(uint64))) % sizeof(uint64); // Align to 8.
    lde.d_reclen = size;

    // buf size limits
    if(lde.d_reclen > n) {
      break;
    }

    // copy error, return -1
    if(copyout2(addr, (char *)&lde, lde.d_reclen) < 0)
      return -1;
    
    addr += lde.d_reclen;
    n -= lde.d_reclen;
  }
  eunlock(f->ep);

  f->off += count * 32;

  return wsize - n; 
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
