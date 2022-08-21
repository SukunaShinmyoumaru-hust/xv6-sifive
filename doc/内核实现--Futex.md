# Futex机制的理解与实现
## Futex应用场景
> Futex是一种用户态和内核态混合的同步机制。首先，同步的进程间通过mmap共享一段内存，futex变量就位于这段共享的内存中且操作是原子的，当进程尝试进入互斥区或者退出互斥区的时候，先去查看共享内存中的futex变量，如果没有竞争发生，则只修改futex，而不用再执行系统调用了。当通过访问futex变量告诉进程有竞争发生，则还是得执行系统调用去完成相应的处理(wait 或者 wake up)。简单的说，futex就是通过在用户态的检查，（motivation）如果了解到没有竞争就不用陷入内核了，大大提高了low-contention时候的效率。

## Futex系统调用
我们已经知道当在用户态检查到有竞争发生时，就会陷入内核态，陷入内核态是通过调用futex系统调用完成的。其函数原型如下：
```c
long 
futex(uint32_t *uaddr, int futex_op, uint32_t val,
                 const struct timespec *timeout,
                 uint32_t *uaddr2, uint32_t val3);
```
其中futex_op指定需要执行的操作类型。

## Futex具体实现
在我们的系统中我们主要对三种类型的操作进行了处理：

|操作类型|说明|
|:---:|---|
|FUTEX_WAIT|在某锁变量满足条件时，在某锁变量上挂起等待|
|FUTEX_WAKE|唤醒若干个在某锁变量上挂起的等待进程|
|FUTEX_REQUEUE|与FUTEX_WAIT类类似，不仅需要唤醒若干个等待某锁变量的进程，还需要将另外若干个等待该锁变量的进程移到另一个等待队列中|


### 与FUTEX_WAIT相关的函数原型
```c
static int 
futex_wait(int *uaddr, unsigned int flags, int val,
		      ktime_t *abs_time, int bitset);
```
读取用户空间中uaddr所指向的值，如果不等于val，则返回-1表示不是原子操作；如果等于val则表示该线程需要等待uaddr所指锁变量，调用sleep函数将其挂起。

### 与FUTEX_WAKE相关的函数原型
```c
static int
futex_wake(int *uaddr, unsigned int flags, int nr_wake, uint32 bitset);
```
唤醒最多nr_wake个等待uaddr指向的锁变量的线程。

### 与FUTEX_REQUEUE相关的函数原型
```c
static int
futex_requeue(int *uaddr1, uint32 flags, int *uaddr2, int nr_wake, int nr_requeue);
```
唤醒最多nr_wake个等待uaddr1指向的锁变量的线程，将其余最多nr_requeue个等待uaddr1的线程放到等待uaddr2指向的变量的队列上。
