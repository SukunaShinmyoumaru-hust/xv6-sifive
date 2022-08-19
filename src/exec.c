#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"
#include"include/proc.h"
#include"include/elf.h"
#include"include/vma.h"
#include"include/vm.h"
#include"include/pm.h"
#include"include/kalloc.h"
#include"include/file.h"
#include"include/string.h"
#define SELF_LOAD 

//read and check elf header
//return 0 on success,-1 on failure
static int
readelfhdr(struct dirent* ep,struct elfhdr* elf){
  // Check ELF header
  if(eread(ep, 0, (uint64)elf, 0, sizeof(struct elfhdr)) != sizeof(struct elfhdr))
    return -1;
  if(elf->magic != ELF_MAGIC)
    return -1;
  return 0;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, struct dirent *ep, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;
  i = 0;
  if((va % PGSIZE) != 0){
    uint64 off = va % PGSIZE;
    uint64 rest = PGSIZE -off;
    pa = walkaddr(pagetable, va)+off;
    if(pa == NULL)
      panic("loadseg: address should exist");
    if(sz - i < rest)
      n = sz - i;
    else
      n = rest;
    //printf("map zone:[%p,%p]\n",va,va+n);
    if(eread(ep, 0, (uint64)pa, offset+i, n) != n)
      return -1;
    i += n;
  }
  
  for(; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == NULL)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(eread(ep, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }

  return 0;
}


// Load a program elf into pagetable
// Returns sz on success, -1 on failure.
// ep must be locked.
static uint64
loadelf(struct proc* p, struct dirent *ep,struct elfhdr* elf,struct proghdr* phdr,uint64 base)
{
  struct proghdr ph;
  int getphdr = 0;
  //struct elfhdr  linkelf;
  pagetable_t pagetable = p->pagetable;
  for(int i=0, off=elf->phoff; i<elf->phnum; i++, off+=sizeof(ph)){
    if(eread(ep, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      return -1;
    if(ph.type == ELF_PROG_LOAD){
      if(ph.memsz < ph.filesz){
        __debug_warn("[exec]load memsz>filesz\n");
        return -1;
      }
      if(ph.vaddr + ph.memsz < ph.vaddr){
        __debug_warn("[exec]memsz not positive\n");
        return -1;
      }
      if(!getphdr&&phdr&&ph.off == 0){ 
        phdr->vaddr = elf->phoff + ph.vaddr;
      }
      uint64 load_start = ph.vaddr+base;
      if(alloc_load_vma(p, load_start, ph.memsz, PTE_R|PTE_W|PTE_X|PTE_U)== NULL){
        __debug_warn("[exec]grow space failed\n");
        return -1;
      }
      if(loadseg(pagetable, load_start, ep, ph.off, ph.filesz) < 0){
        printf("[exec]load segment failed\n");
        return -1;
      }
    }else if(ph.type==ELF_PROG_PHDR){
      if(phdr){
        getphdr = 1;
        *phdr = ph;
      }
    }else if(ph.type==ELF_PROG_INTERP){
      __debug_warn("[exec]dynamic load not success\n");
    }
  }
  return elf->entry;
}

void swap(void* a,void* b,int len){
  char c;
  char *sa = a,*sb = b;
  for(int i = 0;i<len;i++){
    c = sa[i];
    sa[i] = sb[i];
    sb[i] = c;
  }
}

void
auxalloc(uint64* aux,uint64 atid,uint64 value)
{
  //printf("aux[%d] = %p\n",atid,value);
  uint64 argc = aux[0];
  aux[argc*2+1] = atid;
  aux[argc*2+2] = value;
  aux[argc*2+3] = 0;
  aux[argc*2+4] = 0;
  aux[0]++;
}

uint64
loadaux(pagetable_t pagetable,uint64 sp,uint64 stackbase,uint64* aux){
  int argc = aux[0];
  if(!argc)return sp;
  /*
  printf("aux argc:%d\n",argc);
  for(int i=1;i<=2*argc+2;i++){
    printf("final raw aux[%d] = %p\n",i,aux[i]);
  }
  */
  sp -= (2*argc+2) * sizeof(uint64);
  if(sp < stackbase){
    return -1;
  }
  aux[0] = 0;
  if(copyout(pagetable, sp, (char *)(aux+1), (2*argc+2)*sizeof(uint64)) < 0){
    return -1;
  }
  return sp;
}

uint64 
ustackpushstr(pagetable_t pagetable,uint64* ustack,char* str,uint64 sp,uint64 stackbase)
{
  uint64 argc = ++ustack[0];
  if(argc>MAXARG+1)return -1;
  //printf("[exec]push %s\n",str);
  sp -= strlen(str) + 1;
  sp -= sp % 16; // riscv sp must be 16-byte aligned
  if(sp < stackbase)
      return -1;
  if(copyout(pagetable, sp, str, strlen(str) + 1) < 0)
      return -1;
  ustack[argc] = sp;
  ustack[argc+1] = 0;
  return sp;
}

void
stackdisplay(pagetable_t pagetable,uint64 sp,uint64 sz)
{
  for(uint64 i = sp;i<sz;i+=8){
    uint64 *pa = (void*)kwalkaddr1(pagetable,i);
    if(pa)printf("addr %p value %p\n",i,*pa);
    else printf("addr %p value (nil)\n",i);
  }
}

void zerocheck(pagetable_t pagetable, uint64 va,int len){
  printf("[exec] check %p\n",va);
  char* pa = (void*)kwalkaddr1(pagetable,va);
  if(pa){
    printf("[exec] set zero at va:%p pa:%p\n",va,pa);
    for(int i = 0;i*8<len;i++){
      *(pa+i) = 0;
    }
  }
  
}
  
static void
print_sp(uint64 sp)
{
  if(sp%16)
    __debug_info("[exec] sp = %p sp%16=%d\n",sp, sp%16);
}

int
exec(char *path, char **argv, char **env)
{
  int shflag = 0;
  uint64 sp,stackbase,entry;
  uint64 argc,envnum;
  uint64 aux[AUX_CNT*2+3] = {0,0,0};
  uint64 environ[10]={0};
  uint64 ustack[MAXARG+2];
  char *last,*s;
  struct proc* p = myproc();
  struct proc* np = kmalloc(sizeof(struct proc));
  struct elfhdr elf;
  struct dirent *ep;
  np->trapframe = allocpage();
  memcpy(np->trapframe,p->trapframe,sizeof(struct trapframe));
 
  /*
  __debug_warn("[exec] exec %s\n",path);
  for(int aaa = 0;argv[aaa];aaa++){
    __debug_warn("[exec] exec argv[%d] %s\n",aaa,argv[aaa]);
  }
  */
  /*
  if(strncmp(path,"/bin/sh",10)==0){
    strncpy(path,"/busybox",10);
  }
  */
  
  
  if(strncmp(path,argv[0],0x100)!=0){
    strncpy(argv[0],path,0x100);
  }
  
  if ((proc_pagetable(np, 0, 0)) == NULL) {
    __debug_warn("[exec]vma init bad\n");
    goto bad;
  }

  if((ep = ename(NULL,path,0)) == NULL) {
    __debug_warn("[exec] %s not found\n", path);
    goto bad;
  }
  
  elock(ep);
  int inreload = 0;
reload:
  // Check ELF header
  if(readelfhdr(ep,&elf)<0){    
    shflag = 1;
    eunlock(ep);
    eput(ep);
    if((ep = ename(NULL,"/busybox",0))==NULL){
      __debug_warn("[exec] %s not found\n", path);
      goto bad;
    }
    elock(ep);
    inreload++;
    if(inreload==1)goto reload;
    else{
      __debug_warn("[exec] reload many times\n");
    }
  }
  
  
  struct proghdr phdr = {0};
  entry = loadelf(np,ep,&elf,&phdr,0);
  if(entry==-1){
    eunlock(ep);
    __debug_warn("[exec]load elf bad\n");
    goto bad;
  }
  eunlock(ep);
  eput(ep);
  //print_vma_info(p);
  //print_vma_info(np);
  struct vma* stack_vma = type_locate_vma(np->vma,STACK);
  sp = stack_vma->end;
  stackbase = stack_vma->addr;
  ustack[0] = environ[0] =0;
  if((sp = ustackpushstr(np->pagetable,environ,"LD_LIBRARY_PATH=/",sp,stackbase))==-1){
      goto bad;
  }
  
  if(entry!=elf.entry){
    //("elf.entry:%p\n",elf.entry);
    //printf("progentry:%p\n",progentry);
#ifndef SELFLOAD   
    if((sp = ustackpushstr(np->pagetable,ustack,path,sp,stackbase))==-1){
      goto bad;
    }
#endif
  }
  uint64 random[2] = { 0xcde142a16cb93072, 0x128a39c127d8bbf2 };
  sp -= 16;
  if (sp < stackbase || copyout(np->pagetable, sp, (char *)random, 16) < 0) {
    __debug_warn("[exec] random copy bad\n");
    goto bad;
  }
  
  //auxalloc(aux,AT_HWCAP, 0x112d);
  auxalloc(aux,AT_PAGESZ,PGSIZE);
  auxalloc(aux,AT_PHDR, phdr.vaddr);
  auxalloc(aux,AT_PHENT, elf.phentsize);
  auxalloc(aux,AT_PHNUM, elf.phnum);
  auxalloc(aux,AT_UID, 0);
  auxalloc(aux,AT_EUID, 0);
  auxalloc(aux,AT_GID, 0);
  auxalloc(aux,AT_EGID, 0);
  auxalloc(aux,AT_SECURE, 0);
  auxalloc(aux,AT_EGID, 0);		
  auxalloc(aux,AT_RANDOM, sp);

  //printf("[exec]push argv\n");
  // Push argument strings, prepare rest of stack in ustack.
  
  if(shflag){
    if((sp = ustackpushstr(np->pagetable,ustack,"sh",sp,stackbase))==-1){
      __debug_warn("[exec]push argv string bad\n");
      goto bad;
    }
    if((sp = ustackpushstr(np->pagetable,environ,"PATH=/",sp,stackbase))==-1){
      __debug_warn("[exec]push env string bad\n");
      goto bad;
    }
  }
  for(argc = 0; argv[argc]; argc++) {
    if((sp = ustackpushstr(np->pagetable,ustack,argv[argc],sp,stackbase))==-1){
      __debug_warn("[exec]push argv string bad\n");
      goto bad;
    }
  }
  //printf("[exec]push env\n");
  for(envnum = 0; env[envnum]; envnum++) {
    if((sp = ustackpushstr(np->pagetable,environ,env[envnum],sp,stackbase))==-1){
      __debug_warn("[exec]push env string bad\n");
      goto bad;
    }
  }
  //printf("[exec]push end\n");
  if((environ[0]+ustack[0]+1)%2){sp -= 8;}//16 aligned
  
  //load aux
  if((sp = loadaux(np->pagetable,sp,stackbase,aux))<0){
    __debug_warn("[exec]pass aux too many\n");
    goto bad;
  }
  
  argc = environ[0];
  if(argc){
    // push the array of argv[] pointers.
    sp -= (argc+1) * sizeof(uint64);
    if(sp < stackbase){
      __debug_warn("[exec]env address vec too long\n");
      goto bad;
    }
    if(copyout(np->pagetable, sp, (char *)(environ+1), (argc+1)*sizeof(uint64)) < 0){
      __debug_warn("[exec]env address copy bad\n");
      goto bad;
    }
  }
  
  argc = ustack[0];
  //printf("[exec]argc:%d\n",argc);
  // push the array of argv[] pointers.
  sp -= (argc+2) * sizeof(uint64);
  
  if(sp < stackbase){
    __debug_warn("[exec]ustack address vec too long\n");
    goto bad;
  }

  if(copyout(np->pagetable, sp, (char *)ustack, (argc+2)*sizeof(uint64)) < 0){
    __debug_warn("[exec]ustack address copy bad\n");
    goto bad;
  }
  //stackdisplay(pagetable,sp,sz);
  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  //np->trapframe->a0 = argc;
  np->trapframe->a0 = 0;
  np->trapframe->a1 = sp+8;
  
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  strncpy(p->name, last, sizeof(p->name));
  
  np->trapframe->sp = sp;
  np->trapframe->epc = entry;
  swap(&(p->pagetable),&(np->pagetable),sizeof(p->pagetable));
  swap(&(p->vma),&(np->vma),sizeof(p->vma));
  swap(&(p->trapframe),&(np->trapframe),sizeof(p->trapframe));
  for(int fd = 0; fd < NOFILEMAX(p); fd++){
    struct file* f = p->ofile[fd];
    if(f&&p->exec_close[fd]){
      //__debug_warn("[exec]close %d\n",fd);
      fileclose(f);
      p->ofile[fd] = 0;
      p->exec_close[fd]=0;
    }
  }
  w_satp(MAKE_SATP(p->pagetable));
  
  print_sp(sp);
  
  sfence_vma();
  //printf("[exec]argc:%d a0:%p\n",argc,p->trapframe->a0);
  uvmfree(np);
  return argc;
bad:
  uvmfree(np);
  __debug_warn("[exec]exec bad\n");
  return -1;
}
