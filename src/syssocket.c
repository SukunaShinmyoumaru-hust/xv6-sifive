#include "include/types.h"
#include "include/printf.h"
#include "include/fcntl.h"
#include "include/dev.h"
#include "include/syscall.h"
#include "include/mmap.h"
#include "include/copy.h"
#include "include/pipe.h"
#include "include/errno.h"

uint64
sys_socket(void)
{
  int fd;
  int domain;
  int type;
  int protocol;
  argint(0, &domain);
  argint(1, &type);
  argint(2, &protocol);
  //printf("SOCKET domain:%p type:%p protocol:%p\n", domain, type, protocol);
  struct file* f = NULL;
  struct socket* sk = NULL;
  if((f=filealloc())==NULL||(fd=fdalloc(f))<0){
    goto bad;
  }
  f->type = FD_SOCKET;
  sk = socketalloc();
  f->sk = sk;
  sk->domain = domain;
  sk->type = type;
  sk->protocol = protocol;
  printf("create socket:%d\n",fd);
  return fd;
bad:
  if(!f)fileclose(f); 
  if(!sk)socketclose(sk); 
  return -1;
}

uint64
sys_setsockopt(void)
{
  int sockfd;
  int level;
  int optname;
  uint64 optvaladdr;
  socklen_t optlen;
  struct file* f = NULL;
  if(argfd(0, &sockfd, &f)<0){
    return -1;
  }
  argint(1, &level);
  argint(2, &optname);
  argaddr(3, &optvaladdr);
  argint(4, &optlen);
  //struct socket* sk = f->sk;
  printf("[setsockopt]sockfd:%d level:%d optname:%d optaddr:%p optlen:%p\n",sockfd, level,optname, optvaladdr,optlen);
  return 0;
}

uint64
sys_bind(void)
{
  int sockfd;
  struct file* f;
  struct sockaddr* addr;
  socklen_t addrlen;
  if(argfd(0, &sockfd, &f)<0){
    return -1;
  }
  struct socket* sk = f->sk;
  if(argint(2, &addrlen)<0){
    return -1;
  }
  addr = kmalloc(addrlen*sizeof(struct sockaddr));
  printf("sockfd:%d addrlen:%p\n", sockfd, addrlen);
  if(argstruct(1, addr, addrlen*sizeof(struct sockaddr))==0){
    return -1;
  }
  for(int i = 0;i<addrlen;i++){
    print_sockaddr(addr+i);
    socketbind(sk, addr+i);
  }
  kfree(addr);
  return 0;
}

uint64
sys_listen(void)
{
  int sockfd;
  int backlog;
  struct file* f;
  if(argfd(0, &sockfd, &f)<0){
    return -1;
  }
  argint(1, &backlog);
  printf("listen sockfd:%d backlog:%d\n",sockfd,backlog);
  return 0;
}
