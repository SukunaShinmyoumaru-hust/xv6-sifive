void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // printf("[start]-----pid:%d syscall %d:%s-----\n",myproc()->pid,num,sysnames[num]);
    p->trapframe->a0 = syscalls[num]();
    // printf("[end]----pid:%d syscall %d:%s return %p-----\n",myproc()->pid,num,sysnames[num],p->trapframe->a0);
        // trace
    if ((p->tmask & (1 << num)) != 0) {
      printf("pid %d: %s -> %d\n", p->pid, sysnames[num], p->trapframe->a0);
    }
  } else {
    printf("pid %d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
