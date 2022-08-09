#include"include/fat32.h"
#include"include/param.h"
#include"include/string.h"
#include"include/printf.h"
#include"include/copy.h"
#include"include/dev.h"
#include"include/sbi.h"
#include"include/riscv.h"

struct dirent* dev;
int devnum;


#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

struct devsw devsw[NDEV];

extern char sacrifice_start[];
extern uint64 sacrifice_size;
extern char localtime[];
extern uint64 localtime_size;

int devinit()
{
  devnum = 0;
  dev = create(NULL,"/dev",T_DIR,0);
  eunlock(dev);
  struct dirent* ep;
  ep = create(NULL,"/etc/passwd", T_FILE, 0);
  eunlock(ep);
  eput(ep);
  ep = create(NULL,"/etc/localtime", T_FILE, 0);
  ewrite(ep, 0, (uint64)localtime, 0, localtime_size);
  eunlock(ep);
  eput(ep);
  ep = create(NULL,"/mytest.sh",T_FILE,0);
  ewrite(ep, 0, (uint64)sacrifice_start, 0, sacrifice_size);
  eunlock(ep);
  eput(ep);
  __debug_info("devinit\n");
  memset(devsw,0,NDEV*sizeof(struct devsw));
  allocdev("console",consoleread,consolewrite);
  allocdev("null",nullread,nullwrite);
  allocdev("zero",zeroread,zerowrite);
  return 0;
}

int getdevnum(){
  return devnum;
}

int
allocdev(char* name,int (*devread)(int, uint64, int),int (*devwrite)(int, uint64, int)){
  if(devnum == NDEV){
    __debug_warn("[dev]no more space for device\n");
    return -1;
  }
  strncpy(devsw[devnum].name,name,DEV_NAME_MAX+1);
  initlock(&devsw[devnum].lk,name);
  devsw[devnum].read = devread;
  devsw[devnum].write = devwrite;
  devnum++;
  return 0;
}

int 
devlookup(char *name)
{
  for(int i = 0;i < NDEV;i++){
    if(strncmp(name,devsw[i].name,DEV_NAME_MAX+1)==0){
      return i;
    }
  }
  return 0;
}

int
nullread(int user_dst,uint64 addr,int n){
  return 0;
}

int
nullwrite(int user_dst,uint64 addr,int n){
  return n;
}

int
zeroread(int user_dst,uint64 addr,int n){
  if(user_dst)return zero_out(addr,n);
  else{
    memset((void*)addr,0,n);
    return n;
  }
}

int
zerowrite(int user_dst,uint64 addr,int n){
  return n;
}

int
consoleread(int user_dst,uint64 addr,int n){
  char readbuf[CONSOLE_BUF_LEN];
  int ret = 0;
  int interp = 0;
  while(n&&!interp){
    int len = MIN(n,CONSOLE_BUF_LEN);
    int i;
    for(i=0;i<len;i++){
      char c = 0;
      while((c=sbi_console_getchar())==255);
      c = c==13?10:c;
      readbuf[i] = c;
      consputc(c);
      if(c == 10){
        interp = 1;
        break;
      }
    }
    if(either_copyout(user_dst,addr,readbuf,i)<0){
      return ret;
    }
    n -= i;
    ret += i;
    addr += i;
  }
  return ret;
}

int
consolewrite(int user_dst,uint64 addr,int n){
  char writebuf[CONSOLE_BUF_LEN];
  int ret = 0;
  while(n){
    int len = MIN(n,CONSOLE_BUF_LEN);
    if(either_copyin(user_dst,writebuf,addr,len)<0){
      return ret;
    }
    for(int i=0;i<len;i++){
      consputc(writebuf[i]);
    }
    n -= len;
    ret += len;
    addr += len;
  }
  return ret;
}

int 
devkstat(struct devsw* mydev, struct kstat* st){
    st->st_dev = mydev-devsw;
    st->st_size = 0;
    st->st_blksize = 0;
    st->st_blocks = 0;
    st->st_atime_nsec = 0;
    st->st_atime_sec = 0;
    st->st_ctime_nsec = 0;
    st->st_ctime_sec = 0;
    st->st_mtime_nsec = 0;
    st->st_mtime_sec = 0;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_nlink = 1;
    st->st_ino = 0;
    st->st_mode = 0;
  return 0;
}

