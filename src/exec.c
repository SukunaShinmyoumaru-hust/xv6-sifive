#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"
#include"include/proc.h"
#include"include/elf.h"
#include"include/vma.h"
#include"include/vm.h"
#include"include/pm.h"
#include"include/kalloc.h"
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
      uint64 load_start = ph.vaddr+base;
      uint64 load_end = ph.vaddr+ph.memsz+base;
      if(alloc_load_vma(p, load_start, load_end, PTE_R|PTE_W|PTE_X|PTE_U)== NULL){
        __debug_warn("[exec]grow space failed\n");
        return -1;
      }
      if(loadseg(pagetable, load_start, ep, ph.off, ph.filesz) < 0){
        printf("[exec]load segment failed\n");
        return -1;
      }
    }else if(ph.type==ELF_PROG_PHDR){
      if(phdr){
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


static uint64 
ustackpushstr(pagetable_t pagetable,uint64* ustack,char* str,uint64 sp,uint64 stackbase)
{
  uint64 argc = ++ustack[0];
  if(argc>MAXARG+1)return -1;
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
  

int
exec(char *path, char **argv, char **env)
{
  printf("[exec]path:%s\n",path);
  int i;
  i = 0;
  while(argv[i]){
    printf("[exec]argv[%d] = %s\n",i,argv[i]);
    i++;
  }
  i = 0;
  while(env[i]){
    printf("[exec]env[%d] = %s\n",i,env[i]);
    i++;
  }
  uint64 sp,entry;
  struct proc* p = myproc();
  struct proc* np = kmalloc(sizeof(struct proc));
  struct elfhdr elf;
  struct dirent *ep;
  np->trapframe = allocpage();
  memcpy(np->trapframe,p->trapframe,sizeof(struct trapframe));

  if ((proc_pagetable(np)) == NULL) {
    goto bad;
  }

  
  if((ep = ename(NULL,path,0)) == NULL) {
    __debug_warn("[exec] %s not found\n", path);
    goto bad;
  }

  //printf("[exec]exec %s\n",path);
  elock(ep);
  // Check ELF header
  if(readelfhdr(ep,&elf)<0){
    goto bad;
  }
  struct proghdr phdr;
  entry = loadelf(np,ep,&elf,&phdr,0);
  if(entry==-1){
    eunlock(ep);
    goto bad;
  }
  eunlock(ep);
  print_vma_info(p);
  print_vma_info(np);
  sp = type_locate_vma(np->vma,STACK)->end;
  np->trapframe->sp = sp;
  np->trapframe->epc = entry;
  swap(&(p->pagetable),&(np->pagetable),sizeof(p->pagetable));
  swap(&(p->vma),&(np->vma),sizeof(p->vma));
  swap(&(p->trapframe),&(np->trapframe),sizeof(p->trapframe));
  //uvmfree(np);
  w_satp(MAKE_SATP(p->pagetable));
  
  sfence_vma();
  return 0;
bad:
  uvmfree(np);
  __debug_warn("[exec]exec bad\n");
  return -1;
}
